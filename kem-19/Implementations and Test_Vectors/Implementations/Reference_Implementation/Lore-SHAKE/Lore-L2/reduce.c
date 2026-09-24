#include "reduce.h"
#include "params.h"
#include <stdint.h>

/*
 * montgomery_reduce and barrett_reduce implementations
 * are now static inline in reduce.h for cross-TU inlining.
 */

void poly_reduce(poly *r) {
    for(int i=0; i<LORE_N; i+=4) {
        r->coeffs[i]   = barrett_reduce(r->coeffs[i]);
        r->coeffs[i+1] = barrett_reduce(r->coeffs[i+1]);
        r->coeffs[i+2] = barrett_reduce(r->coeffs[i+2]);
        r->coeffs[i+3] = barrett_reduce(r->coeffs[i+3]);
    }
}
