/* MAMBA-Viper implementation and implementation support layer where applicable.
 * FO KEM using SHAKE128 hashes/KDF and deterministic re-encryption.
 */

#include "api.h"
#include "fips202.h"
#include "rng.h"
#include "verify.h"
#include "viper.h"
#include <string.h>

static void hbytes(unsigned char *out, unsigned long long outlen, const unsigned char *in, unsigned long long inlen) {
  shake128(out, outlen, in, inlen);
}

static void h32(unsigned char out[32], const unsigned char *in, unsigned long long inlen) {
  hbytes(out, 32, in, inlen);
}

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk) {
  unsigned char rho[32], sseed[32];
  randombytes(rho, 32);
  randombytes(sseed, 32);
  viper_pke_keypair(pk, sk, rho, sseed);
  memcpy(sk + VIPER_SECRETKEY_PKE_BYTES, pk, VIPER_PUBLICKEYBYTES);
  h32(sk + VIPER_SECRETKEY_PKE_BYTES + VIPER_PUBLICKEYBYTES, pk, VIPER_PUBLICKEYBYTES);
  randombytes(sk + VIPER_SECRETKEY_PKE_BYTES + VIPER_PUBLICKEYBYTES + VIPER_HBYTES, VIPER_SSBYTES);
  return 0;
}

int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk) {
  unsigned char m[VIPER_MSGBYTES], mh[VIPER_MSGBYTES], hpk[32], buf[VIPER_MSGBYTES + 32], kr[VIPER_SSBYTES + VIPER_FALLBACK_KEY_BYTES], hct[32], omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES], kdfin[VIPER_SSBYTES + 32];
  randombytes(m, VIPER_MSGBYTES);
  hbytes(mh, VIPER_MSGBYTES, m, VIPER_MSGBYTES);
  memcpy(m, mh, VIPER_MSGBYTES);
  h32(hpk, pk, VIPER_PUBLICKEYBYTES);
  memcpy(buf, m, VIPER_MSGBYTES);
  memcpy(buf + VIPER_MSGBYTES, hpk, 32);
  shake128(kr, sizeof(kr), buf, sizeof(buf)); /* kr[0:SSBYTES]=kbar, remainder=sigma */
  h32(omega + VIPER_FALLBACK_KEY_BYTES, kr, VIPER_SSBYTES);   /* public mu bound to kbar */
  memcpy(omega, kr + VIPER_SSBYTES, VIPER_FALLBACK_KEY_BYTES);
  viper_pke_enc(ct, pk, m, omega);
  h32(hct, ct, VIPER_CIPHERTEXTBYTES);
  memcpy(kdfin, kr, VIPER_SSBYTES);
  memcpy(kdfin + VIPER_SSBYTES, hct, 32);
  hbytes(ss, VIPER_SSBYTES, kdfin, sizeof(kdfin));
  return 0;
}

int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk) {
  unsigned char m[VIPER_MSGBYTES], buf[VIPER_MSGBYTES + 32], kr[VIPER_SSBYTES + VIPER_FALLBACK_KEY_BYTES], hct[32], kdfin[VIPER_SSBYTES + 32];
  const unsigned char *pk = sk + VIPER_SECRETKEY_PKE_BYTES;
  const unsigned char *hpk = sk + VIPER_SECRETKEY_PKE_BYTES + VIPER_PUBLICKEYBYTES;
  const unsigned char *z = hpk + VIPER_HBYTES;
  viper_pke_dec(m, sk, ct);
  memcpy(buf, m, VIPER_MSGBYTES);
  memcpy(buf + VIPER_MSGBYTES, hpk, 32);
  shake128(kr, sizeof(kr), buf, sizeof(buf));
  int ok = viper_reencrypt_check(ct, pk, m, kr + VIPER_SSBYTES);
  h32(hct, ct, VIPER_CIPHERTEXTBYTES);
  memcpy(kdfin, ok ? kr : z, VIPER_SSBYTES);
  memcpy(kdfin + VIPER_SSBYTES, hct, 32);
  hbytes(ss, VIPER_SSBYTES, kdfin, sizeof(kdfin));
  return 0;
}
