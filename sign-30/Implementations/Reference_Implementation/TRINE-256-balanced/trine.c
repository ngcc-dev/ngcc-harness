#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "canonical.h"
#include "corank1.h"
#include "hashkdf.h"
#include "matrixmod.h"
#include "params.h"
#include "randombytes.h"
#include "triform.h"
#include "trine_codec.h"
#include "trine_expand.h"
#include "util.h"

static const uint8_t TRINE_ROUND_DOMAIN[] =
    "MEDS2END-TRINE-ROUND-v1";

static void trine_secure_clear(
    void *ptr,
    size_t len)
{
  volatile uint8_t *bytes = (volatile uint8_t *)ptr;

  if (ptr == NULL)
    return;

  for (size_t i = 0; i < len; i++)
    bytes[i] = 0;
}

static void trine_store_u32_le(
    uint8_t out[4],
    uint32_t value)
{
  out[0] = (uint8_t)(value & 0xffu);
  out[1] = (uint8_t)((value >> 8) & 0xffu);
  out[2] = (uint8_t)((value >> 16) & 0xffu);
  out[3] = (uint8_t)((value >> 24) & 0xffu);
}

static int trine_init_round_xof(
    trine_xof_state *xof,
    const uint8_t round_seed[TRINE_round_seed_bytes],
    const uint8_t salt[TRINE_salt_bytes],
    uint32_t round_index)
{
  uint8_t round_index_le[4];

  trine_store_u32_le(round_index_le, round_index);

  if (trine_xof_init(xof) != 0 ||
      trine_xof_absorb(
          xof,
          TRINE_ROUND_DOMAIN,
          sizeof(TRINE_ROUND_DOMAIN) - 1u) != 0 ||
      trine_xof_absorb(xof, salt, TRINE_salt_bytes) != 0 ||
      trine_xof_absorb(xof, round_seed, TRINE_round_seed_bytes) != 0 ||
      trine_xof_absorb(xof, round_index_le, sizeof(round_index_le)) != 0 ||
      trine_xof_finalize(xof) != 0)
  {
    trine_xof_release(xof);
    return -1;
  }

  return 0;
}

static void *trine_alloc_array(
    size_t count,
    size_t element_size)
{
  if (element_size != 0u && count > SIZE_MAX / element_size)
    return NULL;

  return malloc(count * element_size);
}

static int trine_message_len_to_size(
    size_t *out,
    unsigned long long value)
{
  if (value > (unsigned long long)SIZE_MAX)
    return -1;

  *out = (size_t)value;
  return 0;
}

static int trine_challenges_valid(
    const trine_challenge_t challenges[TRINE_r])
{
  size_t nonbase_count = 0;
  size_t base_count = 0;

  for (int i = 0; i < TRINE_r; i++)
  {
    if (challenges[i] < TRINE_X)
      nonbase_count++;
    else if (challenges[i] == TRINE_BASE_FORM_INDEX)
      base_count++;
    else
      return 0;
  }

  return nonbase_count == TRINE_K &&
         base_count == TRINE_BASE_SEED_COUNT;
}

static int trine_challenges_equal(
    const trine_challenge_t a[TRINE_r],
    const trine_challenge_t b[TRINE_r])
{
  for (int i = 0; i < TRINE_r; i++)
    if (a[i] != b[i])
      return 0;

  return 1;
}

static int trine_absorb_canonical_form(
    trine_hash_state *transcript,
    uint8_t *encoded_buffer,
    const Fq *psi)
{
  if (trine_codec_encode_triform(
          encoded_buffer,
          TRINE_TRIFORM_BYTES,
          psi,
          TRINE_n) != 0)
    return -1;

  return trine_hash_absorb(transcript, encoded_buffer, TRINE_TRIFORM_BYTES);
}

static int trine_derive_round_commitment_vartime(
    Fq *out_a,
    Fq *out_psi,
    const Fq *base_form,
    const uint8_t round_seed[TRINE_round_seed_bytes],
    const uint8_t salt[TRINE_salt_bytes],
    uint32_t round_index)
{
  if (out_psi == NULL ||
      base_form == NULL ||
      round_seed == NULL ||
      salt == NULL)
    return -1;

  trine_xof_state xof;
  Fq a_tmp[TRINE_n];

  if (trine_init_round_xof(&xof, round_seed, salt, round_index) != 0)
    return -1;

  for (;;)
  {
    if (corank1_cal_vartime(a_tmp, base_form, &xof, TRINE_n) != 0)
    {
      trine_xof_release(&xof);
      return -1;
    }

    if (canonical_form_vartime(out_psi, base_form, a_tmp, TRINE_n) == 0)
    {
      if (out_a != NULL)
        memcpy(out_a, a_tmp, sizeof(a_tmp));

      trine_secure_clear(a_tmp, sizeof(a_tmp));
      trine_xof_release(&xof);
      return 0;
    }
  }
}

int crypto_sign_keypair(
    unsigned char *pk,
    unsigned char *sk)
{
  int result = -1;
  uint8_t secret_seed[TRINE_secret_seed_bytes] = {0};
  uint8_t public_seed[TRINE_public_seed_bytes] = {0};
  uint8_t sk_tmp[TRINE_SK_BYTES] = {0};
  const size_t form_elements = triform_element_count(TRINE_n);

  Fq *base_form = NULL;
  Fq *nonbase_forms = NULL;
  uint8_t *pk_tmp = NULL;
  Fq A[TRINE_n * TRINE_n];
  Fq B[TRINE_n * TRINE_n];
  Fq C[TRINE_n * TRINE_n];

  if (pk == NULL || sk == NULL)
    goto cleanup;

  base_form = trine_alloc_array(form_elements, sizeof(*base_form));
  nonbase_forms = trine_alloc_array(
      (size_t)TRINE_X * form_elements,
      sizeof(*nonbase_forms));
  pk_tmp = trine_alloc_array(TRINE_PK_BYTES, sizeof(*pk_tmp));

  if (base_form == NULL ||
      nonbase_forms == NULL ||
      pk_tmp == NULL)
    goto cleanup;

  if (randombytes(secret_seed, sizeof(secret_seed)) != RANDOMBYTES_SUCCESS)
    goto cleanup;

  if (trine_expand_public_seed(public_seed, secret_seed) != 0)
    goto cleanup;

  if (trine_expand_base_form(base_form, public_seed, TRINE_n) != 0)
    goto cleanup;

  for (uint32_t i = 0; i < (uint32_t)TRINE_X; i++)
  {
    if (trine_expand_secret_matrix_pair_vartime(
            A, NULL, secret_seed, TRINE_MATRIX_A, i, TRINE_n) != 0)
      goto cleanup;

    if (trine_expand_secret_matrix_pair_vartime(
            B, NULL, secret_seed, TRINE_MATRIX_B, i, TRINE_n) != 0)
      goto cleanup;

    if (trine_expand_secret_matrix_pair_vartime(
            C, NULL, secret_seed, TRINE_MATRIX_C, i, TRINE_n) != 0)
      goto cleanup;

    triform_action_pullback(
        nonbase_forms + (size_t)i * form_elements,
        base_form,
        A,
        B,
        C,
        TRINE_n);
  }

  if (trine_codec_encode_public_key(
          pk_tmp,
          TRINE_PK_BYTES,
          public_seed,
          nonbase_forms) != 0)
    goto cleanup;

  if (trine_codec_encode_secret_key(
          sk_tmp,
          sizeof(sk_tmp),
          secret_seed) != 0)
    goto cleanup;

  memcpy(pk, pk_tmp, TRINE_PK_BYTES);
  memcpy(sk, sk_tmp, TRINE_SK_BYTES);
  result = 0;

cleanup:
  trine_secure_clear(secret_seed, sizeof(secret_seed));
  trine_secure_clear(sk_tmp, sizeof(sk_tmp));
  trine_secure_clear(A, sizeof(A));
  trine_secure_clear(B, sizeof(B));
  trine_secure_clear(C, sizeof(C));
  free(base_form);
  free(nonbase_forms);
  free(pk_tmp);
  return result;
}

int crypto_sign(
    unsigned char *sm,
    unsigned long long *smlen,
    const unsigned char *m,
    unsigned long long mlen,
    const unsigned char *sk)
{
  int result = -1;
  size_t message_len = 0;
  uint8_t secret_seed[TRINE_secret_seed_bytes] = {0};
  uint8_t public_seed[TRINE_public_seed_bytes] = {0};
  uint8_t digest[TRINE_digest_bytes] = {0};
  uint8_t salt[TRINE_salt_bytes] = {0};
  trine_challenge_t challenges[TRINE_r];
  const size_t form_elements = triform_element_count(TRINE_n);
  const size_t matrix_elements = (size_t)TRINE_n * (size_t)TRINE_n;
  const size_t response_elements = (size_t)TRINE_K * (size_t)TRINE_n;

  Fq *base_form = NULL;
  Fq *a_all = NULL;
  Fq *a_inverse_cache = NULL;
  Fq *responses = NULL;
  Fq *psi = NULL;
  uint8_t *a_inverse_ready = NULL;
  uint8_t *round_seeds = NULL;
  uint8_t *base_seeds = NULL;
  uint8_t *encoded_psi = NULL;
  uint8_t *signature_tmp = NULL;
  trine_hash_state transcript;
  int transcript_active = 0;

  if (smlen != NULL)
    *smlen = 0;

  if (sm == NULL || smlen == NULL || sk == NULL)
    goto cleanup;

  if (m == NULL && mlen != 0u)
    goto cleanup;

  if (trine_message_len_to_size(&message_len, mlen) != 0)
    goto cleanup;

  if (mlen > ULLONG_MAX - (unsigned long long)TRINE_SIG_BYTES)
    goto cleanup;

  if (trine_codec_decode_secret_key(secret_seed, sk, TRINE_SK_BYTES) != 0)
    goto cleanup;

  base_form = trine_alloc_array(form_elements, sizeof(*base_form));
  a_all = trine_alloc_array((size_t)TRINE_r * (size_t)TRINE_n, sizeof(*a_all));
  a_inverse_cache = trine_alloc_array(
      (size_t)TRINE_X * matrix_elements,
      sizeof(*a_inverse_cache));
  a_inverse_ready = calloc((size_t)TRINE_X, sizeof(*a_inverse_ready));
  responses = trine_alloc_array(response_elements, sizeof(*responses));
  psi = trine_alloc_array(form_elements, sizeof(*psi));
  round_seeds = trine_alloc_array(
      (size_t)TRINE_r * (size_t)TRINE_round_seed_bytes,
      sizeof(*round_seeds));
  base_seeds = trine_alloc_array(TRINE_BASE_SEED_BYTES, sizeof(*base_seeds));
  encoded_psi = trine_alloc_array(TRINE_TRIFORM_BYTES, sizeof(*encoded_psi));
  signature_tmp = trine_alloc_array(TRINE_SIG_BYTES, sizeof(*signature_tmp));

  if (base_form == NULL ||
      a_all == NULL ||
      a_inverse_cache == NULL ||
      a_inverse_ready == NULL ||
      responses == NULL ||
      psi == NULL ||
      round_seeds == NULL ||
      base_seeds == NULL ||
      encoded_psi == NULL ||
      signature_tmp == NULL)
    goto cleanup;

  if (trine_expand_public_seed(public_seed, secret_seed) != 0)
    goto cleanup;

  if (trine_expand_base_form(base_form, public_seed, TRINE_n) != 0)
    goto cleanup;

  if (randombytes(salt, sizeof(salt)) != RANDOMBYTES_SUCCESS)
    goto cleanup;

  if (randombytes(
          round_seeds,
          (unsigned long long)TRINE_r * TRINE_round_seed_bytes) != RANDOMBYTES_SUCCESS)
    goto cleanup;

  if (trine_hash_init(&transcript) != 0)
    goto cleanup;
  transcript_active = 1;
  if (message_len != 0u &&
      trine_hash_absorb(&transcript, m, message_len) != 0)
    goto cleanup;

  for (int round = 0; round < TRINE_r; round++)
  {
    uint8_t *round_seed =
        round_seeds + (size_t)round * (size_t)TRINE_round_seed_bytes;
    Fq *round_a = a_all + (size_t)round * (size_t)TRINE_n;

    if (trine_derive_round_commitment_vartime(
            round_a,
            psi,
            base_form,
            round_seed,
            salt,
            (uint32_t)round) != 0)
      goto cleanup;

    if (trine_absorb_canonical_form(&transcript, encoded_psi, psi) != 0)
      goto cleanup;
  }

  if (trine_hash_finalize(&transcript, digest, sizeof(digest)) != 0)
    goto cleanup;

  if (trine_parse_hash(
          digest,
          sizeof(digest),
          challenges,
          TRINE_r) != 0)
    goto cleanup;

  if (!trine_challenges_valid(challenges))
    goto cleanup;

  size_t response_index = 0;
  size_t base_seed_index = 0;

  for (int round = 0; round < TRINE_r; round++)
  {
    const trine_challenge_t challenge = challenges[round];

    if (challenge < TRINE_X)
    {
      const size_t matrix_offset = (size_t)challenge * matrix_elements;
      Fq *response = responses + response_index * (size_t)TRINE_n;
      const Fq *round_a = a_all + (size_t)round * (size_t)TRINE_n;

      if (!a_inverse_ready[challenge])
      {
        if (trine_expand_secret_matrix_pair_vartime(
                NULL,
                a_inverse_cache + matrix_offset,
                secret_seed,
                TRINE_MATRIX_A,
                challenge,
                TRINE_n) != 0)
          goto cleanup;

        a_inverse_ready[challenge] = 1;
      }

      pmod_mat_vec_mul(
          response,
          a_inverse_cache + matrix_offset,
          round_a,
          TRINE_n);
      response_index++;
    }
    else if (challenge == TRINE_BASE_FORM_INDEX)
    {
      memcpy(
          base_seeds + base_seed_index * (size_t)TRINE_round_seed_bytes,
          round_seeds + (size_t)round * (size_t)TRINE_round_seed_bytes,
          TRINE_round_seed_bytes);
      base_seed_index++;
    }
    else
    {
      goto cleanup;
    }
  }

  if (response_index != TRINE_K ||
      base_seed_index != TRINE_BASE_SEED_COUNT)
    goto cleanup;

  if (trine_codec_encode_signature(
          signature_tmp,
          TRINE_SIG_BYTES,
          responses,
          base_seeds,
          digest,
          salt) != 0)
    goto cleanup;

  if (message_len != 0u)
    memmove(sm + TRINE_SIG_BYTES, m, message_len);
  memcpy(sm, signature_tmp, TRINE_SIG_BYTES);
  *smlen = (unsigned long long)TRINE_SIG_BYTES + mlen;
  result = 0;

cleanup:
  if (transcript_active)
    trine_hash_release(&transcript);
  trine_secure_clear(secret_seed, sizeof(secret_seed));
  trine_secure_clear(digest, sizeof(digest));
  trine_secure_clear(salt, sizeof(salt));
  trine_secure_clear(a_all, (size_t)TRINE_r * (size_t)TRINE_n * sizeof(*a_all));
  trine_secure_clear(
      a_inverse_cache,
      (size_t)TRINE_X * matrix_elements * sizeof(*a_inverse_cache));
  trine_secure_clear(responses, response_elements * sizeof(*responses));
  trine_secure_clear(round_seeds, (size_t)TRINE_r * TRINE_round_seed_bytes);
  trine_secure_clear(base_seeds, TRINE_BASE_SEED_BYTES);
  trine_secure_clear(signature_tmp, TRINE_SIG_BYTES);
  free(base_form);
  free(a_all);
  free(a_inverse_cache);
  free(a_inverse_ready);
  free(responses);
  free(psi);
  free(round_seeds);
  free(base_seeds);
  free(encoded_psi);
  free(signature_tmp);
  return result;
}

int crypto_sign_verify(
    const unsigned char *sig,
    unsigned long long siglen,
    const unsigned char *m,
    unsigned long long mlen,
    const unsigned char *pk)
{
  int result = -1;
  size_t message_len = 0;
  uint8_t public_seed[TRINE_public_seed_bytes] = {0};
  uint8_t digest[TRINE_digest_bytes] = {0};
  uint8_t digest_check[TRINE_digest_bytes] = {0};
  uint8_t salt[TRINE_salt_bytes] = {0};
  trine_challenge_t challenges[TRINE_r];
  trine_challenge_t challenges_check[TRINE_r];
  const size_t form_elements = triform_element_count(TRINE_n);
  const size_t response_elements = (size_t)TRINE_K * (size_t)TRINE_n;

  Fq *base_form = NULL;
  Fq *nonbase_forms = NULL;
  Fq *responses = NULL;
  Fq *psi = NULL;
  uint8_t *base_seeds = NULL;
  uint8_t *encoded_psi = NULL;
  trine_hash_state transcript;
  int transcript_active = 0;

  if (sig == NULL || pk == NULL)
    goto cleanup;

  if (m == NULL && mlen != 0u)
    goto cleanup;

  if (siglen != (unsigned long long)TRINE_SIG_BYTES)
    goto cleanup;

  if (trine_message_len_to_size(&message_len, mlen) != 0)
    goto cleanup;

  base_form = trine_alloc_array(form_elements, sizeof(*base_form));
  nonbase_forms = trine_alloc_array(
      (size_t)TRINE_X * form_elements,
      sizeof(*nonbase_forms));
  responses = trine_alloc_array(response_elements, sizeof(*responses));
  base_seeds = trine_alloc_array(TRINE_BASE_SEED_BYTES, sizeof(*base_seeds));
  psi = trine_alloc_array(form_elements, sizeof(*psi));
  encoded_psi = trine_alloc_array(TRINE_TRIFORM_BYTES, sizeof(*encoded_psi));

  if (base_form == NULL ||
      nonbase_forms == NULL ||
      responses == NULL ||
      base_seeds == NULL ||
      psi == NULL ||
      encoded_psi == NULL)
    goto cleanup;

  if (trine_codec_decode_public_key_checked(
          public_seed,
          nonbase_forms,
          pk,
          TRINE_PK_BYTES) != 0)
    goto cleanup;

  if (trine_expand_base_form(base_form, public_seed, TRINE_n) != 0)
    goto cleanup;

  if (trine_codec_decode_signature_checked(
          responses,
          base_seeds,
          digest,
          salt,
          sig,
          TRINE_SIG_BYTES) != 0)
    goto cleanup;

  if (trine_parse_hash(
          digest,
          sizeof(digest),
          challenges,
          TRINE_r) != 0)
    goto cleanup;

  if (!trine_challenges_valid(challenges))
    goto cleanup;

  if (trine_hash_init(&transcript) != 0)
    goto cleanup;
  transcript_active = 1;
  if (message_len != 0u &&
      trine_hash_absorb(&transcript, m, message_len) != 0)
    goto cleanup;

  size_t response_index = 0;
  size_t base_seed_index = 0;

  for (int round = 0; round < TRINE_r; round++)
  {
    const trine_challenge_t challenge = challenges[round];

    if (challenge < TRINE_X)
    {
      const Fq *selected_form =
          nonbase_forms + (size_t)challenge * form_elements;
      const Fq *response =
          responses + response_index * (size_t)TRINE_n;

      if (canonical_form_vartime(
              psi,
              selected_form,
              response,
              TRINE_n) != 0)
        goto cleanup;

      response_index++;
    }
    else if (challenge == TRINE_BASE_FORM_INDEX)
    {
      const uint8_t *round_seed =
          base_seeds + base_seed_index * (size_t)TRINE_round_seed_bytes;

      if (trine_derive_round_commitment_vartime(
              NULL,
              psi,
              base_form,
              round_seed,
              salt,
              (uint32_t)round) != 0)
        goto cleanup;

      base_seed_index++;
    }
    else
    {
      goto cleanup;
    }

    if (trine_absorb_canonical_form(&transcript, encoded_psi, psi) != 0)
      goto cleanup;
  }

  if (response_index != TRINE_K ||
      base_seed_index != TRINE_BASE_SEED_COUNT)
    goto cleanup;

  if (trine_hash_finalize(&transcript, digest_check, sizeof(digest_check)) != 0)
    goto cleanup;

  if (trine_parse_hash(
          digest_check,
          sizeof(digest_check),
          challenges_check,
          TRINE_r) != 0)
    goto cleanup;

  if (!trine_challenges_equal(challenges, challenges_check))
    goto cleanup;

  result = 0;

cleanup:
  if (transcript_active)
    trine_hash_release(&transcript);
  trine_secure_clear(digest, sizeof(digest));
  trine_secure_clear(digest_check, sizeof(digest_check));
  trine_secure_clear(salt, sizeof(salt));
  trine_secure_clear(responses, response_elements * sizeof(*responses));
  trine_secure_clear(base_seeds, TRINE_BASE_SEED_BYTES);
  free(base_form);
  free(nonbase_forms);
  free(responses);
  free(base_seeds);
  free(psi);
  free(encoded_psi);
  return result;
}

int crypto_sign_open(
    unsigned char *m,
    unsigned long long *mlen,
    const unsigned char *sm,
    unsigned long long smlen,
    const unsigned char *pk)
{
  size_t message_len = 0;

  if (mlen != NULL)
    *mlen = 0;

  if (m == NULL || mlen == NULL || sm == NULL || pk == NULL)
    return -1;

  if (smlen < TRINE_SIG_BYTES)
    return -1;

  if (trine_message_len_to_size(
          &message_len,
          smlen - (unsigned long long)TRINE_SIG_BYTES) != 0)
    return -1;

  const unsigned char *message = sm + TRINE_SIG_BYTES;
  if (crypto_sign_verify(
          sm,
          TRINE_SIG_BYTES,
          message,
          (unsigned long long)message_len,
          pk) != 0)
    return -1;

  if (message_len != 0u)
    memmove(m, message, message_len);
  *mlen = (unsigned long long)message_len;
  return 0;
}
