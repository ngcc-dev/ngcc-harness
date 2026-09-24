/*
 * Bare-metal heap for the QEMU harness — a small self-contained malloc/free over a
 * static arena, overriding newlib's allocator (whose reentrancy/lock internals do not
 * work under -nostdlib/-lnosys). This lets the official ICCS auxfunc.c / drng.c — which
 * use malloc()/free() — run UNMODIFIED on Cortex-M4. First-fit with split + forward
 * coalescing; the DKE auxiliary functions only do short-lived balanced malloc/free pairs.
 *
 * Override the arena size with -DHEAP_ARENA_SIZE=<bytes> for larger-RAM targets.
 */
#include <stddef.h>
#include <string.h>

#ifndef HEAP_ARENA_SIZE
#define HEAP_ARENA_SIZE (48 * 1024)
#endif

static unsigned char heap_arena[HEAP_ARENA_SIZE] __attribute__((aligned(8)));

typedef struct block { size_t size; int used; struct block *next; } block_t;
static block_t *heap_head = 0;

static void heap_init(void) {
    heap_head = (block_t *)heap_arena;
    heap_head->size = HEAP_ARENA_SIZE - sizeof(block_t);
    heap_head->used = 0;
    heap_head->next = 0;
}

void *malloc(size_t n) {
    block_t *b;
    if (!heap_head) heap_init();
    n = (n + 7u) & ~(size_t)7u;                 /* 8-byte align payload */
    for (b = heap_head; b; b = b->next) {
        if (!b->used && b->size >= n) {
            if (b->size >= n + sizeof(block_t) + 8u) {   /* split */
                block_t *nb = (block_t *)((unsigned char *)b + sizeof(block_t) + n);
                nb->size = b->size - n - sizeof(block_t);
                nb->used = 0;
                nb->next = b->next;
                b->size = n;
                b->next = nb;
            }
            b->used = 1;
            return (unsigned char *)b + sizeof(block_t);
        }
    }
    return 0;                                     /* out of arena */
}

void free(void *p) {
    block_t *b, *c;
    if (!p) return;
    b = (block_t *)((unsigned char *)p - sizeof(block_t));
    b->used = 0;
    for (c = heap_head; c; c = c->next)           /* forward coalesce */
        while (c->next && !c->used && !c->next->used) {
            c->size += sizeof(block_t) + c->next->size;
            c->next = c->next->next;
        }
}

void *calloc(size_t a, size_t b) {
    size_t n = a * b;
    void *p = malloc(n);
    if (p) memset(p, 0, n);
    return p;
}

void *realloc(void *p, size_t n) {
    block_t *b;
    void *np;
    if (!p) return malloc(n);
    b = (block_t *)((unsigned char *)p - sizeof(block_t));
    np = malloc(n);
    if (np) { memcpy(np, p, b->size < n ? b->size : n); free(p); }
    return np;
}

/* Fallback for any stray libc reference; the custom malloc above does not use it. */
extern char __end__;
extern char __heap_end;
void *_sbrk(int incr) {
    static char *cur = 0;
    char *prev;
    if (cur == 0) cur = (char *)(((unsigned long)&__end__ + 7u) & ~7ul);
    if (cur + incr > &__heap_end) return (void *)-1;
    prev = cur; cur += incr; return prev;
}
