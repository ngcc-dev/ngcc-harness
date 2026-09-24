/*
 * API_PKC SIG Interface header
 * QingLuan digital signature adapter
 *
 * NOTE: drng.c, auxfunc.c, KAT_SIG.c are NOT modified (per framework rules).
 */

#ifndef SIG_ALGORITHM_INSTANCE_H
#define SIG_ALGORITHM_INSTANCE_H

/* 0 = produce filled KAT vectors; 1 = blank template */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* Algorithm instance name: QingLuan-128 | QingLuan-256 | QingLuan-384 | QingLuan-512
 * Selected by the compile-time QINGLUAN_128/256/384/512 macro. An unhandled
 * level is a hard error (never silently mislabel the KAT output filename).
 */
#if defined(QINGLUAN_512)
#define ALGORITHM_INSTANCE "QingLuan-512"
#elif defined(QINGLUAN_384)
#define ALGORITHM_INSTANCE "QingLuan-384"
#elif defined(QINGLUAN_256)
#define ALGORITHM_INSTANCE "QingLuan-256"
#elif defined(QINGLUAN_128)
#define ALGORITHM_INSTANCE "QingLuan-128"
#else
#error "No QINGLUAN_<level> macro defined for ALGORITHM_INSTANCE"
#endif

#ifdef __cplusplus
extern "C" {
#endif

unsigned long long sig_get_pk_len_bytes(void);
unsigned long long sig_get_sk_len_bytes(void);
unsigned long long sig_get_sn_len_bytes(void);

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m,  unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes);

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m,  unsigned long long m_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
