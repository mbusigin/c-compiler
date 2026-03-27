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

# Note: Global variables are now emitted by the compiler.
# Static variables have internal linkage and won't conflict.
# External variables (like stderr/stdout) are loaded via GOT.
