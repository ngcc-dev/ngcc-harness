#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drng.h"
#include "packing.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "reduce.h"
#include "sign.h"

#define SIGN_TESTS 20
#define MSG_LEN 48
#define DRNG_SEED_BYTES 64

DRNG_ctx drng_algorithm;

static void fill_bytes(uint8_t *buf, size_t len, uint8_t base) {
  for(size_t i = 0; i < len; ++i)
    buf[i] = (uint8_t)(base + 37u * i);
}

static int poly_equal(const poly *a, const poly *b) {
  return memcmp(a, b, sizeof(*a)) == 0;
}

static int polyvecl_equal(const polyvecl *a, const polyvecl *b) {
  return memcmp(a, b, sizeof(*a)) == 0;
}

static int polyveck_equal(const polyveck *a, const polyveck *b) {
  return memcmp(a, b, sizeof(*a)) == 0;
}

static unsigned int poly_weight(const poly *a) {
  unsigned int w = 0;

  for(unsigned i = 0; i < N; ++i)
    if(a->coeffs[i] != 0)
      ++w;

  return w;
}

static int check_polyw1_pack(const poly *w1) {
  uint8_t buf[POLYW1_PACKEDBYTES];

  polyw1_pack(buf, w1);

#if GAMMA2 == (Q-1)/32
  for(unsigned i = 0; i < N / 2; ++i) {
    if((buf[i] & 0x0Fu) != (uint8_t)w1->coeffs[2 * i + 0])
      return 1;
    if((buf[i] >> 4) != (uint8_t)w1->coeffs[2 * i + 1])
      return 1;
  }
#elif GAMMA2 == (Q-1)/48
  for(unsigned i = 0; i < N / 8; ++i) {
    if((buf[5*i + 0] & 0x1Fu) != (uint8_t)w1->coeffs[8*i + 0])
      return 1;
    if((((uint32_t)buf[5*i + 0] >> 5) | (((uint32_t)buf[5*i + 1] & 0x03u) << 3)) != (uint8_t)w1->coeffs[8*i + 1])
      return 1;
    if((((uint32_t)buf[5*i + 1] >> 2) & 0x1Fu) != (uint8_t)w1->coeffs[8*i + 2])
      return 1;
    if((((uint32_t)buf[5*i + 1] >> 7) | (((uint32_t)buf[5*i + 2] & 0x0Fu) << 1)) != (uint8_t)w1->coeffs[8*i + 3])
      return 1;
    if((((uint32_t)buf[5*i + 2] >> 4) | (((uint32_t)buf[5*i + 3] & 0x01u) << 4)) != (uint8_t)w1->coeffs[8*i + 4])
      return 1;
    if((((uint32_t)buf[5*i + 3] >> 1) & 0x1Fu) != (uint8_t)w1->coeffs[8*i + 5])
      return 1;
    if((((uint32_t)buf[5*i + 3] >> 6) | (((uint32_t)buf[5*i + 4] & 0x07u) << 2)) != (uint8_t)w1->coeffs[8*i + 6])
      return 1;
    if(((uint32_t)buf[5*i + 4] >> 3) != (uint8_t)w1->coeffs[8*i + 7])
      return 1;
  }
#elif GAMMA2 == (Q-1)/96
  for(unsigned i = 0; i < N / 4; ++i) {
    if((buf[3*i + 0] & 0x3Fu) != (uint8_t)w1->coeffs[4 * i + 0])
      return 1;
    if((((uint32_t)buf[3*i + 0] >> 6) | (((uint32_t)buf[3*i + 1] & 0x0Fu) << 2)) != (uint8_t)w1->coeffs[4 * i + 1])
      return 1;
    if((((uint32_t)buf[3*i + 1] >> 4) | (((uint32_t)buf[3*i + 2] & 0x03u) << 4)) != (uint8_t)w1->coeffs[4 * i + 2])
      return 1;
    if(((uint32_t)buf[3*i + 2] >> 2) != (uint8_t)w1->coeffs[4 * i + 3])
      return 1;
  }
#endif

  return 0;
}

static void make_sample_hint(polyveck *h) {
  memset(h, 0, sizeof(*h));

  h->vec[0].coeffs[0] = 1;
  h->vec[0].coeffs[17] = 1;
  h->vec[0].coeffs[31] = 1;
  h->vec[1].coeffs[9] = 1;
  h->vec[1].coeffs[255] = 1;
  h->vec[1].coeffs[511] = 1;
  h->vec[2].coeffs[1] = 1;
  h->vec[2].coeffs[2] = 1;
  h->vec[2].coeffs[300] = 1;
}

static int test_module_vectors(void) {
  uint8_t rho[SEEDBYTES];
  uint8_t rho2[SEEDBYTES];
  uint8_t tr[CRHBYTES];
  uint8_t cseed[CTILDEBYTES];
  uint8_t seed_crh[CRHBYTES];
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];
  uint8_t sig[CRYPTO_BYTES];
  uint8_t ctmp[CTILDEBYTES];
  poly p;
  poly c;
  polyvecl s1, s1_unpack, z, z_sig;
  polyvecl mat1[K], mat2[K];
  polyveck s2, s2_unpack;
  polyveck w, w1, w0, w1_hint, t1, t0;
  polyveck h, h_unpack;
  polyveck t1_pk;
  polyveck t0_sk;
  uint8_t buf[CRYPTO_SECRETKEYBYTES];

  fill_bytes(rho, sizeof(rho), 3);
  fill_bytes(rho2, sizeof(rho2), 19);
  fill_bytes(tr, sizeof(tr), 29);
  fill_bytes(cseed, sizeof(cseed), 53);
  fill_bytes(seed_crh, sizeof(seed_crh), 67);

  polyvec_matrix_expand(mat1, rho);
  polyvec_matrix_expand(mat2, rho);
  if(memcmp(mat1, mat2, sizeof(mat1)) != 0) {
    fprintf(stderr, "polyvec_matrix_expand is not deterministic\n");
    return 1;
  }

  polyvecl_uniform_eta(&s1, seed_crh, 0);
  polyeta_pack(buf, &s1.vec[0]);
  polyeta_unpack(&p, buf);
  if(!poly_equal(&p, &s1.vec[0])) {
    fprintf(stderr, "polyeta_pack/polyeta_unpack failed\n");
    return 1;
  }
  if(polyvecl_chknorm(&s1, ETA + 1)) {
    fprintf(stderr, "polyvecl_chknorm failed for eta sample\n");
    return 1;
  }

  polyvecl_uniform_gamma1(&z, seed_crh, 5);
  polyz_pack(buf, &z.vec[0]);
  polyz_unpack(&p, buf);
  if(!poly_equal(&p, &z.vec[0])) {
    fprintf(stderr, "polyz_pack/polyz_unpack failed\n");
    return 1;
  }
  if(polyvecl_chknorm(&z, GAMMA1 + 1)) {
    fprintf(stderr, "polyvecl_chknorm failed for gamma1 sample\n");
    return 1;
  }

  for(unsigned i = 0; i < K; ++i)
    poly_uniform(&w.vec[i], rho2, (uint16_t)(100 + i));

  polyveck_power2round(&t1, &t0, &w);
  for(unsigned i = 0; i < K; ++i) {
    for(unsigned j = 0; j < N; ++j) {
      int32_t rebuilt = (int32_t)((t1.vec[i].coeffs[j] << D) + t0.vec[i].coeffs[j]);
      if(rebuilt != freeze(w.vec[i].coeffs[j])) {
        fprintf(stderr, "polyveck_power2round reconstruction failed at [%u][%u]\n", i, j);
        return 1;
      }
    }
  }

  polyt1_pack(buf, &t1.vec[0]);
  polyt1_unpack(&p, buf);
  if(!poly_equal(&p, &t1.vec[0])) {
    fprintf(stderr, "polyt1_pack/polyt1_unpack failed\n");
    return 1;
  }

  polyt0_pack(buf, &t0.vec[0]);
  polyt0_unpack(&p, buf);
  if(!poly_equal(&p, &t0.vec[0])) {
    fprintf(stderr, "polyt0_pack/polyt0_unpack failed\n");
    return 1;
  }

  if(polyveck_chknorm(&t0, (1 << (D - 1)) + 1)) {
    fprintf(stderr, "polyveck_chknorm failed for t0\n");
    return 1;
  }

  polyveck_decompose(&w1, &w0, &w);
  for(unsigned i = 0; i < K; ++i) {
    for(unsigned j = 0; j < N; ++j) {
      int32_t rebuilt = w1.vec[i].coeffs[j] * 2 * GAMMA2 + w0.vec[i].coeffs[j];
      if(rebuilt < 0)
        rebuilt += Q;
      if(rebuilt != freeze(w.vec[i].coeffs[j])) {
        fprintf(stderr, "polyveck_decompose reconstruction failed at [%u][%u]\n", i, j);
        return 1;
      }
    }
  }

  if(check_polyw1_pack(&w1.vec[0])) {
    fprintf(stderr, "polyw1_pack failed\n");
    return 1;
  }

  polyveck_make_hint(&h, &w0, &w1);
  polyveck_use_hint(&w1_hint, &w, &h);
  if(!polyveck_equal(&w1_hint, &w1)) {
    fprintf(stderr, "polyveck_make_hint/polyveck_use_hint failed\n");
    return 1;
  }

  poly_challenge(&c, cseed);
  if(poly_weight(&c) != TAU) {
    fprintf(stderr, "poly_challenge produced wrong Hamming weight\n");
    return 1;
  }
  for(unsigned i = 0; i < N; ++i) {
    if(c.coeffs[i] != 0 && c.coeffs[i] != 1 && c.coeffs[i] != -1) {
      fprintf(stderr, "poly_challenge produced invalid coefficient\n");
      return 1;
    }
  }

  pack_pk(pk, rho, &t1);
  unpack_pk(rho2, &t1_pk, pk);
  if(memcmp(rho, rho2, SEEDBYTES) != 0 || !polyveck_equal(&t1, &t1_pk)) {
    fprintf(stderr, "pack_pk/unpack_pk failed\n");
    return 1;
  }

  polyveck_uniform_eta(&s2, seed_crh, 50);
  pack_sk(sk, rho, tr, &t0, &s1, &s2);
  unpack_sk(rho2, tr, &t0_sk, &s1_unpack, &s2_unpack, sk);
  if(memcmp(rho, rho2, SEEDBYTES) != 0 ||
     !polyveck_equal(&t0, &t0_sk) ||
     !polyvecl_equal(&s1, &s1_unpack) ||
     !polyveck_equal(&s2, &s2_unpack)) {
    fprintf(stderr, "pack_sk/unpack_sk failed\n");
    return 1;
  }

  make_sample_hint(&h);
  if(poly_weight(&h.vec[0]) + poly_weight(&h.vec[1]) + poly_weight(&h.vec[2]) > OMEGA) {
    fprintf(stderr, "sample hint exceeds OMEGA\n");
    return 1;
  }
  pack_sig(sig, cseed, &z, &h);
  if(unpack_sig(ctmp, &z_sig, &h_unpack, sig)) {
    fprintf(stderr, "unpack_sig rejected packed signature\n");
    return 1;
  }
  if(memcmp(ctmp, cseed, CTILDEBYTES) != 0 ||
     !polyvecl_equal(&z, &z_sig) ||
     !polyveck_equal(&h, &h_unpack)) {
    fprintf(stderr, "pack_sig/unpack_sig failed\n");
    return 1;
  }

  printf("module vectors OK\n");
  return 0;
}

static int test_signing(void) {
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];
  uint8_t sig[CRYPTO_BYTES];
  uint8_t sig_bad[CRYPTO_BYTES];
  uint8_t msg[MSG_LEN];
  uint8_t msg_bad[MSG_LEN];
  uint8_t drng_seed[DRNG_SEED_BYTES];
  const uint8_t ctx[] = "test_vectors_260602";
  size_t siglen = 0;

  for(unsigned int i = 0; i < SIGN_TESTS; ++i) {
    fill_bytes(drng_seed, sizeof(drng_seed), (uint8_t)(11 + 9 * i));
    fill_bytes(msg, sizeof(msg), (uint8_t)(23 + 7 * i));
    memcpy(msg_bad, msg, sizeof(msg));
    msg_bad[0] ^= 0x80;

    if(init_random_number(&drng_algorithm, drng_seed, sizeof(drng_seed)) != 0) {
      fprintf(stderr, "init_random_number failed\n");
      return 1;
    }

    if(crypto_sign_keypair(pk, sk) != 0) {
      fprintf(stderr, "crypto_sign_keypair failed\n");
      return 1;
    }
    if(crypto_sign_signature(sig, &siglen, msg, sizeof(msg),
                             ctx, sizeof(ctx) - 1, sk) != 0) {
      fprintf(stderr, "crypto_sign_signature failed\n");
      return 1;
    }
    if(siglen != CRYPTO_BYTES) {
      fprintf(stderr, "unexpected signature length\n");
      return 1;
    }
    if(crypto_sign_verify(sig, siglen, msg, sizeof(msg),
                          ctx, sizeof(ctx) - 1, pk) != 0) {
      fprintf(stderr, "crypto_sign_verify failed on valid signature\n");
      return 1;
    }

    memcpy(sig_bad, sig, sizeof(sig));
    sig_bad[(7 * i + 3) % CRYPTO_BYTES] ^= 0x5A;
    if(crypto_sign_verify(sig_bad, siglen, msg, sizeof(msg),
                          ctx, sizeof(ctx) - 1, pk) == 0) {
      fprintf(stderr, "tampered signature verified\n");
      return 1;
    }
    if(crypto_sign_verify(sig, siglen, msg_bad, sizeof(msg_bad),
                          ctx, sizeof(ctx) - 1, pk) == 0) {
      fprintf(stderr, "signature verified for wrong message\n");
      return 1;
    }
  }

  printf("sign/verify OK (%d tests)\n", SIGN_TESTS);
  return 0;
}

int main(void) {
  if(test_module_vectors())
    return 1;
  if(test_signing())
    return 1;

  printf("test_vectors-260602 OK\n");
  return 0;
}
