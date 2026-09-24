/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-256 instance.
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "params.h"
#include "poly.h"

uint32_t load32_littleendian(const uint8_t *a)
{
    uint32_t r;
    r  = (uint32_t)a[0];
    r |= (uint32_t)a[1] << 8;
    r |= (uint32_t)a[2] << 16;
    r |= (uint32_t)a[3] << 24;
    return r;
}

void cbd1(int16_t *r, const uint8_t *buf)
{
    unsigned int i, j;
    uint32_t t;
    int16_t a, b;

    for(i = 0; i < ZEN_N/16; i++)
    {
        t = load32_littleendian(buf+4*i);
        for(j = 0; j < 16; j++)
        {
            a = (t >> (2*j+0)) & 0x1;
            b = (t >> (2*j+1)) & 0x1;
            r[16*i+j] = a - b;
        }
    }
}

void tenary3_16(int16_t *r, const uint8_t *buf)
{
    unsigned int i;
    int16_t t[ZEN_N * 4];

    poly_byte2bit_unpack(t, buf, ZEN_N * 4);

    for(i = 0; i < ZEN_N; i++)
    {
        r[i] = (t[i] & t[i + ZEN_N]) - (t[i + 2 * ZEN_N] & t[i + 3 * ZEN_N]);
    }
}

void tenary1_8(int16_t *r, const uint8_t *buf)
{
    unsigned int i;
    int16_t t[ZEN_N * 3];

    poly_byte2bit_unpack(t, buf, ZEN_N * 3);

    for(i = 0; i < ZEN_N; i++)
    {
        r[i] = (t[i] - t[i + ZEN_N]) * t[i + 2 * ZEN_N];
    }
}