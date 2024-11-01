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