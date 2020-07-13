#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct tiny_operation {
    enum tiny_function {
        TINY_NOP,
        TINY_INIT,
        TINY_CLEAR,
        TINY_MALLOC,
        TINY_REALLOC,
        TINY_CALLOC,
        TINY_FREE
    } function;
    bool success;
    size_t size;
} tiny_operation;

void tiny_init(uint8_t *buffer, size_t size);
void tiny_inspect();
void tiny_clear();
void tiny_out_of_memory(bool status);
tiny_operation tiny_last_operation();
size_t tiny_block_size();
void *tiny_malloc(size_t size);
void *tiny_realloc(void *ptr, size_t size);
void *tiny_calloc(size_t num, size_t size);
void tiny_free(void *ptr);