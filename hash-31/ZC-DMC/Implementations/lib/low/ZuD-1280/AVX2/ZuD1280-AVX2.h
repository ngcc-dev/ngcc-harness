/*
Plain C implementation of Xoodoo5x4 (5x4x64), SnP-like API.
*/

#ifndef _ZuD1280_AVX2_h_
#define _ZuD1280_AVX2_h_

#include <stddef.h>
#include <stdint.h>
#include "../ZuD1280.h"

typedef struct {
    uint64_t A[ZUD1280_NLANES];
} ZuD1280_avx2_state;

#define ZuD1280_avx2_GetImplementation() \
    "AVX2 64-bit lanes (5X4X64)"
/* #define Xoodoo_plain_GetFeatures() (SnP_Feature_Main | SnP_Feature_Cyclist) */

#define ZuD1280_avx2_StaticInitialize()
void ZuD1280_avx2_Initialize(ZuD1280_avx2_state *state);
#define ZuD1280_avx2_AddByte(argS, argData, argOffset)    ((uint8_t*)argS)[argOffset] ^= (uint8_t)(argData)
void ZuD1280_avx2_AddBytes(ZuD1280_avx2_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1280_avx2_OverwriteBytes(ZuD1280_avx2_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1280_avx2_OverwriteWithZeroes(ZuD1280_avx2_state *state, unsigned int byteCount);
void ZuD1280_avx2_Permute_Nrounds(ZuD1280_avx2_state *state, unsigned int nrounds);
void ZuD1280_avx2_Permute_6rounds(ZuD1280_avx2_state *state);
void ZuD1280_avx2_Permute_12rounds(ZuD1280_avx2_state *state);
void ZuD1280_avx2_ExtractBytes(const ZuD1280_avx2_state *state, uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1280_avx2_ExtractAndAddBytes(const ZuD1280_avx2_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length);

//size_t Xoodyak_plain_AbsorbKeyedFullBlocks(Xoodoo_plain32_state *state, const uint8_t *X, size_t XLen);
//size_t Xoodyak_plain_AbsorbHashFullBlocks(Xoodoo_plain32_state *state, const uint8_t *X, size_t XLen);
//size_t Xoodyak_plain_SqueezeHashFullBlocks(Xoodoo_plain32_state *state, uint8_t *Y, size_t YLen);
//size_t Xoodyak_plain_SqueezeKeyedFullBlocks(Xoodoo_plain32_state *state, uint8_t *Y, size_t YLen);
//size_t Xoodyak_plain_EncryptFullBlocks(Xoodoo_plain32_state *state, const uint8_t *I, uint8_t *O, size_t IOLen);
//size_t Xoodyak_plain_DecryptFullBlocks(Xoodoo_plain32_state *state, const uint8_t *I, uint8_t *O, size_t IOLen);

#endif
