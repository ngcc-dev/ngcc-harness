#ifndef NGCC_API_H
#define NGCC_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* KEX pass1 function (8 params, no input message — pass1 uses pk of the other party directly). */
typedef int (*kex_pass1_fn_t)(unsigned char *sk, unsigned long long sk_len,
                              unsigned char *pk, unsigned long long pk_len,
                              unsigned char *st, unsigned long long *st_len,
                              unsigned char *m_out, unsigned long long *m_out_len);

/* KEX pass2+ function (10 params, with input message from previous pass). */
typedef int (*kex_pass_fn_t)(unsigned char *sk, unsigned long long sk_len,
                             unsigned char *pk, unsigned long long pk_len,
                             unsigned char *m_in, unsigned long long m_in_len,
                             unsigned char *st, unsigned long long *st_len,
                             unsigned char *m_out, unsigned long long *m_out_len);

#define NGCC_KEX_MIN_PASSES 2ULL
#define NGCC_KEX_MAX_PASSES 100ULL

typedef struct {
    int (*CryptHash)(int digest_len_bits,
                     const unsigned char *msg,
                     unsigned long long msg_len_bits,
                     unsigned char *digest);

    unsigned long long (*sig_get_pk_len_bytes)(void);
    unsigned long long (*sig_get_sk_len_bytes)(void);
    unsigned long long (*sig_get_sn_len_bytes)(void);
    int (*sig_keygen)(unsigned char *pk, unsigned long long *pk_len_bytes,
                      unsigned char *sk, unsigned long long *sk_len_bytes);
    int (*sig_sign)(unsigned char *sk, unsigned long long sk_len_bytes,
                    unsigned char *m, unsigned long long m_len_bytes,
                    unsigned char *sn, unsigned long long *sn_len_bytes);
    int (*sig_verify)(unsigned char *pk, unsigned long long pk_len_bytes,
                      unsigned char *sn, unsigned long long sn_len_bytes,
                      unsigned char *m, unsigned long long m_len_bytes);

    unsigned long long (*kem_get_pk_len_bytes)(void);
    unsigned long long (*kem_get_sk_len_bytes)(void);
    unsigned long long (*kem_get_ss_len_bytes)(void);
    unsigned long long (*kem_get_ct_len_bytes)(void);
    int (*kem_keygen)(unsigned char *pk, unsigned long long *pk_len_bytes,
                      unsigned char *sk, unsigned long long *sk_len_bytes);
    int (*kem_enc)(unsigned char *pk, unsigned long long pk_len_bytes,
                   unsigned char *ss, unsigned long long *ss_len_bytes,
                   unsigned char *ct, unsigned long long *ct_len_bytes);
    int (*kem_dec)(unsigned char *sk, unsigned long long sk_len_bytes,
                   unsigned char *ct, unsigned long long ct_len_bytes,
                   unsigned char *ss, unsigned long long *ss_len_bytes);

    unsigned long long (*kex_get_passes_num)(void);
    unsigned long long (*kex_get_pk_len_bytes)(void);
    unsigned long long (*kex_get_sk_len_bytes)(void);
    unsigned long long (*kex_get_sta_len_bytes)(void);
    unsigned long long (*kex_get_stb_len_bytes)(void);
    unsigned long long (*kex_get_ss_len_bytes)(void);
    unsigned long long (*kex_get_total_msg_len_bytes)(void);

    int (*kex_init_a)(unsigned char *pka, unsigned long long *pka_len_bytes,
                      unsigned char *ska, unsigned long long *ska_len_bytes,
                      unsigned char *sta, unsigned long long *sta_len_bytes);
    int (*kex_init_b)(unsigned char *pkb, unsigned long long *pkb_len_bytes,
                      unsigned char *skb, unsigned long long *skb_len_bytes,
                      unsigned char *stb, unsigned long long *stb_len_bytes);

    unsigned long long kex_passes_num;   /* cached from kex_get_passes_num() */
    kex_pass1_fn_t kex_pass1_fn;         /* pass1 function (always A-side) */
    kex_pass_fn_t *kex_pass_fns;         /* array[0..passes_num-2] for pass2..N via dlsym */

    int (*kex_derive_ss_a)(unsigned char *ska, unsigned long long ska_len_bytes,
                           unsigned char *pkb, unsigned long long pkb_len_bytes,
                           unsigned char *mb, unsigned long long mb_len_bytes,
                           unsigned char *sta, unsigned long long sta_len_bytes,
                           unsigned char *ssa, unsigned long long *ssa_len_bytes);

    int (*kex_derive_ss_b)(unsigned char *skb, unsigned long long skb_len_bytes,
                           unsigned char *pka, unsigned long long pka_len_bytes,
                           unsigned char *ma, unsigned long long ma_len_bytes,
                           unsigned char *stb, unsigned long long stb_len_bytes,
                           unsigned char *ssb, unsigned long long *ssb_len_bytes);
} ngcc_api_t;

#ifdef __cplusplus
}
#endif

#endif
