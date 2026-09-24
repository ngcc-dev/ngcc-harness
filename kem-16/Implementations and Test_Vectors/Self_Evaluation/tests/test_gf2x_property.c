#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "parameters.h"
#include "gf2x.h"

static void xor_bit(uint64_t *v, uint32_t pos) {
    v[pos >> 6] ^= (1ULL << (pos & 63U));
}

static void or_bit(uint64_t *v, uint32_t pos) {
    v[pos >> 6] |= (1ULL << (pos & 63U));
}

static uint32_t get_bit(const uint64_t *v, uint32_t pos) {
    return (uint32_t)((v[pos >> 6] >> (pos & 63U)) & 1ULL);
}

static void mask_tail(uint64_t *v) {
    v[VEC_N_SIZE_64 - 1U] &= BITMASK(PARAM_N, 64);
}

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static uint32_t next_pos(uint32_t *state) {
    return (uint32_t)(((uint64_t)xorshift32(state) * (uint64_t)PARAM_N) >> 32);
}

static int contains(const uint32_t *a, size_t n, uint32_t x) {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] == x) {
            return 1;
        }
    }
    return 0;
}

static void fill_dense(uint64_t *v, uint32_t seed) {
    uint32_t st = seed ^ (uint32_t)PARAM_N;
    for (size_t i = 0; i < VEC_N_SIZE_64; ++i) {
        const uint64_t lo = (uint64_t)xorshift32(&st);
        const uint64_t hi = (uint64_t)xorshift32(&st);
        v[i] = lo ^ (hi << 32);
    }
    mask_tail(v);
}

static int check_sparse_product(void) {
    enum { W = 48 };
    uint64_t a[VEC_N_SIZE_64];
    uint64_t b[VEC_N_SIZE_64];
    uint64_t got[VEC_N_SIZE_64];
    uint64_t expected[VEC_N_SIZE_64];
    uint32_t sa[W];
    uint32_t sb[W];

    memset(a, 0, sizeof(a));
    memset(b, 0, sizeof(b));
    memset(got, 0, sizeof(got));
    memset(expected, 0, sizeof(expected));

    uint32_t st = 0xC0FFEE11u ^ (uint32_t)PARAM_N;
    for (size_t i = 0; i < W; ++i) {
        uint32_t p;
        do {
            p = next_pos(&st);
        } while (contains(sa, i, p));
        sa[i] = p;
        xor_bit(a, p);
    }
    for (size_t i = 0; i < W; ++i) {
        uint32_t p;
        do {
            p = next_pos(&st);
        } while (contains(sb, i, p));
        sb[i] = p;
        xor_bit(b, p);
    }

    vect_mul(got, a, b);

    for (size_t i = 0; i < W; ++i) {
        for (size_t j = 0; j < W; ++j) {
            const uint32_t pos = (uint32_t)(((uint64_t)sa[i] + (uint64_t)sb[j]) % PARAM_N);
            xor_bit(expected, pos);
        }
    }

    mask_tail(expected);
    mask_tail(got);

    if (memcmp(got, expected, sizeof(got)) != 0) {
        fprintf(stderr, "gf2x sparse-product mismatch for PARAM_N=%u\n", (unsigned)PARAM_N);
        return 1;
    }
    return 0;
}

static int check_identity_and_monomial_shifts(void) {
    uint64_t a[VEC_N_SIZE_64];
    uint64_t mono[VEC_N_SIZE_64];
    uint64_t got[VEC_N_SIZE_64];
    uint64_t expected[VEC_N_SIZE_64];

    fill_dense(a, 0x1234abcdU);

    memset(mono, 0, sizeof(mono));
    or_bit(mono, 0);
    memset(got, 0, sizeof(got));
    vect_mul(got, a, mono);
    mask_tail(got);
    if (memcmp(got, a, sizeof(got)) != 0) {
        fprintf(stderr, "gf2x identity mismatch for PARAM_N=%u\n", (unsigned)PARAM_N);
        return 1;
    }

    const uint32_t shifts[] = {
        1U,
        7U,
        63U,
        64U,
        127U,
        (uint32_t)(PARAM_N / 3U),
        (uint32_t)(PARAM_N - 1U)
    };

    for (size_t s = 0; s < sizeof(shifts) / sizeof(shifts[0]); ++s) {
        const uint32_t shift = shifts[s] % PARAM_N;
        memset(mono, 0, sizeof(mono));
        memset(got, 0, sizeof(got));
        memset(expected, 0, sizeof(expected));
        or_bit(mono, shift);
        vect_mul(got, a, mono);
        for (uint32_t i = 0; i < PARAM_N; ++i) {
            if (get_bit(a, i)) {
                const uint32_t pos = (uint32_t)(((uint64_t)i + (uint64_t)shift) % PARAM_N);
                or_bit(expected, pos);
            }
        }
        mask_tail(got);
        mask_tail(expected);
        if (memcmp(got, expected, sizeof(got)) != 0) {
            fprintf(stderr, "gf2x monomial-shift mismatch for PARAM_N=%u shift=%u\n", (unsigned)PARAM_N, shift);
            return 1;
        }
    }

    return 0;
}

static int check_commutativity_and_distributivity(void) {
    uint64_t a[VEC_N_SIZE_64];
    uint64_t b[VEC_N_SIZE_64];
    uint64_t c[VEC_N_SIZE_64];
    uint64_t bc[VEC_N_SIZE_64];
    uint64_t ab[VEC_N_SIZE_64];
    uint64_t ba[VEC_N_SIZE_64];
    uint64_t ac[VEC_N_SIZE_64];
    uint64_t left[VEC_N_SIZE_64];
    uint64_t right[VEC_N_SIZE_64];

    fill_dense(a, 0xA5A5A5A5u);
    fill_dense(b, 0x5A5A5A5Au);
    fill_dense(c, 0x0F0F1234u);

    vect_mul(ab, a, b);
    vect_mul(ba, b, a);
    mask_tail(ab);
    mask_tail(ba);
    if (memcmp(ab, ba, sizeof(ab)) != 0) {
        fprintf(stderr, "gf2x commutativity mismatch for PARAM_N=%u\n", (unsigned)PARAM_N);
        return 1;
    }

    for (size_t i = 0; i < VEC_N_SIZE_64; ++i) {
        bc[i] = b[i] ^ c[i];
    }
    mask_tail(bc);

    vect_mul(left, a, bc);
    vect_mul(ab, a, b);
    vect_mul(ac, a, c);
    for (size_t i = 0; i < VEC_N_SIZE_64; ++i) {
        right[i] = ab[i] ^ ac[i];
    }
    mask_tail(left);
    mask_tail(right);

    if (memcmp(left, right, sizeof(left)) != 0) {
        fprintf(stderr, "gf2x distributivity mismatch for PARAM_N=%u\n", (unsigned)PARAM_N);
        return 1;
    }

    return 0;
}

int main(void) {
    if (check_sparse_product() != 0) {
        return 1;
    }
    if (check_identity_and_monomial_shifts() != 0) {
        return 1;
    }
    if (check_commutativity_and_distributivity() != 0) {
        return 1;
    }
    return 0;
}
