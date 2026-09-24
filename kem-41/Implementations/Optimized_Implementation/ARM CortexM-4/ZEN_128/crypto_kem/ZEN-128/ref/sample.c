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
#include "poly.h"
#include "sample.h"

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

void cbd2(int16_t *r, const uint8_t *buf)
{
    unsigned int i,j;
    uint32_t t, d;
    int16_t a, b;

    for(i = 0; i < ZEN_N/8; i++) 
    {
        t  = load32_littleendian(buf+4*i);
        d  = t & 0x55555555;
        d += (t>>1) & 0x55555555;
        for(j = 0; j < 8; j++) 
        {
            a = (d >> (4*j+0)) & 0x3;
            b = (d >> (4*j+2)) & 0x3;
            r[8*i+j] = a - b;
        }
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

void tenary3_32(int16_t *r, const uint8_t *buf)
{
    unsigned int i;
    int16_t t[ZEN_N * 5];

    poly_byte2bit_unpack(t, buf, ZEN_N * 5);

    for(i = 0; i < ZEN_N; i++)
    {
        r[i] = ((t[i] & t[i + ZEN_N]) - (t[i + 2 * ZEN_N] & t[i + 3 * ZEN_N])) * t[i + 4 * ZEN_N];
    }
}