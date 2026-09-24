#ifndef API_H
#define API_H


unsigned long long rand_get_sd_byts();
int rand_init(unsigned char * s, unsigned long long s_byts);
int rand_byts(unsigned long long r_byts, unsigned char * r);
unsigned long long kem_get_pk_byts();
unsigned long long kem_get_sk_byts();
unsigned long long kem_get_ss_byts();
unsigned long long kem_get_ct_byts();
int kem_keygen(unsigned char * pk, unsigned long long * pk_byts, unsigned char * sk, unsigned long long * sk_byts);
int kem_enc(unsigned char * pk, unsigned long long pk_byts,
unsigned char * ss, unsigned long long * ss_byts,
unsigned char * ct, unsigned long long * ct_byts);
int kem_dec(
unsigned char * sk, unsigned long long sk_byts,
unsigned char * ct, unsigned long long ct_byts,
unsigned char * ss, unsigned long long * ss_byts);
#endif
