#ifndef API_H
#define API_H

#define CRYPTO_ALGNAME "mlwr_medium"

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes);
int sig_sign(const unsigned char *sk, unsigned long long sk_len_bytes, const unsigned char *m, unsigned long long m_len_bytes, unsigned char *sn, unsigned long long *sn_len_bytes);
int sig_verify(const unsigned char *pk, unsigned long long pk_len_bytes, const unsigned char *sn, unsigned long long sn_len_bytes,unsigned char *m, unsigned long long m_len_bytes);

#endif
