/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

#include <stdint.h>
#include "ntt.h"

#if BIT_Q != 26881 || BIT_N != 256
#error "ntt.c constants are generated for BIT_Q=26881 and BIT_N=256"
#endif

#define Beta 65536
#define INVERSE_Q -26879
#define INVERSE_N_BETA 512
#define MONT_R2 1759

static const int16_t M[BIT_N/2] = {
11774,2448,16448,16995,8819,20943,12635,19983,
17598,14846,26310,17219,23857,25613,24862,6087,
22430,1796,8202,7066,8271,26829,2,1352,
3160,12561,25364,22867,15175,16639,8665,24363,
20150,19614,13720,775,3787,6317,9062,23925,
17268,6814,8009,11003,23208,16985,14855,15367,
7492,10964,20256,10627,7566,7226,18332,291,
13723,2803,19536,7765,11756,17161,16916,10791,
15938,21688,15708,613,11632,13980,20140,12854,
20977,14165,19099,8044,20646,5457,824,19404,
12308,13979,2564,12880,4493,26596,17587,7410,
19387,14565,6677,24525,17519,15204,1483,7911,
16354,7213,4892,629,8866,25834,11413,341,
22270,1160,12362,23602,25877,20202,26104,12368,
22452,16668,7630,23609,1146,22028,23966,18654,
17832,11744,20226,17228,18941,8760,18273,14169
};

static const int16_t Mn[BIT_N/2] = {
11774,24433,9886,10433,6898,14246,5938,18062,
20794,2019,1268,3024,9662,571,12035,9283,
2518,18216,10242,11706,4014,1517,14320,23721,
25529,26879,52,18610,19815,18679,25085,4451,
16090,9965,9720,15125,19116,7345,24078,13158,
26590,8549,19655,19315,16254,6625,15917,19389,
11514,12026,9896,3673,15878,18872,20067,9613,
2956,17819,20564,23094,26106,13161,7267,6731,
12712,8608,18121,7940,9653,6655,15137,9049,
8227,2915,4853,25735,3272,19251,10213,4429,
14513,777,6679,1004,3279,14519,25721,4611,
26540,15468,1047,18015,26252,21989,19668,10527,
18970,25398,11677,9362,2356,20204,12316,7494,
19471,9294,285,22388,14001,24317,12902,14573,
7477,26057,21424,6235,18837,7782,12716,5904,
14027,6741,12901,15249,26268,11173,5193,10943
};


static inline int32_t MontgomerymapFull(int32_t a)  
{
    uint32_t m;
    int32_t t, r;

    m = ((uint32_t)a * (uint32_t)INVERSE_Q) & 0xFFFF;
    t = (int32_t)(m * (uint32_t)BIT_Q);
    r = (a >> 16) - (t >> 16);
    r = r + ((r >> 31) & BIT_Q);
    return r;
}

static inline int32_t mul_mod(int16_t a, int16_t b)
{   
     return MontgomerymapFull((int32_t)a*(int32_t)b);
}


static inline int NTT_Lazy(int16_t *a)
{
    
    int t,m,i,j,s,e;
    int32_t U=0,V=0;
    int16_t S;
    t=BIT_N; 

    for(m=1;m<BIT_N/2;m<<=1) 
    {
        t=(t>>1);
        for(i=0;i<m;i++)  
        {
            s = (i * t) << 1;
            e=s+t;
            S=M[m+i];
            for(j=s;j<e;j++)  
            {
                U=a[j];
                V=a[j+t];

                V=mul_mod(V,S);

                a[j]=reduce_modq(U+V);
                a[j+t]=reduce_modq(U-V+BIT_Q);


            }
        }
    }
    
    return 0;
}


static inline int INTT_Lazy(int16_t *a)
{
    int t,m,i,j,s,e,h;
    int32_t U=0,V=0;
    int16_t S;
    t=2; 
   

    for(m=BIT_N/2;m>1;m>>=1) 
    {
        s=0;
        h=m>>1;
        for(i=0;i<h;i++) 
        {
            e=s+t;
            S=Mn[h+i];
            for(j=s;j<e;j++)  
            {
                U=a[j];
                V=a[j+t];

                a[j] = reduce_modq(U + V);

                a[j + t] = mul_mod(reduce_modq(U - V + BIT_Q), S);
            }
            s=s+2*t;
        }
        t*=2;
    }
    
    for(i=0;i<BIT_N;i++)
    {
        a[i] = mul_mod(a[i], INVERSE_N_BETA);
       
    }

    return 0;
}


void ntt_forward(int16_t a[BIT_N]) {
    (void)NTT_Lazy(a);
}

void ntt_inverse(int16_t a[BIT_N]) {
    (void)INTT_Lazy(a);
}

void ntt_basemul_raw(int16_t r[BIT_N], const int16_t a[BIT_N], const int16_t b[BIT_N]) {
    int j=0;

    for(int i=0;i<BIT_N-3;i+=4)
    {
        int32_t root=M[(BIT_N/4)+j];
        int32_t tmp;

        tmp = mul_mod(a[i], b[i]) + mul_mod( mul_mod(a[i+1],b[i+1]) , root);
        r[i] = reduce_modq(tmp);

        tmp = mul_mod(a[i] , b[i+1]) + mul_mod(a[i+1] , b[i]);
        r[i+1] = reduce_modq(tmp);

        tmp = mul_mod(a[i+2] , b[i+2]) + mul_mod( mul_mod(a[i+3] , b[i+3]) , (BIT_Q-root) );
        r[i+2] = reduce_modq(tmp);

        tmp = mul_mod(a[i+2] , b[i+3]) + mul_mod(a[i+3], b[i+2]);
        r[i+3] = reduce_modq(tmp);

        j++;
    }
}

void ntt_basemul_acc_raw(int16_t r[BIT_N], const int16_t a[BIT_N], const int16_t b[BIT_N]) {
    int j=0;

    for(int i=0;i<BIT_N-3;i+=4)
    {
        int32_t root=M[(BIT_N/4)+j];
        int32_t tmp;
        int32_t val;

        tmp = mul_mod(a[i], b[i]) + mul_mod( mul_mod(a[i+1],b[i+1]) , root);
        val = (int32_t)r[i] + reduce_modq(tmp) - BIT_Q;
        val += (val >> 31) & BIT_Q;
        r[i] = (int16_t)val;

        tmp = mul_mod(a[i] , b[i+1]) + mul_mod(a[i+1] , b[i]);
        val = (int32_t)r[i+1] + reduce_modq(tmp) - BIT_Q;
        val += (val >> 31) & BIT_Q;
        r[i+1] = (int16_t)val;

        tmp = mul_mod(a[i+2] , b[i+2]) + mul_mod( mul_mod(a[i+3] , b[i+3]) , (BIT_Q-root) );
        val = (int32_t)r[i+2] + reduce_modq(tmp) - BIT_Q;
        val += (val >> 31) & BIT_Q;
        r[i+2] = (int16_t)val;

        tmp = mul_mod(a[i+2] , b[i+3]) + mul_mod(a[i+3], b[i+2]);
        val = (int32_t)r[i+3] + reduce_modq(tmp) - BIT_Q;
        val += (val >> 31) & BIT_Q;
        r[i+3] = (int16_t)val;

        j++;
    }
}

void ntt_montgomery_lift(int16_t r[BIT_N]) {
    for (int i = 0; i < BIT_N; i++) {
        r[i] = (int16_t)mul_mod(r[i], MONT_R2);
    }
}

