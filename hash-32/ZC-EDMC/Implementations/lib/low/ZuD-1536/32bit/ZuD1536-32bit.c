/*
 * ZC1536 — 32-bit friendly implementation (3x8x64) without uint64_t arithmetic.
 * Compile as ISO C99.
 */

#include <string.h>
#include "ZuD1536-32bit.h"

#include "brg_endian.h"
#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
#error Expecting a little-endian platform
#endif

#define COLS ZUD1536_NCOLUMNS

typedef struct { uint32_t lo, hi; } u64x2;

static inline u64x2 u64x2_xor(u64x2 a, u64x2 b) { u64x2 r = { a.lo ^ b.lo, a.hi ^ b.hi }; return r; }
static inline u64x2 u64x2_andnot(u64x2 a, u64x2 b) { u64x2 r = { (~a.lo) & b.lo, (~a.hi) & b.hi }; return r; }

static inline uint32_t load32_unaligned(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static inline u64x2 u64x2_rol(u64x2 a, unsigned int k)
{
    k &= 63u;
    if (k == 0u) return a;
    if (k < 32u) {
        u64x2 r;
        r.lo = (a.lo << k) | (a.hi >> (32u - k));
        r.hi = (a.hi << k) | (a.lo >> (32u - k));
        return r;
    } else if (k == 32u) {
        u64x2 r = { a.hi, a.lo };
        return r;
    } else {
        unsigned int s = k - 32u;
        u64x2 r;
        r.lo = (a.hi << s) | (a.lo >> (32u - s));
        r.hi = (a.lo << s) | (a.hi >> (32u - s));
        return r;
    }
}

static inline u64x2 lane_get(const ZuD1536_32_state *s, unsigned int row, unsigned int col)
{
    unsigned int i = (row * COLS + col) * 2u;
    u64x2 r = { s->A[i], s->A[i + 1u] };
    return r;
}

static inline void lane_set(ZuD1536_32_state *s, unsigned int row, unsigned int col, u64x2 v)
{
    unsigned int i = (row * COLS + col) * 2u;
    s->A[i] = v.lo;
    s->A[i + 1u] = v.hi;
}

static inline u64x2 laneParity(const ZuD1536_32_state *s, unsigned int x)
{
    u64x2 p = lane_get(s, 0, x);
    p = u64x2_xor(p, lane_get(s, 1, x));
    p = u64x2_xor(p, lane_get(s, 2, x));
    return p;
}

static void theta(ZuD1536_32_state *s)
{
    u64x2 P[COLS];
    for (unsigned int x = 0; x < COLS; x++)
        P[x] = laneParity(s, x);

    for (unsigned int x = 0; x < COLS; x++) {
        u64x2 p = P[(x + COLS - 1u) & (COLS - 1u)];
        u64x2 e = u64x2_xor(u64x2_rol(p, 20u), u64x2_rol(p, 56u));
        for (unsigned int y = 0; y < 3u; y++) {
            u64x2 a = lane_get(s, y, x);
            lane_set(s, y, x, u64x2_xor(a, e));
        }
    }
}

static void rho(ZuD1536_32_state *s)
{
    /* Row 2: shift 5, rotate 0 */
    {
        u64x2 t[COLS];
        for (unsigned int x = 0; x < COLS; x++) t[x] = lane_get(s, 2, x);
        for (unsigned int x = 0; x < COLS; x++) lane_set(s, 2, x, u64x2_rol(t[(x + COLS - 5u) & (COLS - 1u)], 0u));
    }
    /* Row 1: shift 3, rotate 3 */
    {
        u64x2 t[COLS];
        for (unsigned int x = 0; x < COLS; x++) t[x] = lane_get(s, 1, x);
        for (unsigned int x = 0; x < COLS; x++) lane_set(s, 1, x, u64x2_rol(t[(x + COLS - 3u) & (COLS - 1u)], 3u));
    }
}

static void chi(ZuD1536_32_state *s)
{
    for (unsigned int x = 0; x < COLS; x++) {
        u64x2 a0 = lane_get(s, 0, x);
        u64x2 a1 = lane_get(s, 1, x);
        u64x2 a2 = lane_get(s, 2, x);

        a0 = u64x2_xor(a0, u64x2_andnot(a1, a2));
        a1 = u64x2_xor(a1, u64x2_andnot(a2, a0));
        a2 = u64x2_xor(a2, u64x2_andnot(a0, a1));

        lane_set(s, 0, x, a0);
        lane_set(s, 1, x, a1);
        lane_set(s, 2, x, a2);
    }
}

static void iota(ZuD1536_32_state *s, uint32_t rc_lo, uint32_t rc_hi)
{
    s->A[0] ^= rc_lo;
    s->A[1] ^= rc_hi;
}

static inline void roundStep(ZuD1536_32_state *s, uint32_t rc_lo, uint32_t rc_hi)
{
    iota(s, rc_lo, rc_hi);
    chi(s);
    rho(s);
    theta(s);
    rho(s);
}

void ZuD1536_32_Initialize(ZuD1536_32_state *state)
{
    memset(state, 0, sizeof(*state));
}

void ZuD1536_32_AddBytes(ZuD1536_32_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
    uint8_t *st = (uint8_t *)state;
    unsigned int pos = offset;
    unsigned int sizeLeft = length;
    const uint8_t *cur = data;

    while ((sizeLeft != 0u) && ((pos & 3u) != 0u)) {
        st[pos++] ^= *cur++;
        --sizeLeft;
    }

    while (sizeLeft >= 4u) {
        state->A[pos >> 2] ^= load32_unaligned(cur);
        cur += 4;
        pos += 4;
        sizeLeft -= 4u;
    }

    while (sizeLeft != 0u) {
        st[pos++] ^= *cur++;
        --sizeLeft;
    }
}

void ZuD1536_32_OverwriteBytes(ZuD1536_32_state *state, const uint8_t *data, unsigned int offset, unsigned int length)
{
    memcpy(((uint8_t *)state) + offset, data, length);
}

void ZuD1536_32_OverwriteWithZeroes(ZuD1536_32_state *state, unsigned int byteCount)
{
    memset(state, 0, byteCount);
}

void ZuD1536_32_ExtractBytes(const ZuD1536_32_state *state, uint8_t *data, unsigned int offset, unsigned int length)
{
    memcpy(data, ((const uint8_t *)state) + offset, length);
}

void ZuD1536_32_ExtractAndAddBytes(const ZuD1536_32_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length)
{
    const uint8_t *st = ((const uint8_t *)state) + offset;
    for (unsigned int i = 0; i < length; i++)
        output[i] = input[i] ^ st[i];
}

static const uint32_t RC_LO[ZUD1536_MAXROUNDS] = {
    0x00000058u, 0x00000038u, 0x000003C0u, 0x000000D0u, 0x00000120u, 0x00000014u,
    0x00000060u, 0x0000002Cu, 0x00000380u, 0x000000F0u, 0x000001A0u, 0x00000012u
};

void ZuD1536_32_Permute_Nrounds(ZuD1536_32_state *state, unsigned int nr)
{
    if (nr > ZUD1536_MAXROUNDS) nr = ZUD1536_MAXROUNDS;
    for (unsigned int i = ZUD1536_MAXROUNDS - nr; i < ZUD1536_MAXROUNDS; ++i)
        roundStep(state, RC_LO[i], 0u);
}

void ZuD1536_32_Permute_6rounds(ZuD1536_32_state *state)
{
    roundStep(state, 0x00000060u, 0u);
    roundStep(state, 0x0000002Cu, 0u);
    roundStep(state, 0x00000380u, 0u);
    roundStep(state, 0x000000F0u, 0u);
    roundStep(state, 0x000001A0u, 0u);
    roundStep(state, 0x00000012u, 0u);
}

void ZuD1536_32_Permute_12rounds(ZuD1536_32_state *state)
{
    for (unsigned int i = 0; i < ZUD1536_MAXROUNDS; i++)
        roundStep(state, RC_LO[i], 0u);
}
