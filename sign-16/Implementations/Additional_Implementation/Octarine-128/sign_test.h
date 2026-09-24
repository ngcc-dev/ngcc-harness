#ifndef SIGN_TEST_H
#define SIGN_TEST_H

#include <stdint.h>
#include "sign.h"

typedef enum {
  SIGN_TEST_KEYGEN_RANDOM_HASH = 0,
  SIGN_TEST_KEYGEN_A_UNIFORM,
  SIGN_TEST_KEYGEN_A_UNIFORM_BASE,
  SIGN_TEST_KEYGEN_AWIN_BASE_SHAKE128X4,
  SIGN_TEST_KEYGEN_AWIN_BASE_UNPACK,
  SIGN_TEST_KEYGEN_A_PREPARE_P1,
  SIGN_TEST_KEYGEN_A_PREPARE_P2,
  SIGN_TEST_KEYGEN_S_UNIFORM,
  SIGN_TEST_KEYGEN_MUL_AS1,
  SIGN_TEST_KEYGEN_MUL_AS1_NTT_X2,
  SIGN_TEST_KEYGEN_MUL_AS1_NTT_P1,
  SIGN_TEST_KEYGEN_MUL_AS1_NTT_P2,
  SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_X2,
  SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_P1,
  SIGN_TEST_KEYGEN_MUL_AS1_INVNTT_P2,
  SIGN_TEST_KEYGEN_MUL_AS1_CRT,
  SIGN_TEST_KEYGEN_PACK_S1,
  SIGN_TEST_KEYGEN_ROUND_S2,
  SIGN_TEST_KEYGEN_PACK_B,
  SIGN_TEST_KEYGEN_HASH_STORE,

  SIGN_TEST_SIGN_SKDECODE,
  SIGN_TEST_SIGN_PREPARE_A,
  SIGN_TEST_SIGN_UNPACK_NTT_SECRET,
  SIGN_TEST_SIGN_HASH_MU_RHOPP,
  SIGN_TEST_SIGN_SAMPLE_Y,
  SIGN_TEST_SIGN_NTT_Y,
  SIGN_TEST_SIGN_MUL_AY,
  SIGN_TEST_SIGN_W1_HASH_C,
  SIGN_TEST_SIGN_SAMPLE_C_NTT,
  SIGN_TEST_SIGN_CS2_CHECK,
  SIGN_TEST_SIGN_CS1_CHECK,
  SIGN_TEST_SIGN_CB0_HINT,
  SIGN_TEST_SIGN_PACK,

  SIGN_TEST_VERIFY_UNPACK_CHECK,
  SIGN_TEST_VERIFY_PREPARE_A,
  SIGN_TEST_VERIFY_MUL_AZ,
  SIGN_TEST_VERIFY_SAMPLE_C_NTT,
  SIGN_TEST_VERIFY_UNPACK_HINT,
  SIGN_TEST_VERIFY_CB1_W1,
  SIGN_TEST_VERIFY_HASH_COMPARE,

  SIGN_TEST_STAGE_COUNT
} sign_test_stage;

extern uint64_t sign_test_cycles[SIGN_TEST_STAGE_COUNT];
extern unsigned int sign_test_rejection_rounds;
extern int sign_test_measure_awin_base;

void sign_test_add_cycles(unsigned int stage, uint64_t start);
void sign_test_reset_cycles(void);
const char *sign_test_stage_name(unsigned int stage);

int sig_keygen_test(
  unsigned char *pk, unsigned long long *pk_len_bytes,
  unsigned char *sk, unsigned long long *sk_len_bytes);

int sig_sign_test(
  unsigned char *sk, unsigned long long sk_len_bytes,
  unsigned char *m, unsigned long long m_len_bytes,
  unsigned char *sn, unsigned long long *sn_len_bytes);

int sig_verify_test(
  unsigned char *pk, unsigned long long pk_len_bytes,
  unsigned char *sn, unsigned long long sn_len_bytes,
  unsigned char *m, unsigned long long m_len_bytes);

#endif
