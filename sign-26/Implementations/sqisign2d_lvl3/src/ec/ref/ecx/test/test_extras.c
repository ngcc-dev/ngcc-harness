#include "test_extras.h"
#include <bench.h>

// Global constants
extern const digit_t p[NWORDS_FIELD];
extern const digit_t R2[NWORDS_FIELD];

#if 0
int64_t cpucycles(void)
{ // Access system counter for benchmarking
    unsigned int hi, lo;

    asm volatile ("rdtsc\n\t" : "=a" (lo), "=d"(hi));
    return ((int64_t)lo) | (((int64_t)hi) << 32);
}
#endif

int
compare_words(digit_t *a, digit_t *b, unsigned int nwords)
{ // Comparing "nword" elements, a=b? : (1) a>b, (0) a=b, (-1) a<b
  // SECURITY NOTE: this function does not have constant-time execution. TO BE USED FOR TESTING
  // ONLY.
    int i;

    for (i = nwords - 1; i >= 0; i--) {
        if (a[i] > b[i])
            return 1;
        else if (a[i] < b[i])
            return -1;
    }

    return 0;
}

void
sub_test(digit_t *out, digit_t *a, digit_t *b, unsigned int nwords)
{ // Subtraction without borrow, out = a-b where a>b
  // SECURITY NOTE: this function does not have constant-time execution. It is for TESTING ONLY.
    unsigned int i;
    digit_t res, carry, borrow = 0;

    for (i = 0; i < nwords; i++) {
        res = a[i] - b[i];
        carry = (a[i] < b[i]);
        out[i] = res - borrow;
        borrow = carry || (res < borrow);
    }
}

// Make n random-ish field elements (for tests only!).
void
fp_random_test(fp_t *a)
{
    shake_context sc;
    uint8_t tmp[32];
    uint64_t z;

    z = cpucycles();
    shake_init(&sc, 256);
    for (int i = 0; i < 8; i++) {
        tmp[i] = (uint8_t)(z >> (8 * i));
    }
    shake_inject(&sc, tmp, 8);
    shake_flip(&sc);
    shake_extract(&sc, tmp, 32);
    fp_decode_reduce(a, tmp, 32);
}

void
fp2_random_test(fp2_t *a)
{
    fp_random_test(&(a->re));
    fp_random_test(&(a->im));
}
