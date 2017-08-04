all: ev.o coro.o unittest_slist echo_server libcoro.a

CFLAGS=-Wall -Wextra -std=c99 -g -D_GNU_SOURCE

ev.o:
	cc -g -c 3rd/libev/ev.c -o bin/ev.o

coro.o:
	cc $(CFLAGS) -c -I3rd/libev src/coro.c -o bin/coro.o

unittest_slist:
	cc $(CFLAGS) unittests/unittest_slist.c bin/coro.o bin/ev.o -o bin/unittest_slist -Isrc -I3rd/libev -lpthread -lm

echo_server:
	cc $(CFLAGS) examples/echo_server.c bin/coro.o bin/ev.o -o bin/echo_server -Isrc -I3rd/libev -lpthread -lm
	
libcoro.a:
	ar r bin/libcoro.a bin/ev.o bin/coro.o

clean:
	- rm -f bin/*
