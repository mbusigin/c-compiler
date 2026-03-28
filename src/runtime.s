# runtime.s - Minimal runtime for compiler bootstrap (ARM64)
# These functions are called from compiled code

.text

# Note: We need BOTH 3-underscore and 4-underscore versions:
# - GCC adds an extra underscore on macOS, so extern "___builtin_va_start" becomes "____builtin_va_start"
# - Our compiler emits "___builtin_va_start" directly
# Provide both to satisfy all callers

.globl ___builtin_va_start
.globl ____builtin_va_start
___builtin_va_start:
____builtin_va_start:
    # x0 = pointer to va_list structure
    # x1 = address of last named param (not used)
    
    # On ARM64 Darwin, va_list is a single pointer to the stack save area
    # For variadic functions, the prologue saved x1-x7 at [x29 - 56]
    # x29 = sp + 112 for variadic functions
    # So the save area is at [x29 - 56] which is [sp + 56]
    
    # Store the address of the save area in va_list[0]
    sub x8, x29, #56
    str x8, [x0, #0]
    
    ret

.globl ___builtin_va_end
.globl ____builtin_va_end
___builtin_va_end:
____builtin_va_end:
    # Nothing to clean up
    ret

.globl ___builtin_va_copy
.globl ____builtin_va_copy
___builtin_va_copy:
____builtin_va_copy:
    # Copy va_list (just a pointer on Darwin)
    ldr x8, [x1, #0]
    str x8, [x0, #0]
    ret
