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
#include <stdio.h>
#include <stdlib.h>
#include "KEX_AlgorithmInstance.h"
#include "primitive_interfaces.h"
#include "auxfunc.h"
#include "cretake_params.h"
#include "drng.h"
#include "secure_bzero.h"
#undef ALGORITHM_INSTANCE
#undef ALGORITHM_INSTANCE

// DRNG_ctx for generating pseudorandom numbers within the KEX protocol
extern DRNG_ctx drng_algorithm;

static void derive_session_key(unsigned char *outK,
	const unsigned char *tpk,
	const unsigned char *pkI,
	const unsigned char *pkR,
	const unsigned char *ct_tilde,
	const unsigned char *ct_i,
	const unsigned char *k_tilde,
	const unsigned long long k_tilde_len,
	const unsigned char *k_i,
	const unsigned long long k_i_len)
{
	unsigned char in[TPK_LEN + PKI_LEN + PKR_LEN + C_TILDE_LEN + C_I_LEN + KEM_K_LEN + KEM_K_LEN];
	unsigned long long off = 0;
	memcpy(in + off, tpk, TPK_LEN); off += TPK_LEN;
	memcpy(in + off, pkI, PKI_LEN); off += PKI_LEN;
	memcpy(in + off, pkR, PKR_LEN); off += PKR_LEN;
	memcpy(in + off, ct_tilde, C_TILDE_LEN); off += C_TILDE_LEN;
	memcpy(in + off, ct_i, C_I_LEN); off += C_I_LEN;
	memcpy(in + off, k_tilde, k_tilde_len); off += k_tilde_len;
	memcpy(in + off, k_i, k_i_len); off += k_i_len;
	pseudoXOF(K_LEN * 8, in, off * 8, outK);
}

unsigned long long kex_get_passes_num(){ 
	return 2; 
}

unsigned long long kex_get_pk_len_bytes(){ 
	return (PKI_LEN > PKR_LEN) ? PKI_LEN : PKR_LEN; 
}

// Added beyond the ICCS interfaces to support different initiator and responder public-key lengths
unsigned long long kex_get_pk_len_bytes_initiator(){ 
	return PKI_LEN; 
}

// Added beyond the ICCS interfaces to support different initiator and responder public-key lengths
unsigned long long kex_get_pk_len_bytes_responder(){ 
	return PKR_LEN; 
}

unsigned long long kex_get_sk_len_bytes(){ 
	return (SKI_LEN > SKR_LEN) ? SKI_LEN : SKR_LEN; 
}

// Added beyond the ICCS interfaces to support different initiator and responder private-key lengths
unsigned long long kex_get_sk_len_bytes_initiator(){ 
	return SKI_LEN; 
}

// Added beyond the ICCS interfaces to support different initiator and responder private-key lengths
unsigned long long kex_get_sk_len_bytes_responder(){ 
	return SKR_LEN; 
}

unsigned long long kex_get_sta_len_bytes(){ 
	return TPK_LEN + TSK_LEN; 
}

unsigned long long kex_get_stb_len_bytes(){ 
	return K_LEN; 
}

unsigned long long kex_get_ss_len_bytes(){ 
	return K_LEN; 
}

unsigned long long kex_get_total_msg_len_bytes(){ 
	return TPK_LEN + C_TILDE_LEN + C_I_LEN + SIG_LEN; 
}

int kex_init_a(unsigned char *pka,unsigned long long *pka_len_bytes,unsigned char *ska,unsigned long long *ska_len_bytes,unsigned char *sta,unsigned long long *sta_len_bytes){
	(void)sta;
	kem_keygen(pka,pka_len_bytes,ska,ska_len_bytes);
	*sta_len_bytes = 0;
	return 0;
}

int kex_init_b(unsigned char *pkb,unsigned long long *pkb_len_bytes,unsigned char *skb,unsigned long long *skb_len_bytes,unsigned char *stb,unsigned long long *stb_len_bytes){
	(void)stb;
	sig_keygen(pkb,pkb_len_bytes,skb,skb_len_bytes);
	*stb_len_bytes = 0;
	return 0;
}

int kex_generate_pass1_msg_a(unsigned char *ska,unsigned long long ska_len_bytes,unsigned char *pkb,unsigned long long pkb_len_bytes,unsigned char *sta,unsigned long long *sta_len_bytes,unsigned char *m1,unsigned long long *m1_len_bytes){
	(void)ska;(void)ska_len_bytes;(void)pkb;(void)pkb_len_bytes;(void)sta_len_bytes;
	pke_keygen(sta, sta + TPK_LEN);
	*sta_len_bytes = TPK_LEN + TSK_LEN;
	memcpy(m1, sta, TPK_LEN);
	*m1_len_bytes = TPK_LEN;
	return 0;
}

int kex_generate_pass2_msg_b(unsigned char *skb,unsigned long long skb_len_bytes,unsigned char *pka,unsigned long long pka_len_bytes,unsigned char *m1,unsigned long long m1_len_bytes,unsigned char *stb,unsigned long long *stb_len_bytes,unsigned char *m2,unsigned long long *m2_len_bytes){
	unsigned long long l_ct, l_ss, l_sig;
	unsigned char *ct_tilde = m2;
	unsigned char *ct_i = m2 + C_TILDE_LEN;
	unsigned char *sig = m2 + C_TILDE_LEN + C_I_LEN;
	unsigned char *pkb = (unsigned char *)calloc(PKR_LEN, sizeof(unsigned char));
	unsigned char *tpk = (unsigned char *)calloc(m1_len_bytes, sizeof(unsigned char));
	unsigned char k_tilde[KEM_K_LEN], k_i[KEM_K_LEN];
	unsigned char signed_msg[TPK_LEN + C_TILDE_LEN + C_I_LEN];
	unsigned char buf[SEED_BYTES], buf2[MSG_LEN_BYTES + SEED_BYTES];
	uint8_t seed_enc[SEED_BYTES];
	(void)pka_len_bytes;

	memcpy(pkb, skb, PKR_LEN);
	memcpy(tpk, m1, m1_len_bytes);
	/*if (get_random_number(&drng_algorithm, k_tilde, MSG_LEN_BYTES * 8ULL) != 0) {
        return -1;
    }
	if (get_random_number(&drng_algorithm, seed_enc, SEED_BYTES * 8ULL) != 0) {
        return -1;
    }*/
   	if (get_random_number(&drng_algorithm, buf, SEED_BYTES * 8ULL) != 0) {
        return -1;
    }
	pseudoXOF((MSG_LEN_BYTES + SEED_BYTES) * 8, buf, SEED_BYTES, buf2);
	memcpy(k_tilde, buf2, MSG_LEN_BYTES);
	memcpy(seed_enc, buf2 + MSG_LEN_BYTES, SEED_BYTES);
	pke_enc(tpk, k_tilde, seed_enc, ct_tilde);
	//kem_enc(m1, TPK_LEN, k_tilde, &l_ss, ct_tilde, &l_ct);
	//pseudohash(K_LEN * 8, ct_tilde, C_TILDE_LEN * 8, k_tilde);
	
	kem_enc(pka, PKI_LEN, k_i, &l_ss, ct_i, &l_ct);
	/*memcpy(stb, ct_tilde, C_TILDE_LEN);
	memcpy(stb + C_TILDE_LEN, ct_i, C_I_LEN);
	memcpy(stb + C_TILDE_LEN + C_I_LEN, k_tilde, K_LEN);
	memcpy(stb + C_TILDE_LEN + C_I_LEN + K_LEN, k_i, K_LEN);
	*stb_len_bytes = C_TILDE_LEN + C_I_LEN + K_LEN + K_LEN;*/
	memcpy(signed_msg, m1, TPK_LEN);
	memcpy(signed_msg + TPK_LEN, ct_tilde, C_TILDE_LEN);
	memcpy(signed_msg + TPK_LEN + C_TILDE_LEN, ct_i, C_I_LEN);
	sig_sign(skb, skb_len_bytes, signed_msg, sizeof(signed_msg), sig, &l_sig);
	*m2_len_bytes = C_TILDE_LEN + C_I_LEN + l_sig;

	derive_session_key(stb, tpk, pka, pkb, ct_tilde, ct_i, k_tilde, KEM_K_LEN, k_i, KEM_K_LEN);
	*stb_len_bytes = K_LEN;
	free(pkb);
	free(tpk);
	return 1;
}

int kex_derive_ss_a(unsigned char *ska,unsigned long long ska_len_bytes,unsigned char *pkb,unsigned long long pkb_len_bytes,unsigned char *mb,unsigned long long mb_len_bytes,unsigned char *sta,unsigned long long sta_len_bytes,unsigned char *ssa,unsigned long long *ssa_len_bytes){
	unsigned char *ct_tilde = mb;
	unsigned char *ct_i = mb + C_TILDE_LEN;
	unsigned char *sig = mb + C_TILDE_LEN + C_I_LEN;
	unsigned long long sig_len = mb_len_bytes - C_TILDE_LEN - C_I_LEN;
	unsigned char signed_msg[TPK_LEN + C_TILDE_LEN + C_I_LEN];
	unsigned char k_tilde[KEM_K_LEN], k_i[KEM_K_LEN];
	const unsigned char *tpk = sta;
	unsigned char *tsk = sta + TPK_LEN;
	unsigned char pkI[PKI_LEN];
	unsigned long long kem_k_len=0;
	(void)pkb_len_bytes;

	memcpy(pkI, ska + TSK_LEN, PKI_LEN);
	memcpy(signed_msg, tpk, TPK_LEN);
	memcpy(signed_msg + TPK_LEN, ct_tilde, C_TILDE_LEN);
	memcpy(signed_msg + TPK_LEN + C_TILDE_LEN, ct_i, C_I_LEN);
	if(sig_verify(pkb, PKR_LEN, sig, sig_len, signed_msg, sizeof(signed_msg)) != 0){
		return -1;
	}

	pke_dec(tsk, ct_tilde, k_tilde);
	kem_dec(ska, ska_len_bytes, ct_i, C_I_LEN, k_i, &kem_k_len);
	derive_session_key(ssa, tpk, pkI, pkb, ct_tilde, ct_i, k_tilde, KEM_K_LEN, k_i, KEM_K_LEN);
	*ssa_len_bytes = K_LEN;
	secure_bzero(sta, sta_len_bytes);
	return 0;
}

int kex_derive_ss_b(unsigned char *skb,unsigned long long skb_len_bytes,unsigned char *pka,unsigned long long pka_len_bytes,unsigned char *ma,unsigned long long ma_len_bytes,unsigned char *stb,unsigned long long stb_len_bytes,unsigned char *ssb,unsigned long long *ssb_len_bytes){
	(void)skb_len_bytes;(void)pka_len_bytes;(void)ma_len_bytes;(void)skb;(void)pka;(void)ma;
	if(stb_len_bytes != K_LEN) 
		return -1;
	memcpy(ssb, stb, stb_len_bytes);
	*ssb_len_bytes = K_LEN;
	secure_bzero(stb, stb_len_bytes);
	
	return 0;
}
