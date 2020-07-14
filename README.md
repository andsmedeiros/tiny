# Tiny: the testable C memory allocator

## Overview

Tiny is a small and portable memory allocator. It allows unit tests to control and query the allocator during run time.

Tiny uses a very naive allocation alogorithm that is probable not suitable for production builds, but is extremely predictable and easy to inspect.

## Features

- Small and unobtrusive
- Entire in userland for enhanced portability
- Flexible memory model that can be allocated even on the stack
- Can override standard library's implementations
- Can force out-of-memory situations to test for `malloc()` & company returning null pointers

## Build

Download `tiny.c`, `tiny.h` and, optionally, `tiny-override.c` ([Overriding stdlib](#overriding-stdlib)).

Either:
1. Compile the source files along with your project's files;
2. Download `Makefile` and `make` into the directory (Linux only).

There are some [building options](#building-options).

## API

### Drop-in stdlib replacements
```C
void *tiny_malloc(size_t size);
void *tiny_realloc(void *ptr, size_t size);
void *tiny_calloc(size_t num, size_t size);
void tiny_free(void *ptr);
```

### Control functions
```C
// Initialised the library with some buffer. Any allocated memory will be in 
// this buffer.
// Unless the library was compiles with a initial static buffer allocated, all
// drop-in functions will fail until this function is called.
void tiny_init(uint8_t *buffer, size_t size);

// Prints a summary of the library and the memory layout of the currently 
// allocated buffer
void tiny_inspect();

// Releases the buffer from the library. Freeing memory allocated on the
// attached buffer **after** it has been released yields unknown behaviour.
void tiny_clear();

// Forces an out-of-memory situation if set. This causes  drop-in allocation
// functions to return NULL as if there was no memory left.
void tiny_out_of_memory(bool status);

// Returns the last operation processed by the library and whether it was 
// successful or not.
tiny_operation tiny_last_operation();

// Returns the size of each allocated block of memory. Also, the natural 
// alignment of all pointers yielded by the library.
size_t tiny_block_size();
```

## Overriding stdlib

The `tiny-override.c` file redefines `malloc()`, `calloc()`, `realloc()` and `free()` to call tiny's implementations instead of the ones provided by your sdtlib's ones.

There are some methods to inject the overrides in your program, depending on platform and compiler.

1. Assuming `make` was successfully run, the `dist` folder should contain four files:
    - `tiny.o`
    - `tiny-override.o`
    - `libtiny.so`
    - `libtiny-override.so`

    You can then either compile `tiny-override.o` along your final binary build, which will include tiny into the binary itself or `LD_PRELOAD=./dist/libtiny-override.so` when running your binary, which will dynamically inject `tiny` into **all** calls to the stdlib's overriden functions, even performed by other shared libraries.

2. Just compile `tiny-override.c` along other `tiny` files and your own code.

Overriding the stdlib's functions *may*  cause undesired effects, especially if another shared library gets to call them before you get a chance to initialise `tiny`. This means allocation will always fail because there is no buffer associated with the library.

If you need to have a functional implementation at startup time, you can [compile the library with an static buffer](#building-options). By default, the `Makefile` builds `tiny-override.o` and `libtiny-override.so` with a 4000 byte static buffer.

## Building options

There are two macros that control how the library is built:

- `TINY_ALIGNMENT`: If set, expects to alias a type that has the minimum alignment suitable for any data type. If not, the alignment is automatically calculated based on `max_align_t`.

    *E.g.*: `-DTINY_ALIGNMENT=double` will allocate all memory on `alignof(double)` boundaries.

- `TINY_BUFFER`: If set, expects to contain an integer constant value in bytes used to create an static buffer that will be immediately available, even if `tiny_init()` is not called.

    *E.g*: `-DTINY_BUFFER=4000` will build the library with a 4000 byte static buffer already initialised.