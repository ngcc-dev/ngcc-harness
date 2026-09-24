/*
 * MAMBA-NIKE NGCC KEX API conformance checks.
 *
 * Build this file together with one profile's KEX_AlgorithmInstance.c,
 * core sources and drng.c using -DKAT_BUILD.
 */

#include "KEX_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;

#define CHECK(cond, msg) do { \
  if(!(cond)) { \
    fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while(0)

static unsigned char *zalloc(unsigned long long n)
{
  if(n == 0)
    n = 1;
  return (unsigned char *)calloc((size_t)n, 1);
}

static int all_zero(const unsigned char *x, unsigned long long n)
{
  unsigned long long i;
  for(i = 0; i < n; i++)
    if(x[i] != 0)
      return 0;
  return 1;
}

static void seed_drng(unsigned char tag)
{
  unsigned char seed[64];
  unsigned int i;
  for(i = 0; i < sizeof(seed); i++)
    seed[i] = (unsigned char)(tag + 13U * i);
  init_random_number(&drng_algorithm, seed, sizeof(seed));
}

struct ctx {
  unsigned long long pk_len;
  unsigned long long sk_len;
  unsigned long long ss_len;
  unsigned long long m1_len;
  unsigned long long sta_cap;
  unsigned long long stb_cap;
  unsigned char *pka;
  unsigned char *ska;
  unsigned char *pkb;
  unsigned char *skb;
  unsigned char *sta;
  unsigned char *stb;
  unsigned char *m1;
  unsigned char *ssa;
  unsigned char *ssb;
};

static int ctx_alloc(struct ctx *c)
{
  memset(c, 0, sizeof(*c));
  c->pk_len = kex_get_pk_len_bytes();
  c->sk_len = kex_get_sk_len_bytes();
  c->ss_len = kex_get_ss_len_bytes();
  c->m1_len = kex_get_total_msg_len_bytes();
  c->sta_cap = kex_get_sta_len_bytes();
  c->stb_cap = kex_get_stb_len_bytes();

  c->pka = zalloc(c->pk_len);
  c->ska = zalloc(c->sk_len);
  c->pkb = zalloc(c->pk_len);
  c->skb = zalloc(c->sk_len);
  c->sta = zalloc(c->sta_cap);
  c->stb = zalloc(c->stb_cap);
  c->m1 = zalloc(c->m1_len);
  c->ssa = zalloc(c->ss_len);
  c->ssb = zalloc(c->ss_len);

  return !(c->pka && c->ska && c->pkb && c->skb && c->sta &&
           c->stb && c->m1 && c->ssa && c->ssb);
}

static void ctx_free(struct ctx *c)
{
  free(c->pka);
  free(c->ska);
  free(c->pkb);
  free(c->skb);
  free(c->sta);
  free(c->stb);
  free(c->m1);
  free(c->ssa);
  free(c->ssb);
}

static int setup_exchange(struct ctx *c,
                          unsigned long long *sta_len,
                          unsigned long long *stb_len,
                          unsigned long long *m1_len)
{
  unsigned long long out_pk, out_sk;
  int r;

  *sta_len = 1234;
  r = kex_init_a(c->pka, &out_pk, c->ska, &out_sk, c->sta, sta_len);
  CHECK(r == 0, "kex_init_a succeeds");
  CHECK(out_pk == c->pk_len && out_sk == c->sk_len && *sta_len == 0,
        "kex_init_a output lengths");

  *stb_len = 1234;
  r = kex_init_b(c->pkb, &out_pk, c->skb, &out_sk, c->stb, stb_len);
  CHECK(r == 0, "kex_init_b succeeds");
  CHECK(out_pk == c->pk_len && out_sk == c->sk_len && *stb_len == 0,
        "kex_init_b output lengths");

  *sta_len = 9999;
  *m1_len = 9999;
  r = kex_generate_pass1_msg_a(c->ska, c->sk_len, c->pkb, c->pk_len,
                               c->sta, sta_len, c->m1, m1_len);
  CHECK(r == 1, "pass1 returns protocol complete");
  CHECK(*sta_len == c->ss_len && *m1_len == c->m1_len, "pass1 output lengths");
  return 0;
}

static int test_lengths(void)
{
  CHECK(kex_get_passes_num() == 1, "pass count");
  CHECK(kex_get_pk_len_bytes() == NIKE_SENDABYTES, "pk length");
  CHECK(kex_get_sk_len_bytes() == NIKE_SKBYTES, "sk length");
  CHECK(kex_get_sk_len_bytes() == 2ULL * PARAM_N + kex_get_pk_len_bytes(),
        "sk_api = Encode16(secret) || pk");
  CHECK(kex_get_total_msg_len_bytes() == NIKE_SENDBBYTES, "m1 length");
  CHECK(kex_get_ss_len_bytes() == NIKE_SSBYTES, "ss length");
  CHECK(kex_get_sta_len_bytes() == kex_get_ss_len_bytes(), "sta capacity");
  CHECK(kex_get_stb_len_bytes() == 0, "stb capacity");
  printf("lengths: pk=%llu sk=%llu m1=%llu ss=%llu\n",
         kex_get_pk_len_bytes(), kex_get_sk_len_bytes(),
         kex_get_total_msg_len_bytes(), kex_get_ss_len_bytes());
  return 0;
}

static int test_normal_and_cleanup(void)
{
  struct ctx c;
  unsigned long long sta_len, stb_len, m1_len, ssa_len = 777, ssb_len = 777;
  int r;

  CHECK(ctx_alloc(&c) == 0, "allocate normal buffers");
  seed_drng(0x21);
  CHECK(setup_exchange(&c, &sta_len, &stb_len, &m1_len) == 0, "setup exchange");

  r = kex_derive_ss_b(c.skb, c.sk_len, c.pka, c.pk_len, c.m1, m1_len,
                      c.stb, stb_len, c.ssb, &ssb_len);
  CHECK(r == 0 && ssb_len == c.ss_len, "derive_ss_b succeeds");

  r = kex_derive_ss_a(c.ska, c.sk_len, c.pkb, c.pk_len, NULL, 0,
                      c.sta, sta_len, c.ssa, &ssa_len);
  CHECK(r == 0 && ssa_len == c.ss_len, "derive_ss_a succeeds");
  CHECK(memcmp(c.ssa, c.ssb, (size_t)c.ss_len) == 0, "shared secrets match");
  CHECK(all_zero(c.sta, c.ss_len), "derive_ss_a clears sta");

  ctx_free(&c);
  return 0;
}

static int expect_pass1_error(struct ctx *c,
                              unsigned long long sk_len,
                              unsigned long long pk_len)
{
  unsigned long long sta_len = 55;
  unsigned long long m1_len = 66;
  int r = kex_generate_pass1_msg_a(c->ska, sk_len, c->pkb, pk_len,
                                   c->sta, &sta_len, c->m1, &m1_len);
  CHECK(r < 0, "pass1 rejects bad length");
  CHECK(sta_len == 0 && m1_len == 0, "pass1 clears output lengths on error");
  return 0;
}

static int test_null_and_lengths(void)
{
  struct ctx c;
  unsigned long long sta_len, stb_len, m1_len, out_len;
  unsigned long long pk_out = 1, sk_out = 1, state_out = 1;
  int r;

  CHECK(ctx_alloc(&c) == 0, "allocate negative buffers");
  seed_drng(0x42);
  CHECK(setup_exchange(&c, &sta_len, &stb_len, &m1_len) == 0, "setup negative exchange");

  r = kex_init_a(NULL, &pk_out, c.ska, &sk_out, c.sta, &state_out);
  CHECK(r < 0 && pk_out == 0 && sk_out == 0 && state_out == 0,
        "init_a rejects NULL and clears lengths");
  r = kex_init_b(NULL, &pk_out, c.skb, &sk_out, c.stb, &state_out);
  CHECK(r < 0 && pk_out == 0 && sk_out == 0 && state_out == 0,
        "init_b rejects NULL and clears lengths");

  CHECK(expect_pass1_error(&c, c.sk_len - 1, c.pk_len) == 0, "pass1 sk len-1");
  CHECK(expect_pass1_error(&c, c.sk_len + 1, c.pk_len) == 0, "pass1 sk len+1");
  CHECK(expect_pass1_error(&c, c.sk_len, c.pk_len - 1) == 0, "pass1 pk len-1");
  CHECK(expect_pass1_error(&c, c.sk_len, c.pk_len + 1) == 0, "pass1 pk len+1");

  out_len = 99;
  r = kex_derive_ss_a(c.ska, c.sk_len, c.pkb, c.pk_len, c.m1, 1,
                      c.sta, sta_len, c.ssa, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_a rejects non-empty mb");
  out_len = 99;
  r = kex_derive_ss_a(c.ska, c.sk_len - 1, c.pkb, c.pk_len, NULL, 0,
                      c.sta, sta_len, c.ssa, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_a rejects sk len-1");
  out_len = 99;
  r = kex_derive_ss_a(c.ska, c.sk_len, c.pkb, c.pk_len + 1, NULL, 0,
                      c.sta, sta_len, c.ssa, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_a rejects pk len+1");
  out_len = 99;
  r = kex_derive_ss_a(c.ska, c.sk_len, c.pkb, c.pk_len, NULL, 0,
                      c.sta, sta_len - 1, c.ssa, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_a rejects sta len-1");

  out_len = 99;
  r = kex_derive_ss_b(c.skb, c.sk_len - 1, c.pka, c.pk_len, c.m1, m1_len,
                      c.stb, stb_len, c.ssb, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_b rejects sk len-1");
  out_len = 99;
  r = kex_derive_ss_b(c.skb, c.sk_len, c.pka, c.pk_len + 1, c.m1, m1_len,
                      c.stb, stb_len, c.ssb, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_b rejects pka len+1");
  out_len = 99;
  r = kex_derive_ss_b(c.skb, c.sk_len, c.pka, c.pk_len, c.m1, m1_len - 1,
                      c.stb, stb_len, c.ssb, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_b rejects ma len-1");
  out_len = 99;
  r = kex_derive_ss_b(c.skb, c.sk_len, c.pka, c.pk_len, c.m1, m1_len,
                      c.stb, 1, c.ssb, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_b rejects stb len+1");

  c.skb[0] = 0xff;
  c.skb[1] = 0xff;
  out_len = 99;
  r = kex_derive_ss_b(c.skb, c.sk_len, c.pka, c.pk_len, c.m1, m1_len,
                      c.stb, stb_len, c.ssb, &out_len);
  CHECK(r < 0 && out_len == 0, "derive_ss_b rejects non-canonical sk coefficient");

  ctx_free(&c);
  return 0;
}

static int test_tampered_messages(void)
{
  struct ctx c;
  unsigned long long sta_len, stb_len, m1_len, ssb_len;
  unsigned char *tmp;
  unsigned int i;
  const unsigned long long positions[3] = {
    0,
    NIKE_SEEDBYTES,
    NIKE_SEEDBYTES + NIKE_UPOLYBYTES
  };

  CHECK(ctx_alloc(&c) == 0, "allocate tamper buffers");
  tmp = zalloc(c.m1_len);
  CHECK(tmp != NULL, "allocate tamper message");
  seed_drng(0x63);
  CHECK(setup_exchange(&c, &sta_len, &stb_len, &m1_len) == 0, "setup tamper exchange");

  for(i = 0; i < 3; i++) {
    memcpy(tmp, c.m1, (size_t)c.m1_len);
    tmp[positions[i]] ^= 0x01;
    ssb_len = 123;
    CHECK(kex_derive_ss_b(c.skb, c.sk_len, c.pka, c.pk_len, tmp, m1_len,
                          c.stb, stb_len, c.ssb, &ssb_len) == 0,
          "derive_ss_b handles malformed/tampered m1 without crash");
    CHECK(ssb_len == c.ss_len, "tampered m1 produces exact ss length");
  }

  free(tmp);
  ctx_free(&c);
  return 0;
}

static int test_repeated(void)
{
  int i;
  for(i = 0; i < 3; i++)
    CHECK(test_normal_and_cleanup() == 0, "repeated exchange");
  return 0;
}

int main(void)
{
  int failures = 0;

  printf("MAMBA-NIKE NGCC API check: %s\n", ALGORITHM_INSTANCE);

  failures += test_lengths();
  failures += test_normal_and_cleanup();
  failures += test_null_and_lengths();
  failures += test_tampered_messages();
  failures += test_repeated();

  if(failures == 0) {
    printf("api_check: PASS\n");
    return 0;
  }
  printf("api_check: FAIL (%d test groups)\n", failures);
  return 1;
}
