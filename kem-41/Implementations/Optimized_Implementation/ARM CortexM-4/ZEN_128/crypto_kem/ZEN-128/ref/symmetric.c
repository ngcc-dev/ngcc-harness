/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "params.h"
#include "auxfunc.h"
#ifdef USE_KECCAK
#include "fips202.h"
#endif

void ZEN_pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output, uint8_t nonce)
{
    const size_t msg_len_bytes = (size_t)(msg_len_bits / 8);
    uint8_t extseed[msg_len_bytes + 1];

    memcpy(extseed, msg, msg_len_bytes);
    extseed[msg_len_bytes] = nonce;

#ifdef USE_KECCAK
    shake256(output, (size_t)((output_len_bits + 7) / 8), extseed, msg_len_bytes + 1);
#else
    pseudoXOF(output_len_bits, extseed, (msg_len_bits / 8 + 1) * 8, output);
#endif
}
