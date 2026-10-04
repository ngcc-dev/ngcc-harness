#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef void *(*calloc_fn)(size_t, size_t);

void *calloc(size_t count, size_t size)
{
    static calloc_fn real_calloc;
    const char *value;
    unsigned long long target;

    if (real_calloc == NULL) {
        real_calloc = (calloc_fn)dlsym(RTLD_NEXT, "calloc");
        if (real_calloc == NULL) {
            return NULL;
        }
    }

    value = getenv("NGCC_FAIL_CALLOC_NMEMB");
    target = value == NULL ? 0 : strtoull(value, NULL, 10);
    if (target != 0 && count == (size_t)target && size == sizeof(int16_t)) {
        return NULL;
    }
    return real_calloc(count, size);
}
