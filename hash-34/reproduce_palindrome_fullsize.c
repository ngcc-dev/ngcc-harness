/* Check the four-block palindrome identity on the submitted full-width C compression. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wchain_c.h"

typedef void (*compress_fn)(uint64_t *, const uint64_t *, const uint8_t *);

static uint64_t rng_state = UINT64_C(0x786c47b94d2f38e1);

static uint64_t next_word(void) {
    uint64_t z = (rng_state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

static void absorb(uint64_t *older, uint64_t *newer, size_t words,
                   const uint8_t *block, compress_fn compress) {
    uint64_t output[18], next[18];
    compress(output, newer, block);
    for (size_t i = 0; i < words; ++i) next[i] = output[i] ^ older[i];
    memcpy(older, newer, words * sizeof(uint64_t));
    memcpy(newer, next, words * sizeof(uint64_t));
}

static int check(size_t words, compress_fn compress) {
    uint8_t x[288], y[288];
    uint64_t w[18], v[18], u[18], output[18], older[18], newer[18];
    const size_t block_bytes = words * 16;
    unsigned controls_rejected = 0;

    for (unsigned trial = 0; trial < 256; ++trial) {
        for (size_t i = 0; i < block_bytes; ++i) {
            x[i] = (uint8_t)next_word();
            y[i] = (uint8_t)next_word();
        }
        for (size_t i = 0; i < words; ++i) w[i] = next_word();

        /* v = P_y(w); u = w xor CF(v,x). This constructs a valid
           nonzero-start palindrome condition without inverting P_y. */
        compress(output, w, y);
        for (size_t i = 0; i < words; ++i) v[i] = output[i] ^ w[i];
        compress(output, v, x);
        for (size_t i = 0; i < words; ++i) u[i] = w[i] ^ output[i];

        memcpy(older, u, words * sizeof(uint64_t));
        memcpy(newer, v, words * sizeof(uint64_t));
        absorb(older, newer, words, x, compress);
        absorb(older, newer, words, y, compress);
        absorb(older, newer, words, y, compress);
        absorb(older, newer, words, x, compress);
        if (memcmp(older, v, words * sizeof(uint64_t)) ||
            memcmp(newer, u, words * sizeof(uint64_t))) {
            fprintf(stderr, "palindrome failed at trial %u\n", trial);
            return 1;
        }

        /* Same zero checksum, but not the palindrome ordering. */
        memcpy(older, u, words * sizeof(uint64_t));
        memcpy(newer, v, words * sizeof(uint64_t));
        absorb(older, newer, words, x, compress);
        absorb(older, newer, words, y, compress);
        absorb(older, newer, words, x, compress);
        absorb(older, newer, words, y, compress);
        if (memcmp(older, v, words * sizeof(uint64_t)) ||
            memcmp(newer, u, words * sizeof(uint64_t))) ++controls_rejected;
    }
    if (controls_rejected != 256) {
        fprintf(stderr, "control unexpectedly passed (%u/256 rejected)\n", controls_rejected);
        return 1;
    }
    printf("FULL-WIDTH palindrome: %zu-bit state, 256/256 checks, 256/256 controls reject\n",
           words * 64);
    return 0;
}

int main(void) {
    if (check(9, wchain_v1_compress_c) || check(18, wchain_v2_compress_c)) return 1;
    puts("CONDITIONAL hash-34-1 CONFIRMED: palindrome identity on submitted full-width compression");
    puts("FULL-SIZE reset pair NOT FOUND: birthday-like matching remains an assumption");
    return 0;
}
