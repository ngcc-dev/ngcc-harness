/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#ifndef PARAMS_H
#define PARAMS_H

/* ============================================================
   Core constants
   ============================================================ */
#define BIT_N                           256
#define BIT_Q                           26881
#define BIT_BARRETT_SHIFT               32
#define BIT_BARRETT_MULT                159778  /* ceil(2^32/26881) */

#define BIT_Q_HALF                      (BIT_Q / 2)
#define BIT_Q_HAT                       13441
#define BIT_Q_HAT_MINUS                 (BIT_Q_HAT - BIT_Q)

#define BIT_K                           3
#define BIT_L                           3
#define BIT_Y                           (BIT_K + BIT_L + 1)

/* ============================================================
   Sampling parameters
   ============================================================ */
#define BIT_TAU                         30
#define BIT_BETA                        30
#define BIT_GAMMA1_0                    8
#define BIT_Z0_BITS                     4

#define BIT_GAMMA1                      1024
#define BIT_GAMMA1_2                    2048   /* gamma_{1,2} = 2^11 (11-bit triangular y2) */
#define BIT_B_INF                       2048

#define BIT_GAMMA2        682
/* Barrett constants for division by BIT_GAMMA2:  floor(x / 682)  ←  (x * MULT) >> SHIFT
   MULT = ceil(2^32 / 682) = 6297607 (verified exact for x in [0, q + GAMMA2/2)). */
#define BIT_GAMMA2_BARRETT_SHIFT  32
#define BIT_GAMMA2_BARRETT_MULT   6297607U
#define BIT_GAMMA_B       16
#define BIT_GAMMA_B_BITS  4
#define BIT_ALPHA_B       BIT_GAMMA_B

/* ============================================================
   High-bits / hint thresholds
   ============================================================ */
#define BIT_B1_HIGHBITS_MAX  (((BIT_Q - 1) + (BIT_GAMMA_B >> 1)) / BIT_GAMMA_B)
#define BIT_H2_HIGHBITS_MAX  (((BIT_Q - 1) + (BIT_GAMMA2 >> 1)) / BIT_GAMMA2)
#define ALPHA_HINT           (BIT_H2_HIGHBITS_MAX + 1)
/* ============================================================
   Byte-level sizes (seeds / hashes / keys)
   ============================================================ */
#define BIT_SEEDBYTES                   16
#define BIT_CHALLENGEBYTES              32
#define BIT_MESSAGEBYTES                32
#define BIT_TRBYTES                     32
#define BIT_SAMPLE_S1_BUFLEN            136  /* = SHAKE256_RATE: avoids partial-block path in shake256x4 */

/* ============================================================
   Polynomial / vector encoding
   ============================================================ */
#define BIT_POLY_B1_BYTES               344
#define BIT_POLYVECK_B1_BYTES           (BIT_K * BIT_POLY_B1_BYTES)
#define BIT_POLY_B0_BYTES               (BIT_N * 4 / 8)
#define BIT_POLYVECK_B0_BYTES           (BIT_K * BIT_POLY_B0_BYTES)

#define BIT_POLY_Z0_BYTES               (BIT_N * BIT_Z0_BITS / 8)
#define BIT_POLY_Z1_BYTES               (BIT_N * 11 / 8)
#define BIT_POLYVECL_Z1_BYTES           (BIT_L * BIT_POLY_Z1_BYTES)

#define BIT_POLY_W1_BYTES               (BIT_N * 6 / 8)
#define BIT_POLYVECK_W1_BYTES           (BIT_K * BIT_POLY_W1_BYTES)

/* ============================================================
   Hint encoding
   ------------------------------------------------------------
   Two encoders coexist; select with BIT_HINT_BITPACK:
     * defined   -> fixed-width bit packing (||h||_inf = 3, BIT_POLY_H_BITS/coeff)
     * undefined -> base-3 + sparse overflow table (compact; |h|<=2, <=12 of +-2)
   The base-3 path is kept for the later size-compression work.
   ============================================================ */
#define BIT_HINT_BITPACK

/* --- base-3 + sparse overflow (compact; kept for later) --- */
#define BIT_H_PACK_GROUP                5
#define BIT_H_OVERFLOW_MAX              12
#define BIT_POLY_H_GROUPS               ((BIT_N + BIT_H_PACK_GROUP - 1) / BIT_H_PACK_GROUP)
#define BIT_POLY_H_BASE3_BYTES          BIT_POLY_H_GROUPS
#define BIT_POLYVECK_H_BASE3_BYTES      (BIT_K * BIT_POLY_H_BASE3_BYTES)
#define BIT_H_OVERFLOW_BITS             11
#define BIT_POLYVECK_H_OVERFLOW_BYTES   (1 + ((BIT_H_OVERFLOW_MAX * BIT_H_OVERFLOW_BITS + 7) / 8))
#define BIT_POLYVECK_H_BASE3_TOTAL      (BIT_POLYVECK_H_BASE3_BYTES + BIT_POLYVECK_H_OVERFLOW_BYTES)

/* --- fixed-width bit packing: ||h||_inf = 3, each coeff = (h + 3) in 3 bits --- */
#define BIT_H_INF                       3
#define BIT_POLY_H_BITS                 3
#define BIT_POLY_H_BITPACK_BYTES        ((BIT_N * BIT_POLY_H_BITS + 7) / 8)
#define BIT_POLYVECK_H_BITPACK_BYTES    (BIT_K * BIT_POLY_H_BITPACK_BYTES)

#ifdef BIT_HINT_BITPACK
#define BIT_POLYVECK_H_BYTES            BIT_POLYVECK_H_BITPACK_BYTES
#else
#define BIT_POLYVECK_H_BYTES            BIT_POLYVECK_H_BASE3_TOTAL
#endif

/* ============================================================
   Key / signature sizes
   ============================================================ */
#define BIT_PUBLICKEYBYTES              (BIT_SEEDBYTES + BIT_POLYVECK_B1_BYTES)
#define BIT_SECRETKEYBYTES              (BIT_SEEDBYTES + BIT_POLYVECK_B1_BYTES + BIT_SEEDBYTES \
                                        + BIT_TRBYTES + (BIT_L + BIT_K) * BIT_N / 4 \
                                        + BIT_POLYVECK_B0_BYTES)
#define BIT_SIGN_MAX_ITERS              65536

#define BIT_SIGNBYTES                   (BIT_CHALLENGEBYTES + BIT_POLY_Z0_BYTES \
                                        + BIT_POLYVECL_Z1_BYTES + BIT_POLYVECK_H_BYTES)

#endif
