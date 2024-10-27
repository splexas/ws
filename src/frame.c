#include <stdio.h>
#include <websockets/frame.h>

static void ws_payload_unmask(uint8_t *payload, const uint64_t payload_len,
                              uint8_t *masking_key)
{
    for (uint64_t i = 0; i < payload_len; i++)
        payload[i] ^= masking_key[i % 4];
}

void ws_parse_payload(ws_frame_t *frame, ws_payload_t *payload_out)
{
    uint8_t *buf = (uint8_t *)frame;

    uint64_t len = buf[1] & 0x7f; // 9 - 15 bits
    uint8_t *payload = &buf[2];

    if (len == 126) {
        // read 9 - 31 bits
        len = buf[1] << 15 | (buf[2] << 8) | buf[3];
        payload += 2;
    }
    else if (len == 127) {
        // read 9 - 79 bits
        // integer overflows??? (we cannot fit 71 bytes)
        len = buf[1];
        for (int i = 2; i <= 7; i++)
            len = len << 8 | buf[i];
        len = len << 7 | buf[8];
        payload += 8;
    }

    payload_out->len = len;
    printf("payload_out->len: %ld\n", len);

    if (frame->masked) {
        printf("=== BEFORE MASKING PAYLOAD ===\n");
        for (int i = 0; i < len; i++)
            printf("%d ", payload[i]);
        printf("\n===========\n");

        uint8_t *masking_key = payload;
        payload_out->begin = payload + 4;
        ws_payload_unmask(payload_out->begin, payload_out->len, masking_key);

        printf("=== AFTER MASKING PAYLOAD ===\n");
        for (int i = 0; i < len; i++)
            printf("%d ", payload[i]);
        printf("\n===========\n");
    }
    else {
        payload_out->begin = payload;
    }
}