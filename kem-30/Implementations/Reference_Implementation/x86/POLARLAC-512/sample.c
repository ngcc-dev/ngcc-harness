/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the reference POLARLAC-512 instance.
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


// Ternary sampler: {-1:11/128; 0:106/128; 1:11/128}.
void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t x0, x1, x2, x3, x4, x5, x6, k;
    uint16_t mneg, mpos;
    unsigned int i, j;
    uint8_t buf[RL_KEM_N_LEN_BYTES * 7];

    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N_LEN_BYTES * 7);

    i = 0;
    for(j = 0; j < RL_KEM_N;)
    {
        x0 = buf[i++];
        x1 = buf[i++];
        x2 = buf[i++];
        x3 = buf[i++];
        x4 = buf[i++];
        x5 = buf[i++];
        x6 = buf[i++];

        k = x0 & 127;
        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));

        k = x1 & 127;
        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));

        k = x2 & 127;
        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));

        k = x3 & 127;
        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));

        k = x4 & 127;
        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));

        k = x5 & 127;
        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));

        k = x6 & 127;
        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));

        k = (((x0 >> 7) & 1) << 6)
          | (((x1 >> 7) & 1) << 5)
          | (((x2 >> 7) & 1) << 4)
          | (((x3 >> 7) & 1) << 3)
          | (((x4 >> 7) & 1) << 2)
          | (((x5 >> 7) & 1) << 1)
          |  ((x6 >> 7) & 1);

        mneg = (uint16_t)(0 - (((uint16_t)k - 11) >> 15));
        mpos = (uint16_t)(0 - (((uint16_t)116 - (uint16_t)k) >> 15));
        a[j++] = (int16_t)((mpos & 1) - (mneg & 1));
    }
    bit_xof128_stream_release(&stream);
}
