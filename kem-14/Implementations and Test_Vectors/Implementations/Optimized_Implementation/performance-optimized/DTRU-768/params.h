#ifndef PARAMS_H
#define PARAMS_H

#define DTRU_N 768

#define DTRU_Q 3457
#define DTRU_LOGQ 12

#define DTRU_Q2 1024  /* Change this for the ciphertext modulus */
#define DTRU_LOGQ2 10 /* Change this for the ciphertext modulus */

#define DTRU_ETA 3 /* for compressed secret key */
#define DTRU_BOUND (2 * DTRU_ETA + 1) /* for uncompressed secret key */



#define DTRU_MSGBYTES 24 /* (DTRU_N / 4) bits = 192 bits = 24 bytes */
#define DTRU_SEEDBYTES 32

#define SHA3512_OUTPUTBYTES 64
#define DTRU_Z_BYTES 32
#define DTRU_SHAREDKEYBYTES 32
#define DTRU_PREFIXHASHBYTES 33



#define DTRU_CBD2_BYTES (DTRU_N * 4 / 8)
#define DTRU_CBD3_BYTES (DTRU_N * 6 / 8)
#define DTRU_CBD4_BYTES (DTRU_N * 8 / 8)
#define DTRU_COINBYTES_KEYGEN (DTRU_CBD3_BYTES + DTRU_CBD4_BYTES)
#define DTRU_COINBYTES_ENC (DTRU_CBD4_BYTES + DTRU_CBD2_BYTES)

/**
 * 公钥压缩优化
 * @author: wy
 * @date: 2026-06-03
 */
// ============================================================
// #define DTRU_PKE_PUBLICKEYBYTES (DTRU_N * DTRU_LOGQ / 8)
#ifndef PK_PACK_OPT
    #define PK_PACK_OPT 1
#endif

#if PK_PACK_OPT
    #define DTRU_PKE_PUBLICKEYBYTES (1130)
#else
    #define DTRU_PKE_PUBLICKEYBYTES (DTRU_N * DTRU_LOGQ / 8)
#endif
// ============================================================


#define DTRU_PKE_CIPHERTEXTBYTES (DTRU_N * DTRU_LOGQ2 / 8)

#define DTRU_PKE_SECRETKEYBYTES (DTRU_N * 4 / 8) /* ceil(log2(4*eta+2)) = 4 */

#define DTRU_KEM_PUBLICKEYBYTES DTRU_PKE_PUBLICKEYBYTES
#define DTRU_KEM_SECRETKEYBYTES (DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES + DTRU_Z_BYTES)
#define DTRU_KEM_CIPHERTEXTBYTES DTRU_PKE_CIPHERTEXTBYTES

#endif