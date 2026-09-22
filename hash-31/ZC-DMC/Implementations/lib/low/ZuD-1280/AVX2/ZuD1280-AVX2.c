/*
ZuD-1280 (5x4x64) - AVX2 permutation (mainstream x86-64). compile with -mavx2.

Round structure follows Xoodoo:
  iota -> chi -> rho -> theta -> rho

Scaling choices:
  - theta generalized to 8 columns by applying E[x] derived from parity of previous column:
      P[x] = A0[x] ^ A1[x] ^ A2[x] ^ A3[x] ^ A4[x]
      E[x] = ROTL64(P[x-1], 5) ^ ROTL64(P[x-1], 14)
      Ay[x] ^= E[x] for y=0..4
  - rho:
      row2 lanes rotated by ?
      row1 columns rotated right by ?
  - iota: A0[0] ^= rc
  - chi: per-column 5-lane chi, same structure as Xoodoo

This is a custom permutation and not part of upstream XKCP.
*/
#include <immintrin.h>
#include <stdio.h>
#include <string.h>
#include "brg_endian.h"
#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
#error Expecting a little-endian platform
#endif

//#include "../ZuD1280.h"
#include "ZuD1280-AVX2.h"

#define ZUD1280_COLS ZUD1280_NCOLUMNS

/* _mm256_permute4x64_epi64 imm: dst lane i <- src lane imm[2*i+1 : 2*i] */
#define RHO_R4_IMM 0x39u /* (x+1) mod 4 */
#define RHO_R3_IMM 0x4eu /* (x+2) mod 4 */
#define RHO_R2R1_IMM 0x93u /* (x+3) mod 4 */

static inline __m256i mm256_rol64(__m256i x, unsigned int k)
{
    k &= 63u;
    if (k == 0u)
        return x;
    return _mm256_or_si256(_mm256_slli_epi64(x, (int)k), _mm256_srli_epi64(x, (int)(64u - k)));
}

static void load_state(const ZuD1280_avx2_state *s, __m256i v[5])
{
    for (unsigned int r = 0; r < 5u; r++)
        v[r] = _mm256_loadu_si256((const __m256i *)&s->A[r * ZUD1280_COLS]);
}

static void store_state(ZuD1280_avx2_state *s, const __m256i v[5])
{
    for (unsigned int r = 0; r < 5u; r++)
        _mm256_storeu_si256((__m256i *)&s->A[r * ZUD1280_COLS], v[r]);
}

/* ---------------------------------------------------------------- */

void ZuD1280_avx2_Initialize(ZuD1280_avx2_state *state)
{
    memset(state, 0, sizeof(*state));
}

/* ---------------------------------------------------------------- */

void ZuD1280_avx2_AddBytes(ZuD1280_avx2_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
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
            sizeLeft -= 8u;
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

void ZuD1280_avx2_OverwriteBytes(ZuD1280_avx2_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
        memcpy(((uint8_t*)state)+offset, data, length);
}

/* ---------------------------------------------------------------- */

void ZuD1280_avx2_OverwriteWithZeroes(ZuD1280_avx2_state *state, unsigned int byteCount)
{
    memset(state, 0, byteCount);
}

/* ---------------------------------------------------------------- */

void ZuD1280_avx2_ExtractBytes(const ZuD1280_avx2_state *state, uint8_t *data, unsigned int offset, unsigned int length)
{
    memcpy(data, ((const uint8_t*)state)+offset, length);
}

/* ---------------------------------------------------------------- */

void ZuD1280_avx2_ExtractAndAddBytes(const ZuD1280_avx2_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length)
{
    for (unsigned int i = 0; i < length; i++) {
        output[i] = input[i] ^ ((const uint8_t*)state)[offset + i];
    }
}

/* ---------------------------------------------------------------- */
static const uint64_t RC[ZUD1280_MAXROUNDS] = {
    ZUD1280_rc12,
    ZUD1280_rc11,
    ZUD1280_rc10,
    ZUD1280_rc9,
    ZUD1280_rc8,
    ZUD1280_rc7,
    ZUD1280_rc6,
    ZUD1280_rc5,
    ZUD1280_rc4,
    ZUD1280_rc3,
    ZUD1280_rc2,
    ZUD1280_rc1
};

/*
** Theta: Column Parity Mixer, rough code, could be further optimized as Xoodoo way
*/
static inline void theta_avx2(__m256i v[5])
{
    __m256i vP = _mm256_xor_si256(v[0], v[1]);
    vP = _mm256_xor_si256(vP, v[2]);
    vP = _mm256_xor_si256(vP, v[3]);
    vP = _mm256_xor_si256(vP, v[4]);
    __m256i vPrev = _mm256_permute4x64_epi64(vP, (int)RHO_R2R1_IMM);
    __m256i vE = _mm256_xor_si256(mm256_rol64(vPrev, 5u), mm256_rol64(vPrev, 14u));
    v[0] = _mm256_xor_si256(v[0], vE);
    v[1] = _mm256_xor_si256(v[1], vE);
    v[2] = _mm256_xor_si256(v[2], vE);
    v[3] = _mm256_xor_si256(v[3], vE);
    v[4] = _mm256_xor_si256(v[4], vE);
}
/*
** Rho: Plane shift
*/
static inline void rho_avx2(__m256i v[5])
{
    v[4] = mm256_rol64(_mm256_permute4x64_epi64(v[4], (int)RHO_R3_IMM), 3u);
    v[3] = mm256_rol64(v[3], 1u);
    v[2] = mm256_rol64(_mm256_permute4x64_epi64(v[2], (int)RHO_R4_IMM), 24u);
    v[1] = mm256_rol64(_mm256_permute4x64_epi64(v[1], (int)RHO_R2R1_IMM), 5u);
}
/*
** Chi: Non linear step, on colums
*/
static inline void chi_avx2(__m256i v[5])                               \
{
    __m256i t0 = _mm256_xor_si256(v[0], _mm256_andnot_si256(v[1], v[2]));
    __m256i t1 = _mm256_xor_si256(v[1], _mm256_andnot_si256(v[2], v[3]));
    __m256i t2 = _mm256_xor_si256(v[2], _mm256_andnot_si256(v[3], v[4]));
    __m256i t3 = _mm256_xor_si256(v[3], _mm256_andnot_si256(v[4], v[0]));
    __m256i t4 = _mm256_xor_si256(v[4], _mm256_andnot_si256(v[0], v[1]));
    v[0] = t0;
    v[1] = t1;
    v[2] = t2;
    v[3] = t3;
    v[4] = t4;
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
/* Helper: dump a __m256i as 4x uint64_t (lane3..lane0) */
static inline void dump_m256i_u64(const char *tag, __m256i x)
{
    uint64_t t[4];
    _mm256_storeu_si256((__m256i *)t, x); /* t[0] is low 64-bit lane */
    printf("%s: %016llx %016llx %016llx %016llx\n",
           tag,
           (unsigned long long)t[3],
           (unsigned long long)t[2],
           (unsigned long long)t[1],
           (unsigned long long)t[0]);
}

static inline void round_avx2(__m256i v[5], uint64_t rc)
{
    __m256i rcv = _mm256_set_epi64x(0, 0, 0, (long long)rc);
    v[0] = _mm256_xor_si256(v[0], rcv);
    //dump_m256i_u64("after iota v0", v[0]);
    //dump_m256i_u64("after iota v2", v[2]);
    chi_avx2(v);
    //dump_m256i_u64("after chi v0", v[0]);
    rho_avx2(v);
    theta_avx2(v);
    rho_avx2(v);
}

void ZuD1280_avx2_Permute_Nrounds(ZuD1280_avx2_state *state, unsigned int nr)
{
    __m256i v[5];
    if (nr > ZUD1280_MAXROUNDS){
        nr = ZUD1280_MAXROUNDS;
    }
    load_state(state,v);
    for (unsigned int i = ZUD1280_MAXROUNDS - nr; i < ZUD1280_MAXROUNDS; ++i ) {
        round_avx2(v, RC[i]);
    }
    store_state(state, v);
}

void ZuD1280_avx2_Permute_6rounds(ZuD1280_avx2_state *state)
{
    __m256i v[5];
    load_state(state, v);
    round_avx2(v, ZUD1280_rc6);
    round_avx2(v, ZUD1280_rc5);
    round_avx2(v, ZUD1280_rc4);
    round_avx2(v, ZUD1280_rc3);
    round_avx2(v, ZUD1280_rc2);
    round_avx2(v, ZUD1280_rc1);
    store_state(state, v);
}

void ZuD1280_avx2_Permute_12rounds(ZuD1280_avx2_state *state)
{
    __m256i v[5];
    load_state(state, v);
    round_avx2(v, ZUD1280_rc12);
    round_avx2(v, ZUD1280_rc11);
    round_avx2(v, ZUD1280_rc10);
    round_avx2(v, ZUD1280_rc9);
    round_avx2(v, ZUD1280_rc8);
    round_avx2(v, ZUD1280_rc7);
    round_avx2(v, ZUD1280_rc6);
    round_avx2(v, ZUD1280_rc5);
    round_avx2(v, ZUD1280_rc4);
    round_avx2(v, ZUD1280_rc3);
    round_avx2(v, ZUD1280_rc2);
    round_avx2(v, ZUD1280_rc1);
    store_state(state, v);
}

