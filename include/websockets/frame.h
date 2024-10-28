#ifndef _WEBSOCKETS_FRAME_H
#define _WEBSOCKETS_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define WS_OPCODE_CONTINUATION 0
#define WS_OPCODE_TEXT 1
#define WS_OPCODE_BINARY 2
#define WS_OPCODE_CLOSE 8
#define WS_OPCODE_PING 9
#define WS_OPCODE_PONG 10

typedef struct ws_frame {
    uint8_t fin;
    uint8_t opcode;
    uint64_t len;
    uint8_t *begin;
} ws_frame_t;

void ws_parse_frame(uint8_t *buf, const uint64_t buf_len,
                    ws_frame_t *frame_out);

int ws_calc_frame_size(const uint64_t payload_len);

int ws_make_frame(const bool fin, const uint8_t opcode,
                  const uint64_t payload_len, uint8_t *buf_out,
                  const uint64_t buf_len);

#ifdef __cplusplus
}
#endif

#endif // _WEBSOCKETS_FRAME_H