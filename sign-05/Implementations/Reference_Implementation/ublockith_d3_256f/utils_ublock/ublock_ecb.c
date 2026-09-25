/*
 * uBlock-256/256 ECB wrapper
 */

#include "ublock.h"
#include "utils.h"
#include <string.h>



int ublock256_ecb_encrypt(const ublock256_key_t *ks,
                          uint8_t *out, const uint8_t *in,
                          size_t blocks)
{
    if (!ks || !out || !in) {
        return -1;
    }


    for (size_t i = 0; i < blocks; i++) {
        ublock256_256_encrypt(in + i * 32, out + i * 32, ks);
    }
    return 0;
}

