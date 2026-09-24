/*
 * ZuD1536 — 32-bit friendly implementation (3x8x64) without uint64_t arithmetic.
 *
 * Each 64-bit lane is represented as two 32-bit words (lo, hi) in little-endian order.
 */

#ifndef ZUD1536_32BIT_H
#define ZUD1536_32BIT_H

#include <stddef.h>
#include <stdint.h>
#include "../ZuD1536.h"

typedef struct {
    uint32_t A[ZUD1536_NLANES * 2u];
} ZuD1536_32_state;

#define ZuD1536_32_GetImplementation() "32-bit software implementation (3x8x64 emulated)"
#define ZuD1536_32_StaticInitialize()

void ZuD1536_32_Initialize(ZuD1536_32_state *state);
#define ZuD1536_32_AddByte(argS, argData, argOffset) ((uint8_t *)(argS))[(argOffset)] ^= (uint8_t)(argData)
void ZuD1536_32_AddBytes(ZuD1536_32_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1536_32_OverwriteBytes(ZuD1536_32_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1536_32_OverwriteWithZeroes(ZuD1536_32_state *state, unsigned int byteCount);
void ZuD1536_32_Permute_Nrounds(ZuD1536_32_state *state, unsigned int nrounds);
void ZuD1536_32_Permute_6rounds(ZuD1536_32_state *state);
void ZuD1536_32_Permute_12rounds(ZuD1536_32_state *state);
void ZuD1536_32_ExtractBytes(const ZuD1536_32_state *state, uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1536_32_ExtractAndAddBytes(const ZuD1536_32_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length);

#endif
