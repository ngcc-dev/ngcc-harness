/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <stdint.h>
#include "KEX_AlgorithmInstance.h"
#include "ADKEX_parameters.h"
#include "drng.h"
#include "adkex_derand.h"

// DRNG_ctx for generating pseudorandom numbers within the KEX protocol
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kex_get_passes_num()           { return ADKEX_PASSES_NUM; }
unsigned long long kex_get_pk_len_bytes()         { return ADKEX_PKBITS / 8; }
unsigned long long kex_get_sk_len_bytes()         { return ADKEX_SKBITS / 8; }
unsigned long long kex_get_sta_len_bytes()        { return ADKEX_STA_MAX_BITS / 8; }
unsigned long long kex_get_stb_len_bytes()        { return ADKEX_STB_MAX_BITS / 8; }
unsigned long long kex_get_ss_len_bytes()         { return ADKEX_SSBITS / 8; }
unsigned long long kex_get_total_msg_len_bytes()  { return ADKEX_TOTAL_MSG_BITS / 8; }

int kex_init_a(
    unsigned char *pka, unsigned long long *pka_len_bytes,
    unsigned char *ska, unsigned long long *ska_len_bytes,
    unsigned char *sta, unsigned long long *sta_len_bytes)
{
    /* A holds no long-term key and no state after init. */
    (void)pka; (void)ska; (void)sta;
    *pka_len_bytes = 0;
    *ska_len_bytes = 0;
    *sta_len_bytes = 0;
    return 0;
}

int kex_init_b(
    unsigned char *pkb, unsigned long long *pkb_len_bytes,
    unsigned char *skb, unsigned long long *skb_len_bytes,
    unsigned char *stb, unsigned long long *stb_len_bytes)
{
    uint8_t coins[ADKEX_INIT_B_COINBITS / 8];
    (void)stb;

    get_random_number(&drng_algorithm, coins, ADKEX_INIT_B_COINBITS);
    ADKEX_init_b_derand(pkb, skb, coins);

    *pkb_len_bytes = ADKEX_PKBITS / 8;
    *skb_len_bytes = ADKEX_SKBITS / 8;
    *stb_len_bytes = 0;                  /* st_B = epsilon after init */
    return 0;
}

int kex_generate_pass1_msg_a(
    unsigned char *ska, unsigned long long ska_len_bytes,
    unsigned char *pkb, unsigned long long pkb_len_bytes,
    unsigned char *sta, unsigned long long *sta_len_bytes,
    unsigned char *m1,  unsigned long long *m1_len_bytes)
{
    uint8_t coins[ADKEX_PASS1_COINBITS / 8];
    (void)ska; (void)ska_len_bytes; (void)pkb_len_bytes;

    get_random_number(&drng_algorithm, coins, ADKEX_PASS1_COINBITS);
    ADKEX_pass1_msg_a_derand(m1, sta, pkb, coins);

    *m1_len_bytes  = ADKEX_M1_BITS      / 8;
    *sta_len_bytes = ADKEX_STA_MAX_BITS / 8;
    return 0;                            /* not finished (2 passes total) */
}

int kex_generate_pass2_msg_b(
    unsigned char *skb, unsigned long long skb_len_bytes,
    unsigned char *pka, unsigned long long pka_len_bytes,
    unsigned char *m1,  unsigned long long m1_len_bytes,
    unsigned char *stb, unsigned long long *stb_len_bytes,
    unsigned char *m2,  unsigned long long *m2_len_bytes)
{
    /* The ICCS signature for pass2_msg_b does not include pkb, but the
       transcript binder T needs pk_B. DKEM's FO-style secret key already
       embeds the public key (sk_B = sA || pk_B || z), so we recover pk_B
       from the suffix of skb. */
    uint8_t coins[ADKEX_PASS2_COINBITS / 8];
    const uint8_t *pk_B = skb + (ADKEX_SKBITS - ADKEX_PKBITS - ADKEX_SSBITS) / 8;

    (void)skb_len_bytes; (void)pka; (void)pka_len_bytes;
    (void)m1_len_bytes;

    get_random_number(&drng_algorithm, coins, ADKEX_PASS2_COINBITS);
    ADKEX_pass2_msg_b_derand(m2, stb, m1, pk_B, skb, coins);

    *m2_len_bytes  = ADKEX_M2_BITS      / 8;
    *stb_len_bytes = ADKEX_STB_MAX_BITS / 8;
    return 1;                            /* last pass */
}

/* ICCS KAT_KEX.c references kex_generate_pass3_msg_a unconditionally,
   but for the 2-pass protocol pass2_msg_b returns 1 so this is never
   reached at runtime. Provide a defensive stub to satisfy the linker. */
int kex_generate_pass3_msg_a(
    unsigned char *ska, unsigned long long ska_len_bytes,
    unsigned char *pkb, unsigned long long pkb_len_bytes,
    unsigned char *m2,  unsigned long long m2_len_bytes,
    unsigned char *sta, unsigned long long *sta_len_bytes,
    unsigned char *m3,  unsigned long long *m3_len_bytes)
{
    (void)ska; (void)ska_len_bytes; (void)pkb; (void)pkb_len_bytes;
    (void)m2;  (void)m2_len_bytes;  (void)sta; (void)sta_len_bytes;
    (void)m3;  (void)m3_len_bytes;
    return -1;
}

int kex_derive_ss_a(
    unsigned char *ska, unsigned long long ska_len_bytes,
    unsigned char *pkb, unsigned long long pkb_len_bytes,
    unsigned char *mb,  unsigned long long mb_len_bytes,
    unsigned char *sta, unsigned long long sta_len_bytes,
    unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
    /* mb = last responder message = m_2. */
    (void)ska; (void)ska_len_bytes; (void)pkb_len_bytes;
    (void)mb_len_bytes; (void)sta_len_bytes;

    ADKEX_derive_ss_a(ssa, mb, sta, pkb);
    *ssa_len_bytes = ADKEX_SSBITS / 8;
    return 0;
}

int kex_derive_ss_b(
    unsigned char *skb, unsigned long long skb_len_bytes,
    unsigned char *pka, unsigned long long pka_len_bytes,
    unsigned char *ma,  unsigned long long ma_len_bytes,
    unsigned char *stb, unsigned long long stb_len_bytes,
    unsigned char *ssb, unsigned long long *ssb_len_bytes)
{
    /* B already folded everything (ss_e, ss_s, T) into st_B in pass 2. */
    (void)skb; (void)skb_len_bytes; (void)pka; (void)pka_len_bytes;
    (void)ma;  (void)ma_len_bytes;  (void)stb_len_bytes;

    ADKEX_derive_ss_b(ssb, stb);
    *ssb_len_bytes = ADKEX_SSBITS / 8;
    return 0;
}

/*
int kex_generate_pass4_msg_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *m3, unsigned long long m3_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes,
	unsigned char *m4, unsigned long long *m4_len_bytes)
{
	return 1;
}
int kex_generate_pass5_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *m4, unsigned long long m4_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m5, unsigned long long *m5_len_bytes)
{
	return 1;
}
*/
