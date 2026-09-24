#ifndef NSS_HQC_CORE_H
#define NSS_HQC_CORE_H

#include "params.h"

#ifndef NSS_HQC_N_BYTES
#define NSS_HQC_N_BYTES ((NSS_HQC_N + 7u) / 8u)
#endif
#ifndef NSS_HQC_NC_BYTES
#define NSS_HQC_NC_BYTES ((NSS_HQC_NC + 7u) / 8u)
#endif
#ifndef NSS_HQC_CT_V_BYTES
#define NSS_HQC_CT_V_BYTES ((NSS_HQC_CT_V_BITS + 7u) / 8u)
#endif
#define NSS_HQC_VHAT_BYTES NSS_HQC_CT_V_BYTES
#define NSS_HQC_MSG_BYTES NSS_HQC_K1
#define NSS_HQC_MSG_BITS (8u * NSS_HQC_MSG_BYTES)
#define NSS_HQC_PK_SEED_BYTES NSS_HQC_SEED_PKE_BYTES
#ifndef NSS_HQC_SEED_SK_BYTES
#define NSS_HQC_SEED_SK_BYTES NSS_HQC_SEED_PKE_BYTES
#endif
#ifndef NSS_HQC_SIGMA_BYTES
#define NSS_HQC_SIGMA_BYTES NSS_HQC_SS_BYTES
#endif
#ifndef NSS_HQC_SK_BYTES
#define NSS_HQC_SK_BYTES (NSS_HQC_SEED_SK_BYTES + NSS_HQC_SIGMA_BYTES + NSS_HQC_PK_BYTES)
#endif
#ifndef NSS_HQC_K_BYTES
#define NSS_HQC_K_BYTES NSS_HQC_SS_BYTES
#endif
#ifndef NSS_HQC_THETA_BYTES
#define NSS_HQC_THETA_BYTES NSS_HQC_K_BYTES
#endif
#ifndef NSS_HQC_H_DIGEST_BYTES
#define NSS_HQC_H_DIGEST_BYTES NSS_HQC_K_BYTES
#endif
#ifndef NSS_HQC_J_BYTES
#define NSS_HQC_J_BYTES NSS_HQC_SS_BYTES
#endif
#ifndef NSS_HQC_G_OUTPUT_BYTES
#define NSS_HQC_G_OUTPUT_BYTES (NSS_HQC_K_BYTES + NSS_HQC_THETA_BYTES)
#endif
#define NSS_HQC_CT_FULL_BYTES NSS_HQC_CT_KEM_BYTES

int nss_hqc_keygen(unsigned char *pk, unsigned long long *pk_len,
                   unsigned char *sk, unsigned long long *sk_len);

int nss_hqc_enc(const unsigned char *pk, unsigned long long pk_len,
                unsigned char *ss, unsigned long long *ss_len,
                unsigned char *ct, unsigned long long *ct_len);

int nss_hqc_dec(const unsigned char *sk, unsigned long long sk_len,
                const unsigned char *ct, unsigned long long ct_len,
                unsigned char *ss, unsigned long long *ss_len);

int nss_hqc_kat_seed_entropy(const unsigned char *seed, unsigned long long seed_len);

void nss_hqc_shake256_public(unsigned char *out, unsigned long long out_len,
                             const unsigned char *in, unsigned long long in_len);

int nss_hqc_selftest_ring(void);
int nss_hqc_selftest_sample(void);
int nss_hqc_selftest_quant(void);
int nss_hqc_selftest_dither(void);
int nss_hqc_selftest_pke(void);
int nss_hqc_selftest_kem(void);

#endif


