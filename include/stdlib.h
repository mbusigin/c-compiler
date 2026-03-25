/* stdlib.h - standard library definitions */
#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void abort(void);
void exit(int status);
int atoi(const char *nptr);
long atol(const char *nptr);
double atof(const char *nptr);
int system(const char *command);
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
double strtod(const char *nptr, char **endptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
int rand(void);
void srand(unsigned int seed);

#endif /* STDLIB_H */
