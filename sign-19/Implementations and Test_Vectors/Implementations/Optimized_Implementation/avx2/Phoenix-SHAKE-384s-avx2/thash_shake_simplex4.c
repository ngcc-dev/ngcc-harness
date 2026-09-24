/* Phoenix 4-way parallel thash implementation using SHAKE256
 *
 * Tweakable hash function for Phoenix with AVX2 parallel optimization.
 */

#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "fips202x4.h"
#include "params.h"
#include "thashx4.h"
#include "utils.h"

extern void KeccakP1600times4_PermuteAll_24rounds(__m256i *s);

/* Compute 4-way parallel tweakable hash on input blocks */
void thashx4(unsigned char *out0,
             unsigned char *out1,
             unsigned char *out2,
             unsigned char *out3,
             const unsigned char *in0,
             const unsigned char *in1,
             const unsigned char *in2,
             const unsigned char *in3,
             unsigned int inblocks,
             const spx_ctx *ctx,
             uint32_t addrx4[4 * 8])
{
    /* For SPX_N > 32, always use generic path for correctness */
    /* The optimized path has issues with rate/alignment calculations */
    if (SPX_N <= 32 && (inblocks == 1 || inblocks == 2)) {
        /* Direct Keccak state manipulation for small inputs (SPX_N <= 32 only) */
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

        /* Load input blocks into state */
        for (word_idx = 0; word_idx < (SPX_N/8) * inblocks; word_idx++) {
            keccak_state[SPX_N/8+4+word_idx] = _mm256_set_epi64x(
                        ((int64_t*)in3)[word_idx],
                        ((int64_t*)in2)[word_idx],
                        ((int64_t*)in1)[word_idx],
                        ((int64_t*)in0)[word_idx]
                    );
        }

        /* Set domain separator and padding */
        for (word_idx = (SPX_N/8)*(1+inblocks)+4; word_idx < 16; word_idx++) {
            keccak_state[word_idx] = _mm256_set1_epi64x(0);
        }
        keccak_state[16] = _mm256_set1_epi64x((long long)(0x80ULL << 56));
        keccak_state[(SPX_N/8)*(1+inblocks)+4] = _mm256_xor_si256(
            keccak_state[(SPX_N/8)*(1+inblocks)+4],
            _mm256_set1_epi64x(0x1f)
        );
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
    } else {
        /* Use generic shake256x4 for larger inputs or SPX_N > 32 */
        SPX_VLA(unsigned char, buf0, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);
        SPX_VLA(unsigned char, buf1, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);
        SPX_VLA(unsigned char, buf2, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);
        SPX_VLA(unsigned char, buf3, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);

        memcpy(buf0, ctx->pub_seed, SPX_N);
        memcpy(buf1, ctx->pub_seed, SPX_N);
        memcpy(buf2, ctx->pub_seed, SPX_N);
        memcpy(buf3, ctx->pub_seed, SPX_N);
        memcpy(buf0 + SPX_N, addrx4 + 0*8, SPX_ADDR_BYTES);
        memcpy(buf1 + SPX_N, addrx4 + 1*8, SPX_ADDR_BYTES);
        memcpy(buf2 + SPX_N, addrx4 + 2*8, SPX_ADDR_BYTES);
        memcpy(buf3 + SPX_N, addrx4 + 3*8, SPX_ADDR_BYTES);
        memcpy(buf0 + SPX_N + SPX_ADDR_BYTES, in0, inblocks * SPX_N);
        memcpy(buf1 + SPX_N + SPX_ADDR_BYTES, in1, inblocks * SPX_N);
        memcpy(buf2 + SPX_N + SPX_ADDR_BYTES, in2, inblocks * SPX_N);
        memcpy(buf3 + SPX_N + SPX_ADDR_BYTES, in3, inblocks * SPX_N);

        shake256x4(out0, out1, out2, out3, SPX_N,
                   buf0, buf1, buf2, buf3, SPX_N + SPX_ADDR_BYTES + inblocks*SPX_N);
    }
}
