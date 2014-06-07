#include "xbee.h"
#include "lights.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <pt.h>
#include <stdbool.h>

/*
    XBee config notes...
    
    thing that (probably) need to be configured on the XBee modules:
    
        - same firmware revision
        - baud rate, API mode
        - pin configuration (disable RTS/CTS)
        - PAN ID
        - short addrs (optional)
        - description strings (optional)
 */

#define IN_BUF_SIZE 8

static uint8_t in_buf[IN_BUF_SIZE];
static uint8_t in_err[(IN_BUF_SIZE + 7) / 8];
static uint8_t head = 0, count = 0;

// receive a byte (and frame err flag) asynchronously,
// record it so it can be handled outside the interrupt.
void xbee_byte_received(uint8_t byte, bool err) {
    if (count < IN_BUF_SIZE) {
        uint8_t i = (head + count) % IN_BUF_SIZE;
        
        in_buf[i] = byte;
        
        uint8_t err_byte = i >> 3, err_bit = i & 0b111;
        if (err) in_err[err_byte] |=   1 << err_bit;
        else     in_err[err_byte] &= ~(1 << err_bit);
        
        count++;
    }
}

// dequeue an input event recorded by xbee_byte_received
static inline bool get_input_byte(uint8_t *byte, bool *err) {
    if (count == 0) return false;
    
    cli();
    *byte = in_buf[head];
    *err  = in_err[head >> 3] & (1 << (head & 0b111));
    
    head = (head + 1) % IN_BUF_SIZE;
    count--;
    sei();
    
    return true;
}

static void reset_parser(void);
static void parse_byte(uint8_t byte);

static inline void reset_api_msg();
static inline void recv_api_byte(uint8_t byte);
static inline void accept_api_msg();

static inline void reset_msg();
static inline void recv_byte(uint8_t byte);
static inline void accept_msg();

// parser for frame envelopes
enum {RX_IDLE, RX_SZ_HI, RX_SZ_LO, RX_DATA, RX_CKSUM};
uint8_t rx_state = RX_IDLE;
uint8_t rx_sz;
bool rx_esc;
uint8_t cksum;

PT_THREAD(xbee_task(struct pt *pt)) {
    uint8_t in_byte;
    bool frame_err;
    
    PT_BEGIN(pt);
    
    while(1) {
        PT_WAIT_UNTIL(pt, get_input_byte(&in_byte, &frame_err));
        
        if (frame_err) reset_parser();
        else parse_byte(in_byte);
    }
    
    PT_END(pt);
}

static void reset_parser(void) {
    rx_state = RX_IDLE;
    rx_esc = false;
}

static void parse_byte(uint8_t byte) {
    switch (byte) {
        case 0x7e: // start frame
            cksum = 0;
            rx_esc = false;
            rx_state = RX_SZ_HI;
            break;
        case 0x7d: // escape code
            rx_esc = true;
            break;
        case 0x11: // XON
        case 0x13: // XOFF
            // TODO: implement XON/XOFF?
            // I don't think our adapter breaks out RTS or CTS...
            break;
        default:
            // don't bother unescaping till here; properly escaped
            // bytes will never be any of the above, and we don't
            // need to waste time unescaping while we're waiting for
            // the start byte
            if (rx_esc) {
                byte ^= 0x20;
                rx_esc = false;
            }
            
            switch (rx_state) {
                default:
                case RX_IDLE:
                    break;
                case RX_SZ_HI:
                    // being careful to only actually touch 1 byte;
                    // avr-gcc does optimize this correctly as long 
                    // as rx_sz is not volatile.
                    rx_sz = (byte << 8) | (rx_sz & 0x00ff);
                    rx_state = RX_SZ_LO;
                    break;
                case RX_SZ_LO:
                    rx_sz = byte | (rx_sz & 0xff00);
                    if (rx_sz) {
                        reset_api_msg();
                        rx_state = RX_DATA;
                    } else {
                        rx_state = RX_IDLE;
                    }
                    break;
                case RX_DATA:
                    cksum += byte;
                    recv_api_byte(byte);
                    
                    if (--rx_sz == 0) {
                        rx_state = RX_CKSUM;
                    }
                    break;
                case RX_CKSUM:
                    if (cksum + byte == 0xFF) {
                        accept_api_msg();
                    }
                    reset_parser();
                    break;
            }
            
            break;
    }
}

// parser for frame payloads
enum {
    // this type is mainly a countdown to skip data packet headers.
    API_RX_DATA = 0, API_IDLE = 0xfe, API_STOP = 0xff,
};

uint8_t api_state;

static inline void reset_api_msg() {
    api_state = API_IDLE;
    
    reset_msg();
}

static inline void recv_api_byte(uint8_t byte) {
    switch (api_state) {
        // byte received as part of body of message; pass to next parser
        case API_RX_DATA:
            recv_byte(byte);
            break;
        
        case API_IDLE:
            switch (byte) {
                case 0x80: // s1 RX msg with 64-bit source addr; skip 10 bytes
                    api_state = 10;
                    break;
                case 0x81: // s1 RX msg with 16-bit source addr; skip 4 bytes
                    api_state = 4;
                    break;
                case 0x90: // s2 RX msg; skip 11 bytes
                    api_state = 11;
                    break;
                default: // ignore any other msg
                    api_state = API_STOP;
                    break;
            }
            break;
        
        case API_STOP:
            break;
        
        // all other values are interpreted as a number of bytes to skip
        default:
            api_state--;
            break;
    }
}

static inline void accept_api_msg() {
    switch (api_state) {
        case API_RX_DATA:
            accept_msg();
            break;
        
        default:
            break;
    }
}

// parser for "RX" frame message contents
#define BUFSZ 6
static uint8_t incoming[BUFSZ];
static uint8_t msg_pos;

static inline void reset_msg() {
    msg_pos = 0;
}

static inline void recv_byte(uint8_t byte) {
    if (msg_pos < BUFSZ) {
        incoming[msg_pos++] = byte;
    }
}

static inline void accept_msg() {
    if (msg_pos == 6) {
        uint16_t r = (incoming[0] << 8) | incoming[1];
        uint16_t g = (incoming[2] << 8) | incoming[3];
        uint16_t b = (incoming[4] << 8) | incoming[5];
        
        set_rgb(r, g, b);
    }
}
