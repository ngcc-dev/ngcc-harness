#ifndef POLY_INV_NTT_H
#define POLY_INV_NTT_H

#include <stddef.h>
#include <stdint.h>

/*
   Build-time parameters:

      MOD_Q              odd prime, MOD_Q < 10000
      MOD_OMEGA          element of exact order 2^MOD_OMEGA_LOG modulo MOD_Q
      MOD_OMEGA_LOG      order exponent of MOD_OMEGA
      MOD_BARRETT_MU     floor(2^32 / MOD_Q)

   All values are ordinary residues in [0, MOD_Q).  No Montgomery form.

   If n = m*s, the partial NTT is the negacyclic NTT of length m over
   coefficient blocks of s residues.  It is executed over the whole n-slot
   array at once; no per-coefficient mini-NTTs and no half-complex packing.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t modq_t;

#define PARTIAL_NTT_TWIDDLES_LEN(m) (3 * (m))

modq_t mod_reduce_u32(uint32_t x);
modq_t mod_add(modq_t a, modq_t b);
modq_t mod_sub(modq_t a, modq_t b);
modq_t mod_neg(modq_t a);
modq_t mod_mul(modq_t a, modq_t b);
modq_t mod_pow(modq_t a, uint32_t e);
modq_t mod_inv(modq_t a);

/*
   Default split for degree n:
       n = m*s,
       m = min(n, 2^(MOD_OMEGA_LOG-1)).

   psi_out receives an element of exact order 2m.
*/
int partial_ntt_default_split(size_t n, size_t *m_out, size_t *s_out,
                              modq_t *psi_out);

/*
   Precompute twiddles for a full negacyclic NTT of length m.
   psi must have exact order 2m.

   tw must have PARTIAL_NTT_TWIDDLES_LEN(m) entries:
       tw[0*m + k] = psi^rev(k)       for forward butterflies
       tw[1*m + k] = psi^(-rev(k))    for inverse butterflies
       tw[2*m + j] = psi^(2*rev(j)+1) root attached to output block j

   rev() is bit reversal over log2(m) bits.  Only entries actually used by
   the iterative butterflies need nonzero values, but all root entries are set.
*/
int partial_ntt_make_twiddles(modq_t *tw, size_t m, modq_t psi);

/*
   In-place full partial negacyclic NTT over n residues.

   The array is viewed as m consecutive blocks of s coefficients:
       block i = a[i*s .. i*s+s-1].

   The forward transform maps blocks A_i to A(r_j), where
       r_j = tw[2*m + j] = psi^(2*rev(j)+1).

   The inverse recovers the original block array and includes multiplication
   by 1/m.  For m=1 both functions are no-ops.
*/
int partial_ntt_inplace(modq_t *a, size_t n, size_t m,
                        const modq_t *tw);
int partial_intt_inplace(modq_t *a, size_t n, size_t m,
                         const modq_t *tw);

int partial_ntt(modq_t *out, const modq_t *f, size_t n, size_t m,
                const modq_t *tw);
int partial_intt(modq_t *f, const modq_t *in, size_t n, size_t m,
                 const modq_t *tw);

/* c = a*b mod (x^n+1, MOD_Q). */
int poly_mul_xn1_q(modq_t *c, const modq_t *a, const modq_t *b, size_t n);

/* out = 1/f mod (x^n+1, MOD_Q).  Returns -1 if f is not invertible. */
int poly_inv_xn1_q(modq_t *out, const modq_t *f, size_t n);

#ifdef __cplusplus
}
#endif

#endif
