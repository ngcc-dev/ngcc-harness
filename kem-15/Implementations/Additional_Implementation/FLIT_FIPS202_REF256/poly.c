#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "symmetric.h"

/*************************************************
* Name:        poly_freeze
*
* Description: Applies Barrett reduction to all coefficients of a polynomial
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_freeze(poly *r)
{
    unsigned int i;
    for(i = 0; i < N; i++)
        r->coeffs[i] = freeze(r->coeffs[i]);
}

/*************************************************
* Name:        poly_freeze_centered
*
* Description: Applies centered reduction to all coefficients of a polynomial
*           for details of the centered reduction see comments in reduce.c
* 
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_freeze_centered(poly *r)
{
    unsigned int i;
    for(i = 0; i < N; i++)
        r->coeffs[i] = freeze_centered(r->coeffs[i]);
}

/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
void poly_ntt(poly *r)
{
    ntt(r->coeffs);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
void poly_invntt_tomont(poly *r)
{
    invntt_tomont(r->coeffs);
}

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
{
    unsigned int i;
#if KEM_MODE == 128
    for (i = 0; i < N/8; i++) {
        basemul(&r->coeffs[8*i], &a->coeffs[8*i], &b->coeffs[8*i], zetas[64+i]);
        basemul(&r->coeffs[8*i+4], &a->coeffs[8*i+4], &b->coeffs[8*i+4], -zetas[64+i]);
    }
#elif KEM_MODE == 256
    for (i = 0; i < N/16; i++) {
        basemul(&r->coeffs[16*i], &a->coeffs[16*i], &b->coeffs[16*i], zetas[64+i]);
        basemul(&r->coeffs[16*i+8], &a->coeffs[16*i+8], &b->coeffs[16*i+8], -zetas[64+i]);
    }
#elif KEM_MODE == 512
    for (i = 0; i < N/32; i++) {
        basemul(&r->coeffs[32*i], &a->coeffs[32*i], &b->coeffs[32*i], zetas[64+i]);
        basemul(&r->coeffs[32*i+16], &a->coeffs[32*i+16], &b->coeffs[32*i+16], -zetas[64+i]);
    }
#else
#error "Invalid KEM_MODE"
#endif
}

/*************************************************
* Name:        poly_tomont
*
* Description: Inplace conversion of all coefficients of a polynomial
*              from normal domain to Montgomery domain
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_tomont(poly *r)
{
    unsigned int i;
    for (i = 0; i < N; i++)
        r->coeffs[i] = montgomery_reduce((int32_t)r->coeffs[i] * MONTSQ);
}

/*************************************************
* Name:        poly_baseinv
*
* Description: Inversion of polynomial used for inversion
*              of element in Rq in NTT domain
*  
* Arguments:   - poly *b: pointer to the output polynomial
*              - const poly *a: pointer to the input polynomial
***************************************************/
int poly_baseinv(poly *b, const poly *a) 
{
    int result = 0;
    unsigned int i;

#if KEM_MODE == 128
    for(i = 0; i < N/8; i++){
        result += baseinv(&b->coeffs[8*i], &a->coeffs[8*i], zetas[64+i]);
        result += baseinv(&b->coeffs[8*i + 4], &a->coeffs[8*i + 4], -zetas[64+i]);
    }
#elif KEM_MODE == 256
    for(i = 0; i < N/16; i++){
        result += baseinv(&b->coeffs[16*i], &a->coeffs[16*i], zetas[64+i]);
        result += baseinv(&b->coeffs[16*i + 8], &a->coeffs[16*i + 8], -zetas[64+i]);
    }
#elif KEM_MODE == 512
    for(i = 0; i < N/32; i++){
        result += baseinv(&b->coeffs[32*i], &a->coeffs[32*i], zetas[64+i]);
        result += baseinv(&b->coeffs[32*i + 16], &a->coeffs[32*i + 16], -zetas[64+i]);
    }
#else
#error "Invalid KEM_MODE"
#endif

    return result;
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b)
{
    unsigned int i;
    for (i = 0; i < N; i++)
        r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b)
{
    unsigned int i;
    for (i = 0; i < N; i++)
        r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
}

/*************************************************
* Name:        poly_sub_halfq
*
* Description: Subtract half of q from each coefficient of a polynomial
*
* Arguments: - poly *r:       pointer to output polynomial
**************************************************/
void poly_sub_halfq(poly *r)
{
    unsigned int i;
    for (i = 0; i < N; i++) 
    {
        r->coeffs[i] = r->coeffs[i] - HALF_Q;
    }
    poly_freeze_centered(r);
}

/*************************************************
* Name:        poly_compress_and_pack
*
* Description: Compression and subsequent serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (of length KEM_POLYCOMPRESSEDBYTES)
*              - poly *a:    pointer to input polynomial
**************************************************/
void poly_compress_and_pack(uint8_t r[KEM_POLYCOMPRESSEDBYTES], poly *a)
{
    unsigned int i;
    
    poly_freeze(a);

#if D == 8
    for(i = 0; i < N; i++) {          
        r[i] = ((((uint16_t)a->coeffs[i] << D) + Q / 2) / Q) & 0xFF;
    }
#elif D == 9
    uint16_t t[8];
    for (i = 0; i < N/8; i++) {
        t[0] = ((((uint16_t)a->coeffs[8*i+0] << D) + Q / 2) / Q) & 0x1FF;
        t[1] = ((((uint16_t)a->coeffs[8*i+1] << D) + Q / 2) / Q) & 0x1FF;
        t[2] = ((((uint16_t)a->coeffs[8*i+2] << D) + Q / 2) / Q) & 0x1FF;
        t[3] = ((((uint16_t)a->coeffs[8*i+3] << D) + Q / 2) / Q) & 0x1FF;
        t[4] = ((((uint16_t)a->coeffs[8*i+4] << D) + Q / 2) / Q) & 0x1FF;
        t[5] = ((((uint16_t)a->coeffs[8*i+5] << D) + Q / 2) / Q) & 0x1FF;
        t[6] = ((((uint16_t)a->coeffs[8*i+6] << D) + Q / 2) / Q) & 0x1FF;
        t[7] = ((((uint16_t)a->coeffs[8*i+7] << D) + Q / 2) / Q) & 0x1FF;

        r[9*i+0] =  t[0] & 0xFF;
        r[9*i+1] = (t[0] >> 8) | ((t[1] & 0x7F) << 1);
        r[9*i+2] = (t[1] >> 7) | ((t[2] & 0x3F) << 2);
        r[9*i+3] = (t[2] >> 6) | ((t[3] & 0x1F) << 3);
        r[9*i+4] = (t[3] >> 5) | ((t[4] & 0x0F) << 4);
        r[9*i+5] = (t[4] >> 4) | ((t[5] & 0x07) << 5);
        r[9*i+6] = (t[5] >> 3) | ((t[6] & 0x03) << 6);
        r[9*i+7] = (t[6] >> 2) | ((t[7] & 0x01) << 7);
        r[9*i+8] =  t[7] >> 1;
    }
#else
#error "Compression factor needs to be defined for D in {8,9}"
#endif 
}

/*************************************************
* Name:        poly_unpack_and_decompress
*
* Description: De-serialization and subsequent decompression of a polynomial;
*              approximate inverse of poly_compress_and_pack
*
* Arguments:   - poly *r:          pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of length KEM_POLYCOMPRESSEDBYTES bytes)
**************************************************/
void poly_unpack_and_decompress(poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES])
{
    unsigned int i;

#if D == 8
    for(i = 0; i < N; i++) {
        r->coeffs[i] = ((uint16_t)a[i] * Q + (1 << (D-1))) >> D;
    }
#elif D == 9
    for (i = 0; i < N/8; i++) {
        r->coeffs[8*i+0] = (((a[9*i+0] >> 0) | ((a[9*i+1] & 0x01) << 8)) * Q + (1 << (D-1))) >> D;
        r->coeffs[8*i+1] = (((a[9*i+1] >> 1) | ((a[9*i+2] & 0x03) << 7)) * Q + (1 << (D-1))) >> D; 
        r->coeffs[8*i+2] = (((a[9*i+2] >> 2) | ((a[9*i+3] & 0x07) << 6)) * Q + (1 << (D-1))) >> D;
        r->coeffs[8*i+3] = (((a[9*i+3] >> 3) | ((a[9*i+4] & 0x0F) << 5)) * Q + (1 << (D-1))) >> D;
        r->coeffs[8*i+4] = (((a[9*i+4] >> 4) | ((a[9*i+5] & 0x1F) << 4)) * Q + (1 << (D-1))) >> D;
        r->coeffs[8*i+5] = (((a[9*i+5] >> 5) | ((a[9*i+6] & 0x3F) << 3)) * Q + (1 << (D-1))) >> D;
        r->coeffs[8*i+6] = (((a[9*i+6] >> 6) | ((a[9*i+7] & 0x7F) << 2)) * Q + (1 << (D-1))) >> D;
        r->coeffs[8*i+7] = (((a[9*i+7] >> 7) | ((a[9*i+8] & 0xFF) << 1)) * Q + (1 << (D-1))) >> D;
    }
#else
#error "Decompression factor needs to be defined for D in {8,9}"
#endif
}  

/*************************************************
* Name:        poly_to_bytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for KEM_POLYBYTES bytes)
*              - poly *a:    pointer to input polynomial
**************************************************/
void poly_to_bytes(uint8_t r[KEM_POLYBYTES], const poly *a)
{
    unsigned int i;
#if KEM_MODE == 128
    {
        unsigned int j;

        for (i = 0; i < 102; i++) {
            uint64_t code;
            uint32_t L = 0;
            uint64_t H = 0;
            uint64_t base = 1;

            for (j = 0; j < 5; j++) {
                uint16_t c = (uint16_t)a->coeffs[5*i + j];
                uint16_t l = c & 7;
                uint16_t h = c >> 3;
                L |= (uint32_t)l << (3*j);
                H += (uint64_t)h * base;
                base *= 97;
            }
            code = (H << 15) | L;

            r[6*i + 0] = (uint8_t)(code);
            r[6*i + 1] = (uint8_t)(code >>  8);
            r[6*i + 2] = (uint8_t)(code >> 16);
            r[6*i + 3] = (uint8_t)(code >> 24);
            r[6*i + 4] = (uint8_t)(code >> 32);
            r[6*i + 5] = (uint8_t)(code >> 40);
        }

        /* tail: 2 coefficients → 20 bits */
        {
            uint32_t t;
            t  = (uint32_t)(uint16_t)a->coeffs[510];
            t |= (uint32_t)(uint16_t)a->coeffs[511] << 10;
            r[612] = (uint8_t)(t);
            r[613] = (uint8_t)(t >>  8);
            r[614] = (uint8_t)(t >> 16);
        }
    }
#elif KEM_MODE == 256
    {
        unsigned int j;

        for (i = 0; i < 204; i++) {
            uint64_t code;
            uint32_t L = 0;
            uint64_t H = 0;
            uint64_t base = 1;

            for (j = 0; j < 5; j++) {
                uint16_t c = (uint16_t)a->coeffs[5*i + j];
                uint16_t l = c & 7;
                uint16_t h = c >> 3;
                L |= (uint32_t)l << (3*j);
                H += (uint64_t)h * base;
                base *= 97;
            }
            code = (H << 15) | L;

            r[6*i + 0] = (uint8_t)(code);
            r[6*i + 1] = (uint8_t)(code >>  8);
            r[6*i + 2] = (uint8_t)(code >> 16);
            r[6*i + 3] = (uint8_t)(code >> 24);
            r[6*i + 4] = (uint8_t)(code >> 32);
            r[6*i + 5] = (uint8_t)(code >> 40);
        }

        /* tail: 4 coefficients → 40 bits */
        {
            uint64_t t;
            t  = (uint64_t)(uint16_t)a->coeffs[1020];
            t |= (uint64_t)(uint16_t)a->coeffs[1021] << 10;
            t |= (uint64_t)(uint16_t)a->coeffs[1022] << 20;
            t |= (uint64_t)(uint16_t)a->coeffs[1023] << 30;
            r[1224] = (uint8_t)(t);
            r[1225] = (uint8_t)(t >>  8);
            r[1226] = (uint8_t)(t >> 16);
            r[1227] = (uint8_t)(t >> 24);
            r[1228] = (uint8_t)(t >> 32);
        }
    }

#elif KEM_MODE == 512
    {
        uint16_t c0, c1;

        for (i = 0; i < N / 2; i++) {
            c0 = (uint16_t)a->coeffs[2*i + 0];
            c1 = (uint16_t)a->coeffs[2*i + 1];

            r[3*i + 0] = (uint8_t)(c0);
            r[3*i + 1] = (uint8_t)((c0 >> 8) | (c1 << 4));
            r[3*i + 2] = (uint8_t)(c1 >> 4);
        }
    }
#else
    #error "Unsupported KEM_MODE: must be 128, 256, or 512"
#endif
}

/*************************************************
* Name:        poly_from_bytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_to_bytes
*
* Arguments:   - poly *r:          pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of KEM_POLYBYTES bytes)
**************************************************/
void poly_from_bytes(poly *r, const uint8_t a[KEM_POLYBYTES])
{
    unsigned int i;

#if KEM_MODE == 128
    {
        unsigned int j;

        for (i = 0; i < 102; i++) {
            uint64_t code;

            code  = (uint64_t)a[6*i + 0];
            code |= (uint64_t)a[6*i + 1] <<  8;
            code |= (uint64_t)a[6*i + 2] << 16;
            code |= (uint64_t)a[6*i + 3] << 24;
            code |= (uint64_t)a[6*i + 4] << 32;
            code |= (uint64_t)a[6*i + 5] << 40;

            {
                uint32_t L  = (uint32_t)(code & 0x7FFF);
                uint64_t H  = code >> 15;

                for (j = 0; j < 5; j++) {
                    uint16_t l = (L >> (3*j)) & 7;
                    uint16_t h = (uint16_t)(H % 97);
                    H /= 97;
                    r->coeffs[5*i + j] = (int16_t)((h << 3) | l);
                }
            }
        }

        {
            uint32_t t;
            t  = (uint32_t)a[612];
            t |= (uint32_t)a[613] <<  8;
            t |= (uint32_t)a[614] << 16;
            r->coeffs[510] = (int16_t)( t        & 0x3FF);
            r->coeffs[511] = (int16_t)((t >> 10) & 0x3FF);
        }
    }
#elif KEM_MODE == 256
    {
        unsigned int j;

        for (i = 0; i < 204; i++) {
            uint64_t code;

            code  = (uint64_t)a[6*i + 0];
            code |= (uint64_t)a[6*i + 1] <<  8;
            code |= (uint64_t)a[6*i + 2] << 16;
            code |= (uint64_t)a[6*i + 3] << 24;
            code |= (uint64_t)a[6*i + 4] << 32;
            code |= (uint64_t)a[6*i + 5] << 40;

            {
                uint32_t L  = (uint32_t)(code & 0x7FFF);
                uint64_t H  = code >> 15;

                for (j = 0; j < 5; j++) {
                    uint16_t l = (L >> (3*j)) & 7;
                    uint16_t h = (uint16_t)(H % 97);
                    H /= 97;
                    r->coeffs[5*i + j] = (int16_t)((h << 3) | l);
                }
            }
        }

        {
            uint64_t t;
            t  = (uint64_t)a[1224];
            t |= (uint64_t)a[1225] <<  8;
            t |= (uint64_t)a[1226] << 16;
            t |= (uint64_t)a[1227] << 24;
            t |= (uint64_t)a[1228] << 32;
            r->coeffs[1020] = (int16_t)( t        & 0x3FF);
            r->coeffs[1021] = (int16_t)((t >> 10) & 0x3FF);
            r->coeffs[1022] = (int16_t)((t >> 20) & 0x3FF);
            r->coeffs[1023] = (int16_t)((t >> 30) & 0x3FF);
        }
    }
#elif KEM_MODE == 512
    for (i = 0; i < N / 2; i++) {
        r->coeffs[2*i + 0] = (int16_t)( (uint16_t) a[3*i + 0]             |
                                        ((uint16_t)(a[3*i + 1] & 0x0F) << 8));
        r->coeffs[2*i + 1] = (int16_t)(((uint16_t) a[3*i + 1] >> 4)       |
                                        ((uint16_t) a[3*i + 2]        << 4));
    }

#else
    #error "Unsupported KEM_MODE"
#endif
}

/*************************************************
* Name:        poly_from_msg
*
* Description: Convert message to polynomial
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
**************************************************/
void poly_from_msg(poly *r, const uint8_t msg[KEM_CPAPKE_MSGBYTES])
{
    unsigned int i, j;
    int16_t mask;
#if KEM_MODE == 128
    for(i = 0; i < KEM_CPAPKE_MSGBYTES; i++) {    
        for(j = 0; j < 8; j++) 
        {       
            mask = -(int16_t)((msg[i] >> j) & 1);      
            uint16_t val = mask & HALF_Q;
            r->coeffs[8*i + j]       = val;
            r->coeffs[8*i + j + N/2] = val;
        }
    }
#elif KEM_MODE == 256 || KEM_MODE == 512
    for(i = 0; i < KEM_CPAPKE_MSGBYTES; i++) {    
        for(j = 0; j < 8; j++) 
        {       
            mask = -(int16_t)((msg[i] >> j) & 1);      
            uint16_t val = mask & HALF_Q;
            r->coeffs[8*i + j]         = val;
            r->coeffs[8*i + j + N/4]   = val;
            r->coeffs[8*i + j + N/2]   = val;
            r->coeffs[8*i + j + 3*N/4] = val;
        }
    }
#else
#error "Message to polynomial conversion needs to be defined for this KEM mode"
#endif
}

/*************************************************
* Name:        ternary_p
*
* Description: Sample a polynomial with coefficients in {-1,0,1} according to probabilities
*              P(-1) = P(1) = P, P(0) = 1 - 2*P
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - uint8_t *buf:        pointer to input buffer for randomness
*                                      (needs to have enough bytes to sample N coefficients)
**************************************************/
static void ternary_15625(poly *r, const uint8_t *buf)  // 5/32
{
    unsigned int i, j, k;
    
    for (i = 0; i < N/8; i++) {
        uint64_t w = 0;
        
        for (j = 0; j < 5; j++) 
        {
            w |= (uint64_t)buf[5*i + j] << (8*j);
        }
        
        for (k = 0; k < 8; k++) {
            uint16_t v = (w >> (5*k)) & 0x1F;
            uint16_t lt5 = (v - 5) >> 15;
            uint16_t lt10 = (v - 10) >> 15;
            r->coeffs[8*i + k] = (int16_t)lt5 - (int16_t)((lt5 ^ 1) & lt10);
        }
    }
}
static void ternary_1875(poly *r, const uint8_t *buf) // 3/16
{
    unsigned int i;
    for (i = 0; i < N/2; i++) {
        uint8_t w = buf[i];
        uint16_t lo = w & 0x0F;
        uint16_t hi = w >> 4;
        uint16_t lt3_lo = (uint16_t)(lo - 3) >> 15;
        uint16_t lt6_lo = (uint16_t)(lo - 6) >> 15;
        uint16_t lt3_hi = (uint16_t)(hi - 3) >> 15;
        uint16_t lt6_hi = (uint16_t)(hi - 6) >> 15;

        r->coeffs[2*i]   = (int16_t)((2*lt3_lo - 1) * lt6_lo);
        r->coeffs[2*i+1] = (int16_t)((2*lt3_hi - 1) * lt6_hi);
    }
}
static void ternary_3125(poly *r, const uint8_t *buf) // 5/16
{
    unsigned int i;
    for (i = 0; i < N/2; i++) {
        uint8_t w = buf[i];

        uint16_t lo = w & 0x0F;
        uint16_t hi = w >> 4;
        uint16_t lt5_lo = (uint16_t)(lo - 5) >> 15;
        uint16_t lt10_lo = (uint16_t)(lo - 10) >> 15;
        uint16_t lt5_hi = (uint16_t)(hi - 5) >> 15;
        uint16_t lt10_hi = (uint16_t)(hi - 10) >> 15;

        r->coeffs[2*i]   = (int16_t)lt5_lo - (int16_t)((lt5_lo ^ 1) & lt10_lo);
        r->coeffs[2*i+1] = (int16_t)lt5_hi - (int16_t)((lt5_hi ^ 1) & lt10_hi);
    }
}
static void ternary_4375(poly *r, const uint8_t *buf) // 7/16
{
    unsigned int i;
    for (i = 0; i < N/2; i++) {
        uint8_t w = buf[i];

        uint16_t lo = w & 0x0F;
        uint16_t hi = w >> 4;
        uint16_t lt7_lo  = (uint16_t)(lo - 7) >> 15;
        uint16_t lt14_lo = (uint16_t)(lo - 14) >> 15;
        uint16_t lt7_hi  = (uint16_t)(hi - 7) >> 15;
        uint16_t lt14_hi = (uint16_t)(hi - 14) >> 15;

        r->coeffs[2*i] =  (int16_t)lt7_lo - (int16_t)((lt7_lo ^ 1) & lt14_lo);
        r->coeffs[2*i+1] = (int16_t)lt7_hi - (int16_t)((lt7_hi ^ 1) & lt14_hi);
    }
}
static void ternary_25(poly *r, const uint8_t *buf) // 1/4
{   
    unsigned int i;
    for(i = 0; i < N/4; i++) {
        uint8_t w = buf[i];

        r->coeffs[4*i+0] = (int16_t)((w >> 0) & 1) - (int16_t)((w >> 1) & 1);
        r->coeffs[4*i+1] = (int16_t)((w >> 2) & 1) - (int16_t)((w >> 3) & 1);
        r->coeffs[4*i+2] = (int16_t)((w >> 4) & 1) - (int16_t)((w >> 5) & 1);
        r->coeffs[4*i+3] = (int16_t)((w >> 6) & 1) - (int16_t)((w >> 7) & 1);
    }
}
static void ternary_125(poly *r, const uint8_t *buf) // 1/8
{
    unsigned int i,j;
    
    for (i = 0; i < N/8; i++) {
        uint32_t w = (uint32_t)buf[3*i] | ((uint32_t)buf[3*i+1] << 8) | ((uint32_t)buf[3*i+2] << 16);

        for (j = 0; j < 8; j++) {
            uint16_t v = (w >> (3*j)) & 0x07;
            uint16_t lt3 = (uint16_t)(v - 1) >> 15;
            uint16_t lt6 = (uint16_t)(v - 2) >> 15;
            r->coeffs[8*i + j] = (int16_t)((2*lt3 - 1) * lt6);
        }
    } 
}
static void ternary_375(poly *r, const uint8_t *buf) // 3/8
{
    unsigned int i, j;

    for (i = 0; i < N/8; i++) {
        uint32_t w = (uint32_t)buf[3*i] | ((uint32_t)buf[3*i+1] << 8) | ((uint32_t)buf[3*i+2] << 16);
        
        for (j = 0; j < 8; j++) {
            uint16_t v = (w >> (3*j)) & 0x07;
            uint16_t lt3 = (uint16_t)(v - 3) >> 15;
            uint16_t lt6 = (uint16_t)(v - 6) >> 15;
            r->coeffs[8*i + j] = (int16_t)((2*lt3 - 1) * lt6);
        }
    }
}

/*************************************************
* Name:        poly_ternary_p
*
* Description: Dispatch ternary polynomial sampling based on probability.
*              Wrappers pass compile-time constants (PFR/PGE), so the
*              switch is constant-folded by the optimizer.
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*              - uint8_t nonce:       domain-separation nonce
*              - int prob:            one of {125, 3125, 25, 15625, 4375}
**************************************************/
static void poly_ternary_p(poly *r, const uint8_t seed[SEEDBYTES],
                            uint8_t nonce, int prob)
{
    switch (prob) {
    case 125: {   // 1/8
        uint8_t buf[N * 3/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_125(r, buf);
        break;
    }
    case 3125: {  // 5/16
        uint8_t buf[N * 4/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_3125(r, buf);
        break;
    }
    case 25: {    // 1/4
        uint8_t buf[N * 2/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_25(r, buf);
        break;
    }
    case 15625: { // 5/32
        uint8_t buf[N * 5/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_15625(r, buf);
        break;
    }
    case 4375: {  // 7/16
        uint8_t buf[N * 4/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_4375(r, buf);
        break;
    }
    }
}

void poly_f_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PF);
}
void poly_g_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PG);
}
void poly_r_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PR);
}
void poly_e_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PE);
}