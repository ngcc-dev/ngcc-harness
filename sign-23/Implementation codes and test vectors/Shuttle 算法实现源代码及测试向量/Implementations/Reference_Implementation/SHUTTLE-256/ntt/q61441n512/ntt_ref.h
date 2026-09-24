#ifndef S512_NTT_REF_H
#define S512_NTT_REF_H

#include <stdint.h>

/* SHUTTLE namespacing: every public scalar NTT symbol carries the s512_
 * prefix so the three vendored configs co-link in one process.  This mirrors
 * the s512_ prefix the AVX2/AVX512 generators thread through their .S/consts.
 * The internal macro names below (NTT_Q/NTT_N) are also prefixed (S512_*) to
 * avoid the cross-config #define collision when more than one config's header
 * is pulled into a TU; the un-prefixed aliases are kept for the body of
 * ntt_ref.c only. */
#define ntt_ref_R2 s512_ntt_ref_R2
#define ntt_ref_init s512_ntt_ref_init
#define ntt_ref_fqmul s512_ntt_ref_fqmul
#define ntt_ref s512_ntt_ref
#define invntt_tomont_ref s512_invntt_tomont_ref
#define pointwise_ref s512_pointwise_ref
#define polymul_schoolbook s512_polymul_schoolbook

#define NTT_Q 61441
#define NTT_N 512

extern uint16_t ntt_ref_R2;                       /* R^2 mod q */

void     ntt_ref_init(void);
uint16_t ntt_ref_fqmul(uint16_t a, uint16_t b);

void ntt_ref(uint16_t r[NTT_N]);                    /* forward, in place */
void invntt_tomont_ref(uint16_t r[NTT_N]);          /* inverse + to-Montgomery, in place */
void pointwise_ref(const uint16_t a[NTT_N], const uint16_t b[NTT_N], uint16_t c[NTT_N]);
void polymul_schoolbook(const uint16_t a[NTT_N], const uint16_t b[NTT_N], uint16_t c[NTT_N]);

/* the AVX2 assembly entry points and their constant tables (namespaced s512_ by
 * the generator).  Twiddles use the Seiler precompute: each butterfly consumes a
 * (zl,zh) pair, so the zeta tables hold 2x16-lane vectors per butterfly and ninv
 * is 32-int16 (zl[16] then zh[16]).  qdata is {q, 2^16-q, 0} (qinv folded into
 * zl).  cross_fwd = level0(2 pairs)+level1(2 pairs); cross_inv = level1+level0. */
extern const int16_t s512_ntt_qdata[48];
extern const int16_t s512_ntt_ninv[32];
extern const int16_t s512_ntt_qinv[16];
extern const int16_t s512_ntt_r2[32];
extern const int16_t s512_ntt_cross_fwd[8][16];
extern const int16_t s512_ntt_cross_inv[8][16];
extern const int16_t s512_ntt_zetas_fwd[224][16];
extern const int16_t s512_ntt_zetas_inv[224][16];
void s512_ntt_avx   (int16_t *poly, const int16_t *qdata, const int16_t (*ztab)[16], const int16_t (*cross)[16], const int16_t *ninv);
void s512_invntt_tomont_avx(int16_t *poly, const int16_t *qdata, const int16_t (*ztab)[16], const int16_t (*cross)[16], const int16_t *ninv);
void s512_pointwise_avx(int16_t *c, const int16_t *a, const int16_t *b, const int16_t *qdata);
/* nttunpack: standard-order int32 samples (in [0,q)) -> avx NTT slot layout
 * (Kyber poly_nttunpack analogue: forward shuffle network, no butterflies). */
void s512_nttunpack_avx(int16_t *dst, const int32_t *src);

/* Opt-in AVX-512BW full-ZMM path.  Built only by the AVX512=1 Makefile targets. */
extern const int16_t s512_ntt512_qdata[64];
extern const int16_t s512_ntt512_scale[64];
extern const int16_t s512_ntt512_qinv[32];
extern const int16_t s512_ntt512_r2[64];
extern const int16_t s512_ntt512_zetas_fwd[144][32];
extern const int16_t s512_ntt512_zetas_inv[144][32];
void s512_ntt_avx512      (int16_t *poly, const int16_t *qdata, const int16_t (*ztab)[32], const int16_t *scale);
void s512_invntt_tomont_avx512(int16_t *poly, const int16_t *qdata, const int16_t (*ztab)[32], const int16_t *scale);
void s512_pointwise_avx512(int16_t *c, const int16_t *a, const int16_t *b, const int16_t *qdata);
void s512_nttunpack_avx512(int16_t *dst, const int32_t *src);

#endif
