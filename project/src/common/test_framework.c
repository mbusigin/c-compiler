/**
 * test_framework.c - Simple test framework implementation
 */

#include "test_framework.h"
#include <stdio.h>
#include <string.h>

int run_tests(const TestCase *tests, int count) {
    int passed = 0;
    int failed = 0;
    
    printf("\n=== Running %d tests ===\n\n", count);
    
    for (int i = 0; i < count; i++) {
        printf("[%d/%d] %s: ", i + 1, count, tests[i].name);
        fflush(stdout);
        
        if (tests[i].func()) {
            passed++;
        } else {
            failed++;
        }
    }
    
    printf("\n=== Test Results: %d passed, %d failed ===\n", passed, failed);
    
    return failed > 0 ? 1 : 0;
}
