/* QSH self-assessment harness (per ARM/x86 guidelines 1-1 / 1-2), PORTABLE.
 * Measures throughput (MB/s) and cycles/byte at the eight S1..S8 input sizes for
 * a given variant. Cross-architecture: uses wall-clock timing (clock_gettime)
 * and derives cycles/byte from a supplied CPU frequency, so it runs identically
 * on x86-64 and AArch64.
 *
 *   gcc <FLAGS> -I<impl_dir> self_assess.c <impl_dir>/CryptHash_AlgorithmInstance.c -o self_assess
 *   ./self_assess <variant> [cpu_GHz]
 *     variant : 512 / 768 / 1024
 *     cpu_GHz : nominal core frequency for the cyc/byte column (e.g. 3.6).
 *               If omitted, cyc/byte is left blank (record GHz in the report
 *               environment section and fill it in, or pass it here).
 *
 * CSV columns: variant,size_bytes,iters,avg_ns,throughput_MBps,cyc_per_byte
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

int main(int argc, char **argv)
{
    int dl = (argc > 1) ? atoi(argv[1]) : 512;
    double ghz = (argc > 2) ? atof(argv[2]) : 0.0;
    /* S1..S8 (per 1-1/1-2 guideline) + two long-message points for r1 */
    const size_t S[10] = {32,128,512,1024,4096,8192,16384,65536, 262144, 1048576};
    const size_t MAXN = 1048576;
    unsigned char *msg = (unsigned char*)malloc(MAXN), dig[128];
    for (size_t i = 0; i < MAXN; i++) msg[i] = (unsigned char)(i*131 + 7);

    printf("variant,size_bytes,iters,avg_ns,throughput_MBps,cyc_per_byte\n");
    for (int s = 0; s < 10; s++) {
        size_t n = S[s];
        int iters = (n < 4096) ? 200000 : (n < 65536 ? 40000 :
                    (n <= 65536 ? 10000 : (n <= 262144 ? 4000 : 1500))); /* >=100 */
        CryptHash(dl, msg, (unsigned long long)n*8, dig);              /* warm */
        struct timespec t0, t1; clock_gettime(CLOCK_MONOTONIC, &t0);
        for (int i = 0; i < iters; i++) CryptHash(dl, msg, (unsigned long long)n*8, dig);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double secs = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9;
        double per_call = secs / iters;
        double mbps = (double)n * iters / secs / 1e6;
        if (ghz > 0.0)
            printf("QSH-%d,%zu,%d,%.1f,%.1f,%.2f\n", dl, n, iters,
                   per_call*1e9, mbps, (secs/((double)n*iters)) * ghz * 1e9);
        else
            printf("QSH-%d,%zu,%d,%.1f,%.1f,\n", dl, n, iters, per_call*1e9, mbps);
        fflush(stdout);
    }
    free(msg);
    return 0;
}
