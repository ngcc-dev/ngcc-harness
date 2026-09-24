/*
 * AVX2 constant data for DKE-512 (Q=7681, QINV=-7679).
 */
#include "../parameters.h"

#if defined(DKE_USE_AVX2) && (DKE_MODE == 512)

#include <stdint.h>

#if defined(_MSC_VER)
__declspec(align(32))
#else
__attribute__((aligned(32)))
#endif
/* Layout (byte offsets used by the asm via memory operands):
 *   [ 0..15] 16 x Q        (byte 0)
 *   [16..31] 16 x QINV     (byte 32)
 *   [32..47] 16 x V        (byte 64)  Barrett: V = round(2^26 / Q)
 *   [48..63] 16 x (1<<9)   (byte 96)  Barrett rounding constant
 *   [64..79] 16 x INVNTT_F (byte 128) final Montgomery scaling for inverse NTT */
#define DKE_BARRETT_V ((int16_t)(((1 << 26) + DKE_Q / 2) / DKE_Q))
const int16_t dke_qdata512[80] = {
    /* [0..15]: 16 x Q */
    DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q,
    DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q, DKE_Q,
    /* [16..31]: 16 x QINV */
    (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV,
    (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV,
    (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV,
    (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV, (int16_t)DKE_QINV,
    /* [32..47]: 16 x V (Barrett) */
    DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V,
    DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V,
    DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V,
    DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V, DKE_BARRETT_V,
    /* [48..63]: 16 x (1<<9) */
    512, 512, 512, 512, 512, 512, 512, 512,
    512, 512, 512, 512, 512, 512, 512, 512,
    /* [64..79]: 16 x INVNTT_F */
    DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F,
    DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F,
    DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F,
    DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F, DKE_INVNTT_F,
};

#endif
