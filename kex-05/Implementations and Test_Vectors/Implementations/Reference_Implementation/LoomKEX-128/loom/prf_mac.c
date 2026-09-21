#include <stdlib.h>
#include <string.h>
#include "prf_mac.h"
#include "symmetric.h"

/*************************************************
* PRF(K, NI || NR) -> SS || Kmac via SHAKE256
* LOOM_DISPUTE: exact PRF not named in AKE PDF; SHAKE256 chosen.
**************************************************/
int loom_prf_derive(uint8_t ss[LOOM_KEM_SSBYTES],
                     uint8_t mki[LOOM_MACBYTES],
                     uint8_t mkr[LOOM_MACBYTES],
                     const uint8_t k[LOOM_KEM_SSBYTES],
                     const uint8_t ni[LOOM_NONCEBYTES],
                     const uint8_t nr[LOOM_NONCEBYTES])
{
  uint8_t ctr = 0;
  uint8_t buf[2*LOOM_KEM_SSBYTES + 2*LOOM_NONCEBYTES + 1] = {0};
  int offset = 0;

  if(!ss || !mki || !mkr || !k || !ni || !nr)
    return -1;

  offset = 0;
  ctr++;
  memcpy(buf + offset, k, LOOM_KEM_SSBYTES);
  offset += LOOM_KEM_SSBYTES;
  memcpy(buf + offset, ni, LOOM_NONCEBYTES);
  offset += LOOM_NONCEBYTES;
  memcpy(buf + offset, nr, LOOM_NONCEBYTES);
  offset += LOOM_NONCEBYTES;
  memcpy(buf + offset, &ctr, 1);
  offset += 1;
  shake256(ss, LOOM_KEM_SSBYTES, buf, offset);

  offset = 0;
  ctr++;
  memcpy(buf + offset, ss, LOOM_KEM_SSBYTES);
  offset += LOOM_KEM_SSBYTES;
  memcpy(buf + offset, k, LOOM_KEM_SSBYTES);
  offset += LOOM_KEM_SSBYTES;
  memcpy(buf + offset, ni, LOOM_NONCEBYTES);
  offset += LOOM_NONCEBYTES;
  memcpy(buf + offset, nr, LOOM_NONCEBYTES);
  offset += LOOM_NONCEBYTES;
  memcpy(buf + offset, &ctr, 1);
  offset += 1;
  shake256(mki, LOOM_MACBYTES, buf, offset);

  offset = 0;
  ctr++;
  memcpy(buf + offset, mki, LOOM_KEM_SSBYTES);
  offset += LOOM_KEM_SSBYTES;
  memcpy(buf + offset, k, LOOM_KEM_SSBYTES);
  offset += LOOM_KEM_SSBYTES;
  memcpy(buf + offset, ni, LOOM_NONCEBYTES);
  offset += LOOM_NONCEBYTES;
  memcpy(buf + offset, nr, LOOM_NONCEBYTES);
  offset += LOOM_NONCEBYTES;
  memcpy(buf + offset, &ctr, 1);
  offset += 1;
  shake256(mkr, LOOM_MACBYTES, buf, offset);

  return 0;
}

static int hmac_shake256(uint8_t *out, size_t outlen,
                        const uint8_t *key, size_t keylen,
                        const uint8_t *msg, size_t msglen)
{
  uint8_t kpad[LOOM_HMAC_BLOCKBYTES];
  uint8_t *buf;
  size_t buflen, i;

  if(outlen == 0)
    return -1;

  memset(kpad, 0, LOOM_HMAC_BLOCKBYTES);
  if(keylen > LOOM_HMAC_BLOCKBYTES)
    shake256(kpad, LOOM_HMAC_BLOCKBYTES, key, keylen);
  else
    memcpy(kpad, key, keylen);

  for(i = 0; i < LOOM_HMAC_BLOCKBYTES; i++)
    kpad[i] ^= 0x36;

  buflen = LOOM_HMAC_BLOCKBYTES + MAX(msglen, outlen);
  buf = (uint8_t *)malloc(buflen);
  if(!buf)
    return -1;
  memcpy(buf, kpad, LOOM_HMAC_BLOCKBYTES);
  memcpy(buf + LOOM_HMAC_BLOCKBYTES, msg, msglen);
  shake256(out, outlen, buf, LOOM_HMAC_BLOCKBYTES + msglen);

  for(i = 0; i < LOOM_HMAC_BLOCKBYTES; i++)
    kpad[i] ^= 0x6a;

  memcpy(buf, kpad, LOOM_HMAC_BLOCKBYTES);
  memcpy(buf + LOOM_HMAC_BLOCKBYTES, out, outlen);
  shake256(out, outlen, buf, LOOM_HMAC_BLOCKBYTES + outlen);
  free(buf);

  return 0;
}

/*************************************************
* MAC(Kmac, input).
**************************************************/
int loom_mac_compute(uint8_t tau[LOOM_MACBYTES],
                      const uint8_t kmac[LOOM_MACBYTES],
                      const uint8_t *input,
                      size_t inlen)
{
  if(inlen == 0 || !input)
    return -1;

  if(hmac_shake256(tau, LOOM_MACBYTES, kmac, LOOM_MACBYTES, input, inlen))
    return -1;

  return 0;
}

int loom_mac_verify(const uint8_t tau[LOOM_MACBYTES],
                    const uint8_t kmac[LOOM_MACBYTES],
                    const uint8_t *input,
                    size_t inlen)
{
  uint8_t cmp[LOOM_MACBYTES];
  size_t i;
  uint8_t diff = 0;

  if(loom_mac_compute(cmp, kmac, input, inlen))
    return -1;

  for(i = 0; i < LOOM_MACBYTES; i++)
    diff |= tau[i] ^ cmp[i];

  return diff ? -1 : 0;
}
