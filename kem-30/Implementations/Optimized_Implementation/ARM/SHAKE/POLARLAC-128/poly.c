/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial compression, encoding, and arithmetic helpers for the optimized POLARLAC-128 instance.
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



static const int64_t llr_table[] = {282331693575682, 846995080727043, 1411658467878406, 1976321855029769, 2540985242181130, 3105648629332493, 3670312016483855, 4234975403635218, 4799638790786580, 5364302177937941, 5928965565089304, 6493628952240667, 7058292339392028, 7622955726543391, 8187619113694754, 8752282500846115, 9316945887997478, 9881609275148840, 10446272662300202, 11010936049451564, 11575599436602926, 12140262823754288, 12704926210905652, 13269589598057014, 13834252985208376, 14398916372359738, 14963579759511100, 15528243146662462, 16092906533813826, 16657569920965188, 17222233308116552, 17786896695267912, 18351560082419276, 18916223469570636, 19480886856722000, 20045550243873364, 20610213631024724, 21174877018176084, 21739540405327444, 22304203792478796, 22868867179630124, 23433530566781376, 23998193953932336, 24562857341082304, 25127520728228796, 25692184115363164, 26256847502455216, 26821510889399616, 27386174275828768, 27950837660459908, 28515501038816740, 29080164395278804, 29644827675337196, 30209490688778332, 30774152771836272, 31338811608253808, 31903459115397728, 32468067090030568, 33032537136641104, 33596526165102212, 34158840207084848, 34715352316432664, 35252127223491972, 35725639166415748, 36028797018963932, 36022127766359780, 35705631408603296, 35218780960471224, 34668667548203612, 34098816933647500, 33523164386456564, 32945836852787156, 32368028300968316, 31790081821127176, 31212095808774960, 30634098467149120, 30056097878882880, 29478096360233444, 28900094574966760, 28322092713296388, 27744090829731260, 27166088939891820, 26588087048254368, 26010085156101668, 25432083263801312, 24854081371458648, 24276079479103852, 23698077586745592, 23120075694386324, 22542073802026776, 21964071909667152, 21386070017307500, 20808068124947836, 20230066232588180, 19652064340228516, 19074062447868852, 18496060555509188, 17918058663149530, 17340056770789866, 16762054878430206, 16184052986070542, 15606051093710880, 15028049201351218, 14450047308991554, 13872045416631894, 13294043524272228, 12716041631912570, 12138039739552906, 11560037847193244, 10982035954833580, 10404034062473918, 9826032170114256, 9248030277754596, 8670028385394933, 8092026493035271, 7514024600675607, 6936022708315946, 6358020815956283, 5780018923596623, 5202017031236960, 4624015138877297, 4046013246517636, 3468011354157974, 2890009461798312, 2312007569438648, 1734005677078986, 1156003784719324, 578001892359662, 0, -578001892359662, -1156003784719324, -1734005677078986, -2312007569438648, -2890009461798312, -3468011354157974, -4046013246517636, -4624015138877297, -5202017031236960, -5780018923596623, -6358020815956283, -6936022708315946, -7514024600675607, -8092026493035271, -8670028385394933, -9248030277754596, -9826032170114256, -10404034062473918, -10982035954833580, -11560037847193244, -12138039739552906, -12716041631912570, -13294043524272228, -13872045416631894, -14450047308991554, -15028049201351218, -15606051093710880, -16184052986070542, -16762054878430204, -17340056770789866, -17918058663149530, -18496060555509188, -19074062447868852, -19652064340228516, -20230066232588180, -20808068124947836, -21386070017307500, -21964071909667152, -22542073802026776, -23120075694386324, -23698077586745592, -24276079479103852, -24854081371458648, -25432083263801312, -26010085156101668, -26588087048254368, -27166088939891820, -27744090829731260, -28322092713296388, -28900094574966760, -29478096360233444, -30056097878882880, -30634098467149120, -31212095808774956, -31790081821127176, -32368028300968316, -32945836852787156, -33523164386456564, -34098816933647500, -34668667548203612, -35218780960471224, -35705631408603296, -36022127766359780, -36028797018963932, -35725639166415748, -35252127223491972, -34715352316432664, -34158840207084848, -33596526165102212, -33032537136641104, -32468067090030568, -31903459115397728, -31338811608253808, -30774152771836272, -30209490688778332, -29644827675337196, -29080164395278804, -28515501038816740, -27950837660459908, -27386174275828768, -26821510889399616, -26256847502455216, -25692184115363164, -25127520728228796, -24562857341082304, -23998193953932336, -23433530566781376, -22868867179630124, -22304203792478796, -21739540405327444, -21174877018176084, -20610213631024724, -20045550243873364, -19480886856722000, -18916223469570636, -18351560082419276, -17786896695267912, -17222233308116552, -16657569920965188, -16092906533813826, -15528243146662462, -14963579759511100, -14398916372359738, -13834252985208376, -13269589598057014, -12704926210905652, -12140262823754288, -11575599436602926, -11010936049451564, -10446272662300202, -9881609275148840, -9316945887997478, -8752282500846116, -8187619113694754, -7622955726543391, -7058292339392028, -6493628952240667, -5928965565089304, -5364302177937941, -4799638790786580, -4234975403635218, -3670312016483855, -3105648629332493, -2540985242181130, -1976321855029769, -1411658467878406, -846995080727043, -282331693575682};


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
     * Fixed zero-selector lossless PK encoding for RL_KEM_N coefficients in Z_257.
     *
     * Layout:
     *   code_c[0 .. RL_KEM_N-1]
     *       low 8 bits of each coefficient.
     *
     *   code_c[RL_KEM_N .. RL_KEM_N + PK_ZERO_SELECTOR_BYTES - 1]
     *       selector bits for the positions whose low byte is zero, packed in
     *       scan order. A selector bit is 0 for original value 0 and 1 for 256.
     *
     * If the number of low-zero candidates exceeds PK_ZERO_SELECTOR_BITS, the
     * encoder reports overflow and KeyGen resamples b. The coefficient loop
     * uses byte truncation, shifts, and logical operations only; no % or float
     * arithmetic is used on this path.
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
            if (candidates < PK_ZERO_SELECTOR_BITS) {
                uint16_t byte_idx = candidates >> 3;
                uint8_t bit_pos = candidates & 7U;
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
    for (; i < RL_KEM_N; i++) {
        c[i] = 0;
    }
}

//d-bit压缩结果的紧凑打包和对应解包函数
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
        com_c2[2 * i] = (uint8_t)(t & 0x0F);
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
