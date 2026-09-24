#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auxfunc.h"
#include "drng.h"
#include "kem.h"
#include "params.h"
#include "symmetric.h"

DRNG_ctx drng_algorithm;

static void fill_bytes(uint8_t *buf, size_t len, uint8_t seed)
{
  size_t i;

  for(i = 0; i < len; i++)
    buf[i] = (uint8_t)(seed + 17u * (uint8_t)i + (uint8_t)(i >> 1));
}

static void die_mismatch(const char *name, size_t offset, uint8_t got, uint8_t want)
{
  fprintf(stderr, "%s mismatch at %zu: got %02x want %02x\n", name, offset, got, want);
  exit(1);
}

static void assert_equal(const char *name, const uint8_t *got, const uint8_t *want, size_t len)
{
  size_t i;

  for(i = 0; i < len; i++) {
    if(got[i] != want[i])
      die_mismatch(name, i, got[i], want[i]);
  }
}

static void test_pseudoxof_vectors(void)
{
  static const size_t inlens[] = {0, 1, 16, 17, 32, 33, 34, 64, 96, KYBER_SYMBYTES + 2};
  static const size_t outlens[] = {0, 1, 31, 32, 33, 64, XOF_BLOCKBYTES, 2 * XOF_BLOCKBYTES, 3 * XOF_BLOCKBYTES};
  uint8_t in[128];
  uint8_t ref[3 * XOF_BLOCKBYTES];
  uint8_t got[3 * XOF_BLOCKBYTES];
  size_t i, j;

  fill_bytes(in, sizeof(in), 0x10);
  for(i = 0; i < sizeof(inlens) / sizeof(inlens[0]); i++) {
    for(j = 0; j < sizeof(outlens) / sizeof(outlens[0]); j++) {
      memset(ref, 0, sizeof(ref));
      memset(got, 0, sizeof(got));
      if(pseudoXOF((unsigned long long)outlens[j] * 8ULL,
                   in,
                   (unsigned long long)inlens[i] * 8ULL,
                   ref) != 0) {
        fprintf(stderr, "pseudoXOF reference failed\n");
        exit(1);
      }
      if(iccs_pseudoxof_bytes_scalar(got, outlens[j], in, inlens[i]) != 0) {
        fprintf(stderr, "iccs_pseudoxof_bytes_scalar failed\n");
        exit(1);
      }
      assert_equal("pseudoXOF", got, ref, outlens[j]);
    }
  }
}

static void test_xof_squeeze(void)
{
  uint8_t seed[KYBER_SYMBYTES];
  uint8_t extseed[KYBER_SYMBYTES + 2];
  uint8_t ref[3 * XOF_BLOCKBYTES];
  uint8_t one_shot[3 * XOF_BLOCKBYTES];
  uint8_t split[3 * XOF_BLOCKBYTES];
  xof_state state;

  fill_bytes(seed, sizeof(seed), 0x23);
  memcpy(extseed, seed, KYBER_SYMBYTES);
  extseed[KYBER_SYMBYTES] = 3;
  extseed[KYBER_SYMBYTES + 1] = 7;

  if(pseudoXOF((unsigned long long)sizeof(ref) * 8ULL,
               extseed,
               (unsigned long long)sizeof(extseed) * 8ULL,
               ref) != 0) {
    fprintf(stderr, "pseudoXOF xof reference failed\n");
    exit(1);
  }

  xof_absorb(&state, seed, 3, 7);
  xof_squeezeblocks(one_shot, 3, &state);
  assert_equal("xof one-shot", one_shot, ref, sizeof(ref));

  xof_absorb(&state, seed, 3, 7);
  xof_squeezeblocks(split, 1, &state);
  xof_squeezeblocks(split + XOF_BLOCKBYTES, 1, &state);
  xof_squeezeblocks(split + 2 * XOF_BLOCKBYTES, 1, &state);
  assert_equal("xof split", split, ref, sizeof(ref));
}

static void test_prf(void)
{
  uint8_t key[KYBER_SYMBYTES];
  uint8_t extkey[KYBER_SYMBYTES + 1];
  uint8_t ref[KYBER_ETA1 * KYBER_N / 4];
  uint8_t got[KYBER_ETA1 * KYBER_N / 4];
  unsigned int nonce;

  fill_bytes(key, sizeof(key), 0x34);
  memcpy(extkey, key, KYBER_SYMBYTES);
  for(nonce = 0; nonce < 16; nonce++) {
    extkey[KYBER_SYMBYTES] = (uint8_t)nonce;
    if(pseudoXOF((unsigned long long)sizeof(ref) * 8ULL,
                 extkey,
                 (unsigned long long)sizeof(extkey) * 8ULL,
                 ref) != 0) {
      fprintf(stderr, "pseudoXOF prf reference failed\n");
      exit(1);
    }
    prf(got, sizeof(got), key, (uint8_t)nonce);
    assert_equal("prf eta1", got, ref, sizeof(ref));

    if(pseudoXOF((unsigned long long)(KYBER_ETA2 * KYBER_N / 4) * 8ULL,
                 extkey,
                 (unsigned long long)sizeof(extkey) * 8ULL,
                 ref) != 0) {
      fprintf(stderr, "pseudoXOF prf eta2 reference failed\n");
      exit(1);
    }
    prf(got, KYBER_ETA2 * KYBER_N / 4, key, (uint8_t)nonce);
    assert_equal("prf eta2", got, ref, KYBER_ETA2 * KYBER_N / 4);
  }
}

static void test_rkprf(void)
{
  uint8_t key[KYBER_SYMBYTES];
  uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
  uint8_t input[KYBER_SYMBYTES + CRYPTO_CIPHERTEXTBYTES];
  uint8_t ref[KYBER_SSBYTES];
  uint8_t got[KYBER_SSBYTES];

  fill_bytes(key, sizeof(key), 0x45);
  fill_bytes(ct, sizeof(ct), 0x56);
  memcpy(input, key, KYBER_SYMBYTES);
  memcpy(input + KYBER_SYMBYTES, ct, CRYPTO_CIPHERTEXTBYTES);

  if(pseudoXOF((unsigned long long)sizeof(ref) * 8ULL,
               input,
               (unsigned long long)sizeof(input) * 8ULL,
               ref) != 0) {
    fprintf(stderr, "pseudoXOF rkprf reference failed\n");
    exit(1);
  }
  rkprf(got, key, ct);
  assert_equal("rkprf", got, ref, sizeof(ref));
}

static void test_hashes(void)
{
  uint8_t in[97];
  uint8_t ref_h[32];
  uint8_t got_h[32];
  uint8_t ref_g[64];
  uint8_t got_g[64];

  fill_bytes(in, sizeof(in), 0x67);
  if(sm3hash(256, in, (unsigned long long)sizeof(in) * 8ULL, ref_h) != 0) {
    fprintf(stderr, "sm3hash reference failed\n");
    exit(1);
  }
  hash_h(got_h, in, sizeof(in));
  assert_equal("hash_h", got_h, ref_h, sizeof(ref_h));

  if(pseudohash(512, in, (unsigned long long)sizeof(in) * 8ULL, ref_g) != 0) {
    fprintf(stderr, "pseudohash reference failed\n");
    exit(1);
  }
  hash_g(got_g, in, sizeof(in));
  assert_equal("hash_g", got_g, ref_g, sizeof(ref_g));
}

int main(void)
{
  test_pseudoxof_vectors();
  test_xof_squeeze();
  test_prf();
  test_rkprf();
  test_hashes();
  puts("test_iccs: ok");
  return 0;
}
