/* hash-22-1: execution depends on secret message bits at a fixed length.
 *
 * Modes (DBITS = digest bits, MBITS = message bits):
 *   taint DBITS MBITS       hash a message whose bytes are marked undefined;
 *                           run under Valgrind memcheck
 *   one DBITS MBITS SEED    hash one random message and print its Hamming
 *                           weight; run under callgrind for an exact
 *                           instruction count
 *   time DBITS MBITS N      regress the time on the message's Hamming weight
 *                           and compare an all-zero message with random ones
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <valgrind/memcheck.h>
#include <x86intrin.h>

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

static uint64_t s;
static uint64_t rnd(void) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }

static void fill(unsigned char *m, int mbits)
{
    int nb = (mbits + 7) / 8, k;
    for (k = 0; k < nb; k++) m[k] = (unsigned char)rnd();
    if (mbits & 7) m[nb - 1] &= (unsigned char)(0xFF << (8 - (mbits & 7)));
}

static int hamming(const unsigned char *p, int n)
{
    int i, w = 0;
    for (i = 0; i < n; i++) w += __builtin_popcount(p[i]);
    return w;
}

int main(int argc, char **argv)
{
    static unsigned char msg[4096], out[512];
    int dbits = argc > 2 ? atoi(argv[2]) : 0, mbits = argc > 3 ? atoi(argv[3]) : 0;
    int nb = (mbits + 7) / 8, i;

    if (argc >= 4 && !strcmp(argv[1], "taint")) {
        memset(msg, 0x33, sizeof msg);
        VALGRIND_MAKE_MEM_UNDEFINED(msg, nb);
        CryptHash(dbits, msg, (unsigned long long)mbits, out);
        VALGRIND_MAKE_MEM_DEFINED(out, sizeof out);
        return 0;
    }
    if (argc >= 5 && !strcmp(argv[1], "one")) {
        s = strtoull(argv[4], 0, 10) * 0x9E3779B97F4A7C15ULL + 1;
        fill(msg, mbits);
        CryptHash(dbits, msg, (unsigned long long)mbits, out);
        printf("%d\n", hamming(msg, nb));
        return 0;
    }
    if (argc >= 5 && !strcmp(argv[1], "time")) {
        int n = atoi(argv[4]);
        double sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0, mx, my, cov, vx, vy, r;
        double n0 = 0, s0 = 0, q0 = 0, n1 = 0, s1 = 0, q1 = 0, t;
        s = 0x243F6A8885A308D3ULL;
        for (i = 0; i < n; i++) {
            uint64_t best = ~0ULL;
            int zero = (int)(rnd() & 1), rep;
            double x, y;
            fill(msg, mbits);
            if (zero) memset(msg, 0, nb);          /* same buffer for both classes */
            for (rep = 0; rep < 5; rep++) {
                unsigned aux;
                uint64_t t0 = __rdtscp(&aux), t1;
                CryptHash(dbits, msg, (unsigned long long)mbits, out);
                t1 = __rdtscp(&aux);
                if (t1 - t0 < best) best = t1 - t0;
            }
            y = (double)best;
            if (zero) { n0++; s0 += y; q0 += y * y; continue; }
            n1++; s1 += y; q1 += y * y;
            x = hamming(msg, nb);
            sx += x; sy += y; sxx += x * x; sxy += x * y; syy += y * y;
        }
        mx = sx / n1; my = sy / n1;
        cov = sxy / n1 - mx * my; vx = sxx / n1 - mx * mx; vy = syy / n1 - my * my;
        r = cov / sqrt(vx * vy);
        t = (s0 / n0 - s1 / n1) / sqrt((q0 / n0 - (s0 / n0) * (s0 / n0)) / n0 +
                                       (q1 / n1 - (s1 / n1) * (s1 / n1)) / n1);
        printf("mean %.0f cycles; %+.3f cycles per message 1-bit (%.1f sigma); "
               "all-zero minus random: %+.1f cycles (Welch t %+.1f); n=%d\n",
               my, cov / vx, fabs(r) * sqrt(n1), s0 / n0 - s1 / n1, t, n);
        return 0;
    }
    return 2;
}
