// Full regression test suite for the C compiler
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name, src, expected) do { \
    tests_run++; \
    FILE *f = fopen("/tmp/cc_test.c", "w"); \
    fprintf(f, "%s", src); \
    fclose(f); \
    \
    char cmd[512]; \
    snprintf(cmd, sizeof(cmd), "./compiler /tmp/cc_test.c -o /tmp/cc_test 2>&1"); \
    int compile_result = system(cmd); \
    \
    if (compile_result != 0) { \
        printf("  [SKIP] %s (compilation failed)\n", name); \
    } else { \
        int run_result = system("/tmp/cc_test"); \
        int exit_code = WEXITSTATUS(run_result); \
        if (exit_code == expected) { \
            tests_passed++; \
            printf("  [PASS] %s\n", name); \
        } else { \
            printf("  [FAIL] %s (expected %d, got %d)\n", name, expected, exit_code); \
        } \
    } \
} while(0)

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("===========================================\n");
    printf("Compiler Full Regression Tests\n");
    printf("===========================================\n");
    
    printf("\n--- Basic Control Flow ---\n");
    TEST("Simple return 0", "int main() { return 0; }", 0);
    TEST("Simple return 42", "int main() { return 42; }", 42);
    TEST("Return negative", "int main() { return -5; }", -5);
    
    printf("\n--- Arithmetic Operations ---\n");
    TEST("Addition", "int main() { return 2 + 3; }", 5);
    TEST("Subtraction", "int main() { return 10 - 3; }", 7);
    TEST("Multiplication", "int main() { return 4 * 5; }", 20);
    TEST("Division", "int main() { return 20 / 4; }", 5);
    TEST("Modulo", "int main() { return 17 % 5; }", 2);
    TEST("Precedence", "int main() { return 2 + 3 * 4; }", 14);
    TEST("Parens precedence", "int main() { return (2 + 3) * 4; }", 20);
    TEST("Left associativity", "int main() { return 10 - 5 - 2; }", 3);
    
    printf("\n--- Variables ---\n");
    TEST("Simple variable", "int main() { int x = 5; return x; }", 5);
    TEST("Variable assignment", "int main() { int x; x = 10; return x; }", 10);
    TEST("Multiple variables", "int main() { int x = 3; int y = 4; return x + y; }", 7);
    TEST("Variable shadowing", "int main() { int x = 5; { int x = 10; } return x; }", 5);
    
    printf("\n--- Function Calls ---\n");
    TEST("Function return", "int f() { return 42; } int main() { return f(); }", 42);
    TEST("Function with param", "int f(int x) { return x; } int main() { return f(7); }", 7);
    TEST("Function with two params", "int add(int a, int b) { return a + b; } int main() { return add(3, 4); }", 7);
    TEST("Nested calls", "int f(int x) { return x + 1; } int main() { return f(f(f(0))); }", 3);
    TEST("Recursive", "int fact(int n) { if (n <= 1) return 1; return n * fact(n - 1); } int main() { return fact(5); }", 120);
    
    printf("\n--- Comparison ---\n");
    TEST("Less than true", "int main() { return (3 < 5) ? 1 : 0; }", 1);
    TEST("Less than false", "int main() { return (5 < 3) ? 1 : 0; }", 0);
    TEST("Greater than true", "int main() { return (5 > 3) ? 1 : 0; }", 1);
    TEST("Equal true", "int main() { return (5 == 5) ? 1 : 0; }", 1);
    TEST("Equal false", "int main() { return (5 == 3) ? 1 : 0; }", 0);
    TEST("Not equal true", "int main() { return (5 != 3) ? 1 : 0; }", 1);
    
    printf("\n--- Control Flow ---\n");
    TEST("If true", "int main() { if (1) return 5; return 0; }", 5);
    TEST("If false", "int main() { if (0) return 5; return 3; }", 3);
    TEST("If-else", "int main() { if (1) return 1; else return 2; }", 1);
    TEST("While loop", "int main() { int i = 0; while (i < 5) i = i + 1; return i; }", 5);
    TEST("For loop", "int main() { int i; int sum = 0; for (i = 0; i < 5; i = i + 1) sum = sum + i; return sum; }", 10);
    
    printf("\n===========================================\n");
    printf("Regression Test Summary:\n");
    printf("  Tests run: %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_run - tests_passed);
    printf("===========================================\n");
    
    return tests_passed == tests_run ? 0 : 1;
}
