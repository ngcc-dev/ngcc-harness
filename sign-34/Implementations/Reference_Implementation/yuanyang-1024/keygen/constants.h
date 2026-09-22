#ifndef YUANYANG_1024_KEYGEN_CONSTANTS_H
#define YUANYANG_1024_KEYGEN_CONSTANTS_H

#include <stdint.h>

/*
 * Keygen-local floating constants from the 1024 parameter table and the
 * PairGen/SamplerPrecomp algorithms in the specification.
 */
#define YYKG_Q                  4481.0
#define YYKG_QUALITY_ALPHA      1.4
#define YYKG_LOWER_RADIUS       63.12
#define YYKG_UPPER_RADIUS       78.41
#define YYKG_SIGMA_SIG          92.37
#define YYKG_SMOOTHING_ETA      0.974
#define YYKG_NORM_FG_MIN        5076u

#define YYKG_LOWER_RADIUS_SQ \
	(YYKG_LOWER_RADIUS * YYKG_LOWER_RADIUS)
#define YYKG_RADIUS_SQ_SPAN \
	((YYKG_UPPER_RADIUS * YYKG_UPPER_RADIUS) - YYKG_LOWER_RADIUS_SQ)
#define YYKG_PAIRGEN_NORM_MIN \
	(YYKG_Q / (YYKG_QUALITY_ALPHA * YYKG_QUALITY_ALPHA))
#define YYKG_PAIRGEN_NORM_MAX \
	(YYKG_Q * YYKG_QUALITY_ALPHA * YYKG_QUALITY_ALPHA)
#define YYKG_SIGMA \
	(YYKG_SIGMA_SIG / YYKG_SMOOTHING_ETA)
#define YYKG_SIGMA_SQ \
	(YYKG_SIGMA * YYKG_SIGMA)

#define YYKG_TWO_PI             6.283185307179586476925286766559
#define YYKG_HALF_PI            1.570796326794896619231321691640
#define YYKG_TWO_POW_M53        0x1p-53
#define YYKG_U53_MASK           UINT64_C(0x1FFFFFFFFFFFFF)

#endif
