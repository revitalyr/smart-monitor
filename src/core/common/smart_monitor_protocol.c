#include "smart_monitor_protocol.h"
#include <string.h>

// CRC16 calculation (CCITT polynomial)
uint16_t protocol_calculate_crc(const uint8_t* data, ByteCount length) {
    uint16_t crc = 0xFFFF;
    
    for (ByteCount i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0x1021;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

bool protocol_validate_header(const ProtocolHeader* header) {
    // Check magic bytes
    if (header->magic != PROTOCOL_MAGIC_BYTES) {
        return false;
    }
    
    // Check version
    if (header->version != PROTOCOL_VERSION_MAJOR) {
        return false;
    }
    
    // Validate message type
    if (header->type < MSG_TYPE_HEARTBEAT || header->type > MSG_TYPE_RESPONSE) {
        return false;
    }
    
    // Validate payload length
    if (header->payload_length > 4096) { // Max 4KB payload
        return false;
    }
    
    return true;
}

void protocol_create_header(ProtocolHeader* header, MessageType type, 
                        uint32_t sequence, TimestampMs timestamp, 
                        uint16_t payload_length) {
    memset(header, 0, sizeof(ProtocolHeader));
    
    header->magic = PROTOCOL_MAGIC_BYTES;
    header->version = PROTOCOL_VERSION_MAJOR;
    header->type = type;
    header->sequence = sequence;
    header->timestamp = timestamp;
    header->payload_length = payload_length;
    header->checksum = 0; // Will be calculated when sending
}
