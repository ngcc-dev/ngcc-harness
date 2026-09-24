/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <string.h>
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
    uint8_t coins[ADKEX_INIT_A_COINBITS / 8];

    get_random_number(&drng_algorithm, coins, ADKEX_INIT_A_COINBITS);
    ADKEX_init_a_derand(pka, ska, coins);

    /* Stash pk_A in sta so pass1_msg_a can recover it. */
    memcpy(sta, pka, ADKEX_SIGPKBITS / 8);

    *pka_len_bytes = ADKEX_PKBITS / 8;
    *ska_len_bytes = ADKEX_SKBITS / 8;
    *sta_len_bytes = ADKEX_SIGPKBITS / 8;
    return 0;
}

int kex_init_b(
    unsigned char *pkb, unsigned long long *pkb_len_bytes,
    unsigned char *skb, unsigned long long *skb_len_bytes,
    unsigned char *stb, unsigned long long *stb_len_bytes)
{
    uint8_t coins[ADKEX_INIT_B_COINBITS / 8];

    get_random_number(&drng_algorithm, coins, ADKEX_INIT_B_COINBITS);
    ADKEX_init_b_derand(pkb, skb, coins);

    /* Stash pk_B in stb so pass2_msg_b can recover it. */
    memcpy(stb, pkb, ADKEX_SIGPKBITS / 8);

    *pkb_len_bytes = ADKEX_PKBITS / 8;
    *skb_len_bytes = ADKEX_SKBITS / 8;
    *stb_len_bytes = ADKEX_SIGPKBITS / 8;
    return 0;
}

int kex_generate_pass1_msg_a(
    unsigned char *ska, unsigned long long ska_len_bytes,
    unsigned char *pkb, unsigned long long pkb_len_bytes,
    unsigned char *sta, unsigned long long *sta_len_bytes,
    unsigned char *m1,  unsigned long long *m1_len_bytes)
{
    uint8_t coins[ADKEX_PASS1_COINBITS / 8];
    (void)ska; (void)ska_len_bytes; (void)pkb; (void)pkb_len_bytes;
    (void)sta_len_bytes;

    /* sta currently holds pk_A (stashed by init_a). Snapshot it so we
       can re-place it after writing sk_e/M_1, since the derand layer
       expects pk_A at the end of the final layout. */
    uint8_t pk_A_tmp[ADKEX_SIGPKBITS / 8];
    memcpy(pk_A_tmp, sta, ADKEX_SIGPKBITS / 8);

    get_random_number(&drng_algorithm, coins, ADKEX_PASS1_COINBITS);
    ADKEX_pass1_msg_a_derand(m1, sta, pk_A_tmp, coins);

    memset(pk_A_tmp, 0, sizeof pk_A_tmp);

    *m1_len_bytes  = ADKEX_M1_BITS      / 8;
    *sta_len_bytes = ADKEX_STA_MAX_BITS / 8;
    return 0;
}

int kex_generate_pass2_msg_b(
    unsigned char *skb, unsigned long long skb_len_bytes,
    unsigned char *pka, unsigned long long pka_len_bytes,
    unsigned char *m1,  unsigned long long m1_len_bytes,
    unsigned char *stb, unsigned long long *stb_len_bytes,
    unsigned char *m2,  unsigned long long *m2_len_bytes)
{
    uint8_t coins[ADKEX_PASS2_DKEX_COINBITS / 8];
    (void)skb_len_bytes; (void)pka_len_bytes; (void)m1_len_bytes;
    (void)stb_len_bytes;

    /* stb holds pk_B (stashed by init_b). Snapshot, then let the derand
       layer overwrite stb with the pass2 layout. */
    uint8_t pk_B_tmp[ADKEX_SIGPKBITS / 8];
    memcpy(pk_B_tmp, stb, ADKEX_SIGPKBITS / 8);

    get_random_number(&drng_algorithm, coins, ADKEX_PASS2_DKEX_COINBITS);
    ADKEX_pass2_msg_b_derand(m2, stb, m1, pka, pk_B_tmp, skb, coins);

    memset(pk_B_tmp, 0, sizeof pk_B_tmp);

    *m2_len_bytes  = ADKEX_M2_BITS         / 8;
    *stb_len_bytes = ADKEX_STB_PASS2_BITS  / 8;
    return 0;
}

int kex_generate_pass3_msg_a(
    unsigned char *ska, unsigned long long ska_len_bytes,
    unsigned char *pkb, unsigned long long pkb_len_bytes,
    unsigned char *m2,  unsigned long long m2_len_bytes,
    unsigned char *sta, unsigned long long *sta_len_bytes,
    unsigned char *m3,  unsigned long long *m3_len_bytes)
{
    (void)ska_len_bytes; (void)pkb_len_bytes; (void)m2_len_bytes;
    (void)sta_len_bytes;

    int rc = ADKEX_pass3_msg_a_derand(m3, sta, m2, pkb, ska);
    if (rc < 0) return -1;

    *m3_len_bytes  = ADKEX_M3_BITS / 8;
    *sta_len_bytes = ADKEX_KDF_INBITS / 8;   /* st_A = (ss || T) = 2*SSBITS (v20260618) */
    return 1;                            /* last pass */
}

int kex_derive_ss_a(
    unsigned char *ska, unsigned long long ska_len_bytes,
    unsigned char *pkb, unsigned long long pkb_len_bytes,
    unsigned char *mb,  unsigned long long mb_len_bytes,
    unsigned char *sta, unsigned long long sta_len_bytes,
    unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
    /* A finished in pass3; ss_raw is at the front of sta. */
    (void)ska; (void)ska_len_bytes; (void)pkb; (void)pkb_len_bytes;
    (void)mb;  (void)mb_len_bytes;  (void)sta_len_bytes;

    ADKEX_derive_ss_a(ssa, sta);
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
    (void)skb; (void)skb_len_bytes; (void)pka_len_bytes;
    (void)ma_len_bytes; (void)stb_len_bytes;

    int rc = ADKEX_derive_ss_b(ssb, ma, stb, pka);
    if (rc < 0) return -1;
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
