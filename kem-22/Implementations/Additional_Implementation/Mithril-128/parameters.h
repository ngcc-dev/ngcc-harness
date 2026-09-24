#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <stdint.h>
#include "fips202.h"
#ifdef __cplusplus
extern "C"
{
#endif

  #ifndef RRLWR_SECURITY_LEVEL
  #define RRLWR_SECURITY_LEVEL 128
  #endif

  // Algorithm parameters (should not be changed)
  #if (RRLWR_SECURITY_LEVEL == 128)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (5)
    #define RRLWR_PKE_ELL             (1)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (3)
    #define RRLWR_PKE_LOG_ETA         (1)
  #elif (RRLWR_SECURITY_LEVEL == 256)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (9)
    #define RRLWR_PKE_ELL             (2)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (3)
    #define RRLWR_PKE_LOG_ETA         (1)
  #elif (RRLWR_SECURITY_LEVEL == 512)
    #define RRLWR_N                   (128)
    #define RRLWR_K                   (17)
    #define RRLWR_PKE_ELL             (4)
    #define RRLWR_PKE_LOGQ            (13)
    #define RRLWR_PKE_LOGP            (11)
    #define RRLWR_PKE_LOGT            (8)
    #define RRLWR_PKE_LOG_ETA         (1)
  #endif

  #define RRLWR_PKE_SEED_A_LEN        (64)
  #define RRLWR_SEED_S_LEN            (64)
  #define RRLWR_PKE_P                 ((int32_t)1 << RRLWR_PKE_LOGP)
  #define RRLWR_PKE_PACKED_POLYQ_LEN  (RRLWR_PKE_LOGQ*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLYP_LEN  (RRLWR_PKE_LOGP*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLYT_LEN  (RRLWR_PKE_LOGT*RRLWR_N/8)
  #define RRLWR_PKE_PACKED_POLY1_LEN  (RRLWR_N/8)
  #define RRLWR_PACKED_POLY2_LEN      (2*RRLWR_N/8)
  #define RRLWR_PACKED_POLY11_LEN     (11*RRLWR_N/8)

  #define RRLWR_PKE_MESSAGE_LEN       (RRLWR_PKE_ELL*RRLWR_N/8)
  #define RRLWR_PKE_SK_LEN            (RRLWR_K*RRLWR_PACKED_POLY2_LEN)
  #define RRLWR_PKE_PK_LEN            (RRLWR_PKE_SEED_A_LEN + RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN)
  #define RRLWR_PKE_CT_LEN            (RRLWR_PKE_ELL*RRLWR_PKE_PACKED_POLYT_LEN + RRLWR_K*RRLWR_PKE_PACKED_POLYP_LEN)

  #define RRLWR_KEM_SEED_Z_LEN        (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_HPK_LEN           (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_SS_LEN            (RRLWR_PKE_MESSAGE_LEN)
  #define RRLWR_KEM_SK_LEN            (RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN + RRLWR_KEM_SEED_Z_LEN)
  #define RRLWR_KEM_PK_LEN            (RRLWR_PKE_PK_LEN)
  #define RRLWR_KEM_CT_LEN            (RRLWR_PKE_CT_LEN)

  // Define hash functions
  #define RRLWR_XOF_PUBLIC(output, output_len, input, input_len) \
          shake128(output, output_len, input, input_len)
  #define RRLWR_XOF_SECRET(output, output_len, input, input_len) \
          shake256(output, output_len, input, input_len)
  #define GENERATE_RANDOM_BYTES(output, output_len, ctx) \
        get_random_number(ctx, output, 8*(output_len))

  // The choice of prime for the arithmetic does not affect the algorithm, only the implementation.
  // The prime must be selected so that it is greater than the max coefficient of A*s, which lies in [-2*q*n*k, q*n*k].
  // This means log(prime) > log(3*2**13*128*17+1) = 25.67 to work for all parameter sets.
  #define RRLWR_PKE_PRIME             (0x03306001)
  #define RRLWR_PKE_PRIMEINV          (0xdf305fff) // -prime^-1 mod R
  #define RRLWR_KEM_RMODPRIME         (0xe1ffb0)   // R mod prime
  #define RRLWR_KEM_2RMODPRIME        (0x1c3ff60)  // 2*R mod prime
  #define RRLWR_NTTINV_FINALCONST     (0x1f90312)  // R^2 / N
  // Roots of unity for the NTT stored in Montgomery domain
  #define RRLWR_KEM_ZETAS             {14811056, 15557012, 28047398, 19613886, 22014525, 51788907, 48419597, 9957646, 29945921, 40828417, 19580069, 51150115, 29506430, 9529366, 19395954, 4004290, 13872263, 37755838, 42235427, 30502492, 5729903, 37385910, 40675608, 16363928, 30548536, 25157852, 17936602, 53243765, 13453419, 2221178, 5152110, 47075584, 30717484, 14595397, 47015892, 5730340, 38121231, 3716751, 42989631, 14140310, 26237755, 50884504, 8153081, 11893945, 50076192, 53404766, 12485385, 51970214, 32360616, 25905146, 46283828, 45002548, 25787493, 29226822, 35251252, 21276996, 3100536, 9825355, 40620172, 14754030, 35210943, 8704562, 33070433, 26137463, 48698733, 16087390, 19770173, 43456546, 35416303, 40305589, 5803563, 16076682, 14712844, 24857562, 18650951, 43995178, 40428030, 22697250, 36832621, 8466204, 26102989, 20349159, 11577061, 46835933, 20173101, 30169159, 28970679, 42946381, 19653512, 10414600, 31590192, 17267000, 35602822, 30714731, 21933346, 15870608, 47958732, 50950167, 37096816, 34566451, 31706226, 22532184, 4298490, 49521359, 23348615, 12905866, 3752400, 53096042, 50621582, 25133769, 24570237, 2238636, 41125901, 48439930, 34484034, 16603984, 20211229, 32901670, 31411393, 11841002, 30723770, 9520016, 48479116, 44486416, 33110845, 33630608, 30027270, 47585966}

  #define PRECOMPUTE_TWIST

  #ifdef PRECOMPUTE_TWIST
    extern int32_t precomputed_twist[RRLWR_N];
  #endif

#ifdef __cplusplus
}
#endif

#endif
