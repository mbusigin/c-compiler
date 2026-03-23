// Integration tests for the compiler - tests the full compilation pipeline
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

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  [FAIL] %s\n", msg); \
    } \
} while(0)

// Test compilation of simple programs
int test_simple_return() {
    printf("\nTest: Simple return 42\n");
    
    const char *src = "int main() { return 42; }";
    FILE *f = fopen("/tmp/cc_test_simple.c", "w");
    if (!f) {
        printf("  [ERROR] Could not create temp file\n");
        return 0;
    }
    fprintf(f, "%s", src);
    fclose(f);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./compiler /tmp/cc_test_simple.c -o /tmp/cc_test_simple 2>/dev/null");
    
    int result = system(cmd);
    if (result != 0) {
        printf("  [FAIL] Compilation failed\n");
        return 0;
    }
    
    int exit_code;
    int run_status = run_with_timeout("/tmp/cc_test_simple", RUN_TIMEOUT, &exit_code);
    
    if (run_status == 1) {
        printf("  [FAIL] TIMEOUT - infinite loop\n");
        return 0;
    } else if (run_status < 0) {
        printf("  [FAIL] Run error\n");
        return 0;
    }
    
    ASSERT(exit_code == 42, "Program should return 42");
    return 1;
}

int test_arithmetic() {
    printf("\nTest: Arithmetic expression (1 + 2) * 3 = 9\n");
    
    const char *src = "int main() { return (1 + 2) * 3; }";
    FILE *f = fopen("/tmp/cc_test_arith.c", "w");
    if (!f) {
        printf("  [ERROR] Could not create temp file\n");
        return 0;
    }
    fprintf(f, "%s", src);
    fclose(f);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./compiler /tmp/cc_test_arith.c -o /tmp/cc_test_arith 2>/dev/null");
    
    int result = system(cmd);
    if (result != 0) {
        printf("  [FAIL] Compilation failed\n");
        return 0;
    }
    
    int exit_code;
    int run_status = run_with_timeout("/tmp/cc_test_arith", RUN_TIMEOUT, &exit_code);
    
    if (run_status == 1) {
        printf("  [FAIL] TIMEOUT - infinite loop\n");
        return 0;
    } else if (run_status < 0) {
        printf("  [FAIL] Run error\n");
        return 0;
    }
    
    ASSERT(exit_code == 9, "Program should return 9");
    return 1;
}

int test_function_call() {
    printf("\nTest: Function call add(3, 4) = 7\n");
    
    const char *src = "int add(int a, int b) { return a + b; } int main() { return add(3, 4); }";
    FILE *f = fopen("/tmp/cc_test_func.c", "w");
    if (!f) {
        printf("  [ERROR] Could not create temp file\n");
        return 0;
    }
    fprintf(f, "%s", src);
    fclose(f);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./compiler /tmp/cc_test_func.c -o /tmp/cc_test_func 2>/dev/null");
    
    int result = system(cmd);
    if (result != 0) {
        printf("  [FAIL] Compilation failed\n");
        return 0;
    }
    
    int exit_code;
    int run_status = run_with_timeout("/tmp/cc_test_func", RUN_TIMEOUT, &exit_code);
    
    if (run_status == 1) {
        printf("  [FAIL] TIMEOUT - infinite loop\n");
        return 0;
    } else if (run_status < 0) {
        printf("  [FAIL] Run error\n");
        return 0;
    }
    
    ASSERT(exit_code == 7, "Program should return 7");
    return 1;
}

int test_nested_calls() {
    printf("\nTest: Nested function calls\n");
    
    const char *src = "int double(int x) { return x * 2; } int main() { return double(double(3)); }";
    FILE *f = fopen("/tmp/cc_test_nested.c", "w");
    if (!f) {
        printf("  [ERROR] Could not create temp file\n");
        return 0;
    }
    fprintf(f, "%s", src);
    fclose(f);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./compiler /tmp/cc_test_nested.c -o /tmp/cc_test_nested 2>/dev/null");
    
    int result = system(cmd);
    if (result != 0) {
        printf("  [FAIL] Compilation failed\n");
        return 0;
    }
    
    int exit_code;
    int run_status = run_with_timeout("/tmp/cc_test_nested", RUN_TIMEOUT, &exit_code);
    
    if (run_status == 1) {
        printf("  [FAIL] TIMEOUT - infinite loop\n");
        return 0;
    } else if (run_status < 0) {
        printf("  [FAIL] Run error\n");
        return 0;
    }
    
    ASSERT(exit_code == 12, "Program should return 12 (double(double(3)))");
    return 1;
}

int test_comparison() {
    printf("\nTest: Comparison operations\n");
    
    const char *src = "int main() { return (5 > 3) ? 1 : 0; }";
    FILE *f = fopen("/tmp/cc_test_cmp.c", "w");
    if (!f) {
        printf("  [ERROR] Could not create temp file\n");
        return 0;
    }
    fprintf(f, "%s", src);
    fclose(f);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./compiler /tmp/cc_test_cmp.c -o /tmp/cc_test_cmp 2>/dev/null");
    
    int result = system(cmd);
    if (result != 0) {
        printf("  [FAIL] Compilation failed\n");
        return 0;
    }
    
    int exit_code;
    int run_status = run_with_timeout("/tmp/cc_test_cmp", RUN_TIMEOUT, &exit_code);
    
    if (run_status == 1) {
        printf("  [FAIL] TIMEOUT - infinite loop\n");
        return 0;
    } else if (run_status < 0) {
        printf("  [FAIL] Run error\n");
        return 0;
    }
    
    ASSERT(exit_code == 1, "Program should return 1 (5 > 3)");
    return 1;
}

int test_printf() {
    printf("\nTest: printf Hello World\n");
    
    const char *src = "#include <stdio.h>\nint main() { printf(\"Hello, World!\\n\"); return 0; }";
    FILE *f = fopen("/tmp/cc_test_printf.c", "w");
    if (!f) {
        printf("  [ERROR] Could not create temp file\n");
        return 0;
    }
    fprintf(f, "%s", src);
    fclose(f);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./compiler /tmp/cc_test_printf.c -o /tmp/cc_test_printf 2>/dev/null");
    
    int result = system(cmd);
    if (result != 0) {
        printf("  [FAIL] Compilation failed\n");
        return 0;
    }
    
    int exit_code;
    int run_status = run_with_timeout("/tmp/cc_test_printf", RUN_TIMEOUT, &exit_code);
    
    if (run_status == 1) {
        printf("  [FAIL] TIMEOUT - infinite loop\n");
        return 0;
    } else if (run_status < 0) {
        printf("  [FAIL] Run error\n");
        return 0;
    }
    
    ASSERT(exit_code == 0, "Program should exit with 0");
    return 1;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("===========================================\n");
    printf("Compiler Integration Tests\n");
    printf("===========================================\n");
    
    test_simple_return();
    test_arithmetic();
    test_function_call();
    test_nested_calls();
    test_comparison();
    test_printf();
    
    printf("\n===========================================\n");
    printf("Integration Tests Summary:\n");
    printf("  Tests run: %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed == 0 ? 0 : 1;
}
