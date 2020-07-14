CC := gcc
CFLAGS := -std=c11 -Wall -Werror -Wextra -pedantic -g

all: dist/libtiny.so dist/libtiny-override.so dist/tiny.o dist/tiny-override.o

dist/libtiny.so: tiny.c tiny.h
	mkdir -p dist
	$(CC) -o dist/libtiny.so tiny.c -shared -fpic $(CFLAGS)

dist/tiny.o: tiny.c tiny.h
	mkdir -p dist
	$(CC) -c -o dist/tiny.o tiny.c $(CFLAGS)

dist/libtiny-override.so: tiny-override.c tiny.c tiny.h
	mkdir -p dist
	$(CC) -o dist/libtiny-override.so tiny-override.c tiny.c \
		-DTINY_BUFFER=4000	\
		-shared -fpic $(CFLAGS)

dist/tiny-override.o: tiny-override.c tiny.h
	mkdir -p dist
	$(CC) -c -o dist/tiny-override.o tiny-override.c $(CFLAGS) \
		-DTINY_BUFFER=4000

dist/test: test.c dist/libtiny.so
	mkdir -p dist
	$(CC) -o dist/test test.c -Ldist -ltiny $(CFLAGS)

.PHONY: clean test

clean:
	rm -rf dist

test: dist/test dist/libtiny-override.so
	LD_PRELOAD=`pwd`/dist/libtiny-override.so LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):`pwd`/dist dist/test