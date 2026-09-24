#ifndef YUANYANG_512_PARAMS_H
#define YUANYANG_512_PARAMS_H

#include <stdint.h>

#define YUANYANG_D                   512u
#define YUANYANG_LOGD                9u
#define YUANYANG_Q                   2689u
#define YUANYANG_NTTLENGTH           64
#define YUANYANG_ROOTQ               2327

#ifndef YUANYANG_SALT_BYTES
#define YUANYANG_SALT_BYTES          26u
#endif
#ifndef YUANYANG_COMP_SIG_BYTES
/*
 * rANS signature payload bytes. For this parameter set, l=5, gives the best results.
 * slen could be tweaked to get smaller signature, but higher rejection rate.
 * A 10000-signature sweep gives the following distribution of len -> rejection:
 * 	 540 (566 with salt) ->  0.000%
 * 	 539 (565 with salt) ->  0.020%
 * 	 538 (564 with salt) ->  0.070%
 * 	 537 (563 with salt) ->  0.339%
 * 	 536 (562 with salt) ->  0.428%
 * 	 535 (561 with salt) ->  0.862%
 * 	 534 (560 with salt) ->  2.286%
 * 	 533 (559 with salt) -> 11.127%
 */
#define YUANYANG_COMP_SIG_BYTES      535u
#endif
#ifndef YUANYANG_SIG_LOW_BITS
#define YUANYANG_SIG_LOW_BITS        5u
#endif
#if YUANYANG_SIG_LOW_BITS < 1u || YUANYANG_SIG_LOW_BITS > 15u
#error YUANYANG_SIG_LOW_BITS must be between 1 and 15
#endif
#define YUANYANG_SIGNATURE_BYTES     (YUANYANG_SALT_BYTES + YUANYANG_COMP_SIG_BYTES)
#define YUANYANG_PK_BLOCK_COEFFS     4u
#define YUANYANG_PK_BLOCK_BITS       46u
#define YUANYANG_H_BYTES \
	((YUANYANG_D / YUANYANG_PK_BLOCK_COEFFS) * YUANYANG_PK_BLOCK_BITS / 8u)
#define YUANYANG_PK_PREFIX_BYTES     2u
#define YUANYANG_PK_BYTES            (YUANYANG_PK_PREFIX_BYTES + YUANYANG_H_BYTES)


/*
 * Reference expanded secret-key layout:
 *   h || f || g || F || G
 *   || u_hat || A_hat || Sigma_delta
 *
 * Public keys are serialized as q || packed h, where q is a little-endian
 * uint16 and h is compressed so that we account for the fact that each
 * coefficient is mod q. Secret keys store packed h without the q prefix. The
 * compact basis polynomials (f, g, F, G) use signed 8-bit coefficients. A_hat
 * is lower-triangular from the Cholesky step, so A_hat_01 is always zero and is
 * not serialized. Sigma_delta is a full 2x2 matrix. Floating precomputations
 * are exported as scaled signed integers: u_hat on 2 bytes with scale 2^11,
 * A_hat on 4 bytes with scale 2^22, and Sigma_delta as packed signed 22-bit
 * two's-complement coefficients with scale 2^22. The signing code derives T
 * from Sigma_delta after decoding.
 */
#define YUANYANG_U_HAT_PRECISION_BITS 11u
#define YUANYANG_A_HAT_PRECISION_BITS 22u
#define YUANYANG_SIGMA_DELTA_BITS     YUANYANG_A_HAT_PRECISION_BITS
#define YUANYANG_U_HAT_SCALE          (1u << YUANYANG_U_HAT_PRECISION_BITS)
#define YUANYANG_A_HAT_SCALE          (1u << YUANYANG_A_HAT_PRECISION_BITS)
#define YUANYANG_SIGMA_DELTA_SCALE    YUANYANG_A_HAT_SCALE

#define YUANYANG_U_HAT_ENCODED_BYTES  2u
#define YUANYANG_A_HAT_ENCODED_BYTES  4u
#define YUANYANG_COMPACT_SK_BYTES    (YUANYANG_H_BYTES + 4u * YUANYANG_D)
#define YUANYANG_U_HAT_POLY_BYTES    (YUANYANG_D * YUANYANG_U_HAT_ENCODED_BYTES)
#define YUANYANG_A_HAT_STORED_POLYS  3u
#define YUANYANG_A_HAT_LOWER_BYTES \
	(YUANYANG_A_HAT_STORED_POLYS * YUANYANG_D * YUANYANG_A_HAT_ENCODED_BYTES)
#define YUANYANG_SIGMA_DELTA_MATRIX2X2_BYTES     \
	((4u * YUANYANG_D * YUANYANG_SIGMA_DELTA_BITS + 7u) / 8u)
#define YUANYANG_SK_BYTES            \
	(YUANYANG_COMPACT_SK_BYTES      \
	+ YUANYANG_U_HAT_POLY_BYTES    \
	+ YUANYANG_A_HAT_LOWER_BYTES   \
	+ YUANYANG_SIGMA_DELTA_MATRIX2X2_BYTES)

#define YUANYANG_REJECTION_BOUND     2337u
#define YUANYANG_REJECTION_BOUND_SQ  ((uint64_t)YUANYANG_REJECTION_BOUND * (uint64_t)YUANYANG_REJECTION_BOUND)
#define NORM_FG_MIN 				 2964u

/*
 * A 2000-signature diagnostic compared 1, 2, 3, 4 and 8 Jacobi iterations
 * against the exact per-slot solve.  One iteration had a visibly larger
 * threshold error; two or more iterations reached the same fixed-point
 * rounding plateau in that sample.  Keep three iterations as a small margin
 * without changing deterministic KAT behavior.
 */
#define YUANYANG_T_JACOBI_ITERATIONS  3u

#endif
