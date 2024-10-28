#ifndef _WEBSOCKETS_H
#define _WEBSOCKETS_H

#include <event2/bufferevent.h>
#include <event2/event.h>
#include <stdbool.h>
#include <stdint.h>

struct ws_client {
    bool handshake_done;
};

int ws_server_init(struct event_base *base, const char *ip,
                   const uint16_t port);

int ws_client_send(struct bufferevent *bev, const uint8_t opcode,
                   const uint8_t *data, const uint64_t len);

#endif // _WEBSOCKETS_H