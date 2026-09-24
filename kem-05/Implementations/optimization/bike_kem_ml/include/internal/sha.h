/* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0"
 *
 * Written by Nir Drucker, Shay Gueron and Dusan Kostic,
 * AWS Cryptographic Algorithms Group.
 */

#pragma once

#include "cleanup.h"
#include "error.h"
#include "types.h"

#define SHA384_DGST_BYTES  48ULL
#define SHA384_DGST_QWORDS (SHA384_DGST_BYTES / 8)

#define SHA512_DGST_BYTES  64ULL
#define SHA512_DGST_QWORDS (SHA512_DGST_BYTES / 8)

#define SM3_DGST_BYTES  32ULL
#define SM3_DGST_QWORDS (SM3_DGST_BYTES / 8)

typedef struct sha384_dgst_s {
  union {
    uint8_t  raw[SHA384_DGST_BYTES];
    uint64_t qw[SHA384_DGST_QWORDS];
  } u;
} sha384_dgst_t;
bike_static_assert(sizeof(sha384_dgst_t) == SHA384_DGST_BYTES, sha384_dgst_size);

typedef struct sha512_dgst_s {
  union {
    uint8_t  raw[SHA512_DGST_BYTES];
    uint64_t qw[SHA512_DGST_QWORDS];
  } u;
} sha512_dgst_t;
bike_static_assert(sizeof(sha512_dgst_t) == SHA512_DGST_BYTES, sha512_dgst_size);

#if defined(USE_API_PKC_AUX)
#  if(SECURITY_BITS == 512)
typedef sha512_dgst_t sha_dgst_t;
#  else
typedef struct sm3_dgst_s {
  union {
    uint8_t  raw[SM3_DGST_BYTES];
    uint64_t qw[SM3_DGST_QWORDS];
  } u;
} sm3_dgst_t;
bike_static_assert(sizeof(sm3_dgst_t) == SM3_DGST_BYTES, sm3_dgst_size);

typedef sm3_dgst_t sha_dgst_t;
#  endif
#else
#  if(SECURITY_BITS == 512)
#    error "The 512-bit-width profile requires USE_API_PKC_AUX"
#  endif
typedef sha384_dgst_t sha_dgst_t;
#endif
bike_static_assert(sizeof(sha_dgst_t) >= M_BYTES, hash_dgst_smaller_than_m);
bike_static_assert(sizeof(sha_dgst_t) >= SS_BYTES, hash_dgst_smaller_than_ss);
CLEANUP_FUNC(sha_dgst, sha_dgst_t)

#if defined(USE_API_PKC_AUX)

#  include "auxfunc.h"

_INLINE_ ret_t sha(OUT sha_dgst_t *  dgst,
                   IN const uint32_t byte_len,
                   IN const uint8_t *msg)
{
#  if(SECURITY_BITS == 512)
  return pseudohash(512, msg, ((unsigned long long)byte_len) * 8ULL,
                    dgst->u.raw) == 0 ? SUCCESS : FAIL;
#  else
  return sm3hash(256, msg, ((unsigned long long)byte_len) * 8ULL,
                 dgst->u.raw) == 0 ? SUCCESS : FAIL;
#  endif
}

#elif defined(STANDALONE_IMPL)

# if defined(USE_SHA3_AND_SHAKE)

#    include "fips202.h"

_INLINE_ ret_t sha(OUT sha_dgst_t *  dgst,
                   IN const uint32_t byte_len,
                   IN const uint8_t *msg)
{
  sha3_384(dgst->u.raw, msg, byte_len);

  return SUCCESS;
}

# else // USE_SHA3_AND_SHAKE

#  define HASH_BLOCK_BYTES 128ULL

ret_t sha(OUT sha_dgst_t *dgst, IN uint32_t byte_len, IN const uint8_t *msg);
#  endif //USE_SHA3_AND_SHAKE

#else // USE_OPENSSL

#  include "utilities.h"
#  include <openssl/sha.h>

_INLINE_ ret_t sha(OUT sha_dgst_t *  dgst,
                   IN const uint32_t byte_len,
                   IN const uint8_t *msg)
{
  if(SHA384(msg, byte_len, dgst->u.raw) != NULL) {
    return SUCCESS;
  }

  return FAIL;
}

#endif // USE_OPENSSL
