/* stddef.h - standard type definitions */
#ifndef STDDEF_H
#define STDDEF_H

#define NULL ((void*)0)
#define offsetof(type, member) __builtin_offsetof(type, member)

/* Use unsigned long as size_t (common on 64-bit systems) */
typedef unsigned long size_t;
typedef long ptrdiff_t;

#endif /* STDDEF_H */
