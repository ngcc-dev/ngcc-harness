#ifndef PARAMS_H
#define PARAMS_H

#define DTRU_N 512

#define DTRU_Q 769
#define DTRU_LOGQ 10

#define DTRU_Q2 256  /* Change this for the ciphertext modulus */
#define DTRU_LOGQ2 8 /* Change this for the ciphertext modulus */

#define DTRU_ETA 1 /* for compressed secret key */
#define DTRU_BOUND (2 * DTRU_ETA + 1) /* for uncompressed secret key */



#define DTRU_MSGBYTES 16 /* (DTRU_N / 4) bits = 128 bits = 16 bytes */
#define DTRU_SEEDBYTES 32

#define SHA3512_OUTPUTBYTES 64
#define DTRU_Z_BYTES 32
#define DTRU_SHAREDKEYBYTES 32
#define DTRU_PREFIXHASHBYTES 33



#define DTRU_CBD1_BYTES (DTRU_N * 2 / 8)
#define DTRU_CBD2_BYTES (DTRU_N * 4 / 8)
#define DTRU_COINBYTES_KEYGEN (DTRU_CBD1_BYTES + DTRU_CBD2_BYTES)
#define DTRU_COINBYTES_ENC (DTRU_CBD1_BYTES + DTRU_CBD2_BYTES)

#define DTRU_PKE_PUBLICKEYBYTES (DTRU_N * DTRU_LOGQ / 8)
#define DTRU_PKE_CIPHERTEXTBYTES (DTRU_N * DTRU_LOGQ2 / 8)

// #define DTRU_PKE_SECRETKEYBYTES 103 /* Compressed Secret Key, 5 coeff(0,+1,-1)-> 8 bits  */
#define DTRU_PKE_SECRETKEYBYTES (DTRU_N * 3 / 8) /* Uncompressed Secret Key */

#define DTRU_KEM_PUBLICKEYBYTES DTRU_PKE_PUBLICKEYBYTES
#define DTRU_KEM_SECRETKEYBYTES (DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES + DTRU_Z_BYTES)
#define DTRU_KEM_CIPHERTEXTBYTES DTRU_PKE_CIPHERTEXTBYTES

#endif