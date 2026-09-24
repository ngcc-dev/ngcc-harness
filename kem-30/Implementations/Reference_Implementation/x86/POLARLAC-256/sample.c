/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the reference POLARLAC-256 instance.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "params.h"
#include "ntt.h"
#include "symmetric.h"
#include "poly.h"

void poly_generate_uniformQ(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j, t;
    uint8_t buf[RL_KEM_N], exseed[PK_SEED_LEN_BYTES+2];
    xof128_stream stream;
    memcpy(exseed, seed, PK_SEED_LEN_BYTES);

    for(i = 0; i < RL_KEM_K; i++)
    {
        for(j = 0; j < RL_KEM_K; j++)
        {
            
            if(transposed)
            {
                exseed[PK_SEED_LEN_BYTES] = i;
                exseed[PK_SEED_LEN_BYTES+1] = j;
            }
            else
            {
                exseed[PK_SEED_LEN_BYTES] = j;
                exseed[PK_SEED_LEN_BYTES+1] = i;
            }
            bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES+2, 0);
            bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N);
            for(t = 0; t < RL_KEM_N; t++)
            {
                a->row[i].vec[j].coeffs[t] = (int16_t)buf[t];
            }
        }
    }
    bit_xof128_stream_release(&stream);
}

// Ternary sampler: {-1:1/8; 0:3/4; 1:1/8}.
void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i, j;
    uint8_t buf[RL_KEM_N_LEN_BYTES*3];
    int16_t t[RL_KEM_N*3];
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N_LEN_BYTES*3);
    
    for (i = 0; i < 8; i++) 
    {
        for (j = 0; j < RL_KEM_N_LEN_BYTES * 3; j++) 
        {
            t[i * (RL_KEM_N_LEN_BYTES * 3) + j] = (int16_t)(buf[j] & 1U);
            buf[j] >>= 1;
        }
    }
    for (i = 0; i < RL_KEM_N; i++) 
    {
        a[i] = (int16_t)((t[i] - t[i + RL_KEM_N]) * t[i + 2*RL_KEM_N]);
    }
    bit_xof128_stream_release(&stream);
}
