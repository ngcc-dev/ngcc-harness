/* kem-34-6: compare the archived Rudraksh2 Minal decoder on the coefficient
 * representatives that poly_sub can hand it with the same decoder on fully
 * reduced inputs, over every reachable pair.
 *
 * Prints the modulus, the largest poly_sub output, the number of reachable
 * pairs with a coefficient at or above q, and the number of those that decode
 * to a different symbol. For the differing pairs it prints the smallest
 * distance to any codeword, half the code's minimum distance, how many lie
 * strictly inside that half distance of a codeword, and how often the shipped
 * and the reduced decoding return the nearest codeword. Last, both decodings
 * of the pair (2914, 3443). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "minal.h"
#include "reduce.h"

static int dec(int a, int b)
{
    int16_t x[2] = {(int16_t)a, (int16_t)b};
    return minal_b2_code_decode(x);
}

static long dist2(int t0, int t1, const int16_t cw[2])
{
    long best = 1L << 40;
    for (int s0 = -1; s0 <= 1; s0++)
        for (int s1 = -1; s1 <= 1; s1++) {
            long d0 = t0 - cw[0] + s0 * KEM_Q, d1 = t1 - cw[1] + s1 * KEM_Q, d2 = d0 * d0 + d1 * d1;
            if (d2 < best) best = d2;
        }
    return best;
}

int main(void)
{
    static unsigned char reach[16384];
    int16_t cw[16][2];
    int maxout = 0;
    long pairs = 0, differ = 0, inside = 0, shipped = 0, reduced = 0, mind2 = 1L << 40, near2 = 1L << 40;

    /* poly_sub computes barrett_reduce(a + 3q - b); take a, b over [0, q). */
    for (int a = 0; a < KEM_Q; a++)
        for (int b = 0; b < KEM_Q; b++) {
            int o = barrett_reduce((uint16_t)(a + 3 * KEM_Q - b));
            reach[o] = 1;
            if (o > maxout) maxout = o;
        }
    for (int i = 0; i < 16; i++) {
        uint8_t bits[4] = {(i >> 3) & 1, (i >> 2) & 1, (i >> 1) & 1, i & 1};
        minal_b2_code_encode(cw[i], bits);
        cw[i][0] %= KEM_Q;
        cw[i][1] %= KEM_Q;
    }
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 16; j++) {
            long d = i == j ? 1L << 40 : dist2(cw[i][0], cw[i][1], cw[j]);
            if (d < mind2) mind2 = d;
        }
    for (int t0 = 0; t0 <= maxout; t0++)
        for (int t1 = 0; t1 <= maxout; t1++) {
            if (!reach[t0] || !reach[t1] || (t0 < KEM_Q && t1 < KEM_Q)) continue;
            int r0 = t0 % KEM_Q, r1 = t1 % KEM_Q, p = dec(t0, t1), r = dec(r0, r1), arg = -1;
            long best = 1L << 40;
            pairs++;
            if (p == r) continue;
            differ++;
            for (int i = 0; i < 16; i++) {
                long d = dist2(r0, r1, cw[i]);
                if (d < best) { best = d; arg = i; }
            }
            if (best < near2) near2 = best;
            inside += 4 * best < mind2;
            shipped += arg == p;
            reduced += arg == r;
        }
    printf("q=%d max_output=%d noncanonical_pairs=%ld differing=%ld nearest_codeword=%.1f "
           "half_min_distance=%.1f inside=%ld shipped_nearest=%ld reduced_nearest=%ld witness=%d/%d\n",
           KEM_Q, maxout, pairs, differ, differ ? sqrt((double)near2) : 0.0, sqrt((double)mind2) / 2,
           inside, shipped, reduced, dec(2914, 3443), dec(2914 % KEM_Q, 3443 % KEM_Q));
    return 0;
}
