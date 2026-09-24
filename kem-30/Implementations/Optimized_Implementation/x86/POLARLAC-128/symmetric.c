/*
Copyright (c) 2026 Yu Zhang, Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Implements unified symmetric helper routines for the optimized POLARLAC-128 instance.
                  Supports SHAKE (via fips202) and SM3 (via auxfunc) through BIT_USE_SHAKE.
*/

#include <stdint.h>
#include <string.h>
#if BIT_USE_SHAKE
#include "fips202.h"
#include "fips202x4.h"
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

int bit_hash_256_bytes(const unsigned char *msg, size_t msg_len_bytes, unsigned char *digest)
{
#if BIT_USE_SHAKE
    sha3_256(digest, msg, msg_len_bytes);
    return 0;
#else
    return sm3hash(256, msg, (unsigned long long)msg_len_bytes * 8ULL, digest);
#endif
}

int bit_xof_bytes(unsigned char *output, size_t output_len_bytes, const unsigned char *msg, size_t msg_len_bytes)
{
#if BIT_USE_SHAKE
    shake256(output, output_len_bytes, msg, msg_len_bytes);
    return 0;
#else
    return pseudoXOF((unsigned long long)output_len_bytes * 8ULL,
                     msg,
                     (unsigned long long)msg_len_bytes * 8ULL,
                     output);
#endif
}

int bit_hash_256_concat2_bytes(const unsigned char *msg0, size_t msg0_len_bytes,
                               const unsigned char *msg1, size_t msg1_len_bytes,
                               unsigned char *digest)
{
#if BIT_USE_SHAKE
    sha3_256_concat2(digest, msg0, msg0_len_bytes, msg1, msg1_len_bytes);
    return 0;
#else
    unsigned char input[msg0_len_bytes + msg1_len_bytes];

    memcpy(input, msg0, msg0_len_bytes);
    memcpy(input + msg0_len_bytes, msg1, msg1_len_bytes);
    return bit_hash_256_bytes(input, sizeof(input), digest);
#endif
}

int bit_xof_concat2_bytes(unsigned char *output, size_t output_len_bytes,
                          const unsigned char *msg0, size_t msg0_len_bytes,
                          const unsigned char *msg1, size_t msg1_len_bytes)
{
#if BIT_USE_SHAKE
    keccak_state state;

    shake256_init(&state);
    shake256_absorb(&state, msg0, msg0_len_bytes);
    shake256_absorb(&state, msg1, msg1_len_bytes);
    shake256_finalize(&state);
    shake256_squeeze(output, output_len_bytes, &state);
    return 0;
#else
    unsigned char input[msg0_len_bytes + msg1_len_bytes];

    memcpy(input, msg0, msg0_len_bytes);
    memcpy(input + msg0_len_bytes, msg1, msg1_len_bytes);
    return bit_xof_bytes(output, output_len_bytes, input, sizeof(input));
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

int bit_xof128x4_bytes(unsigned char *out0, unsigned char *out1, unsigned char *out2, unsigned char *out3,
                       size_t out_len_bytes,
                       const unsigned char *in0, const unsigned char *in1,
                       const unsigned char *in2, const unsigned char *in3,
                       size_t in_len_bytes)
{
#if BIT_USE_SHAKE
    shake128x4(out0, out1, out2, out3, out_len_bytes,
               in0, in1, in2, in3, in_len_bytes);
    return 0;
#else
    int ret = 0;

    ret |= pseudoXOF((unsigned long long)out_len_bytes * 8ULL,
                     in0, (unsigned long long)in_len_bytes * 8ULL, out0);
    ret |= pseudoXOF((unsigned long long)out_len_bytes * 8ULL,
                     in1, (unsigned long long)in_len_bytes * 8ULL, out1);
    ret |= pseudoXOF((unsigned long long)out_len_bytes * 8ULL,
                     in2, (unsigned long long)in_len_bytes * 8ULL, out2);
    ret |= pseudoXOF((unsigned long long)out_len_bytes * 8ULL,
                     in3, (unsigned long long)in_len_bytes * 8ULL, out3);
    return ret;
#endif
}


int bit_xof128_bytes_nonce0(unsigned char *out, size_t out_len_bytes,
                             const unsigned char *in, size_t in_len_bytes)
{
#if BIT_USE_SHAKE
    uint8_t ext[in_len_bytes + 1];
    memcpy(ext, in, in_len_bytes);
    ext[in_len_bytes] = 0;
    shake128(out, out_len_bytes, ext, in_len_bytes + 1);
    return 0;
#else
    uint8_t ext[in_len_bytes + 5];
    memcpy(ext, in, in_len_bytes);
    ext[in_len_bytes] = 0;      /* nonce */
    ext[in_len_bytes + 1] = 0;  /* block[31:24] */
    ext[in_len_bytes + 2] = 0;  /* block[23:16] */
    ext[in_len_bytes + 3] = 0;  /* block[15:8]  */
    ext[in_len_bytes + 4] = 0;  /* block[7:0]   */
    return pseudoXOF((unsigned long long)out_len_bytes * 8ULL,
                     ext,
                     (unsigned long long)(in_len_bytes + 5) * 8ULL,
                     out);
#endif
}

int bit_xof128x4_bytes_nonce0(unsigned char *out0, unsigned char *out1, unsigned char *out2, unsigned char *out3,
                              size_t out_len_bytes,
                              const unsigned char *in0, const unsigned char *in1,
                              const unsigned char *in2, const unsigned char *in3,
                              size_t in_len_bytes)
{
#if BIT_USE_SHAKE
    uint8_t ext0[in_len_bytes + 1];
    uint8_t ext1[in_len_bytes + 1];
    uint8_t ext2[in_len_bytes + 1];
    uint8_t ext3[in_len_bytes + 1];

    memcpy(ext0, in0, in_len_bytes);
    memcpy(ext1, in1, in_len_bytes);
    memcpy(ext2, in2, in_len_bytes);
    memcpy(ext3, in3, in_len_bytes);
    ext0[in_len_bytes] = 0;
    ext1[in_len_bytes] = 0;
    ext2[in_len_bytes] = 0;
    ext3[in_len_bytes] = 0;

    shake128x4(out0, out1, out2, out3, out_len_bytes,
               ext0, ext1, ext2, ext3, in_len_bytes + 1);
    return 0;
#else
    bit_xof128_bytes_nonce0(out0, out_len_bytes, in0, in_len_bytes);
    bit_xof128_bytes_nonce0(out1, out_len_bytes, in1, in_len_bytes);
    bit_xof128_bytes_nonce0(out2, out_len_bytes, in2, in_len_bytes);
    bit_xof128_bytes_nonce0(out3, out_len_bytes, in3, in_len_bytes);
    return 0;
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

uint8_t bit_xof_stream_next_nonce(const xof_stream *stream)
{
    return (uint8_t)(stream->nonce + 1U);
}

/* backward-compatible wrapper */
void rl_kem_pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output, uint8_t nonce)
{
    unsigned int i;
    uint8_t extseed[msg_len_bits/8+1];

    for(i=0;i<msg_len_bits/8;i++)
    {
        extseed[i] = msg[i];
    }
    extseed[i] = nonce;

    bit_xof(output_len_bits, extseed, msg_len_bits+8, output);
}
