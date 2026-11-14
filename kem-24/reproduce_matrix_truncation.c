/* kem-24-4: compare the archived gen_a coefficients with the specification's
 * BytesToPoly (Algorithm 2: little-endian eps_q-bit fields) applied to the same
 * pseudoXOF output. Built against one Scabbard reference tree. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "params.h"
#include "drng.h"
#include "auxfunc.h"
#include "polyvec.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    static polyvec a[SCABBARD_L];
    uint8_t seed[SCABBARD_SYMBYTES], ext[SCABBARD_SYMBYTES + 2], buf[SCABBARD_POLYBYTES];
    unsigned max_by_pos[4] = {0, 0, 0, 0};
    unsigned long total = 0, mismatches = 0;

    for (int s = 0; s < 8; s++) {
        for (size_t i = 0; i < sizeof seed; i++)
            seed[i] = (uint8_t)(37 * i + 101 * s + 1);
        gen_a(a, seed);
        memcpy(ext, seed, SCABBARD_SYMBYTES);
        for (int i = 0; i < SCABBARD_L; i++) {
            for (int j = 0; j < SCABBARD_L; j++) {
                ext[SCABBARD_SYMBYTES + 0] = (uint8_t)i;
                ext[SCABBARD_SYMBYTES + 1] = (uint8_t)j;
                pseudoXOF(SCABBARD_POLYBYTES * 8, ext, (SCABBARD_SYMBYTES + 2) * 8, buf);
                for (int k = 0; k < SCABBARD_N; k++) {
                    unsigned spec = 0;
                    for (int b = 0; b < SCABBARD_EQ; b++) {
                        int bit = SCABBARD_EQ * k + b;
                        spec |= (unsigned)((buf[bit >> 3] >> (bit & 7)) & 1) << b;
                    }
                    unsigned got = (unsigned)a[i].vec[j].coeffs[k] & SCABBARD_Q;
                    if (got > max_by_pos[k & 3])
                        max_by_pos[k & 3] = got;
                    mismatches += got != spec;
                    total++;
                }
            }
        }
    }
    printf("q=%d coefficients=%lu mismatches=%lu max_by_index_mod4=%u,%u,%u,%u\n",
           SCABBARD_Q + 1, total, mismatches,
           max_by_pos[0], max_by_pos[1], max_by_pos[2], max_by_pos[3]);
    return 0;
}
