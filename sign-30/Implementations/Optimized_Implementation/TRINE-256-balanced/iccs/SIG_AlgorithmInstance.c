#include "SIG_AlgorithmInstance.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"

enum
{
  SIG_ADAPTER_BAD_ARGUMENT = -1,
  SIG_ADAPTER_BAD_LENGTH = -2,
  SIG_ADAPTER_ALLOC_FAILED = -3,
  SIG_ADAPTER_KEYGEN_FAILED = -4,
  SIG_ADAPTER_SIGN_FAILED = -5,
  SIG_ADAPTER_VERIFY_FAILED = -6
};

static void clear_bytes(
    void *ptr,
    size_t len)
{
  volatile uint8_t *bytes = (volatile uint8_t *)ptr;

  if (ptr == NULL)
    return;

  for (size_t i = 0; i < len; i++)
    bytes[i] = 0;
}

unsigned long long sig_get_pk_len_bytes()
{
  return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes()
{
  return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes()
{
  return CRYPTO_BYTES;
}

int sig_keygen(
    unsigned char *pk,
    unsigned long long *pk_len_bytes,
    unsigned char *sk,
    unsigned long long *sk_len_bytes)
{
  if (pk == NULL || pk_len_bytes == NULL ||
      sk == NULL || sk_len_bytes == NULL)
    return SIG_ADAPTER_BAD_ARGUMENT;

  if (*pk_len_bytes < CRYPTO_PUBLICKEYBYTES ||
      *sk_len_bytes < CRYPTO_SECRETKEYBYTES)
    return SIG_ADAPTER_BAD_LENGTH;

  if (crypto_sign_keypair(pk, sk) != 0)
    return SIG_ADAPTER_KEYGEN_FAILED;

  *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
  *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
  return 0;
}

int sig_sign(
    unsigned char *sk,
    unsigned long long sk_len_bytes,
    unsigned char *m,
    unsigned long long m_len_bytes,
    unsigned char *sn,
    unsigned long long *sn_len_bytes)
{
  unsigned char *signed_message = NULL;
  unsigned long long signed_len = 0;
  size_t tmp_len;
  int result = SIG_ADAPTER_SIGN_FAILED;

  if (sk == NULL || sn == NULL || sn_len_bytes == NULL ||
      (m == NULL && m_len_bytes != 0u))
    return SIG_ADAPTER_BAD_ARGUMENT;

  if (sk_len_bytes != CRYPTO_SECRETKEYBYTES ||
      *sn_len_bytes < CRYPTO_BYTES)
    return SIG_ADAPTER_BAD_LENGTH;

  if (m_len_bytes > (unsigned long long)(SIZE_MAX - CRYPTO_BYTES))
    return SIG_ADAPTER_BAD_LENGTH;

  tmp_len = (size_t)m_len_bytes + (size_t)CRYPTO_BYTES;
  signed_message = (unsigned char *)malloc(tmp_len == 0u ? 1u : tmp_len);
  if (signed_message == NULL)
    return SIG_ADAPTER_ALLOC_FAILED;

  if (crypto_sign(
          signed_message,
          &signed_len,
          m,
          m_len_bytes,
          sk) != 0)
    goto cleanup;

  if (signed_len != (unsigned long long)tmp_len)
  {
    result = SIG_ADAPTER_BAD_LENGTH;
    goto cleanup;
  }

  memcpy(sn, signed_message, CRYPTO_BYTES);
  *sn_len_bytes = CRYPTO_BYTES;
  result = 0;

cleanup:
  clear_bytes(signed_message, tmp_len);
  free(signed_message);
  return result;
}

int sig_verify(
    unsigned char *pk,
    unsigned long long pk_len_bytes,
    unsigned char *sn,
    unsigned long long sn_len_bytes,
    unsigned char *m,
    unsigned long long m_len_bytes)
{
  if (pk == NULL || sn == NULL || (m == NULL && m_len_bytes != 0u))
    return SIG_ADAPTER_BAD_ARGUMENT;

  if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES ||
      sn_len_bytes != CRYPTO_BYTES)
    return SIG_ADAPTER_BAD_LENGTH;

  return crypto_sign_verify(
             sn,
             sn_len_bytes,
             m,
             m_len_bytes,
             pk) == 0
             ? 0
             : SIG_ADAPTER_VERIFY_FAILED;
}
