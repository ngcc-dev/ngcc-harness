/* sign-15-7 ATLAS-192: public boundary half-spaces of honest signatures
 * correlate with s1 under a specification-conforming mask decoder.
 *
 * Compiled by reproduce_atlas192_mask_leak.sh inside a temporary copy of
 * Implementation/Reference_Implementation/lwrdsa192 whose rej_gamma1m1()
 * decodes the two independent 20-bit values the specification requires.
 *
 *   acc    SEED start count out.bin      sign messages [start, start+count),
 *                                        accumulate a weighted public estimate
 *   corr   SEED CONTROLSEED part.bin...  correlate the summed estimate with
 *                                        the true and a control key
 *   events SEED start count W out.bin    boundary events for the LP analysis
 *   s1     SEED out.bin                  write centred s1 (validation only)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "params.h"
#include "SIG_AlgorithmInstance.h"
#include "packing.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

#define WEIGHT_WINDOW 60   /* events within 60 of the acceptance bound */

static int32_t cen(uint32_t a)
{
    a &= (Q - 1);
    return (a > Q / 2) ? (int32_t)a - (int32_t)Q : (int32_t)a;
}

static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
static unsigned long long pl, sl;
static int32_t s1c[L][N];

static void keygen(const char *seedstr)
{
    unsigned char seed[64], rho[SEEDBYTES], key[SEEDBYTES], tr[CRHBYTES];
    polyvecl s1;
    polyveck t0;
    memset(seed, 0, sizeof seed);
    strncpy((char *)seed, seedstr, 63);
    init_random_number(&drng_algorithm, seed, 64);
    if (sig_keygen(pk, &pl, sk, &sl) != 0) { fprintf(stderr, "keygen failed\n"); exit(2); }
    unpack_sk(rho, key, tr, &s1, &t0, sk);
    for (unsigned i = 0; i < L; i++)
        for (unsigned j = 0; j < N; j++)
            s1c[i][j] = cen(s1.vec[i].coeffs[j]);
}

/* row r with (c*s)_i = <r, s> in Z[x]/(x^N + 1) */
static void negrow(const int8_t *c, unsigned i, int32_t *r)
{
    memset(r, 0, N * sizeof *r);
    for (unsigned k = 0; k < N; k++) {
        if (!c[k]) continue;
        if (k <= i) r[i - k] += c[k];
        else r[i + N - k] -= c[k];
    }
}

static unsigned char *sn;

static void sign_one(long m, unsigned char msg[32], polyvecl *z, int8_t c8[N])
{
    unsigned long long snl;
    polyveck h;
    poly c;
    memset(msg, 0, 32);
    memcpy(msg, &m, sizeof m);
    sig_sign(sk, sl, msg, 32, sn, &snl);
    unpack_sig(z, &h, &c, sn);
    for (unsigned i = 0; i < N; i++) c8[i] = (int8_t)cen(c.coeffs[i]);
}

static int mode_acc(int argc, char **argv)
{
    if (argc != 6) return 2;
    long start = atol(argv[3]), count = atol(argv[4]);
    keygen(argv[2]);
    static int64_t X[L][N];
    const int32_t B = (int32_t)GAMMA1 - (int32_t)BETA1 - 1;
    long events = 0, violations = 0, verified = 0, bad = 0, mut_ok = 0;
    int32_t r[N];
    for (long m = start; m < start + count; m++) {
        unsigned char msg[32];
        polyvecl z;
        int8_t c8[N];
        sign_one(m, msg, &z, c8);
        if (m - start < 20) {
            unsigned char mm[32];
            memcpy(mm, msg, 32);
            verified++;
            if (sig_verify(pk, pl, sn, CRYPTO_BYTES, mm, 32) != 0) bad++;
            mm[31] ^= 1;   /* changed-message control must reject */
            if (sig_verify(pk, pl, sn, CRYPTO_BYTES, mm, 32) == 0) mut_ok++;
        }
        for (unsigned j = 0; j < L; j++)
            for (unsigned i = 0; i < N; i++) {
                int32_t v = cen(z.vec[j].coeffs[i]);
                int32_t d = B - (v < 0 ? -v : v);
                if (d < 0 || d >= WEIGHT_WINDOW) continue;
                negrow(c8, i, r);
                int64_t cs = 0;
                for (unsigned k = 0; k < N; k++) cs += (int64_t)r[k] * s1c[j][k];
                /* y = z - c*s1 lies in [-(gamma1-1), gamma1-1] */
                if (v > 0 ? cs < (int64_t)v - (int64_t)GAMMA1 + 1
                          : cs > (int64_t)v + (int64_t)GAMMA1 - 1)
                    violations++;
                int64_t w = (WEIGHT_WINDOW - d) * (v > 0 ? 1 : -1);
                for (unsigned k = 0; k < N; k++) X[j][k] += w * r[k];
                events++;
            }
    }
    FILE *f = fopen(argv[5], "wb");
    if (!f) return 2;
    int64_t hdr[6] = {count, events, violations, verified, bad, mut_ok};
    fwrite(hdr, sizeof hdr, 1, f);
    fwrite(X, sizeof X, 1, f);
    fclose(f);
    return 0;
}

static double corr(const double *a, const int32_t *b, size_t n)
{
    double ma = 0, mb = 0, sab = 0, saa = 0, sbb = 0;
    for (size_t i = 0; i < n; i++) { ma += a[i]; mb += b[i]; }
    ma /= n; mb /= n;
    for (size_t i = 0; i < n; i++) {
        double x = a[i] - ma, y = b[i] - mb;
        sab += x * y; saa += x * x; sbb += y * y;
    }
    return sab / sqrt(saa * sbb);
}

static int mode_corr(int argc, char **argv)
{
    if (argc < 5) return 2;
    static int64_t X[L][N], part[L][N];
    static double Xd[L * N];
    static int32_t sa[L][N], sb[L][N];
    int64_t tot[6] = {0};
    for (int a = 4; a < argc; a++) {
        int64_t hdr[6];
        FILE *f = fopen(argv[a], "rb");
        if (!f || fread(hdr, sizeof hdr, 1, f) != 1 || fread(part, sizeof part, 1, f) != 1) {
            fprintf(stderr, "cannot read %s\n", argv[a]);
            return 2;
        }
        fclose(f);
        for (int t = 0; t < 6; t++) tot[t] += hdr[t];
        for (unsigned j = 0; j < L; j++)
            for (unsigned k = 0; k < N; k++) X[j][k] += part[j][k];
    }
    keygen(argv[2]); memcpy(sa, s1c, sizeof sa);
    keygen(argv[3]); memcpy(sb, s1c, sizeof sb);
    for (unsigned j = 0; j < L; j++)
        for (unsigned k = 0; k < N; k++) Xd[j * N + k] = (double)X[j][k];
    printf("signatures=%lld events=%lld halfspace_violations=%lld "
           "verified=%lld verify_failures=%lld mutated_accepted=%lld\n",
           (long long)tot[0], (long long)tot[1], (long long)tot[2],
           (long long)tot[3], (long long)tot[4], (long long)tot[5]);
    printf("corr_true=%.4f corr_control=%.4f one_over_sqrt_n=%.4f\n",
           corr(Xd, &sa[0][0], L * N), corr(Xd, &sb[0][0], L * N), 1.0 / sqrt(L * N));
    return 0;
}

static int mode_events(int argc, char **argv)
{
    if (argc != 7) return 2;
    long start = atol(argv[3]), count = atol(argv[4]);
    int W = atoi(argv[5]);
    keygen(argv[2]);
    FILE *out = fopen(argv[6], "wb");
    if (!out) return 2;
    const int32_t B = (int32_t)GAMMA1 - (int32_t)BETA1 - 1;
    for (long m = start; m < start + count; m++) {
        unsigned char msg[32];
        polyvecl z;
        int8_t c8[N];
        uint16_t ne = 0;
        static uint8_t ej[4096], ei[4096];
        static int32_t ez[4096];
        sign_one(m, msg, &z, c8);
        for (unsigned j = 0; j < L; j++)
            for (unsigned i = 0; i < N; i++) {
                int32_t v = cen(z.vec[j].coeffs[i]);
                if ((v >= B - W || v <= -B + W) && ne < 4096) {
                    ej[ne] = (uint8_t)j; ei[ne] = (uint8_t)i; ez[ne] = v; ne++;
                }
            }
        fwrite(c8, 1, N, out);
        fwrite(&ne, 2, 1, out);
        for (unsigned e = 0; e < ne; e++) {
            fwrite(&ej[e], 1, 1, out); fwrite(&ei[e], 1, 1, out); fwrite(&ez[e], 4, 1, out);
        }
    }
    fclose(out);
    return 0;
}

static int mode_s1(int argc, char **argv)
{
    if (argc != 4) return 2;
    keygen(argv[2]);
    FILE *f = fopen(argv[3], "wb");
    if (!f) return 2;
    fwrite(s1c, sizeof s1c, 1, f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    sn = malloc(CRYPTO_BYTES + 256);
    if (!sn || argc < 2) return 2;
    if (!strcmp(argv[1], "acc")) return mode_acc(argc, argv);
    if (!strcmp(argv[1], "corr")) return mode_corr(argc, argv);
    if (!strcmp(argv[1], "events")) return mode_events(argc, argv);
    if (!strcmp(argv[1], "s1")) return mode_s1(argc, argv);
    return 2;
}
