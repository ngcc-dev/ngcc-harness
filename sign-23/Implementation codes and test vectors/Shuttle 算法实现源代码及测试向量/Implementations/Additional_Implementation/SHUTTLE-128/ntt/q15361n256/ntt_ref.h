#ifndef S256_NTT_REF_H
#define S256_NTT_REF_H

#include <stdint.h>

/* SHUTTLE namespacing: every public scalar NTT symbol carries the
 * s256_ prefix so the three vendored configs co-link in one process.  This
 * mirrors the s256_ prefix the AVX2/AVX512 generators thread through their
 * .S/consts. The internal macro names below (NTT_Q/NTT_N/NTT_LEVELS) are
 * also prefixed (S256_*) to avoid the cross-config #define collision when
 * more than one config's header is pulled into a TU; the un-prefixed
 * aliases are kept for the body of ntt_ref.c only. */
#define ntt_ref_R2 s256_ntt_ref_R2
#define ntt_ref_init s256_ntt_ref_init
#define ntt_ref_fqmul s256_ntt_ref_fqmul
#define ntt_ref s256_ntt_ref
#define invntt_tomont_ref s256_invntt_tomont_ref
#define pointwise_ref s256_pointwise_ref
#define polymul_schoolbook s256_polymul_schoolbook

#define NTT_Q 15361
#define NTT_N 256
#define NTT_LEVELS 8 /* COMPLETE NTT: len 128..1, then pointwise */

extern uint16_t ntt_ref_R2; /* R^2 mod q */

void ntt_ref_init(void);
uint16_t ntt_ref_fqmul(uint16_t a, uint16_t b);

void ntt_ref(uint16_t r[256]); /* forward, in place */
void invntt_tomont_ref(
    uint16_t r[256]); /* inverse + to-Montgomery, in place */
void pointwise_ref(const uint16_t a[256], const uint16_t b[256],
                   uint16_t c[256]);
void polymul_schoolbook(const uint16_t a[256], const uint16_t b[256],
                        uint16_t c[256]);

/* AVX2 assembly entry points + constant tables (namespaced s256_ by the
 * generator).  SIGNED 16-bit representation with LAZY reduction (red16
 * schedule); q < 2^15.  qdata is {q, V, RND} (V/RND = centered Barrett
 * red16 constants).  Twiddles use Seiler precompute: each butterfly
 * consumes a (zl,zh) pair (zl=zeta*qinv mod 2^16, zh=zeta CENTERED), so
 * the zeta tables hold 2x16-lane vectors per butterfly; z0/z0inv/scale are
 * 32-int16 (zl[16] then zh[16]).  The inverse scale is n^-1*R^2
 * (256^-1*R^2), so invntt_tomont maps a bare roundtrip to a*R.
 * Coefficients are read back SIGNED, since q<2^15 makes them true signed
 * int16. */
extern const int16_t s256_ntt_qdata[48];
extern const int16_t s256_ntt_qinv[16];
extern const int16_t s256_ntt_z0[32], s256_ntt_z0inv[32],
    s256_ntt_scale[32];
extern const int16_t s256_ntt_zetas_fwd[112][16];
extern const int16_t s256_ntt_zetas_inv[112][16];
void s256_ntt_avx(int16_t *poly, const int16_t *qdata,
                  const int16_t (*ztab)[16], const int16_t *z0,
                  const int16_t *scale);
void s256_invntt_tomont_avx(int16_t *poly, const int16_t *qdata,
                            const int16_t (*ztab)[16],
                            const int16_t *z0inv, const int16_t *scale);
void s256_pointwise_avx(int16_t *c, const int16_t *a, const int16_t *b,
                        const int16_t *qdata);
void s256_reduce_avx(int16_t *poly);
/* nttunpack: standard-order int32 samples (in [0,q)) -> avx NTT slot
 * layout (Kyber poly_nttunpack analogue: forward shuffle network, no
 * butterflies). */
void s256_nttunpack_avx(int16_t *dst, const int32_t *src);

/* Opt-in AVX-512BW full-ZMM path.  Built only by the AVX512=1 Makefile
 * targets.  Vendoring fix: the upstream header declared
 * ntt512_qdata[64] while ntt_consts_avx512.c DEFINES [96] (signed needs
 * q/V/RND = 3x32); the mismatch is technically UB.  Declared correctly
 * here as [96]; the gen_ntt.py emitter (avx512_codegen emit_consts_signed)
 * already emits [96], so this survives regeneration. */
extern const int16_t s256_ntt512_qdata[96];
extern const int16_t s256_ntt512_scale[64];
extern const int16_t s256_ntt512_qinv[32];
extern const int16_t s256_ntt512_zetas_fwd[64][32];
extern const int16_t s256_ntt512_zetas_inv[64][32];
void s256_ntt_avx512(int16_t *poly, const int16_t *qdata,
                     const int16_t (*ztab)[32], const int16_t *scale);
void s256_invntt_tomont_avx512(int16_t *poly, const int16_t *qdata,
                               const int16_t (*ztab)[32],
                               const int16_t *scale);
void s256_pointwise_avx512(int16_t *c, const int16_t *a, const int16_t *b,
                           const int16_t *qdata);
void s256_nttunpack_avx512(int16_t *dst, const int32_t *src);

#endif
