/*
 * Auto-selecting AVX-512VL/VKS Cuishen-1024 implementation.
 *
 * Build this file for the final AVX-512 C path. With -march=native or
 * -mtune=native it chooses the AMD-tuned implementation on Zen targets and the
 * Intel-tuned implementation otherwise. For explicit A/B builds, define
 * exactly one of:
 *   CUISHEN_AVX512_SELECT_AMD
 *   CUISHEN_AVX512_SELECT_INTEL
 */

#if defined(CUISHEN_AVX512_SELECT_AMD) &&                                \
    defined(CUISHEN_AVX512_SELECT_INTEL)
#error "Define at most one of CUISHEN_AVX512_SELECT_AMD or CUISHEN_AVX512_SELECT_INTEL."
#endif

#if defined(CUISHEN_AVX512_SELECT_AMD)
#include "CryptHash_Cuishen-1024_avx512_amd.c"
#elif defined(CUISHEN_AVX512_SELECT_INTEL)
#include "CryptHash_Cuishen-1024_avx512_intel.c"
#elif defined(__znver1) || defined(__znver1__) ||                           \
    defined(__znver2) || defined(__znver2__) || defined(__znver3) ||         \
    defined(__znver3__) || defined(__znver4) || defined(__znver4__) ||       \
    defined(__znver5) || defined(__znver5__) || defined(__tune_znver1__) ||  \
    defined(__tune_znver2__) || defined(__tune_znver3__) ||                  \
    defined(__tune_znver4__) || defined(__tune_znver5__)
#include "CryptHash_Cuishen-1024_avx512_amd.c"
#else
#include "CryptHash_Cuishen-1024_avx512_intel.c"
#endif
