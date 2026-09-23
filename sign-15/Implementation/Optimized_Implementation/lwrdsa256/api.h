#ifndef API_H
#define API_H

//#define CRYPTO_PUBLICKEYBYTES 2848U
//32+(N/8)*K*8
//#define CRYPTO_SECRETKEYBYTES 5552U
//32+32+48+(N/8)*L*SETABITS+(N/8)*K*D
//#define CRYPTO_BYTES 4656U
//((N/8)*K*(GAMMA1_BITS+1)+OMEGA+N/8+8) //2044U

#define CRYPTO_ALGNAME "mlwr_medium"

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes);

int sig_sign(const unsigned char *sk, unsigned long long sk_len_bytes, const unsigned char *m, unsigned long long m_len_bytes, unsigned char *sn, unsigned long long *sn_len_bytes);
int sig_verify(const unsigned char *pk, unsigned long long pk_len_bytes, const unsigned char *sn, unsigned long long sn_len_bytes,unsigned char *m, unsigned long long m_len_bytes);
#endif
