#include "trine_codec.h"

#include <stdlib.h>
#include <string.h>

static int valid_dimension(int n)
{
  return n >= 1 && n <= TRINE_n;
}

static size_t vector_element_count(int n)
{
  return (size_t)n;
}

static size_t triform_element_count_local(int n)
{
  return (size_t)n * (size_t)n * (size_t)n;
}

size_t trine_codec_fq_array_bytes(
    size_t element_count)
{
  const size_t bit_count = element_count * (size_t)TRINE_q_bits;
  return TRINE_CEIL_DIV(bit_count, 8u);
}

static int validate_fq_array(
    const Fq *values,
    size_t element_count)
{
  for (size_t i = 0; i < element_count; i++)
    if (values[i] >= TRINE_q)
      return -1;

  return 0;
}

static void encode_fq_array_unchecked(
    uint8_t *out,
    const Fq *values,
    size_t element_count)
{
  size_t bit_pos = 0;

  for (size_t i = 0; i < element_count; i++)
  {
    const uint32_t value = values[i];

    for (uint32_t bit = 0; bit < TRINE_q_bits; bit++)
    {
      if (((value >> bit) & 1u) != 0u)
        out[bit_pos / 8u] |= (uint8_t)(1u << (bit_pos % 8u));

      bit_pos++;
    }
  }
}

static Fq decode_fq_unchecked(
    const uint8_t *in,
    size_t element_index)
{
  const size_t base_bit = element_index * (size_t)TRINE_q_bits;
  uint32_t value = 0;

  for (uint32_t bit = 0; bit < TRINE_q_bits; bit++)
  {
    const size_t bit_pos = base_bit + (size_t)bit;
    const uint32_t input_bit =
        (uint32_t)((in[bit_pos / 8u] >> (bit_pos % 8u)) & 1u);

    value |= input_bit << bit;
  }

  return (Fq)value;
}

static int padding_is_zero(
    const uint8_t *in,
    size_t element_count,
    size_t in_len)
{
  const size_t used_bits = element_count * (size_t)TRINE_q_bits;
  const size_t used_in_last_byte = used_bits % 8u;

  if (used_in_last_byte == 0u)
    return 1;

  const uint8_t mask = (uint8_t)(0xffu << used_in_last_byte);
  return (in[in_len - 1u] & mask) == 0u;
}

int trine_codec_encode_fq_array(
    uint8_t *out,
    size_t out_len,
    const Fq *values,
    size_t element_count)
{
  const size_t expected_len = trine_codec_fq_array_bytes(element_count);

  if (out_len != expected_len)
    return -1;

  if (element_count == 0u)
    return 0;

  if (out == NULL || values == NULL)
    return -1;

  if (validate_fq_array(values, element_count) != 0)
    return -1;

  memset(out, 0, out_len);
  encode_fq_array_unchecked(out, values, element_count);

  return 0;
}

int trine_codec_decode_fq_array_checked(
    Fq *out,
    size_t element_count,
    const uint8_t *in,
    size_t in_len)
{
  const size_t expected_len = trine_codec_fq_array_bytes(element_count);

  if (in_len != expected_len)
    return -1;

  if (element_count == 0u)
    return 0;

  if (out == NULL || in == NULL)
    return -1;

  if (!padding_is_zero(in, element_count, in_len))
    return -1;

  Fq *tmp = malloc(element_count * sizeof(*tmp));
  if (tmp == NULL)
    return -1;

  for (size_t i = 0; i < element_count; i++)
  {
    tmp[i] = decode_fq_unchecked(in, i);
    if (tmp[i] >= TRINE_q)
    {
      free(tmp);
      return -1;
    }
  }

  memcpy(out, tmp, element_count * sizeof(*tmp));
  free(tmp);
  return 0;
}

int trine_codec_encode_vector(
    uint8_t *out,
    size_t out_len,
    const Fq *vector,
    int n)
{
  if (!valid_dimension(n))
    return -1;

  return trine_codec_encode_fq_array(
      out,
      out_len,
      vector,
      vector_element_count(n));
}

int trine_codec_decode_vector_checked(
    Fq *out_vector,
    const uint8_t *in,
    size_t in_len,
    int n)
{
  if (!valid_dimension(n))
    return -1;

  return trine_codec_decode_fq_array_checked(
      out_vector,
      vector_element_count(n),
      in,
      in_len);
}

int trine_codec_encode_triform(
    uint8_t *out,
    size_t out_len,
    const Fq *M,
    int n)
{
  if (!valid_dimension(n))
    return -1;

  return trine_codec_encode_fq_array(
      out,
      out_len,
      M,
      triform_element_count_local(n));
}

int trine_codec_decode_triform_checked(
    Fq *out_M,
    const uint8_t *in,
    size_t in_len,
    int n)
{
  if (!valid_dimension(n))
    return -1;

  return trine_codec_decode_fq_array_checked(
      out_M,
      triform_element_count_local(n),
      in,
      in_len);
}

int trine_codec_encode_public_key(
    uint8_t *out_pk,
    size_t out_len,
    const uint8_t public_seed[TRINE_public_seed_bytes],
    const Fq *nonbase_forms)
{
  if (out_len != TRINE_PK_BYTES)
    return -1;

  if (out_pk == NULL ||
      public_seed == NULL ||
      nonbase_forms == NULL)
    return -1;

  const size_t form_elements = triform_element_count_local(TRINE_n);
  if (validate_fq_array(nonbase_forms, (size_t)TRINE_X * form_elements) != 0)
    return -1;

  memcpy(out_pk, public_seed, TRINE_public_seed_bytes);

  for (size_t form = 0; form < (size_t)TRINE_X; form++)
  {
    uint8_t *encoded_form =
        out_pk + TRINE_public_seed_bytes + form * (size_t)TRINE_TRIFORM_BYTES;
    const Fq *form_values =
        nonbase_forms + form * form_elements;

    if (trine_codec_encode_triform(
            encoded_form,
            TRINE_TRIFORM_BYTES,
            form_values,
            TRINE_n) != 0)
      return -1;
  }

  return 0;
}

int trine_codec_decode_public_key_checked(
    uint8_t out_public_seed[TRINE_public_seed_bytes],
    Fq *out_nonbase_forms,
    const uint8_t *pk,
    size_t pk_len)
{
  if (pk_len != TRINE_PK_BYTES)
    return -1;

  if (out_public_seed == NULL ||
      out_nonbase_forms == NULL ||
      pk == NULL)
    return -1;

  const size_t form_elements = triform_element_count_local(TRINE_n);
  const size_t forms_count = (size_t)TRINE_X * form_elements;
  Fq *forms_tmp = malloc(forms_count * sizeof(*forms_tmp));
  if (forms_tmp == NULL)
    return -1;


  for (size_t form = 0; form < (size_t)TRINE_X; form++)
  {
    const uint8_t *encoded_form =
        pk + TRINE_public_seed_bytes + form * (size_t)TRINE_TRIFORM_BYTES;
    Fq *form_values = forms_tmp + form * form_elements;

    if (trine_codec_decode_triform_checked(
            form_values,
            encoded_form,
            TRINE_TRIFORM_BYTES,
            TRINE_n) != 0)
    {
      free(forms_tmp);
      return -1;
    }
  }

  memcpy(out_public_seed, pk, TRINE_public_seed_bytes);
  memcpy(out_nonbase_forms, forms_tmp, forms_count * sizeof(*forms_tmp));
  free(forms_tmp);

  return 0;
}

int trine_codec_encode_secret_key(
    uint8_t *out_sk,
    size_t out_len,
    const uint8_t secret_seed[TRINE_secret_seed_bytes])
{
  if (out_len != TRINE_SK_BYTES)
    return -1;

  if (out_sk == NULL || secret_seed == NULL)
    return -1;

  memcpy(out_sk, secret_seed, TRINE_SK_BYTES);

  return 0;
}

int trine_codec_decode_secret_key(
    uint8_t out_secret_seed[TRINE_secret_seed_bytes],
    const uint8_t *sk,
    size_t sk_len)
{
  if (sk_len != TRINE_SK_BYTES)
    return -1;

  if (out_secret_seed == NULL || sk == NULL)
    return -1;

  memcpy(out_secret_seed, sk, TRINE_SK_BYTES);

  return 0;
}

int trine_codec_encode_signature(
    uint8_t *out_sig,
    size_t out_len,
    const Fq *responses,
    const uint8_t *base_seeds,
    const uint8_t digest[TRINE_digest_bytes],
    const uint8_t salt[TRINE_salt_bytes])
{
  if (out_len != TRINE_SIG_BYTES)
    return -1;

  if (out_sig == NULL ||
      responses == NULL ||
      base_seeds == NULL ||
      digest == NULL ||
      salt == NULL)
    return -1;

  if (trine_codec_encode_fq_array(
          out_sig + TRINE_RESPONSE_OFFSET,
          TRINE_RESPONSE_BYTES,
          responses,
          (size_t)TRINE_K * (size_t)TRINE_n) != 0)
    return -1;

  memcpy(
      out_sig + TRINE_BASE_SEED_OFFSET,
      base_seeds,
      TRINE_BASE_SEED_BYTES);
  memcpy(out_sig + TRINE_DIGEST_OFFSET, digest, TRINE_digest_bytes);
  memcpy(out_sig + TRINE_SALT_OFFSET, salt, TRINE_salt_bytes);

  return 0;
}

int trine_codec_decode_signature_checked(
    Fq *out_responses,
    uint8_t *out_base_seeds,
    uint8_t out_digest[TRINE_digest_bytes],
    uint8_t out_salt[TRINE_salt_bytes],
    const uint8_t *sig,
    size_t sig_len)
{
  if (sig_len != TRINE_SIG_BYTES)
    return -1;

  if (out_responses == NULL ||
      out_base_seeds == NULL ||
      out_digest == NULL ||
      out_salt == NULL ||
      sig == NULL)
    return -1;

  const size_t response_count = (size_t)TRINE_K * (size_t)TRINE_n;
  Fq *responses_tmp = malloc(response_count * sizeof(*responses_tmp));
  if (responses_tmp == NULL)
    return -1;

  uint8_t digest_tmp[TRINE_digest_bytes];
  uint8_t salt_tmp[TRINE_salt_bytes];

  if (trine_codec_decode_fq_array_checked(
          responses_tmp,
          (size_t)TRINE_K * (size_t)TRINE_n,
          sig + TRINE_RESPONSE_OFFSET,
          TRINE_RESPONSE_BYTES) != 0)
  {
    free(responses_tmp);
    return -1;
  }

  memcpy(
      out_base_seeds,
      sig + TRINE_BASE_SEED_OFFSET,
      TRINE_BASE_SEED_BYTES);
  memcpy(digest_tmp, sig + TRINE_DIGEST_OFFSET, sizeof(digest_tmp));
  memcpy(salt_tmp, sig + TRINE_SALT_OFFSET, sizeof(salt_tmp));

  memcpy(
      out_responses,
      responses_tmp,
      response_count * sizeof(*responses_tmp));
  memcpy(out_digest, digest_tmp, sizeof(digest_tmp));
  memcpy(out_salt, salt_tmp, sizeof(salt_tmp));
  free(responses_tmp);

  return 0;
}
