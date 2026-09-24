/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial compression, encoding, and arithmetic helpers for the optimized POLARLAC-256 instance.
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
/// @details This constant lets the reference code replace a runtime division by
///          a multiply-and-shift sequence during `c2` compression.
#define RECIP_Q (((1u << RECIP_K) + (RL_KEM_Q / 2u)) / RL_KEM_Q)



static const int64_t llr_table[] = {141089181650599, 423267544951794, 705445908252992, 987624271554188, 1269802634855385, 1551980998156581, 1834159361457778, 2116337724758975, 2398516088060172, 2680694451361368, 2962872814662565, 3245051177963761, 3527229541264958, 3809407904566154, 4091586267867351, 4373764631168548, 4655942994469744, 4938121357770941, 5220299721072138, 5502478084373333, 5784656447674530, 6066834810975727, 6349013174276923, 6631191537578121, 6913369900879316, 7195548264180514, 7477726627481709, 7759904990782907, 8042083354084104, 8324261717385299, 8606440080686497, 8888618443987694, 9170796807288890, 9452975170590086, 9735153533891284, 10017331897192480, 10299510260493676, 10581688623794872, 10863866987096068, 11146045350397264, 11428223713698454, 11710402076999632, 11992580440300752, 12274758803601676, 12556937166901882, 12839115530199462, 13121293893487482, 13403472256740692, 13685650619867092, 13967828982531580, 14250007343513516, 14532185698366624, 14814364030895008, 15096542282103860, 15378720237100262, 15660897113122826, 15943070058938790, 16225228689192344, 16507335180792430, 16789251844690412, 17070478256459890, 17349206306448962, 17619039711573300, 17858960429665366, 18014398509481964, 18011065694167382, 17848961983721626, 17602375635000394, 17325876599246896, 17040482918628660, 16752590876230024, 16464008581702876, 16175236459473630, 15886412198590908, 15597573622145784, 15308731115494062, 15019887529868496, 14731043648030482, 14442199684872936, 14153355699390664, 13864511707779568, 13575667714485918, 13286823720730356, 12997979726847984, 12709135732930800, 12420291739004058, 12131447745074692, 11842603751144606, 11553759757214322, 11264915763283982, 10976071769353628, 10687227775423272, 10398383781492916, 10109539787562556, 9820695793632196, 9531851799701840, 9243007805771480, 8954163811841120, 8665319817910763, 8376475823980403, 8087631830050044, 7798787836119686, 7509943842189327, 7221099848258969, 6932255854328609, 6643411860398251, 6354567866467892, 6065723872537533, 5776879878607175, 5488035884676815, 5199191890746457, 4910347896816099, 4621503902885740, 4332659908955381, 4043815915025023, 3754971921094664, 3466127927164305, 3177283933233946, 2888439939303588, 2599595945373229, 2310751951442871, 2021907957512511, 1733063963582153, 1444219969651793, 1155375975721435, 866531981791076, 577687987860718, 288843993930359, 0, -288843993930358, -577687987860718, -866531981791076, -1155375975721435, -1444219969651793, -1733063963582153, -2021907957512511, -2310751951442871, -2599595945373229, -2888439939303588, -3177283933233946, -3466127927164305, -3754971921094664, -4043815915025023, -4332659908955382, -4621503902885740, -4910347896816099, -5199191890746457, -5488035884676815, -5776879878607175, -6065723872537533, -6354567866467892, -6643411860398251, -6932255854328609, -7221099848258969, -7509943842189327, -7798787836119686, -8087631830050044, -8376475823980403, -8665319817910763, -8954163811841120, -9243007805771480, -9531851799701840, -9820695793632196, -10109539787562556, -10398383781492916, -10687227775423272, -10976071769353628, -11264915763283982, -11553759757214322, -11842603751144606, -12131447745074692, -12420291739004058, -12709135732930800, -12997979726847984, -13286823720730356, -13575667714485918, -13864511707779568, -14153355699390664, -14442199684872936, -14731043648030482, -15019887529868496, -15308731115494062, -15597573622145784, -15886412198590908, -16175236459473630, -16464008581702876, -16752590876230024, -17040482918628660, -17325876599246896, -17602375635000394, -17848961983721626, -18011065694167382, -18014398509481964, -17858960429665366, -17619039711573300, -17349206306448962, -17070478256459890, -16789251844690412, -16507335180792430, -16225228689192344, -15943070058938790, -15660897113122826, -15378720237100262, -15096542282103860, -14814364030895008, -14532185698366624, -14250007343513516, -13967828982531580, -13685650619867092, -13403472256740692, -13121293893487482, -12839115530199462, -12556937166901882, -12274758803601676, -11992580440300752, -11710402076999632, -11428223713698454, -11146045350397264, -10863866987096068, -10581688623794872, -10299510260493676, -10017331897192480, -9735153533891284, -9452975170590086, -9170796807288890, -8888618443987692, -8606440080686497, -8324261717385299, -8042083354084104, -7759904990782907, -7477726627481709, -7195548264180514, -6913369900879316, -6631191537578120, -6349013174276923, -6066834810975727, -5784656447674530, -5502478084373333, -5220299721072138, -4938121357770941, -4655942994469744, -4373764631168548, -4091586267867351, -3809407904566154, -3527229541264958, -3245051177963761, -2962872814662565, -2680694451361368, -2398516088060172, -2116337724758975, -1834159361457778, -1551980998156581, -1269802634855385, -987624271554188, -705445908252992, -423267544951794, -141089181650599};


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

int poly_compress(uint8_t *code_c, int16_t *c)
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
        // uint16_t x = (uint16_t)c[i];
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
            if (candidates < PK_ZERO_SELECTOR_BITS) {
                uint16_t byte_idx = candidates >> 3;
                uint8_t  bit_pos = candidates & 7U;
                uint16_t sel = (selector[byte_idx] >> bit_pos) & 1U;
                val |= (uint16_t)(sel << 8);
            }
            c[i] = (int16_t)val;
            candidates++;
        } else {
            c[i] = (int16_t)code_c[i];
        }
    }
}

int polyvec_compress(uint8_t *code_c, polarlac_polyvec *c)
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


void poly_compress_c2(uint8_t *code_c, int16_t *c, unsigned int d)
{
    uint32_t i;
    uint32_t mask = (1u << d) - 1u;

    for (i = 0; i < RL_KEM_Lv; i++) {
        uint32_t y = ((uint32_t)c[i] << d) + (RL_KEM_Q >> 1);
        uint32_t t = (uint32_t)(((uint64_t)y * (uint64_t)RECIP_Q) >> RECIP_K);
        code_c[i] = (uint8_t)(t & mask);
    }
}


void poly_decompress_c2(int16_t *c, uint8_t *code_c, unsigned int d)
{
    uint32_t i;
    uint32_t half = 1u << (d - 1);

    for (i = 0; i < RL_KEM_Lv; i++) {
        c[i] = (uint16_t)((((uint32_t)code_c[i] * (uint32_t)RL_KEM_Q) + half) >> d);
    }
    for (; i < RL_KEM_N; i++) {
        c[i] = 0;
    }
}

//压缩到4bit后的紧凑打包和对应解包函数
void pack_c2_dbit(uint8_t *dst, const uint8_t *com_c2)
{
    for (int i = 0; i < RL_KEM_Lv / 2; i++) {
        uint8_t lo = (uint8_t)(com_c2[2 * i] & 0x0F);
        uint8_t hi = (uint8_t)(com_c2[2 * i + 1] & 0x0F);
        dst[i] = (uint8_t)(lo | (hi << 4));
    }
}

void unpack_c2_dbit(uint8_t *com_c2, const uint8_t *src)
{
    for (int i = 0; i < RL_KEM_Lv / 2; i++) {
        uint8_t t = src[i];
        com_c2[2 * i]     = (uint8_t)(t & 0x0F);
        com_c2[2 * i + 1] = (uint8_t)((t >> 4) & 0x0F);
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
