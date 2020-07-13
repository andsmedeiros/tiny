#include "tiny.h"
#include <stdio.h>
#include <string.h>

#ifndef TINY_ALIGNMENT
#define TINY_ALIGNMENT \
    ((uint8_t)offsetof(struct { char c; max_align_t member; }, member))
#endif

#define ALIGN(ptr)  \
    ((uint8_t *)(((uintptr_t)ptr + TINY_ALIGNMENT - 1) & ~(TINY_ALIGNMENT -1)))

#define HEADER_BLOCKS ((uint8_t)(uintptr_t)ALIGN(sizeof(size_t)) / TINY_ALIGNMENT)

#define TAKEN_BIT ((size_t)-1 ^ (((size_t)-1)>>1))

static const char * const operations[] = {         
    "TINY_NOP",
    "TINY_INIT",
    "TINY_CLEAR",
    "TINY_MALLOC",
    "TINY_REALLOC",
    "TINY_CALLOC",
    "TINY_FREE" 
};

typedef uint8_t tiny_block[TINY_ALIGNMENT];

typedef struct tiny_section {
    bool taken;
    size_t size;
    tiny_block *header;
    void *data;
} tiny_section;

static struct tiny {
    tiny_block *buffer;
    size_t size;
    bool out_of_memory;
    tiny_operation last_operation;
} tiny = { NULL, 0, false, { TINY_NOP, true, 0 } };

static void write_header(tiny_block *header, size_t size, bool taken) {
    *(size_t *)header = taken ? TAKEN_BIT | size : size;
}

static tiny_section read_header(tiny_block *header) {
    size_t header_value = *(size_t *)header;
    tiny_section section = { 
        header_value & TAKEN_BIT,
        header_value & ~TAKEN_BIT,
        header,
        (void *)(header + 1) 
    };
    return section;
}

static void allocate_at(tiny_section section, size_t block_count) {
    size_t remaining_space = 
        section.size - block_count - HEADER_BLOCKS;

    if(remaining_space <= 1) {
        write_header(section.header, block_count + remaining_space, true);
    } else {
        write_header(section.header, block_count, true);
        write_header(
            section.header + block_count + 1, 
            remaining_space, 
            false
        );
    }
}

static tiny_block *next_section(tiny_block *header) {
    tiny_section info = read_header(header);
    return header + info.size + 1;
}

static void store_operation(enum tiny_function function, bool success, size_t size) {
    tiny_operation op = { function, success, size };
    tiny.last_operation = op;
}

void tiny_init(uint8_t *buffer, size_t size) {
    uint8_t *aligned = ALIGN(buffer); 
    size_t lost_alignment = aligned - buffer;

    if(lost_alignment + 2 * HEADER_BLOCKS * TINY_ALIGNMENT >= size) {
        store_operation(TINY_INIT, false, size);
        return;
    }

    tiny.buffer = (tiny_block *)aligned;
    tiny.size = (size - lost_alignment) / TINY_ALIGNMENT - 2 * HEADER_BLOCKS;

    write_header(&tiny.buffer[0], tiny.size, false);
    write_header(&tiny.buffer[tiny.size + 1], 0, true);
    store_operation(TINY_INIT, true, size);
}

void tiny_inspect() {
    printf(
        "\nLast tiny operation: %s\n"
        "status: %s; size: %lu\n", 
        operations[tiny.last_operation.function], 
        tiny.last_operation.success ? "success" : "failure",
        tiny.last_operation.size
    );

    printf("Inspecting heap\n");

    if(tiny.buffer == NULL || tiny.size == 0) {
        printf("Heap not allocated.\n");
        return;
    }

    tiny_block *header = &tiny.buffer[0];
    tiny_section info = read_header(header);
    while(info.size > 0) {
        printf(
            "%s section with %lu blocks (%lu bytes) at [%p]; data starts at [%p]\n", 
            info.taken ? "Taken" : "Free",
            info.size,
            info.size * TINY_ALIGNMENT,
            (void *)info.header,
            info.data
        );
        header = next_section(header);
        info = read_header(header);
    } 
    printf("No more blocks to inspect\n");
}

void tiny_clear() {
    tiny.buffer = NULL;
    tiny.size = 0;
    store_operation(TINY_CLEAR, true, 0);
}

void tiny_out_of_memory(bool out_of_memory) {
    tiny.out_of_memory = out_of_memory;
}

tiny_operation tiny_last_operation() {
    return tiny.last_operation;
}

size_t tiny_block_size() { return TINY_ALIGNMENT; }

void *tiny_malloc(size_t size) {
    if(tiny.out_of_memory || tiny.buffer == NULL || size == 0) {
        store_operation(TINY_MALLOC, false, size);
        return NULL;
    }

    size_t blocks_required = (size_t)ALIGN(size) / TINY_ALIGNMENT;
    tiny_block *header = &tiny.buffer[0];
    tiny_section section = read_header(header);
    while(section.size > 0) {
        if(!section.taken && section.size >= blocks_required + HEADER_BLOCKS) {
            allocate_at(section, blocks_required);
            store_operation(TINY_MALLOC, true, size);
            return section.data;
        }
        header = next_section(header);
        section = read_header(header);
    } 
    store_operation(TINY_MALLOC, false, size);
    return NULL;
}

void *tiny_realloc(void *ptr, size_t size) {
    if(ptr == NULL) {
        void *data = tiny_malloc(size);
        store_operation(TINY_REALLOC, data != NULL, size);
        return data;
    }
    if(tiny.out_of_memory || tiny.buffer == NULL || size == 0) {
        store_operation(TINY_REALLOC, false, size);
        return NULL;
    }

    size_t blocks_required = (size_t)ALIGN(size) / TINY_ALIGNMENT;
    tiny_block *header = (tiny_block *)ptr - 1;
    tiny_section section = read_header(header);
    tiny_block *next = next_section(header);
    tiny_section next_section = read_header(next);

    if(!next_section.taken && next_section.size >= blocks_required - section.size + HEADER_BLOCKS) {
        write_header(header, section.size + next_section.size + HEADER_BLOCKS, true);
        allocate_at(read_header(header), blocks_required);
        store_operation(TINY_REALLOC, true, size);
        return  ptr;
    } else {
        void *new_block = tiny_malloc(size);
        if(new_block) {
            memcpy(new_block, section.data, section.size * TINY_ALIGNMENT);
            tiny_free(ptr);
        }
        store_operation(TINY_REALLOC, new_block != NULL, size);
        return new_block;
    }
}

void *tiny_calloc(size_t num, size_t size) {
    size_t full_size = num * size;
    if(size == 0 || num == 0 || full_size / num != size) {
        store_operation(TINY_CALLOC, false, size);
        return NULL;
    }
    void *data = tiny_malloc(full_size);
    if(data != NULL) {
        memset(data, 0, size);
    }
    store_operation(TINY_CALLOC, data != NULL, num * size);
    return data;
}

void tiny_free(void *ptr) {
    tiny_block *current = (tiny_block *)ptr - 1;
    tiny_section current_section = read_header(current);
    write_header(current, current_section.size, false);

    tiny_block *header = &tiny.buffer[0];
    tiny_section section = read_header(header);
    while(section.size > 0) {
        tiny_block *next = next_section(header);
        if(!section.taken) {
            tiny_section next_section = read_header(next);
            if(!next_section.taken) {
                write_header(header, section.size + next_section.size + HEADER_BLOCKS, false);
                section = read_header(header);
                continue;
            }
        }

        header = next;
        section = read_header(header);
    } 
    store_operation(TINY_FREE, true, current_section.size);
}