#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "reduce.h"
#include "ntt.h"

static uint32_t lcg_state = 1;

static uint32_t lcg(void) {
  lcg_state = 1664525u * lcg_state + 1013904223u;
  return lcg_state;
}

static int32_t mod_norm(int64_t x) {
  int64_t r = x % Q;
  if(r < 0) r += Q;
  if(r > (Q/2)) r -= Q;
  return (int32_t)r;
}

static uint32_t mod_pow_u32(uint32_t a, uint64_t e) {
  uint64_t r = 1;
  uint64_t b = a % (uint64_t)Q;
  while(e) {
    if(e & 1) r = (r * b) % (uint64_t)Q;
    b = (b * b) % (uint64_t)Q;
    e >>= 1;
  }
  return (uint32_t)r;
}

static uint32_t mod_mul_u32(uint32_t a, uint32_t b) {
  return (uint32_t)(((uint64_t)a * b) % (uint64_t)Q);
}

static uint32_t find_generator(void) {
  const uint64_t qm1 = (uint64_t)Q - 1;
  const uint32_t factors[4] = {2, 3, 43, 127};
  for(uint32_t g = 2; g < (uint32_t)Q; g++) {
    int ok = 1;
    for(unsigned i = 0; i < 4; i++) {
      if(mod_pow_u32(g, qm1 / factors[i]) == 1) {
        ok = 0;
        break;
      }
    }
    if(ok) return g;
  }
  return 0;
}

typedef struct {
  uint32_t psi;
  uint32_t psi_inv;
  uint32_t omega;
  uint32_t omega_inv;
  uint32_t invn;
  uint32_t psi_pows[N];
  uint32_t psi_inv_pows[N];
  uint32_t omega_pows[N];
  uint32_t omega_inv_pows[N];
} ref_ntt_ctx;

static uint16_t bitrev9(uint16_t x) {
  x = (uint16_t)(((x & 0x00ffu) << 8) | ((x & 0xff00u) >> 8));
  x = (uint16_t)(((x & 0x0f0fu) << 4) | ((x & 0xf0f0u) >> 4));
  x = (uint16_t)(((x & 0x3333u) << 2) | ((x & 0xccccu) >> 2));
  x = (uint16_t)(((x & 0x5555u) << 1) | ((x & 0xaaaau) >> 1));
  return (uint16_t)(x >> 7);
}

static void ref_ntt_init(ref_ntt_ctx *ctx) {
  const uint64_t qm1 = (uint64_t)Q - 1;
  const uint32_t g = find_generator();
  ctx->psi = mod_pow_u32(g, qm1 / (2ULL * (uint64_t)N));
  ctx->psi_inv = mod_pow_u32(ctx->psi, (uint64_t)Q - 2);
  ctx->omega = mod_mul_u32(ctx->psi, ctx->psi);
  ctx->omega_inv = mod_pow_u32(ctx->omega, (uint64_t)Q - 2);
  ctx->invn = mod_pow_u32((uint32_t)N, (uint64_t)Q - 2);

  uint32_t p = 1;
  uint32_t pinv = 1;
  for(unsigned i = 0; i < N; i++) {
    ctx->psi_pows[i] = p;
    ctx->psi_inv_pows[i] = pinv;
    p = mod_mul_u32(p, ctx->psi);
    pinv = mod_mul_u32(pinv, ctx->psi_inv);
  }

  uint32_t w = 1;
  uint32_t winv = 1;
  for(unsigned i = 0; i < N; i++) {
    ctx->omega_pows[i] = w;
    ctx->omega_inv_pows[i] = winv;
    w = mod_mul_u32(w, ctx->omega);
    winv = mod_mul_u32(winv, ctx->omega_inv);
  }
}

static void ref_fwd(int32_t out[N], const int32_t in[N], const ref_ntt_ctx *ctx) {
  uint32_t b[N];
  for(unsigned i = 0; i < N; i++) {
    uint32_t x = (uint32_t)((in[i] % Q + Q) % Q);
    b[i] = mod_mul_u32(x, ctx->psi_pows[i]);
  }

  for(unsigned k = 0; k < N; k++) {
    uint32_t base = ctx->omega_pows[k];
    uint32_t pow = 1;
    int64_t acc = 0;
    for(unsigned i = 0; i < N; i++) {
      acc += (int64_t)mod_mul_u32(b[i], pow);
      pow = mod_mul_u32(pow, base);
    }
    out[k] = mod_norm(acc);
  }
}

static void ref_inv(int32_t out[N], const int32_t in[N], const ref_ntt_ctx *ctx) {
  for(unsigned i = 0; i < N; i++) {
    uint32_t base = ctx->omega_inv_pows[i];
    uint32_t pow = 1;
    int64_t acc = 0;
    for(unsigned k = 0; k < N; k++) {
      uint32_t xk = (uint32_t)((in[k] % Q + Q) % Q);
      acc += (int64_t)mod_mul_u32(xk, pow);
      pow = mod_mul_u32(pow, base);
    }
    uint32_t v = (uint32_t)((acc % Q + Q) % Q);
    v = mod_mul_u32(v, ctx->invn);
    v = mod_mul_u32(v, ctx->psi_inv_pows[i]);
    out[i] = mod_norm((int64_t)v);
  }
}

static void ref_mul(int32_t c[N], const int32_t a[N], const int32_t b[N], const ref_ntt_ctx *ctx) {
  int32_t A[N], B[N], C[N];
  ref_fwd(A, a, ctx);
  ref_fwd(B, b, ctx);
  for(unsigned i = 0; i < N; i++) {
    int64_t t = (int64_t)A[i] * B[i];
    C[i] = mod_norm(t);
  }
  ref_inv(c, C, ctx);
}

static void fast_fwd_inplace(int32_t a[N], const ref_ntt_ctx *ctx) {
  for(unsigned i = 0; i < N; i++) {
    uint32_t x = (uint32_t)((a[i] % Q + Q) % Q);
    a[i] = mod_norm((int64_t)mod_mul_u32(x, ctx->psi_pows[i]));
  }

  {
    int32_t tmp[N];
    for(uint16_t i = 0; i < (uint16_t)N; i++)
      tmp[bitrev9(i)] = a[i];
    for(unsigned i = 0; i < N; i++)
      a[i] = tmp[i];
  }

  for(unsigned len = 1; len < N; len <<= 1) {
    unsigned step = N / (2u * len);
    for(unsigned start = 0; start < N; start += 2u * len) {
      for(unsigned j = 0; j < len; j++) {
        uint32_t w = ctx->omega_pows[j * step];
        int32_t u = a[start + j];
        int32_t v = mod_norm((int64_t)a[start + j + len] * (int64_t)w);
        a[start + j] = mod_norm((int64_t)u + v);
        a[start + j + len] = mod_norm((int64_t)u - v);
      }
    }
  }
}

static void fast_inv_inplace(int32_t a[N], const ref_ntt_ctx *ctx) {
  int32_t tmp[N];
  for(uint16_t i = 0; i < (uint16_t)N; i++)
    tmp[i] = a[bitrev9(i)];
  for(unsigned i = 0; i < N; i++)
    a[i] = tmp[i];

  for(unsigned len = 1; len < N; len <<= 1) {
    unsigned step = N / (2u * len);
    for(unsigned start = 0; start < N; start += 2u * len) {
      for(unsigned j = 0; j < len; j++) {
        uint32_t w = ctx->omega_inv_pows[j * step];
        int32_t u = a[start + j];
        int32_t v = a[start + j + len];
        int32_t t = mod_norm((int64_t)v * (int64_t)w);
        a[start + j] = mod_norm((int64_t)u + t);
        a[start + j + len] = mod_norm((int64_t)u - t);
      }
    }
  }

  for(unsigned i = 0; i < N; i++) {
    uint32_t x = (uint32_t)((a[i] % Q + Q) % Q);
    x = mod_mul_u32(x, ctx->invn);
    x = mod_mul_u32(x, ctx->psi_inv_pows[i]);
    a[i] = mod_norm((int64_t)x);
  }
}

static void fast_mul(int32_t c[N], const int32_t a[N], const int32_t b[N], const ref_ntt_ctx *ctx) {
  int32_t A[N], B[N];
  memcpy(A, a, sizeof(A));
  memcpy(B, b, sizeof(B));
  fast_fwd_inplace(A, ctx);
  fast_fwd_inplace(B, ctx);
  for(unsigned i = 0; i < N; i++)
    A[i] = mod_norm((int64_t)A[i] * B[i]);
  fast_inv_inplace(A, ctx);
  memcpy(c, A, sizeof(A));
}

static void naive_mul(int32_t c[N], const int32_t a[N], const int32_t b[N]) {
  int64_t acc[N];
  for(unsigned i = 0; i < N; i++) acc[i] = 0;

  for(unsigned i = 0; i < N; i++) {
    for(unsigned j = 0; j < N; j++) {
      int64_t prod = (int64_t)a[i] * b[j];
      unsigned ij = i + j;
      if(ij < N) acc[ij] += prod;
      else acc[ij - N] -= prod;
    }
  }
  for(unsigned i = 0; i < N; i++) c[i] = mod_norm(acc[i]);
}

int main(void) {
  int32_t a[N], b[N], c0[N], c1[N];
  for(unsigned i = 0; i < N; i++) {
    a[i] = (int32_t)(lcg() % (uint32_t)Q);
    b[i] = (int32_t)(lcg() % (uint32_t)Q);
    if(a[i] > (Q/2)) a[i] -= Q;
    if(b[i] > (Q/2)) b[i] -= Q;
  }

  ref_ntt_ctx ctx;
  ref_ntt_init(&ctx);

  {
    int32_t rt[N];
    int32_t rt2[N];
    int32_t tmpa[N];
    memcpy(tmpa, a, sizeof(a));
    ref_fwd(rt, tmpa, &ctx);
    ref_inv(rt2, rt, &ctx);
    for(unsigned i = 0; i < N; i++) {
      if(mod_norm(rt2[i]) != mod_norm(a[i])) {
        printf("ref roundtrip mismatch at %u: %d vs %d\n", i, mod_norm(rt2[i]), mod_norm(a[i]));
        return 1;
      }
    }
  }

  {
    int32_t t[N];
    memcpy(t, a, sizeof(a));
    ntt(t);
    invntt_tomont(t);
    for(unsigned idx = 0; idx < 8; idx++) {
      int32_t rt = mod_norm(montgomery_reduce((int64_t)t[idx]));
      int32_t aa = mod_norm(a[idx]);
      printf("rt[%u]=%d a[%u]=%d\n", idx, rt, idx, aa);
    }
  }

  naive_mul(c0, a, b);
  ref_mul(c1, a, b, &ctx);
  for(unsigned i = 0; i < N; i++) {
    if(c0[i] != c1[i]) {
      printf("ref mul mismatch at %u: %d vs %d\n", i, c0[i], c1[i]);
      return 1;
    }
  }
  printf("ref ok\n");

  {
    int32_t t[N];
    memcpy(t, a, sizeof(a));
    fast_fwd_inplace(t, &ctx);
    fast_inv_inplace(t, &ctx);
    for(unsigned i = 0; i < N; i++) {
      if(mod_norm(t[i]) != mod_norm(a[i])) {
        printf("fast roundtrip mismatch at %u: %d vs %d\n", i, mod_norm(t[i]), mod_norm(a[i]));
        return 1;
      }
    }
  }
  printf("fast roundtrip ok\n");

  fast_mul(c1, a, b, &ctx);
  for(unsigned i = 0; i < N; i++) {
    if(c0[i] != c1[i]) {
      printf("fast mul mismatch at %u: %d vs %d\n", i, c0[i], c1[i]);
      return 1;
    }
  }
  printf("fast ok\n");

  memcpy(c1, a, sizeof(a));
  int32_t tmp[N];
  memcpy(tmp, b, sizeof(b));

  ntt(c1);
  ntt(tmp);
  for(unsigned i = 0; i < N; i++)
    c1[i] = montgomery_reduce((int64_t)c1[i] * tmp[i]);
  invntt_tomont(c1);

  for(unsigned i = 0; i < N; i++)
    c1[i] = mod_norm(c1[i]);

  for(unsigned i = 0; i < N; i++) {
    if(c0[i] != c1[i]) {
      printf("mismatch at %u: %d vs %d\n", i, c0[i], c1[i]);
      return 1;
    }
  }

  printf("ok\n");
  return 0;
}
