#ifndef LORE_NEON_CORE_H
#define LORE_NEON_CORE_H

#ifdef LORE_USE_NEON_CORE
#include <arm_neon.h>

static inline int16x8_t lore_barrett_reduce_s16x8(int16x8_t a)
{
    const uint16x8_t mask = vdupq_n_u16(0x00ff);
    int16x8_t lo = vreinterpretq_s16_u16(vandq_u16(vreinterpretq_u16_s16(a), mask));
    int16x8_t t = vsubq_s16(lo, vshrq_n_s16(a, 8));

    lo = vreinterpretq_s16_u16(vandq_u16(vreinterpretq_u16_s16(t), mask));
    t = vsubq_s16(lo, vshrq_n_s16(t, 8));
    return t;
}

#endif

#endif
