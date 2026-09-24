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
#include <string.h>
#include "KEX_AlgorithmInstance.h"
#include "drng.h"
#include "auxfunc.h"
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "polyvec.h"

// DRNG_ctx for generating pseudorandom numbers within the KEX protocol
extern DRNG_ctx drng_algorithm;

#define KEX_API_SUCCESS 0
#define KEX_VERIFY_FAILURE -1
#define KEX_API_CORE_FAILURE -2
#define KEX_API_DRNG_FAILURE -3

static int get_drng_bytes(unsigned char *out, unsigned long long out_len_bytes)
{
	if (get_random_number(&drng_algorithm, out, out_len_bytes * 8ULL) != 0)
		return KEX_API_DRNG_FAILURE;
	return KEX_API_SUCCESS;
}

// Byte-oriented wrapper for the bit-length pseudoXOF API
static void xof_bytes(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
	pseudoXOF((unsigned long long)outlen * 8ULL,
	          in,
	          (unsigned long long)inlen * 8ULL,
	          out);
}


unsigned long long kex_get_passes_num()       { return 4; }
unsigned long long kex_get_pk_len_bytes()     { return 2*KYBER_PUBLICKEYBYTES; }
unsigned long long kex_get_sk_len_bytes()     { return 2*KYBER_SECRETKEYBYTES; }
unsigned long long kex_get_sta_len_bytes()    { return 4*KYBER_SYMBYTES; }
unsigned long long kex_get_stb_len_bytes()    { return 4*KYBER_SYMBYTES; }
unsigned long long kex_get_ss_len_bytes()     { return KYBER_SYMBYTES; }
unsigned long long kex_get_total_msg_len_bytes() { return 2*KYBER_CIPHERTEXTBYTES + 2*KYBER_SYMBYTES; }


static int kex_init_self(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes,
	unsigned char *st, unsigned long long *st_len_bytes)
{
	unsigned char coins[2 * KYBER_SYMBYTES];
	unsigned char pk_s[KYBER_PUBLICKEYBYTES];
	unsigned char sk_s[KYBER_SECRETKEYBYTES];
	if (get_drng_bytes(coins, sizeof(coins)) != 0)
		return KEX_API_DRNG_FAILURE;
	if (crypto_kem_keypair_derand(pk_s, sk_s, coins) != 0)
		return KEX_API_CORE_FAILURE;

	unsigned char seed_e[KYBER_SYMBYTES];
	unsigned char pk_e[KYBER_INDCPA_PUBLICKEYBYTES];
	unsigned char sk_e[KYBER_INDCPA_SECRETKEYBYTES];
	if (get_drng_bytes(seed_e, sizeof(seed_e)) != 0)
		return KEX_API_DRNG_FAILURE;
	indcpa_keypair_ekeygen(pk_s, seed_e, pk_e, sk_e);

	polyvec pk_s_vec, sk_s_vec;
	polyvec sk_e_vec, pk_e_vec;
	polyvec cpk_vec, csk_vec;
	polyvec_frombytes(&sk_s_vec, sk_s);
	polyvec_frombytes(&sk_e_vec, sk_e);
	polyvec_add(&csk_vec, &sk_s_vec, &sk_e_vec);
	polyvec_reduce(&csk_vec);

	polyvec_frombytes(&pk_s_vec, pk_s);
	polyvec_frombytes(&pk_e_vec, pk_e);
	polyvec_add(&cpk_vec, &pk_s_vec, &pk_e_vec);
	polyvec_reduce(&cpk_vec);

	unsigned char csk[KYBER_SECRETKEYBYTES];
	unsigned char cpk[KYBER_PUBLICKEYBYTES];
	polyvec_tobytes(csk, &csk_vec);
	polyvec_tobytes(cpk, &cpk_vec);
	memcpy(cpk + KYBER_POLYVECBYTES, pk_s+KYBER_POLYVECBYTES, KYBER_SYMBYTES);
	memcpy(csk+KYBER_INDCPA_SECRETKEYBYTES, cpk, KYBER_PUBLICKEYBYTES);
	memcpy(csk+KYBER_SECRETKEYBYTES-PREFIXHASHBYTES-KYBER_SYMBYTES,cpk,PREFIXHASHBYTES); // hash_h(sk+KYBER_SECRETKEYBYTES-2*KYBER_SYMBYTES, pk, KYBER_PUBLICKEYBYTES);
	memcpy(csk+KYBER_SECRETKEYBYTES-KYBER_SYMBYTES, coins+KYBER_SYMBYTES, KYBER_SYMBYTES); // Value z for pseudo-random output on reject

	memcpy(pk, cpk, KYBER_PUBLICKEYBYTES);
	memcpy(sk, csk, KYBER_SECRETKEYBYTES);
	memcpy(pk+KYBER_PUBLICKEYBYTES, pk_s, KYBER_PUBLICKEYBYTES); // pk = (cpk,pk_s)
	memcpy(sk+KYBER_SECRETKEYBYTES, sk_s, KYBER_SECRETKEYBYTES); // sk = (csk,sk_s)

	memcpy(st, seed_e, KYBER_SYMBYTES); // seed_X (seed_A or seed_B)
	*pk_len_bytes = 2*KYBER_PUBLICKEYBYTES;
	*sk_len_bytes = 2*KYBER_SECRETKEYBYTES;
	*st_len_bytes = KYBER_SYMBYTES;
	return 0;
}

static int extract_pk(unsigned char *pk_s, unsigned char *cpk, unsigned char *pk_e)
{
	polyvec cpk_vec, pk_e_vec, pk_s_vec;
	polyvec_frombytes(&cpk_vec, cpk);
	polyvec_frombytes(&pk_e_vec, pk_e);
	polyvec_sub(&pk_s_vec, &cpk_vec, &pk_e_vec);
	polyvec_reduce(&pk_s_vec);

	polyvec_tobytes(pk_s, &pk_s_vec);
	memcpy(pk_s + KYBER_POLYVECBYTES, cpk+KYBER_POLYVECBYTES, KYBER_SYMBYTES);
	return 0;
}

int kex_init_a(
	unsigned char *pka, unsigned long long *pka_len_bytes,
	unsigned char *ska, unsigned long long *ska_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes)
{
	kex_init_self(pka, pka_len_bytes, ska, ska_len_bytes, sta, sta_len_bytes);
	return 0;
}

int kex_init_b(
	unsigned char *pkb, unsigned long long *pkb_len_bytes,
	unsigned char *skb, unsigned long long *skb_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes)
{
	kex_init_self(pkb, pkb_len_bytes, skb, skb_len_bytes, stb, stb_len_bytes);
	return 0;
}

int kex_generate_pass1_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m1, unsigned long long *m1_len_bytes)
{
	(void)ska;
	(void)ska_len_bytes;
	(void)pkb_len_bytes;

	unsigned char coins[KYBER_SYMBYTES];
	if (get_drng_bytes(coins, sizeof(coins)) != 0)
		return KEX_API_DRNG_FAILURE;
	if (crypto_kem_enc_derand(m1,  // ct_B
	                          sta+KYBER_SYMBYTES, // K_B
	                          pkb,   // cpk_B
							  coins) != 0)
		return KEX_API_CORE_FAILURE;

	*sta_len_bytes = 2*KYBER_SYMBYTES; // (seed_A,K_B)
	*m1_len_bytes = KYBER_CIPHERTEXTBYTES; // (ct_B)
	return 0;
}

int kex_generate_pass2_msg_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *m1, unsigned long long m1_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes,
	unsigned char *m2, unsigned long long *m2_len_bytes)
{
	(void)skb_len_bytes;
	(void)pka_len_bytes;
	(void)m1_len_bytes;

	unsigned char K_B[KYBER_SYMBYTES];
	if (crypto_kem_dec(K_B, 
						m1, // ct_B
						skb // csk_B
					  ) != 0)
		return KEX_API_CORE_FAILURE;

	unsigned char coins[KYBER_SYMBYTES];
	unsigned char K_A[KYBER_SYMBYTES];
	if (get_drng_bytes(coins, sizeof(coins)) != 0)
		return KEX_API_DRNG_FAILURE;
	if (crypto_kem_enc_derand(m2, // ct_A
							  K_A, 
							  pka, // cpk_A
							  coins) != 0)
		return KEX_API_CORE_FAILURE;

	unsigned char K[2*KYBER_SYMBYTES];
	memcpy(K, K_B, KYBER_SYMBYTES);
	memcpy(K + KYBER_SYMBYTES, K_A, KYBER_SYMBYTES);
	xof_bytes(stb+KYBER_SYMBYTES, 3 * KYBER_SYMBYTES, K, 2 * KYBER_SYMBYTES); // K_session || K_ENC^B || K_ENC^A
	for (int i = 0; i < KYBER_SYMBYTES; i++) {
		m2[KYBER_CIPHERTEXTBYTES+i] = stb[2*KYBER_SYMBYTES+i] ^ stb[i]; // c_seed^B = K_ENC^B ^ seed_B
	}
	*stb_len_bytes = 4*KYBER_SYMBYTES; // (seed_B,K_SESSION,K_ENC^B,K_ENC^A)
	*m2_len_bytes = KYBER_CIPHERTEXTBYTES+KYBER_SYMBYTES; // (ct_A,c_seed^B)
	return 0;
}

int kex_generate_pass3_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *m2, unsigned long long m2_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m3, unsigned long long *m3_len_bytes)
{
	(void)ska_len_bytes;
	(void)pkb_len_bytes;
	(void)m2_len_bytes;

	unsigned char K_A[KYBER_SYMBYTES];
	if (crypto_kem_dec(K_A, 
						m2, // ct_A
						ska // csk_A
						) != 0)
		return KEX_API_CORE_FAILURE;

	unsigned char K[2*KYBER_SYMBYTES];
	memcpy(K, sta+KYBER_SYMBYTES, KYBER_SYMBYTES); // K_B
	memcpy(K + KYBER_SYMBYTES, K_A, KYBER_SYMBYTES);
	xof_bytes(sta+KYBER_SYMBYTES, 3 * KYBER_SYMBYTES, K, 2 * KYBER_SYMBYTES); // K_session || K_ENC^B || K_ENC^A
	for (int i = 0; i < KYBER_SYMBYTES; i++) {
		m3[i] = sta[3*KYBER_SYMBYTES+i] ^ sta[i]; // c_seed^A = K_ENC^A ^ seed_A
	}
	*sta_len_bytes = 4*KYBER_SYMBYTES; // (seed_A,K_SESSION,K_ENC^B,K_ENC^A)
	*m3_len_bytes = KYBER_SYMBYTES; // c_seed^A

	unsigned char seed_B[KYBER_SYMBYTES];
	for (int i = 0; i < KYBER_SYMBYTES; i++) {
		seed_B[i] = sta[2*KYBER_SYMBYTES+i] ^ m2[KYBER_CIPHERTEXTBYTES+i]; // seed_B = K_ENC^B ^ c_seed^B
	}
	unsigned char pk_e[KYBER_PUBLICKEYBYTES];
	unsigned char sk_e[KYBER_INDCPA_SECRETKEYBYTES];
	unsigned char pk_s[KYBER_PUBLICKEYBYTES];
	indcpa_keypair_ekeygen(pkb, seed_B, pk_e, sk_e);
	extract_pk(pk_s, pkb, pk_e);
	if(memcmp(pk_s, pkb+KYBER_PUBLICKEYBYTES, KYBER_PUBLICKEYBYTES) != 0)
		return KEX_VERIFY_FAILURE;
	return 0;
}

int kex_generate_pass4_msg_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *m3, unsigned long long m3_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes,
	unsigned char *m4, unsigned long long *m4_len_bytes)
{
	(void)skb;
	(void)skb_len_bytes;
	(void)pka_len_bytes;
	(void)m3_len_bytes;
	(void)stb_len_bytes;
	(void)m4;
	(void)m4_len_bytes;

	unsigned char seed_A[KYBER_SYMBYTES];
	for (int i = 0; i < KYBER_SYMBYTES; i++) {
		seed_A[i] = stb[3*KYBER_SYMBYTES+i] ^ m3[i]; // seed_A = K_ENC^A ^ c_seed^A
	}
	unsigned char pk_e[KYBER_PUBLICKEYBYTES];
	unsigned char sk_e[KYBER_INDCPA_SECRETKEYBYTES];
	unsigned char pk_s[KYBER_PUBLICKEYBYTES];
	indcpa_keypair_ekeygen(pka, seed_A, pk_e, sk_e);
	extract_pk(pk_s, pka, pk_e);
	if(memcmp(pk_s, pka+KYBER_PUBLICKEYBYTES, KYBER_PUBLICKEYBYTES) != 0)
		return KEX_VERIFY_FAILURE;
	return 1;
}

int kex_derive_ss_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *mb, unsigned long long mb_len_bytes,
	unsigned char *sta, unsigned long long sta_len_bytes,
	unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
	(void)ska;
	(void)ska_len_bytes;
	(void)pkb;
	(void)pkb_len_bytes;
	(void)mb;
	(void)mb_len_bytes;
	(void)sta_len_bytes;

	memcpy(ssa, sta + KYBER_SYMBYTES, KYBER_SYMBYTES);
	*ssa_len_bytes = KYBER_SYMBYTES;
	return 0;
}

int kex_derive_ss_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *ma, unsigned long long ma_len_bytes,
	unsigned char *stb, unsigned long long stb_len_bytes,
	unsigned char *ssb, unsigned long long *ssb_len_bytes)
{
	(void)skb;
	(void)skb_len_bytes;
	(void)pka;
	(void)pka_len_bytes;
	(void)ma;
	(void)ma_len_bytes;
	(void)stb_len_bytes;

	memcpy(ssb, stb + KYBER_SYMBYTES, KYBER_SYMBYTES);
	*ssb_len_bytes = KYBER_SYMBYTES;
	return 0;
}
