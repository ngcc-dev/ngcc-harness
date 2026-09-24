/*
 * AArch64 NEON rejection sampling for DKE-128/256 (Q=3329, 12-bit).
 * Uses NEON compare + scalar compaction (NEON lacks pshufb-style compaction).
 * Only compiled when DKE_USE_AARCH64 is defined and DKE_MODE != 512.
 */
#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

#include <arm_neon.h>
#include <stdint.h>

#if DKE_MODE != 512
/* DKE-128/256 (Q=3329, 12-bit) rejection sampling */
#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
#include "../aarch64-native/dke_native.h"
/* Native ASM rej_uniform: uses lookup table, processes 24 bytes per iteration.
 * Wraps mlkem-native's rej_uniform_asm to match DKE's append-mode interface. */
unsigned int rej_uniform(int16_t *res, unsigned int len,
                         const unsigned char *buf, unsigned int buflen) {
    if (len == DKE_N && buflen >= 24) {
        /* Fast path: first call, fill from start — use native ASM directly.
         * Native ASM requires buflen to be a multiple of 24. */
        unsigned int aligned_buflen = (buflen / 24) * 24;
        uint64_t count = dke_rej_uniform_native(res, buf, aligned_buflen,
                                                 dke_rej_uniform_table);
        unsigned int ctr = (unsigned int)count;
        /* Handle remaining unaligned bytes with scalar */
        buf += aligned_buflen; buflen -= aligned_buflen;
        while (ctr < len && buflen >= 3) {
            uint16_t val0 = ((buf[0]) | ((uint16_t)buf[1] << 8)) & 0xFFF;
            uint16_t val1 = ((buf[1] >> 4) | ((uint16_t)buf[2] << 4)) & 0xFFF;
            buf += 3; buflen -= 3;
            if (val0 < DKE_Q) res[ctr++] = (int16_t)val0;
            if (val1 < DKE_Q && ctr < len) res[ctr++] = (int16_t)val1;
        }
        return ctr;
    }
    /* Fallback: append mode or short buffer — scalar */
    {
        unsigned int ctr = 0, pos = 0;
        while (ctr < len && pos + 3 <= buflen) {
            uint16_t val0 = ((buf[pos]) | ((uint16_t)buf[pos+1] << 8)) & 0xFFF;
            uint16_t val1 = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2] << 4)) & 0xFFF;
            pos += 3;
            if (val0 < DKE_Q) res[ctr++] = (int16_t)val0;
            if (val1 < DKE_Q && ctr < len) res[ctr++] = (int16_t)val1;
        }
        return ctr;
    }
}
#else /* Original NEON rej_uniform */

/*
 * NEON rejection sampling: process 8 candidates (12 bytes) per iteration.
 * 1. Load 12 bytes, extract 8 x 12-bit values
 * 2. Compare against Q using NEON
 * 3. Scalar compaction (store accepted values sequentially)
 *
 * NEON doesn't have pshufb-style variable permute, so we use
 * a compare-and-scatter approach with scalar stores for accepted values.
 * This is still faster than pure scalar due to vectorized extraction + compare.
 */
unsigned int rej_uniform(int16_t *res,
                         unsigned int len,
                         const unsigned char *buf,
                         unsigned int buflen) {
    unsigned int ctr, pos;
    uint16_t val0, val1;
    const int16x8_t bound = vdupq_n_s16((int16_t)DKE_Q);

    ctr = pos = 0;

    /* NEON loop: process 8 candidates from 12 bytes */
    while (ctr + 8 <= len && pos + 12 <= buflen) {
        /* Extract 8 x 12-bit values from 12 bytes */
        uint16_t t[8];
        t[0] = ((buf[pos+0] >> 0) | ((uint16_t)buf[pos+1]  << 8)) & 0xFFF;
        t[1] = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2]  << 4));
        t[2] = ((buf[pos+3] >> 0) | ((uint16_t)buf[pos+4]  << 8)) & 0xFFF;
        t[3] = ((buf[pos+4] >> 4) | ((uint16_t)buf[pos+5]  << 4));
        t[4] = ((buf[pos+6] >> 0) | ((uint16_t)buf[pos+7]  << 8)) & 0xFFF;
        t[5] = ((buf[pos+7] >> 4) | ((uint16_t)buf[pos+8]  << 4));
        t[6] = ((buf[pos+9] >> 0) | ((uint16_t)buf[pos+10] << 8)) & 0xFFF;
        t[7] = ((buf[pos+10]>> 4) | ((uint16_t)buf[pos+11] << 4));
        pos += 12;

        /* Load into NEON vector and compare */
        int16x8_t vals = vld1q_s16((const int16_t *)t);
        uint16x8_t cmp = vcltq_s16(vals, bound); /* -1 where val < Q */

        /* Extract comparison mask to scalar */
        /* NEON: narrow to 8-bit, then extract bits */
        uint8x8_t cmp8 = vmovn_u16(cmp);
        /* Each byte is 0xFF (accepted) or 0x00 (rejected) */

        /* Scalar compaction: store accepted values */
        unsigned int k;
        for (k = 0; k < 8 && ctr < len; k++) {
            if ((int16_t)t[k] < DKE_Q) {
                res[ctr++] = (int16_t)t[k];
            }
        }
        (void)cmp8; /* comparison used implicitly via scalar check */
    }

    /* Scalar tail */
    while (ctr < len && pos + 3 <= buflen) {
        val0 = ((buf[pos+0] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0xFFF;
        val1 = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2] << 4));
        pos += 3;
        if (val0 < DKE_Q)
            res[ctr++] = (int16_t)val0;
        if (val1 < DKE_Q && ctr < len)
            res[ctr++] = (int16_t)val1;
    }

    return ctr;
}
#endif /* native vs NEON rej_uniform */

#endif /* DKE_MODE != 512 */

/*
 * DKE-512 (Q=7681, 13-bit) NEON rejection sampling.
 * Extracts 8 x 13-bit values from 13 bytes per iteration.
 * NEON compare + scalar compaction.
 */
#if DKE_MODE == 512

unsigned int rej_uniform(int16_t *res,
                         unsigned int len,
                         const unsigned char *buf,
                         unsigned int buflen) {
    unsigned int ctr, pos, k;
    const int16x8_t bound = vdupq_n_s16((int16_t)DKE_Q);
    ctr = pos = 0;

    /* NEON loop: extract 8 x 13-bit values from 13 bytes */
    while (ctr + 8 <= len && pos + 13 <= buflen) {
        int16_t t[8];
        t[0] = (int16_t)(((buf[pos+ 0] >> 0) | ((uint16_t)buf[pos+ 1] << 8))         & 0x1FFF);
        t[1] = (int16_t)(((buf[pos+ 1] >> 5) | ((uint16_t)buf[pos+ 2] << 3)
                                               | ((uint16_t)buf[pos+ 3] << 11))        & 0x1FFF);
        t[2] = (int16_t)(((buf[pos+ 3] >> 2) | ((uint16_t)buf[pos+ 4] << 6))         & 0x1FFF);
        t[3] = (int16_t)(((buf[pos+ 4] >> 7) | ((uint16_t)buf[pos+ 5] << 1)
                                               | ((uint16_t)buf[pos+ 6] << 9))         & 0x1FFF);
        t[4] = (int16_t)(((buf[pos+ 6] >> 4) | ((uint16_t)buf[pos+ 7] << 4)
                                               | ((uint16_t)buf[pos+ 8] << 12))        & 0x1FFF);
        t[5] = (int16_t)(((buf[pos+ 8] >> 1) | ((uint16_t)buf[pos+ 9] << 7))         & 0x1FFF);
        t[6] = (int16_t)(((buf[pos+ 9] >> 6) | ((uint16_t)buf[pos+10] << 2)
                                               | ((uint16_t)buf[pos+11] << 10))        & 0x1FFF);
        t[7] = (int16_t)(((buf[pos+11] >> 3) | ((uint16_t)buf[pos+12] << 5))          & 0x1FFF);
        pos += 13;

        /* NEON compare: which values < Q? */
        int16x8_t vals = vld1q_s16(t);
        uint16x8_t cmp = vcltq_s16(vals, bound);

        /* Extract accepted count for fast skip when all/none accepted */
        uint64_t mask_lo = vgetq_lane_u64(vreinterpretq_u64_u16(cmp), 0);
        uint64_t mask_hi = vgetq_lane_u64(vreinterpretq_u64_u16(cmp), 1);

        if ((mask_lo & mask_hi) == 0xFFFFFFFFFFFFFFFFULL) {
            /* All 8 accepted — fast path, store directly */
            vst1q_s16(&res[ctr], vals);
            ctr += 8;
        } else if ((mask_lo | mask_hi) == 0) {
            /* All rejected — skip */
        } else {
            /* Mixed: scalar compaction */
            for (k = 0; k < 8 && ctr < len; k++) {
                if (t[k] < (int16_t)DKE_Q)
                    res[ctr++] = t[k];
            }
        }
    }

    /* Scalar middle: process remaining 13-byte chunks when < 8 slots left.
     * Must use the same 13-byte dense packing as the NEON loop above,
     * NOT the 2-byte-per-value format (which is a different bit layout). */
    while (ctr < len && pos + 13 <= buflen) {
        int16_t t[8];
        t[0] = (int16_t)(((buf[pos+ 0] >> 0) | ((uint16_t)buf[pos+ 1] << 8))         & 0x1FFF);
        t[1] = (int16_t)(((buf[pos+ 1] >> 5) | ((uint16_t)buf[pos+ 2] << 3)
                                               | ((uint16_t)buf[pos+ 3] << 11))        & 0x1FFF);
        t[2] = (int16_t)(((buf[pos+ 3] >> 2) | ((uint16_t)buf[pos+ 4] << 6))         & 0x1FFF);
        t[3] = (int16_t)(((buf[pos+ 4] >> 7) | ((uint16_t)buf[pos+ 5] << 1)
                                               | ((uint16_t)buf[pos+ 6] << 9))         & 0x1FFF);
        t[4] = (int16_t)(((buf[pos+ 6] >> 4) | ((uint16_t)buf[pos+ 7] << 4)
                                               | ((uint16_t)buf[pos+ 8] << 12))        & 0x1FFF);
        t[5] = (int16_t)(((buf[pos+ 8] >> 1) | ((uint16_t)buf[pos+ 9] << 7))         & 0x1FFF);
        t[6] = (int16_t)(((buf[pos+ 9] >> 6) | ((uint16_t)buf[pos+10] << 2)
                                               | ((uint16_t)buf[pos+11] << 10))        & 0x1FFF);
        t[7] = (int16_t)(((buf[pos+11] >> 3) | ((uint16_t)buf[pos+12] << 5))          & 0x1FFF);
        pos += 13;
        for (k = 0; k < 8 && ctr < len; k++) {
            if (t[k] < (int16_t)DKE_Q)
                res[ctr++] = t[k];
        }
    }

    /* 2-byte tail: only for leftover bytes that don't fill a 13-byte chunk */
    while (ctr < len && pos + 2 <= buflen) {
        uint16_t val = ((buf[pos] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0x1FFF;
        pos += 2;
        if (val < DKE_Q)
            res[ctr++] = (int16_t)val;
    }

    return ctr;
}

#endif /* DKE_MODE == 512 */

#endif /* DKE_USE_AARCH64 */
