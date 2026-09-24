// Unified DKE parameters header.
// Define DKE_MODE as 128, 256, or 512 before including this file.

#ifndef DKE_PARAMETERS_H
#define DKE_PARAMETERS_H
#include <stdint.h>
#include <stdio.h>


/* SIMD backend selection.
 *
 * x86-64 (AVX2) is auto-detected by default and uses the verified natural-order
 * AVX2 intrinsic NTT chain (see DKE_AVX2_NTT_INTRINSIC below), which reproduces
 * the reference KATs byte-for-byte for all three parameter sets.
 *
 * The AArch64 (NEON) backend is NOT yet validated for the v20260602 design, so
 * it is opt-in (define DKE_USE_AARCH64 or DKE_ENABLE_SIMD explicitly).
 *
 * Define DKE_FORCE_SCALAR to force the pure-C path. */
#if !defined(DKE_FORCE_SCALAR)
#if !defined(DKE_USE_AVX2) && !defined(DKE_USE_AARCH64)
  #if defined(__aarch64__) || defined(_M_ARM64)
    #define DKE_USE_AARCH64
  #elif defined(__x86_64__) || defined(_M_X64) || defined(_MSC_VER)
    #define DKE_USE_AVX2
  #endif
#endif
#endif

#if defined(DKE_FORCE_SCALAR)
  #undef DKE_USE_AVX2
  #undef DKE_USE_AARCH64
#endif

/* The hand-ported ML-KEM-packed AVX2 NTT-chain assembly is not byte-canonical
 * for this design (and the MASM invntt faults). Use the verified natural-order
 * intrinsic NTT chain + scalar order-sensitive ops by default when AVX2 is on.
 * Define DKE_AVX2_USE_ASM to opt back into the packed asm (once it is fixed). */
#if defined(DKE_USE_AVX2) && !defined(DKE_AVX2_USE_ASM) && !defined(DKE_AVX2_NTT_INTRINSIC)
#define DKE_AVX2_NTT_INTRINSIC
#endif

#ifndef DKE_USE_OPT_C
#define DKE_USE_OPT_C
#endif
/* Note: DKE_USE_OPT_C2 / DKE_USE_OPT_C3 (H(pk) caching) are obsolete under the
 * v20260602 FO design: encapsulation derives randomness from the rho seed embedded
 * in the trailing bytes of pk, and the secret key stores (skCPA | pk | z) with no
 * cached H(pk). The macros are intentionally left undefined. */

#ifndef DKE_HASH
#define DKE_HASH 0    /* 0=SM3 (DKEM reference / KAT), 2=SHAKE128+SHA3-256+SHA3-512+SHAKE256 (ML-KEM suite) */
#endif

/* HASH=1 (SHAKE128 for all operations) has been removed; use 0 (SM3) or 2 (ML-KEM suite). */
#if DKE_HASH == 1
#error "DKE_HASH=1 (SHAKE128-all) was removed; use DKE_HASH=0 (SM3) or DKE_HASH=2 (ML-KEM suite)"
#endif

/* DKE_HASH_SHAKE: true when the SHAKE/SHA3 (ML-KEM-suite) backend is selected. */
#if DKE_HASH == 2
#define DKE_HASH_SHAKE 1
#endif

#ifndef DKE_RANDOM
#define DKE_RANDOM 0  /* 0=DRNG (deterministic, DKEM reference / KAT), 1=system random (ML-KEM-style) */
#endif

#ifndef DKE_MODE
#define DKE_MODE 128
#endif

#if DKE_MODE == 128

#define DKE_N      256
#define DKE_Q      3329
#define DKE_K      2
#define DKE_DA     12
#define DKE_DB     10
#define DKE_L      4
#define DKE_NOISE_A      3
#define DKE_NOISE_B      3
#define DKE_SEEDBYTES    32
#define DKE_SSBYTES      32
#define DKE_POLYBYTES    (12 * DKE_N / 8)
#define DKE_MONT         (-1044)
#define DKE_QINV         (-3327)
#define DKE_NTT_ZETAS_LEN 128
#define DKE_INVNTT_F     1441
#define ALGORITHM_INSTANCE "DKEM-128"

#elif DKE_MODE == 256

#define DKE_N      256
#define DKE_Q      3329
#define DKE_K      4
#define DKE_DA     12
#define DKE_DB     11
#define DKE_L      5
#define DKE_NOISE_A      3
#define DKE_NOISE_B      3
#define DKE_SEEDBYTES    32
#define DKE_SSBYTES      32
#define DKE_POLYBYTES    (12 * DKE_N / 8)
#define DKE_MONT         (-1044)
#define DKE_QINV         (-3327)
#define DKE_NTT_ZETAS_LEN 128
#define DKE_INVNTT_F     1441
#define ALGORITHM_INSTANCE "DKEM-256"

#elif DKE_MODE == 512

#define DKE_N      512
#define DKE_Q      7681
#define DKE_K      4
#define DKE_DA     13
#define DKE_DB     11
#define DKE_L      4
#define DKE_NOISE_A      3
#define DKE_NOISE_B      3
#define DKE_SEEDBYTES    64
#define DKE_SSBYTES      64
#define DKE_POLYBYTES    (13 * DKE_N / 8)
#define DKE_MONT         (-3593)
#define DKE_QINV         (-7679)
#define DKE_NTT_ZETAS_LEN 256
#define DKE_INVNTT_F     1912
#define ALGORITHM_INSTANCE "DKEM-512"

#else
#error "DKE_MODE must be 128, 256, or 512"
#endif


#if defined(__GNUC__)
#define ALIGN(x) __attribute__ ((aligned(x)))
#elif defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x))
#elif defined(__ARMCC_VERSION)
#define ALIGN(x) __align(x)
#else
#define ALIGN(x)
#endif

// Derived sizes (common formulas)
#define DKE_POLYVECBYTES         (DKE_K * DKE_POLYBYTES)
#define DKE_PACOMPRESSEDBYTES    ((DKE_K * DKE_DA * DKE_N) / 8)
#define DKE_PBCOMPRESSEDBYTES    ((DKE_K * DKE_DB * DKE_N) / 8)
#define DKE_SIGNALBYTES          (DKE_L * DKE_N / 8)

#define DKE_PKBYTES      (DKE_PACOMPRESSEDBYTES + DKE_SEEDBYTES)
#define DKE_CPA_CTBYTES  (DKE_PBCOMPRESSEDBYTES + DKE_SIGNALBYTES)
#define DKE_CTBYTES      (DKE_CPA_CTBYTES + DKE_SSBYTES)

#define DKE_CPA_SKABYTES ((DKE_K * DKE_N * DKE_DA) / 8)
#define DKE_CPA_SKBBYTES ((DKE_K * DKE_N * DKE_DA) / 8)

/* Secret-key layout (v20260602 DKEM): CPA_SK | PK | z
 * where z is the SSBYTES rejection secret used for implicit rejection.
 * No cached H(pk): encapsulation re-derives the matrix seed (rho) from the
 * trailing SEEDBYTES of the embedded pk. */
#define DKE_SKBYTES      (DKE_CPA_SKABYTES + DKE_PKBYTES + DKE_SSBYTES)

// CBD parameter
#if DKE_MODE == 256
#define DKE_CBD_ETA   2
#define DKE_CBD_BYTES (2 * DKE_N / 4)
#else
#define DKE_CBD_ETA   3
#define DKE_CBD_BYTES (3 * DKE_N / 4)
#endif

// XOF parameters for gen_matrix
#define DKE_XOF_BLOCKBYTES 168
#define DKE_GEN_MATRIX_NBLOCKS ((DKE_DA * DKE_N / 8 * (1 << DKE_DA) / DKE_Q + DKE_XOF_BLOCKBYTES) / DKE_XOF_BLOCKBYTES)

#endif // DKE_PARAMETERS_H
