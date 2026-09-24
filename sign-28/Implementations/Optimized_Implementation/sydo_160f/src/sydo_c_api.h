#ifndef SYDO_C_API_H
#define SYDO_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned long long sydo_pk_capacity_c(void);
unsigned long long sydo_sk_capacity_c(void);
unsigned long long sydo_sn_capacity_c(void);

unsigned long long sydo_keygen_random_seed_bytes_c(void);
unsigned long long sydo_sign_random_seed_bytes_c(void);

int sydo_keygen_c(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes,
    const unsigned char *random_seed, unsigned long long random_seed_len);

int sydo_sign_c(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes,
    const unsigned char *random_seed, unsigned long long random_seed_len);

int sydo_verify_c(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes);

#ifdef __cplusplus
}
#endif

#endif
