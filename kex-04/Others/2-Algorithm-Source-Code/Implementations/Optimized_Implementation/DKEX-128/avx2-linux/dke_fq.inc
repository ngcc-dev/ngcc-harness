/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 Montgomery reduction macros for GAS assembler.
 * Used by ntt and basemul assembly.
 *
 * Montgomery reduce: given lo=a*b mod 2^16, hi=a*b >> 16
 *   t = lo * QINV (mod 2^16)
 *   result = hi - (t * Q) >> 16  = hi - mulhi(t, Q)
 *
 * fqmul(a, b):
 *   lo = vpmullw(a, b)
 *   hi = vpmulhw(a, b)
 *   t  = vpmullw(lo, QINV_VEC)
 *   t  = vpmulhw(t, Q_VEC)
 *   result = vpsubw(hi, t)
 */

/* Montgomery reduce: inputs in %ymm_lo, %ymm_hi, constants in %ymm_qinv, %ymm_q
 * Clobbers %ymm_tmp. Result in %ymm_hi. */
.macro montgomery_reduce lo, hi, qinv, q, tmp
    vpmullw \qinv, \lo, \tmp
    vpmulhw \q, \tmp, \tmp
    vpsubw  \tmp, \hi, \hi
.endm

/* fqmul: a * b with Montgomery reduce.
 * Result replaces 'hi'. Clobbers 'lo' and 'tmp'. */
.macro fqmul a, b, lo, hi, qinv, q, tmp
    vpmullw \b, \a, \lo
    vpmulhw \b, \a, \hi
    montgomery_reduce \lo, \hi, \qinv, \q, \tmp
.endm
