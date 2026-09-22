/*
The eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP

The Xoodoo permutation, designed by Joan Daemen, Seth Hoffert, Gilles Van Assche and Ronny Van Keer.

Implementation by Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _ZuD1536_plain_h_
#define _ZuD1536_plain_h_

#include <stddef.h>
#include <stdint.h>
#include "../ZuD1536.h"

typedef struct {
    uint64_t A[ZUD1536_NLANES];
} ZuD1536_plain64_state;

#define ZuD1536_plain_GetImplementation() \
    "plain 64-bit implementation (3X8X64 Xoodoo-like)"
/* #define Xoodoo_plain_GetFeatures() (SnP_Feature_Main | SnP_Feature_Cyclist) */

#define ZuD1536_plain_StaticInitialize()
void ZuD1536_plain_Initialize(ZuD1536_plain64_state *state);
#define ZuD1536_plain_AddByte(argS, argData, argOffset)    ((uint8_t*)argS)[argOffset] ^= (uint8_t)(argData)
void ZuD1536_plain_AddBytes(ZuD1536_plain64_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1536_plain_OverwriteBytes(ZuD1536_plain64_state *state, const uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1536_plain_OverwriteWithZeroes(ZuD1536_plain64_state *state, unsigned int byteCount);
void ZuD1536_plain_Permute_Nrounds(ZuD1536_plain64_state *state, unsigned int nrounds);
void ZuD1536_plain_Permute_6rounds(ZuD1536_plain64_state *state);
void ZuD1536_plain_Permute_12rounds(ZuD1536_plain64_state *state);
void ZuD1536_plain_ExtractBytes(const ZuD1536_plain64_state *state, uint8_t *data, unsigned int offset, unsigned int length);
void ZuD1536_plain_ExtractAndAddBytes(const ZuD1536_plain64_state *state, const uint8_t *input, uint8_t *output, unsigned int offset, unsigned int length);

//size_t Xoodyak_plain_AbsorbKeyedFullBlocks(Xoodoo_plain32_state *state, const uint8_t *X, size_t XLen);
//size_t Xoodyak_plain_AbsorbHashFullBlocks(Xoodoo_plain32_state *state, const uint8_t *X, size_t XLen);
//size_t Xoodyak_plain_SqueezeHashFullBlocks(Xoodoo_plain32_state *state, uint8_t *Y, size_t YLen);
//size_t Xoodyak_plain_SqueezeKeyedFullBlocks(Xoodoo_plain32_state *state, uint8_t *Y, size_t YLen);
//size_t Xoodyak_plain_EncryptFullBlocks(Xoodoo_plain32_state *state, const uint8_t *I, uint8_t *O, size_t IOLen);
//size_t Xoodyak_plain_DecryptFullBlocks(Xoodoo_plain32_state *state, const uint8_t *I, uint8_t *O, size_t IOLen);

#endif
