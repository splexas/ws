#ifndef _WEBSOCKETS_H
#define _WEBSOCKETS_H

#include <event2/event.h>
#include <stdint.h>

struct ws_client {
    unsigned char handshake_done : 1;
};

int ws_server_init(struct event_base *base, const char *ip,
                   const uint16_t port);

#endif // _WEBSOCKETS_H