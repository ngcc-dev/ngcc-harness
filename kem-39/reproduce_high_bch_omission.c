/* WeaverKEM-256: one correctable high-layer bit error is not corrected. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "poly.h"
#include "msgenc.h"
#include "bch.h"

int main(void)
{
    uint8_t message[WEAVER_INDCPA_MSGBYTES] = {0};
    uint8_t decoded[WEAVER_INDCPA_MSGBYTES] = {0};
    uint8_t high_data[ELL_BAR_BYTES] = {0};
    uint8_t high_ecc[8] = {0};
    poly encoded;

    poly_frommsg(&encoded, message);
    poly_tomsg(decoded, &encoded);
    if (memcmp(decoded, message, sizeof message) != 0)
        return 1;

    /* The first high-layer decision changes, while the D4 lower-layer
       repetition remains at zero. A BCH(255,223,4) decoder corrects it. */
    encoded.coeffs[0] = WEAVER_HALFQ;
    poly_tomsg(decoded, &encoded);
    high_data[0] = 0x80;
    int corrected = decode_bch_high_nibbles(high_data, ELL_BAR_NIBBLES, high_ecc);

    printf("high-layer one-bit error: output=%02x, BCH corrections=%d, BCH output=%02x\n",
           decoded[0], corrected, high_data[0]);
    if (decoded[0] != 0x80 || corrected != 1 || high_data[0] != 0)
        return 1;
    puts("CONFIRMED: WeaverKEM-256 omits its high-layer BCH correction");
    return 0;
}
