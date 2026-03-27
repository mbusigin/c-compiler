# runtime.s - Minimal runtime for compiler bootstrap (ARM64)
# These functions are called from compiled code

.text
.globl ___builtin_va_start
___builtin_va_start:
    ret

.globl ___builtin_va_end
___builtin_va_end:
    ret

.globl ___builtin_va_copy
___builtin_va_copy:
    ldr x2, [x1]
    str x2, [x0]
    ret

# Global variables from driver.c
.section __DATA,__data
.p2align 3
.globl _VERSION
_VERSION:
    .quad l_.version_string

.globl _COMPILER_NAME
_COMPILER_NAME:
    .quad l_.compiler_name

# Global variables from common/error.c
.globl _error_count
_error_count:
    .quad 0

.globl _warning_count
_warning_count:
    .quad 0

# Global variables from runtime.c
.globl _x8_temp_type
_x8_temp_type:
    .quad 0

.globl _x9_temp_type
_x9_temp_type:
    .quad 0

# Global variables from backend/wasm_emit.c
.globl _wasm_stack_ptr
_wasm_stack_ptr:
    .quad 65536

# Static const from analyzer.c (should be inlined, but we need a fallback)
.globl _MAX_RECURSION
_MAX_RECURSION:
    .quad 200

# vtables for target backends (these should be defined in their respective files)
.globl _arm64_vtable
_arm64_vtable:
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0

.globl _wasm_vtable
_wasm_vtable:
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0

.section __TEXT,__cstring,cstring_literals
l_.version_string:
    .asciz "0.1.0"
l_.compiler_name:
    .asciz "c-compiler"
