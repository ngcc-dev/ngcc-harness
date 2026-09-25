/*
 * Ballet-based PRG interfaces used by BAVC/VOLE paths.
 *
 * Naming notes (important for call sites):
 * - batch:
 *   process `count` independent lanes in one API call. Lane i reads key from
 *   `keys + i * key_stride` and writes output to `out + i * out_stride`.
 * - fixed_tweak:
 *   all lanes share the same tweak value. (Non-fixed forms typically use
 *   tweak_i = tweak_base + i.)
 * - with_sd:
 *   besides PRG outputs, also writes each lane seed/key prefix back to `sd_out`
 *   with stride `sd_stride`.
 * - xor_map:
 *   writeback slot is remapped by
 *   mapped = (sd_index_base + i) ^ sd_index_xor;
 *   when mapped == 0, that lane's mapped destination is treated as skipped.
 * - fuse / and_288:
 *   fused APIs generate multiple outputs in one pass, reusing key schedule and
 *   counter setup to reduce overhead:
 *   * `and_288`: this d3-128f/bf160 path emits 288-byte
 *     output (+ optional sd writeback)
 */

#ifndef SM4TH_PRG_H
#define SM4TH_PRG_H

#include "params.h"

#include <stdint.h>
#include <stdlib.h>

/* Batched 2 * Nblock expansion, one independent key per lane. */
// used for GGM seeds expansion, each prg has different keys and different tweaks (plaintext)
void prg_2_lambda_independent_batch(const uint8_t* keys, size_t key_stride, const uint8_t* iv,
                                    uint32_t tweak_base, uint8_t* out, size_t out_stride,
                                    unsigned int seclvl, size_t count);

/* Fused batched path: seed writeback + 2 * Nblock + 288-byte output. */
void prg_2_lambda_and_288_fixed_tweak_independent_batch_with_sd(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_288, uint8_t* sd_out, size_t sd_stride, uint8_t* out_2lambda,
    size_t out_2lambda_stride, uint8_t* out_288, size_t out_288_stride, unsigned int seclvl,
    size_t count);

/* XOR-mapped variant of fused (2 * Nblock + 288 + seed writeback). */
void prg_2_lambda_and_288_fixed_tweak_independent_batch_with_sd_xor_map(
    const uint8_t* keys, size_t key_stride, const uint8_t* iv, uint32_t tweak_2lambda,
    uint32_t tweak_288, uint8_t* sd_out, size_t sd_stride, size_t sd_index_base,
    size_t sd_index_xor, uint8_t* out_2lambda, size_t out_2lambda_stride, uint8_t* out_288,
    size_t out_288_stride, unsigned int seclvl, size_t count);

#endif
