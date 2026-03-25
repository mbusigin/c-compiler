/**
 * runtime.c - Minimal runtime for compiler bootstrap
 * 
 * This file provides implementations for compiler builtins that are
 * called from compiled code but need to be resolved at link time.
 */

#include <stdarg.h>

// These are typically provided by libgcc or compiler-rt
// For bootstrap purposes, we provide minimal implementations

// The compiler generates calls to ___builtin_va_* 
// These are the names the linker expects (with leading underscores)

void ___builtin_va_start(void *ap) {
    // Minimal implementation - do nothing
    // Real implementation would set up ap for variadic access
    (void)ap;
}

void ___builtin_va_end(void *ap) {
    (void)ap;
}

// For __builtin_va_copy, we need to use a different approach
// since it's a builtin. We'll define a wrapper that va_copy can call.
// Note: The compiler-generated code should be calling our functions,
// so this is just in case.
void __va_copy_impl(void *dst, void *src) {
    char **dst_list = (char **)dst;
    char **src_list = (char **)src;
    *dst_list = *src_list;
    (void)src;
}
