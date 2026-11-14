/* kem-24-3: histogram of the decryption noise m' - mu, per coefficient, for
 * honest Scabbard PKE encryptions. Built against one Scabbard reference tree.
 * Usage: noise KEYS ENCRYPTIONS_PER_KEY SEED_BYTE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "params.h"
#include "drng.h"
#include "indcpa.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"

DRNG_ctx drng_algorithm;
static unsigned long long hist[SCABBARD_Q + 1];

int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    long keys = atol(argv[1]), encs = atol(argv[2]);
    unsigned char seed[64];
    memset(seed, 0x5c, sizeof seed);
    seed[0] = (unsigned char)atoi(argv[3]);
    init_random_number(&drng_algorithm, seed, sizeof seed);

    static uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], sk[SCABBARD_INDCPA_SECRETKEYBYTES];
    static uint8_t ct[SCABBARD_INDCPA_BYTES];
    uint8_t msg[SCABBARD_INDCPA_MSGBYTES], out[SCABBARD_INDCPA_MSGBYTES], r[SCABBARD_SYMBYTES];
    long msg_failures = 0;

    for (long k = 0; k < keys; k++) {
        indcpa_keypair(pk, sk);
        for (long e = 0; e < encs; e++) {
            get_random_number(&drng_algorithm, msg, sizeof msg * 8);
            get_random_number(&drng_algorithm, r, sizeof r * 8);
            indcpa_enc(ct, msg, pk, r);

            /* Algorithm 11 lines 7-8 as in indcpa_dec, stopping before decoding. */
            polyvec skpv, u;
            poly v, w, mu;
            unpack_ciphertext(&u, &v, ct);
            unpack_sk(&skpv, sk);
            inner_prod(&w, &u, &skpv);
            poly_frommsg(&mu, msg);
            for (int i = 0; i < SCABBARD_N; i++) {
                int a = (v.coeffs[i] & SCABBARD_Q) << (SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B));
                int b = (w.coeffs[i] & SCABBARD_P) << (SCABBARD_EQ - SCABBARD_EP);
                hist[(b - a - mu.coeffs[i]) & SCABBARD_Q]++;
            }
            indcpa_dec(out, ct, sk);
            msg_failures += memcmp(out, msg, sizeof msg) != 0;
        }
    }
    for (int d = 0; d <= SCABBARD_Q; d++)
        if (hist[d])
            printf("H %d %llu\n", d >= (SCABBARD_Q + 1) / 2 ? d - (SCABBARD_Q + 1) : d, hist[d]);
    printf("S encryptions=%ld msg_failures=%ld\n", keys * encs, msg_failures);
    return 0;
}
