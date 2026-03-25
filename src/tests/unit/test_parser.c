/**
 * test_parser.c - Parser unit tests
 */

#include "../../common/test_framework.h"
#include "../../common/util.h"
#include "../../parser/ast.h"
#include <stdio.h>
#include <string.h>

static bool test_ast_creation(void) {
    TEST_START("Test AST Node Creation");
    
    ASTNode *node = ast_create(AST_FUNCTION_DECL);
    TEST_ASSERT(node != NULL, "Node should not be NULL");
    TEST_ASSERT(node->type == AST_FUNCTION_DECL, "Node type should be FUNCTION_DECL");
    
    ast_free(node);
    TEST_PASS();
    return true;
}

static bool test_type_system(void) {
    TEST_START("Test Type System");
    
    Type *t_int = type_int();
    TEST_ASSERT(t_int != NULL, "int type should not be NULL");
    TEST_ASSERT(t_int->kind == TYPE_INT, "Type kind should be INT");
    TEST_ASSERT(t_int->size == 4, "int size should be 4");
    
    Type *t_ptr = type_pointer(t_int);
    TEST_ASSERT(t_ptr != NULL, "Pointer type should not be NULL");
    TEST_ASSERT(t_ptr->kind == TYPE_POINTER, "Type kind should be POINTER");
    TEST_ASSERT(t_ptr->base == t_int, "Base type should be int");
    
    type_free(t_ptr);
    type_free(t_int);
    
    TEST_PASS();
    return true;
}

static bool test_type_array(void) {
    TEST_START("Test Array Type");
    
    Type *elem = type_int();
    Type *arr = type_array(elem, 10);
    
    TEST_ASSERT(arr->kind == TYPE_ARRAY, "Should be array type");
    TEST_ASSERT(arr->array_size == 10, "Array size should be 10");
    TEST_ASSERT(arr->size == 40, "Total array size should be 40");
    
    type_free(arr);
    
    TEST_PASS();
    return true;
}

static bool test_function_type(void) {
    TEST_START("Test Function Type");
    
    Type *ret = type_int();
    Type *param1 = type_int();
    Type *param2 = type_char();
    
    Type *func = type_create(TYPE_FUNCTION);
    func->return_type = ret;
    func->param_types = xmalloc(sizeof(Type *) * 2);
    func->param_types[0] = param1;
    func->param_types[1] = param2;
    func->num_params = 2;
    
    TEST_ASSERT(func->kind == TYPE_FUNCTION, "Should be function type");
    TEST_ASSERT(func->num_params == 2, "Should have 2 params");
    
    type_free(func);
    
    TEST_PASS();
    return true;
}

int main(void) {
    TestCase tests[] = {
        {"AST Node Creation", test_ast_creation},
        {"Type System Basics", test_type_system},
        {"Array Types", test_type_array},
        {"Function Types", test_function_type}
    };
    
    int result = run_tests(tests, sizeof(tests) / sizeof(tests[0]));
    
    if (result == 0) {
        printf("\n*** All parser tests passed ***\n");
    }
    
    return result;
}
