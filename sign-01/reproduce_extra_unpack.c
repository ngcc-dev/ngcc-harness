/* Build with AddressSanitizer against the unmodified Aigis-Sig+-I sources. */
#include <stdint.h>

#include "polyvec.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    unsigned char seed[SEEDBYTES + CRHBYTES] = {0};
    polyvecl y;
    /* This is the mask-sampling call made by the honest signer. */
    return polyvecl_uniform_gamma1(&y, seed, 0);
}
