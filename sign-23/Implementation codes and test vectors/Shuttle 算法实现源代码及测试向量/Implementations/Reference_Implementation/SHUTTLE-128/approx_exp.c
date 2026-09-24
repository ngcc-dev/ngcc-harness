/*
 * approx_exp.c -- translation-unit anchor for approx_exp.h.
 *
 * The ApproxExp kernel (shuttle_exp_accept_poly_q64{,_x4} and the
 * shuttle_high64_* primitives) is `static inline` in
 * tools/approx_exp_poly.h so it inlines straight into the SIMD-lane
 * caller. This .c therefore holds NO logic of its own.  It exists so the
 * build has a stable TU to (a) anchor the namespaced symbols if
 * -DDISABLE_NAMESPACE forces external linkage, and (b) host any future
 * non-inline fallback, keeping the file map symmetric with approx_log.c.
 * `static inline` functions that are never referenced here do NOT trigger
 * -Wunused-function, so this stays warning-clean under -Werror.
 */
#include "approx_exp.h"
