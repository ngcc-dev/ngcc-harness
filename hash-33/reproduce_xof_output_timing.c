/* hash-33-1: the XOF output copy branches on each output bit.
 *
 * Modes:
 *   taint        hash messages whose bytes are marked undefined; run under
 *                Valgrind memcheck, which reports any branch on them
 *   one SEED     hash one message and print the output's Hamming weight; run
 *                under callgrind for an exact instruction count
 *   time N       time N random messages (minimum of 5 runs each) and regress
 *                the time on the output's Hamming weight
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

#define NU 1280                 /* one squeeze block of the 256-bit XOF */

static uint64_t s;
static uint64_t rnd(void) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }

static int hamming(const unsigned char *p, int n)
{
    int i, w = 0;
    for (i = 0; i < n; i++) w += __builtin_popcount(p[i]);
    return w;
}

int main(int argc, char **argv)
{
    unsigned char msg[32], out[NU / 8];
    int i, k;

    if (argc >= 2 && !strcmp(argv[1], "taint")) {
        memset(msg, 0x33, sizeof msg);
        VALGRIND_MAKE_MEM_UNDEFINED(msg, sizeof msg);
        CryptHash(NU, msg, 256, out);
        VALGRIND_MAKE_MEM_DEFINED(out, sizeof out);
        return 0;
    }
    if (argc >= 3 && !strcmp(argv[1], "one")) {
        s = strtoull(argv[2], 0, 10) * 0x9E3779B97F4A7C15ULL + 1;
        for (k = 0; k < 32; k++) msg[k] = (unsigned char)rnd();
        CryptHash(NU, msg, 256, out);
        printf("%d\n", hamming(out, NU / 8));
        return 0;
    }
    if (argc >= 3 && !strcmp(argv[1], "time")) {
        int n = atoi(argv[2]);
        double sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0, mx, my, cov, vx, vy, r;
        s = 0x243F6A8885A308D3ULL;
        for (i = 0; i < n; i++) {
            uint64_t best = ~0ULL;
            double x, y;
            int rep;
            for (k = 0; k < 32; k++) msg[k] = (unsigned char)rnd();
            for (rep = 0; rep < 5; rep++) {
                unsigned aux;
                uint64_t t0 = __rdtscp(&aux), t1;
                CryptHash(NU, msg, 256, out);
                t1 = __rdtscp(&aux);
                if (t1 - t0 < best) best = t1 - t0;
            }
            x = hamming(out, NU / 8); y = (double)best;
            sx += x; sy += y; sxx += x * x; sxy += x * y; syy += y * y;
        }
        mx = sx / n; my = sy / n;
        cov = sxy / n - mx * my; vx = sxx / n - mx * mx; vy = syy / n - my * my;
        r = cov / sqrt(vx * vy);
        printf("mean %.0f cycles, %+.3f cycles per output 1-bit, correlation %+.4f (%.1f sigma, n=%d)\n",
               my, cov / vx, r, fabs(r) * sqrt((double)n), n);
        return 0;
    }
    return 2;
}
