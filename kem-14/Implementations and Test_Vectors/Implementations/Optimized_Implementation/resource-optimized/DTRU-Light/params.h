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

// AVX2 implementation

//mont = 65536 mod 769 = 171    
//mont^-1 = 9

// #define CTRU_N 512 /* Change this for different security strengths */


// #define MULTMODE 1 /* 0 for 8-bit schoolbook, 1 for ToomCook/multimoduli NTT */

// #define POLY_DECODEMULTMODE 0 /* 0 for multimoduli NTT, 1 for ToomCook */

// #define CTRU_Q 769
// #define CTRU_LOGQ 10


// #define CTRU_SEEDBYTES CTRU_MSGBYTES


// #define SHA3512_OUTPUTBYTES 64

// #define CTRU_CBD1_BYTES CTRU_N/4
// #define CTRU_COINBYTES_KEYGEN CTRU_CBD1_BYTES*2
// #define CTRU_COINBYTES_ENC CTRU_CBD1_BYTES


// #define CTRU_Q2 256  /* Change this for the ciphertext modulus */
// #define CTRU_LOGQ2 8 /* Change this for the ciphertext modulus */

// #define CTRU_PKE_PUBLICKEYBYTES 512*10/8
// #define CTRU_PKE_CIPHERTEXTBYTES (CTRU_N)
// #define CTRU_BOUND 3



// #define CTRU_Z_BYTES 32
// #define CTRU_SHAREDKEYBYTES 32
// #define CTRU_MSGBYTES 32
// #define CTRU_PREFIXHASHBYTES 33


// #define SECRETBYTES 3
// #define CTRU_PKE_SECRETKEYBYTES (SECRETBYTES * CTRU_N / 8) 

// #define CTRU_KEM_PUBLICKEYBYTES CTRU_PKE_PUBLICKEYBYTES
// #define CTRU_KEM_SECRETKEYBYTES (CTRU_PKE_SECRETKEYBYTES + CTRU_PKE_PUBLICKEYBYTES + CTRU_Z_BYTES)
// #define CTRU_KEM_CIPHERTEXTBYTES CTRU_PKE_CIPHERTEXTBYTES

#endif