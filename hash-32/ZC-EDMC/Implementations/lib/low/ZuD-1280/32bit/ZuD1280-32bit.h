/*
 * ZuD1280 — 32-bit friendly implementation (5x4x64) without uint64_t arithmetic.
 *
 * Each 64-bit lane is represented as two 32-bit words (lo, hi) in little-endian order.
 * This is intended for 32-bit embedded software platforms.
 */

#ifndef ZUD1280_32BIT_H
#define ZUD1280_32BIT_H

#include <stddef.h>
#include <stdint.h>
#include "../ZuD1280.h"

typedef struct {
    /* A[(row*cols + col)*2 + 0] = lo32, +1 = hi32 */
    uint32_t A[ZUD1280_NLANES * 2u];
} ZuD1280_32_state;

#define ZuD1280_32_GetImplementation() "32-bit software implementation (5x4x64 emulated)"
#define ZuD1280_32_StaticInitialize()

void ZuD1280_32_Initialize(ZuD1280_32_state *state);
#define ZuD1280_32_AddByte(argS, argData, argOffset) ((uint8_t *)(argS))[(argOffset)] ^= (uint8_t)(argData)
void ZuD1280_32_AddBytes(ZuD1280_32_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1280_32_OverwriteBytes(ZuD1280_32_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1280_32_OverwriteWithZeroes(ZuD1280_32_state *state, unsigned int byteCount);
void ZuD1280_32_Permute_Nrounds(ZuD1280_32_state *state, unsigned int nrounds);
void ZuD1280_32_Permute_6rounds(ZuD1280_32_state *state);
void ZuD1280_32_Permute_12rounds(ZuD1280_32_state *state);
void ZuD1280_32_ExtractBytes(const ZuD1280_32_state *state, uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1280_32_ExtractAndAddBytes(const ZuD1280_32_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length);

#endif
