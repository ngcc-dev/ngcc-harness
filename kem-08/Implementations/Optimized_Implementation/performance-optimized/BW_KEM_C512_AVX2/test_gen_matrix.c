#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auxfunc.h"
#include "drng.h"
#include "indcpa.h"
#include "params.h"
#include "polyvec.h"
#include "symmetric.h"

DRNG_ctx drng_algorithm;

#define GEN_MATRIX_NBLOCKS ((12 * KYBER_N / 8 * (1 << 12) / KYBER_Q + XOF_BLOCKBYTES) / XOF_BLOCKBYTES)

static void fill_bytes(uint8_t *buf, size_t len, uint8_t seed)
{
  size_t i;

  for(i = 0; i < len; i++)
    buf[i] = (uint8_t)(seed + 23u * (uint8_t)i + (uint8_t)(i >> 2));
}

static unsigned int ref_rej_uniform(int16_t *r, unsigned int len, const uint8_t *buf, unsigned int buflen)
{
  unsigned int ctr = 0;
  unsigned int pos = 0;

  while(ctr < len && pos + 3 <= buflen) {
    uint16_t val0 = ((buf[pos + 0] >> 0) | ((uint16_t)buf[pos + 1] << 8)) & 0xFFF;
    uint16_t val1 = ((buf[pos + 1] >> 4) | ((uint16_t)buf[pos + 2] << 4)) & 0xFFF;
    pos += 3;
    if(val0 < KYBER_Q)
      r[ctr++] = val0;
    if(ctr < len && val1 < KYBER_Q)
      r[ctr++] = val1;
  }
  return ctr;
}

static void ref_gen_matrix(polyvec *a, const uint8_t seed[KYBER_SYMBYTES], int transposed)
{
  unsigned int i, j;

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_K; j++) {
      uint8_t extseed[KYBER_SYMBYTES + 2];
      uint8_t buf[GEN_MATRIX_NBLOCKS * XOF_BLOCKBYTES];
      unsigned int ctr;
      unsigned int buflen;
      uint32_t counter_blocks = GEN_MATRIX_NBLOCKS;

      memcpy(extseed, seed, KYBER_SYMBYTES);
      if(transposed) {
        extseed[KYBER_SYMBYTES] = (uint8_t)i;
        extseed[KYBER_SYMBYTES + 1] = (uint8_t)j;
      } else {
        extseed[KYBER_SYMBYTES] = (uint8_t)j;
        extseed[KYBER_SYMBYTES + 1] = (uint8_t)i;
      }

      if(pseudoXOF((unsigned long long)sizeof(buf) * 8ULL,
                   extseed,
                   (unsigned long long)sizeof(extseed) * 8ULL,
                   buf) != 0) {
        fprintf(stderr, "pseudoXOF failed\n");
        exit(1);
      }
      buflen = sizeof(buf);
      ctr = ref_rej_uniform(a[i].vec[j].coeffs, KYBER_N, buf, buflen);
      while(ctr < KYBER_N) {
        uint8_t one[XOF_BLOCKBYTES];
        size_t total = (++counter_blocks) * XOF_BLOCKBYTES;
        uint8_t stream[total];
        if(pseudoXOF((unsigned long long)total * 8ULL,
                     extseed,
                     (unsigned long long)sizeof(extseed) * 8ULL,
                     stream) != 0) {
          fprintf(stderr, "pseudoXOF retry failed\n");
          exit(1);
        }
        memcpy(one, stream + total - XOF_BLOCKBYTES, XOF_BLOCKBYTES);
        ctr += ref_rej_uniform(a[i].vec[j].coeffs + ctr, KYBER_N - ctr, one, XOF_BLOCKBYTES);
      }
    }
  }
}

static void assert_matrix_equal(const polyvec *got, const polyvec *want)
{
  unsigned int i, j, k;

  for(i = 0; i < KYBER_K; i++) {
    for(j = 0; j < KYBER_K; j++) {
      for(k = 0; k < KYBER_N; k++) {
        if(got[i].vec[j].coeffs[k] != want[i].vec[j].coeffs[k]) {
          fprintf(stderr, "gen_matrix mismatch at [%u][%u][%u]: got %d want %d\n",
                  i, j, k, got[i].vec[j].coeffs[k], want[i].vec[j].coeffs[k]);
          exit(1);
        }
      }
    }
  }
}

int main(void)
{
  uint8_t seed[KYBER_SYMBYTES];
  polyvec got[KYBER_K];
  polyvec ref[KYBER_K];
  unsigned int t;

  for(t = 0; t < 1000; t++) {
    fill_bytes(seed, sizeof(seed), (uint8_t)t);
    gen_matrix(got, seed, 0);
    ref_gen_matrix(ref, seed, 0);
    assert_matrix_equal(got, ref);
    gen_matrix(got, seed, 1);
    ref_gen_matrix(ref, seed, 1);
    assert_matrix_equal(got, ref);
  }

  puts("test_gen_matrix: ok");
  return 0;
}
