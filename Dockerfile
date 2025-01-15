FROM alpine:latest AS builder
RUN apk add --no-cache gcc musl-dev sqlite-dev openssl-dev vim
COPY base64url.c /workspace/base64url.c
RUN gcc -fPIC -shared -o /workspace/base64url.so /workspace/base64url.c -lsqlite3 -lssl -lcrypto

FROM alpine:latest
RUN apk add --no-cache sqlite readline
COPY --from=builder /workspace/base64url.so /app/base64url.so
COPY sqliterc /root/.sqliterc
ENV SQLITE_HISTORY=/root/.sqlite_history
WORKDIR /workspace
CMD ["sqlite3"]
