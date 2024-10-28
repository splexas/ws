#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <websockets/frame.h>
#include <websockets/handshake.h>
#include <websockets/websockets.h>

#include <stdio.h>
#include <string.h>

#include <libbase64.h>
#include <openssl/sha.h>

#include <stdint.h>
#include <zconf.h>
#include <zlib.h>

static const char WS_MAGIC[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static void read_cb(struct bufferevent *bev, void *ctx)
{
    struct ws_client *cl = (struct ws_client *)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);

    struct evbuffer_iovec vec[1];
    evbuffer_peek(input, -1, NULL, vec, 1);

    int len = evbuffer_get_length(input);

    if (cl->handshake_done == 0) {
        printf("%.*s\n", (int)vec[0].iov_len, (char *)vec[0].iov_base);
        struct header_entry *head =
            ws_parse_handshake((char *)vec[0].iov_base, len);
        if (!head) {
            fprintf(stderr,
                    "Failed to parse the handshake header of websocket\n");
            return;
        }

        char *websocket_key = ws_header_get(head, "Sec-WebSocket-Key")->value;
        printf("%s\n", websocket_key);

        /* Joining strings */
        int websocket_key_len = strlen(websocket_key);

        char *concat = malloc(websocket_key_len + sizeof(WS_MAGIC));
        snprintf(concat, websocket_key_len + sizeof(WS_MAGIC), "%s%s",
                 websocket_key, WS_MAGIC);

        /* SHA1 hash the joined strings */
        unsigned char hash[SHA_DIGEST_LENGTH];
        int concat_len =
            websocket_key_len + sizeof(WS_MAGIC) - 1; // without null terminator
        SHA1((const unsigned char *)concat, concat_len, hash);

        /* Cleanup stuff since we have a hash array */
        ws_free_handshake(head);
        free(concat);

        evbuffer_drain(input, len);

        /* Base64 encode the SHA1 hash */
        char base64_out[1024];
        size_t base64_out_len = 0;

        base64_encode((const char *)hash, sizeof(hash), base64_out,
                      &base64_out_len, 0);

        /* Make a response */
        char response[1024];
        int written =
            snprintf(response, sizeof(response),
                     "HTTP/1.1 101 Switching Protocols\r\n"
                     "Upgrade: websocket\r\n"
                     "Connection: Upgrade\r\n"
                     //"Sec-WebSocket-Extensions: permessage-deflate\r\n"
                     "Sec-WebSocket-Accept: %.*s\r\n\r\n",
                     (int)base64_out_len, base64_out);

        /* Send back the response */
        if (bufferevent_write(bev, response, written) != 0) {
            bufferevent_free(bev);
            return;
        }

        cl->handshake_done = 1;
    }
    else {
        printf("vec[0].len: %d\n", len);
        uint8_t *buf = (uint8_t *)vec[0].iov_base;
        for (int i = 0; i < len; i++) {
            printf("%02x ", buf[i]);
        }
        printf("\n");

        ws_frame_t frame;
        ws_parse_frame(buf, len, &frame);
        printf("Parsed frame: \n");
        printf("FIN: %u, opcode: %u, length: %lu\n", frame.fin, frame.opcode,
               frame.len);

        if (frame.opcode == WS_OPCODE_TEXT) {
            printf("%.*s\n", (int)frame.len, frame.begin);

            // Make a response frame
        }
    }
}

static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR) {
        fprintf(stderr, "Error from bufferevent");
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        printf("Disconnecting client\n");
        bufferevent_free(bev);
    }
}

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd,
                           struct sockaddr *address, int socklen, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev =
        bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    if (!bev)
        return;

    struct ws_client *cl = (struct ws_client *)malloc(sizeof(struct ws_client));

    if (!cl) {
        bufferevent_free(bev);
        return;
    }

    cl->handshake_done = 0;

    bufferevent_setcb(bev, read_cb, NULL, event_cb, (void *)cl);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr,
            "Got an error %d (%s) on the listener. "
            "Shutting down.\n",
            err, evutil_socket_error_to_string(err));
}

int ws_server_init(struct event_base *base, const char *ip, const uint16_t port)
{
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (evutil_inet_pton(AF_INET, ip, &sin.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed\n");
        return 1;
    }

    struct evconnlistener *listener = evconnlistener_new_bind(
        base, accept_conn_cb, NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1, (struct sockaddr *)&sin, sizeof(sin));
    if (!listener) {
        fprintf(stderr, "Couldn't create listener\n");
        return 1;
    }

    evconnlistener_set_error_cb(listener, accept_error_cb);
    return 0;
}