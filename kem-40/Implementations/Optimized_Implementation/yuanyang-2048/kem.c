/*
 * Falcon3-512 signing.
 *
 * This follows the current Python reference online path:
 *   1. decode the secret key and derive B_hat_inv from B and u_hat,
 *   2. hash m || pk once, then hash salt || seed to the challenge point,
 *   3. sample a nearby lattice point with the hybrid sampler using
 *      A_hat, derived B_hat_inv, T and u_hat,
 *   4. emit salt || compress(s1), where s1 is the first signature half.
 *
 * The one-dimensional Gaussian sampler now follows Falcon's
 * gaussian0_sampler/BerExp/sampler structure, adapted to Falcon3's
 * table-specific sigmas and without the sigma_min/sigma ccs factor.
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "auxfunc.h"
#include "drng.h"
#include "codec.h"
#include "yy_kem_inner.h"
#include "ntt.h"
#include "ntttable.h"
#include "prng.h"

#define M_PI acos(-1)
#define YY_SEC YUANYANG_D/4

extern DRNG_ctx drng_algorithm;

const int SCALETAB[]={3,4,4};
const double SIGMASTAB[]={443/256.,362/256.,362/256.};
const double SIGMAETAB[]={404/256.,323/256.,1};
const int ENCODETAB[]={1,4,1}; // encode blocks of K in ciphertext


size_t yy_get_ciphertext_bytes(){
	int id=YUANYANG_LOGD-9;
	return ceil(log((YUANYANG_Q-1.)/SCALETAB[id]+1)*ENCODETAB[id]/log(2))*YUANYANG_D/ENCODETAB[id]/8+YY_SEC/8;
}

size_t yy_get_pk_len_bytes(){
	return ceil(log(YUANYANG_Q)*4/log(2))*YUANYANG_D/4/8;
}

size_t yy_get_sk_len_bytes(){
	return 3*YUANYANG_D/2+YY_SEC/8*2+yy_get_pk_len_bytes(); // f followed by finvint,H(pk),K',pk
}

size_t yy_get_ss_len_bytes(){
	return YUANYANG_D/32;
}

static int
drng_randombytes(void *ctx, unsigned char *buf, unsigned long long len_bytes)
{
	return get_random_number((DRNG_ctx *)ctx, buf, 8u * len_bytes);
}

static int
ring_samp(
	int16_t s[YUANYANG_D],int16_t e[YUANYANG_D],
	prng *rng)
{
#if YUANYANG_LOGD==9
	const uint8_t sgauss[]={30, 79, 109, 123, 127};
	const uint8_t egauss[]={32, 85, 114, 125, 128};
#elif YUANYANG_LOGD==10
	const uint8_t sgauss[]={36, 92, 119, 127, 128};
	const uint8_t egauss[]={40, 99, 122, 127, 128};
#elif YUANYANG_LOGD==11
	const uint8_t sgauss[]={36, 92, 119, 127, 128};
	const uint8_t egauss[]={51, 113, 127, 128, 128};
#endif
	for (size_t u = 0; u < YUANYANG_D; u++) {
		uint8_t random=prng_get_u8(rng);
		int sign=random&1,value=0;
		random>>=1;
		for(size_t i=0;i<sizeof(sgauss)/sizeof(*sgauss);i++)
			value+=sgauss[i]<=random;
		s[u]=sign? value : -value;
	}
	for (size_t u = 0; u < YUANYANG_D; u++) {
		uint8_t random=prng_get_u8(rng);
		int sign=1-2*(random&1),value=0;
		random>>=1;
		for(size_t i=0;i<sizeof(egauss)/sizeof(*egauss);i++)
			value+=egauss[i]<=random;
		e[u]=value*sign;
	}
	return prng_status(rng);
}

static int centered_mod(int x, int q) {
    int r = x % q;
    r=(r+q)%q;
    return r-(r>q/2)*q;
}

static int yy_encrypt(
	uint16_t output[YUANYANG_D+YY_SEC/16],const uint8_t seed[YUANYANG_D/32+1],
	const uint8_t message[YUANYANG_D/32],const uint16_t h[YUANYANG_D]){
	int16_t s[YUANYANG_D],e[YUANYANG_D];
	uint64_t blindmessage[YY_SEC/64+1];
	DRNG_ctx drng;
	init_random_number(&drng,seed,1+YY_SEC/8<SEEDLEN ? 1+YY_SEC/8 : SEEDLEN);
	prng rng;
	prng_init(&rng,drng_randombytes,&drng);
	if(prng_status(&rng)!=YUANYANG_SUCCESS)
		return prng_status(&rng);
	for(unsigned int i=0;i<YY_SEC/64;i++){
		blindmessage[i]=0;
	}
	get_random_number(&drng, (unsigned char*)blindmessage,YY_SEC);
	ring_samp(s,e,&rng);
	yuanyang_mul_mod_xn_plus_1_ntt_big(output,h,s);
	int sc=SCALETAB[YUANYANG_LOGD-9],q=YUANYANG_Q;
	unsigned int half=YUANYANG_D/2;
	for(unsigned int i=0;i<YUANYANG_D;i++){
		int t=i&(half/2) ? 0 : ((blindmessage[(i%half)/64]>>(i%64))&1)*(q/2+1)*(1-(i>=half)*2);
		t+=output[i];
		output[i]=((t%q+q+sc/2+1-sc%2)%q)/sc;
	}
	((uint8_t*)blindmessage)[YY_SEC/8]=44;
	uint8_t hashoutput[512/8];
	pseudohash(512,(unsigned char*)blindmessage,YY_SEC+8,(unsigned char*)hashoutput);
	for(unsigned int i=0;i<YY_SEC/8;i++){
		((uint8_t*)(output+YUANYANG_D))[i]=(message)[i]^hashoutput[i];
	}
	return YY_SUCCESS;
}

static int yy_encode_message(
	uint8_t *ct, unsigned long long *ct_len_bytes,const uint8_t seed[YY_SEC/8+1],
	const uint8_t message[YY_SEC/8],const uint16_t h[YUANYANG_D]){
	uint16_t output[YUANYANG_D+YY_SEC/16];
	yy_encrypt(output,seed,message,h);
	yuanyang_encode_uniform(output,YUANYANG_D,(YUANYANG_Q-1)/SCALETAB[YUANYANG_LOGD-9]+2,ENCODETAB[YUANYANG_LOGD-9],ct);
	int k=yy_get_ciphertext_bytes()-YY_SEC/8;
	memcpy(ct+k,output+YUANYANG_D,YY_SEC/8);
	*ct_len_bytes=k+YY_SEC/8;
	return YY_SUCCESS;
}


int yy_encapsulate_API(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes){
	uint16_t h[YUANYANG_D];
	yuanyang_decode_uniform(h,YUANYANG_D,YUANYANG_Q,4,pk);
	uint8_t msg[YY_SEC/8];
	drng_randombytes(&drng_algorithm,msg,YY_SEC/8);
	uint8_t tmphash[3000]; // YY_SEC/8+1+ctsize
	memcpy(tmphash,msg,YY_SEC/8);
	pseudohash(512,pk,pk_len_bytes*8,tmphash+YY_SEC/8); // tmphash=msg||Hash(pk)||42
	tmphash[2*YY_SEC/8]=42;
	pseudohash(1024,tmphash,2*YY_SEC+8,tmphash); // tmphash=H(msg||Hash(pk)||42)=K||seed
	tmphash[2*YY_SEC/8]=45;
	yy_encode_message(ct,ct_len_bytes,tmphash+YY_SEC/8,msg,h);
	memcpy(tmphash+YY_SEC/8,ct,*ct_len_bytes);
	tmphash[YY_SEC/8+*ct_len_bytes]=43;
	pseudohash(512,tmphash,YY_SEC+(*ct_len_bytes)*8+8,tmphash);
	*ss_len_bytes=YY_SEC/8;
	memcpy(ss,tmphash,YY_SEC/8);
	return 0;
}

int yy_decrypt_ciphertext(
	uint8_t message[YUANYANG_D/32],
	const unsigned char *ct, unsigned long long ct_len_bytes, yy_kem_expanded_sk *sk){
	int q=YUANYANG_Q;
	int scale=SCALETAB[YUANYANG_LOGD-9];
	uint8_t blindmessage[YY_SEC/8+1];
	uint16_t decode[YUANYANG_D];
	yuanyang_decode_uniform(decode,YUANYANG_D,(q-1)/scale+2,ENCODETAB[YUANYANG_LOGD-9],ct);
	for(unsigned i=0;i<YUANYANG_D;i++){
		int coeff=decode[i];
		sk->tmp[i]=coeff*scale;
	}
	qntt_forward(sk->tmp);
	qntt_mul(sk->tmp,sk->f);
	int h=YUANYANG_D/2;
	for(int i=0;i<h;i++){
		int a=sk->tmp[i  ];
		int b=sk->tmp[i+h]; // we multiply by (a+bx^h)*(1+x^h)
		int c=centered_mod(a-b-sk->fmbias[i],q)+sk->fmbias[i];
		int d=centered_mod(a+b-sk->fmbias[i+h],q)+sk->fmbias[i+h];
//		if(i==35) c-=q; // For ECC testing
		sk->tmp[i]=(c^d)&1; // we reduce mod (1+x^h) after a centered mod
	}
	memset(sk->tmp+h,0,h*4);
	qntt_forward(sk->tmp);
	qntt_mul_int(sk->tmp,sk->finv);
	memset(blindmessage,0,YUANYANG_D/32);
	for(int i=0;i<h/2;i++){
		int res=sk->tmp[i]-sk->tmp[i+h];
		blindmessage[i/8]|=(res&1)<<(i%8);
	}
	int hamming=0;
	for(int i=0;i<h/2;i++){
		int b=(sk->tmp[i+h/2]+sk->tmp[i+h+h/2])&1;
		sk->tmp[h/2-i]=b; // conjugate(high part*x^h)
		hamming+=b;
	}
	sk->tmp[0]=0; // tmp+=conjugate((high part*x^(h/2)%x^(h-1)))
	memset(sk->tmp+h/2+1,0,(2*h-h/2-1)*4);
	qntt_forward(sk->tmp);
	qntt_mul_int(sk->tmp,sk->finv);
	int overflow=0;
	for(int i=1;i<h;i++){
		int32_t dotp=sk->tmp[h-i]+sk->tmp[2*h-i];
		int ok=dotp==hamming;
		overflow|=ok*i;
	}
	int mul=hamming!=0; // Now error is mul*f*x^overflow
	for(int i=0;i<h/2;i++){
		int b=0;
		b^=(mul*(sk->finvint[(i-overflow+h)%h])%2)&1;
		blindmessage[i/8]=blindmessage[i/8] ^ (b<<(i%8));
	}
	
	uint8_t hashoutput[512/8];
	blindmessage[YY_SEC/8]=44;
	pseudohash(512,blindmessage,YY_SEC+8,(unsigned char*)hashoutput);
	for(unsigned int i=0;i<YY_SEC/8;i++){
		message[i]=(ct+ct_len_bytes-YY_SEC/8)[i]^hashoutput[i];
	}
	return 0;
}

int yy_decapsulate_API(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes){
	if(ct_len_bytes!=yy_get_ciphertext_bytes() || sk_len_bytes!=yy_get_sk_len_bytes())
		return -1;
	uint8_t message[YY_SEC/8];
	yy_kem_expanded_sk skexp;

	memcpy(skexp.compact.f,sk,YUANYANG_D);
	memcpy(skexp.finvint,sk+YUANYANG_D,YUANYANG_D/2);
	yy_kem_expand_private_key(&skexp,&skexp.compact);
	yy_decrypt_ciphertext(message,ct,ct_len_bytes,&skexp);
	uint8_t tmphash[4096]; // YY_SEC/8+BTAB[YUANYANG_LOGD-9]*YUANYANG_D/8
	memcpy(tmphash,message,YY_SEC/8);
	memcpy(tmphash+YY_SEC/8,sk+3*YUANYANG_D/2,YY_SEC/8);// tmphash=msg||Hash(pk)||42
	tmphash[YY_SEC/4]=42;
	pseudohash(1024,tmphash,2*YY_SEC+8,tmphash); // tmphash=H(msg||Hash(pk))=K||seed
	uint8_t *pk=sk+3*YUANYANG_D/2+YY_SEC/8*2;
	uint16_t h[YUANYANG_D];
	yuanyang_decode_uniform(h,YUANYANG_D,YUANYANG_Q,4,pk);
	unsigned long long ct_b;
	tmphash[2*YY_SEC/8]=45;
	yy_encode_message(tmphash+YY_SEC/8,&ct_b,tmphash+YY_SEC/8,message,h);
	int ok=1;
	for(unsigned long long i=0;i<ct_b;i++){
		ok=ok & (tmphash[i+YY_SEC/8]==ct[i]);
	}
	for(unsigned int i=0;i<YY_SEC/8;i++){
		int Kp=sk[3*YUANYANG_D/2+YY_SEC/8+i];
		int Kbar=tmphash[i];
		tmphash[i]=ok*Kbar+(1-ok)*Kp;
	}
	memcpy(tmphash+YY_SEC/8,ct,ct_len_bytes);
	tmphash[YY_SEC/8+ct_len_bytes]=43;
	pseudohash(512,tmphash,YY_SEC+(ct_len_bytes)*8+8,tmphash);
	memcpy(ss,tmphash,YY_SEC/8);
	*ss_len_bytes=YY_SEC/8;
	return 0;
}


int yy_kem_expand_private_key(
	yy_kem_expanded_sk *expanded,
	const yy_kem_compact_sk *compact){
	for(size_t i=0;i<YUANYANG_D;i++){
		expanded->f[i]=compact->f[i]<0 ? QNTT+(compact->f[i]) : (unsigned)compact->f[i];
	}
	for(unsigned int i=0;i<YUANYANG_D/2;i++){
		expanded->finv[i]=expanded->finvint[i];
	}
	memset(expanded->finv+YUANYANG_D/2,0,YUANYANG_D/2*sizeof(*expanded->finv));
	qntt_forward(expanded->finv);
	for(unsigned int i=0;i<YUANYANG_D;i++)
		expanded->tmp[i]=i<YUANYANG_D/4+(1-SCALETAB[YUANYANG_LOGD-9]%2)*2*(i>=YUANYANG_D/2);
	qntt_forward(expanded->tmp);
	qntt_forward(expanded->f);
	qntt_mul_int(expanded->tmp,expanded->f);
	for(unsigned int i=0;i<YUANYANG_D;i++){
		expanded->fmbias[i]=expanded->tmp[i]/2;
	}
	return YY_SUCCESS;
}
