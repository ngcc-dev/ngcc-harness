/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

/* The official KAT harness reads these three macros to select output mode,
 * identify this instance, and size the fixed-length digest. */
// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
// Set "OUTPUT_BLANK_TEST_VECTORS" as 1 to generate blank template (default)
#define OUTPUT_BLANK_TEST_VECTORS 0

// Set "ALGORITHM_INSTANCE" as your algorithm instance name (no more than 64 bytes)
// Only letters, numbers, '-' or '_' are permitted
#define ALGORITHM_INSTANCE "Iphe-1024"

// Set "DIGEST_BIT_LENGTH" as the message digest length of your algorithm instance
#define DIGEST_BIT_LENGTH 1024

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Compute the fixed-length Iphe digest required by the official interface.
     *
     * @param[in] digest_len_bits Requested digest length in bits; it must equal
     *                           DIGEST_BIT_LENGTH for this instance.
     * @param[in] msg             Message buffer. A null pointer is accepted only
     *                           when msg_len_bits is zero.
     * @param[in] msg_len_bits    Exact message length in bits, so the last byte
     *                           may be only partially significant.
     * @param[out] digest         Caller-provided output buffer of at least
     *                           DIGEST_BIT_LENGTH / 8 bytes.
     * @return 0 on success, or -1 for an invalid pointer or digest length.
     *
     * The function has no persistent side effects; all state is local to the
     * call and the output buffer is written only for a valid request.
     */
    int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif
#endif
