/*
Copyright (c) 2026 Yu Zhang, Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Implements unified symmetric helper routines for the reference POLARLAC-512-Star instance.
                  Supports SHAKE (via fips202) and SM3 (via auxfunc) through BIT_USE_SHAKE.
*/

#include <stdint.h>
#include <string.h>
#if BIT_USE_SHAKE
#include "fips202.h"
#else
#include "auxfunc.h"
#endif
#include "symmetric.h"

int bit_hash_256(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
#if BIT_USE_SHAKE
    if ((msg_len_bits & 7ULL) != 0) {
        return -1;
    }
    sha3_256(digest, msg, (size_t)(msg_len_bits / 8));
    return 0;
#else
    return sm3hash(256, msg, msg_len_bits, digest);
#endif
}

int bit_xof(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output)
{
#if BIT_USE_SHAKE
    if ((output_len_bits & 7ULL) != 0 || (msg_len_bits & 7ULL) != 0) {
        return -1;
    }
    shake256(output, (size_t)(output_len_bits / 8), msg, (size_t)(msg_len_bits / 8));
    return 0;
#else
    return pseudoXOF(output_len_bits, msg, msg_len_bits, output);
#endif
}

void bit_xof_stream_init(xof_stream *stream, const unsigned char *seed, size_t seed_len_bytes, uint8_t nonce)
{
#if BIT_USE_SHAKE
    shake256_init(&stream->state);
    shake256_absorb(&stream->state, seed, seed_len_bytes);
    shake256_absorb(&stream->state, &nonce, sizeof(nonce));
    shake256_finalize(&stream->state);
#else
    stream->seed = seed;
    stream->seed_len_bytes = seed_len_bytes;
    stream->block = 0;
#endif
    stream->nonce = nonce;
}

int bit_xof_stream_squeeze(xof_stream *stream, unsigned char *output, size_t output_len_bytes)
{
#if BIT_USE_SHAKE
    shake256_squeeze(output, output_len_bytes, &stream->state);
    return 0;
#else
    uint8_t extseed[stream->seed_len_bytes + 5];

    memcpy(extseed, stream->seed, stream->seed_len_bytes);
    extseed[stream->seed_len_bytes] = stream->nonce;
    extseed[stream->seed_len_bytes + 1] = (uint8_t)(stream->block >> 24);
    extseed[stream->seed_len_bytes + 2] = (uint8_t)(stream->block >> 16);
    extseed[stream->seed_len_bytes + 3] = (uint8_t)(stream->block >> 8);
    extseed[stream->seed_len_bytes + 4] = (uint8_t)stream->block;
    stream->block++;

    return bit_xof(output_len_bytes * 8, extseed, (stream->seed_len_bytes + 5) * 8, output);
#endif
}

void bit_xof_stream_release(xof_stream *stream)
{
#if BIT_USE_SHAKE
    (void)stream;
#else
    (void)stream;
#endif
}

void bit_xof128_stream_init(xof128_stream *stream, const unsigned char *seed, size_t seed_len_bytes, uint8_t nonce)
{
#if BIT_USE_SHAKE
    shake128_init(&stream->state);
    shake128_absorb(&stream->state, seed, seed_len_bytes);
    shake128_absorb(&stream->state, &nonce, sizeof(nonce));
    shake128_finalize(&stream->state);
#else
    stream->seed = seed;
    stream->seed_len_bytes = seed_len_bytes;
    stream->block = 0;
#endif
    stream->nonce = nonce;
}

int bit_xof128_stream_squeeze(xof128_stream *stream, unsigned char *output, size_t output_len_bytes)
{
#if BIT_USE_SHAKE
    shake128_squeeze(output, output_len_bytes, &stream->state);
    return 0;
#else
    uint8_t extseed[stream->seed_len_bytes + 5];

    memcpy(extseed, stream->seed, stream->seed_len_bytes);
    extseed[stream->seed_len_bytes] = stream->nonce;
    extseed[stream->seed_len_bytes + 1] = (uint8_t)(stream->block >> 24);
    extseed[stream->seed_len_bytes + 2] = (uint8_t)(stream->block >> 16);
    extseed[stream->seed_len_bytes + 3] = (uint8_t)(stream->block >> 8);
    extseed[stream->seed_len_bytes + 4] = (uint8_t)stream->block;
    stream->block++;

    return bit_xof(output_len_bytes * 8, extseed, (stream->seed_len_bytes + 5) * 8, output);
#endif
}

void bit_xof128_stream_release(xof128_stream *stream)
{
#if BIT_USE_SHAKE
    (void)stream;
#else
    (void)stream;
#endif
}
