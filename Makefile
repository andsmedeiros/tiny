CC := gcc
CFLAGS := -std=c11 -Wall -Werror -Wextra -pedantic -g

all: dist/libtiny.so dist/libtiny-override.so

dist/libtiny.so: tiny.c tiny.h
	mkdir -p dist
	$(CC) -o dist/libtiny.so tiny.c -shared -fpic $(CFLAGS)

dist/libtiny-override.so: tiny-override.c tiny.c tiny.h
	mkdir -p dist
	$(CC) -o dist/libtiny-override.so tiny-override.c tiny.c -shared -fpic $(CFLAGS)

dist/test: test.c dist/libtiny.so
	mkdir -p dist
	$(CC) -o dist/test test.c -Ldist -ltiny $(CFLAGS)

.PHONY: clean

clean:
	rm -rf dist