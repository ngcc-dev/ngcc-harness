/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

*/

#include "kem.h"
#include "hash_domain.h"
#include "randombytes.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
  return RRLWR_KEM_PK_LEN;
}

unsigned long long kem_get_sk_len_bytes()
{
  return RRLWR_KEM_SK_LEN;
}

unsigned long long kem_get_ss_len_bytes()
{
  return RRLWR_KEM_SS_LEN;
}

unsigned long long kem_get_ct_len_bytes()
{
  return RRLWR_KEM_CT_LEN;
}

int kem_keygen(
  unsigned char *pk, unsigned long long *pk_len_bytes,
  unsigned char *sk, unsigned long long *sk_len_bytes)
{
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
  unsigned char seedS[RRLWR_SEED_S_LEN];

  // Generate PKE key pair
  GENERATE_RANDOM_BYTES(seedA, RRLWR_PKE_SEED_A_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedS, RRLWR_SEED_S_LEN, &drng_algorithm);
  pke_keygen(pk, sk, seedA, seedS);

  // Copy the PKE public key to the KEM private key
  for(unsigned int i = 0; i < RRLWR_PKE_PK_LEN; i++) {
    sk[RRLWR_PKE_SK_LEN + i] = pk[i];
  }

  // Hash the PKE public key
  RRLWR_KEM_HASH_F(&sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN], pk);

  // Generate the FO secret z
  GENERATE_RANDOM_BYTES(&sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN], RRLWR_KEM_SEED_Z_LEN, &drng_algorithm);

  // Return output lengths
  *sk_len_bytes = RRLWR_KEM_SK_LEN;
  *pk_len_bytes = RRLWR_KEM_PK_LEN;

  return 0;
}

int kem_enc(
  unsigned char *pk, unsigned long long pk_len_bytes,
  unsigned char *ss, unsigned long long *ss_len_bytes,
  unsigned char *ct, unsigned long long *ct_len_bytes)
{
  unsigned char hpk_m[RRLWR_KEM_HPK_LEN + RRLWR_PKE_MESSAGE_LEN]; // (hpk, m)
  unsigned char *hpk = hpk_m;                                      // RRLWR_KEM_HPK_LEN
  unsigned char *m = hpk_m + RRLWR_KEM_HPK_LEN;                   // RRLWR_PKE_MESSAGE_LEN
  unsigned char K_seedSp[RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN];    // (K, Sp)
  unsigned char *K = K_seedSp;                                    // RRLWR_KEM_SS_LEN
  unsigned char *seedSp = K + RRLWR_KEM_SS_LEN;                    // RRLWR_SEED_S_LEN

  // Return failure if public key length is not correct
  if (pk_len_bytes != RRLWR_KEM_PK_LEN) {
    return -1;
  }

  // Generate a random message
  GENERATE_RANDOM_BYTES(m, RRLWR_PKE_MESSAGE_LEN, &drng_algorithm);

  // Hash the PKE public key
  RRLWR_KEM_HASH_F(hpk, pk);

  // Hash (K, seedSp) = H(H(pk), m)
  RRLWR_KEM_HASH_G(K_seedSp, RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN, hpk, m);

  // Encrypt the message
  pke_encrypt(ct, pk, m, seedSp);

  // Copy the shared secret to the output
  for(unsigned int i = 0; i < RRLWR_KEM_SS_LEN; i++) {
    ss[i] = K[i];
  }

  *ss_len_bytes = RRLWR_KEM_SS_LEN;
  *ct_len_bytes = RRLWR_KEM_CT_LEN;

  return 0;
}

int kem_dec(
  unsigned char *sk, unsigned long long sk_len_bytes,
  unsigned char *ct, unsigned long long ct_len_bytes,
  unsigned char *ss, unsigned long long *ss_len_bytes)
{
  unsigned char hpk_mp[RRLWR_KEM_HPK_LEN + RRLWR_PKE_MESSAGE_LEN];                   // (H(pk), m)
  unsigned char *mp = hpk_mp + RRLWR_KEM_HPK_LEN;                                    // RRLWR_PKE_MESSAGE_LEN
  unsigned char K_seedSp[RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN];                      // (K, Sp)
  unsigned char *K = K_seedSp;                                                      // RRLWR_KEM_SS_LEN
  unsigned char *seedSp = K + RRLWR_KEM_SS_LEN;                                      // RRLWR_SEED_S_LEN
  unsigned char Kb[RRLWR_KEM_SS_LEN];                                               // Kbar
  unsigned char ctp[RRLWR_KEM_CT_LEN];                                              // ct_prime
  unsigned char z[RRLWR_KEM_SEED_Z_LEN];                                            // RRLWR_KEM_SEED_Z_LEN
  unsigned char *pk = sk + RRLWR_PKE_SK_LEN;                                        // RRLWR_PKE_PK_LEN

  // Return failure if private key or ciphertext length is not correct
  if (sk_len_bytes != RRLWR_KEM_SK_LEN || ct_len_bytes != RRLWR_KEM_CT_LEN) {
    return -1;
  }
  
  // Copy H(pk) from the private key
  for(unsigned int i = 0; i < RRLWR_KEM_HPK_LEN; i++) {
    hpk_mp[i] = sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + i];
  }

  // Decrypt the message m
  pke_decrypt(mp, ct, sk);

  // Hash (K, seedSp) = H(H(pk), m)
  RRLWR_KEM_HASH_G(K_seedSp, RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN, hpk_mp, mp);

  // Re-encrypt the ciphertext
  pke_encrypt(ctp, pk, mp, seedSp);

  // Hash (c, z)
  for(unsigned int i = 0; i < RRLWR_KEM_SEED_Z_LEN; i++) {
    z[i] = sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN + i];
  }
  RRLWR_KEM_HASH_H(Kb, RRLWR_KEM_SS_LEN, ct, z);

  // Constant-time select either K (ct = ctp) or Kb (ct != ctp) into the shared secret
  uint8_t compare = ct_cmp(ct, ctp);
  for(unsigned int i = 0; i < RRLWR_KEM_SS_LEN; i++) {
    ss[i] = K[i] ^ ((K[i] ^ Kb[i]) & compare);
  }

  // Return the shared secret length
  *ss_len_bytes = RRLWR_KEM_SS_LEN;

  return 0;
}
