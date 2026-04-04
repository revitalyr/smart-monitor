/**
 * Mermaid Diagram Height and Scale Utility
 * Provides interactive scaling and height adjustment for Mermaid diagrams
 */
class MermaidHeightAdjuster {
    constructor() {
        this.scales = new Map();
        this.minScale = 50;
        this.maxScale = 200;
        this.scaleStep = 10;
        this.defaultScale = 100;
    }

    /**
     * Initialize the utility with tab switching and scale controls
     */
    init() {
        this.setupTabSwitching();
        this.setupScaleControls();
        
        // Render the first tab's diagram
        const activeTab = document.querySelector('.nav-tab.active');
        if (activeTab) {
            this.renderDiagramForTab(activeTab.dataset.tab);
        }
    }

    /**
     * Setup tab switching functionality
     */
    setupTabSwitching() {
        const tabs = document.querySelectorAll('.nav-tab');
        const panes = document.querySelectorAll('.tab-pane');

        tabs.forEach(tab => {
            tab.addEventListener('click', (e) => {
                const targetTab = e.target.dataset.tab;
                
                // Remove active class from all tabs and panes
                tabs.forEach(t => t.classList.remove('active'));
                panes.forEach(p => p.classList.remove('active'));
                
                // Add active class to clicked tab and corresponding pane
                e.target.classList.add('active');
                const targetPane = document.getElementById(targetTab);
                if (targetPane) {
                    targetPane.classList.add('active');
                }
                
                // Render diagram for the active tab
                this.renderDiagramForTab(targetTab);
            });
        });
    }

    /**
     * Setup scale controls with event delegation
     */
    setupScaleControls() {
        document.addEventListener('click', (e) => {
            // Handle scale delta buttons
            if (e.target.matches('[data-scale-delta]')) {
                const delta = parseInt(e.target.dataset.scaleDelta);
                const container = e.target.closest('.diagram-container');
                if (container) {
                    this.changeScale(container, delta);
                }
            }
            
            // Handle reset scale button
            if (e.target.matches('[data-reset-scale]')) {
                const container = e.target.closest('.diagram-container');
                if (container) {
                    this.resetScale(container);
                }
            }
        });
    }

    /**
     * Render Mermaid diagram for a specific tab
     */
    async renderDiagramForTab(tabName) {
        const diagramId = `${tabName}-diagram`;
        const container = document.getElementById(diagramId);
        
        if (!container) {
            console.warn(`Diagram container not found: ${diagramId}`);
            return;
        }

        const diagramCode = this.getDiagramCode(tabName);
        if (!diagramCode) {
            console.warn(`No diagram code found for tab: ${tabName}`);
            return;
        }

        // Clear existing content
        container.innerHTML = '';
        
        // Create temporary div for Mermaid rendering
        const tempDiv = document.createElement('div');
        tempDiv.style.visibility = 'hidden';
        tempDiv.innerHTML = `<div class="mermaid">${diagramCode}</div>`;
        container.appendChild(tempDiv);

        try {
            // Wait a bit for DOM to update
            await new Promise(resolve => setTimeout(resolve, 100));
            
            // Render the diagram
            const svgElement = await mermaid.run({
                nodes: tempDiv
            });
            
            // Clear temporary content and add the rendered SVG
            container.innerHTML = '';
            if (svgElement && svgElement.length > 0) {
                container.appendChild(svgElement[0]);
                
                // Apply saved scale or default
                const savedScale = this.scales.get(diagramId) || this.defaultScale;
                this.applyScale(container, savedScale);
                
                // Adjust container height
                this.adjustDiagram(container);
            }
        } catch (error) {
            console.error('Error rendering Mermaid diagram:', error);
            container.innerHTML = `<div style="color: #dc3545; padding: 20px;">Error rendering diagram: ${error.message}</div>`;
        }
    }

    /**
     * Get diagram code for a specific tab
     */
    getDiagramCode(tabName) {
        if (typeof diagrams !== 'undefined' && diagrams[tabName]) {
            return diagrams[tabName];
        }
        return null;
    }

    /**
     * Change scale for a diagram container
     */
    changeScale(container, delta) {
        const currentScale = this.getCurrentScale(container);
        const newScale = Math.max(this.minScale, Math.min(this.maxScale, currentScale + delta));
        
        this.applyScale(container, newScale);
        this.adjustDiagram(container);
        
        // Save scale for this diagram
        const diagramId = this.getDiagramId(container);
        if (diagramId) {
            this.scales.set(diagramId, newScale);
        }
    }

    /**
     * Reset scale to default
     */
    resetScale(container) {
        this.applyScale(container, this.defaultScale);
        this.adjustDiagram(container);
        
        // Remove saved scale
        const diagramId = this.getDiagramId(container);
        if (diagramId) {
            this.scales.delete(diagramId);
        }
    }

    /**
     * Apply scale transformation to diagram
     */
    applyScale(container, scale) {
        const svg = container.querySelector('svg');
        if (svg) {
            svg.style.transform = `scale(${scale / 100})`;
            svg.style.transformOrigin = 'top left';
            svg.style.transition = 'transform 0.2s ease';
        }
        
        this.updateScaleDisplay(container, scale);
    }

    /**
     * Get current scale from container
     */
    getCurrentScale(container) {
        const svg = container.querySelector('svg');
        if (svg) {
            const transform = svg.style.transform;
            const match = transform.match(/scale\(([\d.]+)\)/);
            return match ? parseFloat(match[1]) * 100 : this.defaultScale;
        }
        return this.defaultScale;
    }

    /**
     * Update scale display
     */
    updateScaleDisplay(container, scale) {
        const display = container.querySelector('.scale-display');
        if (display) {
            display.textContent = `${Math.round(scale)}%`;
        }
    }

    /**
     * Adjust diagram container height based on SVG content
     */
    adjustDiagram(container) {
        const svg = container.querySelector('svg');
        if (svg) {
            // Get the SVG bounding box
            const bbox = svg.getBBox();
            const padding = 40; // Add some padding
            
            // Set container height to accommodate scaled SVG
            const scale = this.getCurrentScale(container);
            const scaledHeight = (bbox.height + padding) * (scale / 100);
            const scaledWidth = (bbox.width + padding) * (scale / 100);
            
            container.style.minHeight = `${Math.max(scaledHeight, 300)}px`;
            container.style.minWidth = `${Math.max(scaledWidth, 400)}px`;
        }
    }

    /**
     * Get diagram ID from container
     */
    getDiagramId(container) {
        const diagramDiv = container.querySelector('[id$="-diagram"]');
        return diagramDiv ? diagramDiv.id : null;
    }
}

// Export for global access
if (typeof window !== 'undefined') {
    window.MermaidHeightAdjuster = MermaidHeightAdjuster;
}
