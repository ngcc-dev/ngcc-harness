/*
 * User-space wrapper for Hygon CIS SM4 instructions.
 *
 * The assembly backend follows the round-key layout used by the existing SM4
 * implementation: 32 host-endian uint32_t round keys generated from a
 * big-endian SM4 key.
 */
#ifndef HYGON_CIS_SM4_H
#define HYGON_CIS_SM4_H

#include <stddef.h>
#include <stdint.h>

void hygon_cis_sm4_set_key(const uint8_t* key, uint32_t* rk);
void hygon_cis_sm4_encrypt_block(const uint32_t* rk, const uint8_t* in, uint8_t* out);
void hygon_cis_sm4_encrypt_blocks(const uint32_t* rk, const uint8_t* in, uint8_t* out,
                                  size_t blocks);
void hygon_cis_sm4_prg_2lambda_from_key(const uint8_t* key, const uint8_t* ctr2,
                                        uint8_t* out2);
void hygon_cis_sm4_prg_2lambda_276_from_key(const uint8_t* key, const uint8_t* ctr2,
                                            uint8_t* out2, const uint8_t* ctr16,
                                            const uint8_t* ctr_tail, uint8_t* out276);
void hygon_cis_sm4_prg_2lambda_4way_from_key(const uint8_t* keys, size_t key_stride,
                                             const uint8_t* ctr2, uint8_t* out2,
                                             size_t out2_stride);
void hygon_cis_sm4_prg_2lambda_276_4way_from_key(const uint8_t* keys, size_t key_stride,
                                                 const uint8_t* ctr2, uint8_t* out2,
                                                 size_t out2_stride, const uint8_t* ctr16,
                                                 const uint8_t* ctr_tail,
                                                 uint8_t* const out276[4]);
void hygon_cis_sm4_prg_2lambda_276_8way_from_key(const uint8_t* keys, size_t key_stride,
                                                 const uint8_t* ctr2, uint8_t* out2,
                                                 size_t out2_stride, const uint8_t* ctr16,
                                                 const uint8_t* ctr_tail,
                                                 uint8_t* const out276[8]);

int hygon_cis_sm4_selftest(void);
int hygon_cis_sm4_is_supported(void);

#endif
