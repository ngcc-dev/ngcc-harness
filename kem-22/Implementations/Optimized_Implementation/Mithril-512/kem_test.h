#ifndef KEM_TEST_H
#define KEM_TEST_H

#include <stdint.h>
#include "kem.h"

typedef enum {
  KEM_TEST_PKE_KEYGEN_A_UNIFORM = 0,
  KEM_TEST_PKE_KEYGEN_A_UNIFORM_BASE,
  KEM_TEST_PKE_KEYGEN_A_PREPARE,
  KEM_TEST_PKE_KEYGEN_S_UNIFORM,
  KEM_TEST_PKE_KEYGEN_PACK_S,
  KEM_TEST_PKE_KEYGEN_MUL_AS_ROUND,
  KEM_TEST_PKE_KEYGEN_COPY_SEED_A,
  KEM_TEST_PKE_KEYGEN_PACK_B,

  KEM_TEST_PKE_ENC_A_UNIFORM,
  KEM_TEST_PKE_ENC_A_UNIFORM_BASE,
  KEM_TEST_PKE_ENC_A_PREPARE,
  KEM_TEST_PKE_ENC_SP_UNIFORM,
  KEM_TEST_PKE_ENC_MUL_ASP_ROUND,
  KEM_TEST_PKE_ENC_UNPACK_B,
  KEM_TEST_PKE_ENC_UNPACK_B_BASE,
  KEM_TEST_PKE_ENC_UNPACK_B_PREPARE,
  KEM_TEST_PKE_ENC_MUL_BSP_REDUCE,
  KEM_TEST_PKE_ENC_ADD_MSG,
  KEM_TEST_PKE_ENC_PACK_CM,
  KEM_TEST_PKE_ENC_PACK_BP,

  KEM_TEST_PKE_DEC_UNPACK_S,
  KEM_TEST_PKE_DEC_UNPACK_BP,
  KEM_TEST_PKE_DEC_UNPACK_CM,
  KEM_TEST_PKE_DEC_BP_TO_AWIN,
  KEM_TEST_PKE_DEC_BP_TO_AWIN_BASE,
  KEM_TEST_PKE_DEC_BP_TO_AWIN_PREPARE,
  KEM_TEST_PKE_DEC_MUL_BPS_REDUCE,
  KEM_TEST_PKE_DEC_SUB_ROUND_PACK,

  KEM_TEST_KEM_KEYGEN_RANDOM_SEEDS,
  KEM_TEST_KEM_KEYGEN_PKE_KEYGEN,
  KEM_TEST_KEM_KEYGEN_COPY_PK,
  KEM_TEST_KEM_KEYGEN_HASH_PK,
  KEM_TEST_KEM_KEYGEN_RANDOM_Z,
  KEM_TEST_KEM_KEYGEN_SET_LENGTHS,

  KEM_TEST_KEM_ENC_CHECK_PK_LEN,
  KEM_TEST_KEM_ENC_RANDOM_M,
  KEM_TEST_KEM_ENC_HASH_PK,
  KEM_TEST_KEM_ENC_HASH_G,
  KEM_TEST_KEM_ENC_PKE_ENCRYPT,
  KEM_TEST_KEM_ENC_COPY_SS,
  KEM_TEST_KEM_ENC_SET_LENGTHS,

  KEM_TEST_KEM_DEC_CHECK_LENGTHS,
  KEM_TEST_KEM_DEC_COPY_HPK,
  KEM_TEST_KEM_DEC_PKE_DECRYPT,
  KEM_TEST_KEM_DEC_HASH_G,
  KEM_TEST_KEM_DEC_REENCRYPT,
  KEM_TEST_KEM_DEC_HASH_CTZ,
  KEM_TEST_KEM_DEC_COMPARE_SELECT,
  KEM_TEST_KEM_DEC_SET_LENGTHS,

  KEM_TEST_STAGE_COUNT
} kem_test_stage;

extern uint64_t kem_test_cycles[KEM_TEST_STAGE_COUNT];

void kem_test_add_cycles(unsigned int stage, uint64_t start);
void kem_test_reset_cycles(void);
const char *kem_test_stage_name(unsigned int stage);

int kem_keygen_test(
  unsigned char *pk, unsigned long long *pk_len_bytes,
  unsigned char *sk, unsigned long long *sk_len_bytes);

int kem_enc_test(
  unsigned char *pk, unsigned long long pk_len_bytes,
  unsigned char *ss, unsigned long long *ss_len_bytes,
  unsigned char *ct, unsigned long long *ct_len_bytes);

int kem_dec_test(
  unsigned char *sk, unsigned long long sk_len_bytes,
  unsigned char *ct, unsigned long long ct_len_bytes,
  unsigned char *ss, unsigned long long *ss_len_bytes);

#endif
