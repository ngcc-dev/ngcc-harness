#ifndef VISTRUTITH_CONSTRAINTS_H
#define VISTRUTITH_CONSTRAINTS_H

#include <stdbool.h>
#include <stddef.h>

#include "../fields.h"
#include "vistrutah.h"
#include "vistrutith_witness.h"

#define VISTRUTITH_ENC_CSTRNTS_STEPS (VISTRUTAH_512_ROUNDS_LONG_512KEY / ROUNDS_PER_STEP)
#define VISTRUTITH_ENC_CSTRNTS_NORM_LEN \
  (VISTRUTITH_ENC_CSTRNTS_STEPS * VISTRUTAH_512_BLOCK_SIZE)
#define VISTRUTITH_ENC_CSTRNTS_IO_LEN \
  (VISTRUTITH_ENC_CSTRNTS_STEPS * VISTRUTAH_512_BLOCK_SIZE)

/* Prover bits-side aliases (concrete-byte operations). */
#define vistrutith_shuffle_key_bits vistrutah_shuffle_key
#define vistrutith_rotate_bytes_bits vistrutah_rotate_bytes

/* ShuffleKey */
void vistrutith_shuffle_key_prover(uint8_t fixed_key_bits[VISTRUTAH_KEY_SIZE_512],
                                   bf512_t fixed_key_tag[VISTRUTAH_KEY_SIZE_512 * 8u]);
void vistrutith_shuffle_key_verifier(bf512_t fixed_key_key[VISTRUTAH_KEY_SIZE_512 * 8u]);

/* RotateBytes */
void vistrutith_rotate_bytes_prover(uint8_t* fixed_key_bits, bf512_t* fixed_key_tag,
                                    int shift, int len);
void vistrutith_rotate_bytes_verifier(bf512_t* fixed_key_key, int shift, int len);

/* StateToBytes */
void vistrutith_state_to_bytes_prover(bf512_t out_val[16], bf512_t out_tag[16],
                                      const uint8_t state_bits[16],
                                      const bf512_t state_tag[128]);
void vistrutith_state_to_bytes_verifier(bf512_t out_key[16],
                                        const bf512_t state_key[128]);

/* StateToConjugates */
void vistrutith_state_to_conjugates_prover(bf512_t out_val[128], bf512_t out_tag[128],
                                           const uint8_t state_bits[16],
                                           const bf512_t state_tag[128]);
void vistrutith_state_to_conjugates_verifier(bf512_t out_key[128],
                                             const bf512_t state_key[128]);

/* InvNormToConjugates */
void vistrutith_invnorm_to_conjugates_prover(bf512_t out_val[4], bf512_t out_tag[4],
                                             uint8_t x_bits, const bf512_t x_tag[4]);
void vistrutith_invnorm_to_conjugates_verifier(bf512_t out_key[4],
                                               const bf512_t x_key[4]);

/* SBoxAffine */
// Note: the input values are also bf512
void vistrutith_sbox_affine_prover(bf512_t* out_val, bf512_t* out_tag,
                                   const bf512_t* in_val, const bf512_t* in_tag,
                                   size_t nbytes, bool sq);
void vistrutith_sbox_affine_verifier(bf512_t* out_key, const bf512_t* in_key,
                                     size_t nbytes, bool sq);

/* ShiftRows */
void vistrutith_shiftrows_prover(bf512_t* out_val, bf512_t* out_tag,
                                 const bf512_t* in_val, const bf512_t* in_tag,
                                 size_t nbytes);
void vistrutith_shiftrows_verifier(bf512_t* out_key, const bf512_t* in_key, size_t nbytes);

/* MixColumns */
void vistrutith_mixcolumns_prover(bf512_t* out_val, bf512_t* out_tag,
                                  const bf512_t* in_val, const bf512_t* in_tag,
                                  size_t nbytes, bool sq);
void vistrutith_mixcolumns_verifier(bf512_t* out_key, const bf512_t* in_key,
                                    size_t nbytes, bool sq);

/* BitInvMixColumns */
void vistrutith_bit_inv_mixcolumns_prover(uint8_t out_bits[16], bf512_t out_tag[128],
                                          const uint8_t in_bits[16],
                                          const bf512_t in_tag[128]);
void vistrutith_bit_inv_mixcolumns_verifier(bf512_t out_key[128],
                                            const bf512_t in_key[128]);

/* BitInvShiftRows */
void vistrutith_bit_inv_shiftrows_prover(uint8_t out_bits[16], bf512_t out_tag[128],
                                         const uint8_t in_bits[16],
                                         const bf512_t in_tag[128]);
void vistrutith_bit_inv_shiftrows_verifier(bf512_t out_key[128],
                                           const bf512_t in_key[128]);

/* BitInvSBoxAffine */
void vistrutith_bit_inv_sbox_affine_prover(uint8_t out_bits[16], bf512_t out_tag[128],
                                           const uint8_t in_bits[16],
                                           const bf512_t in_tag[128]);
void vistrutith_bit_inv_sbox_affine_verifier(bf512_t out_key[128],
                                             const bf512_t in_key[128]);

/* BitInvMix (inverse of ViMix on 512-bit state) */
void vistrutith_inv_mix_prover(
    uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t in_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]);
void vistrutith_inv_mix_verifier(
    bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t in_key[VISTRUTAH_512_BLOCK_SIZE * 8u]);

/* ForwardByNorm */
void vistrutith_forward_by_norm_prover(
    bf512_t out_a_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_a_tag[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t out_a_sq_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_a_sq_tag[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t z_norm_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t z_norm_tag[VISTRUTAH_512_BLOCK_SIZE],
    const uint8_t in_state_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_state_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t norm_packed[VISTRUTAH_512_BLOCK_SIZE / 2u],
    const bf512_t norm_tag[VISTRUTAH_512_BLOCK_SIZE * 4u],
    const uint8_t fixed_key_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t fixed_key_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]);

void vistrutith_forward_by_norm_verifier(
    bf512_t out_a_key[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_a_sq_key[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t z_norm_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_state_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t norm_key[VISTRUTAH_512_BLOCK_SIZE * 4u],
    const bf512_t fixed_key_key[VISTRUTAH_512_BLOCK_SIZE * 8u]);

/* BackwardZeroRndByBits */
void vistrutith_backward_zerornd_by_bits_prover(
    uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t m_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t m_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t rk_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t rk_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t rc_bits[16]);
void vistrutith_backward_zerornd_by_bits_verifier(
    bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t m_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t rk_key[VISTRUTAH_512_BLOCK_SIZE * 8u], const uint8_t rc_bits[16]);

/* BackwardFinalRndByBits */
void vistrutith_backward_finalrnd_by_bits_prover(
    uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t ct_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t ct_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t rk_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t rk_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]);
void vistrutith_backward_finalrnd_by_bits_verifier(
    bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t ct_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t rk_key[VISTRUTAH_512_BLOCK_SIZE * 8u]);

/* AppendCstrntsByBits */
void vistrutith_append_cstrnts_by_bits_prover(
    bf512_t z0_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t z0_tag[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t z1_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t z1_tag[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_val[VISTRUTAH_512_BLOCK_SIZE], const bf512_t x_tag[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_sq_val[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_sq_tag[VISTRUTAH_512_BLOCK_SIZE],
    const uint8_t y_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t y_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]);
void vistrutith_append_cstrnts_by_bits_verifier(
    bf512_t z0_key[VISTRUTAH_512_BLOCK_SIZE], bf512_t z1_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_sq_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t y_key[VISTRUTAH_512_BLOCK_SIZE * 8u]);

/* EncCstrnts */
void vistrutith_enc_constraints_prover(
    bf512_t z_norm_val[VISTRUTITH_ENC_CSTRNTS_NORM_LEN],
    bf512_t z_norm_tag[VISTRUTITH_ENC_CSTRNTS_NORM_LEN],
    bf512_t z_io0_val[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io0_tag[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io1_val[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io1_tag[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    const uint8_t in_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t w[VISTRUTAH_WITNESS_TOTAL_BYTES],
    const bf512_t w_tag[VISTRUTAH_WITNESS_TOTAL_BYTES * 8u]);

void vistrutith_enc_constraints_verifier(
    bf512_t z_norm_key[VISTRUTITH_ENC_CSTRNTS_NORM_LEN],
    bf512_t z_io0_key[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io1_key[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    const bf512_t in_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t w_key[VISTRUTAH_WITNESS_TOTAL_BYTES * 8u]);

#endif
