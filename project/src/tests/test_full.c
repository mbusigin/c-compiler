// Full regression test suite for the C compiler
// This test harness properly handles:
// - Compilation failures (reported as failures, not skipped)
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

// Run compilation (no timeout needed, but limited output capture)
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
                printf("  [FAIL] %s (expected %d, got %d)\n", name, expected, exit_code); \
            } \
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
    TEST("Return negative", "int main() { return -5; }", 251);  // -5 wraps to 251 in unsigned 8-bit exit code
    
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
    printf("  Tests failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed == 0 ? 0 : 1;
}
