#ifndef LOONG_PARAMS_H
#define LOONG_PARAMS_H

// Prime dimension
#define N 20
#define K1 72
#define K2 8

// Prime modulus
#define Q 8191
#define QBITS 13 // Ceiling(log(2, Q))
#define DELTA 4096 //(Q+1)/2
#define REJECT_THRESHOLD  65528 // 8*Q 

// Error distribution parameter
#define ETA 3

#define DB 11
#define DU 11
#define DV 6

// Shared key and seed length (bytes)
#define SEED_BYTES 64
#define SHARED_KEY_BYTES 48
#define HASH_BYTES 32

// Ciphertext and public key size (bytes)
#define CIPHERTEXT_BYTES 6680 // (K1*N*d_u+K2*N*N*d_u + N*N*d_v)/8
#define PUBLICKEY_BYTES 6444 //  SEED_BYTES + (K1*N*d_u+K2*N*N*d_u)/8

#define NOISE_POLY_BYTES 800  // (K1+K2)*N/2
#define SECRETKEY_BYTES (NOISE_POLY_BYTES+PUBLICKEY_BYTES+HASH_BYTES+SEED_BYTES)

#define MSG_BYTES  50  // N*N/8
#define COIN_BYTES (3*SEED_BYTES)


#endif