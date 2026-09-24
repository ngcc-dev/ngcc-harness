#ifndef TRINE_CODEC_H
#define TRINE_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "params.h"

size_t trine_codec_fq_array_bytes(
    size_t element_count);

int trine_codec_encode_fq_array(
    uint8_t *out,
    size_t out_len,
    const Fq *values,
    size_t element_count);

int trine_codec_decode_fq_array_checked(
    Fq *out,
    size_t element_count,
    const uint8_t *in,
    size_t in_len);

int trine_codec_encode_vector(
    uint8_t *out,
    size_t out_len,
    const Fq *vector,
    int n);

int trine_codec_decode_vector_checked(
    Fq *out_vector,
    const uint8_t *in,
    size_t in_len,
    int n);

int trine_codec_encode_triform(
    uint8_t *out,
    size_t out_len,
    const Fq *M,
    int n);

int trine_codec_decode_triform_checked(
    Fq *out_M,
    const uint8_t *in,
    size_t in_len,
    int n);

int trine_codec_encode_public_key(
    uint8_t *out_pk,
    size_t out_len,
    const uint8_t public_seed[TRINE_public_seed_bytes],
    const Fq *nonbase_forms);

int trine_codec_decode_public_key_checked(
    uint8_t out_public_seed[TRINE_public_seed_bytes],
    Fq *out_nonbase_forms,
    const uint8_t *pk,
    size_t pk_len);

int trine_codec_encode_secret_key(
    uint8_t *out_sk,
    size_t out_len,
    const uint8_t secret_seed[TRINE_secret_seed_bytes]);

int trine_codec_decode_secret_key(
    uint8_t out_secret_seed[TRINE_secret_seed_bytes],
    const uint8_t *sk,
    size_t sk_len);

int trine_codec_encode_signature(
    uint8_t *out_sig,
    size_t out_len,
    const Fq *responses,
    const uint8_t *base_seeds,
    const uint8_t digest[TRINE_digest_bytes],
    const uint8_t salt[TRINE_salt_bytes]);

int trine_codec_decode_signature_checked(
    Fq *out_responses,
    uint8_t *out_base_seeds,
    uint8_t out_digest[TRINE_digest_bytes],
    uint8_t out_salt[TRINE_salt_bytes],
    const uint8_t *sig,
    size_t sig_len);

#endif
