#include "encoding.h"
#include "params.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    poly z1 = {0};
    poly z_rest[D_REST] = {{0}};
    const size_t cap = (size_t)(1 + D_REST) * N * 4 + 256;
    uint8_t *out = calloc(cap, 1);
    if (out == NULL) return 2;
    z1.coeffs[0] = getenv("CONTROL") ? 0 : B0;
    size_t n = encode_z(out, cap, &z1, z_rest);
    printf("B0=%d z1[0]=%d encoded=%zu\n", B0, z1.coeffs[0], n);
    free(out);
    return n ? 0 : 1;
}
