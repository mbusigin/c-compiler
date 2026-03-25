/**
 * test_wasm.c - WASM backend test suite
 * 
 * Tests the WASM code generator by compiling test programs and validating output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../common/test_framework.h"

// Test that WASM backend generates valid WAT for arithmetic
bool test_wasm_arithmetic(void) {
    TEST_START("wasm_arithmetic");
    int result = system("./compiler --target=wasm tests/wasm/wasm_test_arith.c -o /tmp/test_arith.wat 2>/dev/null");
    TEST_ASSERT(result == 0, "Compilation failed");
    
    FILE *f = fopen("/tmp/test_arith.wat", "r");
    TEST_ASSERT(f != NULL, "Could not open output file");
    
    char buf[1024];
    int found_const = 0, found_add = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "i32.const")) found_const = 1;
        if (strstr(buf, "i32.add")) found_add = 1;
    }
    fclose(f);
    
    TEST_ASSERT(found_const, "Missing i32.const");
    TEST_ASSERT(found_add, "Missing i32.add");
    TEST_PASS();
    return true;
}

// Test that WASM backend generates control flow instructions
bool test_wasm_control_flow(void) {
    TEST_START("wasm_control_flow");
    int result = system("./compiler --target=wasm tests/wasm/wasm_test_branch.c -o /tmp/test_branch.wat 2>/dev/null");
    TEST_ASSERT(result == 0, "Compilation failed");
    
    FILE *f = fopen("/tmp/test_branch.wat", "r");
    TEST_ASSERT(f != NULL, "Could not open output file");
    
    char buf[1024];
    int found_br_if = 0, found_loop = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "br_if")) found_br_if = 1;
        if (strstr(buf, "loop")) found_loop = 1;
    }
    fclose(f);
    
    TEST_ASSERT(found_br_if, "Missing br_if");
    TEST_ASSERT(found_loop, "Missing loop");
    TEST_PASS();
    return true;
}

// Test that WASM backend generates comparison instructions
bool test_wasm_comparison(void) {
    TEST_START("wasm_comparison");
    int result = system("./compiler --target=wasm tests/wasm/wasm_test_cmp.c -o /tmp/test_cmp.wat 2>/dev/null");
    TEST_ASSERT(result == 0, "Compilation failed");
    
    FILE *f = fopen("/tmp/test_cmp.wat", "r");
    TEST_ASSERT(f != NULL, "Could not open output file");
    
    char buf[1024];
    int found_cmp = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "i32.lt_s") || strstr(buf, "i32.gt_s") || strstr(buf, "i32.eq")) {
            found_cmp = 1;
            break;
        }
    }
    fclose(f);
    
    TEST_ASSERT(found_cmp, "Missing comparison");
    TEST_PASS();
    return true;
}

// Test that WASM backend generates memory operations
bool test_wasm_memory(void) {
    TEST_START("wasm_memory");
    int result = system("./compiler --target=wasm tests/wasm/wasm_test_memory.c -o /tmp/test_mem.wat 2>/dev/null");
    TEST_ASSERT(result == 0, "Compilation failed");
    
    FILE *f = fopen("/tmp/test_mem.wat", "r");
    TEST_ASSERT(f != NULL, "Could not open output file");
    
    char buf[1024];
    int found_load = 0, found_store = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "i32.load")) found_load = 1;
        if (strstr(buf, "i32.store")) found_store = 1;
    }
    fclose(f);
    
    TEST_ASSERT(found_load, "Missing i32.load");
    TEST_ASSERT(found_store, "Missing i32.store");
    TEST_PASS();
    return true;
}

// Test that WASM backend generates bitwise operations
bool test_wasm_bitwise(void) {
    TEST_START("wasm_bitwise");
    int result = system("./compiler --target=wasm tests/wasm/wasm_test_bitwise.c -o /tmp/test_bitwise.wat 2>/dev/null");
    TEST_ASSERT(result == 0, "Compilation failed");
    
    FILE *f = fopen("/tmp/test_bitwise.wat", "r");
    TEST_ASSERT(f != NULL, "Could not open output file");
    
    char buf[1024];
    int found_and = 0, found_or = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "i32.and")) found_and = 1;
        if (strstr(buf, "i32.or")) found_or = 1;
    }
    fclose(f);
    
    TEST_ASSERT(found_and, "Missing i32.and");
    TEST_ASSERT(found_or, "Missing i32.or");
    TEST_PASS();
    return true;
}

// Test that WASM backend generates loop constructs
bool test_wasm_loops(void) {
    TEST_START("wasm_loops");
    int result = system("./compiler --target=wasm tests/wasm/wasm_test_loop.c -o /tmp/test_loop.wat 2>/dev/null");
    TEST_ASSERT(result == 0, "Compilation failed");
    
    FILE *f = fopen("/tmp/test_loop.wat", "r");
    TEST_ASSERT(f != NULL, "Could not open output file");
    
    char buf[1024];
    int found_loop = 0, found_local = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "loop")) found_loop = 1;
        if (strstr(buf, "local.get") || strstr(buf, "local.set")) found_local = 1;
    }
    fclose(f);
    
    TEST_ASSERT(found_loop, "Missing loop");
    TEST_ASSERT(found_local, "Missing local ops");
    TEST_PASS();
    return true;
}

// Test that WASM backend generates function calls
bool test_wasm_functions(void) {
    TEST_START("wasm_functions");
    int result = system("./compiler --target=wasm tests/wasm/wasm_test_arith.c -o /tmp/test_func.wat 2>/dev/null");
    TEST_ASSERT(result == 0, "Compilation failed");
    
    FILE *f = fopen("/tmp/test_func.wat", "r");
    TEST_ASSERT(f != NULL, "Could not open output file");
    
    char buf[1024];
    int found_func = 0, found_call = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, "(func")) found_func = 1;
        if (strstr(buf, "call")) found_call = 1;
    }
    fclose(f);
    
    TEST_ASSERT(found_func, "Missing func");
    TEST_ASSERT(found_call, "Missing call");
    TEST_PASS();
    return true;
}

int main(void) {
    printf("===========================================\n");
    printf("WASM Backend Test Suite\n");
    printf("===========================================\n\n");
    
    int passed = 0, failed = 0;
    
    if (test_wasm_arithmetic()) passed++; else failed++;
    if (test_wasm_control_flow()) passed++; else failed++;
    if (test_wasm_comparison()) passed++; else failed++;
    if (test_wasm_memory()) passed++; else failed++;
    if (test_wasm_bitwise()) passed++; else failed++;
    if (test_wasm_loops()) passed++; else failed++;
    if (test_wasm_functions()) passed++; else failed++;
    
    printf("\n===========================================\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("===========================================\n");
    
    return failed > 0 ? 1 : 0;
}
