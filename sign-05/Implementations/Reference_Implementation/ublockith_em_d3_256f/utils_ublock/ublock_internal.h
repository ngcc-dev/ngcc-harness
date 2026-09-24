#ifndef UBLOCK_INTERNAL_H
#define UBLOCK_INTERNAL_H

#include <stdint.h>

#define UB_B0(w) ((w) >> 24)
#define UB_B1(w) (((w) >> 16) & 0xFFu)
#define UB_B2(w) (((w) >> 8) & 0xFFu)
#define UB_B3(w) ((w) & 0xFFu)
#define UB_PACK(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

static inline void ublock_perm_PL(uint32_t* out, const uint32_t* in)
{
    const uint32_t w0 = in[0], w1 = in[1], w2 = in[2], w3 = in[3];
    out[0]            = UB_PACK(UB_B2(w0), UB_B3(w1), UB_B0(w2), UB_B1(w3));
    out[1]            = UB_PACK(UB_B3(w0), UB_B2(w1), UB_B1(w2), UB_B0(w3));
    out[2]            = UB_PACK(UB_B1(w0), UB_B0(w1), UB_B3(w3), UB_B2(w2));
    out[3]            = UB_PACK(UB_B2(w3), UB_B3(w2), UB_B1(w1), UB_B0(w0));
}

static inline void ublock_perm_PR(uint32_t* out, const uint32_t* in)
{
    const uint32_t w0 = in[0], w1 = in[1], w2 = in[2], w3 = in[3];
    out[0]            = UB_PACK(UB_B2(w1), UB_B3(w2), UB_B1(w0), UB_B0(w3));
    out[1]            = UB_PACK(UB_B1(w2), UB_B0(w1), UB_B2(w0), UB_B3(w3));
    out[2]            = UB_PACK(UB_B3(w1), UB_B0(w0), UB_B1(w3), UB_B2(w2));
    out[3]            = UB_PACK(UB_B2(w3), UB_B3(w0), UB_B0(w2), UB_B1(w1));
}

#undef UB_B0
#undef UB_B1
#undef UB_B2
#undef UB_B3
#undef UB_PACK

#endif
