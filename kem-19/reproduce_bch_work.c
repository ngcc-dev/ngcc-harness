/* Component witness for kem-19-3. No timing or key recovery is claimed. */
#include "Implementations and Test_Vectors/Implementations/Reference_Implementation/Lore-SM3/Lore-L2/bch_codec.c"

#include <stdio.h>
#include <string.h>

static int fail(const char *reason)
{
    fprintf(stderr, "NOT REPRODUCED kem-19-3: %s\n", reason);
    return 1;
}

int main(void)
{
    struct bch_control *bch = init_bch(LORE_BCH_M, LORE_BCH_T, 0);
    uint8_t data[LORE_MSG_BYTES] = {0};
    uint8_t ecc[LORE_ECC_BYTES] = {0};
    uint8_t changed_ecc[LORE_ECC_BYTES];
    unsigned int errloc[BCH_MAX_T] = {0};
    uint16_t syndrome[2 * BCH_MAX_T + 1];
    int clean, one, over;

    if (bch == NULL) return fail("BCH initialization failed");
    encode_bch(bch, data, sizeof data, ecc);
    clean = decode_bch(bch, data, sizeof data, ecc, NULL, NULL, errloc);
    compute_syndromes(bch, data, bch->msg_bits, ecc, syndrome);
    if (clean != 0 || !syndromes_are_zero(bch, syndrome)) {
        free_bch(bch);
        return fail("clean word did not take the zero-syndrome exit");
    }

    memcpy(changed_ecc, ecc, sizeof ecc);
    toggle_bit(changed_ecc, 507 - bch->msg_bits);
    one = decode_bch(bch, data, sizeof data, changed_ecc, NULL, NULL, errloc);
    compute_syndromes(bch, data, bch->msg_bits, changed_ecc, syndrome);
    if (one != 1 || errloc[0] != 507 || syndromes_are_zero(bch, syndrome)) {
        free_bch(bch);
        return fail("one-error word did not enter correction");
    }

    memset(data, 0, sizeof data);
    for (unsigned int i = 0; i < LORE_BCH_T + 1; i++) toggle_bit(data, i);
    over = decode_bch(bch, data, sizeof data, ecc, NULL, NULL, errloc);
    free_bch(bch);
    if (over != -1) return fail("beyond-radius word did not fail");

    printf("clean=%d one_error=%d beyond_radius=%d\n", clean, one, over);
    puts("COMPONENT kem-19-3 CONFIRMED: decoder takes input-dependent paths");
    puts("Whole-decapsulation timing and secret recovery were not tested.");
    return 0;
}
