#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpucycles.h"
#include "packing.h"

#define NTESTS 10000

void poly_pack8_avx(unsigned char *b, const poly *r);
void poly_unpack2_avx(poly *r, const unsigned char *b);
void poly_unpack3_avx(poly *r, const unsigned char *b);
void poly_unpack5_avx(poly *r, const unsigned char *b);
void poly_unpack6_avx(poly *r, const unsigned char *b);
void poly_unpack8_avx(poly *r, const unsigned char *b);
void poly_unpack9_avx(poly *r, const unsigned char *b);
void poly_unpack10_avx(poly *r, const unsigned char *b);
void poly_unpack11_avx(poly *r, const unsigned char *b);
void poly_unpack13_avx(poly *r, const unsigned char *b);
void poly_unpack14_avx(poly *r, const unsigned char *b);
void poly_unpack19_avx(poly *r, const unsigned char *b);
void poly_unpack20_avx(poly *r, const unsigned char *b);
void poly_unpack22_avx(poly *r, const unsigned char *b);
void poly_pack2_avx(unsigned char *b, const poly *r);
void poly_pack3_avx(unsigned char *b, const poly *r);
void poly_pack5_avx(unsigned char *b, const poly *r);
void poly_pack6_avx(unsigned char *b, const poly *r);
void poly_pack9_avx(unsigned char *b, const poly *r);
void poly_pack10_avx(unsigned char *b, const poly *r);
void poly_pack11_avx(unsigned char *b, const poly *r);
void poly_pack13_avx(unsigned char *b, const poly *r);
void poly_pack14_avx(unsigned char *b, const poly *r);
void poly_pack19_avx(unsigned char *b, const poly *r);
void poly_pack20_avx(unsigned char *b, const poly *r);
void poly_pack22_avx(unsigned char *b, const poly *r);
void poly_pack24_avx(unsigned char *b, const poly *r);
void poly_unpack24_avx(poly *r, const unsigned char *b);

static uint64_t cycles[NTESTS];

static int cmp_uint64(const void *a, const void *b) {
  uint64_t x = *(const uint64_t *)a;
  uint64_t y = *(const uint64_t *)b;
  return (x > y) - (x < y);
}

static uint64_t median(uint64_t *t) {
  qsort(t, NTESTS, sizeof(uint64_t), cmp_uint64);
  return t[NTESTS/2];
}

static uint32_t pack_coeff_ref(int32_t c, unsigned int bitlen) {
  return (((uint32_t)1 << (bitlen-1))-1u-(uint32_t)c) & (((uint32_t)1 << bitlen)-1u);
}

static int32_t unpack_coeff_ref(uint32_t c, unsigned int bitlen) {
  return (int32_t)(((uint32_t)1 << (bitlen-1))-1u-c);
}

static void poly_pack8_ref(unsigned char *b, const poly *r) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    b[i] = (unsigned char)pack_coeff_ref(r->coeffs[i], 8);
  }
}

static void poly_pack_ref(unsigned char *b, const poly *r, unsigned int bitlen) {
  uint64_t acc = 0;
  unsigned int acc_shift = 0;
  unsigned int bpos = 0;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    acc |= (uint64_t)pack_coeff_ref(r->coeffs[i], bitlen) << acc_shift;
    acc_shift += bitlen;
    while(acc_shift >= 8) {
      b[bpos++] = (unsigned char)acc;
      acc >>= 8;
      acc_shift -= 8;
    }
  }
}

static void poly_unpack8_ref(poly *r, const unsigned char *b) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = 127-(int32_t)b[i];
  }
}

static void poly_unpack_ref(poly *r, const unsigned char *b, unsigned int bitlen) {
  uint64_t acc = 0;
  unsigned int acc_shift = 0;
  unsigned int bpos = 0;
  uint32_t mask = ((uint32_t)1 << bitlen)-1u;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    while(acc_shift < bitlen) {
      acc |= (uint64_t)b[bpos++] << acc_shift;
      acc_shift += 8;
    }
    r->coeffs[i] = unpack_coeff_ref((uint32_t)acc & mask, bitlen);
    acc >>= bitlen;
    acc_shift -= bitlen;
  }
}

static void poly_pack14_ref(unsigned char *b, const poly *r) {
  for(unsigned int i = 0, j = 0; i < RRLWR_N; i += 4, j += 7) {
    uint64_t x = (uint64_t)pack_coeff_ref(r->coeffs[i+0], 14);
    x |= (uint64_t)pack_coeff_ref(r->coeffs[i+1], 14) << 14;
    x |= (uint64_t)pack_coeff_ref(r->coeffs[i+2], 14) << 28;
    x |= (uint64_t)pack_coeff_ref(r->coeffs[i+3], 14) << 42;
    b[j+0] = (unsigned char)x;
    b[j+1] = (unsigned char)(x >> 8);
    b[j+2] = (unsigned char)(x >> 16);
    b[j+3] = (unsigned char)(x >> 24);
    b[j+4] = (unsigned char)(x >> 32);
    b[j+5] = (unsigned char)(x >> 40);
    b[j+6] = (unsigned char)(x >> 48);
  }
}

static void poly_unpack24_ref(poly *r, const unsigned char *b) {
  for(unsigned int i = 0, j = 0; i < RRLWR_N; i++, j += 3) {
    uint32_t x = (uint32_t)b[j+0] | ((uint32_t)b[j+1] << 8) | ((uint32_t)b[j+2] << 16);
    r->coeffs[i] = 0x7fffff-(int32_t)x;
  }
}

static void fill_poly(poly *r, int32_t bitlen) {
  uint32_t mask = ((uint32_t)1 << bitlen)-1u;
  int32_t offset = ((int32_t)1 << (bitlen-1))-1;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint32_t x = ((uint32_t)rand()) & mask;
    r->coeffs[i] = offset-(int32_t)x;
  }
}

static void fill_poly_wide(poly *r) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    uint32_t x = ((uint32_t)rand() << 17) ^ (uint32_t)rand();
    r->coeffs[i] = (int32_t)(x & 0x01ffffffu) - 0x01000000;
  }
}

static void fill_bytes(unsigned char *b, size_t len) {
  for(size_t i = 0; i < len; i++) {
    b[i] = (unsigned char)rand();
  }
}

static uint64_t bench_pack(void (*fn)(unsigned char *, const poly *), unsigned char *b, const poly *r) {
  uint64_t overhead = cpucycles_overhead();
  for(unsigned int i = 0; i < NTESTS; i++) {
    uint64_t start = cpucycles();
    fn(b, r);
    cycles[i] = cpucycles() - start - overhead;
  }
  return median(cycles);
}

static uint64_t bench_unpack(void (*fn)(poly *, const unsigned char *), poly *r, const unsigned char *b) {
  uint64_t overhead = cpucycles_overhead();
  for(unsigned int i = 0; i < NTESTS; i++) {
    uint64_t start = cpucycles();
    fn(r, b);
    cycles[i] = cpucycles() - start - overhead;
  }
  return median(cycles);
}

static uint64_t bench_pack_ref(unsigned char *b, const poly *r, unsigned int bitlen) {
  uint64_t overhead = cpucycles_overhead();
  for(unsigned int i = 0; i < NTESTS; i++) {
    uint64_t start = cpucycles();
    poly_pack_ref(b, r, bitlen);
    cycles[i] = cpucycles() - start - overhead;
  }
  return median(cycles);
}

static uint64_t bench_unpack_ref(poly *r, const unsigned char *b, unsigned int bitlen) {
  uint64_t overhead = cpucycles_overhead();
  for(unsigned int i = 0; i < NTESTS; i++) {
    uint64_t start = cpucycles();
    poly_unpack_ref(r, b, bitlen);
    cycles[i] = cpucycles() - start - overhead;
  }
  return median(cycles);
}

struct pack_case {
  const char *name;
  unsigned int bitlen;
  void (*fn)(unsigned char *, const poly *);
};

struct unpack_case {
  const char *name;
  unsigned int bitlen;
  void (*fn)(poly *, const unsigned char *);
};

int main(void) {
  poly r;
  poly ref;
  poly got;
  unsigned char in24[3*RRLWR_N];
  unsigned char ref8[RRLWR_N];
  unsigned char got8[RRLWR_N];
  unsigned char ref14[14*(RRLWR_N >> 3)];
  unsigned char got14[14*(RRLWR_N >> 3)];
  unsigned char refpack[22*(RRLWR_N >> 3)];
  unsigned char gotpack[24*(RRLWR_N >> 3)];
  unsigned char refpack24[24*(RRLWR_N >> 3)];
  const struct pack_case pack_cases[] = {
      {"poly_pack2", 2, poly_pack2_avx},
      {"poly_pack3", 3, poly_pack3_avx},
      {"poly_pack5", 5, poly_pack5_avx},
      {"poly_pack6", 6, poly_pack6_avx},
      {"poly_pack8", 8, poly_pack8_avx},
      {"poly_pack9", 9, poly_pack9_avx},
      {"poly_pack10", 10, poly_pack10_avx},
      {"poly_pack11", 11, poly_pack11_avx},
      {"poly_pack13", 13, poly_pack13_avx},
      {"poly_pack14", 14, poly_pack14_avx},
      {"poly_pack19", 19, poly_pack19_avx},
      {"poly_pack20", 20, poly_pack20_avx},
      {"poly_pack22", 22, poly_pack22_avx},
      {"poly_pack24", 24, poly_pack24_avx},
  };
  const struct unpack_case unpack_cases[] = {
      {"poly_unpack2", 2, poly_unpack2_avx},
      {"poly_unpack3", 3, poly_unpack3_avx},
      {"poly_unpack5", 5, poly_unpack5_avx},
      {"poly_unpack6", 6, poly_unpack6_avx},
      {"poly_unpack8", 8, poly_unpack8_avx},
      {"poly_unpack9", 9, poly_unpack9_avx},
      {"poly_unpack10", 10, poly_unpack10_avx},
      {"poly_unpack11", 11, poly_unpack11_avx},
      {"poly_unpack13", 13, poly_unpack13_avx},
      {"poly_unpack14", 14, poly_unpack14_avx},
      {"poly_unpack19", 19, poly_unpack19_avx},
      {"poly_unpack20", 20, poly_unpack20_avx},
      {"poly_unpack22", 22, poly_unpack22_avx},
      {"poly_unpack24", 24, poly_unpack24_avx},
  };

  srand(1);

  fill_poly(&r, 8);
  poly_pack8_ref(ref8, &r);
  poly_pack8_avx(got8, &r);
  assert(!memcmp(ref8, got8, sizeof(ref8)));

  fill_bytes(ref8, sizeof(ref8));
  poly_unpack8_ref(&ref, ref8);
  poly_unpack8_avx(&got, ref8);
  assert(!memcmp(&ref, &got, sizeof(poly)));

  fill_poly(&r, 14);
  poly_pack14_ref(ref14, &r);
  poly_pack14_avx(got14, &r);
  assert(!memcmp(ref14, got14, sizeof(ref14)));

  for(unsigned int i = 0; i < sizeof(pack_cases)/sizeof(pack_cases[0]); i++) {
    size_t pack_len = (size_t)pack_cases[i].bitlen*(RRLWR_N >> 3);
    fill_poly(&r, (int32_t)pack_cases[i].bitlen);
    unsigned char *refbuf = pack_cases[i].bitlen == 24 ? refpack24 : refpack;
    memset(refbuf, 0, pack_len);
    memset(gotpack, 0, sizeof(gotpack));
    poly_pack_ref(refbuf, &r, pack_cases[i].bitlen);
    pack_cases[i].fn(gotpack, &r);
    if(memcmp(refbuf, gotpack, pack_len)) {
      fprintf(stderr, "%s correctness mismatch\n", pack_cases[i].name);
      assert(!memcmp(refbuf, gotpack, pack_len));
    }

    memset(gotpack, 0, sizeof(gotpack));
    poly_pack(gotpack, &r, (int32_t)pack_cases[i].bitlen);
    if(memcmp(refbuf, gotpack, pack_len)) {
      fprintf(stderr, "poly_pack dispatch mismatch for bitlen %u\n", pack_cases[i].bitlen);
      assert(!memcmp(refbuf, gotpack, pack_len));
    }

    fill_poly_wide(&r);
    memset(refbuf, 0, pack_len);
    memset(gotpack, 0, sizeof(gotpack));
    poly_pack_ref(refbuf, &r, pack_cases[i].bitlen);
    pack_cases[i].fn(gotpack, &r);
    if(memcmp(refbuf, gotpack, pack_len)) {
      fprintf(stderr, "%s wide-input correctness mismatch\n", pack_cases[i].name);
      assert(!memcmp(refbuf, gotpack, pack_len));
    }

    memset(gotpack, 0, sizeof(gotpack));
    poly_pack(gotpack, &r, (int32_t)pack_cases[i].bitlen);
    if(memcmp(refbuf, gotpack, pack_len)) {
      fprintf(stderr, "poly_pack wide-input dispatch mismatch for bitlen %u\n", pack_cases[i].bitlen);
      assert(!memcmp(refbuf, gotpack, pack_len));
    }
  }

  for(unsigned int i = 0; i < sizeof(unpack_cases)/sizeof(unpack_cases[0]); i++) {
    size_t pack_len = (size_t)unpack_cases[i].bitlen*(RRLWR_N >> 3);
    unsigned char *buf = unpack_cases[i].bitlen == 24 ? refpack24 : refpack;
    fill_bytes(buf, pack_len);

    poly_unpack_ref(&ref, buf, unpack_cases[i].bitlen);
    unpack_cases[i].fn(&got, buf);
    if(memcmp(&ref, &got, sizeof(poly))) {
      fprintf(stderr, "%s correctness mismatch\n", unpack_cases[i].name);
      assert(!memcmp(&ref, &got, sizeof(poly)));
    }

    poly_unpack(&got, buf, (int32_t)unpack_cases[i].bitlen);
    if(memcmp(&ref, &got, sizeof(poly))) {
      fprintf(stderr, "poly_unpack dispatch mismatch for bitlen %u\n", unpack_cases[i].bitlen);
      assert(!memcmp(&ref, &got, sizeof(poly)));
    }
  }

  fill_bytes(in24, sizeof(in24));
  poly_unpack24_ref(&ref, in24);
  poly_unpack24_avx(&got, in24);
  assert(!memcmp(&ref, &got, sizeof(poly)));

  printf("%-18s %10s %10s %8s\n", "function", "ref", "avx", "speedup");
  for(unsigned int i = 0; i < sizeof(pack_cases)/sizeof(pack_cases[0]); i++) {
    fill_poly(&r, (int32_t)pack_cases[i].bitlen);
    uint64_t ref_cycles = bench_pack_ref(gotpack, &r, pack_cases[i].bitlen);
    uint64_t avx_cycles = bench_pack(pack_cases[i].fn, gotpack, &r);
    printf("%-18s %10lu %10lu %8.2f\n", pack_cases[i].name, ref_cycles, avx_cycles, (double)ref_cycles/(double)avx_cycles);
  }
  for(unsigned int i = 0; i < sizeof(unpack_cases)/sizeof(unpack_cases[0]); i++) {
    size_t pack_len = (size_t)unpack_cases[i].bitlen*(RRLWR_N >> 3);
    unsigned char *buf = unpack_cases[i].bitlen == 24 ? refpack24 : refpack;
    fill_bytes(buf, pack_len);
    uint64_t ref_cycles = bench_unpack_ref(&got, buf, unpack_cases[i].bitlen);
    uint64_t avx_cycles = bench_unpack(unpack_cases[i].fn, &got, buf);
    printf("%-18s %10lu %10lu %8.2f\n", unpack_cases[i].name, ref_cycles, avx_cycles, (double)ref_cycles/(double)avx_cycles);
  }
  printf("Success!\n");

  return 0;
}
