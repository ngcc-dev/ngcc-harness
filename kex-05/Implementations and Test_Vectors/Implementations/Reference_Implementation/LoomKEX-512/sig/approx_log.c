/*
 * approx_log.c -- translation-unit anchor for approx_log.h.
 *
 * The ApproxLog kernel (shuttle_log2_frac_q62{,_x2} and the
 * shuttle_log_{mulhi,eqmask,fetch_row} primitives) plus the constant table
 * kShuttleLogPoly are `static inline` / `static const` in
 * tools/approx_log_poly.h so the kernel inlines into the caller and the
 * optimizer can fold the full-table scan.  This .c therefore holds NO
 * logic of its own; it mirrors approx_exp.c's role (symbol anchor for
 * -DDISABLE_NAMESPACE, file-map symmetry, future non-inline fallback). The
 * 448-byte kShuttleLogPoly is duplicated per TU (acceptable for ref/; the
 * SIMD single-definition extern-const option is a size-only deferral).
 * `static inline` functions and the unreferenced `static const` table do
 * NOT trip -Wunused under -Werror in this TU.
 */
#include "approx_log.h"
