/*
Plain C implementation of ZuD (3x8x64).

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

#include <stdio.h>
#include <string.h>
#include "brg_endian.h"
#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
#error Expecting a little-endian platform
#endif

#include "../ZuD1536.h"
#include "ZuD1536-plain.h"

#define ZUD1536_COLS ZUD1536_NCOLUMNS
/*
#define VERBOSE         0

#if (VERBOSE > 0)
    #define    Dump(__t)    printf(__t "\n"); \
                            printf("a00 %08x, a01 %08x, a02 %08x, a03 %08x\n", a00, a01, a02, a03, a04, a05, a06, a07 ); \
                            printf("a10 %08x, a11 %08x, a12 %08x, a13 %08x\n", a10, a11, a12, a13, a14, a15, a16, a17 ); \
                            printf("a20 %08x, a21 %08x, a22 %08x, a23 %08x\n\n", a20, a21, a22, a23, a24, a25, a26, a27 );
#else
    #define    Dump(__t)
#endif

#if (VERBOSE >= 1)
    #define    Dump1(__t)    Dump(__t)
#else
    #define    Dump1(__t)
#endif

#if (VERBOSE >= 2)
    #define    Dump2(__t)    Dump(__t)
#else
    #define    Dump2(__t)
#endif

#if (VERBOSE >= 3)
    #define    Dump3(__t)    Dump(__t)
#else
    #define    Dump3(__t)
#endif
*/
/* ---------------------------------------------------------------- */

void ZuD1536_plain_Initialize(ZuD1536_plain64_state *state)
{
    memset(state, 0, sizeof(*state));
}

/* ---------------------------------------------------------------- */

void ZuD1536_plain_AddBytes(ZuD1536_plain64_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
        unsigned int sizeLeft = length;
        unsigned int pos = offset;
        const uint8_t *cur = data;

        while (sizeLeft && (pos & 7u)) {
            ((uint8_t*)state)[pos] ^= *cur++;
            pos++;
            sizeLeft--;
        }

        while(sizeLeft >= 8) {
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

void ZuD1536_plain_OverwriteBytes(ZuD1536_plain64_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
        memcpy(((uint8_t*)state)+offset, data, length);
}

/* ---------------------------------------------------------------- */

void ZuD1536_plain_OverwriteWithZeroes(ZuD1536_plain64_state *state, unsigned int byteCount)
{
    memset(state, 0, byteCount);
}

/* ---------------------------------------------------------------- */

void ZuD1536_plain_ExtractBytes(const ZuD1536_plain64_state *state, uint8_t *data, unsigned int offset, unsigned int length)
{
    memcpy(data, ((const uint8_t*)state)+offset, length);
}

/* ---------------------------------------------------------------- */

void ZuD1536_plain_ExtractAndAddBytes(const ZuD1536_plain64_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length)
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

/*
** Theta: Column Parity Mixer, rough code, could be further optimized as Xoodoo way
*/
static inline void theta(ZuD1536_plain64_state *s)
{
    uint64_t *A = s->A;
    uint64_t p0 = A[0] ^ A[8]  ^ A[16];
    uint64_t p1 = A[1] ^ A[9]  ^ A[17];
    uint64_t p2 = A[2] ^ A[10] ^ A[18];
    uint64_t p3 = A[3] ^ A[11] ^ A[19];
    uint64_t p4 = A[4] ^ A[12] ^ A[20];
    uint64_t p5 = A[5] ^ A[13] ^ A[21];
    uint64_t p6 = A[6] ^ A[14] ^ A[22];
    uint64_t p7 = A[7] ^ A[15] ^ A[23];
    uint64_t e0 = ROTL64(p7, 20) ^ ROTL64(p7, 56);
    uint64_t e1 = ROTL64(p0, 20) ^ ROTL64(p0, 56);
    uint64_t e2 = ROTL64(p1, 20) ^ ROTL64(p1, 56);
    uint64_t e3 = ROTL64(p2, 20) ^ ROTL64(p2, 56);
    uint64_t e4 = ROTL64(p3, 20) ^ ROTL64(p3, 56);
    uint64_t e5 = ROTL64(p4, 20) ^ ROTL64(p4, 56);
    uint64_t e6 = ROTL64(p5, 20) ^ ROTL64(p5, 56);
    uint64_t e7 = ROTL64(p6, 20) ^ ROTL64(p6, 56);

    A[0] ^= e0; A[8]  ^= e0; A[16] ^= e0;
    A[1] ^= e1; A[9]  ^= e1; A[17] ^= e1;
    A[2] ^= e2; A[10] ^= e2; A[18] ^= e2;
    A[3] ^= e3; A[11] ^= e3; A[19] ^= e3;
    A[4] ^= e4; A[12] ^= e4; A[20] ^= e4;
    A[5] ^= e5; A[13] ^= e5; A[21] ^= e5;
    A[6] ^= e6; A[14] ^= e6; A[22] ^= e6;
    A[7] ^= e7; A[15] ^= e7; A[23] ^= e7;
}
/*
** Rho: Plane shift
*/
static inline void rho(ZuD1536_plain64_state *s)
{
    uint64_t *A = s->A;
    uint64_t a10 = A[8],  a11 = A[9],  a12 = A[10], a13 = A[11];
    uint64_t a14 = A[12], a15 = A[13], a16 = A[14], a17 = A[15];
    uint64_t a20 = A[16], a21 = A[17], a22 = A[18], a23 = A[19];
    uint64_t a24 = A[20], a25 = A[21], a26 = A[22], a27 = A[23];

    A[8]  = ROTL64(a15, 3);
    A[9]  = ROTL64(a16, 3);
    A[10] = ROTL64(a17, 3);
    A[11] = ROTL64(a10, 3);
    A[12] = ROTL64(a11, 3);
    A[13] = ROTL64(a12, 3);
    A[14] = ROTL64(a13, 3);
    A[15] = ROTL64(a14, 3);
    A[16] = a23;
    A[17] = a24;
    A[18] = a25;
    A[19] = a26;
    A[20] = a27;
    A[21] = a20;
    A[22] = a21;
    A[23] = a22;
}
/*
** Iota: Round constants
*/
static void iota(ZuD1536_plain64_state *s, uint64_t rc)
{
    s->A[0] ^= rc;
}
/*
** Chi: Non linear step, on colums
*/
static inline void chi(ZuD1536_plain64_state *s)
{
    uint64_t *A = s->A;
#define ZUD1536_CHI_COLUMN(x)                                                  \
    do {                                                                       \
        uint64_t a0 = A[(x)];                                                   \
        uint64_t a1 = A[ZUD1536_COLS + (x)];                                    \
        uint64_t a2 = A[2 * ZUD1536_COLS + (x)];                                \
        a0 ^= (~a1) & a2;                                                       \
        a1 ^= (~a2) & a0;                                                       \
        a2 ^= (~a0) & a1;                                                       \
        A[(x)] = a0;                                                            \
        A[ZUD1536_COLS + (x)] = a1;                                             \
        A[2 * ZUD1536_COLS + (x)] = a2;                                         \
    } while (0)

    ZUD1536_CHI_COLUMN(0);
    ZUD1536_CHI_COLUMN(1);
    ZUD1536_CHI_COLUMN(2);
    ZUD1536_CHI_COLUMN(3);
    ZUD1536_CHI_COLUMN(4);
    ZUD1536_CHI_COLUMN(5);
    ZUD1536_CHI_COLUMN(6);
    ZUD1536_CHI_COLUMN(7);

#undef ZUD1536_CHI_COLUMN
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
static inline void roundStep(ZuD1536_plain64_state *s, uint64_t rc)
{
                    iota(s,rc);
                    //printf("rc=%016llx A0=%016llx\n",
                    //    (unsigned long long)rc,
                    //    (unsigned long long)s->A[0]);
                    chi(s);
                    rho(s);
                    theta(s);
                    rho(s);
}

void ZuD1536_plain_Permute_Nrounds(ZuD1536_plain64_state *state, unsigned int nr)
{
    if (nr > ZUD1536_MAXROUNDS){
        nr = ZUD1536_MAXROUNDS;
    }
    for (unsigned int i = ZUD1536_MAXROUNDS - nr; i < ZUD1536_MAXROUNDS; ++i ) {
        roundStep(state, RC[i]);
    }
}

void ZuD1536_plain_Permute_6rounds(ZuD1536_plain64_state *state)
{
    roundStep(state, ZUD1536_rc6);
    roundStep(state, ZUD1536_rc5);
    roundStep(state, ZUD1536_rc4);
    roundStep(state, ZUD1536_rc3);
    roundStep(state, ZUD1536_rc2);
    roundStep(state, ZUD1536_rc1);
}

void ZuD1536_plain_Permute_12rounds(ZuD1536_plain64_state *state)
{
    roundStep(state, ZUD1536_rc12);
    roundStep(state, ZUD1536_rc11);
    roundStep(state, ZUD1536_rc10);
    roundStep(state, ZUD1536_rc9);
    roundStep(state, ZUD1536_rc8);
    roundStep(state, ZUD1536_rc7);
    roundStep(state, ZUD1536_rc6);
    roundStep(state, ZUD1536_rc5);
    roundStep(state, ZUD1536_rc4);
    roundStep(state, ZUD1536_rc3);
    roundStep(state, ZUD1536_rc2);
    roundStep(state, ZUD1536_rc1);
}
