/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial compression, encoding, and arithmetic helpers for the reference POLARLAC-512 instance.
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


static const int64_t llr_table[] = {70434865856991, 211304597570973, 352174329284955, 493044060998937, 633913792712920, 774783524426901, 915653256140884, 1056522987854866, 1197392719568848, 1338262451282830, 1479132182996812, 1620001914710794, 1760871646424776, 1901741378138758, 2042611109852740, 2183480841566722, 2324350573280704, 2465220304994686, 2606090036708669, 2746959768422650, 2887829500136632, 3028699231850615, 3169568963564596, 3310438695278579, 3451308426992561, 3592178158706543, 3733047890420525, 3873917622134507, 4014787353848489, 4155657085562471, 4296526817276453, 4437396548990435, 4578266280704417, 4719136012418399, 4860005744132381, 5000875475846363, 5141745207560345, 5282614939274327, 5423484670988309, 5564354402702291, 5705224134416273, 5846093866130255, 5986963597844236, 6127833329558212, 6268703061272169, 6409572792986046, 6550442524699585, 6691312256411706, 6832181988117875, 6973051719799022, 7113921451375032, 7254791182509256, 7395660911787100, 7536530633264449, 7677400321964200, 7818269872932860, 7959138845158393, 8100005385540349, 8240861707865808, 8381675103341918, 8522308285206142, 8662187104823200, 8798946244756284, 8923417021784182, 9007199254740983, 9005535439012078, 8918425574597467, 8790627166111757, 8650540394720863, 8507333943645994, 8363373130323960, 8219232103390039, 8075048149606769, 7930853977767003, 7786657374083659, 7642460191657189, 7498262871499627, 7354065518564467, 7209868157828813, 7065670795236778, 6921473432202959, 6777276069064000, 6633078705900022, 6488881342730089, 6344683979558740, 6200486616387052, 6056289253215284, 5912091890043497, 5767894526871706, 5623697163699914, 5479499800528121, 5335302437356328, 5191105074184536, 5046907711012743, 4902710347840951, 4758512984669158, 4614315621497365, 4470118258325573, 4325920895153780, 4181723531981988, 4037526168810195, 3893328805638402, 3749131442466609, 3604934079294816, 3460736716123024, 3316539352951232, 3172341989779439, 3028144626607646, 2883947263435854, 2739749900264060, 2595552537092268, 2451355173920476, 2307157810748683, 2162960447576890, 2018763084405097, 1874565721233305, 1730368358061512, 1586170994889719, 1441973631717927, 1297776268546134, 1153578905374341, 1009381542202549, 865184179030756, 720986815858963, 576789452687171, 432592089515378, 288394726343585, 144197363171793, 0, -144197363171793, -288394726343585, -432592089515378, -576789452687171, -720986815858963, -865184179030756, -1009381542202549, -1153578905374341, -1297776268546134, -1441973631717927, -1586170994889719, -1730368358061512, -1874565721233305, -2018763084405097, -2162960447576890, -2307157810748683, -2451355173920476, -2595552537092268, -2739749900264060, -2883947263435854, -3028144626607646, -3172341989779439, -3316539352951232, -3460736716123024, -3604934079294816, -3749131442466609, -3893328805638402, -4037526168810195, -4181723531981988, -4325920895153780, -4470118258325573, -4614315621497365, -4758512984669158, -4902710347840951, -5046907711012743, -5191105074184536, -5335302437356328, -5479499800528121, -5623697163699914, -5767894526871706, -5912091890043497, -6056289253215284, -6200486616387052, -6344683979558740, -6488881342730089, -6633078705900022, -6777276069064000, -6921473432202959, -7065670795236778, -7209868157828813, -7354065518564467, -7498262871499627, -7642460191657189, -7786657374083659, -7930853977767003, -8075048149606769, -8219232103390039, -8363373130323960, -8507333943645994, -8650540394720863, -8790627166111757, -8918425574597467, -9005535439012078, -9007199254740983, -8923417021784182, -8798946244756284, -8662187104823200, -8522308285206142, -8381675103341918, -8240861707865808, -8100005385540349, -7959138845158393, -7818269872932860, -7677400321964200, -7536530633264449, -7395660911787100, -7254791182509256, -7113921451375032, -6973051719799022, -6832181988117875, -6691312256411706, -6550442524699585, -6409572792986046, -6268703061272169, -6127833329558212, -5986963597844236, -5846093866130255, -5705224134416273, -5564354402702291, -5423484670988309, -5282614939274327, -5141745207560345, -5000875475846363, -4860005744132381, -4719136012418399, -4578266280704417, -4437396548990435, -4296526817276453, -4155657085562471, -4014787353848489, -3873917622134507, -3733047890420525, -3592178158706543, -3451308426992561, -3310438695278579, -3169568963564596, -3028699231850615, -2887829500136632, -2746959768422650, -2606090036708669, -2465220304994686, -2324350573280704, -2183480841566722, -2042611109852740, -1901741378138758, -1760871646424776, -1620001914710794, -1479132182996812, -1338262451282830, -1197392719568848, -1056522987854866, -915653256140884, -774783524426901, -633913792712920, -493044060998937, -352174329284955, -211304597570973, -70434865856991};


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
        uint32_t y = ((uint32_t)c[i] << d) + (RL_KEM_Q >> 1);
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

void pack_c2_dbit(uint8_t *dst, const uint8_t *com_c2)
{
#if D_C2_BITS == 4
    for (int i = 0; i < RL_KEM_Lv / 2; i++) {
        uint8_t lo = (uint8_t)(com_c2[2 * i] & 0x0F);
        uint8_t hi = (uint8_t)(com_c2[2 * i + 1] & 0x0F);
        dst[i] = (uint8_t)(lo | (hi << 4));
    }
#elif D_C2_BITS == 3
    for (int i = 0, j = 0; i < RL_KEM_Lv; i += 8, j += 3) {
        uint32_t t =
            ((uint32_t)(com_c2[i + 0] & 0x07)      ) |
            ((uint32_t)(com_c2[i + 1] & 0x07) <<  3) |
            ((uint32_t)(com_c2[i + 2] & 0x07) <<  6) |
            ((uint32_t)(com_c2[i + 3] & 0x07) <<  9) |
            ((uint32_t)(com_c2[i + 4] & 0x07) << 12) |
            ((uint32_t)(com_c2[i + 5] & 0x07) << 15) |
            ((uint32_t)(com_c2[i + 6] & 0x07) << 18) |
            ((uint32_t)(com_c2[i + 7] & 0x07) << 21);

        dst[j + 0] = (uint8_t)( t        & 0xFF);
        dst[j + 1] = (uint8_t)((t >> 8 ) & 0xFF);
        dst[j + 2] = (uint8_t)((t >> 16) & 0xFF);
    }
#else
#error "Unsupported D_C2_BITS"
#endif
}

void unpack_c2_dbit(uint8_t *com_c2, const uint8_t *src)
{
#if D_C2_BITS == 4
    for (int i = 0; i < RL_KEM_Lv / 2; i++) {
        uint8_t t = src[i];
        com_c2[2 * i]     = (uint8_t)(t & 0x0F);
        com_c2[2 * i + 1] = (uint8_t)((t >> 4) & 0x0F);
    }
#elif D_C2_BITS == 3
    for (int i = 0, j = 0; i < RL_KEM_Lv; i += 8, j += 3) {
        uint32_t t =
            ((uint32_t)src[j + 0]      ) |
            ((uint32_t)src[j + 1] <<  8) |
            ((uint32_t)src[j + 2] << 16);

        com_c2[i + 0] = (uint8_t)((t      ) & 0x07);
        com_c2[i + 1] = (uint8_t)((t >>  3) & 0x07);
        com_c2[i + 2] = (uint8_t)((t >>  6) & 0x07);
        com_c2[i + 3] = (uint8_t)((t >>  9) & 0x07);
        com_c2[i + 4] = (uint8_t)((t >> 12) & 0x07);
        com_c2[i + 5] = (uint8_t)((t >> 15) & 0x07);
        com_c2[i + 6] = (uint8_t)((t >> 18) & 0x07);
        com_c2[i + 7] = (uint8_t)((t >> 21) & 0x07);
    }
#else
#error "Unsupported D_C2_BITS"
#endif
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
