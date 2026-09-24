#include <string.h>
#include "kem_test.h"
#include "hash_domain.h"
#include "pke_test.h"
#include "drng.h"
#include "test/cpucycles.h"

extern DRNG_ctx drng_algorithm;

uint64_t kem_test_cycles[KEM_TEST_STAGE_COUNT];

static uint64_t kem_test_overhead;

void kem_test_add_cycles(unsigned int stage, uint64_t start)
{
  uint64_t elapsed = cpucycles() - start;

  if(stage < KEM_TEST_STAGE_COUNT && elapsed > kem_test_overhead) {
    kem_test_cycles[stage] += elapsed - kem_test_overhead;
  }
}

void kem_test_reset_cycles(void)
{
  if(kem_test_overhead == 0) {
    kem_test_overhead = cpucycles_overhead();
  }

  memset(kem_test_cycles, 0, sizeof(kem_test_cycles));
}

const char *kem_test_stage_name(unsigned int stage)
{
  static const char *names[KEM_TEST_STAGE_COUNT] = {
    "pke.keygen.A_uniform_Awin",
    "pke.keygen.Awin_base",
    "pke.keygen.Awin_prepare",
    "pke.keygen.s_uniform",
    "pke.keygen.pack_s",
    "pke.keygen.A_times_s_round",
    "pke.keygen.copy_seedA",
    "pke.keygen.pack_b",

    "pke.enc.A_uniform_Awin",
    "pke.enc.Awin_base",
    "pke.enc.Awin_prepare",
    "pke.enc.sp_uniform",
    "pke.enc.A_times_sp_round",
    "pke.enc.unpack_b_Awin",
    "pke.enc.unpack_b_base",
    "pke.enc.unpack_b_prepare",
    "pke.enc.b_times_sp_reduce",
    "pke.enc.add_msg",
    "pke.enc.pack_cm",
    "pke.enc.pack_bp",

    "pke.dec.unpack_s",
    "pke.dec.unpack_bp",
    "pke.dec.unpack_cm",
    "pke.dec.bp_to_Awin",
    "pke.dec.bp_to_Awin_base",
    "pke.dec.bp_to_Awin_prepare",
    "pke.dec.bp_times_s_reduce",
    "pke.dec.sub_round_pack",

    "kem.keygen.random_seeds",
    "kem.keygen.pke_keygen",
    "kem.keygen.copy_pk",
    "kem.keygen.hash_pk",
    "kem.keygen.random_z",
    "kem.keygen.set_lengths",

    "kem.enc.check_pk_len",
    "kem.enc.random_m",
    "kem.enc.hash_pk",
    "kem.enc.hash_g",
    "kem.enc.pke_encrypt",
    "kem.enc.copy_ss",
    "kem.enc.set_lengths",

    "kem.dec.check_lengths",
    "kem.dec.copy_hpk",
    "kem.dec.pke_decrypt",
    "kem.dec.hash_g",
    "kem.dec.reencrypt",
    "kem.dec.hash_c_z",
    "kem.dec.compare_select",
    "kem.dec.set_lengths"
  };

  return stage < KEM_TEST_STAGE_COUNT ? names[stage] : "unknown";
}

int kem_keygen_test(
  unsigned char *pk, unsigned long long *pk_len_bytes,
  unsigned char *sk, unsigned long long *sk_len_bytes)
{
  uint64_t ts;
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
  unsigned char seedS[RRLWR_SEED_S_LEN];

  ts = cpucycles();
  GENERATE_RANDOM_BYTES(seedA, RRLWR_PKE_SEED_A_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedS, RRLWR_SEED_S_LEN, &drng_algorithm);
  kem_test_add_cycles(KEM_TEST_KEM_KEYGEN_RANDOM_SEEDS, ts);

  ts = cpucycles();
  pke_keygen_test(pk, sk, seedA, seedS);
  kem_test_add_cycles(KEM_TEST_KEM_KEYGEN_PKE_KEYGEN, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_PKE_PK_LEN; i++) {
    sk[RRLWR_PKE_SK_LEN + i] = pk[i];
  }
  kem_test_add_cycles(KEM_TEST_KEM_KEYGEN_COPY_PK, ts);

  ts = cpucycles();
  RRLWR_KEM_HASH_F(&sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN], pk);
  kem_test_add_cycles(KEM_TEST_KEM_KEYGEN_HASH_PK, ts);

  ts = cpucycles();
  GENERATE_RANDOM_BYTES(&sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN],
                        RRLWR_KEM_SEED_Z_LEN,
                        &drng_algorithm);
  kem_test_add_cycles(KEM_TEST_KEM_KEYGEN_RANDOM_Z, ts);

  ts = cpucycles();
  *sk_len_bytes = RRLWR_KEM_SK_LEN;
  *pk_len_bytes = RRLWR_KEM_PK_LEN;
  kem_test_add_cycles(KEM_TEST_KEM_KEYGEN_SET_LENGTHS, ts);

  return 0;
}

int kem_enc_test(
  unsigned char *pk, unsigned long long pk_len_bytes,
  unsigned char *ss, unsigned long long *ss_len_bytes,
  unsigned char *ct, unsigned long long *ct_len_bytes)
{
  uint64_t ts;
  unsigned char hpk_m[RRLWR_KEM_HPK_LEN + RRLWR_PKE_MESSAGE_LEN];
  unsigned char *hpk = hpk_m;
  unsigned char *m = hpk_m + RRLWR_KEM_HPK_LEN;
  unsigned char K_seedSp[RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN];
  unsigned char *K = K_seedSp;
  unsigned char *seedSp = K + RRLWR_KEM_SS_LEN;

  ts = cpucycles();
  if(pk_len_bytes != RRLWR_KEM_PK_LEN) {
    kem_test_add_cycles(KEM_TEST_KEM_ENC_CHECK_PK_LEN, ts);
    return -1;
  }
  kem_test_add_cycles(KEM_TEST_KEM_ENC_CHECK_PK_LEN, ts);

  ts = cpucycles();
  GENERATE_RANDOM_BYTES(m, RRLWR_PKE_MESSAGE_LEN, &drng_algorithm);
  kem_test_add_cycles(KEM_TEST_KEM_ENC_RANDOM_M, ts);

  ts = cpucycles();
  RRLWR_KEM_HASH_F(hpk, pk);
  kem_test_add_cycles(KEM_TEST_KEM_ENC_HASH_PK, ts);

  ts = cpucycles();
  RRLWR_KEM_HASH_G(K_seedSp, RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN,
                   hpk, m);
  kem_test_add_cycles(KEM_TEST_KEM_ENC_HASH_G, ts);

  ts = cpucycles();
  pke_encrypt_test(ct, pk, m, seedSp);
  kem_test_add_cycles(KEM_TEST_KEM_ENC_PKE_ENCRYPT, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_KEM_SS_LEN; i++) {
    ss[i] = K[i];
  }
  kem_test_add_cycles(KEM_TEST_KEM_ENC_COPY_SS, ts);

  ts = cpucycles();
  *ss_len_bytes = RRLWR_KEM_SS_LEN;
  *ct_len_bytes = RRLWR_KEM_CT_LEN;
  kem_test_add_cycles(KEM_TEST_KEM_ENC_SET_LENGTHS, ts);

  return 0;
}

int kem_dec_test(
  unsigned char *sk, unsigned long long sk_len_bytes,
  unsigned char *ct, unsigned long long ct_len_bytes,
  unsigned char *ss, unsigned long long *ss_len_bytes)
{
  uint64_t ts;
  unsigned char hpk_mp[RRLWR_KEM_HPK_LEN + RRLWR_PKE_MESSAGE_LEN];
  unsigned char *mp = hpk_mp + RRLWR_KEM_HPK_LEN;
  unsigned char K_seedSp[RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN];
  unsigned char *K = K_seedSp;
  unsigned char *seedSp = K + RRLWR_KEM_SS_LEN;
  unsigned char Kb[RRLWR_KEM_SS_LEN];
  unsigned char ctp[RRLWR_KEM_CT_LEN];
  unsigned char z[RRLWR_KEM_SEED_Z_LEN];
  unsigned char *pk = sk + RRLWR_PKE_SK_LEN;

  ts = cpucycles();
  if(sk_len_bytes != RRLWR_KEM_SK_LEN || ct_len_bytes != RRLWR_KEM_CT_LEN) {
    kem_test_add_cycles(KEM_TEST_KEM_DEC_CHECK_LENGTHS, ts);
    return -1;
  }
  kem_test_add_cycles(KEM_TEST_KEM_DEC_CHECK_LENGTHS, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_KEM_HPK_LEN; i++) {
    hpk_mp[i] = sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + i];
  }
  kem_test_add_cycles(KEM_TEST_KEM_DEC_COPY_HPK, ts);

  ts = cpucycles();
  pke_decrypt_test(mp, ct, sk);
  kem_test_add_cycles(KEM_TEST_KEM_DEC_PKE_DECRYPT, ts);

  ts = cpucycles();
  RRLWR_KEM_HASH_G(K_seedSp, RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN,
                   hpk_mp, mp);
  kem_test_add_cycles(KEM_TEST_KEM_DEC_HASH_G, ts);

  ts = cpucycles();
  pke_encrypt_test(ctp, pk, mp, seedSp);
  kem_test_add_cycles(KEM_TEST_KEM_DEC_REENCRYPT, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_KEM_SEED_Z_LEN; i++) {
    z[i] = sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN + i];
  }
  RRLWR_KEM_HASH_H(Kb, RRLWR_KEM_SS_LEN, ct, z);
  kem_test_add_cycles(KEM_TEST_KEM_DEC_HASH_CTZ, ts);

  ts = cpucycles();
  uint8_t compare = ct_cmp(ct, ctp);
  for(unsigned int i = 0; i < RRLWR_KEM_SS_LEN; i++) {
    ss[i] = K[i] ^ ((K[i] ^ Kb[i]) & compare);
  }
  kem_test_add_cycles(KEM_TEST_KEM_DEC_COMPARE_SELECT, ts);

  ts = cpucycles();
  *ss_len_bytes = RRLWR_KEM_SS_LEN;
  kem_test_add_cycles(KEM_TEST_KEM_DEC_SET_LENGTHS, ts);

  return 0;
}
