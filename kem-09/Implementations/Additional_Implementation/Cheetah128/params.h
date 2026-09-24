#ifndef CHEETAH_PARAMS_H
#define CHEETAH_PARAMS_H

// Prime dimension
#define N 640
#define CHEETAH_K 1


// Prime modulus
#define Q 7681
#define QBITS 13 // Ceiling(log(2, Q))
#define DELTA 3841 //(Q+1)/2
#define REJECT_THRESHOLD  61448 // 8*Q 

// Error distribution parameter
#define ETA 5

#define DB 10
#define DU 10
#define DV 4

// Shared key and seed length (bytes)
#define SEED_BYTES 32
#define SHARED_KEY_BYTES 16
#define HASH_BYTES 32

// Ciphertext and public key size (bytes)
#define CIPHERTEXT_BYTES 864 // N*d_u/8+SHARED_KEY_BYTES*d_v
#define PUBLICKEY_BYTES 832 //  SEED_BYTES + (CHEETAH_K*N*d_u)/8

#define NOISE_POLY_BYTES 1040  // CHEETAH_K*N*QBITS/8
#define SECRETKEY_BYTES (NOISE_POLY_BYTES+PUBLICKEY_BYTES+HASH_BYTES+SEED_BYTES)

#define MSG_BYTES  16  // 
#define COIN_BYTES (3*SEED_BYTES)


#endif