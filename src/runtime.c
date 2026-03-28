/**
 * runtime.c - Minimal runtime for compiler bootstrap
 * 
 * This file provides implementations for compiler builtins that are
 * called from compiled code but need to be resolved at link time.
 * The actual implementations are in runtime.s (assembly).
 */

// These are typically provided by libgcc or compiler-rt
// For bootstrap purposes, we provide minimal implementations

// Global variables needed by the compiler itself when it compiles itself
// These are file-scope variables that the compiler can't yet emit
int x8_temp_type = 0;
int x9_temp_type = 0;
// wasm_stack_ptr is defined in wasm_emit.h

// The actual implementations are in runtime.s (assembly)
// These declarations ensure the linker knows these symbols exist

// Declarations for assembly implementations
// Note: Using 3 underscores to match what the compiler emits
extern void ___builtin_va_start(void *ap, ...);
extern void ___builtin_va_end(void *ap);
extern void ___builtin_va_copy(void *dst, void *src);

// For __builtin_va_copy, we need to use a different approach
// since it's a builtin. We'll define a wrapper that va_copy can call.
void __va_copy_impl(void *dst, void *src) {
    ___builtin_va_copy(dst, src);
}
