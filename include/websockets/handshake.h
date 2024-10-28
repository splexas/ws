#ifndef _WEBSOCKETS_HANDSHAKE_H
#define _WEBSOCKETS_HANDSHAKE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uthash.h"

struct header_entry {
    char key[256];
    char value[256];
    UT_hash_handle hh;
};

struct header_entry *ws_parse_handshake(const char *data, const int data_len);
void ws_free_handshake(struct header_entry *head);
struct header_entry *ws_header_get(struct header_entry *head, const char *key);

#ifdef __cplusplus
}
#endif

#endif // _WEBSOCKETS_HANDSHAKE_H