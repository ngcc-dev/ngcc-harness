#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "address.h"
#include "params.h"
#include "fips202x4.h"
#include "hashx4.h"

extern void KeccakP1600times4_PermuteAll_24rounds(__m256i *s);

/* Compute PRF for 4 addresses in parallel using 4-way SHAKE256 */
void prf_addrx4(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3,
                const spx_ctx *ctx,
                const uint32_t addrx4[4 * 8])
{
    __m256i keccak_state[25];
    int lane_idx, word_idx;

    /* Load public seed into state */
    for (word_idx = 0; word_idx < SPX_N/8; word_idx++) {
        keccak_state[word_idx] = _mm256_set1_epi64x(((int64_t*)ctx->pub_seed)[word_idx]);
    }

    /* Interleave 4 addresses into state */
    for (lane_idx = 0; lane_idx < 4; lane_idx++) {
        keccak_state[SPX_N/8+lane_idx] = _mm256_set_epi32(
            addrx4[3*8+1+2*lane_idx],
            addrx4[3*8+2*lane_idx],
            addrx4[2*8+1+2*lane_idx],
            addrx4[2*8+2*lane_idx],
            addrx4[8+1+2*lane_idx],
            addrx4[8+2*lane_idx],
            addrx4[1+2*lane_idx],
            addrx4[2*lane_idx]
        );
    }

    /* Load secret seed into state */
    for (word_idx = 0; word_idx < SPX_N/8; word_idx++) {
        keccak_state[SPX_N/8+lane_idx+word_idx] = _mm256_set1_epi64x(((int64_t*)ctx->sk_seed)[word_idx]);
    }

    /* Set SHAKE domain separator and padding */
    keccak_state[SPX_N/4+4] = _mm256_set1_epi64x(0x1f);
    for (word_idx = SPX_N/4+5; word_idx < 16; word_idx++) {
        keccak_state[word_idx] = _mm256_set1_epi64x(0);
    }
    keccak_state[16] = _mm256_set1_epi64x((long long)(0x80ULL << 56));

    /* Zero remaining state positions */
    for (word_idx = 17; word_idx < 25; word_idx++) {
        keccak_state[word_idx] = _mm256_set1_epi64x(0);
    }

    /* Perform 4-way Keccak permutation */
    KeccakP1600times4_PermuteAll_24rounds(&keccak_state[0]);

    /* Extract output for each lane */
    for (word_idx = 0; word_idx < SPX_N/8; word_idx++) {
        ((int64_t*)out0)[word_idx] = _mm256_extract_epi64(keccak_state[word_idx], 0);
        ((int64_t*)out1)[word_idx] = _mm256_extract_epi64(keccak_state[word_idx], 1);
        ((int64_t*)out2)[word_idx] = _mm256_extract_epi64(keccak_state[word_idx], 2);
        ((int64_t*)out3)[word_idx] = _mm256_extract_epi64(keccak_state[word_idx], 3);
    }
}