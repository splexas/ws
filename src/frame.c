#include <stdint.h>
#include <websockets/frame.h>

static void ws_payload_unmask(uint8_t *payload, const uint64_t payload_len,
                              uint8_t *masking_key)
{
    for (uint64_t i = 0; i < payload_len; i++)
        payload[i] ^= masking_key[i % 4];
}

void ws_parse_frame(uint8_t *buf, const uint64_t buf_len, ws_frame_t *frame_out)
{
    // 1|000|1011
    frame_out->opcode = buf[0] & 0xf;
    frame_out->fin = buf[0] >> 7;

    // 1|1111101 (125 payload len)
    uint64_t payload_len = buf[1] & 0x7f;
    uint8_t masked = buf[1] >> 7;

    uint8_t *payload_begin = &buf[2];

    if (payload_len == 126) {
        // 16bit
        payload_len = (uint64_t) * (uint16_t *)payload_begin;
        payload_begin += 2;
    }
    else if (payload_len == 127) {
        // 64 bit
        payload_len = *(uint64_t *)payload_begin;
        payload_begin += 8;
    }

    if (masked) {
        uint8_t *masking_key = payload_begin;
        payload_begin += 4;
        ws_payload_unmask(payload_begin, payload_len, masking_key);
    }

    frame_out->begin = payload_begin;
    frame_out->len = payload_len;
}