/*
 * AXIS (hash-02): the UpFull beat is a permutation of the 1536-bit state for
 * every message-bit pair, so the whole state can be computed backwards.
 *
 * Each register update reads only ExL/ExL_prev positions {0,11,12,32,96,107,
 * 125}, flips only UpL positions {66,75,90,162,178,188}, and then rotates the
 * register by one bit.  Registers are updated in order 0..7, so undoing them
 * in order 7..0 sees exactly the neighbour values the forward beat used.
 *
 * This witness runs the submission's own axis_step() forward for random
 * states and message bits, applies the inverse below, and checks that the
 * original state is restored, for all three variants.
 */
#include "axis_core.c"

#include <stdio.h>

static uint64_t rng = 0x9E3779B97F4A7C15ULL;
static uint64_t next64(void)
{
    rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
    return rng;
}

static void rotate_back(uint64_t r[AXIS_REGISTER_WORDS])
{
    const uint64_t top = r[2] >> 63;
    r[2] = (r[2] << 1) | (r[1] >> 63);
    r[1] = (r[1] << 1) | (r[0] >> 63);
    r[0] = (r[0] << 1) | top;
}

static void inverse_step(axis_core_t* ctx, uint8_t m0, uint8_t m1)
{
    const axis_variant_config_t* c = ctx->config;
    const uint32_t n = c->register_count;
    for (uint32_t j = n; j-- > 0;) {
        uint64_t* r = ctx->regs[j];
        const uint32_t prev = (j == 0U) ? (n - 1U) : (j - 1U);
        const uint32_t n1 = (j + 1U) % n, n3 = (j + 3U) % n, n5 = (j + 5U) % n;
        const uint8_t m = ((j & 1U) == 0U) ? m0 : m1;
        rotate_back(r);  /* read positions are untouched by the flips */
        const uint8_t b = (uint8_t)(m
            ^ axis_sbox6(axis_get_bit(r, c->exl[5]), axis_get_bit(r, c->exl[2]),
                         axis_get_bit(r, c->exl[4]), axis_get_bit(r, c->exl[1]),
                         axis_get_bit(r, c->exl[3]), axis_get_bit(r, c->exl[0]))
            ^ axis_sbox6(axis_get_bit(ctx->regs[n5], c->exl[3]), axis_get_bit(ctx->regs[n5], c->exl[2]),
                         axis_get_bit(ctx->regs[n3], c->exl[5]), axis_get_bit(ctx->regs[n3], c->exl[1]),
                         axis_get_bit(ctx->regs[n1], c->exl[4]), axis_get_bit(ctx->regs[n1], c->exl[0]))
            ^ axis_get_bit(ctx->regs[prev], c->exl_prev));
        const uint8_t t0 = (uint8_t)(axis_get_bit(r, c->exl[0]) ^ axis_get_bit(r, c->exl[1])
                                     ^ axis_get_bit(r, c->exl[2]) ^ b);
        const uint8_t t1 = (uint8_t)(axis_get_bit(r, c->exl[3]) ^ axis_get_bit(r, c->exl[4])
                                     ^ axis_get_bit(r, c->exl[5]) ^ b);
        for (int k = 0; k < 3; k++) axis_flip_bit(r, c->upl[k], t1);
        for (int k = 3; k < 6; k++) axis_flip_bit(r, c->upl[k], t0);
    }
}

int main(void)
{
    static const axis_variant_t variants[] = {AXIS_VARIANT_512, AXIS_VARIANT_768, AXIS_VARIANT_1024};
    static const char* names[] = {"AXIS-512", "AXIS-768", "AXIS-1024"};
    enum { TRIALS = 200, BEATS = 4096 };
    static uint8_t m0[BEATS], m1[BEATS];
    int failures = 0;

    for (int v = 0; v < 3; v++) {
        int ok = 0;
        for (int t = 0; t < TRIALS; t++) {
            axis_core_t ctx;
            uint64_t start[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS];
            axis_core_init(&ctx, variants[v]);
            for (int j = 0; j < AXIS_MAX_REGISTERS; j++)
                for (int w = 0; w < AXIS_REGISTER_WORDS; w++)
                    ctx.regs[j][w] = next64();
            memcpy(start, ctx.regs, sizeof(start));
            for (int i = 0; i < BEATS; i++) {
                const uint64_t x = next64();
                m0[i] = (uint8_t)(x & 1U);
                m1[i] = (uint8_t)((x >> 1) & 1U);
                axis_step(&ctx, m0[i], m1[i]);
            }
            for (int i = BEATS; i-- > 0;)
                inverse_step(&ctx, m0[i], m1[i]);
            ok += memcmp(start, ctx.regs, sizeof(start)) == 0;
        }
        printf("%s: %d/%d random states restored after %d forward and inverse beats\n",
               names[v], ok, TRIALS, BEATS);
        failures += ok != TRIALS;
    }
    if (failures == 0) {
        printf("ATTACK hash-02-3 CONFIRMED: the AXIS beat is invertible for known message bits\n");
        return 0;
    }
    printf("FAIL   hash-02-3: inverse did not restore the state\n");
    return 1;
}
