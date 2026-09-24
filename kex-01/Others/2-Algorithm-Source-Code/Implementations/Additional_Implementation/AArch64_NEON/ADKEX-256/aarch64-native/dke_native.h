/*
 * DKE AArch64 native assembly declarations.
 * Based on mlkem-native (Apache-2.0 OR ISC OR MIT).
 *
 * Provides hand-written AArch64 assembly for NTT, INVNTT, basemul, etc.
 * Only active when DKE_USE_AARCH64_NATIVE is defined.
 *
 * Currently supports DKE-128/256 (N=256, Q=3329) — same ring as ML-KEM.
 * DKE-512 (N=512, Q=7681) requires separate implementation.
 */
#ifndef DKE_AARCH64_NATIVE_H
#define DKE_AARCH64_NATIVE_H

#include "../parameters.h"

#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329

#include <stdint.h>

/* ---- Zeta tables (pre-expanded for NEON register layout) ---- */
extern const int16_t dke_aarch64_ntt_zetas_layer12345[];
extern const int16_t dke_aarch64_ntt_zetas_layer67[];
extern const int16_t dke_aarch64_invntt_zetas_layer12345[];
extern const int16_t dke_aarch64_invntt_zetas_layer67[];
extern const int16_t dke_aarch64_zetas_mulcache_native[];
extern const int16_t dke_aarch64_zetas_mulcache_twisted_native[];

/* ---- Assembly function declarations ---- */

/* NTT: in-place forward NTT on 256 coefficients.
 * Input bounds: |p[i]| < 8192.  Output bounds: |p[i]| < 23595. */
void dke_ntt_native(int16_t p[256],
                    const int16_t twiddles12345[80],
                    const int16_t twiddles56[384]);

/* INVNTT: in-place inverse NTT on 256 coefficients.
 * Input bounds: |p[i]| < 8192.  Output bounds: |p[i]| < 26625. */
void dke_intt_native(int16_t p[256],
                     const int16_t twiddles12345[80],
                     const int16_t twiddles56[384]);

/* Barrett reduction: reduce all coefficients to [0, Q). */
void dke_poly_reduce_native(int16_t p[256]);

/* Montgomery conversion: multiply all coefficients by R mod Q. */
void dke_poly_tomont_native(int16_t p[256]);

/* Serialize polynomial to bytes (12-bit packing). */
void dke_poly_tobytes_native(uint8_t r[384], const int16_t a[256]);

/* Mulcache: precompute b[2i+1] * zeta for basemul. */
void dke_poly_mulcache_compute_native(int16_t cache[128],
                                       const int16_t poly[256],
                                       const int16_t zetas[128],
                                       const int16_t zetas_twisted[128]);

/* Basemul K=2: r = sum(a[i] * b[i]) for i=0..1, using cached b. */
void dke_polyvec_basemul_acc_cached_k2(int16_t r[256],
                                        const int16_t a[512],
                                        const int16_t b[512],
                                        const int16_t b_cache[256]);

/* Basemul K=4: r = sum(a[i] * b[i]) for i=0..3, using cached b. */
void dke_polyvec_basemul_acc_cached_k4(int16_t r[256],
                                        const int16_t a[1024],
                                        const int16_t b[1024],
                                        const int16_t b_cache[512]);

/* Rejection sampling: extract uniform coefficients < Q from byte stream.
 * Uses lookup table for compact output. Returns number of valid coefficients. */
extern const uint8_t dke_rej_uniform_table[];
uint64_t dke_rej_uniform_native(int16_t r[256], const uint8_t *buf,
                                 unsigned buflen, const uint8_t table[2048]);

/* ---- Convenience wrappers ---- */

static inline void dke_ntt_asm(int16_t p[256]) {
    dke_ntt_native(p, dke_aarch64_ntt_zetas_layer12345,
                      dke_aarch64_ntt_zetas_layer67);
}

static inline void dke_intt_asm(int16_t p[256]) {
    dke_intt_native(p, dke_aarch64_invntt_zetas_layer12345,
                       dke_aarch64_invntt_zetas_layer67);
}

#endif /* DKE_USE_AARCH64_NATIVE && N==256 && Q==3329 */

/* ---- DKE-512 (N=512, Q=7681) native assembly ---- */
#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 512 && DKE_Q == 7681

/* Compiler-extracted NTT/INVNTT from Clang -O2 NEON intrinsics.
 * Same code as compiled ntt_neon.c but frozen as assembly for reproducibility.
 * Uses DKE_zetas[] and DKE_montgomery_reduce() directly. */
void dke_ntt512_compiler(int16_t r[512]);
void dke_intt512_compiler(int16_t r[512]);

#endif /* DKE_USE_AARCH64_NATIVE && N==512 && Q==7681 */

#endif /* DKE_AARCH64_NATIVE_H */
