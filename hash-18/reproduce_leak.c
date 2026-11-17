/* hash-18-3: CryptHash leaks a heap block on every call.
 *
 * Wraps malloc and free at link time, calls CryptHash 100 times on short
 * messages, and prints how many allocations are still live afterwards.
 */
#include <stdio.h>
#include <stdlib.h>

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

void *__real_malloc(size_t n);
void __real_free(void *p);
static long live;
void *__wrap_malloc(size_t n) { void *p = __real_malloc(n); if (p) live++; return p; }
void __wrap_free(void *p) { if (p) live--; __real_free(p); }

int main(int argc, char **argv)
{
    unsigned char msg[64] = { 'a', 'b', 'c' }, digest[512];
    int bits = atoi(argv[1]), i;
    long before = live;
    for (i = 0; i < 100; i++)
        if (CryptHash(bits, msg, (unsigned long long)(8 * (i % 40)), digest) != 0) return 2;
    printf("live_blocks_after_100_calls=%ld\n", live - before);
    return 0;
}
