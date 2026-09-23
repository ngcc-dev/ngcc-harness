#ifndef LOONG_KEM_H
#define LOONG_KEM_H

#include "loong_parameters.h"

#define LOONG_KEM_G_SECRET_BYTES LOONG_G_BYTES
#define LOONG_KEM_THETA_BYTES LOONG_G_BYTES

int loong_kem_keygen(unsigned char *pk, unsigned long long pk_len_bytes,
                     unsigned char *sk, unsigned long long sk_len_bytes);

int loong_kem_encapsulate(const unsigned char *pk,
                          unsigned long long pk_len_bytes,
                          unsigned char *ss,
                          unsigned long long ss_len_bytes,
                          unsigned char *ct,
                          unsigned long long ct_len_bytes);

int loong_kem_decapsulate(const unsigned char *sk,
                          unsigned long long sk_len_bytes,
                          const unsigned char *ct,
                          unsigned long long ct_len_bytes,
                          unsigned char *ss,
                          unsigned long long ss_len_bytes);

#endif
