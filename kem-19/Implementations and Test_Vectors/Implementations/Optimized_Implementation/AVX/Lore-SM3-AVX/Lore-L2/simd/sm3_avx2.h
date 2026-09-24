#ifndef LORE_SM3_AVX2_H
#define LORE_SM3_AVX2_H

#include <stdint.h>

#ifdef LORE_USE_AVX2_SM3
int lore_sm3_hash_avx2(unsigned char *digest,
                       const unsigned char *msg,
                       unsigned long long msg_len_bits);

int lore_sm3_pseudoXOF_avx2(unsigned long long output_len_bits,
                            const unsigned char *msg,
                            unsigned long long msg_len_bits,
                            unsigned char *output);
#endif

#endif
