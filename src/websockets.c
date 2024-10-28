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

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <stdint.h>

#define WS_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

static ws_ctx_t _ctx = {NULL};

static void read_cb(struct bufferevent *bev, void *ctx)
{
    struct ws_client *cl = (struct ws_client *)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);

    struct evbuffer_iovec vec[1];
    evbuffer_peek(input, -1, NULL, vec, 1);

    int len = evbuffer_get_length(input);

    if (!cl->handshake_done) {
        struct header_entry *head =
            ws_parse_handshake((char *)vec[0].iov_base, len);

        if (!head) {
            bufferevent_free(bev);
            return;
        }

        struct header_entry *entry = ws_header_get(head, "Sec-WebSocket-Key");
        if (!entry) {
            ws_free_handshake(head);
            bufferevent_free(bev);
            return;
        }

        char *websocket_key = entry->value;

        /* Joining strings */
        int websocket_key_len = strlen(websocket_key);
        char *concat = malloc(websocket_key_len + sizeof(WS_MAGIC));
        if (!concat) {
            ws_free_handshake(head);
            bufferevent_free(bev);
            return;
        }

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

        // base64_encode((const char *)hash, sizeof(hash), base64_out,
        //               &base64_out_len, 0);

        int base64_out_len = EVP_EncodeBlock((unsigned char *)base64_out, hash,
                                             SHA_DIGEST_LENGTH);

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

        cl->handshake_done = true;

        if (_ctx.accept_cb)
            _ctx.accept_cb(bev, cl);
    }
    else {
        uint8_t *buf = (uint8_t *)vec[0].iov_base;
        ws_frame_t frame;
        ws_parse_frame(buf, len, &frame);

        if (frame.opcode == WS_OPCODE_CLOSE) {
            if (_ctx.close_cb)
                _ctx.close_cb(bev, cl->ctx);

            uint8_t tmp[256];
            int n =
                ws_make_frame(true, frame.opcode, frame.len, tmp, sizeof(tmp));

            if (bufferevent_write(bev, tmp, n) != 0) {
                bufferevent_free(bev);
                return;
            }

            cl->disconnect = true;
        }
        else {
            if (_ctx.read_cb)
                _ctx.read_cb(bev, frame.opcode, (void *)frame.begin, frame.len,
                             cl->ctx);
        }

        evbuffer_drain(input, len);
    }
}

static void write_cb(struct bufferevent *bev, void *ctx)
{
    struct ws_client *cl = (struct ws_client *)ctx;
    if (cl->disconnect) {
        bufferevent_free(bev);
        free(cl);
    }
}

static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    struct ws_client *cl = (struct ws_client *)ctx;
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
        free(cl);
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

    cl->handshake_done = false;
    cl->disconnect = false;
    cl->ctx = NULL;

    bufferevent_setcb(bev, read_cb, NULL, event_cb, (void *)cl);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    event_base_loopexit(base, NULL);
}

int ws_server_init(struct event_base *base, const char *ip, const uint16_t port,
                   ws_ctx_t ctx)
{
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (evutil_inet_pton(AF_INET, ip, &sin.sin_addr) != 1)
        return 1;

    struct evconnlistener *listener = evconnlistener_new_bind(
        base, accept_conn_cb, NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1, (struct sockaddr *)&sin, sizeof(sin));

    if (!listener)
        return 1;

    evconnlistener_set_error_cb(listener, accept_error_cb);

    _ctx = ctx;
    return 0;
}

int ws_client_send(struct bufferevent *bev, const uint8_t opcode,
                   const void *data, const uint64_t len)
{
    int frame_size = ws_calc_frame_size(len);
    uint8_t *buf = (uint8_t *)malloc(frame_size + len);
    if (!buf)
        return 1;

    int n = ws_make_frame(true, opcode, len, buf, frame_size + len);
    if (n != frame_size) {
        free(buf);
        return 1;
    }

    memcpy(buf + n, data, len);

    if (bufferevent_write(bev, buf, frame_size + len) != 0) {
        free(buf);
        return 1;
    }

    free(buf);
    return 0;
}

void ws_client_set_ctx(struct ws_client *client, void *ctx)
{
    client->ctx = ctx;
}