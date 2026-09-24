#include "ccapke.h"

#include <stdlib.h>

#include "aes256ctr.h"
#include "auxfunc.h"
#include "ccakem.h"

static const uint8_t OTP_NONCE[12] = {0x08};

static void derive_aes_key(uint8_t aes_key[32],
                           const uint8_t kem_key[CCAKEM_KEY_SIZE]) {
  pseudoXOF(8ULL * 32, kem_key, 8ULL * CCAKEM_KEY_SIZE, aes_key);
}

void ccapke_keygen(uint8_t pk[CCAPKE_PK_SIZE],
                   uint8_t sk[CCAPKE_SK_SIZE],
                   const uint8_t coin[PARAMS_SEED_SIZE]) {
  ccakem_keygen(pk, sk, coin);
}

void ccapke_encrypt(uint8_t **ct, int *ctlen,
                    const uint8_t pk[CCAPKE_PK_SIZE],
                    const uint8_t *msg, int msglen,
                    const uint8_t coin[PARAMS_RAND_SIZE]) {
  uint8_t key[CCAKEM_KEY_SIZE];
  uint8_t aes_key[32];

  *ctlen = CCAKEM_CT_SIZE + msglen;
  *ct = (uint8_t *) malloc((size_t) *ctlen);
  if(*ct == NULL) {
    abort();
  }

  ccakem_encaps(key, *ct, pk, coin);
  derive_aes_key(aes_key, key);
  aes256ctr_prf(*ct + CCAKEM_CT_SIZE, (size_t) msglen, aes_key, OTP_NONCE);
  for(int i = 0; i < msglen; ++i) {
    (*ct)[CCAKEM_CT_SIZE + i] ^= msg[i];
  }
}

void ccapke_decrypt(uint8_t **msg, int *msglen,
                    const uint8_t sk[CCAPKE_SK_SIZE],
                    const uint8_t *ct, int ctlen) {
  uint8_t key[CCAKEM_KEY_SIZE];
  uint8_t aes_key[32];

  if(*msg != NULL) {
    free(*msg);
  }

  *msglen = ctlen - CCAKEM_CT_SIZE;
  *msg = (uint8_t *) malloc((size_t) *msglen);
  if(*msg == NULL) {
    abort();
  }

  ccakem_decaps(key, sk, ct);
  derive_aes_key(aes_key, key);
  aes256ctr_prf(*msg, (size_t) *msglen, aes_key, OTP_NONCE);
  for(int i = 0; i < *msglen; ++i) {
    (*msg)[i] ^= ct[CCAKEM_CT_SIZE + i];
  }
}
