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

#include <string.h>
#include <websockets/handshake.h>

struct header_entry *ws_parse_handshake(const char *data, const int data_len)
{
    struct header_entry *head = NULL;

    char *line_begin = (char *)data;
    int line_len = 0;

    for (int i = 0; i < data_len; i++) {
        if (data[i] == '\n') {
            // Skip the first line (GET....)
            if (line_begin == (char *)data)
                goto end_line;

            /* Indicates header ending */
            if (line_len == 1)
                break;

            int carr_pos = line_len - 1;
            int delim_pos = 0;

            for (int j = 0; j < line_len; j++) {
                if (line_begin[j] == ':') {
                    delim_pos = j;
                    break;
                }
            }

            char *key_begin = line_begin;
            int key_len = delim_pos;

            char *val_begin = key_begin + key_len + 2;
            int val_len = carr_pos - delim_pos - 2;

            struct header_entry *entry =
                (struct header_entry *)malloc(sizeof(struct header_entry));

            if (!entry)
                return NULL;

            strncpy(entry->key, key_begin, key_len);
            strncpy(entry->value, val_begin, val_len);

            HASH_ADD_STR(head, key, entry);

        end_line:
            line_begin = (char *)data + i + 1;
            line_len = 0;
        }
        else {
            line_len++;
        }
    }
    return head;
}

void ws_free_handshake(struct header_entry *head)
{
    struct header_entry *entry, *tmp;
    HASH_ITER(hh, head, entry, tmp)
    {
        HASH_DEL(head, entry);
        free(entry);
    }
}

struct header_entry *ws_header_get(struct header_entry *head, const char *key)
{
    struct header_entry *entry;
    HASH_FIND_STR(head, key, entry);
    return entry;
}
