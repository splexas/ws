#ifndef _WEBSOCKETS_H
#define _WEBSOCKETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <event2/bufferevent.h>
#include <event2/event.h>
#include <stdbool.h>
#include <stdint.h>

struct ws_client {
    bool handshake_done;
    bool disconnect;
    void *ctx;
};

typedef void (*ws_read_cb)(struct bufferevent *bev, const uint8_t opcode,
                           void *payload, const uint64_t len, void *ctx);

typedef void (*ws_accept_cb)(struct bufferevent *bev, struct ws_client *client);
typedef void (*ws_close_cb)(struct bufferevent *bev, void *ctx);

typedef struct ws_ctx {
    ws_read_cb read_cb;
    ws_accept_cb accept_cb;
    ws_close_cb close_cb;
} ws_ctx_t;

int ws_server_init(struct event_base *base, const char *ip, const uint16_t port,
                   ws_ctx_t ctx);

int ws_client_send(struct bufferevent *bev, const uint8_t opcode,
                   const void *data, const uint64_t len);

void ws_client_set_ctx(struct ws_client *client, void *ctx);

#ifdef __cplusplus
}
#endif

#endif // _WEBSOCKETS_H