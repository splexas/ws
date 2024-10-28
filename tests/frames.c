#include <stdio.h>
#include <websockets/frame.h>

int main()
{
    unsigned char buf[1000];
    int bytes_written =
        ws_make_frame(false, 1, NULL, 44444444, buf, sizeof(buf));
    printf("bytes_written: %d\n", bytes_written);
    if (bytes_written == -1)
        return 1;

    ws_frame_t frame;
    ws_parse_frame(buf, bytes_written, &frame);
    printf("fin: %u, opcode: %u, len: %lu\n", frame.fin, frame.opcode,
           frame.len);

    return 0;
}