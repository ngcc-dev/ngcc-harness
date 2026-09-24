/*
 * DKE Cortex-M4 Plantard backend.
 * Huang et al., "Improved Plantard Arithmetic for Efficient Implementations
 * of Lattice-based Cryptography", TCHES 2022.
 * ASM from ICCS Additional_Implementation, Apache 2.0 license.
 *
 * Compile only with -DDKE_USE_CORTEX_M4_PLANTARD.
 */
#ifdef DKE_USE_CORTEX_M4_PLANTARD  /* bench_config.h 中开启后才编译 */
#include "../parameters.h"
#include "../ntt.h"
#include "../poly.h"
#include "../polyvec.h"

/* ── Plantard ASM function declarations (internal to this backend) ── */
extern void ntt_fast(int16_t *poly, const int32_t *twiddles);
extern void invntt_fast(int16_t *poly, const int32_t *twiddles);
extern void basemul_asm(int16_t *r, const int16_t *a, const int16_t *b,
                         const int32_t *zetas);
extern void basemul_asm_acc(int16_t *r, const int16_t *a, const int16_t *b,
                              const int32_t *zetas);
extern void pointwise_add(int16_t *r, const int16_t *a, const int16_t *b);
extern void pointwise_sub(int16_t *r, const int16_t *a, const int16_t *b);
extern void poly_reduce_asm(int16_t *r);
extern void asm_fromplant(int16_t *r);

#if DKE_N == 256
extern void basemul_asm_opt_16_32(int32_t *, const int16_t *, const int16_t *,
                                   const int16_t *);
extern void basemul_asm_acc_opt_32_32(int32_t *, const int16_t *, const int16_t *,
                                       const int16_t *);
extern void basemul_asm_acc_opt_32_16(int16_t *, const int16_t *, const int16_t *,
                                       const int16_t *, const int32_t *);
extern void poly_tobytes_asm(uint8_t *bytes, const int16_t *coeffs);
#endif

/* ── Plantard zeta tables (from plantard_zetas.c) ─────────────────── */
extern const int32_t zetas[];
extern const int32_t zetas_asm[];
#if DKE_N == 256
extern const int32_t zetas_inv_CT_asm[];   /* Q=3329 inverse NTT twiddles */
#elif DKE_N == 512
extern const int32_t zetas_inv_asm[];      /* Q=7681 inverse NTT twiddles */
#endif

/* ================================================================
 * DKE_* interface — delegates to Plantard ASM
 * ================================================================ */

void DKE_ntt(int16_t r[DKE_N]) {
    ntt_fast(r, zetas_asm);
}

void DKE_invntt(int16_t r[DKE_N]) {
#if DKE_N == 256
    invntt_fast(r, zetas_inv_CT_asm);
#else
    invntt_fast(r, zetas_inv_asm);
#endif
}

/* Single-pair basemul: Plantard bulk path (basemul_asm) is used in
 * DKE_poly_basemul_montgomery; this per-pair stub is a no-op placeholder. */
void DKE_basemul(int16_t r[2], const int16_t a[2],
                 const int16_t b[2], int16_t zeta) {
    (void)r; (void)a; (void)b; (void)zeta;
}

void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    basemul_asm(res->coeffs, a->coeffs, b->coeffs, zetas);
}

void DKE_poly_add(poly *r, const poly *a, const poly *b) {
    pointwise_add(r->coeffs, a->coeffs, b->coeffs);
}

void DKE_poly_sub(poly *r, const poly *a, const poly *b) {
    pointwise_sub(r->coeffs, a->coeffs, b->coeffs);
}

void DKE_poly_reduce(poly *pol) {
    poly_reduce_asm(pol->coeffs);
}

/* In Plantard mode poly_tomont converts FROM Plantard domain to standard form.
 * (Montgomery mode: tomont multiplies by R^2 mod Q to enter Montgomery domain.) */
void DKE_poly_tomont(poly *pol) {
    asm_fromplant(pol->coeffs);
}

void DKE_polyvec_basemul_acc_montgomery_m4(poly *res,
                                            const polyvec *a, const polyvec *b) {
    unsigned int i;
    basemul_asm(res->coeffs, a->vec[0].coeffs, b->vec[0].coeffs, zetas);
    for (i = 1; i < DKE_K; i++)
        basemul_asm_acc(res->coeffs, a->vec[i].coeffs, b->vec[i].coeffs, zetas);
}

#endif /* DKE_USE_CORTEX_M4_PLANTARD */
