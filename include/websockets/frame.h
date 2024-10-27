#ifndef _WEBSOCKETS_FRAME_H
#define _WEBSOCKETS_FRAME_H

#include <stdint.h>

#define WS_OPCODE_CONTINUATION 0
#define WS_OPCODE_TEXT 1
#define WS_OPCODE_BINARY 2
#define WS_OPCODE_CLOSE 8
#define WS_OPCODE_PING 9
#define WS_OPCODE_PONG 10

typedef struct ws_frame {
    uint8_t fin : 1;
    uint8_t rsv1 : 1;
    uint8_t rsv2 : 1;
    uint8_t rsv3 : 1;
    uint8_t opcode : 4;
    uint8_t masked : 1;
} __attribute__((packed)) ws_frame_t;

typedef struct ws_payload {
    uint8_t *begin;
    uint64_t len;
} ws_payload_t;

void ws_frame_unmask(uint8_t *frame, const uint64_t frame_len, uint8_t *output);
void ws_parse_payload(ws_frame_t *frame, ws_payload_t *payload_out);

#endif // _WEBSOCKETS_FRAME_H