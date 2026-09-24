#ifndef EIJEN_INTERNAL_H
#define EIJEN_INTERNAL_H

#include "eijen.h"
#include <string.h>

#define EIJEN_ROUNDS 16

static const eijen_config EIJEN_CONFIGS[5] = {
    {256, 27, 5, 4, 216, 32},
    {384, 25, 7, 6, 200, 48},
    {512, 23, 9, 8, 184, 64},
    {768, 19, 13, 12, 152, 96},
    {1024, 15, 17, 16, 120, 128}
};

static const uint64_t EIJEN_RC0[16] = {
    UINT64_C(0x243F6A8885A308D3), UINT64_C(0xA4093822299F31D0),
    UINT64_C(0x452821E638D01377), UINT64_C(0xC0AC29B7C97C50DD),
    UINT64_C(0x9216D5D98979FB1B), UINT64_C(0x2FFD72DBD01ADFB7),
    UINT64_C(0xBA7C9045F12C7F99), UINT64_C(0x0801F2E2858EFC16),
    UINT64_C(0xA458FEA3F4933D7E), UINT64_C(0x718BCD5882154AEE),
    UINT64_C(0x9C30D5392AF26013), UINT64_C(0xCA417918B8DB38EF),
    UINT64_C(0x6C9E0E8BB01E8A3E), UINT64_C(0x78AF2FDA55605C60),
    UINT64_C(0x5748986263E81440), UINT64_C(0xB4CC5C341141E8CE)
};

static const uint64_t EIJEN_RC1[16] = {
    UINT64_C(0x13198A2E03707344), UINT64_C(0x082EFA98EC4E6C89),
    UINT64_C(0xBE5466CF34E90C6C), UINT64_C(0x3F84D5B5B5470917),
    UINT64_C(0xD1310BA698DFB5AC), UINT64_C(0xB8E1AFED6A267E96),
    UINT64_C(0x24A19947B3916CF7), UINT64_C(0x636920D871574E69),
    UINT64_C(0x0D95748F728EB658), UINT64_C(0x7B54A41DC25A59B5),
    UINT64_C(0xC5D1B023286085F0), UINT64_C(0x8E79DCB0603A180E),
    UINT64_C(0xD71577C1BD314B27), UINT64_C(0xE65525F3AA55AB94),
    UINT64_C(0x55CA396A2AAB10B6), UINT64_C(0xA15486AF7C72E993)
};

static inline uint64_t eijen_load64_le(const uint8_t *src) {
    return ((uint64_t)src[0]) |
           ((uint64_t)src[1] << 8) |
           ((uint64_t)src[2] << 16) |
           ((uint64_t)src[3] << 24) |
           ((uint64_t)src[4] << 32) |
           ((uint64_t)src[5] << 40) |
           ((uint64_t)src[6] << 48) |
           ((uint64_t)src[7] << 56);
}

static inline void eijen_store64_le(uint8_t *dst, uint64_t v) {
    dst[0] = (uint8_t)(v & 0xffU);
    dst[1] = (uint8_t)((v >> 8) & 0xffU);
    dst[2] = (uint8_t)((v >> 16) & 0xffU);
    dst[3] = (uint8_t)((v >> 24) & 0xffU);
    dst[4] = (uint8_t)((v >> 32) & 0xffU);
    dst[5] = (uint8_t)((v >> 40) & 0xffU);
    dst[6] = (uint8_t)((v >> 48) & 0xffU);
    dst[7] = (uint8_t)((v >> 56) & 0xffU);
}

static inline uint64_t eijen_rotl64(uint64_t x, unsigned int n) {
    return (x << n) | (x >> (64U - n));
}

static inline const eijen_config *eijen_config_by_bits(int digest_bits) {
    size_t i;
    for (i = 0; i < sizeof(EIJEN_CONFIGS) / sizeof(EIJEN_CONFIGS[0]); i++) {
        if (EIJEN_CONFIGS[i].digest_bits == digest_bits) {
            return &EIJEN_CONFIGS[i];
        }
    }
    return NULL;
}

#endif
