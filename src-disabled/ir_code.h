// codes on my remote are CODE(0,x<<3), where 'x' is:
// 
//  0x14 0x04 0x0c 0x1c
//  0x12 0x02 0x0a 0x1a
//  0x16 0x06 0x0e 0x1e
//  0x15 0x05 0x0d 0x1d
//  0x13 0x03 0x0b 0x1b
//  0x11 0x01 0x09 0x19
//
// and CODE(d,n), for 0 <= n < 32, is:
#define CODE(d,n)   (  (uint32_t)(d)        << 24   \
                    | ((uint32_t)(d)^0xFFL) << 16   \
                    |  (uint32_t)(n)        << 8    \
                    | ((uint32_t)(n)^0xFFL))

// recognize a code from our particular remote.
// 
// requires that the 'device' portion be 0 and the other 
// part be a multiple of 8.
static inline uint8_t identify_code(uint32_t code, uint8_t no_match) {
    if (code >> 16 != 0xFF) return no_match;
    uint8_t a = code;
    uint8_t b = code>>8;
    
    return (((a ^ b) == 0xFF) && !(b & 0x7)) ? b>>3 : no_match;
}

static inline uint8_t code_x(uint8_t code) {
    uint8_t raw_x = (code >> 3) & 3;
    switch(raw_x) {
        default:
        case 2: return 0;
        case 0: return 1;
        case 1: return 2;
        case 3: return 3;
    }
}

static inline uint8_t code_y(uint8_t code) {
    uint8_t raw_y = code & 7;
    switch(raw_y) {
        default:
        case 4: return 5;
        case 2: return 0;
        case 6: return 1;
        case 5: return 2;
        case 3: return 3;
        case 1: return 4;
    }
}

