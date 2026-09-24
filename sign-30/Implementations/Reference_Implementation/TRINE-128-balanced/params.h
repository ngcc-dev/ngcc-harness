#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>

#ifndef PARAMS
#define PARAMS 1
#endif

#if PARAMS < 1 || PARAMS > 6
#error "PARAMS must be in the range 1..6"
#endif

#if PARAMS == 1
#define TRINE_PARAMETER_SET_NAME "Balanced-I"
#define TRINE_PARAMETER_TARGET_SUFFIX "balanced_i"
#define TRINE_lambda 128
#define TRINE_n 22
#define TRINE_q 4093
#define TRINE_r 138
#define TRINE_K 52
#define TRINE_X 1
#define TRINE_EXPECTED_VECTOR_BYTES 33u
#define TRINE_EXPECTED_MATRIX_BYTES 726u
#define TRINE_EXPECTED_TRIFORM_BYTES 15972u
#define TRINE_EXPECTED_PK_BYTES 16004u
#define TRINE_EXPECTED_SK_BYTES 32u
#define TRINE_EXPECTED_RESPONSE_BYTES 1716u
#define TRINE_EXPECTED_BASE_SEED_COUNT 86u
#define TRINE_EXPECTED_BASE_SEED_BYTES 1376u
#define TRINE_EXPECTED_SIG_BYTES 3156u
#elif PARAMS == 2
#define TRINE_PARAMETER_SET_NAME "Balanced-III"
#define TRINE_PARAMETER_TARGET_SUFFIX "balanced_iii"
#define TRINE_lambda 256
#define TRINE_n 40
#define TRINE_q 4093
#define TRINE_r 271
#define TRINE_K 104
#define TRINE_X 1
#define TRINE_EXPECTED_VECTOR_BYTES 60u
#define TRINE_EXPECTED_MATRIX_BYTES 2400u
#define TRINE_EXPECTED_TRIFORM_BYTES 96000u
#define TRINE_EXPECTED_PK_BYTES 96064u
#define TRINE_EXPECTED_SK_BYTES 64u
#define TRINE_EXPECTED_RESPONSE_BYTES 6240u
#define TRINE_EXPECTED_BASE_SEED_COUNT 167u
#define TRINE_EXPECTED_BASE_SEED_BYTES 5344u
#define TRINE_EXPECTED_SIG_BYTES 11712u
#elif PARAMS == 3
#define TRINE_PARAMETER_SET_NAME "Balanced-V"
#define TRINE_PARAMETER_TARGET_SUFFIX "balanced_v"
#define TRINE_lambda 512
#define TRINE_n 81
#define TRINE_q 4093
#define TRINE_r 534
#define TRINE_K 211
#define TRINE_X 1
#define TRINE_EXPECTED_VECTOR_BYTES 122u
#define TRINE_EXPECTED_MATRIX_BYTES 9842u
#define TRINE_EXPECTED_TRIFORM_BYTES 797162u
#define TRINE_EXPECTED_PK_BYTES 797290u
#define TRINE_EXPECTED_SK_BYTES 128u
#define TRINE_EXPECTED_RESPONSE_BYTES 25637u
#define TRINE_EXPECTED_BASE_SEED_COUNT 323u
#define TRINE_EXPECTED_BASE_SEED_BYTES 20672u
#define TRINE_EXPECTED_SIG_BYTES 46565u
#elif PARAMS == 4
#define TRINE_PARAMETER_SET_NAME "ShortSig-I"
#define TRINE_PARAMETER_TARGET_SUFFIX "shortsig_i"
#define TRINE_lambda 128
#define TRINE_n 22
#define TRINE_q 4093
#define TRINE_r 61
#define TRINE_K 36
#define TRINE_X 4
#define TRINE_EXPECTED_VECTOR_BYTES 33u
#define TRINE_EXPECTED_MATRIX_BYTES 726u
#define TRINE_EXPECTED_TRIFORM_BYTES 15972u
#define TRINE_EXPECTED_PK_BYTES 63920u
#define TRINE_EXPECTED_SK_BYTES 32u
#define TRINE_EXPECTED_RESPONSE_BYTES 1188u
#define TRINE_EXPECTED_BASE_SEED_COUNT 25u
#define TRINE_EXPECTED_BASE_SEED_BYTES 400u
#define TRINE_EXPECTED_SIG_BYTES 1652u
#elif PARAMS == 5
#define TRINE_PARAMETER_SET_NAME "ShortSig-III"
#define TRINE_PARAMETER_TARGET_SUFFIX "shortsig_iii"
#define TRINE_lambda 256
#define TRINE_n 40
#define TRINE_q 4093
#define TRINE_r 116
#define TRINE_K 76
#define TRINE_X 4
#define TRINE_EXPECTED_VECTOR_BYTES 60u
#define TRINE_EXPECTED_MATRIX_BYTES 2400u
#define TRINE_EXPECTED_TRIFORM_BYTES 96000u
#define TRINE_EXPECTED_PK_BYTES 384064u
#define TRINE_EXPECTED_SK_BYTES 64u
#define TRINE_EXPECTED_RESPONSE_BYTES 4560u
#define TRINE_EXPECTED_BASE_SEED_COUNT 40u
#define TRINE_EXPECTED_BASE_SEED_BYTES 1280u
#define TRINE_EXPECTED_SIG_BYTES 5968u
#elif PARAMS == 6
#define TRINE_PARAMETER_SET_NAME "ShortSig-V"
#define TRINE_PARAMETER_TARGET_SUFFIX "shortsig_v"
#define TRINE_lambda 512
#define TRINE_n 81
#define TRINE_q 4093
#define TRINE_r 232
#define TRINE_K 149
#define TRINE_X 4
#define TRINE_EXPECTED_VECTOR_BYTES 122u
#define TRINE_EXPECTED_MATRIX_BYTES 9842u
#define TRINE_EXPECTED_TRIFORM_BYTES 797162u
#define TRINE_EXPECTED_PK_BYTES 3188776u
#define TRINE_EXPECTED_SK_BYTES 128u
#define TRINE_EXPECTED_RESPONSE_BYTES 18104u
#define TRINE_EXPECTED_BASE_SEED_COUNT 83u
#define TRINE_EXPECTED_BASE_SEED_BYTES 5312u
#define TRINE_EXPECTED_SIG_BYTES 23672u
#endif


#define TRINE_q_bits 12

typedef uint16_t Fq;
typedef uint16_t trine_challenge_t;

#define TRINE_FORM_COUNT (TRINE_X + 1)
#define TRINE_NONBASE_COUNT TRINE_X
#define TRINE_BASE_FORM_INDEX TRINE_X

#if TRINE_n < 5
#error "TRINE CF requires TRINE_n >= 5"
#endif

#if TRINE_K > TRINE_r
#error "TRINE_K must not exceed TRINE_r"
#endif

#if TRINE_X < 1 || TRINE_X > 256
#error "TRINE requires 1 <= TRINE_X <= 256"
#endif

#if TRINE_lambda % 8 != 0
#error "TRINE_lambda must be byte aligned"
#endif

_Static_assert(
    ((uint32_t)1u << (TRINE_q_bits - 1)) < TRINE_q,
    "TRINE_q_bits is too large");

_Static_assert(
    TRINE_q <= ((uint32_t)1u << TRINE_q_bits),
    "TRINE_q_bits is too small");

_Static_assert(
    (uint32_t)UINT16_MAX >= (TRINE_q - 1u),
    "Fq cannot store q - 1");

_Static_assert(
    (uint64_t)TRINE_n * (uint64_t)(TRINE_q - 1u) * (uint64_t)(TRINE_q - 1u)
        <= UINT64_MAX,
    "matrix dot-product accumulator may overflow");

#define TRINE_CEIL_DIV(x,y) (((x) + (y) - 1) / (y))

#define TRINE_lambda_bytes (TRINE_lambda / 8)
#define TRINE_digest_bytes (2 * TRINE_lambda_bytes)
#define TRINE_public_seed_bytes (2 * TRINE_lambda_bytes)
#define TRINE_secret_seed_bytes (2 * TRINE_lambda_bytes)
#define TRINE_round_seed_bytes TRINE_lambda_bytes
#define TRINE_salt_bytes (2 * TRINE_lambda_bytes)

#define TRINE_VECTOR_BYTES TRINE_CEIL_DIV(TRINE_n * TRINE_q_bits, 8)
#define TRINE_MATRIX_BYTES TRINE_CEIL_DIV((TRINE_n * TRINE_n) * TRINE_q_bits, 8)
#define TRINE_TRIFORM_BYTES TRINE_CEIL_DIV((TRINE_n * TRINE_n * TRINE_n) * TRINE_q_bits, 8)
#define TRINE_PK_BYTES (TRINE_public_seed_bytes + TRINE_X * TRINE_TRIFORM_BYTES)
#define TRINE_SK_BYTES TRINE_secret_seed_bytes
#define TRINE_RESPONSE_BYTES TRINE_CEIL_DIV((TRINE_K * TRINE_n * TRINE_q_bits), 8)
#define TRINE_BASE_SEED_COUNT (TRINE_r - TRINE_K)
#define TRINE_BASE_SEED_BYTES (TRINE_BASE_SEED_COUNT * TRINE_round_seed_bytes)
#define TRINE_RESPONSE_OFFSET 0
#define TRINE_BASE_SEED_OFFSET (TRINE_RESPONSE_OFFSET + TRINE_RESPONSE_BYTES)
#define TRINE_DIGEST_OFFSET (TRINE_BASE_SEED_OFFSET + TRINE_BASE_SEED_BYTES)
#define TRINE_SALT_OFFSET (TRINE_DIGEST_OFFSET + TRINE_digest_bytes)
#define TRINE_SIG_BYTES (TRINE_SALT_OFFSET + TRINE_salt_bytes)

_Static_assert(TRINE_VECTOR_BYTES == TRINE_EXPECTED_VECTOR_BYTES, "unexpected vector size");
_Static_assert(TRINE_MATRIX_BYTES == TRINE_EXPECTED_MATRIX_BYTES, "unexpected matrix size");
_Static_assert(TRINE_TRIFORM_BYTES == TRINE_EXPECTED_TRIFORM_BYTES, "unexpected triform size");
_Static_assert(TRINE_PK_BYTES == TRINE_EXPECTED_PK_BYTES, "unexpected public-key size");
_Static_assert(TRINE_SK_BYTES == TRINE_EXPECTED_SK_BYTES, "unexpected secret-key size");
_Static_assert(TRINE_RESPONSE_BYTES == TRINE_EXPECTED_RESPONSE_BYTES, "unexpected response size");
_Static_assert(TRINE_BASE_SEED_COUNT == TRINE_EXPECTED_BASE_SEED_COUNT, "unexpected base-seed count");
_Static_assert(TRINE_BASE_SEED_BYTES == TRINE_EXPECTED_BASE_SEED_BYTES, "unexpected base-seed segment size");
_Static_assert(TRINE_SIG_BYTES == TRINE_EXPECTED_SIG_BYTES, "unexpected signature size");

#endif
