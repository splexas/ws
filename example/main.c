#include <event2/bufferevent.h>
#include <event2/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <websockets/frame.h>
#include <websockets/websockets.h>

void read_cb(struct bufferevent *bev, const uint8_t opcode, void *payload,
             const uint64_t len, void *ctx)
{
    printf("Opcode: %u\n", opcode);
    printf("Integer from ctx: %d\n", *(int *)ctx);

    switch (opcode) {
    case WS_OPCODE_CONTINUATION: {
        printf("Packet is continuing\n");
        break;
    }
    case WS_OPCODE_TEXT: {
        printf("Data received: %.*s\n", (int)len, (char *)payload);
        break;
    }
    case WS_OPCODE_BINARY: {
        printf("We received binary payload\n");
        break;
    }
    case WS_OPCODE_PING: {
        printf("Client has sent a PING\n");
        break;
    }
    case WS_OPCODE_PONG: {
        printf("Client has sent a PONG\n");
        break;
    }
    };

    char resp[] = "WebSockets!";
    int r = ws_client_send(bev, WS_OPCODE_TEXT, resp, sizeof(resp) - 1);
    if (r != 0) {
        printf("Failed to deliver a message to the client!\n");
    }
}

void accept_cb(struct bufferevent *bev, struct ws_client *client)
{
    printf("Accepted a WebSocket client!\n");

    /* Allocate an integer to pass into ctx */
    int *a = (int *)malloc(sizeof(int));
    *a = 10;
    ws_client_set_ctx(client, a);
}

void close_cb(struct bufferevent *bev, void *ctx)
{
    printf("WebSocket client will close after this callback\n");
    int *a = (int *)ctx;
    free(a);
}

int main()
{
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Failed to alloc event_base\n");
        return 1;
    }

    ws_ctx_t ctx = {
        .read_cb = read_cb, .accept_cb = accept_cb, .close_cb = close_cb};

    if (ws_server_init(base, "127.0.0.1", 44444, ctx) != 0) {
        fprintf(stderr, "Failed to init websocket server\n");
        return 1;
    }

    event_base_dispatch(base);
    return 0;
}