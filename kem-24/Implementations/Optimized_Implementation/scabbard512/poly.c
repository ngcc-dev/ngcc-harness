#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "poly.h"
#include "minal.h"


void poly_modq_frombytes(
    poly *r, 
    const uint8_t buf[SCABBARD_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 8; i++) {
        r->coeffs[8 * i + 0] = ((buf[13 * i + 0] >> 0) | ((uint16_t)buf[13 * i + 1] << 8)) & 0x1FFF;
        r->coeffs[8 * i + 1] = ((buf[13 * i + 1] >> 5) | ((uint16_t)buf[13 * i + 2] << 3) | ((uint16_t)buf[13 * i + 3] << 11)) & 0x1FFF;
        r->coeffs[8 * i + 2] = ((buf[13 * i + 3] >> 2) | ((uint16_t)buf[13 * i + 4] << 6)) & 0x1FFF;
        r->coeffs[8 * i + 3] = ((buf[13 * i + 4] >> 7) | ((uint16_t)buf[13 * i + 5] << 1) | ((uint16_t)buf[13 * i + 6] << 9)) & 0x1FFF;
        r->coeffs[8 * i + 4] = ((buf[13 * i + 6] >> 4) | ((uint16_t)buf[13 * i + 7] << 4) | ((uint16_t)buf[13 * i + 8] << 12)) & 0x1FFF;
        r->coeffs[8 * i + 5] = ((buf[13 * i + 8] >> 1) | ((uint16_t)buf[13 * i + 9] << 7)) & 0x1FFF;
        r->coeffs[8 * i + 6] = ((buf[13 * i + 9] >> 6) | ((uint16_t)buf[13 * i + 10] << 2) | ((uint16_t)buf[13 * i + 11] << 10)) & 0x1FFF;
        r->coeffs[8 * i + 7] = ((buf[13 * i + 11] >> 3) | ((uint16_t)buf[13 * i + 12] << 5)) & 0x1FFF;
    }
}

void poly_modp_tobytes(
    uint8_t r[SCABBARD_P_POLYBYTES], 
    const poly *a)
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 8; i++) {
        r[11 * i + 0] = ( a->coeffs[ 8 * i + 0 ] & (0xff));
        r[11 * i + 1] = ( (a->coeffs[ 8 * i + 0 ] >>8) & 0x07 ) | ((a->coeffs[ 8 * i + 1 ] & 0x1f) << 3);
        r[11 * i + 2] = ( (a->coeffs[ 8 * i + 1 ] >>5) & 0x3f ) | ((a->coeffs[ 8 * i + 2 ] & 0x03) << 6);
        r[11 * i + 3] = ( (a->coeffs[ 8 * i + 2 ] >>2) & 0xff );
        r[11 * i + 4] = ( (a->coeffs[ 8 * i + 2 ] >>10) & 0x01 ) | ((a->coeffs[ 8 * i + 3 ] & 0x7f) << 1);
        r[11 * i + 5] = ( (a->coeffs[ 8 * i + 3 ] >>7) & 0x0f ) | ((a->coeffs[ 8 * i + 4 ] & 0x0f) << 4);
        r[11 * i + 6] = ( (a->coeffs[ 8 * i + 4 ] >>4) & 0x7f ) | ((a->coeffs[ 8 * i + 5 ] & 0x01) << 7);
        r[11 * i + 7] = ( (a->coeffs[ 8 * i + 5 ] >>1) & 0xff );
        r[11 * i + 8] = ( (a->coeffs[ 8 * i + 5 ] >>9) & 0x03 ) | ((a->coeffs[ 8 * i + 6 ] & 0x3f) << 2);
        r[11 * i + 9] = ( (a->coeffs[ 8 * i + 6 ] >>6) & 0x1f ) | ((a->coeffs[ 8 * i + 7 ] & 0x07) << 5);
        r[11 * i + 10] = ( (a->coeffs[ 8 * i + 7 ] >>3) & 0xff );
    }
}


void poly_modp_frombytes(
    poly *r, 
    const uint8_t buf[SCABBARD_P_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 8; i++) {
        r->coeffs[8 * i + 0]= buf[ 11 * i + 0 ] |  ((buf[ 11 * i + 1 ] & 0x07)<<8);
        r->coeffs[8 * i + 1]= ( (buf[ 11 * i + 1 ]>>3) & (0x1f)) |  ((buf[ 11 * i + 2 ] & 0x3f)<<5);
        r->coeffs[8 * i + 2]= ( (buf[ 11 * i + 2 ]>>6) & (0x03)) |  ((buf[ 11 * i + 3 ] & 0xff)<<2) |  ((buf[ 11 * i + 4 ] & 0x01)<<10);
        r->coeffs[8 * i + 3]= ( (buf[ 11 * i + 4 ]>>1) & (0x7f)) |  ((buf[ 11 * i + 5 ] & 0x0f)<<7);
        r->coeffs[8 * i + 4]= ( (buf[ 11 * i + 5 ]>>4) & (0x0f)) |  ((buf[ 11 * i + 6 ] & 0x7f)<<4);
        r->coeffs[8 * i + 5]= ( (buf[ 11 * i + 6 ]>>7) & (0x01)) |  ((buf[ 11 * i + 7 ] & 0xff)<<1) |  ((buf[ 11 * i + 8 ] & 0x03)<<9);
        r->coeffs[8 * i + 6]= ( (buf[ 11 * i + 8 ]>>2) & (0x3f)) |  ((buf[ 11 * i + 9 ] & 0x1f)<<6);
        r->coeffs[8 * i + 7]= ( (buf[ 11 * i + 9 ]>>5) & (0x07)) |  ((buf[ 11 * i + 10 ] & 0xff)<<3);
    }
}


void poly_s_tobytes(
    uint8_t r[SCABBARD_S_POLYBYTES],
    const poly *s)
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 2; i++) {
        r[i] = (s->coeffs[2 * i] & 0x0F) | ((s->coeffs[2 * i + 1] & 0x0F) << 4);
    }
}

void poly_s_frombytes(
    poly *r,
    const uint8_t s[SCABBARD_S_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 2; i++) {
        r->coeffs[2 * i + 0] = (((s[i] & 0x0F) ^ 0x08) - 0x08) & SCABBARD_P;
        r->coeffs[2 * i + 1] = (((s[i] >> 4) ^ 0x08) - 0x08) & SCABBARD_P;
    }
}

void poly_m_tobytes(
    uint8_t r[SCABBARD_POLYCOMPRESSEDBYTES],
    const poly *m)
{
    size_t i;

    for (i = 0; i < SCABBARD_N; i++) {
        r[i] = m->coeffs[i] & 0xFF; 
    }
}

void poly_m_frombytes(
    poly *r,
    const uint8_t m[SCABBARD_POLYCOMPRESSEDBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N; i++) {
        r->coeffs[i] = m[i] & 0xFF;
    }
}

/* Minal Encode/Decode */

void poly_tomsg(
    uint8_t msg[SCABBARD_INDCPA_MSGBYTES],
    const poly *v)
{
    unsigned int i,j;
    int16_t codeword[2];
    uint32_t base = 0;

    for(i=0;i<SCABBARD_N/4;i++) {
        msg[i] = 0;
        for(j=0;j<8;j+=4) {
        codeword[0] = (int16_t) v->coeffs[base];
        codeword[1] = (int16_t) v->coeffs[base + SCABBARD_N/2];

        msg[i] |= (minal_b2_code_decode(codeword) << j);

        base++;
        }
    }
}

void poly_frommsg(
    poly *r,
    const uint8_t msg[SCABBARD_INDCPA_MSGBYTES])
{
  unsigned int i,j;
  int16_t codeword[2];
  uint8_t msg_bits[4]; // 4--> 2 bits for internal code, 2 bits for external code

  size_t base = 0;
  for(i=0;i<SCABBARD_N/4;i++) {
    for(j=0;j<8;j+=4) {
      msg_bits[0] = (msg[i] >> (j + 3)) & 1;
      msg_bits[1] = (msg[i] >> (j + 2)) & 1;
      msg_bits[2] = (msg[i] >> (j + 1)) & 1;
      msg_bits[3] = (msg[i] >> (j + 0)) & 1;

      minal_b2_code_encode(codeword, msg_bits);

      r->coeffs[base] = codeword[0];
      r->coeffs[base + SCABBARD_N/2] = codeword[1];

      base++;
    }
  }
}

/* Polynomial Multiplication */

void poly_TC_evaluate(
    uint16_t b_weighted[3][3][N_SM_16],
    const poly *b)
{
    ALIGN16 const uint16_t *B0 = b->coeffs;
    ALIGN16 const uint16_t *B1 = b->coeffs + (2 * N_SM_16); // Since N_SM = 2 * N_SM_16

        __m256i v_bw2_low  = _mm256_loadu_si256((const __m256i*)&B0[0]);
        __m256i v_bw2_high = _mm256_loadu_si256((const __m256i*)&B0[N_SM_16]);

        __m256i v_bw0_low  = _mm256_loadu_si256((const __m256i*)&B1[0]);
        __m256i v_bw0_high = _mm256_loadu_si256((const __m256i*)&B1[N_SM_16]);

        __m256i v_bw1_low  = _mm256_add_epi16(v_bw2_low, v_bw0_low);
        __m256i v_bw1_high = _mm256_add_epi16(v_bw2_high, v_bw0_high);

        __m256i v_bw0_mid  = _mm256_add_epi16(v_bw0_low, v_bw0_high);
        __m256i v_bw1_mid  = _mm256_add_epi16(v_bw1_low, v_bw1_high);
        __m256i v_bw2_mid  = _mm256_add_epi16(v_bw2_low, v_bw2_high);

        _mm256_storeu_si256((__m256i*)&b_weighted[0][2][0], v_bw0_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][0][0], v_bw0_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][1][0], v_bw0_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[1][2][0], v_bw1_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][0][0], v_bw1_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][1][0], v_bw1_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[2][2][0], v_bw2_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][0][0], v_bw2_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][1][0], v_bw2_mid);


        
        v_bw2_low  = _mm256_loadu_si256((const __m256i*)&B0[16]);
        v_bw2_high = _mm256_loadu_si256((const __m256i*)&B0[16+N_SM_16]);

        v_bw0_low  = _mm256_loadu_si256((const __m256i*)&B1[16]);
        v_bw0_high = _mm256_loadu_si256((const __m256i*)&B1[16+N_SM_16]);

        v_bw1_low  = _mm256_add_epi16(v_bw2_low, v_bw0_low);
        v_bw1_high = _mm256_add_epi16(v_bw2_high, v_bw0_high);

        v_bw0_mid  = _mm256_add_epi16(v_bw0_low, v_bw0_high);
        v_bw1_mid  = _mm256_add_epi16(v_bw1_low, v_bw1_high);
        v_bw2_mid  = _mm256_add_epi16(v_bw2_low, v_bw2_high);

        _mm256_storeu_si256((__m256i*)&b_weighted[0][2][16], v_bw0_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][0][16], v_bw0_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][1][16], v_bw0_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[1][2][16], v_bw1_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][0][16], v_bw1_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][1][16], v_bw1_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[2][2][16], v_bw2_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][0][16], v_bw2_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][1][16], v_bw2_mid);



        v_bw2_low  = _mm256_loadu_si256((const __m256i*)&B0[32]);
        v_bw2_high = _mm256_loadu_si256((const __m256i*)&B0[32+N_SM_16]);

        v_bw0_low  = _mm256_loadu_si256((const __m256i*)&B1[32]);
        v_bw0_high = _mm256_loadu_si256((const __m256i*)&B1[32+N_SM_16]);

        v_bw1_low  = _mm256_add_epi16(v_bw2_low, v_bw0_low);
        v_bw1_high = _mm256_add_epi16(v_bw2_high, v_bw0_high);

        v_bw0_mid  = _mm256_add_epi16(v_bw0_low, v_bw0_high);
        v_bw1_mid  = _mm256_add_epi16(v_bw1_low, v_bw1_high);
        v_bw2_mid  = _mm256_add_epi16(v_bw2_low, v_bw2_high);

        _mm256_storeu_si256((__m256i*)&b_weighted[0][2][32], v_bw0_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][0][32], v_bw0_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][1][32], v_bw0_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[1][2][32], v_bw1_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][0][32], v_bw1_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][1][32], v_bw1_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[2][2][32], v_bw2_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][0][32], v_bw2_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][1][32], v_bw2_mid);



        v_bw2_low  = _mm256_loadu_si256((const __m256i*)&B0[48]);
        v_bw2_high = _mm256_loadu_si256((const __m256i*)&B0[48+N_SM_16]);

        v_bw0_low  = _mm256_loadu_si256((const __m256i*)&B1[48]);
        v_bw0_high = _mm256_loadu_si256((const __m256i*)&B1[48+N_SM_16]);

        v_bw1_low  = _mm256_add_epi16(v_bw2_low, v_bw0_low);
        v_bw1_high = _mm256_add_epi16(v_bw2_high, v_bw0_high);

        v_bw0_mid  = _mm256_add_epi16(v_bw0_low, v_bw0_high);
        v_bw1_mid  = _mm256_add_epi16(v_bw1_low, v_bw1_high);
        v_bw2_mid  = _mm256_add_epi16(v_bw2_low, v_bw2_high);

        _mm256_storeu_si256((__m256i*)&b_weighted[0][2][48], v_bw0_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][0][48], v_bw0_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[0][1][48], v_bw0_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[1][2][48], v_bw1_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][0][48], v_bw1_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[1][1][48], v_bw1_mid);

        _mm256_storeu_si256((__m256i*)&b_weighted[2][2][48], v_bw2_low);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][0][48], v_bw2_high);
        _mm256_storeu_si256((__m256i*)&b_weighted[2][1][48], v_bw2_mid);

        
}

void poly_TC_pointwise_acc(
    uint16_t acc[3][3][N_SM_16_RES],
    const uint16_t a_weighted[3][3][N_SM_16],
    const uint16_t b_weighted[3][3][N_SM_16])
{
    size_t i, j;

    for (i = 0; i < N_SM_16; i++) {
        for(j=0; j<N_SM_16; j+=16){
        __m256i a_00 = _mm256_set1_epi16(a_weighted[0][0][i]);
        __m256i a_01 = _mm256_set1_epi16(a_weighted[0][1][i]);
        __m256i a_02 = _mm256_set1_epi16(a_weighted[0][2][i]);

        __m256i a_10 = _mm256_set1_epi16(a_weighted[1][0][i]);
        __m256i a_11 = _mm256_set1_epi16(a_weighted[1][1][i]);
        __m256i a_12 = _mm256_set1_epi16(a_weighted[1][2][i]);

        __m256i a_20 = _mm256_set1_epi16(a_weighted[2][0][i]);
        __m256i a_21 = _mm256_set1_epi16(a_weighted[2][1][i]);
        __m256i a_22 = _mm256_set1_epi16(a_weighted[2][2][i]);

        __m256i b_00   = _mm256_loadu_si256((const __m256i*)&b_weighted[0][0][j]);
        __m256i acc_00 = _mm256_loadu_si256((const __m256i*)&acc[0][0][i+j]);
        __m256i prod_00 = _mm256_mullo_epi16(a_00, b_00);
        acc_00 = _mm256_add_epi16(acc_00, prod_00);
        _mm256_storeu_si256((__m256i*)&acc[0][0][i+j], acc_00);

        __m256i b_01   = _mm256_loadu_si256((const __m256i*)&b_weighted[0][1][j]);
        __m256i acc_01 = _mm256_loadu_si256((const __m256i*)&acc[0][1][i+j]);
        __m256i prod_01 = _mm256_mullo_epi16(a_01, b_01);
        acc_01 = _mm256_add_epi16(acc_01, prod_01);
        _mm256_storeu_si256((__m256i*)&acc[0][1][i+j], acc_01);

        __m256i b_02   = _mm256_loadu_si256((const __m256i*)&b_weighted[0][2][j]);
        __m256i acc_02 = _mm256_loadu_si256((const __m256i*)&acc[0][2][i+j]);
        __m256i prod_02 = _mm256_mullo_epi16(a_02, b_02);
        acc_02 = _mm256_add_epi16(acc_02, prod_02);
        _mm256_storeu_si256((__m256i*)&acc[0][2][i+j], acc_02);

        __m256i b_10   = _mm256_loadu_si256((const __m256i*)&b_weighted[1][0][j]);
        __m256i acc_10 = _mm256_loadu_si256((const __m256i*)&acc[1][0][i+j]);
__m256i prod_10 = _mm256_mullo_epi16(a_10, b_10);
        acc_10 = _mm256_add_epi16(acc_10, prod_10);
        _mm256_storeu_si256((__m256i*)&acc[1][0][i+j], acc_10);

        __m256i b_11   = _mm256_loadu_si256((const __m256i*)&b_weighted[1][1][j]);
        __m256i acc_11 = _mm256_loadu_si256((const __m256i*)&acc[1][1][i+j]);
        __m256i prod_11 = _mm256_mullo_epi16(a_11, b_11);
        acc_11 = _mm256_add_epi16(acc_11, prod_11);
        _mm256_storeu_si256((__m256i*)&acc[1][1][i+j], acc_11);

        __m256i b_12   = _mm256_loadu_si256((const __m256i*)&b_weighted[1][2][j]);
        __m256i acc_12 = _mm256_loadu_si256((const __m256i*)&acc[1][2][i+j]);
        __m256i prod_12 = _mm256_mullo_epi16(a_12, b_12);
        acc_12 = _mm256_add_epi16(acc_12, prod_12);
        _mm256_storeu_si256((__m256i*)&acc[1][2][i+j], acc_12);

        __m256i b_20   = _mm256_loadu_si256((const __m256i*)&b_weighted[2][0][j]);
        __m256i acc_20 = _mm256_loadu_si256((const __m256i*)&acc[2][0][i+j]);
        __m256i prod_20 = _mm256_mullo_epi16(a_20, b_20);
        acc_20 = _mm256_add_epi16(acc_20, prod_20);
        _mm256_storeu_si256((__m256i*)&acc[2][0][i+j], acc_20);

        __m256i b_21   = _mm256_loadu_si256((const __m256i*)&b_weighted[2][1][j]);
        __m256i acc_21 = _mm256_loadu_si256((const __m256i*)&acc[2][1][i+j]);
        __m256i prod_21 = _mm256_mullo_epi16(a_21, b_21);
        acc_21 = _mm256_add_epi16(acc_21, prod_21);
        _mm256_storeu_si256((__m256i*)&acc[2][1][i+j], acc_21);

        __m256i b_22   = _mm256_loadu_si256((const __m256i*)&b_weighted[2][2][j]);
        __m256i acc_22 = _mm256_loadu_si256((const __m256i*)&acc[2][2][i+j]);
        __m256i prod_22 = _mm256_mullo_epi16(a_22, b_22);
        acc_22 = _mm256_add_epi16(acc_22, prod_22);
        _mm256_storeu_si256((__m256i*)&acc[2][2][i+j], acc_22);
        }
    }
}

void poly_TC_interpolate(
    poly *r,
    const uint16_t acc[3][3][N_SM_16_RES])
{
    size_t i;

    uint16_t w0[N_SM_RES] = {0};
    uint16_t w1[N_SM_RES] = {0};
    uint16_t w2[N_SM_RES] = {0};
    uint16_t result_interp[2 * SCABBARD_N] = {0};

    __m256i acc00, acc01, acc02, acc10, acc11, acc12, acc20, acc21, acc22, r0, r1, r2;
    __m256i old_w_i, old_w_i64, old_w_i128;
    for (i = 0; i < N_SM_16_RES; i += 16) {
        acc00 = _mm256_loadu_si256((const __m256i*)&acc[0][0][i]);
        acc01 = _mm256_loadu_si256((const __m256i*)&acc[0][1][i]);
        acc02 = _mm256_loadu_si256((const __m256i*)&acc[0][2][i]);

        acc10 = _mm256_loadu_si256((const __m256i*)&acc[1][0][i]);
        acc11 = _mm256_loadu_si256((const __m256i*)&acc[1][1][i]);
        acc12 = _mm256_loadu_si256((const __m256i*)&acc[1][2][i]);

        acc20 = _mm256_loadu_si256((const __m256i*)&acc[2][0][i]);
        acc21 = _mm256_loadu_si256((const __m256i*)&acc[2][1][i]);
        acc22 = _mm256_loadu_si256((const __m256i*)&acc[2][2][i]);

        r0 = _mm256_sub_epi16(_mm256_sub_epi16(acc01, acc00), acc02);
        r1 = _mm256_sub_epi16(_mm256_sub_epi16(acc11, acc10), acc12);
        r2 = _mm256_sub_epi16(_mm256_sub_epi16(acc21, acc20), acc22);

        old_w_i = _mm256_loadu_si256((const __m256i*)&w0[i]);
        _mm256_storeu_si256((__m256i*)&w0[i], _mm256_add_epi16(old_w_i, acc02));
       
        old_w_i64 = _mm256_loadu_si256((const __m256i*)&w0[i + 64]);
        _mm256_storeu_si256((__m256i*)&w0[i + 64], _mm256_add_epi16(old_w_i64, r0));
        
        old_w_i128 = _mm256_loadu_si256((const __m256i*)&w0[i + 128]);
        _mm256_storeu_si256((__m256i*)&w0[i + 128], _mm256_add_epi16(old_w_i128, acc00));

        old_w_i = _mm256_loadu_si256((const __m256i*)&w1[i]);
        _mm256_storeu_si256((__m256i*)&w1[i], _mm256_add_epi16(old_w_i, acc12));
        
        old_w_i64 = _mm256_loadu_si256((const __m256i*)&w1[i + 64]);
        _mm256_storeu_si256((__m256i*)&w1[i + 64], _mm256_add_epi16(old_w_i64, r1));
        
        old_w_i128 = _mm256_loadu_si256((const __m256i*)&w1[i + 128]);
        _mm256_storeu_si256((__m256i*)&w1[i + 128], _mm256_add_epi16(old_w_i128, acc10));

        old_w_i = _mm256_loadu_si256((const __m256i*)&w2[i]);
        _mm256_storeu_si256((__m256i*)&w2[i], _mm256_add_epi16(old_w_i, acc22));
        
        old_w_i64 = _mm256_loadu_si256((const __m256i*)&w2[i + 64]);
        _mm256_storeu_si256((__m256i*)&w2[i + 64], _mm256_add_epi16(old_w_i64, r2));
        
        old_w_i128 = _mm256_loadu_si256((const __m256i*)&w2[i + 128]);
        _mm256_storeu_si256((__m256i*)&w2[i + 128], _mm256_add_epi16(old_w_i128, acc20));
    }
    __m256i vr0, vr1, vr2, sum_r0_r2;
    __m256i old_res_i;
    for (i = 0; i < N_SM_RES; i += 16) {
        vr0 = _mm256_loadu_si256((const __m256i*)&w0[i]);
        vr1 = _mm256_loadu_si256((const __m256i*)&w1[i]);
        vr2 = _mm256_loadu_si256((const __m256i*)&w2[i]);

        sum_r0_r2 = _mm256_add_epi16(vr0, vr2);
        vr1 = _mm256_sub_epi16(vr1, sum_r0_r2);

        old_res_i = _mm256_loadu_si256((const __m256i*)&result_interp[i]);
        _mm256_storeu_si256((__m256i*)&result_interp[i], _mm256_add_epi16(old_res_i, vr2));

        old_res_i = _mm256_loadu_si256((const __m256i*)&result_interp[i + 128]);
        _mm256_storeu_si256((__m256i*)&result_interp[i + 128], _mm256_add_epi16(old_res_i, vr1));

        old_res_i = _mm256_loadu_si256((const __m256i*)&result_interp[i + 256]);
        _mm256_storeu_si256((__m256i*)&result_interp[i + 256], _mm256_add_epi16(old_res_i, vr0));
    }

    for (i = SCABBARD_N; i < 2 * SCABBARD_N; i += 16) {
        __m256i res_low  = _mm256_loadu_si256((const __m256i*)&result_interp[i - SCABBARD_N]);
        __m256i res_high = _mm256_loadu_si256((const __m256i*)&result_interp[i]);

       __m256i diff = _mm256_sub_epi16(res_low, res_high);
        _mm256_storeu_si256((__m256i*)&r->coeffs[i - SCABBARD_N], diff);
    }
}
