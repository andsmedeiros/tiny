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
- Fully inspectable internal state

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
void tiny_init(unsigned char *buffer, size_t size);
```

Initialises the library with some buffer. Any allocated memory will be in  this buffer.

Unless the library was compiles with a initial static buffer allocated, all drop-in functions will fail until this function is called.

```C
void tiny_clear(void);
```

Releases the buffer from the library. Freeing memory allocated on the attached buffer **after** it has been released yields unknown behaviour.

```C
void tiny_reset(void);
```

Resets the library buffer to its initial value. Ensure no memory allocated on the previous buffer is freed after this.

```C
void tiny_out_of_memory(bool out_of_memory);
```

Forces an out-of-memory situation if set. This causes drop-in allocation functions to return NULL as if there was no memory left.

```C
tiny_operation tiny_last_operation(void);
```

Returns the last operation processed by the library and whether it was  successful or not.

```C
size_t tiny_block_size(void);
```

Returns the size of each allocated block of memory. Also, the natural  alignment of all pointers yielded by the library.

```C
void tiny_print(bool summary, bool last_op, bool heap);
```

Prints information of the library. 

Each boolean argument defines whether some info section should be printed:
- **summary**: prints information on taken and free memory, buffer address and size and alignment
- **last_op**: prints the last operation processed by the library and its status and associated size
- **heap**: prints the current heap layout

```C
tiny_summary tiny_inspect(void);
```

Returns a summary of the library containing information of taken and free memory, buffer address and size and alignment.

```C
tiny_section tiny_next_section(void *previous_header);
```

Iterates through the library's data sections, returning each section's header and data addresses, its size in blocks and whether it is taken or not.

*E.g.*:

```C
// Passing NULL as argument yields the first section
tiny_section section = tiny_next_section(NULL);

// The last section is always empty. We stop when we reach it.
while(section.data) {

    // Do something with `section`

    // Yields the next section
    section = tiny_next_section(section.header);
}
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

## Unit tests and code coverage

Most of the public API is adequately tested. At the moment, only a few diagnostic functions are not properly tested.

The test suite is written with [µnit](https://github.com/nemequ/munit/), so all its features and CLI switches are available. All needed test files are in the `test` directory.

If you are building with `make`, there are two convenient rules in the `Makefile`:

- `test`: Compiles and runs the suite with `libtiny.so` dynamically linked;
- `coverage`: Same as `test`, but also generates code coverage information. This requires `gcov`, `lcov` and `genhtml` to be in your `PATH`.
    
    Once generated, the coverage report can be found in `coverage/index.html`.

## Allocation algorithm

### Natural alignment

The library has a compile-time-defined natural alignment. Whenever a pointer or size is aligned, it means it holds the smallest multiple of this natual alignment that is greater than the requested pointer or size. 

The notation `x'` means `x aligned`.

*E.g.*: Given an alignment of `16`, an struct `s` with size `53` and a pointer `p` with value `0xffffff07`, `sizeof(s)' == 64` and `p' == 0xffffff10`. 

### Partitioning

When the library, compiled with an alignment `a`, is initialised with a buffer with address `b` of size `n`, `b` is aligned and the remaining space is partitioned in blocks of size `a`. This partitioned area is the actually usable memory.

**Note**: Because all blocks are aligned, there may be padding before or after the actual usable memory area.

### Headers

A header is a `size_t` with its upper bit reserved. It marks the size, in blocks, of a *section*, a contiguous area that contains the allocated memory.

Once the heap is partitioned, two headers are written, one in the first blocks and one in the last blocks. How many blocks a header take depends on `sizeof(size_t)` and the natural alignment.

The first header contains the number of blocks between this two main headers and its `taken` flag is unset (meaning the section is free). The last header has always `size=0` and serves as a marker for the end of the heap.

### Allocation

When requested to allocate an object of size `s`, the library iterates over each section, until it finds one that is free and can hold `s'/a` blocks.

If no suitable section is found, the library returns NULL (for all allocation functions) and logs the failure. Otherwise, this section is taken and its address is returned.

When taking a section, the difference between its size, in blocks, and the required blocks is calculated. If the remaining area in the section is big enough to hold another section (a header and at least one block), the section is split in two and a new header is created after the required amount of blocks. If the reamining area is too small, the whole section is taken and no splitting takes place.

## Deallocation

When a pointer is deallocated, its header is written with the `taken` flag unset. After that, the library iterates over the sections, merging any two consecutive free sections.