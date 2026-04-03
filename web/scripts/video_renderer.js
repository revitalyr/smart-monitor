// video_renderer.js
// Canvas-визуализация состояния ребёнка.
// Запускается автоматически при инициализации (не требует нажатия Start).
// Все визуальные параметры коррелируют с физической моделью из data_agent.c:
//   - Яркость сцены ← light_lux + nightlight
//   - Амплитуда движений фигурки ← motion_prob (опережает аудио ~0.5 с)
//   - Пульсация контура ← noise_level (с задержкой ~1 с)
//   - Рамка детектора ← motion_detected / audio_triggered

// ── Константы ────────────────────────────────────────────────────────────────

const W = 640;
const H = 480;

// Цвета зон детекции
const COLOR_MOTION  = '#00ff88';   // видеодетектор сработал
const COLOR_AUDIO   = '#ff9900';   // аудиодетектор сработал
const COLOR_BOTH    = '#ff4444';   // оба сработали
const COLOR_IDLE    = '#334455';   // тихо

// Подписи состояния
const STATE_LABELS = {
    'SLEEPING_PEACEFUL': { text: 'SLEEPING',     color: '#44bb88' },
    'RESTLESS':          { text: 'RESTLESS',      color: '#aabb44' },
    'STIRRING':          { text: 'STIRRING',      color: '#ddaa22' },
    'FUSSY':             { text: 'FUSSY',          color: '#ff8800' },
    'CRYING_SOFT':       { text: 'CRYING (soft)', color: '#ff6622' },
    'CRYING_LOUD':       { text: 'CRYING (loud)', color: '#ff2200' },
    'SCREAMING':         { text: 'SCREAMING',     color: '#ff0055' },
    'FALLING_ASLEEP':    { text: 'CALMING DOWN',  color: '#66bbaa' },
};

// ── Состояние рендерера ───────────────────────────────────────────────────────

let _canvas = null;
let _ctx    = null;
let _rafId  = null;
let _pollId = null;

// Данные с сервера (обновляются через /video/status каждые 500 мс)
let _videoData = {
    baby_state:      'SLEEPING_PEACEFUL',
    motion_prob:     0,
    noise_level:     0.02,
    motion_detected: false,
    audio_triggered: false,
    nightlight:      false,
    light_lux:       3,
    room_temp:       22.0,
    body_temp:       36.6,
    humidity:        45.0,
};

// Интерполированные значения для плавной анимации (обновляются в каждом кадре)
let _smooth = {
    motionAmp:  0,   // амплитуда покачивания фигурки [0..1]
    noiseGlow:  0,   // свечение контура [0..1]
    brightness: 0.1, // общая яркость сцены [0..1]
};

// Счётчик кадров для анимаций
let _frame = 0;

// ── Публичный API ─────────────────────────────────────────────────────────────

/**
 * Инициализирует и автоматически запускает рендерер.
 * @param {HTMLCanvasElement} canvas
 */
export function initVideoRenderer(canvas) {
    _canvas = canvas;
    _ctx    = canvas.getContext('2d');
    _canvas.width  = W;
    _canvas.height = H;

    _startPolling();
    _startRenderLoop();
}

export function destroyVideoRenderer() {
    if (_rafId)  cancelAnimationFrame(_rafId);
    if (_pollId) clearInterval(_pollId);
    _rafId = _pollId = null;
}

// ── Опрос сервера ─────────────────────────────────────────────────────────────

function _startPolling() {
    // Немедленный первый запрос
    _fetchVideoStatus();
    // Затем каждые 500 мс
    _pollId = setInterval(_fetchVideoStatus, 500);
}

function _fetchVideoStatus() {
    fetch('/video/status')
        .then(r => r.json())
        .then(data => { _videoData = data; })
        .catch(() => { /* сервер недоступен — продолжаем с последними данными */ });
}

// ── Render loop ───────────────────────────────────────────────────────────────

function _startRenderLoop() {
    function loop() {
        _updateSmooth();
        _drawFrame();
        _frame++;
        _rafId = requestAnimationFrame(loop);
    }
    _rafId = requestAnimationFrame(loop);
}

// LP-фильтр первого порядка (зеркало lp_filter из data_agent.c)
function lp(current, target, alpha) {
    return current + alpha * (target - current);
}

function _updateSmooth() {
    const mp = _videoData.motion_prob  || 0;
    const nl = _videoData.noise_level  || 0;

    // motion_prob нарастает быстро (α=0.12, ~0.5 с при 60 fps),
    // спадает медленно (α=0.02), — зеркало τ из C-кода
    const alphaMotUp  = 0.12;
    const alphaMotDn  = 0.02;
    const alphaNoiseUp = 0.06;   // медленнее — ~1 с задержки
    const alphaNoiseDn = 0.015;
    const alphaBright  = 0.05;

    _smooth.motionAmp = lp(
        _smooth.motionAmp, mp,
        mp > _smooth.motionAmp ? alphaMotUp : alphaMotDn
    );
    _smooth.noiseGlow = lp(
        _smooth.noiseGlow, nl,
        nl > _smooth.noiseGlow ? alphaNoiseUp : alphaNoiseDn
    );

    // Яркость: ночник включён → 0.75, темно → 0.08
    const targetBright = _videoData.nightlight ? 0.75 : 0.08 + (_videoData.light_lux || 3) / 60;
    _smooth.brightness = lp(_smooth.brightness, targetBright, alphaBright);
}

// ── Отрисовка ─────────────────────────────────────────────────────────────────

function _drawFrame() {
    const ctx = _ctx;
    const t   = _frame;

    // 1. Фон — тёмная комната, яркость от ночника/light_lux
    const b = Math.min(1, _smooth.brightness);
    const bgL = Math.round(b * 38);
    ctx.fillStyle = `rgb(${bgL},${Math.round(bgL * 1.1)},${Math.round(bgL * 1.3)})`;
    ctx.fillRect(0, 0, W, H);

    // 2. Контуры кроватки
    _drawCrib(ctx, b);

    // 3. Фигурка ребёнка с движением
    _drawBaby(ctx, b, t);

    // 4. Зона детекции движения (overlay рамка)
    _drawDetectionOverlay(ctx, t);

    // 5. Оверлей: шумомер + уровень освещения
    _drawMetricsOverlay(ctx);

    // 6. HUD — состояние, температура, влажность
    _drawHUD(ctx, t);

    // 7. Имитация «зернистости» ночного видения
    _drawNoise(ctx, b);
}

function _drawCrib(ctx, brightness) {
    const bv = Math.min(255, Math.round(80 + brightness * 120));
    ctx.strokeStyle = `rgb(${bv},${bv},${Math.round(bv * 0.7)})`;
    ctx.lineWidth = 3;

    // Боковые стойки
    ctx.beginPath();
    ctx.moveTo(120, 100); ctx.lineTo(120, 380);
    ctx.moveTo(520, 100); ctx.lineTo(520, 380);
    ctx.stroke();

    // Горизонтальные перекладины
    for (let y = 120; y <= 360; y += 60) {
        ctx.beginPath();
        ctx.moveTo(120, y); ctx.lineTo(520, y);
        ctx.globalAlpha = 0.3;
        ctx.stroke();
        ctx.globalAlpha = 1.0;
    }

    // Основание
    ctx.lineWidth = 5;
    ctx.beginPath();
    ctx.moveTo(120, 380); ctx.lineTo(520, 380);
    ctx.stroke();
}

function _drawBaby(ctx, brightness, t) {
    const amp  = _smooth.motionAmp;
    const bv   = Math.min(255, Math.round(160 + brightness * 80));

    // Покачивание: нарастает вместе с motion_prob (опережает аудио)
    const wobble = amp * 8 * Math.sin(t * 0.18);
    const breathe = 2 * Math.sin(t * 0.04);  // медленное дыхание всегда

    const cx = W / 2 + wobble;
    const cy = 260 + breathe;

    // Одеяло
    const blanketColor = `rgba(${bv},${Math.round(bv * 0.85)},${Math.round(bv * 0.6)}, 0.9)`;
    ctx.fillStyle = blanketColor;
    ctx.beginPath();
    ctx.ellipse(cx, cy + 30, 110 + amp * 15, 55, wobble * 0.015, 0, Math.PI * 2);
    ctx.fill();

    // Голова
    const headJitter = amp * 3 * Math.sin(t * 0.22 + 1.5);
    ctx.fillStyle = `rgb(${Math.round(bv * 1.1)}, ${Math.round(bv * 0.9)}, ${Math.round(bv * 0.75)})`;
    ctx.beginPath();
    ctx.ellipse(cx + headJitter, cy - 38, 32, 30, 0, 0, Math.PI * 2);
    ctx.fill();

    // Лицо: цвет глаз зависит от состояния
    _drawBabyFace(ctx, cx + headJitter, cy - 38, amp, bv, t);

    // Контур свечения от уровня шума (аудио, нарастает с задержкой ~1 с)
    if (_smooth.noiseGlow > 0.05) {
        const glow = _smooth.noiseGlow;
        ctx.save();
        ctx.globalAlpha = glow * 0.6;
        ctx.strokeStyle = _smooth.noiseGlow > 0.5 ? '#ff4400' : '#ffaa00';
        ctx.lineWidth = 3 + glow * 6;
        ctx.shadowColor = ctx.strokeStyle;
        ctx.shadowBlur  = 12 * glow;
        ctx.beginPath();
        ctx.ellipse(cx, cy - 10, 130 + glow * 20, 75 + glow * 15, 0, 0, Math.PI * 2);
        ctx.stroke();
        ctx.restore();
    }
}

function _drawBabyFace(ctx, cx, cy, amp, bv, t) {
    // Глаза: открыты при активности, закрыты при сне
    const eyeOpen = amp > 0.3;
    ctx.fillStyle = eyeOpen ? '#223344' : `rgb(${Math.round(bv*0.7)},${Math.round(bv*0.6)},${Math.round(bv*0.5)})`;

    if (eyeOpen) {
        // Открытые глаза
        ctx.beginPath(); ctx.ellipse(cx - 11, cy - 3, 5, 6, 0, 0, Math.PI * 2); ctx.fill();
        ctx.beginPath(); ctx.ellipse(cx + 11, cy - 3, 5, 6, 0, 0, Math.PI * 2); ctx.fill();
    } else {
        // Закрытые (дуги)
        ctx.strokeStyle = `rgb(${Math.round(bv*0.5)},${Math.round(bv*0.4)},${Math.round(bv*0.35)})`;
        ctx.lineWidth = 1.5;
        ctx.beginPath(); ctx.arc(cx - 11, cy - 2, 5, Math.PI, 0); ctx.stroke();
        ctx.beginPath(); ctx.arc(cx + 11, cy - 2, 5, Math.PI, 0); ctx.stroke();
    }

    // Рот: дрожит при крике/плаче
    const mouthAmp = amp > 0.6 ? amp * 3 * Math.sin(t * 0.35) : 0;
    ctx.beginPath();
    ctx.arc(cx, cy + 8 + mouthAmp, 5 + amp * 4, 0.1, Math.PI - 0.1);
    ctx.strokeStyle = `rgb(${Math.round(bv*0.6)},${Math.round(bv*0.4)},${Math.round(bv*0.35)})`;
    ctx.lineWidth = 1.5;
    ctx.stroke();
}

function _drawDetectionOverlay(ctx, t) {
    const md = _videoData.motion_detected;
    const at = _videoData.audio_triggered;

    if (!md && !at) return;

    let color;
    if (md && at)  color = COLOR_BOTH;
    else if (md)   color = COLOR_MOTION;
    else           color = COLOR_AUDIO;

    // Мерцание рамки
    const blink = 0.5 + 0.5 * Math.sin(t * (md && at ? 0.4 : 0.2));
    ctx.save();
    ctx.globalAlpha = 0.7 * blink;
    ctx.strokeStyle = color;
    ctx.lineWidth   = 2;
    ctx.shadowColor = color;
    ctx.shadowBlur  = 10;

    // Угловые маркеры зоны детекции
    const mx = 100, my = 80, mw = 440, mh = 320, cs = 24;
    const corners = [
        [mx, my, cs, 0, 0, cs],
        [mx + mw, my, -cs, 0, 0, cs],
        [mx, my + mh, cs, 0, 0, -cs],
        [mx + mw, my + mh, -cs, 0, 0, -cs],
    ];
    for (const [x, y, dx1, dy1, dx2, dy2] of corners) {
        ctx.beginPath();
        ctx.moveTo(x + dx1, y + dy1);
        ctx.lineTo(x, y);
        ctx.lineTo(x + dx2, y + dy2);
        ctx.stroke();
    }

    // Метка детектора
    ctx.globalAlpha = 0.9 * blink;
    ctx.font        = 'bold 11px monospace';
    ctx.fillStyle   = color;
    ctx.shadowBlur  = 4;
    const label = md && at ? 'MOTION + AUDIO' : md ? 'MOTION DETECTED' : 'AUDIO TRIGGERED';
    ctx.fillText(label, mx + 6, my - 6);

    ctx.restore();
}

function _drawMetricsOverlay(ctx) {
    const mp = _videoData.motion_prob  || 0;
    const nl = _videoData.noise_level  || 0;

    // Вертикальные индикаторы справа
    const barW = 8, barH = 80, x0 = W - 40, y0 = H - 110;

    _drawBar(ctx, x0,      y0, barW, barH, mp, COLOR_MOTION, 'MOT');
    _drawBar(ctx, x0 + 18, y0, barW, barH, nl, COLOR_AUDIO,  'AUD');
}

function _drawBar(ctx, x, y, w, h, value, color, label) {
    // Фон
    ctx.fillStyle = 'rgba(0,0,0,0.5)';
    ctx.fillRect(x, y, w, h);

    // Заполнение
    const fh = Math.round(h * value);
    ctx.fillStyle = color;
    ctx.globalAlpha = 0.85;
    ctx.fillRect(x, y + h - fh, w, fh);
    ctx.globalAlpha = 1;

    // Подпись
    ctx.font      = 'bold 9px monospace';
    ctx.fillStyle = '#aaaaaa';
    ctx.fillText(label, x - 1, y + h + 12);
}

function _drawHUD(ctx, t) {
    const state    = _videoData.baby_state || 'SLEEPING_PEACEFUL';
    const labelCfg = STATE_LABELS[state] || { text: state, color: '#ffffff' };

    // Верхняя строка: состояние
    ctx.save();
    ctx.font      = 'bold 14px monospace';
    ctx.fillStyle = labelCfg.color;
    ctx.shadowColor = labelCfg.color;
    ctx.shadowBlur  = 6;
    ctx.fillText(`STATE: ${labelCfg.text}`, 14, 26);
    ctx.restore();

    // Вторая строка: физика
    ctx.font      = '11px monospace';
    ctx.fillStyle = 'rgba(180,200,220,0.85)';
    ctx.fillText(
        `ROOM ${(_videoData.room_temp || 22).toFixed(1)}°C  ` +
        `BODY ${(_videoData.body_temp || 36.6).toFixed(1)}°C  ` +
        `HUM ${(_videoData.humidity || 45).toFixed(0)}%`,
        14, 44
    );

    // Метка ночника
    if (_videoData.nightlight) {
        ctx.font      = 'bold 11px monospace';
        ctx.fillStyle = '#ffdd88';
        ctx.fillText('NIGHTLIGHT ON', 14, 60);
    }

    // Временна́я метка
    const now = new Date();
    const ts  = `${String(now.getHours()).padStart(2,'0')}:` +
                `${String(now.getMinutes()).padStart(2,'0')}:` +
                `${String(now.getSeconds()).padStart(2,'0')}`;
    ctx.font      = '10px monospace';
    ctx.fillStyle = 'rgba(150,170,180,0.7)';
    ctx.fillText(ts, W - 58, H - 10);
}

function _drawNoise(ctx, brightness) {
    // Зернистость ночного видения — убывает при включённом ночнике
    const intensity = Math.max(0, 0.06 - brightness * 0.05);
    if (intensity < 0.005) return;

    const imgData = ctx.getImageData(0, 0, W, H);
    const d = imgData.data;
    for (let i = 0; i < d.length; i += 4) {
        const n = (Math.random() - 0.5) * 255 * intensity;
        d[i]   = Math.min(255, Math.max(0, d[i]   + n));
        d[i+1] = Math.min(255, Math.max(0, d[i+1] + n));
        d[i+2] = Math.min(255, Math.max(0, d[i+2] + n));
    }
    ctx.putImageData(imgData, 0, 0);
}
