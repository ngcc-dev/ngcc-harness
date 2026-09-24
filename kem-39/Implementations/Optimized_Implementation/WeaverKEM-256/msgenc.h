#ifndef MSGENC_H
#define MSGENC_H

#include <stdint.h>
#include "params.h"

#if   (WEAVER_MODE == 1)
  #define ELL_BAR_BYTES        14
  #define ELL_DDOT_BYTES       2
  #define LOW_ECC_BYTES        2
  #define LOW_CODEWORD_BYTES   4
  #define D4_STEP_LEN          32

#elif (WEAVER_MODE == 3)
  #define ELL_BAR_BYTES        28 // ceil 220 bits = 28 bytes
  #define ELL_BAR_NIBBLES      55 // ceil 220 bits = 55 NIBBLES
  #define ELL_DDOT_BYTES       5  // ceil 33 bits = 5 bytes
  #define ELL_DDOT_NIBBLES     9  // ceil 36 bits = 9 NIBBLES
  #define LOW_ECC_NIBBLES      6  // (63,39,4) BCH code; ceil （4*6=24） = 6 NIBBLES 
  #define LOW_CODEWORD_NIBBLES 15  // (ELL_DDOT_NIBBLES + LOW_ECC_NIBBLES) < 16 NIBBLES = 64 bits
  #define LOW_CODEWORD_BYTES   8
  #define D4_STEP_LEN 64

#elif (WEAVER_MODE == 5)
  /* High: BCH(511,448,7) — 56 data + 8 ECC = 64 bytes (mu_tilde full). */
  #define ELL_BAR_BYTES        56
  /* Low: BCH(127,64,7) — 8 data + 7 ECC = 15 bytes D4 codeword. */
  #define ELL_DDOT_BYTES       8
  #define LOW_ECC_BYTES        7
  #define LOW_CODEWORD_BYTES   15
  #define D4_STEP_LEN          128

#else
  #error "WEAVER_MODE must be in {1,3,5}"
#endif

#define poly_frommsg WEAVER_NAMESPACE(_poly_frommsg)
void poly_frommsg(poly *r, const uint8_t msg[WEAVER_INDCPA_MSGBYTES]);
#define poly_tomsg WEAVER_NAMESPACE(_poly_tomsg)
void poly_tomsg(uint8_t msg[WEAVER_INDCPA_MSGBYTES], const poly *a);

#define poly_compress WEAVER_NAMESPACE(_poly_compress)
void poly_compress(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress WEAVER_NAMESPACE(_poly_decompress)
void poly_decompress(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES]);

#endif
