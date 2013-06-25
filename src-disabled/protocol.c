#include <stdint.h>

#include <util/crc16.h>

#define SYNC_BYTE   0x42

// if necessary, the CRC can be replaced with a version
// optimized for size rather than speed; the versions in
// crc16.h are a bit large.
#define CRC_INIT    0xFFFF
#define CRC_UPDATE  _crc_ccitt_update

typedef enum parser_state_t {
    EXPECTING_SYNC,
    EXPECTING_SIZE,
    EXPECTING_DATA,     // 'length' indicates remaining data bytes
    EXPECTING_CRC,      // 'length' indicates remaining crc bytes
} parser_state_t;

static parser_state_t state;
static uint8_t length;
static uint16_t crc;

// TODO: implement payload processing here
static void reset_payload(uint8_t length) {}
static void receive_payload_byte(uint8_t b) {}
static void accept_payload() {}

void reset_protocol_parser() {
    state = EXPECTING_SYNC;
}

void receive_byte(uint8_t b) {
    switch (state) {
        case EXPECTING_SYNC:
            if (b == SYNC_BYTE) state = EXPECTING_SIZE;
            break;
        
        case EXPECTING_SIZE:
            state = EXPECTING_DATA;
            crc = CRC_INIT;
            length = b;
            reset_payload(b);
            break;
        
        case EXPECTING_DATA:
            CRC_UPDATE(crc, b);
            
            if (--length == 0) {
                state = EXPECTING_CRC;
                length = 2;
            }
            
            receive_payload_byte(b);
            break;
        
        case EXPECTING_CRC:
        default:
            CRC_UPDATE(crc, b);
            if (--length == 0) {
                state = EXPECTING_SYNC;
                if (crc == 0) accept_payload();
            }
            break;
    }
}