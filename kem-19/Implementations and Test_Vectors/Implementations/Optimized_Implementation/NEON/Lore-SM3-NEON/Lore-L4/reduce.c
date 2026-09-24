#include "reduce.h"
#include "params.h"
#include <stdint.h>
#ifdef LORE_USE_NEON_CORE
#include "simd/neon_core.h"
#endif

/*
 * montgomery_reduce and barrett_reduce implementations
 * are now static inline in reduce.h for cross-TU inlining.
 */

void poly_reduce(poly *r) {
#ifdef LORE_USE_NEON_CORE
    int i = 0;

    for(; i <= LORE_N - 8; i += 8) {
        const int16x8_t coeffs = vld1q_s16(&r->coeffs[i]);
        vst1q_s16(&r->coeffs[i], lore_barrett_reduce_s16x8(coeffs));
    }
    for(; i < LORE_N; ++i) {
        r->coeffs[i] = barrett_reduce(r->coeffs[i]);
    }
#else
    for(int i=0; i<LORE_N; i+=4) {
        r->coeffs[i]   = barrett_reduce(r->coeffs[i]);
        r->coeffs[i+1] = barrett_reduce(r->coeffs[i+1]);
        r->coeffs[i+2] = barrett_reduce(r->coeffs[i+2]);
        r->coeffs[i+3] = barrett_reduce(r->coeffs[i+3]);
    }
#endif
}
