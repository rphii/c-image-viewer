#include "mem.h"

#undef malloc
#undef calloc
#undef realloc
#undef free

#include "trace.h"

#include <stdbool.h>

typedef enum {
    MEM_NONE,
    MEM_MALLOCD,
    MEM_CALLOCD,
    MEM_REALLOCD,
    MEM_FREED,
} MemList;

typedef struct Mem {
    MemList state;
    Trace trace;
    int line;
    const char *file;
    const char *func;
} Mem;

#include "lut.h"

void mem_free(Mem *mem) {
    trace_free(&mem->trace);
    memset(mem, 0, sizeof(*mem));
}

size_t tmem_hash(const void *ptr) {
    /* https://stackoverflow.com/questions/20953390/what-is-the-fastest-hash-function-for-pointers */
    size_t ad = (size_t)ptr;
    ad = (size_t)(ad ^ (ad >> 16));
    return ad;
}
int tmem_cmp(const void *a, const void *b) {
    return a - b;
}

LUT_INCLUDE(TMem, tmem, void *, BY_VAL, Mem, BY_REF);
LUT_IMPLEMENT(TMem, tmem, void *, BY_VAL, Mem, BY_REF, tmem_hash, tmem_cmp, 0, mem_free);

static TMem tmem;

void *mem_custom_malloc(size_t size, const char *file, int line, const char *func) {
    printff("MALLOC");
    void *result = malloc(size);

    /* record entry */
    Mem mem = { .state = MEM_MALLOCD };
    mem.file = file;
    mem.line = line;
    mem.func = func;
    TRYC(trace_grab(&mem.trace));
    TRYG(tmem_set(&tmem, result, &mem));

    return result;
error: ABORT(ERR_UNREACHABLE);
}

void *mem_custom_calloc(size_t nmemb, size_t size, const char *file, int line, const char *func) {
    printff("CALLOC");
    void *result = calloc(nmemb, size);

    /* record entry */
    Mem mem = { .state = MEM_CALLOCD };
    mem.file = file;
    mem.line = line;
    mem.func = func;
    TRYC(trace_grab(&mem.trace));
    TRYG(tmem_set(&tmem, result, &mem));

    return result;
error: ABORT(ERR_UNREACHABLE);
}

void *mem_custom_realloc(void *ptr, size_t size, const char *file, int line, const char *func) {
    printff("REALLOC");
    void *result = realloc(ptr, size);

    /* record entry */
    Mem mem = { .state = MEM_REALLOCD };
    mem.file = file;
    mem.line = line;
    mem.func = func;
    TRYC(trace_grab(&mem.trace));
    TRYG(tmem_set(&tmem, result, &mem));

    return result;
error: ABORT(ERR_UNREACHABLE);
}

void mem_custom_free(void *ptr, const char *file, int line, const char *func) {
    printff("FREE %p", ptr);
    //Mem *mem = tmem_get(&tmem, ptr);
    tmem_del(&tmem, ptr);

    /* done; free at last. */
    free(ptr);
}

void mem_log(void) {
    size_t n = 0;
    for(TMemItem **iter = tmem_iter_all(&tmem, 0); iter; iter = tmem_iter_all(&tmem, iter), ++n) {
        Mem *mem = (*iter)->key;
        printff("%zu: %d", n, mem->line);
        //printff("%zu: %s:%d:%s", n, mem->file, mem->line, mem->func);
    }
}

