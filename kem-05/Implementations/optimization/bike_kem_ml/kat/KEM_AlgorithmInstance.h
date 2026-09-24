/*
 * BIKE KEM adapter for the API_PKC KEM KAT driver.
 */

#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0

#ifndef BIKE_ALGORITHM_INSTANCE_NAME
#if defined(SECURITY_BITS) && (SECURITY_BITS == 512)
#define BIKE_ALGORITHM_INSTANCE_NAME "BIKEKEM_v2_512"
#elif defined(SECURITY_BITS) && (SECURITY_BITS == 256)
#define BIKE_ALGORITHM_INSTANCE_NAME "BIKEKEM_v2_256"
#elif defined(SECURITY_BITS) && (SECURITY_BITS == 192)
#define BIKE_ALGORITHM_INSTANCE_NAME "BIKEKEM_v2_192"
#else
#define BIKE_ALGORITHM_INSTANCE_NAME "BIKEKEM_v2_128"
#endif
#endif

#define ALGORITHM_INSTANCE BIKE_ALGORITHM_INSTANCE_NAME

#ifdef __cplusplus
extern "C" {
#endif

unsigned long long kem_get_pk_len_bytes(void);
unsigned long long kem_get_sk_len_bytes(void);
unsigned long long kem_get_ss_len_bytes(void);
unsigned long long kem_get_ct_len_bytes(void);

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes);

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif

#endif
