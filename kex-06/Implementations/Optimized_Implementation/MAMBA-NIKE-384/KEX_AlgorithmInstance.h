/*
MAMBA-NIKE — MAMBA Non-Interactive Key Exchange
Power-of-two modulus NIKE using Toom-Cook-4 polynomial multiplication.

Algorithm instance for NGCC KEX API.
*/

#ifndef KEX_ALGORITHM_INSTANCE_H
#define KEX_ALGORITHM_INSTANCE_H

// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
#define OUTPUT_BLANK_TEST_VECTORS 0

// Algorithm instance name
#define ALGORITHM_INSTANCE "MAMBA-NIKE-384"

#ifdef __cplusplus
extern "C"
{
#endif

	unsigned long long kex_get_passes_num();
	unsigned long long kex_get_pk_len_bytes();
	unsigned long long kex_get_sk_len_bytes();
	unsigned long long kex_get_sta_len_bytes();
	unsigned long long kex_get_stb_len_bytes();
	unsigned long long kex_get_ss_len_bytes();
	unsigned long long kex_get_total_msg_len_bytes();

	int kex_init_a(
		unsigned char *pka, unsigned long long *pka_len_bytes,
		unsigned char *ska, unsigned long long *ska_len_bytes,
		unsigned char *sta, unsigned long long *sta_len_bytes);

	int kex_init_b(
		unsigned char *pkb, unsigned long long *pkb_len_bytes,
		unsigned char *skb, unsigned long long *skb_len_bytes,
		unsigned char *stb, unsigned long long *stb_len_bytes);

	int kex_generate_pass1_msg_a(
		unsigned char *ska, unsigned long long ska_len_bytes,
		unsigned char *pkb, unsigned long long pkb_len_bytes,
		unsigned char *sta, unsigned long long *sta_len_bytes,
		unsigned char *m1, unsigned long long *m1_len_bytes);

	int kex_generate_pass2_msg_b(
		unsigned char *skb, unsigned long long skb_len_bytes,
		unsigned char *pka, unsigned long long pka_len_bytes,
		unsigned char *m1, unsigned long long m1_len_bytes,
		unsigned char *stb, unsigned long long *stb_len_bytes,
		unsigned char *m2, unsigned long long *m2_len_bytes);

	int kex_generate_pass3_msg_a(
		unsigned char *ska, unsigned long long ska_len_bytes,
		unsigned char *pkb, unsigned long long pkb_len_bytes,
		unsigned char *m2, unsigned long long m2_len_bytes,
		unsigned char *sta, unsigned long long *sta_len_bytes,
		unsigned char *m3, unsigned long long *m3_len_bytes);

	int kex_derive_ss_a(
		unsigned char *ska, unsigned long long ska_len_bytes,
		unsigned char *pkb, unsigned long long pkb_len_bytes,
		unsigned char *mb, unsigned long long mb_len_bytes,
		unsigned char *sta, unsigned long long sta_len_bytes,
		unsigned char *ssa, unsigned long long *ssa_len_bytes);

	int kex_derive_ss_b(
		unsigned char *skb, unsigned long long skb_len_bytes,
		unsigned char *pka, unsigned long long pka_len_bytes,
		unsigned char *ma, unsigned long long ma_len_bytes,
		unsigned char *stb, unsigned long long stb_len_bytes,
		unsigned char *ssb, unsigned long long *ssb_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
