#include <event2/event.h>
#include <stdio.h>
#include <websockets/websockets.h>

int main()
{
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Failed to alloc event_base\n");
        return 1;
    }

    if (ws_server_init(base, "127.0.0.1", 44444) != 0) {
        fprintf(stderr, "Failed to init websocket server\n");
        return 1;
    }

    event_base_dispatch(base);
    return 0;
}