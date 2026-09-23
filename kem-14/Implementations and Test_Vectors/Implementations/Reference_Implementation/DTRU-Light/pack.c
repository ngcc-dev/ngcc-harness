#include "pack.h"
#include "params.h"

void pack_pk(unsigned char *r, const poly *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 4; i++)
    {
        r[5 * i + 0] = (a->coeffs[4 * i] >> 0);
        r[5 * i + 1] = (a->coeffs[4 * i] >> 8) | (a->coeffs[4 * i + 1] << 2);
        r[5 * i + 2] = (a->coeffs[4 * i + 1] >> 6) | (a->coeffs[4 * i + 2] << 4);
        r[5 * i + 3] = (a->coeffs[4 * i + 2] >> 4) | (a->coeffs[4 * i + 3] << 6);
        r[5 * i + 4] = (a->coeffs[4 * i + 3] >> 2);
    }
}

void unpack_pk(poly *r, const unsigned char *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 4; i++)
    {
        r->coeffs[4 * i] = ((a[5 * i + 0] >> 0) | ((uint16_t)a[5 * i + 1] << 8)) & 0x3FF;
        r->coeffs[4 * i + 1] = ((a[5 * i + 1] >> 2) | ((uint16_t)a[5 * i + 2] << 6)) & 0x3FF;
        r->coeffs[4 * i + 2] = ((a[5 * i + 2] >> 4) | ((uint16_t)a[5 * i + 3] << 4)) & 0x3FF;
        r->coeffs[4 * i + 3] = ((a[5 * i + 3] >> 6) | ((uint16_t)a[5 * i + 4] << 2)) & 0x3FF;
    }
}

void pack_sk(unsigned char *r, const poly *a)
{
    unsigned int i;
    uint8_t t[8];

    for (i = 0; i < DTRU_N / 8; i++)
    {
        t[0] = DTRU_BOUND - a->coeffs[8 * i + 0];
        t[1] = DTRU_BOUND - a->coeffs[8 * i + 1];
        t[2] = DTRU_BOUND - a->coeffs[8 * i + 2];
        t[3] = DTRU_BOUND - a->coeffs[8 * i + 3];
        t[4] = DTRU_BOUND - a->coeffs[8 * i + 4];
        t[5] = DTRU_BOUND - a->coeffs[8 * i + 5];
        t[6] = DTRU_BOUND - a->coeffs[8 * i + 6];
        t[7] = DTRU_BOUND - a->coeffs[8 * i + 7];
        r[3 * i + 0] = (t[0] >> 0) | (t[1] << 3) | (t[2] << 6) ;
        r[3 * i + 1] = (t[2] >> 2) | (t[3] << 1)  | (t[4] << 4) | (t[5] << 7);
        r[3 * i + 2] = (t[5] >> 1) | (t[6] << 2)| (t[7] << 5);
    }
}

void unpack_sk(poly *r, const unsigned char *a)
{
    int i;
    for (i = 0; i < DTRU_N / 8; ++i)
    {
        r->coeffs[8 * i + 0] = DTRU_BOUND - ((a[3*i] >> 0)& 0x7);
        r->coeffs[8 * i + 1] = DTRU_BOUND - ((a[3*i] >> 3)& 0x7);
        r->coeffs[8 * i + 2] = DTRU_BOUND - (((a[3*i] >> 6) | (a[3*i+1] << 2 ))& 0x7);
        r->coeffs[8 * i + 3] = DTRU_BOUND - ((a[3*i+1] >> 1)& 0x7);
        r->coeffs[8 * i + 4] = DTRU_BOUND - ((a[3*i+1] >> 4)& 0x7);
        r->coeffs[8 * i + 5] = DTRU_BOUND - (((a[3*i+1] >> 7) | (a[3*i+2] << 1 ))& 0x7);
        r->coeffs[8 * i + 6] = DTRU_BOUND - ((a[3*i+2] >> 2)& 0x7);
        r->coeffs[8 * i + 7] = DTRU_BOUND - ((a[3*i+2] >> 5)& 0x7);
    }
}

void pack_ct(unsigned char *r, const poly *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N; ++i)
    {
        r[i] = (uint8_t)a->coeffs[i];
    }
}

void unpack_decompress_ct(poly *r, const unsigned char *a)
{
    unsigned int i;
    int32_t temp;
    for (i = 0; i < DTRU_N; ++i)
    {
        temp = ((int32_t)(a[i] * DTRU_Q) + (DTRU_Q2 >> 1)) >> DTRU_LOGQ2;
        r->coeffs[i] = temp;
    }
}


// void pack_sk_f(unsigned char *r, const poly *a)
// {
//   int i;
//   unsigned char c;

//   for(i=0; i<DTRU_N/5; i++)
//   {
//     c =       (a->coeffs[5*i+4] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+3] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+2] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+1] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+0] + DTRU_ETA) & 255;
//     r[i] = c;
//   }
// #if DTRU_N > (DTRU_N / 5) * 5  
//   int j;
//   i = DTRU_N / 5;
//   c = 0;
//   for(j = DTRU_N - (5*i) - 1; j>=0; j--)
//     c = (3*c + a->coeffs[5*i+j] + DTRU_ETA) & 255;
//   r[i] = c;
// #endif
// }

// void unpack_sk_f(poly *r, const unsigned char *a)
// {
//   int i;
//   unsigned char c;

//   for(i=0; i<DTRU_N/5; i++)
//   {
//     c = a[i];
//     r->coeffs[5*i+0] = (c%3 - DTRU_ETA) << 1; c/=3; 
//     r->coeffs[5*i+1] = (c%3 - DTRU_ETA) << 1; c/=3;  
//     r->coeffs[5*i+2] = (c%3 - DTRU_ETA) << 1; c/=3; 
//     r->coeffs[5*i+3] = (c%3 - DTRU_ETA) << 1; c/=3;  
//     r->coeffs[5*i+4] = (c%3 - DTRU_ETA) << 1;  
//   }
// #if DTRU_N > (DTRU_N / 5) * 5  // if 5 does not divide NTRU_N-1
//   i = DTRU_N/5;
//   int j;
//   c = a[i];
//   for(j=0; (5*i+j)<DTRU_N; j++)
//   {
//     r->coeffs[5*i+j] =(c%3 - DTRU_ETA) << 1; c /= 3;
//   }
// #endif
//   r->coeffs[0] += 1; 
// }
