#ifndef MSGENC_H
#define MSGENC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#define poly_frommsg WEAVER_NAMESPACE(_poly_frommsg)
void poly_frommsg(poly *r, const uint8_t msg[WEAVER_INDCPA_MSGBYTES]);

#define poly_tomsg WEAVER_NAMESPACE(_poly_tomsg)
void poly_tomsg(uint8_t msg[WEAVER_INDCPA_MSGBYTES], const poly *a);

#define poly_compress WEAVER_NAMESPACE(_poly_compress)
void poly_compress(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a);

#define poly_decompress WEAVER_NAMESPACE(_poly_decompress)
void poly_decompress(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES]);

#endif
