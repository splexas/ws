# WebSockets
- This is a basic implementation of WebSockets based entirely on libevent.
# TODO
- [ ] Pass a `evbuffer_iovec` array to payload argument for callbacks. There is a possibility that when big enough packet is received, data will become stripped.
- [ ] Support multiple WebSocket extensions (permessage-deflate, etc...)
- [ ] Support HTTPS (wss://)
# Libraries used
- https://github.com/libevent/libevent
- https://github.com/troydhanson/uthash (header-only, used for parsing handshake headers)
- https://github.com/openssl/openssl (used for BASE64 and SHA1)