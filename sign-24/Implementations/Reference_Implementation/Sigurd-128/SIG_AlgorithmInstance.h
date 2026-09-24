/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef SIG_ALGORITHM_INSTANCE_H
#define SIG_ALGORITHM_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0
/* Public instance name used by the KAT generator and submission tooling. */
#define ALGORITHM_INSTANCE "Sigurd128_REF"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Return the serialized public-key size in bytes for this parameter set.
 * Callers should allocate at least this many bytes before calling sig_keygen().
 */
unsigned long long sig_get_pk_len_bytes(void);
/*
 * Return the serialized secret-key size in bytes for this parameter set.
 * The secret key is exactly H_seed || e_seed.
 */
unsigned long long sig_get_sk_len_bytes(void);
/*
 * Return the maximum serialized signature buffer size in bytes.
 * Actual signatures may be shorter because Merkle proof overlap changes the
 * number of authentication hashes serialized for each sampled transcript.
 */
unsigned long long sig_get_sn_len_bytes(void);

/*
 * Generate one compact key pair using the template DRNG state.
 * `pk_len_bytes` and `sk_len_bytes` are written with the exact serialized sizes.
 */
int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);

/*
 * Sign the message and serialize the full non-interactive proof transcript.
 * The caller must provide an output buffer at least sig_get_sn_len_bytes() long.
 */
int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes);

/*
 * Verify a serialized signature transcript against the public key and message.
 * Returns 0 on acceptance and a negative value on parse, size, or proof failure.
 */
int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
