#ifndef POLY_COMPRESS_AVX_H
#define POLY_COMPRESS_AVX_H

#include "params.h"
#include "poly.h"

#if defined(WEAVER_USE_AVX_COMPRESS)

#if (WEAVER_N == 128 || WEAVER_N == 256 || WEAVER_N == 512) && \
    (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 9 / 8) || \
     WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 9 / 8))
#define poly_compress9_avx WEAVER_NAMESPACE(_poly_compress9_avx)
void poly_compress9_avx(uint8_t r[(WEAVER_N * 9) / 8], const poly *a);
#define poly_compress9_quant_avx WEAVER_NAMESPACE(_poly_compress9_quant_avx)
void poly_compress9_quant_avx(uint16_t t[WEAVER_N], const poly *a);
#endif

#if (WEAVER_N == 256 || WEAVER_N == 512) && (WEAVER_Q == 7681)
#define poly_compress10_avx WEAVER_NAMESPACE(_poly_compress10_avx)
void poly_compress10_avx(uint8_t r[(WEAVER_N * 10) / 8], const poly *a);
#define poly_decompress10_avx WEAVER_NAMESPACE(_poly_decompress10_avx)
void poly_decompress10_avx(poly *r, const uint8_t a[(WEAVER_N * 10) / 8]);
#endif

#if (WEAVER_N == 512) && (WEAVER_Q == 7681)
#define poly_compress11_avx WEAVER_NAMESPACE(_poly_compress11_avx)
void poly_compress11_avx(uint8_t r[(WEAVER_N * 11) / 8], const poly *a);
#define poly_decompress11_avx WEAVER_NAMESPACE(_poly_decompress11_avx)
void poly_decompress11_avx(poly *r, const uint8_t a[(WEAVER_N * 11) / 8]);
#endif

#if (WEAVER_DV == 8)
#define poly_compress_d8_avx WEAVER_NAMESPACE(_poly_compress_d8_avx)
void poly_compress_d8_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress_d8_avx WEAVER_NAMESPACE(_poly_decompress_d8_avx)
void poly_decompress_d8_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES]);
#endif

#if (WEAVER_DV == 9)
#define poly_compress_d9_avx WEAVER_NAMESPACE(_poly_compress_d9_avx)
void poly_compress_d9_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress_d9_avx WEAVER_NAMESPACE(_poly_decompress_d9_avx)
void poly_decompress_d9_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES]);
#endif

#if (WEAVER_DV == 4)
#define poly_compress_d4_avx WEAVER_NAMESPACE(_poly_compress_d4_avx)
void poly_compress_d4_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress_d4_avx WEAVER_NAMESPACE(_poly_decompress_d4_avx)
void poly_decompress_d4_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES]);
#endif

#if (WEAVER_DV == 5)
#define poly_compress_d5_avx WEAVER_NAMESPACE(_poly_compress_d5_avx)
void poly_compress_d5_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress_d5_avx WEAVER_NAMESPACE(_poly_decompress_d5_avx)
void poly_decompress_d5_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES]);
#endif

#if (WEAVER_DV == 6)
#define poly_compress_d6_avx WEAVER_NAMESPACE(_poly_compress_d6_avx)
void poly_compress_d6_avx(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a);
#define poly_decompress_d6_avx WEAVER_NAMESPACE(_poly_decompress_d6_avx)
void poly_decompress_d6_avx(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES]);
#endif

#endif /* WEAVER_USE_AVX_COMPRESS */

#endif
