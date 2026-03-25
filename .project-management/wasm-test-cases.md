# WASM Test Case Specifications

## Overview

This document defines the test suite structure and test case specifications for the WASM backend, organized by category and complexity.

## Test Directory Structure

```
tests/wasm/
├── unit/
│   ├── test_arithmetic.c
│   ├── test_arithmetic.expected
│   ├── test_bitwise.c
│   └── test_bitwise.expected
├── control_flow/
│   ├── test_if.c
│   ├── test_if.expected
│   ├── test_while.c
│   └── test_while.expected
├── memory/
│   ├── test_locals.c
│   ├── test_locals.expected
│   ├── test_array.c
│   └── test_array.expected
├── functions/
│   ├── test_call.c
│   ├── test_call.expected
│   ├── test_recursive.c
│   └── test_recursive.expected
├── io/
│   ├── test_printf.c
│   ├── test_printf.expected
│   └── test_hello.c
└── integration/
    ├── test_full.c
    └── test_full.expected
```

## Category 1: Arithmetic Tests

### test_arithmetic_add
```c
// tests/wasm/unit/test_arithmetic_add.c
int add(int a, int b) {
    return a + b;
}

int main() {
    return add(3, 5);
}
// Expected: 8
```

### test_arithmetic_sub
```c
// tests/wasm/unit/test_arithmetic_sub.c
int sub(int a, int b) {
    return a - b;
}

int main() {
    return sub(10, 4);
}
// Expected: 6
```

### test_arithmetic_mul
```c
// tests/wasm/unit/test_arithmetic_mul.c
int mul(int a, int b) {
    return a * b;
}

int main() {
    return mul(7, 6);
}
// Expected: 42
```

### test_arithmetic_div
```c
// tests/wasm/unit/test_arithmetic_div.c
int divide(int a, int b) {
    return a / b;
}

int main() {
    return divide(20, 4);
}
// Expected: 5
```

### test_arithmetic_mod
```c
// tests/wasm/unit/test_arithmetic_mod.c
int mod(int a, int b) {
    return a % b;
}

int main() {
    return mod(17, 5);
}
// Expected: 2
```

### test_arithmetic_neg
```c
// tests/wasm/unit/test_arithmetic_neg.c
int negate(int a) {
    return -a;
}

int main() {
    return negate(42);
}
// Expected: -42
```

### test_arithmetic_complex
```c
// tests/wasm/unit/test_arithmetic_complex.c
int complex_expr(int a, int b, int c) {
    return (a + b) * c - (a / b);
}

int main() {
    return complex_expr(10, 2, 3);
}
// Expected: (10+2)*3 - (10/2) = 36 - 5 = 31
```

## Category 2: Bitwise Tests

### test_bitwise_and
```c
// tests/wasm/unit/test_bitwise_and.c
int bitwise_and(int a, int b) {
    return a & b;
}

int main() {
    return bitwise_and(0b1100, 0b1010);
}
// Expected: 8 (0b1000)
```

### test_bitwise_or
```c
// tests/wasm/unit/test_bitwise_or.c
int bitwise_or(int a, int b) {
    return a | b;
}

int main() {
    return bitwise_or(0b1100, 0b1010);
}
// Expected: 14 (0b1110)
```

### test_bitwise_xor
```c
// tests/wasm/unit/test_bitwise_xor.c
int bitwise_xor(int a, int b) {
    return a ^ b;
}

int main() {
    return bitwise_xor(0b1100, 0b1010);
}
// Expected: 6 (0b0110)
```

### test_bitwise_not
```c
// tests/wasm/unit/test_bitwise_not.c
int logical_not(int a) {
    return !a;
}

int main() {
    return logical_not(0);
}
// Expected: 1
```

### test_bitwise_shift
```c
// tests/wasm/unit/test_bitwise_shift.c
int shift_left(int a, int b) {
    return a << b;
}

int main() {
    return shift_left(5, 2);
}
// Expected: 20
```

## Category 3: Control Flow Tests

### test_if_simple
```c
// tests/wasm/control_flow/test_if_simple.c
int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int main() {
    return max(10, 20);
}
// Expected: 20
```

### test_if_else
```c
// tests/wasm/control_flow/test_if_else.c
int abs(int x) {
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
}

int main() {
    return abs(-15);
}
// Expected: 15
```

### test_while_loop
```c
// tests/wasm/control_flow/test_while_loop.c
int sum_to_n(int n) {
    int sum = 0;
    while (n > 0) {
        sum += n;
        n--;
    }
    return sum;
}

int main() {
    return sum_to_n(10);
}
// Expected: 55
```

### test_for_loop
```c
// tests/wasm/control_flow/test_for_loop.c
int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

int main() {
    return factorial(5);
}
// Expected: 120
```

### test_nested_if
```c
// tests/wasm/control_flow/test_nested_if.c
int classify(int x) {
    if (x > 0) {
        if (x > 100) {
            return 2;  // Large positive
        }
        return 1;  // Small positive
    } else if (x < 0) {
        return -1;  // Negative
    }
    return 0;  // Zero
}

int main() {
    return classify(50);
}
// Expected: 1
```

### test_break_continue
```c
// tests/wasm/control_flow/test_break_continue.c
int sum_skip_5(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        if (i == 5) {
            continue;  // Skip 5
        }
        if (i == 8) {
            break;  // Stop at 8
        }
        sum += i;
    }
    return sum;
}

int main() {
    return sum_skip_5(10);
}
// Expected: 0+1+2+3+4+6+7 = 23
```

## Category 4: Memory Tests

### test_locals_simple
```c
// tests/wasm/memory/test_locals_simple.c
int use_locals() {
    int a = 10;
    int b = 20;
    int c = a + b;
    return c;
}

int main() {
    return use_locals();
}
// Expected: 30
```

### test_variable_assignment
```c
// tests/wasm/memory/test_variable_assignment.c
int reassign() {
    int x = 5;
    x = x + 10;
    x = x * 2;
    return x;
}

int main() {
    return reassign();
}
// Expected: 30
```

### test_array_access
```c
// tests/wasm/memory/test_array_access.c
int array_get(int *arr, int index) {
    return arr[index];
}

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    return array_get(arr, 2);
}
// Expected: 30
```

### test_pointer_deref
```c
// tests/wasm/memory/test_pointer_deref.c
int deref(int *p) {
    return *p;
}

int main() {
    int x = 42;
    return deref(&x);
}
// Expected: 42
```

## Category 5: Function Tests

### test_function_call
```c
// tests/wasm/functions/test_function_call.c
int double_it(int x) {
    return x * 2;
}

int main() {
    return double_it(21);
}
// Expected: 42
```

### test_multiple_args
```c
// tests/wasm/functions/test_multiple_args.c
int multi_arg(int a, int b, int c, int d) {
    return a + b + c + d;
}

int main() {
    return multi_arg(1, 2, 3, 4);
}
// Expected: 10
```

### test_recursive
```c
// tests/wasm/functions/test_recursive.c
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    return fibonacci(10);
}
// Expected: 55
```

### test_mutual_recursion
```c
// tests/wasm/functions/test_mutual_recursion.c
int is_even(int n);
int is_odd(int n);

int is_even(int n) {
    if (n == 0) return 1;
    return is_odd(n - 1);
}

int is_odd(int n) {
    if (n == 0) return 0;
    return is_even(n - 1);
}

int main() {
    return is_even(10) + is_odd(10) * 2;
}
// Expected: 1 + 0*2 = 1
```

## Category 6: I/O Tests

### test_hello_world
```c
// tests/wasm/io/test_hello.c
#include <stdio.h>

int main() {
    printf("Hello, WASM!\n");
    return 0;
}
// Expected output: Hello, WASM!
```

### test_printf_int
```c
// tests/wasm/io/test_printf_int.c
#include <stdio.h>

int main() {
    int x = 42;
    printf("The answer is %d\n", x);
    return 0;
}
// Expected output: The answer is 42
```

### test_putchar
```c
// tests/wasm/io/test_putchar.c
#include <stdio.h>

int main() {
    putchar('H');
    putchar('i');
    putchar('\n');
    return 0;
}
// Expected output: Hi
```

## Category 7: Type Tests

### test_long_long
```c
// tests/wasm/types/test_long_long.c
long long big_add(long long a, long long b) {
    return a + b;
}

int main() {
    return (int)big_add(1000000LL, 2000000LL);
}
// Expected: 3000000
```

### test_float_basic
```c
// tests/wasm/types/test_float.c
float add_float(float a, float b) {
    return a + b;
}

int main() {
    return (int)(add_float(1.5f, 2.5f));
}
// Expected: 4
```

### test_type_cast
```c
// tests/wasm/types/test_cast.c
int cast_test() {
    int i = 42;
    long l = (long)i;
    return (int)l;
}

int main() {
    return cast_test();
}
// Expected: 42
```

## Test Execution Matrix

| Category | Tests | Priority | Sprint |
|----------|-------|----------|--------|
| Arithmetic | 7 | High | 1 |
| Bitwise | 5 | High | 1 |
| Control Flow | 6 | High | 2 |
| Memory | 4 | High | 3 |
| Functions | 4 | High | 4 |
| I/O | 3 | Medium | 4 |
| Types | 3 | Medium | 5 |

## Expected File Format

Each `.expected` file contains the expected output:

```
# For return value tests (single line)
42

# For printf tests (full output)
Hello, WASM!
```

## Test Runner Script

```bash
#!/bin/bash
# tests/wasm/run_tests.sh

PASSED=0
FAILED=0

for test in tests/wasm/unit/*.c; do
    name=$(basename $test .c)
    expected=$(cat tests/wasm/unit/${name}.expected)
    
    ./compiler --target=wasm $test -o /tmp/${name}.wat
    wat2wasm /tmp/${name}.wat -o /tmp/${name}.wasm
    actual=$(wasmtime /tmp/${name}.wasm --invoke main 2>&1)
    
    if [ "$expected" = "$actual" ]; then
        echo "[PASS] $name"
        PASSED=$((PASSED + 1))
    else
        echo "[FAIL] $name: expected '$expected', got '$actual'"
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "Results: $PASSED passed, $FAILED failed"
exit $FAILED
```
