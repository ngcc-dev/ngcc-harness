/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial compression, encoding, and arithmetic helpers for the reference POLARLAC-Light instance.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "polar.h"

/// @brief Reciprocal precision used by the fixed-point `c2` compression helper.
#define RECIP_K 24u
/// @brief Fixed-point reciprocal approximation of `1 / RL_KEM_Q`.
/// @details This constant lets the implementation replace a runtime division by
///          a multiply-and-shift sequence during `c2` compression.
#define RECIP_Q (((1u << RECIP_K) + (RL_KEM_Q / 2u)) / RL_KEM_Q)


// // Definitions for tables declared extern in poly.h


static const int64_t llr_table[] = {281882223332513, 845646669997541, 1409411116662568, 1973175563327597, 2536940009992624, 3100704456657651, 3664468903322679, 4228233349987706, 4791997796652733, 5355762243317761, 5919526689982789, 6483291136647816, 7047055583312844, 7610820029977870, 8174584476642899, 8738348923307925, 9302113369972952, 9865877816637980, 10429642263303008, 10993406709968036, 11557171156633062, 12120935603298090, 12684700049963118, 13248464496628144, 13812228943293170, 14375993389958200, 14939757836623226, 15503522283288254, 16067286729953284, 16631051176618308, 17194815623283338, 17758580069948364, 18322344516613392, 18886108963278420, 19449873409943448, 20013637856608472, 20577402303273500, 21141166749938532, 21704931196603556, 22268695643268584, 22832460089933608, 23396224536598632, 23959988983263644, 24523753429928600, 25087517876593340, 25651282323257224, 26215046769917684, 26778811216564468, 27342575663156560, 27906340109530116, 28470104555030300, 29033868997040044, 29597633425100388, 30161397797412308, 30725161946927312, 31288925206041804, 31852684906711856, 32416430386415628, 32980119036519308, 33543580634013488, 34106135863356460, 34665085358076792, 35209883244070948, 35701930291182596, 36028797018963964, 36022138383767132, 35681954385592104, 35176590068086796, 34618474911698976, 34046208146584984, 33470335646848348, 32893556778960500, 32316550858463164, 31739488108365724, 31162411137302012, 30585330607793864, 30008249187885196, 29431167545179612, 28854085846725608, 28277004134322196, 27699922418428356, 27122840701661136, 26545758984675376, 25968677267634940, 25391595550580812, 24814513833523268, 24237432116464864, 23660350399406248, 23083268682347572, 22506186965288888, 21929105248230200, 21352023531171512, 20774941814112824, 20197860097054136, 19620778379995444, 19043696662936752, 18466614945878064, 17889533228819374, 17312451511760686, 16735369794701996, 16158288077643306, 15581206360584618, 15004124643525926, 14427042926467238, 13849961209408546, 13272879492349858, 12695797775291168, 12118716058232480, 11541634341173790, 10964552624115100, 10387470907056412, 9810389189997722, 9233307472939034, 8656225755880343, 8079144038821651, 7502062321762965, 6924980604704273, 6347898887645585, 5770817170586896, 5193735453528206, 4616653736469516, 4039572019410827, 3462490302352138, 2885408585293447, 2308326868234758, 1731245151176068, 1154163434117380, 577081717058690, 0, -577081717058690, -1154163434117380, -1731245151176068, -2308326868234758, -2885408585293447, -3462490302352138, -4039572019410827, -4616653736469516, -5193735453528206, -5770817170586896, -6347898887645585, -6924980604704273, -7502062321762965, -8079144038821651, -8656225755880343, -9233307472939034, -9810389189997722, -10387470907056412, -10964552624115100, -11541634341173790, -12118716058232480, -12695797775291168, -13272879492349858, -13849961209408546, -14427042926467238, -15004124643525926, -15581206360584618, -16158288077643306, -16735369794701996, -17312451511760686, -17889533228819376, -18466614945878064, -19043696662936752, -19620778379995444, -20197860097054136, -20774941814112824, -21352023531171512, -21929105248230200, -22506186965288888, -23083268682347572, -23660350399406248, -24237432116464864, -24814513833523268, -25391595550580812, -25968677267634936, -26545758984675376, -27122840701661136, -27699922418428356, -28277004134322196, -28854085846725608, -29431167545179612, -30008249187885196, -30585330607793864, -31162411137302012, -31739488108365724, -32316550858463164, -32893556778960500, -33470335646848348, -34046208146584984, -34618474911698976, -35176590068086796, -35681954385592104, -36022138383767132, -36028797018963964, -35701930291182596, -35209883244070948, -34665085358076792, -34106135863356460, -33543580634013488, -32980119036519308, -32416430386415628, -31852684906711856, -31288925206041804, -30725161946927312, -30161397797412308, -29597633425100388, -29033868997040044, -28470104555030300, -27906340109530116, -27342575663156560, -26778811216564468, -26215046769917684, -25651282323257224, -25087517876593340, -24523753429928600, -23959988983263644, -23396224536598632, -22832460089933608, -22268695643268584, -21704931196603556, -21141166749938532, -20577402303273500, -20013637856608472, -19449873409943448, -18886108963278420, -18322344516613392, -17758580069948364, -17194815623283338, -16631051176618308, -16067286729953284, -15503522283288254, -14939757836623226, -14375993389958200, -13812228943293170, -13248464496628144, -12684700049963118, -12120935603298090, -11557171156633062, -10993406709968036, -10429642263303008, -9865877816637980, -9302113369972952, -8738348923307925, -8174584476642899, -7610820029977870, -7047055583312844, -6483291136647816, -5919526689982789, -5355762243317761, -4791997796652733, -4228233349987706, -3664468903322679, -3100704456657651, -2536940009992624, -1973175563327597, -1409411116662568, -845646669997540, -281882223332513};


void byte_compress(uint8_t *dst, const int16_t *src)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        dst[i] = (uint8_t)src[i];
    }
}
void byte_decompress(int16_t *dst, const uint8_t *src)
{
    for (int i = 0; i < RL_KEM_N; i++) {
        dst[i] = (int16_t)src[i];
    }
}

void polyvec_byte_compress(uint8_t *dst, const polarlac_polyvec *src)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        byte_compress(dst + k * RL_KEM_N, src->vec[k].coeffs);
    }
}

void polyvec_byte_decompress(polarlac_polyvec *dst, const uint8_t *src)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        byte_decompress(dst->vec[k].coeffs, src + k * RL_KEM_N);
    }
}

int poly_compress(uint8_t *code_c, const int16_t *c)
{
    /*
     * Fixed zero-selector lossless PK encoding for coefficients in Z_257.
     *
     * Layout:
     *   code_c[0 .. RL_KEM_N-1]
     *       low 8 bits of each coefficient.
     *
     *   code_c[RL_KEM_N .. RL_KEM_N + PK_ZERO_SELECTOR_BYTES - 1]
     *       selector bits for low-zero positions, packed in scan order.
     *
     * This function is intended for canonical modulo-257 polynomials such as
     * public b_ntt and cached secret s_ntt. Avoiding an extra rl_kem_mod_q()
     * call here keeps the encoding path minimal.
     */
    uint8_t *selector = code_c + RL_KEM_N;
    uint16_t candidates = 0;
    uint16_t byte_idx = 0;
    uint8_t bit_pos = 0;

    memset(selector, 0, PK_ZERO_SELECTOR_BYTES);
    for (int i = 0; i < RL_KEM_N; i++) {
        uint8_t low = (uint8_t)c[i];
        code_c[i] = low;

        if (low == 0) {
            if (candidates >= PK_ZERO_SELECTOR_BITS) {
                return -1;
            }
            byte_idx = candidates >> 3;
            bit_pos = candidates & 7U;
            selector[byte_idx] |= (uint8_t)((c[i] >> 8) << bit_pos);
            candidates++;
        }
    }

    return 0;
}

void poly_decompress(int16_t *c, const uint8_t *code_c)
{
    const uint8_t *selector = code_c + RL_KEM_N;
    uint16_t candidates = 0;

    for (int i = 0; i < RL_KEM_N; i++) {
        if (code_c[i] == 0) {
            uint16_t val = (uint16_t)code_c[i];
                uint16_t byte_idx = candidates >> 3;
                uint8_t  bit_pos = candidates & 7U;
                uint16_t sel = (selector[byte_idx] >> bit_pos) & 1U;
                val |= (uint16_t)(sel << 8);
            c[i] = (int16_t)val;
            candidates++;
        } else {
            c[i] = (int16_t)code_c[i];
        }
    }
}

int polyvec_compress(uint8_t *code_c, const polarlac_polyvec *c)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        if (poly_compress(code_c + k * PK_POLY_BYTES, c->vec[k].coeffs) != 0) {
            return -1;
        }
    }
    return 0;
}

void polyvec_decompress(polarlac_polyvec *c, const uint8_t *code_c)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        poly_decompress(c->vec[k].coeffs, code_c + k * PK_POLY_BYTES);
    }
}


void poly_compress_c2(uint8_t *code_c, const int16_t *c, unsigned int d)
{
    uint32_t i;
    uint32_t mask = (1u << d) - 1u;

    for (i = 0; i < RL_KEM_Lv; i++) {
        uint32_t x = (uint32_t)c[i];
        uint32_t y = (x << d) + (RL_KEM_Q >> 1);
        uint32_t t = (uint32_t)(((uint64_t)y * (uint64_t)RECIP_Q) >> RECIP_K);
        code_c[i] = (uint8_t)(t & mask);
    }
}


void poly_decompress_c2(int16_t *c, const uint8_t *code_c, unsigned int d)
{
    uint32_t i;
    uint32_t half = 1u << (d - 1);

    for (i = 0; i < RL_KEM_Lv; i++) {
        c[i] = (uint16_t)((((uint32_t)code_c[i] * (uint32_t)RL_KEM_Q) + half) >> d);
    }
    for (i = RL_KEM_Lv; i < RL_KEM_N; i++) {
        c[i] = 0;
    }
}

// d-bit压缩结果的紧凑打包和对应解包函数
void pack_c2_dbit(uint8_t *dst, const uint8_t *com_c2)
{
    for (int i = 0, j = 0; i < RL_KEM_Lv; i += 8, j += 3) {
        uint32_t t =
            ((uint32_t)(com_c2[i + 0] & 0x07)) |
            ((uint32_t)(com_c2[i + 1] & 0x07) << 3) |
            ((uint32_t)(com_c2[i + 2] & 0x07) << 6) |
            ((uint32_t)(com_c2[i + 3] & 0x07) << 9) |
            ((uint32_t)(com_c2[i + 4] & 0x07) << 12) |
            ((uint32_t)(com_c2[i + 5] & 0x07) << 15) |
            ((uint32_t)(com_c2[i + 6] & 0x07) << 18) |
            ((uint32_t)(com_c2[i + 7] & 0x07) << 21);

        dst[j + 0] = (uint8_t)(t & 0xFF);
        dst[j + 1] = (uint8_t)((t >> 8) & 0xFF);
        dst[j + 2] = (uint8_t)((t >> 16) & 0xFF);
    }
}

void unpack_c2_dbit(uint8_t *com_c2, const uint8_t *src)
{
    for (int i = 0, j = 0; i < RL_KEM_Lv; i += 8, j += 3) {
        uint32_t t = (uint32_t)src[j + 0] |
                     ((uint32_t)src[j + 1] << 8) |
                     ((uint32_t)src[j + 2] << 16);

        com_c2[i + 0] = (uint8_t)(t & 0x07);
        com_c2[i + 1] = (uint8_t)((t >> 3) & 0x07);
        com_c2[i + 2] = (uint8_t)((t >> 6) & 0x07);
        com_c2[i + 3] = (uint8_t)((t >> 9) & 0x07);
        com_c2[i + 4] = (uint8_t)((t >> 12) & 0x07);
        com_c2[i + 5] = (uint8_t)((t >> 15) & 0x07);
        com_c2[i + 6] = (uint8_t)((t >> 18) & 0x07);
        com_c2[i + 7] = (uint8_t)((t >> 21) & 0x07);
    }
}


void poly_mul(int16_t *out, const int16_t *a, const int16_t *s)
{
    int16_t a_ntt[RL_KEM_N];
    int16_t s_ntt[RL_KEM_N];

    memcpy(a_ntt, a, RL_KEM_N * sizeof(int16_t));
    memcpy(s_ntt, s, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(a_ntt);
    mq_poly_ntt(s_ntt);
    mq_poly_pointwise_mul(out, a_ntt, s_ntt);
    mq_poly_intt(out);
}

void poly_mul_with_cached_ntt(int16_t *out, const int16_t *a, const int16_t *s_ntt)
{
    int16_t a_ntt[RL_KEM_N];

    memcpy(a_ntt, a, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(a_ntt);
    mq_poly_pointwise_mul(out, a_ntt, (int16_t *)s_ntt);
    mq_poly_intt(out);
}

void poly_mul_add_with_ntt(int16_t *out, const int16_t *a, const int16_t *s, const int16_t *e)
{
    int16_t s_ntt[RL_KEM_N];
    int16_t prod[RL_KEM_N];

    memcpy(s_ntt, s, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(s_ntt);
    mq_poly_pointwise_mul(prod, (int16_t *)a, s_ntt);
    mq_poly_intt(prod);


    for (int i = 0; i < RL_KEM_N; i++) {
        out[i] = rl_kem_mod_q((int32_t)prod[i] + e[i]);
    }


}


void poly_mul_add_with_cached_ntt(int16_t *out, const int16_t *a_ntt, const int16_t *b_ntt, const int16_t *e)
{
    int16_t prod[RL_KEM_N];

    mq_poly_pointwise_mul(prod, (int16_t *)a_ntt, (int16_t *)b_ntt);
    mq_poly_intt(prod);

    for (int i = 0; i < RL_KEM_N; i++) {
        out[i] = rl_kem_mod_q((int32_t)prod[i] + e[i]);
    }
}


void poly_mul_add(int16_t *out, const int16_t *a, const int16_t *s, const int16_t *e)
{
    int16_t a_ntt[RL_KEM_N];
    int16_t s_ntt[RL_KEM_N];
    int16_t prod[RL_KEM_N];

    memcpy(a_ntt, a, RL_KEM_N * sizeof(int16_t));
    memcpy(s_ntt, s, RL_KEM_N * sizeof(int16_t));

    mq_poly_ntt(a_ntt);
    mq_poly_ntt(s_ntt);
    mq_poly_pointwise_mul(prod, a_ntt, s_ntt);
    mq_poly_intt(prod);


    for (int i = 0; i < RL_KEM_N; i++) {
        out[i] = rl_kem_mod_q((int32_t)prod[i] + e[i]);
    }


}


/* FOR POLAR */
// #define MAX_RECORDS 100000 // number of tests
// uint64_t polar_enc_cycle[MAX_RECORDS];
// int polar_enc_index = 0;
// uint64_t polar_dec_cycle[MAX_RECORDS];
// int polar_dec_index = 0;


void Encode_m(uint8_t *code_m, uint8_t *m)
{
	int i, j;
    int info_cnt = 0;

    /* polar encoding */
    uint64_t u_64[CODE_LEN/8]; /* source sequence: each element stores 64 bit */
    memset(u_64, 0, (CODE_LEN/8)*sizeof(uint64_t));

	/* fill the message m into the source sequence */
    for (i = 0; i < CODE_LEN/8; i++)
    {
        for(j = 0; j < 64; j++)
        {
            if(info_nodes[64*i+j] == 1)
            {
				u_64[i] |= (((uint64_t)(m[info_cnt/8] >> (info_cnt%8)) & 0x01) << j);
                info_cnt++;
            }
        }
    }

    // uint64_t polar_enc_start = rl_kem_read_cycles();
    encode_polar_opt(u_64);
    // uint64_t polar_enc_end = rl_kem_read_cycles();
    // if (polar_enc_index < MAX_RECORDS) {
    //     polar_enc_cycle[polar_enc_index] = polar_enc_end - polar_enc_start;
    //     polar_enc_index++;
    // }

    /* extract first RL_KEM_Lv bits */
    for(i = 0; i < CODE_LEN/8; i++)
    {
        for(j = 0; j < 64; j++)
        {
            code_m[i*64+j] = (uint8_t)((u_64[i] >> j) & 0x0000000000000001);
        }
    }
}



void Decode_m(uint8_t *m, int16_t *hatm)
{
    int half_2 = 65; // q/2 /2
    int64_t llr[CODE_LEN*8]; // log-likelihood ratio of the received signal
   
	// compute llr
	for(int i = 0; i < RL_KEM_Lv; i++)
	{
		int16_t centered = (int16_t)(hatm[i] - half_2);
		if (centered > (RATIO - 1)) {
            centered = (int16_t)(centered - RL_KEM_Q);
		} else if (centered < -RATIO) {
            centered = (int16_t)(centered + RL_KEM_Q);
        }
		llr[i] = llr_table[centered + RATIO - 1]; // 0 is modulated to -q/4, and 1 is modulated to q/4
	}

	// uint64_t polar_dec_start = rl_kem_read_cycles();
	//polar decode to recover m
	decode_polar(m, llr);
	// uint64_t polar_dec_end = rl_kem_read_cycles();
	// if (polar_dec_index < MAX_RECORDS) {
    //     polar_dec_cycle[polar_dec_index] = polar_dec_end - polar_dec_start;
	// 	polar_dec_index++;
    // }
}
/* END FOR POLAR */
