#ifndef SIGN_H
#define SIGN_H

#include "params.h"
#include "poly.h"
#include "polyvec.h"

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "lwrdsa512"


unsigned long long sig_get_pk_len_bytes(void);
unsigned long long sig_get_sk_len_bytes(void);
unsigned long long sig_get_sn_len_bytes(void);

void expand_mat(polyvecl mat[K], const unsigned char rho[SEEDBYTES]);
void challenge(poly *c, const unsigned char mu[CRHBYTES],
               const polyveck *w1);

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes);

int sig_sign(const unsigned char *sk, unsigned long long sk_len_bytes, const unsigned char *m, unsigned long long m_len_bytes, unsigned char *sn, unsigned long long *sn_len_bytes);
int sig_verify(const unsigned char *pk, unsigned long long pk_len_bytes, const unsigned char *sn, unsigned long long sn_len_bytes,unsigned char *m, unsigned long long m_len_bytes);

#endif
