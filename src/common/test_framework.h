#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>

// Test framework macros
#define TEST_START(name) printf("Running %s...\n", name)
#define TEST_PASS() printf("  PASSED\n")
#define TEST_FAIL(msg) do { printf("  FAILED: %s\n", msg); return false; } while(0)
#define TEST_ASSERT(cond, msg) do { if (!(cond)) { TEST_FAIL(msg); } } while(0)
#define TEST_ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { printf("  FAILED: %s (got %d, expected %d)\n", msg, (int)(a), (int)(b)); return false; } } while(0)
#define TEST_ASSERT_STR_EQ(a, b, msg) do { if (strcmp((a), (b)) != 0) { printf("  FAILED: %s\n", msg); return false; } } while(0)

typedef bool (*TestFunc)(void);

typedef struct {
    const char *name;
    TestFunc func;
} TestCase;

int run_tests(const TestCase *tests, int count);

#endif // TEST_FRAMEWORK_H
