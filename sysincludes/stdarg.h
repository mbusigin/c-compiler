#ifndef _STDARG_H
#define _STDARG_H

// On ARM64 macOS/Darwin, va_list is a pointer to the register save area.
// Our runtime.s implements ___builtin_va_start to store this pointer.
typedef void *va_list;

#define va_start(ap, param) ___builtin_va_start(&(ap), param)
#define va_end(ap) ___builtin_va_end(&(ap))
#define va_arg(ap, type) ___builtin_va_arg(&(ap), type)
#define va_copy(dest, src) ___builtin_va_copy(&(dest), &(src))

#endif
