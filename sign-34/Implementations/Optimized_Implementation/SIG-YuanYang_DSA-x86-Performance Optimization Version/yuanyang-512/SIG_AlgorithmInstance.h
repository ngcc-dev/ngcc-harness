/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/


/*
 * yuanyang-512 reference signature API.
 */

#ifndef YUANYANG_512_SIG_ALGORITHM_INSTANCE_H
#define YUANYANG_512_SIG_ALGORITHM_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "yuanyang-512"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Return the exact number of bytes required for an encoded public key.
 */
unsigned long long sig_get_pk_len_bytes(void);

/*
 * Return the exact number of bytes required for an encoded secret key.
 */
unsigned long long sig_get_sk_len_bytes(void);

/*
 * Return the exact number of bytes required for an encoded signature.
 */
unsigned long long sig_get_sn_len_bytes(void);

/*
 * Generate a YuanYang signature key pair.  pk_len_bytes and sk_len_bytes are
 * set to the encoded lengths on entry when the pointers are non-NULL; on
 * success, pk and sk contain the encoded public and secret keys.
 */
int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes);

/*
 * Sign message m with encoded secret key sk.  On success, sn contains the
 * encoded signature and sn_len_bytes is set to sig_get_sn_len_bytes().
 */
int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes);

/*
 * Verify encoded signature sn on message m under encoded public key pk.
 * Returns 0 for a valid signature, -1 for an invalid signature, and another
 * negative YuanYang error code for malformed inputs or internal failures.
 */
int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes);

#ifdef __cplusplus
}
#endif

#endif
