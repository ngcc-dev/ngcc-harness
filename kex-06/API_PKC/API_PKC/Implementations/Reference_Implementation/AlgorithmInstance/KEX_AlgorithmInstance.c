/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEX_AlgorithmInstance.h"
#include "drng.h"

// DRNG_ctx for generating pseudorandom numbers within the KEX protocol
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kex_get_passes_num()
{
	return 3;
}

unsigned long long kex_get_pk_len_bytes()
{
	return 0;
}

unsigned long long kex_get_sk_len_bytes()
{
	return 0;
}

unsigned long long kex_get_sta_len_bytes()
{
	return 0;
}

unsigned long long kex_get_stb_len_bytes()
{
	return 0;
}

unsigned long long kex_get_ss_len_bytes()
{
	return 0;
}

unsigned long long kex_get_total_msg_len_bytes()
{
	return 0;
}

int kex_init_a(
	unsigned char *pka, unsigned long long *pka_len_bytes,
	unsigned char *ska, unsigned long long *ska_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes)
{
	return 0;
}

int kex_init_b(
	unsigned char *pkb, unsigned long long *pkb_len_bytes,
	unsigned char *skb, unsigned long long *skb_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes)
{
	return 0;
}

int kex_generate_pass1_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m1, unsigned long long *m1_len_bytes)
{
	return 0;
}

int kex_generate_pass2_msg_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *m1, unsigned long long m1_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes,
	unsigned char *m2, unsigned long long *m2_len_bytes)
{
	return 0;
}

int kex_generate_pass3_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *m2, unsigned long long m2_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m3, unsigned long long *m3_len_bytes)
{
	return 1;
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

int kex_derive_ss_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *mb, unsigned long long mb_len_bytes,
	unsigned char *sta, unsigned long long sta_len_bytes,
	unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
	return 0;
}

int kex_derive_ss_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *ma, unsigned long long ma_len_bytes,
	unsigned char *stb, unsigned long long stb_len_bytes,
	unsigned char *ssb, unsigned long long *ssb_len_bytes)
{
	return 0;
}