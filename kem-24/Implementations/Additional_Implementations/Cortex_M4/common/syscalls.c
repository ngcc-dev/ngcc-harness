#include <errno.h>
#include <stddef.h>
#include <stdint.h>

extern char end;
extern char _stack;

static char *heap_end;

void *_sbrk(ptrdiff_t incr)
{
    char *prev_heap_end;
    const ptrdiff_t stack_reserve = 8192;

    if (heap_end == NULL) {
        heap_end = &end;
    }

    prev_heap_end = heap_end;
    if (heap_end + incr > &_stack - stack_reserve) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end += incr;
    return prev_heap_end;
}
