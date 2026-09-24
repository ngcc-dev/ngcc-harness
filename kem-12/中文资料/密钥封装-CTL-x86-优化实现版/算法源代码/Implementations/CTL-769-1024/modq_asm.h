/*
 * CTL AVX2+Assembly Deep Optimization Header
 *
 * Note: This header is now a compatibility wrapper.
 * The actual AVX2 optimizations are implemented in ctl_avx2_modq.h
 * using Intel intrinsics for better portability.
 *
 * This file remains for backward compatibility and contains
 * declarations that were previously implemented in assembly.
 */

#ifndef CTL_MODQ_ASM_H__
#define CTL_MODQ_ASM_H__

#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>  /* Required for __m256i type */

/* Define UNUSED macro if not already defined */
#if defined(__GNUC__) || defined(__clang__)
#define CTL_UNUSED __attribute__((unused))
#else
#define CTL_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compatibility declarations - these are now implemented in ctl_avx2_modq.h
 * using Intel intrinsics instead of raw assembly.
 */

/* Dummy function to satisfy linker */
static inline void mq_asm_dummy(void) {}

/*
 * Initialize function - now handled by mq_avx2_init() in ctl_avx2_modq.h
 */
static inline void mq_asm_init(uint32_t q_val CTL_UNUSED, uint32_t q1i_val CTL_UNUSED) {
    /* Initialization is now done in ctl_avx2_modq.h via mq_avx2_init() */
}

#ifdef __cplusplus
}
#endif

#endif /* CTL_MODQ_ASM_H__ */
