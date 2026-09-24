#ifndef RHYME_SM3_AVX2_H
#define RHYME_SM3_AVX2_H

#include <stddef.h>
#include <stdint.h>

#ifdef USE_AVX2_SM3
/* Single-hash AVX2 SM3 (digest_len_bits must be 256, msg_len_bits byte-aligned) */
int rhyme_sm3_hash_avx2(unsigned char *digest,
                        const unsigned char *msg,
                        unsigned long long msg_len_bits);

/* Batch-hash 8 XOF blocks: lane i = SM3(msg || (counter_start+i)_be32) */
void rhyme_sm3_xof_batch8(unsigned char lanes[8][32],
                          const unsigned char *msg,
                          size_t msg_len,
                          unsigned int counter_start);
#endif
#endif
