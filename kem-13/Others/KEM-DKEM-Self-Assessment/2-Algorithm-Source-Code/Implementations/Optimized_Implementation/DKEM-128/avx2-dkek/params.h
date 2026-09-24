/* Minimal params for the vendored ntt256 AVX2 NTT asm, namespaced as dke_k_*.
 * Used only by avx2-dkek/{ntt,invntt,basemul,shuffle,fq}.S + consts.c.
 * N=256, Q=3329 — the DKE-128/256 ring (identical to ntt256).
 * All macros guarded so a stray -DDKEK_K=... from a shared build rule is harmless. */
#ifndef PARAMS_H
#define PARAMS_H

#undef DKEK_NAMESPACE
#define DKEK_NAMESPACE(s) dke_k_##s

#ifndef DKEK_N
#define DKEK_N 256
#endif
#ifndef DKEK_Q
#define DKEK_Q 3329
#endif
#ifndef DKEK_K
#define DKEK_K 2
#endif

#endif
