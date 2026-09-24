#ifndef CHEETAH_NTT_H
#define CHEETAH_NTT_H


/*
 * NTT (Number Theoretic Transform) related constants definition
 * Parameter configuration for implementing 640-point number theoretic transform
 */
#define N1          128U         // Radix-2 sub-length (2^7)
#define N2          5U           // Radix-5 sub-length
#define PSI         5U           // Primitive root of Q (base primitive root, does not affect specified constants)
#define INV_PSI     6145         // Inverse of PSI
#define W1          3074U        // Specified 128-point NTT primitive root
#define W2          4143U        // Specified 5-point NTT primitive root
#define INV_N       7669U        // Specified 1/640 mod 7681
#define BIT_WIDTH   7U           // Binary bit width for 128-point bit-reversal (since 128 = 2^7)

typedef enum {
    NTT_FORWARD = 0,  // 正向NTT
    NTT_INVERSE = 1   // 逆向INTT
} ntt_direction_enum;

void ntt_640(int16_t *a, ntt_direction_enum dir);

#endif