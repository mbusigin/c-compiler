// Fast C Compiler Regression Tests
// This test harness properly handles:
// - Compilation failures (reported as failures, not silently ignored)
// - Timeout detection (detects infinite loops)
// - Proper exit codes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static volatile sig_atomic_t timed_out = 0;

// Timeout in seconds for running compiled programs
#define RUN_TIMEOUT 5

void timeout_handler(int sig) {
    (void)sig;
    timed_out = 1;
}

// Run a command with timeout, returns: 0=ok, 1=timeout, -1=error
int run_with_timeout(const char *cmd, int timeout_sec, int *exit_code) {
    timed_out = 0;
    
    struct sigaction sa;
    sa.sa_handler = timeout_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    struct sigaction old_sa_alarm;
    sigaction(SIGALRM, &sa, &old_sa_alarm);
    
    pid_t pid = fork();
    if (pid < 0) {
        sigaction(SIGALRM, &old_sa_alarm, NULL);
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        sigaction(SIGALRM, &old_sa_alarm, NULL);
        alarm(timeout_sec);
        execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }
    
    // Parent process
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            sigaction(SIGALRM, &old_sa_alarm, NULL);
            return -1;
        }
    }
    
    alarm(0);
    sigaction(SIGALRM, &old_sa_alarm, NULL);
    
    if (timed_out) {
        // Kill any remaining child processes
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        return 1;
    }
    
    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
        return 0;
    } else if (WIFSIGNALED(status)) {
        *exit_code = 128 + WTERMSIG(status);
        return 0;
    }
    
    return -1;
}

// Run compilation (no timeout needed)
int compile_program(const char *src_file, const char *out_file) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./compiler %s -o %s 2>/dev/null", src_file, out_file);
    return system(cmd);
}

#define TEST(name, src, expected) do { \
    tests_run++; \
    FILE *f = fopen("/tmp/cc_test.c", "w"); \
    if (!f) { \
        printf("  [ERROR] %s (could not create temp file)\n", name); \
        tests_failed++; \
    } else { \
        fprintf(f, "%s", src); \
        fclose(f); \
        \
        int compile_result = compile_program("/tmp/cc_test.c", "/tmp/cc_test"); \
        \
        if (compile_result != 0) { \
            printf("  [FAIL] %s (compilation failed)\n", name); \
            tests_failed++; \
        } else { \
            int exit_code = -1; \
            int run_status = run_with_timeout("/tmp/cc_test", RUN_TIMEOUT, &exit_code); \
            \
            if (run_status == 1) { \
                printf("  [FAIL] %s (TIMEOUT - infinite loop)\n", name); \
                tests_failed++; \
            } else if (run_status < 0) { \
                printf("  [ERROR] %s (run error)\n", name); \
                tests_failed++; \
            } else if (exit_code == expected) { \
                tests_passed++; \
                printf("  [PASS] %s\n", name); \
            } else { \
                tests_failed++; \
                printf("  [FAIL] %s (exp %d, got %d)\n", name, expected, exit_code); \
            } \
        } \
    } \
} while(0)

int main() {
    printf("===========================================\n");
    printf("C Compiler Fast Regression Tests\n");
    printf("===========================================\n\n");
    
    // BASIC CONTROL FLOW
    printf("--- Basic Control Flow ---\n");
    TEST("Return 0", "int main() { return 0; }", 0);
    TEST("Return 1", "int main() { return 1; }", 1);
    TEST("Return 42", "int main() { return 42; }", 42);
    TEST("Return 255", "int main() { return 255; }", 255);
    
    // ARITHMETIC
    printf("\n--- Arithmetic ---\n");
    TEST("Add", "int main() { return 2 + 3; }", 5);
    TEST("Sub", "int main() { return 10 - 3; }", 7);
    TEST("Mul", "int main() { return 4 * 5; }", 20);
    TEST("Div", "int main() { return 20 / 4; }", 5);
    TEST("Mod", "int main() { return 17 % 5; }", 2);
    TEST("Precedence", "int main() { return 2 + 3 * 4; }", 14);
    TEST("Parens", "int main() { return (2 + 3) * 4; }", 20);
    
    // COMPARISONS
    printf("\n--- Comparisons ---\n");
    TEST("LT true", "int main() { return (3 < 5) ? 1 : 0; }", 1);
    TEST("LT false", "int main() { return (5 < 3) ? 1 : 0; }", 0);
    TEST("GT true", "int main() { return (5 > 3) ? 1 : 0; }", 1);
    TEST("EQ true", "int main() { return (5 == 5) ? 1 : 0; }", 1);
    TEST("EQ false", "int main() { return (5 == 3) ? 1 : 0; }", 0);
    TEST("NE true", "int main() { return (5 != 3) ? 1 : 0; }", 1);
    
    // LOGICAL
    printf("\n--- Logical ---\n");
    TEST("And true", "int main() { return (1 && 1) ? 1 : 0; }", 1);
    TEST("And false", "int main() { return (1 && 0) ? 1 : 0; }", 0);
    TEST("Or true", "int main() { return (0 || 1) ? 1 : 0; }", 1);
    TEST("Or false", "int main() { return (0 || 0) ? 1 : 0; }", 0);
    TEST("Not", "int main() { return (!0) ? 1 : 0; }", 1);
    
    // BITWISE
    printf("\n--- Bitwise ---\n");
    TEST("Bitand", "int main() { return (0xFF & 0x0F); }", 15);
    TEST("Bitor", "int main() { return (0xF0 | 0x0F); }", 255);
    TEST("Xor", "int main() { return (0xAA ^ 0x55); }", 255);
    TEST("Shl", "int main() { return (1 << 3); }", 8);
    TEST("Shr", "int main() { return (16 >> 2); }", 4);
    
    // IF/ELSE
    printf("\n--- If/Else ---\n");
    TEST("If true", "int main() { if (1) return 42; return 0; }", 42);
    TEST("If false", "int main() { if (0) return 42; return 0; }", 0);
    TEST("If-else true", "int main() { if (1) return 1; else return 2; }", 1);
    TEST("If-else false", "int main() { if (0) return 1; else return 2; }", 2);
    
    // WHILE
    printf("\n--- While ---\n");
    TEST("While 5x", "int main() { int i = 0; while (i < 5) i++; return i; }", 5);
    TEST("While sum", "int main() { int i = 0; int s = 0; while (i < 10) { s += i; i++; } return s; }", 45);
    TEST("While break", "int main() { int i = 0; while (1) { i++; if (i >= 10) break; } return i; }", 10);
    
    // FOR
    printf("\n--- For ---\n");
    TEST("For 5x", "int main() { int s = 0; for (int i = 0; i < 5; i++) s++; return s; }", 5);
    TEST("For sum", "int main() { int s = 0; for (int i = 1; i <= 10; i++) s += i; return s; }", 55);
    TEST("Nested for", "int main() { int s = 0; for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) s++; return s; }", 9);
    
    // BREAK/CONTINUE
    printf("\n--- Break/Continue ---\n");
    TEST("Break", "int main() { int i; for (i = 0; i < 100; i++) { if (i == 5) break; } return i; }", 5);
    TEST("Continue", "int main() { int s = 0; for (int i = 0; i < 5; i++) { if (i == 2) continue; s++; } return s; }", 4);
    
    // TERNARY
    printf("\n--- Ternary ---\n");
    TEST("Ternary true", "int main() { return (1 ? 42 : 0); }", 42);
    TEST("Ternary false", "int main() { return (0 ? 42 : 0); }", 0);
    
    // FUNCTIONS
    printf("\n--- Functions ---\n");
    TEST("Func ret 42", "int f() { return 42; } int main() { return f(); }", 42);
    TEST("Func param", "int f(int x) { return x; } int main() { return f(5); }", 5);
    TEST("Func add", "int add(int a, int b) { return a + b; } int main() { return add(3, 4); }", 7);
    TEST("Func nested", "int f(int x) { return x + 1; } int main() { return f(f(f(0))); }", 3);
    TEST("Func mult", "int add(int a, int b) { return a + b; } int mul(int a, int b) { return a * b; } int main() { return add(mul(2, 3), mul(4, 5)); }", 26);
    
    // RECURSION (small)
    printf("\n--- Recursion ---\n");
    TEST("Fact 5", "int fact(int n) { if (n <= 1) return 1; return n * fact(n-1); } int main() { return fact(5); }", 120);
    TEST("Sum n", "int sum(int n) { if (n <= 0) return 0; return n + sum(n-1); } int main() { return sum(10); }", 55);
    
    // COMPOUND
    printf("\n--- Compound ---\n");
    TEST("Plus assign", "int main() { int a = 5; a += 3; return a; }", 8);
    TEST("Pre inc", "int main() { int a = 5; return ++a; }", 6);
    TEST("Post inc", "int main() { int a = 5; int b = a++; return b; }", 5);
    TEST("Pre dec", "int main() { int a = 5; return --a; }", 4);
    
    // SUMMARY
    printf("\n===========================================\n");
    printf("Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("===========================================\n");
    return tests_failed == 0 ? 0 : 1;
}
