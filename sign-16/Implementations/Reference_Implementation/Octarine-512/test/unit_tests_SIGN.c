#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "drng.h"
#include "crt.h"
#include "poly.h"
#include "sign.h"

#define RNG_SEED_LENGTH 32
DRNG_ctx drng_algorithm;

#define NUMBER_OF_TESTS 100
#define TEST_PRIMES {RRLWR_SIGN_PRIME1} // Random 28-bit primes for test purposes only
#define TEST_PRIMES_INV {RRLWR_SIGN_PRIME1INV} // -p^-1 mod 2^32
#define NUMBER_OF_TEST_PRIMES 1

static int32_t generate_random_int32(DRNG_ctx *drng) {
  unsigned char x[4];
  get_random_number(drng, x, 32);
  return x[0] | (x[1] << 8) | (x[2] << 16) | (x[3] << 24);
}

static int test_crt() {

  int32_t t0, t1;
  int64_t y1, y2, y, z;
  int64_t p1p2 = (int64_t)RRLWR_SIGN_PRIME1 * (int64_t)RRLWR_SIGN_PRIME2;

  t0 = generate_random_int32(&drng_algorithm);
  t1 = generate_random_int32(&drng_algorithm);
  y = ((int64_t)t0 & (((int64_t)1<<32)-1)) | ((int64_t)t1 << 32);
  y %= p1p2; // Random value mod p1*p2
  if (y > ((p1p2-1) >> 1)) y -= p1p2; // Reduce to unique signed representation
  if (y <= -((p1p2-1) >> 1) + 1) y += p1p2;
  y1 = y % (int64_t)RRLWR_SIGN_PRIME1; // Reduce mod p1
  y2 = y % (int64_t)RRLWR_SIGN_PRIME2; // Reduce mod p2
  z = crt(y1, y2);

  assert(y <= (p1p2-1)/2);
  assert(y >= -(p1p2-1)/2+1);
  assert(z <= (p1p2-1)/2);
  assert(z >= -(p1p2-1)/2+1);
  assert(z == y);

  return 0;
}

// Schoolbook polynomial multiplication
static void schoolbook_mul(poly *h, poly *f, poly *g, int32_t prime) {

    poly fc, gc;

    for(unsigned int j = 0; j < RRLWR_N; j++) {
      fc.coeffs[j] = f->coeffs[j]; // To make sure h = f and h = g are supported
      gc.coeffs[j] = g->coeffs[j]; // To make sure h = f and h = g are supported
      h->coeffs[j] = 0;
    }

    for(unsigned int j = 0; j < RRLWR_N; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        int32_t t = ((int64_t)fc.coeffs[j] * gc.coeffs[k]) % prime;
        if (j+k >= RRLWR_N) {
          t = -t;
        }
        h->coeffs[(j+k) % RRLWR_N] = (h->coeffs[(j+k) % RRLWR_N] + t) % prime;
      }
    }
}

static int test_poly_mul32() {

  poly f, g, hp, hs;
  int32_t prime, primeinv;

  // Test for prime 1
  prime = RRLWR_SIGN_PRIME1;
  primeinv = RRLWR_SIGN_PRIME1INV;

  for(unsigned int j = 0; j < RRLWR_N; j++) {
    f.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
    g.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
  }

  schoolbook_mul(&hs, &f, &g, prime);

  poly_ntt32(&f, prime, primeinv, rrlwr_sign_zetas1);          // No Montgomery factor in input/output, only in zetas
  poly_ntt32(&g, prime, primeinv, rrlwr_sign_zetas1);          // No Montgomery factor in input/output, only in zetas
  poly_basemul32(&hp, &f, &g, prime, primeinv); // Includes Montgomery factor R^-1
  poly_invntt32(&hp, prime, primeinv, RRLWR_SIGN_NTTINV_FINALCONST1, rrlwr_sign_zetas1);      // Removes Montgomery factor from outputwith final multiplication

  // Convert to a unique representation in [0, p-1] for comparison
  for(unsigned j = 0; j < RRLWR_N; j++) {
    hs.coeffs[j] = ((hs.coeffs[j] % prime) + prime) % prime;
    hp.coeffs[j] = ((hp.coeffs[j] % prime) + prime) % prime;
  }

  // Check that f*g = hs = hp = invntt(ntt(f)*ntt(g))
  assert(!memcmp(hs.coeffs, hp.coeffs, RRLWR_N << 2));

  // Test for prime 2
  prime = RRLWR_SIGN_PRIME2;
  primeinv = RRLWR_SIGN_PRIME2INV;

  for(unsigned int j = 0; j < RRLWR_N; j++) {
    f.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
    g.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
  }

  schoolbook_mul(&hs, &f, &g, prime);

  poly_ntt32(&f, prime, primeinv, rrlwr_sign_zetas2);          // No Montgomery factor in input/output, only in zetas
  poly_ntt32(&g, prime, primeinv, rrlwr_sign_zetas2);          // No Montgomery factor in input/output, only in zetas
  poly_basemul32(&hp, &f, &g, prime, primeinv); // Includes Montgomery factor R^-1
  poly_invntt32(&hp, prime, primeinv, RRLWR_SIGN_NTTINV_FINALCONST2, rrlwr_sign_zetas2);      // Removes Montgomery factor from outputwith final multiplication

  // Convert to a unique representation in [0, p-1] for comparison
  for(unsigned j = 0; j < RRLWR_N; j++) {
    hs.coeffs[j] = ((hs.coeffs[j] % prime) + prime) % prime;
    hp.coeffs[j] = ((hp.coeffs[j] % prime) + prime) % prime;
  }

  // Check that f*g = hs = hp = invntt(ntt(f)*ntt(g))
  assert(!memcmp(hs.coeffs, hp.coeffs, RRLWR_N << 2));

  return 0;
}

static void schoolbook_ring_mul(ring_element *h, ring_element *a, ring_element *s, int32_t prime) {
    poly t, yp2;

    // Set up polynomial (y+2)
    for(unsigned j = 0; j < RRLWR_N; j++) {
      yp2.coeffs[j] = 0;
    }
    yp2.coeffs[0] = 2;
    yp2.coeffs[1] = 1;

    for(unsigned j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        h->x[j].coeffs[k] = 0;
      }
      for(unsigned int k = 0; k < RRLWR_K; k++) {
        schoolbook_mul(&t, &a->x[(j-k+RRLWR_K) % RRLWR_K], &s->x[k], prime);
        if (k > j) {
          schoolbook_mul(&t, &t, &yp2, prime); // Multiply with (y+2) in upper triangle
        }
        for(unsigned int l = 0; l < RRLWR_N; l++) {
          h->x[j].coeffs[l] = (h->x[j].coeffs[l] + t.coeffs[l]) % prime;
        }
      }
    }
}

static int test_ring_mul32() {

  ring_element r, a, s;
  ring_element rc, ac, sc;
  int32_t modulus;

  modulus = RRLWR_SIGN_Q;

  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      a.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % RRLWR_SIGN_Q;
      if(a.x[j].coeffs[k] < -(RRLWR_SIGN_Q >> 1)) a.x[j].coeffs[k] += RRLWR_SIGN_Q;
      if(a.x[j].coeffs[k] >= (RRLWR_SIGN_Q >> 1)) a.x[j].coeffs[k] -= RRLWR_SIGN_Q;
      assert(a.x[j].coeffs[k] >= -(RRLWR_SIGN_Q>>1));
      assert(a.x[j].coeffs[k] < RRLWR_SIGN_Q>>1);
      s.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % RRLWR_SIGN_GAMMA1;
      if(s.x[j].coeffs[k] < -RRLWR_SIGN_GAMMA1) s.x[j].coeffs[k] += RRLWR_SIGN_GAMMA1;
      if(s.x[j].coeffs[k] >= RRLWR_SIGN_GAMMA1) s.x[j].coeffs[k] -= RRLWR_SIGN_GAMMA1;
      assert(s.x[j].coeffs[k] >= -(RRLWR_SIGN_GAMMA1));
      assert(s.x[j].coeffs[k] < RRLWR_SIGN_GAMMA1);
      ac.x[j].coeffs[k] = a.x[j].coeffs[k];
      sc.x[j].coeffs[k] = s.x[j].coeffs[k];
      r.x[j].coeffs[k] = 1;
      rc.x[j].coeffs[k] = 0;
    }
  }

  // Compare against schoolbook multiplication
  ring_element h;
  schoolbook_ring_mul(&h, &a, &s, modulus);

  // Sanity check: multiplication is commutative
  ring_mul32x2(&r, &a, &s);
  ring_mul32x2(&rc, &sc, &ac);

  assert(!memcmp(&r, &rc, RRLWR_K*(RRLWR_N << 2)));

  // Check that range of output is in [-(p-1)/2, (p-1)/2-1]
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      assert(r.x[j].coeffs[k] >= -(RRLWR_SIGN_Q>>1));
      assert(r.x[j].coeffs[k] < RRLWR_SIGN_Q>>1);
    }
  }

  // Convert to a unique representation in [0, p-1] for comparison
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      h.x[j].coeffs[k] = ((h.x[j].coeffs[k] % modulus) + modulus) % modulus;
      r.x[j].coeffs[k] = ((r.x[j].coeffs[k] % modulus) + modulus) % modulus;
      if (h.x[j].coeffs[k] != r.x[j].coeffs[k]) {
        printf("%d %d %d %d\n", j, k, h.x[j].coeffs[k], r.x[j].coeffs[k]);
      }
    }
  }

  assert(!memcmp(&r, &h, RRLWR_K*(RRLWR_N << 2)));

  return 0;
}

static int test_packing() {
  unsigned char bufferd[RRLWR_K*32*(RRLWR_N>>3)]; // Max buffer size
  ring_element r, rc1, rc2;
  int32_t q;

  for(unsigned int d = 1; d < 25; d++) {

    // Generate random ring elements mod q = 2^d
    q = (int32_t)1 << d;
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        r.x[j].coeffs[k] = ((generate_random_int32(&drng_algorithm) % q) + q) % q; // Make sure it is in [0, 2**d-1]
        r.x[j].coeffs[k] = (q/2)-1-r.x[j].coeffs[k]; // Make signed in [-2**d/2, 2**d/2-1]
        rc1.x[j].coeffs[k] = r.x[j].coeffs[k]; // Store a copy
      }
    }

    ring_pack(bufferd, &r, d);
    ring_unpack(&rc2, bufferd, d);

    assert(!memcmp(&rc1, &rc2, RRLWR_K*(RRLWR_N << 2)));
  }

  return 0;
}

static int test_poly_reduce_pow2() {
  poly r, f;
  int32_t q;

  for(unsigned int d = 1; d < 25; d++) {

    q = (int32_t)1 << d;
    for(unsigned int i = 0; i < RRLWR_N; i++) {
      f.coeffs[i] = generate_random_int32(&drng_algorithm);
    }

    poly_reduce_pow2(&r, &f, d);

    // Check that the output is in the right range
    for(unsigned int i = 0; i < RRLWR_N; i++) {
        assert((((r.coeffs[i] % q) + q) % q) == (((f.coeffs[i] % q) + q) % q));
        assert(r.coeffs[i] >= -(q >> 1));
        assert(r.coeffs[i] <= (q >> 1)-1);
    }
  }

  return 0;
}

static int test_uniform() {
  ring_element r;
  unsigned char seed[24*RRLWR_K*(RRLWR_N>>3)];
  int32_t seed_len = 64;
  int32_t q;

  for(unsigned int d = 1; d < 25; d++) {

    // Generate random ring elements mod q = 2^d
    q = (int32_t)1 << d;
    get_random_number(&drng_algorithm, seed, 8*seed_len);
    ring_uniform_secret(&r, d, seed, seed_len);

    // Check that the output is in the right range
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        assert(r.x[j].coeffs[k] >= -(q >> 1));
        assert(r.x[j].coeffs[k] <= (q >> 1)-1);
      }
    }
  }

  return 0;
}


static int test_uniform_invalid_seed_len() {
  ring_element r;
  unsigned char seed[1] = {0};

  memset(&r, 0xA5, sizeof(r));
  ring_uniform_public_x4(&r, RRLWR_SIGN_LOGQ, seed, -1);
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      assert(r.x[j].coeffs[k] == 0);
    }
  }

  memset(&r, 0xA5, sizeof(r));
  ring_uniform_secret_x4(&r, RRLWR_SIGN_LOGQ, seed, -1);
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      assert(r.x[j].coeffs[k] == 0);
    }
  }

  memset(&r, 0xA5, sizeof(r));
  ring_uniform_secret(&r, RRLWR_SIGN_LOGQ, seed, -1);
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      assert(r.x[j].coeffs[k] == 0);
    }
  }

  return 0;
}

static int test_w1() {
  ring_element r, w;
  poly r0, r1;
  unsigned char seed[24*RRLWR_K*(RRLWR_N>>3)];
  unsigned char w1[RRLWR_SIGN_PACKED_W1_LEN];
  int32_t seed_len = 64;
  int32_t q;

  q = (int32_t)1 << (RRLWR_SIGN_LOGQ - (RRLWR_SIGN_LOG_GAMMA2+1));
  get_random_number(&drng_algorithm, seed, 8*seed_len);
  ring_uniform_secret(&r, RRLWR_SIGN_LOGQ, seed, seed_len);
  ring_w1(w1, &r);
  ring_unpack(&w, w1, RRLWR_SIGN_LOGQ-(RRLWR_SIGN_LOG_GAMMA2+1));

  // Check that the output is in the right range
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    poly_power2round(&r0, &r1, &r.x[j], RRLWR_SIGN_LOG_GAMMA2+1);
    poly_reduce_pow2(&r0, &r0, RRLWR_SIGN_LOG_GAMMA2+1);
    poly_reduce_pow2(&r1, &r1, RRLWR_SIGN_LOGQ-(RRLWR_SIGN_LOG_GAMMA2+1));
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      assert(w.x[j].coeffs[k] >= -(q >> 1));
      assert(w.x[j].coeffs[k] <= (q >> 1)-1);
      assert(w.x[j].coeffs[k] == r1.coeffs[k]);
      assert(r.x[j].coeffs[k] >= -(RRLWR_SIGN_Q >> 1));
      assert(r.x[j].coeffs[k] <= (RRLWR_SIGN_Q >> 1)-1);
      // Check correctness modulo q
      assert((r.x[j].coeffs[k] & (RRLWR_SIGN_Q-1)) == ((r0.coeffs[k] + (r1.coeffs[k] << (RRLWR_SIGN_LOG_GAMMA2+1))) & (RRLWR_SIGN_Q-1)));
    }
  }

  return 0;
}

static int test_power2round() {
  ring_element r, r0, r1;
  int32_t seed_len = 64;
  unsigned char seed[64];

  get_random_number(&drng_algorithm, seed, 8*seed_len);
  ring_uniform_secret(&r, RRLWR_SIGN_LOGP, seed, seed_len);

  // Check that the output is in the right range
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    poly_power2round(&r0.x[j], &r1.x[j], &r.x[j], RRLWR_SIGN_D);
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      assert((r.x[j].coeffs[k] & (RRLWR_SIGN_P-1)) == ((r0.x[j].coeffs[k] + (r1.x[j].coeffs[k] << RRLWR_SIGN_D)) & (RRLWR_SIGN_P-1)));
    }
  }

  return 0;
}

static int test_sampleInBall() {
  poly c;
  unsigned char ctilde[RRLWR_SIGN_CTILDE_LEN];

  get_random_number(&drng_algorithm, ctilde, 8*RRLWR_SIGN_CTILDE_LEN);
  poly_sampleInBall(&c, ctilde);

  int32_t ctr = 0;
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    if(c.coeffs[i] != 0) {
      assert((c.coeffs[i] == 1) || (c.coeffs[i] == -1)); // Assert that non-zero coefficients are +1 or -1
      ctr++;
    }
  }
  assert(ctr == RRLWR_SIGN_TAU); // Assert that there are exactly tau non-zero coefficients

  return 0;
}

static int test_pack_hint() {
  ring_element h0, h1;
  unsigned char hints[RRLWR_SIGN_H_LEN8];

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      h0.x[i].coeffs[j] = 0;
      h1.x[i].coeffs[j] = 0;
    }
  }

  // Arbitrary hint vector locations
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    h0.x[i].coeffs[0] = 1;
    h0.x[i].coeffs[RRLWR_N>>2] = 1;
    h0.x[i].coeffs[RRLWR_N>>1] = 1;
  }

  ring_pack_hint(hints, &h0);
  ring_unpack_hint(&h1, hints);

  assert(!memcmp(h0.x, h1.x, RRLWR_K*(RRLWR_N << 2)));

  return 0;
}

int main() {
  /* Initialize RNG*/
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_crt();
  }
  printf("crt tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_poly_mul32();
  }
  printf("poly_mul32 tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_ring_mul32();
  }
  printf("ring_mul32 tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_packing();
  }
  printf("packing tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_uniform();
  }
  printf("uniform tests complete\n");


  test_uniform_invalid_seed_len();
  printf("uniform invalid seed_len tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_w1();
  }
  printf("w1 tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_sampleInBall();
  }
  printf("sampleInBall tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_poly_reduce_pow2();
  }
  printf("poly_reduce_pow2 tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_power2round();
  }
  printf("power2round tests complete\n");

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_pack_hint();
  }
  printf("pack_hint tests complete\n");

  printf("Success!\n");
}