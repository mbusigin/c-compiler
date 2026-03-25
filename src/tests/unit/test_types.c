/**
 * test_types.c - Type system unit tests
 */

#include "../../common/test_framework.h"
#include "../../common/util.h"
#include "../../parser/ast.h"
#include <stdio.h>
#include <string.h>

static bool test_basic_types(void) {
    TEST_START("Test Basic Types");
    
    struct { Type *(*creator)(void); const char *name; int expected_size; } tests[] = {
        {type_void, "void", 0},
        {type_char, "char", 1},
        {type_int, "int", 4},
        {type_long, "long", 8},
        {type_float, "float", 4},
        {type_double, "double", 8},
        {type_bool, "bool", 1}
    };
    
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Type *t = tests[i].creator();
        TEST_ASSERT(t != NULL, tests[i].name);
        TEST_ASSERT(t->size == tests[i].expected_size, tests[i].name);
        type_free(t);
    }
    
    TEST_PASS();
    return true;
}

static bool test_pointer_arithmetic(void) {
    TEST_START("Test Pointer Arithmetic");
    
    Type *int_type = type_int();
    Type *ptr = type_pointer(int_type);
    
    TEST_ASSERT(ptr->kind == TYPE_POINTER, "Should be pointer type");
    TEST_ASSERT(ptr->size == 8, "Pointer size should be 8 on 64-bit");
    
    type_free(ptr);
    type_free(int_type);
    
    TEST_PASS();
    return true;
}

static bool test_type_copy(void) {
    TEST_START("Test Type Copy");
    
    Type *original = type_int();
    Type *copy = type_copy(original);
    
    TEST_ASSERT(copy != NULL, "Copy should not be NULL");
    TEST_ASSERT(copy != original, "Copy should be different pointer");
    TEST_ASSERT(copy->kind == original->kind, "Copy should have same kind");
    TEST_ASSERT(copy->size == original->size, "Copy should have same size");
    
    type_free(copy);
    type_free(original);
    
    TEST_PASS();
    return true;
}

int main(void) {
    TestCase tests[] = {
        {"Basic Types", test_basic_types},
        {"Pointer Arithmetic", test_pointer_arithmetic},
        {"Type Copy", test_type_copy}
    };
    
    int result = run_tests(tests, sizeof(tests) / sizeof(tests[0]));
    
    if (result == 0) {
        printf("\n*** All type system tests passed ***\n");
    }
    
    return result;
}
