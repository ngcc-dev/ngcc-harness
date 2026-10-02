/* sign-06-5: measure the delivered COMPASS-SIG challenge sampler against
 * Figure 1 of the specification.
 *
 * Built once per parameter set with -DCOMPASS_SIG_MODE=<N> against the archived
 * Reference_Implementation/COMPASS-SIG-<N> sources (see the Makefile target
 * reproduce-challenge-image), so poly_challenge() and its XOF are the
 * submitted ones. The program derives 20,000 challenge hashes from a fixed
 * label, expands each with poly_challenge(), and counts
 *
 *   - nonzero coefficients at positions [256, N-TAU), which Figure 1 reaches
 *     but a byte-wide position index cannot ("dead zone");
 *   - the number of -1 coefficients, which a 64-bit sign word caps at 64;
 *   - a position histogram chi-square statistic against the uniform
 *     TAU/N per-position rate implied by Figure 1.
 *
 * With an optional KAT file argument (NGCC "Sn = <hex>" records) it repeats
 * the dead-zone and sign counts on the shipped signatures' challenge hashes.
 *
 * Usage: reproduce_challenge_image [KAT_SIG_COMPASS-SIG-<N>.txt]
 */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "poly.h"
#include "symmetric.h"

#define TRIALS 20000
#define DEAD_LO 256

struct acc {
    long challenges;
    long dead_hits;
    long neg_sum;
    int neg_max;
    long malformed;
    unsigned long hist[N];
};

static void account(struct acc *a, const poly *c)
{
    int weight = 0, neg = 0;
    for (int i = 0; i < N; i++) {
        int32_t v = c->coeffs[i];
        if (v == 0)
            continue;
        if (v != 1 && v != -1)
            a->malformed++;
        weight++;
        if (v == -1)
            neg++;
        a->hist[i]++;
        if (i >= DEAD_LO && i < N - TAU)
            a->dead_hits++;
    }
    if (weight != TAU)
        a->malformed++;
    a->challenges++;
    a->neg_sum += neg;
    if (neg > a->neg_max)
        a->neg_max = neg;
}

static int hexval(int ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

/* Expand the challenge hash at the front of every "Sn = " record. */
static int account_kat(struct acc *a, const char *path)
{
    FILE *f = fopen(path, "r");
    char *line = NULL;
    size_t cap = 0;
    if (!f)
        return -1;
    while (getline(&line, &cap, f) > 0) {
        if (strncmp(line, "Sn = ", 5) != 0)
            continue;
        uint8_t seed[CTILDEBYTES];
        const char *p = line + 5;
        int ok = 1;
        for (int i = 0; i < CTILDEBYTES; i++) {
            int hi = hexval(p[2 * i]), lo = hexval(p[2 * i + 1]);
            if (hi < 0 || lo < 0) { ok = 0; break; }
            seed[i] = (uint8_t)(hi << 4 | lo);
        }
        if (!ok)
            continue;
        poly c;
        poly_challenge(&c, seed);
        account(a, &c);
    }
    free(line);
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    static struct acc fresh, kat;
    const int dead_width = N - TAU - DEAD_LO;
    const double per_position = (double)TRIALS * TAU / N;

    for (uint32_t i = 0; i < TRIALS; i++) {
        uint8_t in[32 + 4], seed[CTILDEBYTES];
        poly c;
        memcpy(in, "ngcc sign-06-5 challenge-image  ", 32);
        in[32] = (uint8_t)i; in[33] = (uint8_t)(i >> 8);
        in[34] = (uint8_t)(i >> 16); in[35] = (uint8_t)(i >> 24);
        shake256(seed, CTILDEBYTES, in, sizeof in);
        poly_challenge(&c, seed);
        account(&fresh, &c);
    }

    double chi2 = 0;
    for (int i = 0; i < N; i++) {
        double d = (double)fresh.hist[i] - per_position;
        chi2 += d * d / per_position;
    }
    const double chi2_df = chi2 / (N - 1);
    const double expected_dead = dead_width > 0
        ? (double)TRIALS * TAU * dead_width / N : 0.0;

    printf("COMPASS-SIG-%d: N=%d TAU=%d CTILDEBYTES=%d trials=%d\n",
           COMPASS_SIG_MODE, N, TAU, CTILDEBYTES, TRIALS);
    printf("  dead zone [%d,%d): width=%d nonzero_hits=%ld figure1_expected=%.0f\n",
           DEAD_LO, N - TAU, dead_width > 0 ? dead_width : 0,
           fresh.dead_hits, expected_dead);
    printf("  negative coefficients: mean=%.2f max=%d figure1_mean=%.1f\n",
           (double)fresh.neg_sum / fresh.challenges, fresh.neg_max, TAU / 2.0);
    printf("  position histogram chi-square/df=%.2f (df=%d) malformed=%ld\n",
           chi2_df, N - 1, fresh.malformed);

    if (argc > 1) {
        if (account_kat(&kat, argv[1]) == 0 && kat.challenges > 0)
            printf("  KAT %s: signatures=%ld dead_zone_hits=%ld negative_max=%d malformed=%ld\n",
                   argv[1], kat.challenges, kat.dead_hits, kat.neg_max, kat.malformed);
        else
            printf("  KAT %s: unreadable or no Sn records\n", argv[1]);
    } else {
        printf("  KAT: no file given\n");
    }

    if (fresh.malformed || kat.malformed) {
        printf("UNEXPECTED COMPASS-SIG-%d: malformed challenge output\n", COMPASS_SIG_MODE);
        return 1;
    }
    if (N == 512) {
        int ok = fresh.dead_hits == 0 && fresh.neg_max <= 64 && chi2_df > 100.0
                 && (kat.challenges == 0 || (kat.dead_hits == 0 && kat.neg_max <= 64));
        if (!ok) {
            printf("UNEXPECTED COMPASS-SIG-%d: sampler does not show the reported structure\n",
                   COMPASS_SIG_MODE);
            return 1;
        }
        printf("CONFIRMED sign-06-5 COMPASS-SIG-%d: %d positions never set, at most 64 negative signs\n",
               COMPASS_SIG_MODE, dead_width);
        return 0;
    }
    if (dead_width > 0 || TAU > 64 || chi2_df > 2.0) {
        printf("UNEXPECTED COMPASS-SIG-%d: control tier is not clean\n", COMPASS_SIG_MODE);
        return 1;
    }
    printf("CONTROL COMPASS-SIG-%d: full position range and %d<=64 sign bits, chi-square/df=%.2f\n",
           COMPASS_SIG_MODE, TAU, chi2_df);
    return 0;
}
