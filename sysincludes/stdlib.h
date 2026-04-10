#ifndef _STDLIB_H
#define _STDLIB_H

typedef unsigned long size_t;

#define NULL ((void*)0)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 2147483647

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void *aligned_alloc(size_t alignment, size_t size);

void abort(void);
void exit(int status);
void _exit(int status);
void quick_exit(int status);
int atexit(void (*func)(void));
int at_quick_exit(void (*func)(void));

char *getenv(const char *name);
int system(const char *command);

int abs(int n);
long labs(long n);
long long llabs(long long n);

int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
double atof(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

int rand(void);
void srand(unsigned int seed);

#endif
