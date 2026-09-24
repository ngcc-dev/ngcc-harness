/*
MAMBA-NIKE KEX Algorithm Instance
Bridges MAMBA-NIKE to the NGCC KEX API.

Protocol: responder-static, initiator-ephemeral one-pass NIKE.
The initiator ska/pka objects are generated only for NGCC harness
compatibility and are not used by the core mathematical protocol.
*/

#include "KEX_AlgorithmInstance.h"
#include "nike.h"
#include "params.h"
#include "randombytes.h"
#include <stdint.h>
#include <string.h>

#if defined(KAT_BUILD)
#include "drng.h"
#else
#include <errno.h>
#if defined(_WIN32)
#include <windows.h>
#include <ntsecapi.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#endif

#define SENDA_BYTES       NIKE_SENDABYTES
#define SENDB_BYTES       NIKE_SENDBBYTES
#define SK_BYTES          NIKE_SKBYTES
#define SS_BYTES          NIKE_SSBYTES
#define STA_BYTES         SS_BYTES
#define STB_BYTES         0

#define KEX_ERR_NULL      (-1)
#define KEX_ERR_LENGTH    (-2)
#define KEX_ERR_RNG       (-3)
#define KEX_ERR_ENCODING  (-4)

#if defined(KAT_BUILD)
extern DRNG_ctx drng_algorithm;
#endif

static void secure_zero(void *p, unsigned long long n)
{
  volatile unsigned char *v = (volatile unsigned char *)p;
  while(n--)
    *v++ = 0;
}

static void set_len(unsigned long long *p, unsigned long long v)
{
  if(p)
    *p = v;
}

static void zero_len2(unsigned long long *a, unsigned long long *b)
{
  set_len(a, 0);
  set_len(b, 0);
}

static void zero_keypair_lengths(unsigned long long *pk_len,
                                 unsigned long long *sk_len,
                                 unsigned long long *state_len)
{
  set_len(pk_len, 0);
  set_len(sk_len, 0);
  set_len(state_len, 0);
}

int nike_randombytes(unsigned char *x, unsigned long long xlen)
{
  if(x == 0 && xlen != 0)
    return KEX_ERR_NULL;

  if(xlen == 0)
    return 0;

#if defined(KAT_BUILD)
  return get_random_number(&drng_algorithm, x, xlen * 8) == 0 ? 0 : KEX_ERR_RNG;
#elif defined(_WIN32)
  while(xlen > 0) {
    ULONG chunk = xlen > 0x7ffff000ULL ? 0x7ffff000UL : (ULONG)xlen;
    if(!SystemFunction036(x, chunk))
      return KEX_ERR_RNG;
    x += chunk;
    xlen -= chunk;
  }
  return 0;
#else
  int fd = open("/dev/urandom", O_RDONLY);
  if(fd < 0)
    return KEX_ERR_RNG;

  while(xlen > 0) {
    size_t want = xlen > 1048576ULL ? 1048576U : (size_t)xlen;
    ssize_t got = read(fd, x, want);
    if(got < 0) {
      if(errno == EINTR)
        continue;
      close(fd);
      return KEX_ERR_RNG;
    }
    if(got == 0) {
      close(fd);
      return KEX_ERR_RNG;
    }
    x += (unsigned long long)got;
    xlen -= (unsigned long long)got;
  }

  close(fd);
  return 0;
#endif
}

void randombytes(unsigned char *x, unsigned long long xlen)
{
  (void)nike_randombytes(x, xlen);
}

unsigned long long kex_get_passes_num(void)     { return 1; }
unsigned long long kex_get_pk_len_bytes(void)   { return SENDA_BYTES; }
unsigned long long kex_get_sk_len_bytes(void)   { return SK_BYTES; }
unsigned long long kex_get_sta_len_bytes(void)  { return STA_BYTES; }
unsigned long long kex_get_stb_len_bytes(void)  { return STB_BYTES; }
unsigned long long kex_get_ss_len_bytes(void)   { return SS_BYTES; }
unsigned long long kex_get_total_msg_len_bytes(void) { return SENDB_BYTES; }

static void poly_to_bytes(unsigned char *out, const poly *p)
{
  int i;
  for(i = 0; i < PARAM_N; i++) {
    uint16_t t = p->coeffs[i] & (PARAM_Q - 1);
    out[2 * i] = (unsigned char)(t & 0xff);
    out[2 * i + 1] = (unsigned char)(t >> 8);
  }
}

static int bytes_to_poly_checked(poly *p, const unsigned char *in)
{
  int i;
  for(i = 0; i < PARAM_N; i++) {
    uint16_t t = (uint16_t)in[2 * i] | ((uint16_t)in[2 * i + 1] << 8);
    if(t >= PARAM_Q) {
      secure_zero(p, sizeof(*p));
      return KEX_ERR_ENCODING;
    }
    p->coeffs[i] = t;
  }
  return 0;
}

static int check_sk_canonical(const unsigned char *sk)
{
  poly tmp;
  int ret = bytes_to_poly_checked(&tmp, sk);
  secure_zero(&tmp, sizeof(tmp));
  return ret;
}

int kex_init_a(
  unsigned char *pka, unsigned long long *pka_len_bytes,
  unsigned char *ska, unsigned long long *ska_len_bytes,
  unsigned char *sta, unsigned long long *sta_len_bytes)
{
  poly sk_poly;

  zero_keypair_lengths(pka_len_bytes, ska_len_bytes, sta_len_bytes);
  if(pka == 0 || pka_len_bytes == 0 || ska == 0 || ska_len_bytes == 0 ||
     sta == 0 || sta_len_bytes == 0)
    return KEX_ERR_NULL;

  if(nike_keygen(pka, &sk_poly) != 0) {
    secure_zero(pka, SENDA_BYTES);
    secure_zero(ska, SK_BYTES);
    secure_zero(sta, STA_BYTES);
    secure_zero(&sk_poly, sizeof(sk_poly));
    return KEX_ERR_RNG;
  }

  poly_to_bytes(ska, &sk_poly);
  memcpy(ska + POLY_BYTES, pka, SENDA_BYTES);
  secure_zero(sta, STA_BYTES);

  *pka_len_bytes = SENDA_BYTES;
  *ska_len_bytes = SK_BYTES;
  *sta_len_bytes = 0;

  secure_zero(&sk_poly, sizeof(sk_poly));
  return 0;
}

int kex_init_b(
  unsigned char *pkb, unsigned long long *pkb_len_bytes,
  unsigned char *skb, unsigned long long *skb_len_bytes,
  unsigned char *stb, unsigned long long *stb_len_bytes)
{
  poly sk_poly;

  zero_keypair_lengths(pkb_len_bytes, skb_len_bytes, stb_len_bytes);
  if(pkb == 0 || pkb_len_bytes == 0 || skb == 0 || skb_len_bytes == 0 ||
     stb_len_bytes == 0)
    return KEX_ERR_NULL;
  if(STB_BYTES != 0 && stb == 0)
    return KEX_ERR_NULL;

  if(nike_keygen(pkb, &sk_poly) != 0) {
    secure_zero(pkb, SENDA_BYTES);
    secure_zero(skb, SK_BYTES);
    if(stb)
      secure_zero(stb, STB_BYTES);
    secure_zero(&sk_poly, sizeof(sk_poly));
    return KEX_ERR_RNG;
  }

  poly_to_bytes(skb, &sk_poly);
  memcpy(skb + POLY_BYTES, pkb, SENDA_BYTES);
  if(stb)
    secure_zero(stb, STB_BYTES);

  *pkb_len_bytes = SENDA_BYTES;
  *skb_len_bytes = SK_BYTES;
  *stb_len_bytes = 0;

  secure_zero(&sk_poly, sizeof(sk_poly));
  return 0;
}

int kex_generate_pass1_msg_a(
  unsigned char *ska, unsigned long long ska_len_bytes,
  unsigned char *pkb, unsigned long long pkb_len_bytes,
  unsigned char *sta, unsigned long long *sta_len_bytes,
  unsigned char *m1, unsigned long long *m1_len_bytes)
{
  unsigned char ss[SS_BYTES];

  zero_len2(sta_len_bytes, m1_len_bytes);
  if(ska == 0 || pkb == 0 || sta == 0 || sta_len_bytes == 0 ||
     m1 == 0 || m1_len_bytes == 0)
    return KEX_ERR_NULL;
  if(ska_len_bytes != SK_BYTES || pkb_len_bytes != SENDA_BYTES)
    return KEX_ERR_LENGTH;
  if(check_sk_canonical(ska) != 0)
    return KEX_ERR_ENCODING;

  if(nike_sharedb(ss, m1, pkb) != 0) {
    secure_zero(ss, sizeof(ss));
    secure_zero(sta, STA_BYTES);
    secure_zero(m1, SENDB_BYTES);
    return KEX_ERR_RNG;
  }

  memcpy(sta, ss, SS_BYTES);
  *sta_len_bytes = SS_BYTES;
  *m1_len_bytes = SENDB_BYTES;
  secure_zero(ss, sizeof(ss));
  return 1;
}

int kex_generate_pass2_msg_b(
  unsigned char *skb, unsigned long long skb_len_bytes,
  unsigned char *pka, unsigned long long pka_len_bytes,
  unsigned char *m1, unsigned long long m1_len_bytes,
  unsigned char *stb, unsigned long long *stb_len_bytes,
  unsigned char *m2, unsigned long long *m2_len_bytes)
{
  (void)skb;
  (void)skb_len_bytes;
  (void)pka;
  (void)pka_len_bytes;
  (void)m1;
  (void)m1_len_bytes;

  zero_len2(stb_len_bytes, m2_len_bytes);
  if(stb_len_bytes == 0 || m2_len_bytes == 0)
    return KEX_ERR_NULL;
  if(stb)
    secure_zero(stb, STB_BYTES);
  if(m2)
    secure_zero(m2, SENDB_BYTES);
  return 0;
}

int kex_generate_pass3_msg_a(
  unsigned char *ska, unsigned long long ska_len_bytes,
  unsigned char *pkb, unsigned long long pkb_len_bytes,
  unsigned char *m2, unsigned long long m2_len_bytes,
  unsigned char *sta, unsigned long long *sta_len_bytes,
  unsigned char *m3, unsigned long long *m3_len_bytes)
{
  (void)ska;
  (void)ska_len_bytes;
  (void)pkb;
  (void)pkb_len_bytes;
  (void)m2;
  (void)m2_len_bytes;

  zero_len2(sta_len_bytes, m3_len_bytes);
  if(sta_len_bytes == 0 || m3_len_bytes == 0)
    return KEX_ERR_NULL;
  if(sta)
    secure_zero(sta, STA_BYTES);
  if(m3)
    secure_zero(m3, SENDB_BYTES);
  return 0;
}

int kex_derive_ss_a(
  unsigned char *ska, unsigned long long ska_len_bytes,
  unsigned char *pkb, unsigned long long pkb_len_bytes,
  unsigned char *mb, unsigned long long mb_len_bytes,
  unsigned char *sta, unsigned long long sta_len_bytes,
  unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
  set_len(ssa_len_bytes, 0);
  if(ska == 0 || pkb == 0 || sta == 0 || ssa == 0 || ssa_len_bytes == 0)
    return KEX_ERR_NULL;
  if(mb_len_bytes != 0 && mb == 0)
    return KEX_ERR_NULL;
  if(ska_len_bytes != SK_BYTES || pkb_len_bytes != SENDA_BYTES ||
     mb_len_bytes != 0 || sta_len_bytes != SS_BYTES)
    return KEX_ERR_LENGTH;
  if(check_sk_canonical(ska) != 0)
    return KEX_ERR_ENCODING;

  memcpy(ssa, sta, SS_BYTES);
  *ssa_len_bytes = SS_BYTES;
  secure_zero(sta, SS_BYTES);
  return 0;
}

int kex_derive_ss_b(
  unsigned char *skb, unsigned long long skb_len_bytes,
  unsigned char *pka, unsigned long long pka_len_bytes,
  unsigned char *ma, unsigned long long ma_len_bytes,
  unsigned char *stb, unsigned long long stb_len_bytes,
  unsigned char *ssb, unsigned long long *ssb_len_bytes)
{
  poly sk_poly;
  const unsigned char *pkb;

  set_len(ssb_len_bytes, 0);
  if(skb == 0 || pka == 0 || ma == 0 || ssb == 0 || ssb_len_bytes == 0)
    return KEX_ERR_NULL;
  if(stb_len_bytes != 0 && stb == 0)
    return KEX_ERR_NULL;
  if(skb_len_bytes != SK_BYTES || pka_len_bytes != SENDA_BYTES ||
     ma_len_bytes != SENDB_BYTES || stb_len_bytes != STB_BYTES)
    return KEX_ERR_LENGTH;
  if(bytes_to_poly_checked(&sk_poly, skb) != 0)
    return KEX_ERR_ENCODING;

  pkb = skb + POLY_BYTES;
  if(nike_shareda(ssb, &sk_poly, pkb, ma) != 0) {
    secure_zero(&sk_poly, sizeof(sk_poly));
    secure_zero(ssb, SS_BYTES);
    return KEX_ERR_ENCODING;
  }

  *ssb_len_bytes = SS_BYTES;
  secure_zero(&sk_poly, sizeof(sk_poly));
  return 0;
}
