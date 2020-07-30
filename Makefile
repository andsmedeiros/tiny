CC := gcc
CFLAGS := -std=c11 -Wall -Werror -Wextra -pedantic -g

all: dist/libtiny.so dist/libtiny-override.so dist/tiny.o dist/tiny-override.o

dist/libtiny.so: tiny.c tiny.h
	mkdir -p dist
	$(CC) -o dist/libtiny.so tiny.c \
	 -DTINY_BUFFER=4000 \
	 -shared -fpic \
	 -fprofile-arcs -ftest-coverage \
	 $(CFLAGS)

dist/tiny.o: tiny.c tiny.h
	mkdir -p dist
	$(CC) -c -o dist/tiny.o tiny.c $(CFLAGS)

dist/libtiny-override.so: tiny-override.c tiny.c tiny.h
	mkdir -p dist
	$(CC) -o dist/libtiny-override.so tiny-override.c tiny.c \
		-DTINY_BUFFER=4000	\
		-shared -fpic 	\
		$(CFLAGS)

dist/tiny-override.o: tiny-override.c tiny.h
	mkdir -p dist
	$(CC) -c -o dist/tiny-override.o tiny-override.c \
		-DTINY_BUFFER=4000	\
		$(CFLAGS) 

dist/test: test/test.c test/helpers.h dist/libtiny.so
	mkdir -p dist
	$(CC) -o dist/test test/*.c -I. -Itest -Ldist -ltiny $(CFLAGS) -Wno-unused-parameter

.PHONY: clean test coverage

clean:
	rm -rf dist coverage

test: dist/test
	LD_LIBRARY_PATH=./dist dist/test

coverage: dist/test
	mkdir -p coverage
	LD_LIBRARY_PATH=./dist dist/test
	gcov test/test.c
	lcov -c --directory . --output-file coverage/test.info
	genhtml coverage/test.info --output-directory coverage