#ifndef CBD_H
#define CBD_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#define cbd_eta1 WEAVER_NAMESPACE(_cbd_eta1)
void cbd_eta1(poly *r, const uint8_t buf[WEAVER_ETA1*WEAVER_N/4]);

#define cbd_eta2 WEAVER_NAMESPACE(_cbd_eta2)
void cbd_eta2(poly *r, const uint8_t buf[WEAVER_ETA2*WEAVER_N/4]);

#if defined(WEAVER_USE_AVX_CBD)
#define cbd_eta1_avx WEAVER_NAMESPACE(_cbd_eta1_avx)
void cbd_eta1_avx(poly *r, const uint8_t buf[WEAVER_ETA1*WEAVER_N/4]);
#define cbd_eta2_avx WEAVER_NAMESPACE(_cbd_eta2_avx)
void cbd_eta2_avx(poly *r, const uint8_t buf[WEAVER_ETA2*WEAVER_N/4]);
#endif

#endif
