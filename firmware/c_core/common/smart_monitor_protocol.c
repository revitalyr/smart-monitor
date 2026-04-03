#include "smart_monitor_protocol.h"
#include <string.h>

// CRC16 calculation (CCITT polynomial)
uint16_t protocol_calculate_crc(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
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

bool protocol_validate_header(const protocol_header_t* header) {
    // Check magic bytes
    if (header->magic != PROTOCOL_MAGIC) {
        return false;
    }
    
    // Check version
    if (header->version != PROTOCOL_VERSION) {
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

void protocol_create_header(protocol_header_t* header, message_type_t type, 
                        uint32_t sequence, uint32_t timestamp, 
                        uint16_t payload_length) {
    memset(header, 0, sizeof(protocol_header_t));
    
    header->magic = PROTOCOL_MAGIC;
    header->version = PROTOCOL_VERSION;
    header->type = type;
    header->sequence = sequence;
    header->timestamp = timestamp;
    header->payload_length = payload_length;
    header->checksum = 0; // Will be calculated when sending
}
