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
#include "twokem.h"
#include "secure_bzero.h"
#undef ALGORITHM_INSTANCE
#undef ALGORITHM_INSTANCE

// DRNG_ctx for generating pseudorandom numbers within the KEX protocol
extern DRNG_ctx drng_algorithm;

static void derive_session_key(unsigned char *outK,
	const unsigned char *tpk,
	const unsigned char *pkI,
	const unsigned char *pkR,
	const unsigned char *ct_i,
	const unsigned char *ct_j,
	const unsigned char *k_i,
	const unsigned long long k_i_len,
	const unsigned char *k_j,
	const unsigned long long k_j_len)
{
	unsigned char in[TPK_LEN + PKI_LEN + PKR_LEN + C_I_LEN + C_R_LEN + KEM_K_LEN + KEM_K_LEN];
	unsigned long long off = 0;
	memcpy(in + off, tpk, TPK_LEN); off += TPK_LEN;
	memcpy(in + off, pkI, PKI_LEN); off += PKI_LEN;
	memcpy(in + off, pkR, PKR_LEN); off += PKR_LEN;
	memcpy(in + off, ct_i, C_I_LEN); off += C_I_LEN;
	memcpy(in + off, ct_j, C_R_LEN); off += C_R_LEN;
	memcpy(in + off, k_i, k_i_len); off += k_i_len;
	memcpy(in + off, k_j, k_j_len); off += k_j_len;
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
	return SEED_BYTES + KEM_K_LEN + C_R_LEN; 
}

unsigned long long kex_get_stb_len_bytes(){ 
	return K_LEN; 
}

unsigned long long kex_get_ss_len_bytes(){ 
	return K_LEN; 
}

unsigned long long kex_get_total_msg_len_bytes(){ 
	return TPK_LEN + C_R_LEN + C_I_LEN; 
}

int kex_init_a(unsigned char *pka,unsigned long long *pka_len_bytes,unsigned char *ska,unsigned long long *ska_len_bytes,unsigned char *sta,unsigned long long *sta_len_bytes){
	(void)sta;
	twokem_keygen1(pka,pka_len_bytes,ska,ska_len_bytes);
	*sta_len_bytes = 0;
	return 0;
}

int kex_init_b(unsigned char *pkb,unsigned long long *pkb_len_bytes,unsigned char *skb,unsigned long long *skb_len_bytes,unsigned char *stb,unsigned long long *stb_len_bytes){
	(void)stb;
	kem_keygen(pkb,pkb_len_bytes,skb,skb_len_bytes);
	*stb_len_bytes = 0;
	return 0;
}

int kex_generate_pass1_msg_a(unsigned char *ska,unsigned long long ska_len_bytes,unsigned char *pkb,unsigned long long pkb_len_bytes,unsigned char *sta,unsigned long long *sta_len_bytes,unsigned char *m1,unsigned long long *m1_len_bytes){
	(void)ska;(void)ska_len_bytes;(void)pkb;(void)pkb_len_bytes;(void)sta_len_bytes;
	unsigned char seed_kg[SEED_BYTES];
	unsigned char *tpk = m1;
	unsigned char *ct_j = m1 + TPK_LEN;
	unsigned char buf[SEED_BYTES + SKI_LEN], tsk[TSK_LEN], k_j[KEM_K_LEN];
	unsigned long long tpk_l=0, tsk_l=0, ss_l=0, ct_l=0;
	

	int ret;
	if (get_random_number(&drng_algorithm, buf, SEED_BYTES * 8ULL) != 0) {
        return -1;
    }
	memcpy(buf + SEED_BYTES, ska, SKI_LEN);
	pseudohash(SEED_BYTES * 8, buf, SEED_BYTES + SKI_LEN, seed_kg);
	ret = twokem_keygen2_withseed(seed_kg, SEED_BYTES, tpk, &tpk_l, tsk, &tsk_l);
    if (ret != 0) {
        return -2;
    }
	ret = kem_enc(pkb, PKR_LEN, k_j, &ss_l, ct_j, &ct_l);
	if (ret != 0) {
        return -3;
    }

	*m1_len_bytes = TPK_LEN + C_R_LEN;

	memcpy(sta, buf, SEED_BYTES);  // randomness r
	*sta_len_bytes = SEED_BYTES;
	memcpy(sta + *sta_len_bytes, k_j, ss_l);  // kj
	*sta_len_bytes = *sta_len_bytes + ss_l;
	memcpy(sta + *sta_len_bytes, ct_j, ct_l);  // ctj
	*sta_len_bytes = *sta_len_bytes + ct_l;
	
	return 0;
}

int kex_generate_pass2_msg_b(unsigned char *skb,unsigned long long skb_len_bytes,unsigned char *pka,unsigned long long pka_len_bytes,unsigned char *m1,unsigned long long m1_len_bytes,unsigned char *stb,unsigned long long *stb_len_bytes,unsigned char *m2,unsigned long long *m2_len_bytes){
	unsigned long long l_ct=0, l_ss=0;
	unsigned char ct_j[C_R_LEN], k_j[KEM_K_LEN];
	unsigned char *ct_i = m2;
	unsigned char *pkb = (unsigned char *)calloc(PKR_LEN, sizeof(unsigned char));
	unsigned char *tpk = (unsigned char *)calloc(TPK_LEN, sizeof(unsigned char));
	unsigned char k_i[KEM_K_LEN];
	(void)m1_len_bytes;

	memcpy(pkb, skb + TSK_LEN, PKR_LEN);
	memcpy(tpk, m1, TPK_LEN);
	memcpy(ct_j, m1 + TPK_LEN, C_R_LEN);

	twokem_enc(pka, pka_len_bytes, tpk, TPK_LEN, k_i, &l_ss, ct_i, &l_ct);
	*m2_len_bytes = l_ct;

	kem_dec(skb, skb_len_bytes, ct_j, C_R_LEN, k_j, &l_ss);

	derive_session_key(stb, tpk, pka, pkb, ct_i, ct_j, k_i, KEM_K_LEN, k_j, KEM_K_LEN);
	*stb_len_bytes = K_LEN;
	free(pkb);
	free(tpk);
	return 1;
}

int kex_derive_ss_a(unsigned char *ska,unsigned long long ska_len_bytes,unsigned char *pkb,unsigned long long pkb_len_bytes,unsigned char *mb,unsigned long long mb_len_bytes,unsigned char *sta,unsigned long long sta_len_bytes,unsigned char *ssa,unsigned long long *ssa_len_bytes){
	unsigned char *ct_i = mb;
	unsigned char k_i[KEM_K_LEN];
	const unsigned char *buf = sta;
	const unsigned char *k_j = sta + SEED_BYTES;
	const unsigned char *ct_j = sta + SEED_BYTES + KEM_K_LEN;
	unsigned char pkI[PKI_LEN];
	unsigned char buf2[SEED_BYTES + SKI_LEN], tpk[TPK_LEN], tsk[TSK_LEN], seed_kg[SEED_BYTES];
	unsigned long long tpk_l=0, tsk_l=0, k_i_l=0;
	(void)pkb_len_bytes;(void)mb_len_bytes;


	memcpy(buf2, buf, SEED_BYTES);
	memcpy(buf2 + SEED_BYTES, ska, SKI_LEN);
	pseudohash(SEED_BYTES * 8, buf2, SEED_BYTES + SKI_LEN, seed_kg);
	if (twokem_keygen2_withseed(seed_kg, SEED_BYTES, tpk, &tpk_l, tsk, &tsk_l) != 0) {
        return -1;
    }
	if (twokem_dec(ska, ska_len_bytes, tsk, tsk_l, tpk, tpk_l, ct_i, C_I_LEN, k_i, &k_i_l) != 0) {
        return -2;
    }

	memcpy(pkI, ska + TSK_LEN, PKI_LEN);
	derive_session_key(ssa, tpk, pkI, pkb, ct_i, ct_j, k_i, KEM_K_LEN, k_j, KEM_K_LEN);
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
