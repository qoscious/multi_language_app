gcc main.c -o list_api $(pkg-config --cflags libpq) -lmicrohttpd -ljansson $(pkg-config --libs libpq)
