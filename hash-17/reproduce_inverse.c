#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "MasterCube/Implementations/Reference_Implementation/MasterCube-512/MasterCube-512.c"

static void swap_halves(struct mc_state *s)
{
    struct xy_slice t;
    memcpy(&t, &s->data.halves.left, sizeof(t));
    memcpy(&s->data.halves.left, &s->data.halves.right, sizeof(t));
    memcpy(&s->data.halves.right, &t, sizeof(t));
}

static void forward_round(struct mc_state *s, int rn)
{
    CrossMixs(s); SwapRows(s);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 0, 1, 8);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 14, 5, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 9, 12, 8);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 14, 5, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 0, 1, 8);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 8, 9, 0);
    CrossMixs(s); SwapRows(s); MixColumns(s);
    ShiftRows(&s->data.halves.left, &s->data.halves.right);
    MixRows(s); AddConstant(s, rn);
}

static void submitted_inverse_round(struct mc_state *s, int rn)
{
    AddConstant(s, rn); MixRows(s);
    ShiftRows(&s->data.halves.right, &s->data.halves.left);
    MixColumns(s); SwapRows(s); CrossMixs(s);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 8, 9, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 0, 1, 8);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 14, 5, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 9, 12, 8);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 14, 5, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 0, 1, 8);
    swap_halves(s); SwapRows(s); CrossMixs(s);
}

static void corrected_inverse_round(struct mc_state *s, int rn)
{
    AddConstant(s, rn); MixRows(s);
    ShiftRows(&s->data.halves.right, &s->data.halves.left);
    swap_halves(s); MixColumns(s); swap_halves(s);
    SwapRows(s); CrossMixs(s);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 8, 9, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 0, 1, 8);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 14, 5, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 9, 12, 8);
    MAndRXs(&s->data.halves.right, &s->data.halves.left, 14, 5, 0);
    MAndRXs(&s->data.halves.left, &s->data.halves.right, 0, 1, 8);
    SwapRows(s); CrossMixs(s);
}

int main(void)
{
    struct mc_state original = {0}, bad, good;
    unsigned i;

    for (i = 0; i < 48; i++)
        original.data.halves.left.data.raw_data[i] = 1;
    bad = good = original;
    forward_round(&bad, 9); submitted_inverse_round(&bad, 9);
    forward_round(&good, 9); corrected_inverse_round(&good, 9);

    if (memcmp(&good, &original, sizeof(good)) != 0) {
        puts("FAIL: corrected inverse did not recover the input");
        return 1;
    }
    if (memcmp(&bad.data.halves.left, &original.data.halves.right,
               sizeof(struct xy_slice)) != 0 ||
        memcmp(&bad.data.halves.right, &original.data.halves.left,
               sizeof(struct xy_slice)) != 0) {
        puts("FAIL: submitted inverse produced an unexpected witness");
        return 1;
    }
    puts("CONFIRMED: forward round followed by submitted inverse swaps the slices");
    puts("CONTROL: corrected inverse recovers the input exactly");
    return 0;
}
