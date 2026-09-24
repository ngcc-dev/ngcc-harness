/**
 * @file parsing.c
 * @brief Functions to parse HARE public keys, secret keys, and ciphertexts
 */

 #include "parsing.h"
 #include <stdint.h>
 #include <string.h>
 #include "crypto_memset.h"
 #include "symmetric.h"
 #include "vector.h"
 
 static uint8_t get_bit_u64_vec(const uint64_t *src, uint32_t bitpos) {
     return (uint8_t)((src[bitpos >> 6] >> (bitpos & 63u)) & 1ULL);
 }
 
 static void set_bit_u64_vec(uint64_t *dst, uint32_t bitpos, uint8_t bit) {
     const uint64_t mask = 1ULL << (bitpos & 63u);
     if (bit) {
         dst[bitpos >> 6] |= mask;
     } else {
         dst[bitpos >> 6] &= ~mask;
     }
 }
 
 static uint8_t get_bit_u8_buf(const uint8_t *src, uint32_t bitpos) {
     return (uint8_t)((src[bitpos >> 3] >> (bitpos & 7u)) & 1u);
 }
 
 static void set_bit_u8_buf(uint8_t *dst, uint32_t bitpos, uint8_t bit) {
     const uint8_t mask = (uint8_t)(1u << (bitpos & 7u));
     if (bit) {
         dst[bitpos >> 3] |= mask;
     } else {
         dst[bitpos >> 3] &= (uint8_t)~mask;
     }
 }
 
 
 static void canonicalize_tail_bits(uint64_t *vec, uint32_t bit_len, uint32_t words64) {
     if (bit_len == 0 || words64 == 0) return;
     const uint32_t used = bit_len & 63u;
     if (used == 0) return;
     const uint64_t mask = (used == 64u) ? ~0ULL : ((1ULL << used) - 1ULL);
     vec[words64 - 1u] &= mask;
 }
 /**
  * @brief Deserializes a decryption key into its internal vectorized form.
  *
  * @param[out] y        Pointer to the output buffer where the internal vectorized key will be stored.
  * @param[in]  dk_pke   Pointer to the serialized decryption key.
  */
 void hqc_dk_pke_from_string(uint64_t *y, const uint8_t *dk_pke) {
     shake256_xof_ctx dk_xof_ctx = {0};
     xof_init(&dk_xof_ctx, dk_pke, SEED_BYTES);
     vect_sample_fixed_weight1(&dk_xof_ctx, y, PARAM_OMEGA);
 
     // Zeroize sensitive data
     memset_zero(&dk_xof_ctx, sizeof dk_xof_ctx);
 }
 
 /**
  * @brief Deserializes an encryption key into its internal representation.
  *
  * @param[out] h        Pointer to the output buffer for `h` the first internal component of the key.
  * @param[out] s        Pointer to the output buffer for `s` the second internal component of the key.
  * @param[in]  ek_pke   Pointer to the serialized encryption key.
  */
 void hqc_ek_pke_from_string(uint64_t *h, uint64_t *s, const uint8_t *ek_pke) {
     shake256_xof_ctx ek_xof_ctx = {0};
 
     xof_init(&ek_xof_ctx, ek_pke, SEED_BYTES);
     vect_set_random(&ek_xof_ctx, h);
 
     memcpy(s, ek_pke + SEED_BYTES, VEC_N_SIZE_BYTES);
 }
 
 /**
  * @brief Serializes a KEM ciphertext structure into a byte array.
  *
  * @param[out] ct       Pointer to the output buffer where the serialized ciphertext will be stored.
  * @param[in]  c_kem    Pointer to the KEM ciphertext structure to be serialized.
  */
 void hqc_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem) {
     uint8_t *payload = ct + VEC_N_SIZE_BYTES;
 
     memcpy(ct, c_kem->c_pke.u, VEC_N_SIZE_BYTES);
     memset(payload, 0, VEC_COMPRESSED_PAYLOAD_BYTES);
 
     for (uint32_t i = 0; i < PARAM_NM; i++) {
         set_bit_u8_buf(payload, i, get_bit_u64_vec(c_kem->c_pke.m_tilde, i));
     }
#if PARAM_NV2 > 0
     for (uint32_t i = 0; i < PARAM_NV2; i++) {
         set_bit_u8_buf(payload, PARAM_NM + i, get_bit_u64_vec(c_kem->c_pke.v2, i));
     }
#endif
 
     memcpy(ct + VEC_N_SIZE_BYTES + VEC_COMPRESSED_PAYLOAD_BYTES, c_kem->salt, SALT_BYTES);
 }
 
 /**
  * @brief Deserializes a KEM ciphertext byte array into its structured components.
  *
  * @param[out] c_pke    Pointer to the output buffer where the deserialized PKE ciphertext will be stored.
  * @param[out] salt     Pointer to the output buffer where the extracted salt will be stored.
  * @param[in]  ct       Pointer to the serialized KEM ciphertext.
  */
 void hqc_c_kem_from_string(ciphertext_pke_t *c_pke, uint8_t *salt, const uint8_t *ct) {
     const uint8_t *payload = ct + VEC_N_SIZE_BYTES;
 
     memcpy(c_pke->u, ct, VEC_N_SIZE_BYTES);
     memset(c_pke->m_tilde, 0, sizeof c_pke->m_tilde);
     memset(c_pke->v2, 0, sizeof c_pke->v2);
 
     for (uint32_t i = 0; i < PARAM_NM; i++) {
         set_bit_u64_vec(c_pke->m_tilde, i, get_bit_u8_buf(payload, i));
     }
#if PARAM_NV2 > 0
     for (uint32_t i = 0; i < PARAM_NV2; i++) {
         set_bit_u64_vec(c_pke->v2, i, get_bit_u8_buf(payload, PARAM_NM + i));
     }
#endif
 
     canonicalize_tail_bits(c_pke->u, PARAM_N, VEC_N_SIZE_64);
     canonicalize_tail_bits(c_pke->m_tilde, PARAM_NM, VEC_NM_SIZE_64);
     canonicalize_tail_bits(c_pke->v2, PARAM_NV2, VEC_NV2_SIZE_64);
 
     memcpy(salt, ct + VEC_N_SIZE_BYTES + VEC_COMPRESSED_PAYLOAD_BYTES, SALT_BYTES);
 } 
