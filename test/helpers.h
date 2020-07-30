#ifndef TINY_TEST_HELPERS_H
#define TINY_TEST_HELPERS_H

#include "tiny.h"

#define ALIGN(size, alignment)  \
    (((size + alignment - 1) & ~(alignment - 1)))

#define ALIGN_PTR(ptr, alignment)   \
    (unsigned char *)ALIGN((uintptr_t)ptr, alignment)

#define SIZE_BLOCKS(size, alignment) \
    (ALIGN(size, alignment) / alignment)
    
#define OBJ_BLOCKS(obj, alignment)  \
    SIZE_BLOCKS(sizeof(obj), alignment)

#define DECLARE_HEAP(buff_size, obj_factor, section_num)                \
        tiny_reset();                                                   \
                                                                        \
        unsigned char buffer[buff_size];                                \
        size_t alignment = tiny_block_size();                           \
        size_t header_blocks = OBJ_BLOCKS(size_t, alignment);           \
        size_t obj_size = alignment * obj_factor;                       \
        size_t obj_blocks = SIZE_BLOCKS(obj_size, alignment);           \
        size_t obj_section = obj_blocks + header_blocks;                \
        size_t size = obj_section * alignment * section_num;            \
        size_t available_blocks = size / alignment - 2 * header_blocks; \
                                                                        \
        tiny_init(buffer, size);                                        \
        ASSERT_OP(INIT, true, size);                                    \
        ASSERT_HEAP({ { false, available_blocks } });                   \

#define ASSERT_OP(fn, succ, siz) do {                   \
    tiny_operation last_op = tiny_last_operation();     \
    assert_int(last_op.function, ==, TINY_##fn);        \
    assert_##succ(last_op.success);                     \
    assert_size(last_op.size, ==, siz);                 \
} while(0)

#define ASSERT_HEAP_EMPTY do {                  \
    tiny_summary summary = tiny_inspect();      \
    assert_ptr_null(summary.buffer);            \
    assert_int(summary.total.bytes, ==, 0);     \
    assert_int(summary.total.blocks, ==, 0);    \
    assert_int(summary.free.bytes, ==, 0);      \
    assert_int(summary.free.blocks, ==, 0);     \
    assert_int(summary.taken.bytes, ==, 0);     \
    assert_int(summary.taken.blocks, ==, 0);    \
    assert_int(summary.sections.total, ==, 0);  \
    assert_int(summary.sections.free, ==, 0);   \
    assert_int(summary.sections.taken, ==, 0);  \
} while(0)

struct tiny_test_section {
    bool taken;
    size_t size;
};

#define ASSERT_HEAP(...) do {                                           \
    size_t alignment = tiny_block_size();                               \
    struct tiny_test_section sections[] = __VA_ARGS__;                  \
    tiny_section section = tiny_next_section(NULL);                     \
    size_t i = 0, size = sizeof(sections) / sizeof(sections[0]);        \
    size_t taken_blocks = 0, free_blocks = 0, total_blocks = 0;         \
    size_t taken_sections = 0, free_sections = 0;                       \
    while(section.data) {                                               \
        assert_size(i, <, size);                                        \
        assert_int(section.taken, ==, sections[i].taken);               \
        assert_size(section.size.blocks, ==, sections[i].size);         \
        if(section.taken) {                                             \
            taken_sections++;                                           \
            taken_blocks += section.size.blocks;                        \
        } else {                                                        \
            free_sections++;                                            \
            free_blocks += section.size.blocks;                         \
        }                                                               \
        total_blocks +=                                                 \
            section.size.blocks + OBJ_BLOCKS(size_t, alignment);        \
        i++;                                                            \
        section = tiny_next_section(section.header);                    \
    }                                                                   \
    total_blocks -= OBJ_BLOCKS(size_t, alignment);                      \
    assert_size(i, ==, size);                                           \
    tiny_summary summary = tiny_inspect();                              \
    assert_int(summary.total.blocks, ==, total_blocks);                 \
    assert_int(summary.taken.blocks, ==, taken_blocks);                 \
    assert_int(summary.free.blocks, ==, free_blocks);                   \
    assert_int(summary.sections.total, ==, i);                          \
    assert_int(summary.sections.free, ==, free_sections);               \
    assert_int(summary.sections.taken, ==, taken_sections);             \
} while(0)

#endif /* end of guard: TINY_TEST_HELPERS_H */