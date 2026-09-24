#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "randombytes.h"
#include "parameters.h"
#include "fprime.h"
#include "poly.h"
#include "ring.h"
#include "pke.h"

#define RNG_SEED_LENGTH 32
DRNG_ctx drng_algorithm;

#define NUMBER_OF_TESTS 100
#define TEST_PRIMES {RRLWR_PKE_PRIME} // Random 28-bit primes for test purposes only
#define TEST_PRIMES_INV {RRLWR_PKE_PRIMEINV} // -p^-1 mod 2^32
#define NUMBER_OF_TEST_PRIMES 1

extern int32_t rrlwr_pke_zetas[RRLWR_N] __attribute__((aligned(32)));

static int32_t generate_random_int32(DRNG_ctx *drng) {
  unsigned char x[4];
  GENERATE_RANDOM_BYTES(x, 4, drng);
  return x[0] | (x[1] << 8) | (x[2] << 16) | (x[3] << 24);
}

static int test_montgomery_mul32() {

  int32_t x, xR, y, z, prime, primeinv, R, r;
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;
  int32_t test_primes_inv[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES_INV;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];
    primeinv = test_primes_inv[i];
    R = ((int64_t)1 << 32) % prime;

    x = generate_random_int32(&drng_algorithm) % prime;
    xR = (int32_t)(((int64_t)x*R) % prime); // Convert to Montgomery domain
    y = generate_random_int32(&drng_algorithm) % prime;
    z = montgomery_mul32(xR, y, prime, primeinv);

    assert(z >= -prime+1);
    assert(z <= prime);

    z = ((z % prime) + prime) % prime; // Make positive
    r = ((int64_t)x*y) % prime;
    r = ((r % prime) + prime) % prime; // Make positive

    assert(z == r);
  }

  return 0;
}

static int test_add32() {

  int32_t x, y, z, prime;
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];

    x = generate_random_int32(&drng_algorithm) % prime;
    y = generate_random_int32(&drng_algorithm) % prime;
    z = add32(x, y, prime);

    assert(z >= -prime);
    assert(z <= prime);

    z = ((z % prime) + prime) % prime; // Make positive
    int32_t r = (x + y) % prime;
    r = ((r % prime) + prime) % prime; // Make positive

    assert(z == r);
  }

  return 0;
}

static int test_sub32() {

  int32_t x, y, z, prime;
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];

    x = generate_random_int32(&drng_algorithm) % prime;
    y = generate_random_int32(&drng_algorithm) % prime;
    z = sub32(x, y, prime);

    assert(z >= -prime);
    assert(z <= prime);

    z = ((z % prime) + prime) % prime; // Make positive
    int32_t r = (x - y) % prime;
    r = ((r % prime) + prime) % prime; // Make positive

    assert(z == r);
  }

  return 0;
}

static int test_ntt() {

  poly f, fc __attribute__((aligned(32)));
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;
  int32_t test_primes_inv[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES_INV;
  int32_t prime, primeinv;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];
    primeinv = test_primes_inv[i];

    for(unsigned int j = 0; j < RRLWR_N; j++) {
      f.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
      fc.coeffs[j] = f.coeffs[j];
    }

    poly_ntt32(&f, prime, primeinv, rrlwr_pke_zetas);
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      f.coeffs[j] %= prime; // Reduce as invntt does lazy reduction, would overflow otherwise
      assert(f.coeffs[j] >= -prime);
      assert(f.coeffs[j] <= prime-1);
    }
    poly_invntt32(&f, prime, primeinv, RRLWR_NTTINV_FINALCONST, rrlwr_pke_zetas);

    // Output from invntt has a Montgomery factor R
    // Multiply in fc to check equality
    for(unsigned j = 0; j < RRLWR_N; j++) {
      fc.coeffs[j] = ((int64_t)fc.coeffs[j] * RRLWR_KEM_RMODPRIME) % prime;
    }

    // Convert to a unique representation in [0, p-1] for comparison
    for(unsigned j = 0; j < RRLWR_N; j++) {
      fc.coeffs[j] = ((fc.coeffs[j] % prime) + prime) % prime;
      f.coeffs[j] = ((f.coeffs[j] % prime) + prime) % prime;
    }

    // Check that invntt(ntt(f)) = f
    assert(!memcmp(fc.coeffs, f.coeffs, RRLWR_N << 2));
  }

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

  poly f, g, hp, hs __attribute__((aligned(32)));
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;
  int32_t test_primes_inv[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES_INV;
  int32_t prime, primeinv;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];
    primeinv = test_primes_inv[i];

    for(unsigned int j = 0; j < RRLWR_N; j++) {
      f.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
      g.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
    }

    schoolbook_mul(&hs, &f, &g, prime);

    poly_ntt32(&f, prime, primeinv, rrlwr_pke_zetas);      // No Montgomery factor in input/output, only in zetas
    poly_ntt32(&g, prime, primeinv, rrlwr_pke_zetas);      // No Montgomery factor in input/output, only in zetas
    poly_basemul32(&hp, &f, &g, prime, primeinv);          // Includes Montgomery factor R^-1
    poly_invntt32(&hp, prime, primeinv, RRLWR_NTTINV_FINALCONST, rrlwr_pke_zetas);  // Removes Montgomery factor from outputwith final multiplication

    // Convert to a unique representation in [0, p-1] for comparison
    for(unsigned j = 0; j < RRLWR_N; j++) {
      hs.coeffs[j] = ((hs.coeffs[j] % prime) + prime) % prime;
      hp.coeffs[j] = ((hp.coeffs[j] % prime) + prime) % prime;
    }

    // Check that f*g = hs = hp = invntt(ntt(f)*ntt(g))
    assert(!memcmp(hs.coeffs, hp.coeffs, RRLWR_N << 2));
  }

  return 0;
}

static int test_poly_add32() {

  poly f, g, hp, hs;
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;
  int32_t prime;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];

    for(unsigned int j = 0; j < RRLWR_N; j++) {
      f.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
      g.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
    }

    // Schoolbook polynomial multiplication
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      hs.coeffs[j] = f.coeffs[j] + g.coeffs[j];
    }

    poly_add32(&hp, &f, &g, prime);

    // Convert to a unique representation in [0, p-1] for comparison
    for(unsigned j = 0; j < RRLWR_N; j++) {
      hs.coeffs[j] = ((hs.coeffs[j] % prime) + prime) % prime;
      hp.coeffs[j] = ((hp.coeffs[j] % prime) + prime) % prime;
    }

    // Check that f*g = hs = hp = invntt(ntt(f)*ntt(g))
    assert(!memcmp(hs.coeffs, hp.coeffs, RRLWR_N << 2));
  }

  return 0;
}

static int test_poly_sub32() {

  poly f, g, hp, hs;
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;
  int32_t prime;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];

    for(unsigned int j = 0; j < RRLWR_N; j++) {
      f.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
      g.coeffs[j] = generate_random_int32(&drng_algorithm) % prime;
    }

    // Schoolbook polynomial multiplication
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      hs.coeffs[j] = f.coeffs[j] - g.coeffs[j];
    }

    poly_sub32(&hp, &f, &g, prime);

    // Convert to a unique representation in [0, p-1] for comparison
    for(unsigned j = 0; j < RRLWR_N; j++) {
      hs.coeffs[j] = ((hs.coeffs[j] % prime) + prime) % prime;
      hp.coeffs[j] = ((hp.coeffs[j] % prime) + prime) % prime;
    }

    // Check that f*g = hs = hp = invntt(ntt(f)*ntt(g))
    assert(!memcmp(hs.coeffs, hp.coeffs, RRLWR_N << 2));
  }

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

  ring_element r __attribute__((aligned(32)));
  ring_element a __attribute__((aligned(32)));
  ring_element s __attribute__((aligned(32)));
  ring_element rc __attribute__((aligned(32)));
  ring_element ac __attribute__((aligned(32)));
  ring_element sc __attribute__((aligned(32)));
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;
  int32_t test_primes_inv[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES_INV;
  int32_t prime, primeinv;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];
    primeinv = test_primes_inv[i];

    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        a.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % prime;
        s.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % prime;
        ac.x[j].coeffs[k] = a.x[j].coeffs[k];
        sc.x[j].coeffs[k] = s.x[j].coeffs[k];
        r.x[j].coeffs[k] = 1;
        rc.x[j].coeffs[k] = 0;
      }
    }

    // Compare against schoolbook multiplication
    ring_element h;
    schoolbook_ring_mul(&h, &a, &s, prime);

    // Sanity check: multiplication is commutative
    ring_mul32(r.x, &a, &s, RRLWR_K, prime, primeinv, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas);
    ring_mul32(rc.x, &sc, &ac, RRLWR_K, prime, primeinv, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas);
    assert(!memcmp(&r, &rc, RRLWR_K*(RRLWR_N << 2)));

    // Check that range of output is in [-(p-1)/2+1, (p-1)/2]
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        assert(r.x[j].coeffs[k] >= -(prime-1)/2+1);
        assert(r.x[j].coeffs[k] <= (prime-1)/2);
      }
    }

    // Convert to a unique representation in [0, p-1] for comparison
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        h.x[j].coeffs[k] = ((h.x[j].coeffs[k] % prime) + prime) % prime;
        r.x[j].coeffs[k] = ((r.x[j].coeffs[k] % prime) + prime) % prime;
        if (h.x[j].coeffs[k] != r.x[j].coeffs[k]) {
          printf("%d %d %d %d\n", j, k, h.x[j].coeffs[k], r.x[j].coeffs[k]);
        }
      }
    }

    assert(!memcmp(&r, &h, RRLWR_K*(RRLWR_N << 2)));

    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        a.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % prime;
        s.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % prime;
        ac.x[j].coeffs[k] = a.x[j].coeffs[k];
        sc.x[j].coeffs[k] = s.x[j].coeffs[k];
        r.x[j].coeffs[k] = 1;
        rc.x[j].coeffs[k] = 0;
      }
    }

    // Compare against schoolbook multiplication
    schoolbook_ring_mul(&h, &a, &s, prime);

    // Sanity check: multiplication is commutative with partial computation
    ring_mul32(r.x, &a, &s, 2, prime, primeinv, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas);
    ring_mul32(rc.x, &sc, &ac, 2, prime, primeinv, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas);

    // Convert to a unique representation in [0, p-1] for comparison
    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        h.x[j].coeffs[k] = ((h.x[j].coeffs[k] % prime) + prime) % prime;
        r.x[j].coeffs[k] = ((r.x[j].coeffs[k] % prime) + prime) % prime;
        rc.x[j].coeffs[k] = ((rc.x[j].coeffs[k] % prime) + prime) % prime;
      }
    }

    assert(!memcmp(&r.x[0], &rc.x[0], RRLWR_N << 2));
    assert(!memcmp(&r.x[1], &rc.x[1], RRLWR_N << 2));
    assert(memcmp(&r.x[2], &rc.x[2], RRLWR_N << 2));
    assert(!memcmp(&r.x[0], &h.x[RRLWR_K-2], RRLWR_N << 2));
    assert(!memcmp(&r.x[1], &h.x[RRLWR_K-1], RRLWR_N << 2));
    assert(memcmp(&r.x[2], &h.x[RRLWR_K-3], RRLWR_N << 2));
  }

  return 0;
}

static int test_ring_mul32_modq() {

  ring_element r __attribute__((aligned(32)));
  ring_element a __attribute__((aligned(32)));
  ring_element s __attribute__((aligned(32)));
  int32_t test_primes[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES;
  int32_t test_primes_inv[NUMBER_OF_TEST_PRIMES] = TEST_PRIMES_INV;
  int32_t prime, primeinv;
  int32_t logq = RRLWR_PKE_LOGQ;
  int32_t q = (int32_t)1 << logq;
  int32_t logs = 2;

  for(unsigned int i = 0; i < NUMBER_OF_TEST_PRIMES; i++) {
    prime = test_primes[i];
    primeinv = test_primes_inv[i];

    for(unsigned int j = 0; j < RRLWR_K; j++) {
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        a.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % q;
        s.x[j].coeffs[k] = generate_random_int32(&drng_algorithm) % ((int32_t)1 << logs);
      }
    }

    // Compare against schoolbook multiplication
    ring_element h;
    schoolbook_ring_mul(&h, &a, &s, q);

    // Sanity check: multiplication is commutative
    ring_mul32(r.x, &a, &s, RRLWR_K, prime, primeinv, RRLWR_NTTINV_FINALCONST, RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME, rrlwr_pke_zetas);

    for(unsigned int j = 0; j < RRLWR_K; j++) {
      // Reduce mod q and make positive
      for(unsigned int k = 0; k < RRLWR_N; k++) {
        h.x[j].coeffs[k] = ((h.x[j].coeffs[k] % q) + q) % q;
        r.x[j].coeffs[k] = ((r.x[j].coeffs[k] % q) + q) % q;
      }
    }

    assert(!memcmp(&r, &h, RRLWR_K*(RRLWR_N << 2)));
  }

  return 0;
}

static int test_packing() {
  unsigned char buffer2[RRLWR_K*RRLWR_PACKED_POLY2_LEN];
  unsigned char buffert[RRLWR_K*RRLWR_PKE_PACKED_POLYT_LEN];
  unsigned char bufferp[RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN];
  unsigned char bufferq[RRLWR_K*RRLWR_PKE_PACKED_POLYQ_LEN];
  ring_element r, rc1, rc2;
  int32_t q;

  // Generate random ring element
  q = (int32_t)1 << 2;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = ((generate_random_int32(&drng_algorithm) % q) + q) % q; // Make sure it is in [0, 3]
      r.x[j].coeffs[k] = 1-r.x[j].coeffs[k]; // Make signed in [-2, 1]
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k]; // Store a copy
    }
  }

  ring_pack(buffer2, &r, RRLWR_PKE_LOG_ETA+1);
  ring_unpack(&rc2, buffer2, RRLWR_PKE_LOG_ETA+1);

  assert(!memcmp(&rc1, &rc2, RRLWR_K*(RRLWR_N << 2)));

  // Generate random ring element
  q = (int32_t)1 << RRLWR_PKE_LOGQ;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = ((generate_random_int32(&drng_algorithm) % q) + q) % q; // Make sure it is positive
      r.x[j].coeffs[k] = (q/2)-1-r.x[j].coeffs[k]; // Make signed in [-q/2, q/2-1]
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k]; // Store a copy
    }
  }

  ring_pack(bufferq, &r, RRLWR_PKE_LOGQ);
  ring_unpack(&rc2, bufferq, RRLWR_PKE_LOGQ);

  assert(!memcmp(&rc1, &rc2, RRLWR_K*(RRLWR_N << 2)));

  // Generate random ring element
  q = (int32_t)1 << RRLWR_PKE_LOGP;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = ((generate_random_int32(&drng_algorithm) % q) + q) % q; // Make sure it is positive
      r.x[j].coeffs[k] = (q/2)-1-r.x[j].coeffs[k]; // Make signed in [-q/2, q/2-1]
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k]; // Store a copy
    }
  }

  ring_pack(bufferp, &r, RRLWR_PKE_LOGP);
  ring_unpack(&rc2, bufferp, RRLWR_PKE_LOGP);

  assert(!memcmp(&rc1, &rc2, RRLWR_K*(RRLWR_N << 2)));

  // Generate random ring element
  q = (int32_t)1 << RRLWR_PKE_LOGT;
  for(unsigned int j = 0; j < RRLWR_K; j++) {
    for(unsigned int k = 0; k < RRLWR_N; k++) {
      r.x[j].coeffs[k] = ((generate_random_int32(&drng_algorithm) % q) + q) % q; // Make sure it is positive
      r.x[j].coeffs[k] = (q/2)-1-r.x[j].coeffs[k]; // Make signed in [-q/2, q/2-1]
      rc1.x[j].coeffs[k] = r.x[j].coeffs[k]; // Store a copy
    }
  }

  ring_pack(buffert, &r, RRLWR_PKE_LOGT);
  ring_unpack(&rc2, buffert, RRLWR_PKE_LOGT);

  assert(!memcmp(&rc1, &rc2, RRLWR_K*(RRLWR_N << 2)));

  return 0;
}

static int test_pke() {
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
  unsigned char seedS[RRLWR_SEED_S_LEN];
  unsigned char seedSp[RRLWR_SEED_S_LEN];
  unsigned char sk[RRLWR_PKE_SK_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN];
  unsigned char ct[RRLWR_PKE_CT_LEN];
  unsigned char m[RRLWR_PKE_MESSAGE_LEN];
  unsigned char mp[RRLWR_PKE_MESSAGE_LEN];

  GENERATE_RANDOM_BYTES(seedA,  RRLWR_PKE_SEED_A_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedS,  RRLWR_SEED_S_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedSp, RRLWR_SEED_S_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(m,      RRLWR_PKE_MESSAGE_LEN, &drng_algorithm);

  pke_keygen(pk, sk, seedA, seedS);
  pke_encrypt(ct, pk, m, seedSp);
  pke_decrypt(mp, ct, sk);

  assert(!memcmp(m, mp, RRLWR_PKE_MESSAGE_LEN));

  return 0;
}

int main() {
  /* Initialize RNG*/
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_montgomery_mul32();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_add32();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_sub32();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_ntt();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_poly_mul32();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_poly_add32();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_poly_sub32();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_ring_mul32();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_ring_mul32_modq();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_packing();
  }

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    test_pke();
  }

  printf("Success!\n");
}