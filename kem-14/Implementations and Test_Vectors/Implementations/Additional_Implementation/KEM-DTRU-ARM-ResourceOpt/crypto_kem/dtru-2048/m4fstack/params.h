#ifndef PARAMS_H
#define PARAMS_H

#define DTRU_N 2048

#define DTRU_Q 3457
#define DTRU_LOGQ 12

#define DTRU_Q2 1024  /* Change this for the ciphertext modulus */
#define DTRU_LOGQ2 10 /* Change this for the ciphertext modulus */

#define DTRU_ETA 1 /* for compressed secret key */
#define DTRU_BOUND (2 * DTRU_ETA + 1) /* for uncompressed secret key */



#define DTRU_MSGBYTES 64 /* (DTRU_N / 4) bits = 512 bits = 64 bytes */
#define DTRU_SEEDBYTES 64

#define SHA3512_OUTPUTBYTES 64
#define DTRU_Z_BYTES 64
#define DTRU_SHAREDKEYBYTES 64
#define DTRU_PREFIXHASHBYTES 65



#define DTRU_CBD1_BYTES (DTRU_N * 2 / 8)
#define DTRU_CBD2_BYTES (DTRU_N * 4 / 8)
#define DTRU_CBD5_BYTES (DTRU_N * 10 / 8)
#define DTRU_COINBYTES_KEYGEN (DTRU_CBD1_BYTES + DTRU_CBD5_BYTES)
#define DTRU_COINBYTES_ENC (DTRU_CBD2_BYTES + DTRU_CBD1_BYTES)


#ifndef PK_PACK_OPT
    #define PK_PACK_OPT 0
#endif

#if PK_PACK_OPT
    #define DTRU_PKE_PUBLICKEYBYTES 3012
#else
    #define DTRU_PKE_PUBLICKEYBYTES (DTRU_N * DTRU_LOGQ / 8)
#endif

#define DTRU_PKE_CIPHERTEXTBYTES (DTRU_N * DTRU_LOGQ2 / 8)

#define DTRU_PKE_SECRETKEYBYTES (DTRU_N * 3 / 8) /* ceil(log2(4*eta+2)) = 4 */

#define DTRU_KEM_PUBLICKEYBYTES DTRU_PKE_PUBLICKEYBYTES
#define DTRU_KEM_SECRETKEYBYTES (DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES + DTRU_Z_BYTES)
#define DTRU_KEM_CIPHERTEXTBYTES DTRU_PKE_CIPHERTEXTBYTES

#endif