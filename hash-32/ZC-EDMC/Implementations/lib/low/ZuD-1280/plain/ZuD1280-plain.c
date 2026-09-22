/*
Plain C implementation of ZuD (5x4x64).

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

#include <stdio.h>
#include <string.h>
#include "brg_endian.h"
#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
#error Expecting a little-endian platform
#endif

#include "../ZuD1280.h"
#include "ZuD1280-plain.h"

#define ZUD1280_COLS ZUD1280_NCOLUMNS
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

void ZuD1280_plain_Initialize(ZuD1280_plain64_state *state)
{
    memset(state, 0, sizeof(*state));
}

/* ---------------------------------------------------------------- */

void ZuD1280_plain_AddBytes(ZuD1280_plain64_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
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

void ZuD1280_plain_OverwriteBytes(ZuD1280_plain64_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
        memcpy(((uint8_t*)state)+offset, data, length);
}

/* ---------------------------------------------------------------- */

void ZuD1280_plain_OverwriteWithZeroes(ZuD1280_plain64_state *state, unsigned int byteCount)
{
    memset(state, 0, byteCount);
}

/* ---------------------------------------------------------------- */

void ZuD1280_plain_ExtractBytes(const ZuD1280_plain64_state *state, uint8_t *data, unsigned int offset, unsigned int length)
{
    memcpy(data, ((const uint8_t*)state)+offset, length);
}

/* ---------------------------------------------------------------- */

void ZuD1280_plain_ExtractAndAddBytes(const ZuD1280_plain64_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length)
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
static inline void theta(ZuD1280_plain64_state *s)
{
    uint64_t *A = s->A;
    uint64_t p0 = A[0] ^ A[4] ^ A[8]  ^ A[12] ^ A[16];
    uint64_t p1 = A[1] ^ A[5] ^ A[9]  ^ A[13] ^ A[17];
    uint64_t p2 = A[2] ^ A[6] ^ A[10] ^ A[14] ^ A[18];
    uint64_t p3 = A[3] ^ A[7] ^ A[11] ^ A[15] ^ A[19];
    uint64_t e0 = ROTL64(p3, 5) ^ ROTL64(p3, 14);
    uint64_t e1 = ROTL64(p0, 5) ^ ROTL64(p0, 14);
    uint64_t e2 = ROTL64(p1, 5) ^ ROTL64(p1, 14);
    uint64_t e3 = ROTL64(p2, 5) ^ ROTL64(p2, 14);

    A[0]  ^= e0; A[4]  ^= e0; A[8]  ^= e0; A[12] ^= e0; A[16] ^= e0;
    A[1]  ^= e1; A[5]  ^= e1; A[9]  ^= e1; A[13] ^= e1; A[17] ^= e1;
    A[2]  ^= e2; A[6]  ^= e2; A[10] ^= e2; A[14] ^= e2; A[18] ^= e2;
    A[3]  ^= e3; A[7]  ^= e3; A[11] ^= e3; A[15] ^= e3; A[19] ^= e3;
}
/*
** Rho: Plane shift
*/
static inline void rho(ZuD1280_plain64_state *s)
{
    uint64_t *A = s->A;
    uint64_t a10 = A[4],  a11 = A[5],  a12 = A[6],  a13 = A[7];
    uint64_t a20 = A[8],  a21 = A[9],  a22 = A[10], a23 = A[11];
    uint64_t a30 = A[12], a31 = A[13], a32 = A[14], a33 = A[15];
    uint64_t a40 = A[16], a41 = A[17], a42 = A[18], a43 = A[19];

    A[4]  = ROTL64(a13, 5);
    A[5]  = ROTL64(a10, 5);
    A[6]  = ROTL64(a11, 5);
    A[7]  = ROTL64(a12, 5);
    A[8]  = ROTL64(a21, 24);
    A[9]  = ROTL64(a22, 24);
    A[10] = ROTL64(a23, 24);
    A[11] = ROTL64(a20, 24);
    A[12] = ROTL64(a30, 1);
    A[13] = ROTL64(a31, 1);
    A[14] = ROTL64(a32, 1);
    A[15] = ROTL64(a33, 1);
    A[16] = ROTL64(a42, 3);
    A[17] = ROTL64(a43, 3);
    A[18] = ROTL64(a40, 3);
    A[19] = ROTL64(a41, 3);
}
/*
** Iota: Round constants
*/
static void iota(ZuD1280_plain64_state *s, uint64_t rc)
{
    s->A[0] ^= rc;
}
/*
** Chi: Non linear step, on colums
*/
static inline void chi(ZuD1280_plain64_state *s)
{
    uint64_t *A = s->A;
#define ZUD1280_CHI_COLUMN(x)                                                  \
    do {                                                                       \
        uint64_t b0 = A[(x)];                                                   \
        uint64_t b1 = A[ZUD1280_COLS + (x)];                                    \
        uint64_t b2 = A[2 * ZUD1280_COLS + (x)];                                \
        uint64_t b3 = A[3 * ZUD1280_COLS + (x)];                                \
        uint64_t b4 = A[4 * ZUD1280_COLS + (x)];                                \
        A[(x)] = b0 ^ ((~b1) & b2);                                             \
        A[ZUD1280_COLS + (x)] = b1 ^ ((~b2) & b3);                              \
        A[2 * ZUD1280_COLS + (x)] = b2 ^ ((~b3) & b4);                          \
        A[3 * ZUD1280_COLS + (x)] = b3 ^ ((~b4) & b0);                          \
        A[4 * ZUD1280_COLS + (x)] = b4 ^ ((~b0) & b1);                          \
    } while (0)

    ZUD1280_CHI_COLUMN(0);
    ZUD1280_CHI_COLUMN(1);
    ZUD1280_CHI_COLUMN(2);
    ZUD1280_CHI_COLUMN(3);

#undef ZUD1280_CHI_COLUMN
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
static inline void roundStep(ZuD1280_plain64_state *s, uint64_t rc)
{
                    iota(s,rc);
                    //printf("rc=%016llx A0=%016llx\n %016llx\n %016llx\n %016llx\n %016llx\n",
                    //    (unsigned long long)rc,
                    //    (unsigned long long)s->A[0], (unsigned long long)s->A[1], (unsigned long long)s->A[2], (unsigned long long)s->A[3], (unsigned long long)s->A[10]);
                    chi(s);
                    rho(s);
                    theta(s);
                    rho(s);
}

void ZuD1280_plain_Permute_Nrounds(ZuD1280_plain64_state *state, unsigned int nr)
{
    if (nr > ZUD1280_MAXROUNDS){
        nr = ZUD1280_MAXROUNDS;
    }
    for (unsigned int i = ZUD1280_MAXROUNDS - nr; i < ZUD1280_MAXROUNDS; ++i ) {
        roundStep(state, RC[i]);
    }
}

void ZuD1280_plain_Permute_6rounds(ZuD1280_plain64_state *state)
{
    roundStep(state, ZUD1280_rc6);
    roundStep(state, ZUD1280_rc5);
    roundStep(state, ZUD1280_rc4);
    roundStep(state, ZUD1280_rc3);
    roundStep(state, ZUD1280_rc2);
    roundStep(state, ZUD1280_rc1);
}

void ZuD1280_plain_Permute_12rounds(ZuD1280_plain64_state *state)
{
    roundStep(state, ZUD1280_rc12);
    roundStep(state, ZUD1280_rc11);
    roundStep(state, ZUD1280_rc10);
    roundStep(state, ZUD1280_rc9);
    roundStep(state, ZUD1280_rc8);
    roundStep(state, ZUD1280_rc7);
    roundStep(state, ZUD1280_rc6);
    roundStep(state, ZUD1280_rc5);
    roundStep(state, ZUD1280_rc4);
    roundStep(state, ZUD1280_rc3);
    roundStep(state, ZUD1280_rc2);
    roundStep(state, ZUD1280_rc1);
}
