#ifndef PQCLEAN_SNTRUP761_AVX2_CRYPTO_CORE_INVSNTRUP761_new_H
#define PQCLEAN_SNTRUP761_AVX2_CRYPTO_CORE_INVSNTRUP761_new_H

#include <stdint.h>
#include "params.h"
#define PQCLEAN_SNTRUP761_AVX2_crypto_core_invsntrup761_OUTPUTBYTES 1523
#define PQCLEAN_SNTRUP761_AVX2_crypto_core_invsntrup761_INPUTBYTES 761
#define PQCLEAN_SNTRUP761_AVX2_crypto_core_invsntrup761_KEYBYTES 0
#define PQCLEAN_SNTRUP761_AVX2_crypto_core_invsntrup761_CONSTBYTES 0
//used in Rq_inverse(avx2)
#define qinv 31777 /* reciprocal of q mod 2^16 */
#define q27 66544 /* closest integer to 2^27/q */
#define q18 130 /* closest integer to 2^18/q */
#define ppad 1089

int inv_dtru_prime1087_avx2(int16_t *out, const int16_t *in);
#endif
