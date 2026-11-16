/* sign-02-4: BiT-512 hint bytes have several encodings that the reference
 * verifier accepts as the same signature.
 *
 * Signs one message, then for each of the three hint polynomials rewrites the
 * final byte (four real coefficients and one unused base-3 digit) with each of
 * the three values of the unused digit, and rewrites one complete-group byte
 * whose value is at most 12 by adding 243. Counts accepted signatures that
 * differ from the original as byte strings, and checks that every accepted
 * variant is rejected for a different message. */
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "sign.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    static unsigned char pk[BIT_PUBLICKEYBYTES], sk[BIT_SECRETKEYBYTES];
    static unsigned char sig[BIT_SIGNBYTES], alt[BIT_SIGNBYTES];
    unsigned char seed[48], m[32], m2[32];
    unsigned long long pkl, skl, sl;
    const size_t hint = BIT_CHALLENGEBYTES + BIT_POLY_Z0_BYTES + BIT_POLYVECL_Z1_BYTES;
    int honest, tail_ok = 0, tail_new = 0, full_ok = 0, full_new = 0, other_rejected = 1;

    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(7 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
    for (int i = 0; i < 32; i++) { m[i] = (unsigned char)i; m2[i] = (unsigned char)(i ^ 0x33); }
    bit_sig_keygen(pk, &pkl, sk, &skl);
    bit_sig_sign(sk, skl, m, 32, sig, &sl);
    honest = bit_sig_verify(sig, sl, m, 32, pk, pkl);

    for (int k = 0; k < BIT_K; k++) {
        size_t last = hint + (size_t)(k + 1) * BIT_POLY_H_BASE3_BYTES - 1;
        unsigned low = sig[last] % 81;                 /* four real coefficients */
        for (unsigned digit = 0; digit < 3; digit++) {
            memcpy(alt, sig, sl);
            alt[last] = (unsigned char)(low + 81 * digit);
            if (bit_sig_verify(alt, sl, m, 32, pk, pkl) == 0) {
                tail_ok++;
                tail_new += memcmp(alt, sig, sl) != 0;
                other_rejected &= bit_sig_verify(alt, sl, m2, 32, pk, pkl) != 0;
            }
        }
    }
    for (size_t i = hint; i < hint + BIT_POLYVECK_H_BASE3_BYTES; i++) {
        if ((i - hint + 1) % BIT_POLY_H_BASE3_BYTES == 0 || sig[i] > 12) continue;
        memcpy(alt, sig, sl);
        alt[i] = (unsigned char)(sig[i] + 243);
        if (bit_sig_verify(alt, sl, m, 32, pk, pkl) == 0) {
            full_ok++;
            full_new++;
            other_rejected &= bit_sig_verify(alt, sl, m2, 32, pk, pkl) != 0;
        }
        break;
    }
    printf("honest=%d final_byte_variants_accepted=%d distinct=%d complete_group_alias_accepted=%d "
           "other_message_rejected=%d\n", honest, tail_ok, tail_new, full_ok, other_rejected);
    return 0;
}
