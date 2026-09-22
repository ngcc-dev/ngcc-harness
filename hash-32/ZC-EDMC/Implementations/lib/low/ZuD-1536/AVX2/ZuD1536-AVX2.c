/*
AVX2 hyrbrid implementation of ZuD1536 (3x8x64): SIMD chi; rho/theta identical to plain C.

Round structure follows Xoodoo:
  iota -> chi -> rho -> theta -> rho

Scaling choices:
  - theta generalized to 8 columns by applying E[x] derived from parity of previous column:
      P[x] = A0[x] ^ A1[x] ^ A2[x]
      E[x] = ROTL64(P[x-1], 5) ^ ROTL64(P[x-1], 14)
      Ay[x] ^= E[x] for y=0..2
  - rho:
      row2 lanes rotated by ?
      row1 columns rotated right by ?
  - iota: A0[0] ^= rc
  - chi: per-column 3-lane chi, same structure as Xoodoo

This is a custom permutation and not part of upstream XKCP.
*/

#include <immintrin.h>
#include <stdio.h>
#include <string.h>
#include "brg_endian.h"
#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
#error Expecting a little-endian platform
#endif

#include "../ZuD1536.h"
#include "ZuD1536-AVX2.h"

#define ZUD1536_COLS ZUD1536_NCOLUMNS
/* ---------------------------------------------------------------- */

void ZuD1536_avx2_Initialize(ZuD1536_avx2_state *state)
{
    memset(state, 0, sizeof(*state));
}

/* ---------------------------------------------------------------- */

void ZuD1536_avx2_AddBytes(ZuD1536_avx2_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
        unsigned int sizeLeft = length;
        unsigned int pos = offset;
        const uint8_t *cur = data;

        while (sizeLeft && (pos & 7u)) {
            ((uint8_t*)state)[pos] ^= *cur++;
            pos++;
            sizeLeft--;
        }

        while(sizeLeft >= 8u) {
            uint64_t lane = READ64_UNALIGNED( cur );
            unsigned int laneIndex = pos >> 3;
            state->A[laneIndex] ^= lane;
            cur += 8;
            pos += 8;
            sizeLeft -= 8;
        }

        while (sizeLeft) {
            ((uint8_t*)state)[pos] ^= *cur++;
            pos++;
            sizeLeft--;
        }
}
//#else
//    #error "Not yet implemented"
//#endif
//}

/* ---------------------------------------------------------------- */

void ZuD1536_avx2_OverwriteBytes(ZuD1536_avx2_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
        memcpy(((uint8_t*)state)+offset, data, length);
}

/* ---------------------------------------------------------------- */

void ZuD1536_avx2_OverwriteWithZeroes(ZuD1536_avx2_state *state, unsigned int byteCount)
{
    memset(state, 0, byteCount);
}

/* ---------------------------------------------------------------- */

void ZuD1536_avx2_ExtractBytes(const ZuD1536_avx2_state *state, uint8_t *data, unsigned int offset, unsigned int length)
{
    memcpy(data, ((const uint8_t*)state)+offset, length);
}

/* ---------------------------------------------------------------- */

void ZuD1536_avx2_ExtractAndAddBytes(const ZuD1536_avx2_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length)
{
    for (unsigned int i = 0; i < length; i++) {
        output[i] = input[i] ^ ((const uint8_t*)state)[offset + i];
    }
}

/* ---------------------------------------------------------------- */
static const uint64_t RC[ZUD1536_MAXROUNDS] = {
    ZUD1536_rc12,
    ZUD1536_rc11,
    ZUD1536_rc10,
    ZUD1536_rc9,
    ZUD1536_rc8,
    ZUD1536_rc7,
    ZUD1536_rc6,
    ZUD1536_rc5,
    ZUD1536_rc4,
    ZUD1536_rc3,
    ZUD1536_rc2,
    ZUD1536_rc1
};

static inline uint64_t laneParity(const ZuD1536_avx2_state *s, unsigned int x)
{
    return s->A[0*ZUD1536_COLS + x] ^ s->A[1*ZUD1536_COLS + x] ^ s->A[2*ZUD1536_COLS + x];
}
/*
** Theta: Column Parity Mixer, rough code, could be further optimized as Xoodoo way
*/
static void theta(ZuD1536_avx2_state *s)
{
    uint64_t P[ZUD1536_COLS];
    for (unsigned int x = 0; x < ZUD1536_COLS; x++) {
        P[x] = laneParity(s, x);
    }
    for (unsigned int x = 0; x < ZUD1536_COLS; x++) {
        uint64_t p = P[(x + ZUD1536_COLS - 1u) & (ZUD1536_COLS - 1u)];
        uint64_t e = ROTL64(p, 20) ^ ROTL64(p, 56);
        s->A[0*ZUD1536_COLS + x] ^= e;
        s->A[1*ZUD1536_COLS + x] ^= e;
        s->A[2*ZUD1536_COLS + x] ^= e;
    }
}
/*
** Rho: Plane shift
*/
static void rho(ZuD1536_avx2_state *s)
{
    { 
        /* Row 2: <<< (5,0)  (column shift 5, rotate 0) */
        uint64_t t[ZUD1536_COLS];
        for (unsigned int x = 0; x < ZUD1536_COLS; x++) {
            t[x] = s->A[2*ZUD1536_COLS + x];
        }
        for (unsigned int x = 0; x < ZUD1536_COLS; x++) {
            s->A[2*ZUD1536_COLS + x] = ROTL64(t[(x + ZUD1536_COLS - 5u) & (ZUD1536_COLS - 1u)], 0);
        }
    }
    {
    /* Row 1: <<< (3,3)  (column shift 3, rotate 8) */
        uint64_t t[ZUD1536_COLS];
        for (unsigned int x = 0; x < ZUD1536_COLS; x++) {
            t[x] = s->A[1*ZUD1536_COLS + x];
        }
        for (unsigned int x = 0; x < ZUD1536_COLS; x++) {
            s->A[1*ZUD1536_COLS + x] = ROTL64(t[(x + ZUD1536_COLS - 3u) & (ZUD1536_COLS - 1u)], 3);
        }
    }
}
/*
** Iota: Round constants
*/
static void iota(ZuD1536_avx2_state *s, uint64_t rc)
{
    s->A[0] ^= rc;
}
/*
** Chi: Non linear step, on colums
*/
static inline void chi3_avx2(__m256i *v0, __m256i *v1, __m256i *v2)
{
    __m256i t0 = _mm256_xor_si256(*v0, _mm256_andnot_si256(*v1, *v2));
    __m256i t1 = _mm256_xor_si256(*v1, _mm256_andnot_si256(*v2, t0));
    __m256i t2 = _mm256_xor_si256(*v2, _mm256_andnot_si256(t0, t1));
    *v0 = t0;
    *v1 = t1;
    *v2 = t2;
}

/* Four columns at a time: lanes (row0),(row1),(row2) for x..x+3. */
static inline void chi_avx2(ZuD1536_avx2_state *s)
{
    __m256i v0 = _mm256_loadu_si256((const __m256i *)&s->A[0 * ZUD1536_COLS]);
    __m256i v1 = _mm256_loadu_si256((const __m256i *)&s->A[1 * ZUD1536_COLS]);
    __m256i v2 = _mm256_loadu_si256((const __m256i *)&s->A[2 * ZUD1536_COLS]);
    chi3_avx2(&v0, &v1, &v2);
    _mm256_storeu_si256((__m256i *)&s->A[0 * ZUD1536_COLS], v0);
    _mm256_storeu_si256((__m256i *)&s->A[1 * ZUD1536_COLS], v1);
    _mm256_storeu_si256((__m256i *)&s->A[2 * ZUD1536_COLS], v2);

    v0 = _mm256_loadu_si256((const __m256i *)&s->A[0 * ZUD1536_COLS + 4u]);
    v1 = _mm256_loadu_si256((const __m256i *)&s->A[1 * ZUD1536_COLS + 4u]);
    v2 = _mm256_loadu_si256((const __m256i *)&s->A[2 * ZUD1536_COLS + 4u]);
    chi3_avx2(&v0, &v1, &v2);
    _mm256_storeu_si256((__m256i *)&s->A[0 * ZUD1536_COLS + 4u], v0);
    _mm256_storeu_si256((__m256i *)&s->A[1 * ZUD1536_COLS + 4u], v1);
    _mm256_storeu_si256((__m256i *)&s->A[2 * ZUD1536_COLS + 4u], v2);
}

/*
** Rho-east: Plane shift
#define Rho_east()                          \
                    a10 = ROTL32(a10, 1);   \
                    a11 = ROTL32(a11, 1);   \
                    a12 = ROTL32(a12, 1);   \
                    a13 = ROTL32(a13, 1);   \
                    v1  = ROTL32(a23, 8);   \
                    a23 = ROTL32(a21, 8);   \
                    a21 = v1;               \
                    v1  = ROTL32(a22, 8);   \
                    a22 = ROTL32(a20, 8);   \
                    a20 = v1
*/
static void round_step(ZuD1536_avx2_state *s, uint64_t rc)
{
                    iota(s,rc);
                    //printf("rc=%016llx A0=%016llx\n",
                    //    (unsigned long long)rc,
                    //    (unsigned long long)s->A[0]);
                    chi_avx2(s);
                    rho(s);
                    theta(s);
                    rho(s);
}

void ZuD1536_avx2_Permute_Nrounds(ZuD1536_avx2_state *state, unsigned int nr)
{
    if (nr > ZUD1536_MAXROUNDS){
        nr = ZUD1536_MAXROUNDS;
    }
    for (unsigned int i = ZUD1536_MAXROUNDS - nr; i < ZUD1536_MAXROUNDS; ++i ) {
        round_step(state, RC[i]);
    }
}

void ZuD1536_avx2_Permute_6rounds(ZuD1536_avx2_state *state)
{
    round_step(state, ZUD1536_rc6);
    round_step(state, ZUD1536_rc5);
    round_step(state, ZUD1536_rc4);
    round_step(state, ZUD1536_rc3);
    round_step(state, ZUD1536_rc2);
    round_step(state, ZUD1536_rc1);
}

void ZuD1536_avx2_Permute_12rounds(ZuD1536_avx2_state *state)
{
    round_step(state, ZUD1536_rc12);
    round_step(state, ZUD1536_rc11);
    round_step(state, ZUD1536_rc10);
    round_step(state, ZUD1536_rc9);
    round_step(state, ZUD1536_rc8);
    round_step(state, ZUD1536_rc7);
    round_step(state, ZUD1536_rc6);
    round_step(state, ZUD1536_rc5);
    round_step(state, ZUD1536_rc4);
    round_step(state, ZUD1536_rc3);
    round_step(state, ZUD1536_rc2);
    round_step(state, ZUD1536_rc1);
}

