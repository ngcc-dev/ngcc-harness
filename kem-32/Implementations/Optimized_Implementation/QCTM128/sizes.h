#ifndef SIZES_H
#define SIZES_H

#include <stdio.h>

#include "gf.h"

#define HASH_SIZE 32

#define BITS_TO_BYTES(nb_bits) (((nb_bits) - 1) / 8 + 1)

#define BIT_SIZE_OF_LONG (8 * sizeof(long))
#define BITS_TO_LONG(nb_bits) (((nb_bits) - 1) / BIT_SIZE_OF_LONG + 1)

#define EXT_DEGREE GF_EXT_DEGREE
#define FIELD_CARDINALITY (1U << EXT_DEGREE)
#define MATGEN_GOPPA_ROWS (GOPPA_DEGREE - 1)
#define MATGEN_BINARY_ROWS (MATGEN_GOPPA_ROWS * EXT_DEGREE)
#define MATGEN_TOTAL_ROWS (MATGEN_BINARY_ROWS + 1)
#define SYSTEMATIC_TAIL_ROWS (MATGEN_TOTAL_ROWS % ORDER)
#define SYSTEMATIC_QC_ROWS (MATGEN_TOTAL_ROWS - SYSTEMATIC_TAIL_ROWS)
#define FULL_SYNDROME_LENGTH (GOPPA_DEGREE * EXT_DEGREE)
#define CODIMENSION MATGEN_TOTAL_ROWS
#define DIMENSION (LENGTH - CODIMENSION)
#define PUBLIC_T_COLUMNS (LENGTH - SYSTEMATIC_QC_ROWS)
#define PSI_T1_ROWS (SYSTEMATIC_QC_ROWS / ORDER)
#define PSI_T1_BITS (PSI_T1_ROWS * PUBLIC_T_COLUMNS)
#define T2_BITS (SYSTEMATIC_TAIL_ROWS * PUBLIC_T_COLUMNS)
#define PUBLIC_T_BITS (PSI_T1_BITS + T2_BITS)
#define PUBLICKEY_BITS PUBLIC_T_BITS
#define PUBLICKEY_BYTES ((int)BITS_TO_BYTES(PUBLICKEY_BITS))
#define SYNDROME_LENGTH CODIMENSION
#define SYNDROME_BYTES BITS_TO_BYTES(SYNDROME_LENGTH)
#define FULL_SYNDROME_BYTES BITS_TO_BYTES(FULL_SYNDROME_LENGTH)

#define PARAM_T0 (GOPPA_DEGREE / ORDER)
#define PARAM_ERROR_T0 (ERROR_WEIGHT / ORDER)
#define PARAM_SIGMA1 EXT_DEGREE
#define PARAM_SIGMA2 (2 * EXT_DEGREE)
#define PARAM_ELL (8 * HASH_SIZE)
#define PARAM_RHO_MULT     ((LENGTH == FIELD_CARDINALITY) ? 1 :      (LENGTH >= FIELD_CARDINALITY / 2) ? 2 :      (LENGTH >= FIELD_CARDINALITY / 4) ? 4 :      (LENGTH >= FIELD_CARDINALITY / 8) ? 8 :      (LENGTH >= FIELD_CARDINALITY / 16) ? 16 :      (LENGTH >= FIELD_CARDINALITY / 32) ? 32 :      (LENGTH >= FIELD_CARDINALITY / 64) ? 64 : 128)
#define PARAM_RHO (PARAM_RHO_MULT * ERROR_WEIGHT)

#define FIXED_WEIGHT_BITS (PARAM_SIGMA1 * PARAM_RHO)
#define FIXED_WEIGHT_BYTES ((int)BITS_TO_BYTES(FIXED_WEIGHT_BITS))

#define DECODE_MAP_BYTES ((int)(FULL_SYNDROME_LENGTH * BITS_TO_BYTES(CODIMENSION)))
#define SECRETKEY_GAMMA_BITS ((LENGTH + GOPPA_DEGREE) * EXT_DEGREE)
#define SECRETKEY_GAMMA_BYTES ((int)BITS_TO_BYTES(SECRETKEY_GAMMA_BITS))
#define SECRETKEY_S_BYTES ((int)BITS_TO_BYTES(LENGTH))
#define SECRETKEY_DECODE_MAP_OFFSET SECRETKEY_GAMMA_BYTES
#define SECRETKEY_CORE_BYTES SECRETKEY_GAMMA_BYTES
#define SECRETKEY_PK_OFFSET SECRETKEY_CORE_BYTES
#define SECRETKEY_S_OFFSET (SECRETKEY_PK_OFFSET + PUBLICKEY_BYTES)
#define SECRETKEY_BYTES ((int)(SECRETKEY_S_OFFSET + SECRETKEY_S_BYTES))
#define CIPHERTEXT_LENGTH SYNDROME_LENGTH
#define CIPHERTEXT_BYTES ((int)SYNDROME_BYTES)

#define QCTM_RIGHT_WIDTH PUBLIC_T_COLUMNS
#define QCTM_PSI_T1_ROWS PSI_T1_ROWS
#define QCTM_T2_ROWS SYSTEMATIC_TAIL_ROWS
#define QCTM_PK_BYTES PUBLICKEY_BYTES
#define QCTM_GAMMA_BYTES SECRETKEY_GAMMA_BYTES
#define QCTM_S_BYTES SECRETKEY_S_BYTES
#define QCTM_API_SK_BYTES SECRETKEY_BYTES
#define QCTM_CT_BYTES CIPHERTEXT_BYTES
#define QCTM_SS_BYTES HASH_SIZE

#if LENGTH == 10070 && GOPPA_DEGREE == 285 && EXT_DEGREE == 18 && ORDER == 19
_Static_assert(QCTM_PK_BYTES == 167987, "QCTM128 PK byte length mismatch");
_Static_assert(QCTM_GAMMA_BYTES == 23299, "QCTM128 GammaPrime byte length mismatch");
_Static_assert(QCTM_S_BYTES == 1259, "QCTM128 s byte length mismatch");
_Static_assert(QCTM_API_SK_BYTES == 192545, "QCTM128 API sk byte length mismatch");
_Static_assert(QCTM_CT_BYTES == 640, "QCTM128 CT byte length mismatch");
_Static_assert(QCTM_SS_BYTES == 32, "QCTM128 SS byte length mismatch");
_Static_assert(QCTM_RIGHT_WIDTH == 4959, "QCTM128 right-width mismatch");
_Static_assert(QCTM_PSI_T1_ROWS == 269, "QCTM128 psi(T1) row count mismatch");
_Static_assert(QCTM_T2_ROWS == 2, "QCTM128 T2 row count mismatch");
#endif

#define RANDOM_LENGTH FIXED_WEIGHT_BITS
#define RANDOM_BYTES FIXED_WEIGHT_BYTES

#define ind(v, i) ((v)[(i) / 8] >> ((i) % 8) & 1)

#define show_params() {                                                           printf("(n,m,l,t,matgen_rows,w,t0)=(%d,%d,%d,%d,%d,%d,%d)\n",                       LENGTH, EXT_DEGREE, ORDER, GOPPA_DEGREE, MATGEN_GOPPA_ROWS,             ERROR_WEIGHT, PARAM_T0);                                                printf("sigma1=%d sigma2=%d rho=%d\n",                                             PARAM_SIGMA1, PARAM_SIGMA2, PARAM_RHO);                               printf("Ciphertext: %d bits\n", CIPHERTEXT_LENGTH);                        printf("Public key: %ld bytes\n", (long)PUBLICKEY_BYTES);                  printf("Secret key: %ld bytes\n", (long)SECRETKEY_BYTES);              }

#endif
