#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"

static void pack_h(uint8_t *out, const polyveck *h) {
  uint16_t pos[OMEGA];
  unsigned cnt = 0;
  unsigned bound[K];
  const uint16_t posmask = (uint16_t)((1u << POLYVECH_POSBITS) - 1u);

  for(unsigned i = 0; i < K; i++) {
    bound[i] = 0;
    for(unsigned j = 0; j < N; j++) {
      if(h->vec[i].coeffs[j]) {
        pos[cnt++] = (uint16_t)j; 
        bound[i]++;
      }
    }
  }

  while(cnt < OMEGA) pos[cnt++] = 0;

  memset(out, 0, POLYVECH_POSBYTES);
  for(unsigned i = 0; i < OMEGA; i++) {
    uint16_t v = (uint16_t)(pos[i] & posmask);
    unsigned bitpos = i * POLYVECH_POSBITS;
    unsigned bytepos = bitpos >> 3; 
    unsigned shift = bitpos & 7; 
    
    uint16_t cur = (uint16_t)out[bytepos] | (uint16_t)((uint16_t)out[bytepos + 1] << 8);
    cur |= (uint16_t)(v << shift);
    out[bytepos] = (uint8_t)cur;
    out[bytepos + 1] = (uint8_t)(cur >> 8);
  }

  uint8_t *bounds = out + POLYVECH_POSBYTES;
  unsigned idx = 0;
  for(unsigned i = 0; i < K; i++) {
    idx += bound[i];
    bounds[i] = (uint8_t)idx;
  }
}

static int unpack_h(polyveck *h, const uint8_t *in) {
  uint16_t pos[OMEGA];
  const uint8_t *bounds = in + POLYVECH_POSBYTES;
  const uint16_t posmask = (uint16_t)((1u << POLYVECH_POSBITS) - 1u);

#if OMEGA == 85
  if (in[95] & 0xE0)
        return 1;
#elif OMEGA == 90
  if (in[112] & 0xF0)
        return 1;
#endif

  for(unsigned i = 0; i < OMEGA; i++) {
    unsigned bitpos = i * POLYVECH_POSBITS;
    unsigned bytepos = bitpos >> 3;
    unsigned shift = bitpos & 7;
    uint16_t cur = (uint16_t)in[bytepos] | (uint16_t)((uint16_t)in[bytepos + 1] << 8);
    pos[i] = (uint16_t)((cur >> shift) & posmask);
    if(pos[i] >= N) return 1;
  }

  for(unsigned i = 0; i < K; i++)
    for(unsigned j = 0; j < N; j++)
      h->vec[i].coeffs[j] = 0;

  unsigned start = 0;
  for(unsigned lane = 0; lane < K; lane++) {
    unsigned end = bounds[lane];
    if(end < start || end > OMEGA) return 1;
    for(unsigned i = start; i < end; i++) {
      if(i > start && pos[i] <= pos[i-1]) return 1;
      h->vec[lane].coeffs[pos[i]] = 1;
    }
    start = end;
  }
  for(unsigned i = start; i < OMEGA; i++)
    if(pos[i] != 0)
      return 1;

  return 0;
}

void pack_pk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], const uint8_t rho[SEEDBYTES], const polyveck *t1) {
  memcpy(pk, rho, SEEDBYTES);
  for(unsigned i = 0; i < K; i++)
    polyt1_pack(pk + SEEDBYTES + i*POLYT1_PACKEDBYTES, &t1->vec[i]);
}

void unpack_pk(uint8_t rho[SEEDBYTES], polyveck *t1, const uint8_t pk[CRYPTO_PUBLICKEYBYTES]) {
  memcpy(rho, pk, SEEDBYTES);
  for(unsigned i = 0; i < K; i++)
    polyt1_unpack(&t1->vec[i], pk + SEEDBYTES + i*POLYT1_PACKEDBYTES);
}

void pack_sk(uint8_t sk[CRYPTO_SECRETKEYBYTES],
             const uint8_t rho[SEEDBYTES],
             const uint8_t tr[CRHBYTES],
             const polyveck *t0,
             const polyvecl *s1,
             const polyveck *s2)
{
  uint8_t *p = sk;

  memcpy(p, rho, SEEDBYTES); p += SEEDBYTES;
  memcpy(p, tr, CRHBYTES); p += CRHBYTES;

  for(unsigned i = 0; i < L; i++) {
    polyeta_pack(p, &s1->vec[i]);
    p += POLYETA_PACKEDBYTES;
  }

  for(unsigned i = 0; i < K; i++) {
    polyeta_pack(p, &s2->vec[i]);
    p += POLYETA_PACKEDBYTES;
  }

  for(unsigned i = 0; i < K; i++) {
    polyt0_pack(p, &t0->vec[i]);
    p += POLYT0_PACKEDBYTES;
  }
}

void unpack_sk(uint8_t rho[SEEDBYTES],
               uint8_t tr[CRHBYTES],
               polyveck *t0,
               polyvecl *s1,
               polyveck *s2,
               const uint8_t sk[CRYPTO_SECRETKEYBYTES])
{
  const uint8_t *p = sk;

  memcpy(rho, p, SEEDBYTES); p += SEEDBYTES;
  memcpy(tr, p, CRHBYTES); p += CRHBYTES;

  for(unsigned i = 0; i < L; i++) {
    polyeta_unpack(&s1->vec[i], p);
    p += POLYETA_PACKEDBYTES;
  }

  for(unsigned i = 0; i < K; i++) {
    polyeta_unpack(&s2->vec[i], p);
    p += POLYETA_PACKEDBYTES;
  }

  for(unsigned i = 0; i < K; i++) {
    polyt0_unpack(&t0->vec[i], p);
    p += POLYT0_PACKEDBYTES;
  }
}

void pack_sig(uint8_t sig[CRYPTO_BYTES], const uint8_t c[CTILDEBYTES], const polyvecl *z, const polyveck *h) {
  uint8_t *p = sig;
  memcpy(p, c, CTILDEBYTES); p += CTILDEBYTES;

  for(unsigned i = 0; i < L; i++) {
    polyz_pack(p, &z->vec[i]);
    p += POLYZ_PACKEDBYTES;
  }

  pack_h(p, h);
}

int unpack_sig(uint8_t c[CTILDEBYTES], polyvecl *z, polyveck *h, const uint8_t sig[CRYPTO_BYTES]) {
  const uint8_t *p = sig;
  memcpy(c, p, CTILDEBYTES); p += CTILDEBYTES;

  for(unsigned i = 0; i < L; i++) {
    polyz_unpack(&z->vec[i], p);
    p += POLYZ_PACKEDBYTES;
  }

  if(unpack_h(h, p))
    return 1;

  return 0;
}
