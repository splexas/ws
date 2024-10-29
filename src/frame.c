/*
MIT License

Copyright (c) 2024 splexas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <arpa/inet.h>
#include <stdint.h>
#include <websockets/frame.h>

/* WS operates on big endian */

static uint64_t ntohll(uint64_t net)
{
    uint32_t high = ntohl((uint32_t)(net >> 32));
    uint32_t low = ntohl((uint32_t)(net & 0xFFFFFFFF));
    return ((uint64_t)high << 32) | low;
}

static uint64_t htonll(uint64_t host)
{
    uint32_t high = htonl((uint32_t)(host >> 32));
    uint32_t low = htonl((uint32_t)(host & 0xFFFFFFFF));
    return ((uint64_t)high << 32) | low;
}

static void ws_payload_unmask(uint8_t *payload, const uint64_t payload_len,
                              uint8_t *masking_key)
{
    for (uint64_t i = 0; i < payload_len; i++)
        payload[i] ^= masking_key[i % 4];
}

int ws_parse_frame(uint8_t *buf, const uint64_t buf_len, ws_frame_t *frame_out)
{
    // 1|000|1011
    frame_out->opcode = buf[0] & 0xf;
    frame_out->fin = buf[0] >> 7;

    // 1|1111101 (125 payload len)
    uint64_t payload_len = buf[1] & 0x7f;
    bool masked = buf[1] >> 7;

    if (!masked) {
        // clients must send masked payloads
        return -1;
    }

    uint8_t *payload_begin = &buf[2];

    if (payload_len == 126) {
        // 16bit
        payload_len = (uint64_t)ntohs(*(uint16_t *)payload_begin);
        payload_begin += 2;
    }
    else if (payload_len == 127) {
        // 64 bit
        payload_len = ntohll(*(uint64_t *)payload_begin);
        payload_begin += 8;
    }

    uint8_t *masking_key = payload_begin;
    payload_begin += 4;
    ws_payload_unmask(payload_begin, payload_len, masking_key);

    frame_out->begin = payload_begin;
    frame_out->len = payload_len;

    return (int)(payload_begin - buf);
}

int ws_calc_frame_size(const uint64_t payload_len)
{
    int r = 2;
    if (payload_len <= 125)
        return r;
    return r + (payload_len <= 65535 ? 2 : 8);
}

int ws_make_frame(const bool fin, const uint8_t opcode,
                  const uint64_t payload_len, uint8_t *buf_out,
                  const uint64_t buf_len)
{
    if (!buf_out || buf_len < 2)
        return -1;

    buf_out[0] = fin << 7 | opcode;

    uint8_t *buf_tail = &buf_out[1];

    if (payload_len <= 125) {
        buf_out[1] = (uint8_t)payload_len;
        buf_tail++;
    }
    else if (payload_len <= 65535) {
        buf_out[1] = 126;
        // fill out additional 16 bits
        *(uint16_t *)&buf_out[2] = htons(payload_len);
        buf_tail += 3;
    }
    else {
        buf_out[1] = 127;
        // fill out additional 64 bits
        *(uint64_t *)&buf_out[2] = htonll(payload_len);
        buf_tail += 9;
    }

    return (int)(buf_tail - buf_out);
}