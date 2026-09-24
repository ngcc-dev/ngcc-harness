#include "pke_test.h"
#include "packing.h"
#include "test/cpucycles.h"

extern int32_t rrlwr_pke_zetas[RRLWR_N];

static void unpack_Awin_base_ncoeffs(ring_element_Awin *aw,
                                     const unsigned char *b,
                                     int32_t bitlen)
{
  unsigned int offset = bitlen * (RRLWR_N >> 3);

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_unpack(&aw->x[RRLWR_K - 1 - i], b + i * offset, bitlen);
  }
}

static void to_Awin_base(ring_element_Awin *aw, ring_element *a)
{
  for(int i = 0; i < RRLWR_K; i++) {
    aw->x[RRLWR_K - 1 - i] = a->x[i];
  }
}

int pke_keygen_test(unsigned char pk[RRLWR_PKE_PK_LEN],
                    unsigned char sk[RRLWR_PKE_SK_LEN],
                    const unsigned char seedA[RRLWR_PKE_SEED_A_LEN],
                    const unsigned char seedS[RRLWR_SEED_S_LEN])
{
  uint64_t ts, ta;
  ring_element s __attribute__((aligned(32)));
  ring_element b __attribute__((aligned(32)));
  ring_element_Awin a __attribute__((aligned(32)));

  ts = cpucycles();
  ta = cpucycles();
  ring_uniform_Awin_base(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN);
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_A_UNIFORM_BASE, ta);

  ta = cpucycles();
  ring_Awin_ntt_prepare(&a, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                        RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                        rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_A_PREPARE, ta);
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_A_UNIFORM, ts);

  ts = cpucycles();
  ring_uniform(&s, RRLWR_PKE_LOG_ETA+1, seedS, RRLWR_SEED_S_LEN);
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_S_UNIFORM, ts);

  ts = cpucycles();
  ring_pack(sk, &s, RRLWR_PKE_LOG_ETA+1);
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_PACK_S, ts);

  ts = cpucycles();
  ring_mul_Awin_round_xtoy_32(b.x, &a, &s, RRLWR_K,
                              RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                              RRLWR_NTTINV_FINALCONST,
                              RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                              rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_MUL_AS_ROUND, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_PKE_SEED_A_LEN; i++) {
    pk[i] = seedA[i];
  }
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_COPY_SEED_A, ts);

  ts = cpucycles();
  ring_pack(pk + RRLWR_PKE_SEED_A_LEN, &b, RRLWR_PKE_LOGP);
  kem_test_add_cycles(KEM_TEST_PKE_KEYGEN_PACK_B, ts);

  return 0;
}

int pke_encrypt_test(unsigned char ct[RRLWR_PKE_CT_LEN],
                     const unsigned char pk[RRLWR_PKE_PK_LEN],
                     const unsigned char m[RRLWR_PKE_MESSAGE_LEN],
                     const unsigned char seedSp[RRLWR_SEED_S_LEN])
{
  uint64_t ts, ta;
  ring_element sp __attribute__((aligned(32)));
  ring_element bp __attribute__((aligned(32)));
  ring_element_Awin a __attribute__((aligned(32)));
  ring_element_Awin b __attribute__((aligned(32)));
  poly vp[RRLWR_PKE_ELL];
  const unsigned char *seedA = &pk[0];

  ts = cpucycles();
  ta = cpucycles();
  ring_uniform_Awin_base(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_A_UNIFORM_BASE, ta);

  ta = cpucycles();
  ring_Awin_ntt_prepare(&a, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                        RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                        rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_A_PREPARE, ta);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_A_UNIFORM, ts);

  ts = cpucycles();
  ring_uniform(&sp, RRLWR_PKE_LOG_ETA+1, seedSp, RRLWR_SEED_S_LEN);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_SP_UNIFORM, ts);

  ts = cpucycles();
  ring_mul_Awin_round_xtoy_32(bp.x, &a, &sp, RRLWR_K,
                              RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                              RRLWR_NTTINV_FINALCONST,
                              RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                              rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_MUL_ASP_ROUND, ts);

  ts = cpucycles();
  ta = cpucycles();
  unpack_Awin_base_ncoeffs(&b, pk + RRLWR_PKE_SEED_A_LEN, RRLWR_PKE_LOGP);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_UNPACK_B_BASE, ta);

  ta = cpucycles();
  ring_Awin_ntt_prepare_ncoeffs(&b, RRLWR_PKE_ELL,
                                RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                                rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_UNPACK_B_PREPARE, ta);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_UNPACK_B, ts);

  ts = cpucycles();
  ring_mul_Awin_invntt_reduce_pow2_32(vp, &b, &sp, RRLWR_PKE_ELL,
                                      RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                      RRLWR_NTTINV_FINALCONST,
                                      RRLWR_PKE_LOGP, rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_MUL_BSP_REDUCE, ts);

  ts = cpucycles();
  poly_add_msg(vp, m);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_ADD_MSG, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_compress(&vp[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT);
    poly_pack(ct + i*RRLWR_PKE_PACKED_POLYT_LEN, &vp[i], RRLWR_PKE_LOGT);
  }
  kem_test_add_cycles(KEM_TEST_PKE_ENC_PACK_CM, ts);

  ts = cpucycles();
  ring_pack(ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, &bp, RRLWR_PKE_LOGP);
  kem_test_add_cycles(KEM_TEST_PKE_ENC_PACK_BP, ts);

  return 0;
}

int pke_decrypt_test(unsigned char m[RRLWR_PKE_MESSAGE_LEN],
                     const unsigned char ct[RRLWR_PKE_CT_LEN],
                     const unsigned char sk[RRLWR_PKE_SK_LEN])
{
  uint64_t ts, ta;
  ring_element bp __attribute__((aligned(32)));
  ring_element s __attribute__((aligned(32)));
  ring_element_Awin bp_aw __attribute__((aligned(32)));
  poly v[RRLWR_PKE_ELL] __attribute__((aligned(32)));
  poly cm[RRLWR_PKE_ELL] __attribute__((aligned(32)));

  ts = cpucycles();
  ring_unpack(&s, sk, RRLWR_PKE_LOG_ETA+1);
  kem_test_add_cycles(KEM_TEST_PKE_DEC_UNPACK_S, ts);

  ts = cpucycles();
  ring_unpack(&bp, ct + RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGP);
  kem_test_add_cycles(KEM_TEST_PKE_DEC_UNPACK_BP, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_unpack(&cm[i], ct + i*RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGT);
    poly_decompress(&cm[i], RRLWR_PKE_LOGP - RRLWR_PKE_LOGT);
  }
  kem_test_add_cycles(KEM_TEST_PKE_DEC_UNPACK_CM, ts);

  ts = cpucycles();
  ta = cpucycles();
  to_Awin_base(&bp_aw, &bp);
  kem_test_add_cycles(KEM_TEST_PKE_DEC_BP_TO_AWIN_BASE, ta);

  ta = cpucycles();
  ring_Awin_ntt_prepare_ncoeffs(&bp_aw, RRLWR_PKE_ELL,
                                RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                                rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_DEC_BP_TO_AWIN_PREPARE, ta);
  kem_test_add_cycles(KEM_TEST_PKE_DEC_BP_TO_AWIN, ts);

  ts = cpucycles();
  ring_mul_Awin_reduce_pow2_32(v, &bp_aw, &s, RRLWR_PKE_ELL,
                               RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                               RRLWR_NTTINV_FINALCONST, RRLWR_PKE_LOGP,
                               rrlwr_pke_zetas);
  kem_test_add_cycles(KEM_TEST_PKE_DEC_MUL_BPS_REDUCE, ts);

  ts = cpucycles();
  for(unsigned int i = 0; i < RRLWR_PKE_ELL; i++) {
    poly_subp(&v[i], &v[i], &cm[i]);
    poly_round_xtoy(&v[i], &v[i], RRLWR_PKE_LOGP, 1);
    poly_pack(m + i*RRLWR_PKE_PACKED_POLY1_LEN, &v[i], 1);
  }
  kem_test_add_cycles(KEM_TEST_PKE_DEC_SUB_ROUND_PACK, ts);

  return 0;
}
