#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"
#include "helpers.h"
#include <stdint.h>

static MunitResult test_init_clear_reset(const MunitParameter params[], void* fixture) {
    tiny_reset();
    tiny_summary initial = tiny_inspect();
    const size_t alignment = initial.alignment;

    if(initial.static_buffer) {
        assert_ptr_equal(initial.static_buffer, initial.buffer);
    }

    tiny_clear();
    ASSERT_OP(CLEAR, true, 0);
    ASSERT_HEAP_EMPTY;
    
    tiny_summary cleared = tiny_inspect();
    assert_ptr_null(cleared.buffer);
    assert_size(cleared.total.bytes, ==, 0);

    unsigned char buffer[1024] = {0};

    tiny_init(buffer, alignment * 2);
    ASSERT_OP(INIT, false, alignment * 2);

    tiny_init(buffer, 1024);
    ASSERT_OP(INIT, true, 1024);
    
    size_t usable = buffer + 1024 - ALIGN_PTR(buffer, alignment);
    size_t header_blocks = OBJ_BLOCKS(size_t, alignment);
    ASSERT_HEAP({ { false, usable / alignment - 2 * header_blocks } });

    tiny_summary init = tiny_inspect();
    assert_ptr_equal(init.buffer, buffer);

    tiny_reset();
    ASSERT_OP(RESET, true, initial.total.blocks);

    tiny_summary reset = tiny_inspect();
    assert_ptr_equal(reset.buffer, initial.static_buffer);

    return MUNIT_OK;
}

static MunitResult test_malloc(const MunitParameter params[], void *fixture) {
    
    DECLARE_HEAP(2048, 3.5, 3);

    void *obj1 = tiny_malloc(obj_size);
    assert_ptr_not_null(obj1);
    ASSERT_OP(MALLOC, true, obj_size);
    ASSERT_HEAP({
        { true, obj_blocks },
        { false, available_blocks - obj_section }
    });

    void *obj2 = tiny_malloc(obj_size);
    assert_ptr_not_null(obj2);
    ASSERT_OP(MALLOC, true, obj_size);
    ASSERT_HEAP({
        { true, obj_blocks },
        { true, obj_blocks },
        { false, obj_blocks - header_blocks }
    });

    void *obj3 = tiny_malloc(obj_size);
    assert_ptr_null(obj3);
    ASSERT_OP(MALLOC, false, obj_size);
    ASSERT_HEAP({
        { true, obj_blocks },
        { true, obj_blocks },
        { false, obj_blocks - header_blocks }
    });

    void *obj4 = tiny_malloc((obj_blocks - header_blocks - 1) * alignment);
    assert_ptr_not_null(obj4);
    ASSERT_OP(MALLOC, true, (obj_blocks - header_blocks - 1) * alignment);
    ASSERT_HEAP({
        { true, obj_blocks },
        { true, obj_blocks },
        { true, obj_blocks - header_blocks }
    });

    void *obj5 = tiny_malloc(SIZE_MAX);
    assert_ptr_null(obj5);
    ASSERT_OP(MALLOC, false, SIZE_MAX);

    tiny_clear();

    void *obj6 = tiny_malloc(obj_size);
    assert_ptr_null(obj6);
    ASSERT_OP(MALLOC, false, obj_size);
    ASSERT_HEAP_EMPTY;

    return MUNIT_OK;
}

static MunitResult test_realloc(const MunitParameter params[], void *fixture) {
    DECLARE_HEAP(2048, 2.7, 10);

    void *obj1 = tiny_realloc(NULL, 20 * obj_size);
    assert_ptr_null(obj1);
    ASSERT_OP(REALLOC, false, 20 * obj_size);

    void *obj2 = tiny_realloc(NULL, obj_size);
    assert_ptr_not_null(obj2);
    ASSERT_OP(REALLOC, true, obj_size);
    ASSERT_HEAP({
        { true, obj_blocks },
        { false, available_blocks - obj_section }
    });

    void *obj3 = tiny_realloc(obj2, 2 * obj_size);
    assert_ptr_equal(obj2, obj3);
    ASSERT_OP(REALLOC, true, 2 * obj_size);
    ASSERT_HEAP({
        { true, 2 * obj_blocks },
        { false, available_blocks - obj_section - obj_blocks }
    });

    void *obj4 = tiny_realloc(NULL, obj_size);
    assert_ptr_not_null(obj4);
    ASSERT_OP(REALLOC, true, obj_size);
    ASSERT_HEAP({
        { true, 2 * obj_blocks },
        { true, obj_blocks },
        { false, available_blocks - 2 * obj_section - obj_blocks }
    });

    void *obj5 = tiny_realloc(obj3, 3 * obj_size);
    assert_ptr_not_null(obj5);
    assert_ptr_not_equal(obj5, obj3);
    ASSERT_OP(REALLOC, true, 3 * obj_size);
    ASSERT_HEAP({
        { false, 2 * obj_blocks },
        { true, obj_blocks },
        { true, 3 * obj_blocks },
        { false, available_blocks - 3 * obj_section - 3 * obj_blocks }
    });

    tiny_clear();
    void *obj6 = tiny_realloc(NULL, obj_size);
    assert_ptr_null(obj6);
    ASSERT_OP(REALLOC, false, obj_size);
    ASSERT_HEAP_EMPTY;

    return MUNIT_OK;
}

static MunitResult test_calloc(const MunitParameter paramsp[], void *fixture) {
    DECLARE_HEAP(2048, 2, 10);

    void *obj1 = tiny_calloc(3, obj_size);
    assert_ptr_not_null(obj1);
    ASSERT_OP(CALLOC, true, 3 * obj_size);
    ASSERT_HEAP({
        { true, 3 * obj_blocks },
        { false, available_blocks - obj_section - 2 * obj_blocks }
    });
    for(size_t i = 0; i < 3 * obj_size; i++) {
        assert_int(((unsigned char *)obj1)[i], ==, 0);
    }

    void *obj2 = tiny_calloc(2, SIZE_MAX);
    assert_ptr_null(obj2);
    ASSERT_OP(CALLOC, false, SIZE_MAX);

    void *obj3 = tiny_calloc(1, SIZE_MAX);
    assert_ptr_null(obj3);
    ASSERT_OP(CALLOC, false, SIZE_MAX);

    void *obj4 = tiny_calloc(1, 0);
    assert_ptr_null(obj4);
    ASSERT_OP(CALLOC, false, 0);

    void *obj5 = tiny_calloc(0, 1);
    assert_ptr_null(obj5);
    ASSERT_OP(CALLOC, false, 1);

    tiny_clear();

    void *obj6 = tiny_calloc(1, 1);
    assert_ptr_null(obj6);
    ASSERT_OP(CALLOC, false, 1);

    return MUNIT_OK;
}

static MunitResult test_free(const MunitParameter params[], void *fixture) {
    DECLARE_HEAP(2048, 5.3, 5);

    void *obj1 = tiny_malloc(obj_size);
    ASSERT_OP(MALLOC, true, obj_size);

    void *obj2 = tiny_malloc(obj_size);
    ASSERT_OP(MALLOC, true, obj_size);

    void *obj3 = tiny_malloc(obj_size);
    ASSERT_OP(MALLOC, true, obj_size);

    ASSERT_HEAP({
        { true, obj_blocks },
        { true, obj_blocks },
        { true, obj_blocks },
        { false, 2 * obj_blocks }
    });

    tiny_free(obj2);
    ASSERT_OP(FREE, true, obj_blocks);
    ASSERT_HEAP({
        { true, obj_blocks },
        { false, obj_blocks },
        { true, obj_blocks },
        { false, 2 * obj_blocks }
    });

    tiny_free(obj1);
    ASSERT_OP(FREE, true, obj_blocks);
    ASSERT_HEAP({
        { false, 2 * obj_blocks + header_blocks },
        { true, obj_blocks },
        { false, 2 * obj_blocks }
    });

    tiny_free(obj3);
    ASSERT_OP(FREE, true, obj_blocks);
    ASSERT_HEAP({
        { false, available_blocks }
    });

    tiny_free(NULL);
    ASSERT_OP(FREE, false, 0);

    return MUNIT_OK;
}

static MunitResult test_out_of_memory(const MunitParameter params[], void *fixture) {
    DECLARE_HEAP(2048, 1, 10);

    tiny_out_of_memory(true);

    void *obj1 = tiny_malloc(obj_size);
    assert_ptr_null(obj1);
    ASSERT_OP(MALLOC, false, obj_size);

    void *obj2 = tiny_realloc((void *)1, obj_size);
    assert_ptr_null(obj2);
    ASSERT_OP(REALLOC, false, obj_size);

    void *obj3 = tiny_calloc(1, obj_size);
    assert_ptr_null(obj3);
    ASSERT_OP(CALLOC, false, obj_size);

    tiny_out_of_memory(false);

    void *obj4 = tiny_malloc(obj_size);
    assert_ptr_not_null(obj4);
    ASSERT_OP(MALLOC, true, obj_size);
    ASSERT_HEAP({
        { true, obj_blocks },
        { false, available_blocks - obj_section }
    });

    void *obj5 = tiny_realloc(NULL, obj_size);
    assert_ptr_not_null(obj5);
    ASSERT_OP(REALLOC, true, obj_size);
    ASSERT_HEAP({
        { true, obj_blocks },
        { true, obj_blocks },
        { false, available_blocks - 2 * obj_section }
    });

    void *obj6 = tiny_calloc(1, obj_size);
    assert_ptr_not_null(obj6);
    ASSERT_OP(CALLOC, true, obj_size);
    ASSERT_HEAP({
        { true, obj_blocks },
        { true, obj_blocks },
        { true, obj_blocks },
        { false, available_blocks - 3 * obj_section }
    });

    return MUNIT_OK;
}

static MunitTest tests[] = {
    { 
        "/init-clear-reset", 
        test_init_clear_reset, 
        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL 
    },
    {
        "/malloc",
        test_malloc,
        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
    },
    {
        "/realloc",
        test_realloc,
        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
    },
    {
        "/calloc",
        test_calloc,
        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
    },
    {
        "/free",
        test_free,
        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
    },
    {
        "/out-of-memory",
        test_out_of_memory,
        NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
    },
    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite suite = {
    "/tiny-test",
    tests,
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char * const argv[]) {
    return munit_suite_main(&suite, NULL, argc, argv);
}
