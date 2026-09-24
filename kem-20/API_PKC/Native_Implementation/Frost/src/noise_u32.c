#include "frost_u32_core.h"

void frost_sample_n_u32(int32_t *s, const uint16_t *rnd, size_t n, unsigned eta)
{
    for (size_t i = 0; i < n; i++) {
        uint16_t x = rnd[i];
        int32_t a = 0, b = 0;
        for (unsigned j = 0; j < eta; j++) {
            a += (x >> j) & 1u;
            b += (x >> (eta + j)) & 1u;
        }
        s[i] = a - b;
    }
}
