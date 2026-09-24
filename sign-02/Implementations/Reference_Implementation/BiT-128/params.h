/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef PARAMS_H
#define PARAMS_H

/* Core constants */
#define BIT_N             256
#define BIT_Q             26881
#define BIT_BARRETT_SHIFT 32
#define BIT_BARRETT_MULT  159778

#define BIT_Q_HALF        (BIT_Q / 2)
#define BIT_Q_HAT         13441
#define BIT_Q_HAT_MINUS   (BIT_Q_HAT - BIT_Q)

#define BIT_K  3
#define BIT_L  3
#define BIT_Y  (BIT_K + BIT_L + 1)

/* Sampling parameters */
#define BIT_TAU   30
#define BIT_BETA  30
#define BIT_GAMMA1_0   8
#define BIT_Z0_BITS    4

#define BIT_GAMMA1     1024
#define BIT_GAMMA1_2   2048
#define BIT_B_INF      2048

#define BIT_GAMMA2        682
#define BIT_GAMMA2_BARRETT_SHIFT  32
#define BIT_GAMMA2_BARRETT_MULT   6297607U
#define BIT_GAMMA_B       16
#define BIT_GAMMA_B_BITS  4
#define BIT_ALPHA_B       BIT_GAMMA_B

/* High-bits and hint thresholds */
#define BIT_B1_HIGHBITS_MAX  (((BIT_Q - 1) + (BIT_GAMMA_B >> 1)) / BIT_GAMMA_B)
#define BIT_H2_HIGHBITS_MAX  (((BIT_Q - 1) + (BIT_GAMMA2 >> 1)) / BIT_GAMMA2)
#define ALPHA_HINT           (BIT_H2_HIGHBITS_MAX + 1)

/* Byte-level sizes */
#define BIT_SEEDBYTES       16
#define BIT_CHALLENGEBYTES  32
#define BIT_MESSAGEBYTES    32
#define BIT_TRBYTES         32
#define BIT_SAMPLE_S1_BUFLEN 128

/* Polynomial and vector encoding */
#define BIT_POLY_B1_BYTES          344
#define BIT_POLYVECK_B1_BYTES      (BIT_K * BIT_POLY_B1_BYTES)
#define BIT_POLY_B0_BYTES          (BIT_N * 4 / 8)
#define BIT_POLYVECK_B0_BYTES      (BIT_K * BIT_POLY_B0_BYTES)

#define BIT_POLY_Z0_BYTES          (BIT_N * BIT_Z0_BITS / 8)
#define BIT_POLY_Z1_BYTES          (BIT_N * 11 / 8)
#define BIT_POLYVECL_Z1_BYTES      (BIT_L * BIT_POLY_Z1_BYTES)

#define BIT_POLY_W1_BYTES          (BIT_N * 6 / 8)
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
