#include "tiny.h"

void *malloc(size_t size) {
    return tiny_malloc(size);
}

void *realloc(void *ptr, size_t size) {
    return tiny_realloc(ptr, size);
}

void *calloc(size_t num, size_t size) {
    return tiny_calloc(num, size);
}
void free(void *ptr) {
    tiny_free(ptr);
}