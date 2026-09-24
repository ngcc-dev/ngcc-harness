/// @file gf16_tabs.h
/// @brief Defining the constant tables for performing GF arithmetics.
///

#ifndef _GF16_TABS_H_
#define _GF16_TABS_H_

#include <stdint.h>

//IF_CRYPTO_CORE:define CRYPTO_NAMESPACE

#ifdef  __cplusplus
extern  "C" {
#endif

extern const unsigned char __gf16_inv[];
extern const unsigned char __gf16_squ[];
extern const unsigned char __gf16_exp[];
extern const unsigned char __gf16_log[];

extern const unsigned char __gf16_mulbase[];
extern const unsigned char __gf16_mul[];

extern const unsigned char __gf256_squ[];
extern const unsigned char __gf256_mulbase[];
extern const unsigned char __gf256_mul[];


#ifdef  __cplusplus
}
#endif



#endif // _GF16_TABS_H_
