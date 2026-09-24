#ifndef LORE_SM3_NEON_H
#define LORE_SM3_NEON_H

#include <stdint.h>

#ifdef LORE_USE_NEON_SM3
int lore_sm3_hash_neon(unsigned char *digest,
                       const unsigned char *msg,
                       unsigned long long msg_len_bits);

int lore_sm3_pseudoXOF_neon(unsigned long long output_len_bits,
                            const unsigned char *msg,
                            unsigned long long msg_len_bits,
                            unsigned char *output);
#endif

#endif
