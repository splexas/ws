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