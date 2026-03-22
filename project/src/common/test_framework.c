/**
 * test_framework.c - Simple test framework
 */

#include "test_framework.h"
#include <stdio.h>
#include <string.h>

int run_tests(const TestCase *tests, int count) {
    int passed = 0;
    int failed = 0;
    
    printf("Running %d test(s)...\n", count);
    
    for (int i = 0; i < count; i++) {
        printf("\n[%d/%d] %s: ", i + 1, count, tests[i].name);
        fflush(stdout);
        
        if (tests[i].func()) {
            passed++;
            printf("PASSED\n");
        } else {
            failed++;
            printf("FAILED\n");
        }
    }
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", count);
    
    return failed == 0 ? 0 : 1;
}
