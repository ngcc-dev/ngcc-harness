/* SPDX-License-Identifier: MIT */
#ifndef UBLOCKITH_PRG_H
#define UBLOCKITH_PRG_H

#include "params.h"

#include <stdint.h>
#include <stdlib.h>

void prg(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
         unsigned int seclvl, size_t outlen);
void prg_2_lambda_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                    uint32_t tweak_base, uint8_t* out, size_t out_stride,
                                    unsigned int seclvl, size_t count);
void prg_2_lambda_fixed_tweak_independent_batch_with_sd(const uint8_t* keys, size_t key_stride,
                                                        const uint8_t* iv, uint32_t tweak,
                                                        uint8_t* sd_out, size_t sd_stride,
                                                        uint8_t* out, size_t out_stride,
                                                        unsigned int seclvl, size_t count);
void prg_2_lambda_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak, uint8_t* sd_out,
    size_t sd_stride, size_t sd_index_base, size_t sd_index_xor, uint8_t* out, size_t out_stride,
    unsigned int seclvl, size_t count);
void prg_fixed_tweak_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                       uint32_t tweak, uint8_t* out, size_t out_stride,
                                       unsigned int seclvl, size_t outlen, size_t count);
void prg_4_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
                  unsigned int seclvl);

#endif
