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
	unsigned char in[TPK_LEN + PKI_LEN + PKR_LEN + C_TILDE_LEN + C_R_LEN + KEM_K_LEN + KEM_K_LEN];
	unsigned long long off = 0;
	memcpy(in + off, tpk, TPK_LEN); off += TPK_LEN;
	memcpy(in + off, pkI, PKI_LEN); off += PKI_LEN;
	memcpy(in + off, pkR, PKR_LEN); off += PKR_LEN;
	memcpy(in + off, ct_tilde, C_TILDE_LEN); off += C_TILDE_LEN;
	memcpy(in + off, ct_i, C_R_LEN); off += C_R_LEN;
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
	return SEED_BYTES + C_R_LEN + KEM_K_LEN; 
}

unsigned long long kex_get_stb_len_bytes(){ 
	return K_LEN; 
}

unsigned long long kex_get_ss_len_bytes(){ 
	return K_LEN; 
}

unsigned long long kex_get_total_msg_len_bytes(){ 
	return TPK_LEN + C_R_LEN + SIG_LEN + C_TILDE_LEN; 
}

int kex_init_a(unsigned char *pka,unsigned long long *pka_len_bytes,unsigned char *ska,unsigned long long *ska_len_bytes,unsigned char *sta,unsigned long long *sta_len_bytes){
	(void)sta;
	sig_keygen(pka,pka_len_bytes,ska,ska_len_bytes);
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
	(void)ska;(void)pkb;(void)pkb_len_bytes;(void)sta_len_bytes;
	unsigned long long l_ct, l_ss, l_sig;
	unsigned char seed_kg[SEED_BYTES], buf[SEED_BYTES + SKI_LEN], tsk[TSK_LEN], k_j[KEM_K_LEN];
	unsigned char *tpk = m1;
	unsigned char *ct_j = m1 + TPK_LEN;
	unsigned char *sig = m1 + TPK_LEN + C_R_LEN;
	int ret;
	if (get_random_number(&drng_algorithm, buf, SEED_BYTES * 8ULL) != 0) {
        return -1;
    }
	memcpy(buf + SEED_BYTES, ska, SKI_LEN);
	pseudohash(SEED_BYTES * 8, buf, SEED_BYTES + SKI_LEN, seed_kg);
	ret = PKE_KeyGen(tpk, tsk, seed_kg);
    if (ret != 0) {
        return -1;
    }
	ret = kem_enc(pkb, PKR_LEN, k_j, &l_ss, ct_j, &l_ct);
	if (ret != 0) {
        return -1;
    }
	sig_sign(ska, ska_len_bytes, m1, TPK_LEN + C_R_LEN, sig, &l_sig);
	*m1_len_bytes = TPK_LEN + C_R_LEN + l_sig;

	memcpy(sta, buf, SEED_BYTES); 
	*sta_len_bytes = SEED_BYTES;
	memcpy(sta + *sta_len_bytes, ct_j, l_ct); 
	*sta_len_bytes = *sta_len_bytes + l_ct;
	memcpy(sta + *sta_len_bytes, k_j, l_ss);
	*sta_len_bytes = *sta_len_bytes + l_ss;

	return 0;
}

int kex_generate_pass2_msg_b(unsigned char *skb,unsigned long long skb_len_bytes,unsigned char *pka,unsigned long long pka_len_bytes,unsigned char *m1,unsigned long long m1_len_bytes,unsigned char *stb,unsigned long long *stb_len_bytes,unsigned char *m2,unsigned long long *m2_len_bytes){
	unsigned char *tpk = m1;
	unsigned char *ct_j = m1 + TPK_LEN;
	unsigned char *sig = m1 + TPK_LEN + C_R_LEN;
	unsigned char *ct_tilde = m2;
	unsigned char pkb[PKR_LEN];
	unsigned long long kem_k_len=0;
	
	unsigned char k_tilde[KEM_K_LEN], k_j[KEM_K_LEN];
	unsigned char buf[SEED_BYTES], buf2[MSG_LEN_BYTES + SEED_BYTES];
	unsigned char seed_enc[SEED_BYTES];
	(void)pka_len_bytes;(void)m1_len_bytes;

	if(sig_verify(pka, PKI_LEN, sig, SIG_LEN, m1, TPK_LEN + C_R_LEN) != 0){
		return -1;
	}

	/*if (get_random_number(&drng_algorithm, buf, MSG_LEN_BYTES * 8ULL) != 0) {
        return -1;
    }
    if (get_random_number(&drng_algorithm, seed_enc, SEED_BYTES * 8ULL) != 0) {
        return -1;
    }*/
	if (get_random_number(&drng_algorithm, buf, SEED_BYTES * 8ULL) != 0) {
        return -1;
    }
	pseudoXOF((MSG_LEN_BYTES + SEED_BYTES)*8, buf, SEED_BYTES, buf2);
	memcpy(k_tilde, buf2, MSG_LEN_BYTES);
	memcpy(seed_enc, buf2 + MSG_LEN_BYTES, SEED_BYTES);
	PKE_Encrypt(ct_tilde, tpk, k_tilde, seed_enc);

	kem_dec(skb, skb_len_bytes, ct_j, C_R_LEN, k_j, &kem_k_len);
	
	*m2_len_bytes = C_TILDE_LEN;

	memcpy(pkb, skb + TSK_LEN, PKR_LEN);
	derive_session_key(stb, tpk, pka, pkb, ct_tilde, ct_j, k_tilde, KEM_K_LEN, k_j, KEM_K_LEN);
	*stb_len_bytes = K_LEN;

	return 1;
}

int kex_derive_ss_a(unsigned char *ska,unsigned long long ska_len_bytes,unsigned char *pkb,unsigned long long pkb_len_bytes,unsigned char *mb,unsigned long long mb_len_bytes,unsigned char *sta,unsigned long long sta_len_bytes,unsigned char *ssa,unsigned long long *ssa_len_bytes){
	unsigned char *ct_tilde = mb;
	unsigned char *buf = sta;
	unsigned char *ct_j = sta + SEED_BYTES;
	unsigned char *k_j = sta + SEED_BYTES + C_R_LEN;
	unsigned char buf2[SEED_BYTES + SKI_LEN];

	unsigned char k_tilde[KEM_K_LEN];
	unsigned char tpk[TPK_LEN], tsk[TSK_LEN];
	unsigned char pkI[PKI_LEN];
	unsigned char seed_kg[SEED_BYTES];
	int ret;
	(void)pkb_len_bytes;(void)mb_len_bytes;(void)ska_len_bytes;

	memcpy(buf2, buf, SEED_BYTES);
	memcpy(buf2 + SEED_BYTES, ska, SKI_LEN);
	pseudohash(SEED_BYTES * 8, buf2, SEED_BYTES + SKI_LEN, seed_kg);
	ret = PKE_KeyGen(tpk, tsk, seed_kg);
    if (ret != 0) {
        return -1;
    }
	PKE_Decrypt(k_tilde, ct_tilde, tsk);

	memcpy(pkI, ska, PKI_LEN);
	derive_session_key(ssa, tpk, pkI, pkb, ct_tilde, ct_j, k_tilde, KEM_K_LEN, k_j, KEM_K_LEN);
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
