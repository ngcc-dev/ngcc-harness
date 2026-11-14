/* kem-24-3: characterize the submitted Scabbard-256 Minal decoder per
 * coordinate, for noise values that are multiples of four (the only values
 * decryption produces), over all 16 symbols and the whole two-coordinate grid.
 *   [S0, S1]: if both coordinates' noise lies here, every symbol decodes;
 *   [F0, F1]: if either coordinate's noise lies outside, no symbol decodes,
 *             whatever the other coordinate.
 * Prints "sure_success S0 S1 sure_failure_outside F0 F1 violations V" and
 * exits 0 when both statements hold on the full grid. */
#include <stdio.h>
#include <stdint.h>
#include "params.h"
#include "minal.h"

#define QQ (SCABBARD_Q + 1)
#define STEPS (QQ / 4)

static int decodes(unsigned v, int e0, int e1)
{
    uint8_t bits[4] = {(v >> 3) & 1, (v >> 2) & 1, (v >> 1) & 1, v & 1};
    int16_t cw[2], t[2];
    minal_b2_code_encode(cw, bits);
    t[0] = (int16_t)((cw[0] + e0) & SCABBARD_Q);
    t[1] = (int16_t)((cw[1] + e1) & SCABBARD_Q);
    return minal_b2_code_decode(t) == v;
}

static unsigned char ok[16][STEPS][STEPS];

int main(void)
{
    for (unsigned v = 0; v < 16; v++)
        for (int i = 0; i < STEPS; i++)
            for (int j = 0; j < STEPS; j++)
                ok[v][i][j] = (unsigned char)decodes(v, 4 * i - QQ / 2, 4 * j - QQ / 2);

    /* Grow the sure-success box symmetrically in index space from noise 0. */
    int z = QQ / 8, lo = z, hi = z;     /* index of noise 0 is QQ/8 */
    for (;;) {
        int nlo = lo - 1, nhi = hi + 1, good = nlo >= 0 && nhi < STEPS;
        for (unsigned v = 0; good && v < 16; v++)
            for (int i = nlo; good && i <= nhi; i++)
                for (int j = nlo; good && j <= nhi; j++)
                    good = ok[v][i][j];
        if (!good) break;
        lo = nlo; hi = nhi;
    }
    /* Extend each side separately while the box stays all-correct. */
    for (int side = 0; side < 2; side++) {
        for (;;) {
            int nlo = side ? lo : lo - 1, nhi = side ? hi + 1 : hi, good = nlo >= 0 && nhi < STEPS;
            for (unsigned v = 0; good && v < 16; v++)
                for (int i = nlo; good && i <= nhi; i++)
                    for (int j = nlo; good && j <= nhi; j++)
                        good = ok[v][i][j];
            if (!good) break;
            lo = nlo; hi = nhi;
        }
    }
    /* Possible-success range: noise values of either coordinate at which any
     * symbol decodes for some value of the other coordinate. */
    int flo = STEPS, fhi = -1;
    for (unsigned v = 0; v < 16; v++)
        for (int i = 0; i < STEPS; i++)
            for (int j = 0; j < STEPS; j++)
                if (ok[v][i][j]) {
                    if (i < flo) flo = i;
                    if (j < flo) flo = j;
                    if (i > fhi) fhi = i;
                    if (j > fhi) fhi = j;
                }
    long violations = 0;
    for (unsigned v = 0; v < 16; v++)
        for (int i = 0; i < STEPS; i++)
            for (int j = 0; j < STEPS; j++) {
                int sure_ok = lo <= i && i <= hi && lo <= j && j <= hi;
                int sure_bad = i < flo || i > fhi || j < flo || j > fhi;
                violations += (sure_ok && !ok[v][i][j]) || (sure_bad && ok[v][i][j]);
            }
    printf("sure_success %d %d sure_failure_outside %d %d violations %ld\n",
           4 * lo - QQ / 2, 4 * hi - QQ / 2, 4 * flo - QQ / 2, 4 * fhi - QQ / 2, violations);
    return violations != 0;
}
