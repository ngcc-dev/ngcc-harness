#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "minal.h"
#include "poly.h"


void poly_modq_frombytes(
    poly *r, 
    const uint8_t buf[SCABBARD_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        r->coeffs[4 * i + 0] = ((buf[7 * i + 0] & 0xff) | ((buf[7 * i + 1] & 0x3f) << 8)) & 0x3fff;
		r->coeffs[4 * i + 1] = (((buf[7 * i + 1] >> 6) & 0x03) | (((buf[7 * i + 2] & 0xff) << 2) & 0xff) | (((buf[7 * i + 3] & 0xf) << 10) & 0x0f)) & 0x3fff;
        r->coeffs[4 * i + 2] = (((buf[7 * i + 3] >> 4) & 0x0F) | (((buf[7 * i + 4] & 0xff) << 4) & 0xff) | (((buf[7 * i + 5] & 0x3) << 12) & 0x03)) & 0x3fff;
        r->coeffs[4 * i + 3] = (((buf[7 * i + 5] >> 2) & 0x3F) | (((buf[7 * i + 6] & 0xff) << 6) & 0xff)) & 0x3fff;
    }
}

void poly_modp_tobytes(
    uint8_t r[SCABBARD_P_POLYBYTES], 
    const poly *a)
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        r[5 * i + 0] = (a->coeffs[4 * i + 0] & (0xff));
		r[5 * i + 1] = ((a->coeffs[4 * i + 0] >> 8) & 0x03) | ((a->coeffs[4 * i + 1] & 0x3f) << 2);
		r[5 * i + 2] = ((a->coeffs[4 * i + 1] >> 6) & 0x0f) | ((a->coeffs[4 * i + 2] & 0x0f) << 4);
		r[5 * i + 3] = ((a->coeffs[4 * i + 2] >> 4) & 0x3f) | ((a->coeffs[4 * i + 3] & 0x03) << 6);
		r[5 * i + 4] = ((a->coeffs[4 * i + 3] >> 2) & 0xff);
    }
}


void poly_modp_frombytes(
    poly *r, 
    const uint8_t buf[SCABBARD_P_POLYBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 4; i++) {
        r->coeffs[4 * i + 0] = (buf[5 * i + 0] & (0xff)) | ((uint16_t)buf[5 * i + 1] & 0x03) << 8;
        r->coeffs[4 * i + 1] = ((buf[5 * i + 1] >> 2) & (0x3f)) | (((uint16_t)buf[5 * i + 2] & 0x0f) << 6);
        r->coeffs[4 * i + 2] = ((buf[5 * i + 2] >> 4) & (0x0f)) | (((uint16_t)buf[5 * i + 3] & 0x3f) << 4);
        r->coeffs[4 * i + 3] = ((buf[5 * i + 3] >> 6) & (0x03)) | (((uint16_t)buf[5 * i + 4] & 0xff) << 2);
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

    for (i = 0; i < SCABBARD_N / 8; i++) {
        r[5 * i + 0]= (m->coeffs[8 * i + 0] & 0x1f) | ( (m->coeffs[8 * i + 1] & 0x07)<<5 );
		r[5 * i + 1]= ((m->coeffs[8 * i + 1] >> 3 ) & 0x03)  | ( (m->coeffs[8 * i + 2] & 0x1f)<<2 ) | ( (m->coeffs[8 * i + 3] & 0x01)<<7 );
		r[5 * i + 2]= ((m->coeffs[8 * i + 3] >> 1 ) & 0x0f)  | ( (m->coeffs[8 * i + 4] & 0x0f)<<4 );
		r[5 * i + 3]= ((m->coeffs[8 * i + 4] >> 4 ) & 0x01)  | ( (m->coeffs[8 * i + 5] & 0x1f)<<1 ) | ( (m->coeffs[8 * i + 6] & 0x03)<<6 );
		r[5 * i + 4]= ((m->coeffs[8 * i + 6] >> 2 ) & 0x07)  | ( (m->coeffs[8 * i + 7] & 0x1f)<<3 );
    }
}

void poly_m_frombytes(
    poly *r,
    const uint8_t m[SCABBARD_POLYCOMPRESSEDBYTES])
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 8; i++) {
        r->coeffs[8 * i + 0] = (m[5 * i + 0])&0x1f;
		r->coeffs[8 * i + 1] = ( ( (m[5 * i + 0])>>5 )&0x07) | ( ( (m[5 * i + 1])&0x03)<<3 );
		r->coeffs[8 * i + 2] = ( ( (m[5 * i + 1])>>2 )&0x1f);
		r->coeffs[8 * i + 3] = ( ( (m[5 * i + 1])>>7 )&0x01) | ( ( (m[5 * i + 2])&0x0f)<<1 );
		r->coeffs[8 * i + 4] = ( ( (m[5 * i + 2])>>4 )&0x0f) | ( ( (m[5 * i + 3])&0x01)<<4 );
		r->coeffs[8 * i + 5] = ( ( (m[5 * i + 3])>>1 )&0x1f);
		r->coeffs[8 * i + 6] = ( ( (m[5 * i + 3])>>6 )&0x03) | ( ( (m[5 * i + 4])&0x07)<<2 );
		r->coeffs[8 * i + 7] = ( (m[5 * i + 4]>>3)&0x1f );

    }
}

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
}

void poly_TC_pointwise_acc(
    uint16_t acc[3][3][N_SM_16_RES],
    const uint16_t a_weighted[3][3][N_SM_16],
    const uint16_t b_weighted[3][3][N_SM_16])
{
    size_t i;

    for (i = 0; i < N_SM_16; i++) {
        __m256i a_00 = _mm256_set1_epi16(a_weighted[0][0][i]);
        __m256i a_01 = _mm256_set1_epi16(a_weighted[0][1][i]);
        __m256i a_02 = _mm256_set1_epi16(a_weighted[0][2][i]);

        __m256i a_10 = _mm256_set1_epi16(a_weighted[1][0][i]);
        __m256i a_11 = _mm256_set1_epi16(a_weighted[1][1][i]);
        __m256i a_12 = _mm256_set1_epi16(a_weighted[1][2][i]);

        __m256i a_20 = _mm256_set1_epi16(a_weighted[2][0][i]);
        __m256i a_21 = _mm256_set1_epi16(a_weighted[2][1][i]);
        __m256i a_22 = _mm256_set1_epi16(a_weighted[2][2][i]);

            __m256i b_00   = _mm256_loadu_si256((const __m256i*)&b_weighted[0][0][0]);
            __m256i acc_00 = _mm256_loadu_si256((const __m256i*)&acc[0][0][i]);
            __m256i prod_00 = _mm256_mullo_epi16(a_00, b_00);
            acc_00 = _mm256_add_epi16(acc_00, prod_00);
            _mm256_storeu_si256((__m256i*)&acc[0][0][i], acc_00);

            __m256i b_01   = _mm256_loadu_si256((const __m256i*)&b_weighted[0][1][0]);
            __m256i acc_01 = _mm256_loadu_si256((const __m256i*)&acc[0][1][i]);
            __m256i prod_01 = _mm256_mullo_epi16(a_01, b_01);
            acc_01 = _mm256_add_epi16(acc_01, prod_01);
            _mm256_storeu_si256((__m256i*)&acc[0][1][i], acc_01);

            __m256i b_02   = _mm256_loadu_si256((const __m256i*)&b_weighted[0][2][0]);
            __m256i acc_02 = _mm256_loadu_si256((const __m256i*)&acc[0][2][i]);
            __m256i prod_02 = _mm256_mullo_epi16(a_02, b_02);
            acc_02 = _mm256_add_epi16(acc_02, prod_02);
            _mm256_storeu_si256((__m256i*)&acc[0][2][i], acc_02);

            __m256i b_10   = _mm256_loadu_si256((const __m256i*)&b_weighted[1][0][0]);
            __m256i acc_10 = _mm256_loadu_si256((const __m256i*)&acc[1][0][i]);
            __m256i prod_10 = _mm256_mullo_epi16(a_10, b_10);
            acc_10 = _mm256_add_epi16(acc_10, prod_10);
            _mm256_storeu_si256((__m256i*)&acc[1][0][i], acc_10);

            __m256i b_11   = _mm256_loadu_si256((const __m256i*)&b_weighted[1][1][0]);
            __m256i acc_11 = _mm256_loadu_si256((const __m256i*)&acc[1][1][i]);
            __m256i prod_11 = _mm256_mullo_epi16(a_11, b_11);
            acc_11 = _mm256_add_epi16(acc_11, prod_11);
            _mm256_storeu_si256((__m256i*)&acc[1][1][i], acc_11);

            __m256i b_12   = _mm256_loadu_si256((const __m256i*)&b_weighted[1][2][0]);
            __m256i acc_12 = _mm256_loadu_si256((const __m256i*)&acc[1][2][i]);
            __m256i prod_12 = _mm256_mullo_epi16(a_12, b_12);
            acc_12 = _mm256_add_epi16(acc_12, prod_12);
            _mm256_storeu_si256((__m256i*)&acc[1][2][i], acc_12);

            __m256i b_20   = _mm256_loadu_si256((const __m256i*)&b_weighted[2][0][0]);
            __m256i acc_20 = _mm256_loadu_si256((const __m256i*)&acc[2][0][i]);
            __m256i prod_20 = _mm256_mullo_epi16(a_20, b_20);
            acc_20 = _mm256_add_epi16(acc_20, prod_20);
            _mm256_storeu_si256((__m256i*)&acc[2][0][i], acc_20);

            __m256i b_21   = _mm256_loadu_si256((const __m256i*)&b_weighted[2][1][0]);
            __m256i acc_21 = _mm256_loadu_si256((const __m256i*)&acc[2][1][i]);
            __m256i prod_21 = _mm256_mullo_epi16(a_21, b_21);
            acc_21 = _mm256_add_epi16(acc_21, prod_21);
            _mm256_storeu_si256((__m256i*)&acc[2][1][i], acc_21);

            __m256i b_22   = _mm256_loadu_si256((const __m256i*)&b_weighted[2][2][0]);
            __m256i acc_22 = _mm256_loadu_si256((const __m256i*)&acc[2][2][i]);
            __m256i prod_22 = _mm256_mullo_epi16(a_22, b_22);
            acc_22 = _mm256_add_epi16(acc_22, prod_22);
            _mm256_storeu_si256((__m256i*)&acc[2][2][i], acc_22);
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

    for (i = 0; i < N_SM_16_RES; i += 16) {
        __m256i acc00 = _mm256_loadu_si256((const __m256i*)&acc[0][0][i]);
        __m256i acc01 = _mm256_loadu_si256((const __m256i*)&acc[0][1][i]);
        __m256i acc02 = _mm256_loadu_si256((const __m256i*)&acc[0][2][i]);

        __m256i acc10 = _mm256_loadu_si256((const __m256i*)&acc[1][0][i]);
        __m256i acc11 = _mm256_loadu_si256((const __m256i*)&acc[1][1][i]);
        __m256i acc12 = _mm256_loadu_si256((const __m256i*)&acc[1][2][i]);

        __m256i acc20 = _mm256_loadu_si256((const __m256i*)&acc[2][0][i]);
        __m256i acc21 = _mm256_loadu_si256((const __m256i*)&acc[2][1][i]);
        __m256i acc22 = _mm256_loadu_si256((const __m256i*)&acc[2][2][i]);

        __m256i r0 = _mm256_sub_epi16(_mm256_sub_epi16(acc01, acc00), acc02);
        __m256i r1 = _mm256_sub_epi16(_mm256_sub_epi16(acc11, acc10), acc12);
        __m256i r2 = _mm256_sub_epi16(_mm256_sub_epi16(acc21, acc20), acc22);

        __m256i old_w0_i = _mm256_loadu_si256((const __m256i*)&w0[i]);
        _mm256_storeu_si256((__m256i*)&w0[i], _mm256_add_epi16(old_w0_i, acc02));
       
        __m256i old_w0_i16 = _mm256_loadu_si256((const __m256i*)&w0[i + 16]);
        _mm256_storeu_si256((__m256i*)&w0[i + 16], _mm256_add_epi16(old_w0_i16, r0));
        
        __m256i old_w0_i32 = _mm256_loadu_si256((const __m256i*)&w0[i + 32]);
        _mm256_storeu_si256((__m256i*)&w0[i + 32], _mm256_add_epi16(old_w0_i32, acc00));

        __m256i old_w1_i = _mm256_loadu_si256((const __m256i*)&w1[i]);
        _mm256_storeu_si256((__m256i*)&w1[i], _mm256_add_epi16(old_w1_i, acc12));
        // offset i + 16
        __m256i old_w1_i16 = _mm256_loadu_si256((const __m256i*)&w1[i + 16]);
        _mm256_storeu_si256((__m256i*)&w1[i + 16], _mm256_add_epi16(old_w1_i16, r1));
        // offset i + 32
        __m256i old_w1_i32 = _mm256_loadu_si256((const __m256i*)&w1[i + 32]);
        _mm256_storeu_si256((__m256i*)&w1[i + 32], _mm256_add_epi16(old_w1_i32, acc10));

        __m256i old_w2_i = _mm256_loadu_si256((const __m256i*)&w2[i]);
        _mm256_storeu_si256((__m256i*)&w2[i], _mm256_add_epi16(old_w2_i, acc22));
        // offset i + 16
        __m256i old_w2_i16 = _mm256_loadu_si256((const __m256i*)&w2[i + 16]);
        _mm256_storeu_si256((__m256i*)&w2[i + 16], _mm256_add_epi16(old_w2_i16, r2));
        // offset i + 32
        __m256i old_w2_i32 = _mm256_loadu_si256((const __m256i*)&w2[i + 32]);
        _mm256_storeu_si256((__m256i*)&w2[i + 32], _mm256_add_epi16(old_w2_i32, acc20));
    }

    for (i = 0; i < N_SM_RES; i += 16) {
        __m256i vr0 = _mm256_loadu_si256((const __m256i*)&w0[i]);
        __m256i vr1 = _mm256_loadu_si256((const __m256i*)&w1[i]);
        __m256i vr2 = _mm256_loadu_si256((const __m256i*)&w2[i]);

        __m256i sum_r0_r2 = _mm256_add_epi16(vr0, vr2);
        vr1 = _mm256_sub_epi16(vr1, sum_r0_r2);

        __m256i old_res_i = _mm256_loadu_si256((const __m256i*)&result_interp[i]);
        _mm256_storeu_si256((__m256i*)&result_interp[i], _mm256_add_epi16(old_res_i, vr2));

        __m256i old_res_i32 = _mm256_loadu_si256((const __m256i*)&result_interp[i + 32]);
        _mm256_storeu_si256((__m256i*)&result_interp[i + 32], _mm256_add_epi16(old_res_i32, vr1));

        __m256i old_res_i64 = _mm256_loadu_si256((const __m256i*)&result_interp[i + 64]);
        _mm256_storeu_si256((__m256i*)&result_interp[i + 64], _mm256_add_epi16(old_res_i64, vr0));
    }

    for (i = SCABBARD_N; i < 2 * SCABBARD_N; i += 16) {
        size_t target_idx = i - SCABBARD_N;

        __m256i res_low  = _mm256_loadu_si256((const __m256i*)&result_interp[target_idx]);
        __m256i res_high = _mm256_loadu_si256((const __m256i*)&result_interp[i]);

       __m256i diff = _mm256_sub_epi16(res_low, res_high);
        _mm256_storeu_si256((__m256i*)&r->coeffs[target_idx], diff);
    }
}