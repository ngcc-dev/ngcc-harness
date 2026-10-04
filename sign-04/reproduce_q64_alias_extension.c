/* Conservative certificate for the sign-04-2 q=2^64 forgery extension.
 *
 * This models CEDRUSALPHA-160f at one complete bottom address.  The submitted
 * one-byte chain address pools each leaf across 14 same-parity FORC
 * coordinates.  We deliberately allow only one level of public Merkle
 * closure; the unrestricted attack can close known children repeatedly and
 * is stronger.  The Monte Carlo is deterministic and the reported Poisson
 * mixture omits nonnegative occupancy tails.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#define B 128
#define W 4
#define K 28
#define RMAX 18

static inline uint64_t splitmix64(uint64_t *x)
{
    uint64_t z = (*x += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

static long double one_transcript(int r, uint64_t seed)
{
    unsigned char xs[RMAX][K], min_pos[2][B];
    uint64_t state = seed;
    long double product = 1.0L;
    int i, r_i, x;

    for (i = 0; i < 2; i++)
        for (x = 0; x < B; x++)
            min_pos[i][x] = W;

    for (r_i = 0; r_i < r; r_i++) {
        for (i = 0; i < K; i++) {
            uint64_t z = splitmix64(&state);
            int pos;
            xs[r_i][i] = z & (B - 1);
            pos = (z >> 7) & (W - 1);
            if (pos < min_pos[i & 1][xs[r_i][i]])
                min_pos[i & 1][xs[r_i][i]] = pos;
        }
    }

    for (i = 0; i < K; i++) {
        unsigned char known[8][B] = {{0}};
        int good = 0;

        /* Any aliased disclosure can be advanced to its chain endpoint. */
        for (x = 0; x < B; x++)
            if (min_pos[i & 1][x] < W)
                known[0][x] = 1;

        /* Each signature reveals one sibling at every Merkle height. */
        for (r_i = 0; r_i < r; r_i++) {
            x = xs[r_i][i];
            for (int height = 0; height < 7; height++)
                known[height][(x >> height) ^ 1] = 1;
        }

        /* Conservative restriction: close leaf pairs only, not higher nodes. */
        for (x = 0; x < B / 2; x++)
            if (known[0][2 * x] && known[0][2 * x + 1])
                known[1][x] = 1;

        for (x = 0; x < B; x++) {
            int auth = min_pos[i & 1][x] < W;
            for (int height = 0; height < 7; height++)
                auth &= known[height][(x >> height) ^ 1];
            if (auth)
                good += W - min_pos[i & 1][x];
        }
        product *= (long double)good / (B * W);
    }
    return product;
}

static long double log2_poisson_mass(int r)
{
    const long double lambda = 0.25L; /* 2^64 signatures / 2^66 addresses */
    return (-lambda + r * logl(lambda) - lgammal(r + 1)) / logl(2.0L);
}

int main(int argc, char **argv)
{
    uint64_t samples = 200000;
    long double success = 0.0L;

    if (argc == 2)
        samples = strtoull(argv[1], NULL, 0);
    if (samples < 10000) {
        fprintf(stderr, "need at least 10000 samples per occupancy\n");
        return 2;
    }

    for (int r = 6; r <= 18; r++) {
        long double sum = 0.0L;
#pragma omp parallel for reduction(+:sum) schedule(static)
        for (uint64_t j = 0; j < samples; j++) {
            uint64_t seed = UINT64_C(0xd1b54a32d192ed03) * (j + 1)
                          + (uint64_t)r * UINT64_C(0x9e3779b97f4a7c15);
            sum += one_transcript(r, seed);
        }
        success += exp2l(log2_poisson_mass(r)) * (sum / samples);
    }

    long double log_success = log2l(success);
    printf("signatures_log2=64.000000\n");
    printf("restricted_closure_log2_success=%.6Lf\n", log_success);
    printf("restricted_closure_log2_target_trials=%.6Lf\n", -log_success);

    if (!(log_success > -124.0L && log_success < -122.5L)) {
        puts("NOT-CONFIRMED sign-04-2");
        return 1;
    }
    puts("PROBABLE sign-04-2: the one-byte FORC aliases give a below-2^128 forgery estimate with 2^64 signatures");
    return 0;
}
