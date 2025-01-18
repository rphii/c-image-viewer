#ifndef MEM_H

#include <stdlib.h>

void *mem_custom_malloc(size_t size, const char *file, int line, const char *function);
void *mem_custom_calloc(size_t nmemb, size_t size, const char *file, int line, const char *function);
void *mem_custom_realloc(void *ptr, size_t size, const char *file, int line, const char *function);
void mem_custom_free(void *ptr, const char *file, int line, const char *function);

#define malloc(X)       mem_custom_malloc(X, __FILE__, __LINE__, __func__)
#define calloc(X, Y)    mem_custom_calloc(X, Y, __FILE__, __LINE__, __func__)
#define realloc(X, Y)   mem_custom_realloc(X, Y, __FILE__, __LINE__, __func__)
#define free(X)         mem_custom_free(X, __FILE__, __LINE__, __func__)

void mem_log(void);

#define MEM_H
#endif

