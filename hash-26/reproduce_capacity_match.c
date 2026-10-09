/* Test the submitted CHIME-1024 rate/capacity and feed-forward behavior. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CHIME/Implementations/Reference_Implementation/CHIME-1024/CryptHash_AlgorithmInstance.c"

static uint64_t rng_state = UINT64_C(0x6812d3a70f94c5be);
static uint64_t rnd(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static void step(p_state_t *state, const uint64_t rate[7]) {
    p_state_t old = *state;
    for (int i = 0; i < 4; ++i) state->A0[i] ^= rate[i];
    for (int i = 1; i < 4; ++i) state->A1[i] ^= rate[i + 3];
    p_pn(state, 20);
    feedforward_inner_1088(&old, state);
}

static int restricted(const p_state_t *s) {
    const uint64_t *groups[5] = {s->A0, s->B0, s->B1, s->C0, s->C1};
    for (int g = 0; g < 5; ++g)
        for (int i = 1; i < 4; ++i)
            if (groups[g][i] != groups[g][0]) return 0;
    for (int i = 2; i < 4; ++i)
        if (s->A1[i] != s->A1[1]) return 0;
    return 1;
}

int main(void) {
    p_state_t s = {0};
    for (int trial = 0; trial < 100; ++trial) {
        uint64_t rate[7] = {0};
        const uint64_t word = rnd();
        for (int i = 0; i < 4; ++i) rate[i] = word;
        for (int i = 1; i < 4; ++i) rate[i + 3] = s.A1[i] ^ s.A1[0];
        step(&s, rate);
        if (!restricted(&s)) return 1;
    }
    puts("STRUCTURAL hash-26-1: 100 controlled blocks keep five capacity words folded");

    for (int trial = 0; trial < 100; ++trial) {
        p_state_t left = {0}, right;
        uint64_t rate_left[7], rate_right[7];
        uint64_t *words = (uint64_t *)&left;
        for (int i = 0; i < 24; ++i) words[i] = rnd();
        right = left;
        for (int i = 0; i < 4; ++i) right.A0[i] ^= rnd();
        for (int i = 1; i < 4; ++i) right.A1[i] ^= rnd();
        for (int i = 0; i < 7; ++i) rate_left[i] = rnd();
        for (int i = 0; i < 4; ++i)
            rate_right[i] = rate_left[i] ^ left.A0[i] ^ right.A0[i];
        for (int i = 1; i < 4; ++i)
            rate_right[i + 3] = rate_left[i + 3] ^ left.A1[i] ^ right.A1[i];
        step(&left, rate_left);
        step(&right, rate_right);
        if (memcmp(&left, &right, sizeof left) != 0) return 1;
    }
    puts("STRUCTURAL hash-26-1: equal-capacity states align after one chosen block");
    puts("FULL COLLISION hash-26-1 NOT RUN: capacity birthday search is 2^160");
    return 0;
}
