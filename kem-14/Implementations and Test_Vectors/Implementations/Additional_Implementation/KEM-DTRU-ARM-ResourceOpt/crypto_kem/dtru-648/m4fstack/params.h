#ifndef PARAMS_H
#define PARAMS_H

#define DTRU_N 648

#define DTRU_Q 3457
#define DTRU_LOGQ 12

#define DTRU_Q2 512  /* Change this for the ciphertext modulus */
#define DTRU_LOGQ2 9 /* Change this for the ciphertext modulus */

#define DTRU_ETA 2 /* for compressed secret key */
#define DTRU_BOUND (2 * DTRU_ETA + 1) /* for uncompressed secret key */



#define DTRU_MSGBYTES 20 /* (DTRU_N / 4) bits = 162 bits -> 20 bytes */
#define DTRU_SEEDBYTES 32

#define SHA3512_OUTPUTBYTES 64
#define DTRU_Z_BYTES 32
#define DTRU_SHAREDKEYBYTES 32
#define DTRU_PREFIXHASHBYTES 33



#define DTRU_CBD2_BYTES (DTRU_N * 4 / 8)
#define DTRU_CBD9_BYTES (DTRU_N * 18 / 8)
#define DTRU_COINBYTES_KEYGEN (DTRU_CBD2_BYTES + DTRU_CBD9_BYTES)
#define DTRU_COINBYTES_ENC (DTRU_CBD2_BYTES + DTRU_CBD2_BYTES)


#ifndef PK_PACK_OPT
    #define PK_PACK_OPT 0
#endif

#if PK_PACK_OPT
    #define DTRU_PKE_PUBLICKEYBYTES 954
#else
    #define DTRU_PKE_PUBLICKEYBYTES (DTRU_N * DTRU_LOGQ / 8)
#endif

#define DTRU_PKE_CIPHERTEXTBYTES (DTRU_N * DTRU_LOGQ2 / 8)

#define DTRU_PKE_SECRETKEYBYTES (DTRU_N * 4 / 8) /* ceil(log2(4*eta+2)) = 4 */

#define DTRU_KEM_PUBLICKEYBYTES DTRU_PKE_PUBLICKEYBYTES
#define DTRU_KEM_SECRETKEYBYTES (DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES + DTRU_Z_BYTES)
#define DTRU_KEM_CIPHERTEXTBYTES DTRU_PKE_CIPHERTEXTBYTES

#endif