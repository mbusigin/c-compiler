# runtime.s - Minimal runtime for compiler bootstrap (ARM64)
# These functions are called from compiled code

.text

# ___builtin_va_start(ap, last_named_param)
# Initialize va_list structure to access variadic arguments
# On ARM64, va_list is a struct with 5 fields:
#   void *__stack  - stack args start
#   void *__gr_top - end of GP arg save area
#   void *__vr_top - end of FP/SIMD arg save area
#   long __gr_off  - offset from __gr_top
#   long __vr_off  - offset from __vr_top

# runtime.s - Minimal runtime for compiler bootstrap (ARM64)
# These functions are called from compiled code

.text

# ___builtin_va_start(ap, last_named_param)
# Initialize va_list structure to access variadic arguments
# On ARM64, va_list is a struct with 5 fields:
#   void *__stack  - stack args start
#   void *__gr_top - end of GP arg save area
#   void *__vr_top - end of FP/SIMD arg save area
#   long __gr_off  - offset from __gr_top
#   long __vr_off  - offset from __vr_top

# Note: We need BOTH 3-underscore and 4-underscore versions:
# - GCC adds an extra underscore on macOS, so extern "___builtin_va_start" becomes "____builtin_va_start"
# - Our compiler emits "___builtin_va_start" directly
# Provide both to satisfy all callers

.globl ___builtin_va_start
.globl ____builtin_va_start
___builtin_va_start:
____builtin_va_start:
    # x0 = pointer to va_list structure
    # x1 = address of last named param (not used in this simple implementation)
    
    # Simple implementation: just set up to read from stack
    # Get current stack pointer
    mov x8, sp
    
    # Initialize va_list structure
    # __stack = sp (where stack args would start)
    str x8, [x0, #0]
    
    # __gr_top = sp (general purpose register save area)
    str x8, [x0, #8]
    
    # __vr_top = sp (vector register save area)  
    str x8, [x0, #16]
    
    # __gr_off = 0 (offset to next GP arg)
    str xzr, [x0, #24]
    
    # __vr_off = 0 (offset to next FP/SIMD arg)
    str xzr, [x0, #32]
    
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
    # Copy entire va_list structure (40 bytes)
    ldp x2, x3, [x1, #0]
    stp x2, x3, [x0, #0]
    ldp x2, x3, [x1, #16]
    stp x2, x3, [x0, #16]
    ldr x2, [x1, #32]
    str x2, [x0, #32]
    ret

# Note: Global variables are now emitted by the compiler.
# Static variables have internal linkage and won't conflict.
# External variables (like stderr/stdout) are loaded via GOT.
