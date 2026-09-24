/*
 * Copyright (c) 2026, Hang Zhang
 */
#ifndef PARAMS_H
#define PARAMS_H

/* Core constants */
#define BIT_N             512
#define BIT_Q             119297
#define BIT_BARRETT_SHIFT 34
#define BIT_BARRETT_MULT  144010

#define BIT_Q_HALF        (BIT_Q / 2)
#define BIT_Q_HAT         59649
#define BIT_Q_HAT_MINUS   (BIT_Q_HAT - BIT_Q)

#define BIT_K  3
#define BIT_L  3
#define BIT_Y  (BIT_K + BIT_L + 1)

/* Sampling parameters */
#define BIT_TAU        58
#define BIT_BETA       58
#define BIT_GAMMA1_0   16
#define BIT_Z0_BITS     5

#define BIT_GAMMA1   4096
#define BIT_GAMMA1_2 8192
#define BIT_B_INF    8192

#define BIT_GAMMA2      5517
#define BIT_GAMMA2_BARRETT_SHIFT  35
#define BIT_GAMMA2_BARRETT_MULT   6227976ULL
#define BIT_GAMMA_B       64
#define BIT_GAMMA_B_BITS   6       /* 64 == 2^6 */
#define BIT_ALPHA_B       BIT_GAMMA_B

/* High-bits and hint thresholds */
#define BIT_B1_HIGHBITS_MAX  (((BIT_Q - 1) + (BIT_GAMMA_B >> 1)) / BIT_GAMMA_B)
#define BIT_H2_HIGHBITS_MAX  (((BIT_Q - 1) + (BIT_GAMMA2 >> 1)) / BIT_GAMMA2)
#define ALPHA_HINT           (BIT_H2_HIGHBITS_MAX + 1)

/* Byte-level sizes */
#define BIT_SEEDBYTES       32
#define BIT_CHALLENGEBYTES  64
#define BIT_MESSAGEBYTES    64
#define BIT_TRBYTES         64
#define BIT_SAMPLE_S1_BUFLEN 256

/* Polynomial and vector encoding */
#define BIT_POLY_B1_BYTES          (BIT_N * BIT_POLY_B1_BITS / 8)
#define BIT_POLYVECK_B1_BYTES      (BIT_K * BIT_POLY_B1_BYTES)
#define BIT_POLY_B0_BYTES          (BIT_N * BIT_GAMMA_B_BITS / 8)
#define BIT_POLYVECK_B0_BYTES      (BIT_K * BIT_POLY_B0_BYTES)

#define BIT_POLY_B1_BITS  11
#define BIT_POLY_B0_BITS  BIT_GAMMA_B_BITS
#define BIT_POLY_Z0_BITS  BIT_Z0_BITS
#define BIT_POLY_Z1_BITS  13
#define BIT_POLY_W1_BITS   5

#define BIT_POLY_Z0_BYTES          (BIT_N * BIT_Z0_BITS / 8)
#define BIT_POLY_Z1_BYTES          (BIT_N * BIT_POLY_Z1_BITS / 8)
#define BIT_POLYVECL_Z1_BYTES      (BIT_L * BIT_POLY_Z1_BYTES)

#define BIT_POLY_W1_BYTES          (BIT_N * BIT_POLY_W1_BITS / 8)
#define BIT_POLYVECK_W1_BYTES      (BIT_K * BIT_POLY_W1_BYTES)

/* Hint encoding: fixed-width bit packing, ||h||_inf = BIT_H_INF */
#define BIT_H_INF                       3
#define BIT_POLY_H_BITS                 3
#define BIT_POLY_H_BITPACK_BYTES        ((BIT_N * BIT_POLY_H_BITS + 7) / 8)
#define BIT_POLYVECK_H_BITPACK_BYTES    (BIT_K * BIT_POLY_H_BITPACK_BYTES)

#define BIT_POLYVECK_H_BYTES            BIT_POLYVECK_H_BITPACK_BYTES

/* Key and signature sizes */
#define BIT_PUBLICKEYBYTES  (BIT_SEEDBYTES + BIT_POLYVECK_B1_BYTES)
#define BIT_SECRETKEYBYTES  (BIT_SEEDBYTES + BIT_POLYVECK_B1_BYTES + BIT_SEEDBYTES \
                             + BIT_TRBYTES + (BIT_L + BIT_K) * BIT_N / 4 \
                             + BIT_POLYVECK_B0_BYTES)
#define BIT_SIGN_MAX_ITERS  65536

#define BIT_SIGNBYTES       (BIT_CHALLENGEBYTES + BIT_POLY_Z0_BYTES \
                             + BIT_POLYVECL_Z1_BYTES + BIT_POLYVECK_H_BYTES)

#endif
