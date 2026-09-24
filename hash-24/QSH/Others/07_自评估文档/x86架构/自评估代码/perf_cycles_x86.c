/* x86 performance measurement for QSH (per 1-2 guideline 3.1):
 *   "时钟周期数" via rdtsc (invariant TSC -> reproducible, turbo-independent;
 *    on a frequency-locked machine this equals core cycles), and
 *   "运算吞吐量" (MB/s) from wall-clock time.
 *
 * Iteration count is ADAPTIVE: a ~50 ms probe estimates the per-hash time, then
 * the measured loop targets ~0.25 s (but always >= 100 iterations, as required).
 * This keeps the run short for both the SIMD and the scalar (reference /
 * resource) versions. Reports S1..S8 (32 B..64 KiB) + two long-message points.
 *
 *   gcc <FLAGS> -I<impl> perf_cycles_x86.c <impl>/CryptHash_AlgorithmInstance.c -o perf
 *   ./perf <variant>            # 512 / 768 / 1024
 *
 * CSV: variant,size_bytes,iters,avg_cycles,cyc_per_byte,throughput_MBps
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);

static inline uint64_t rdtsc(void){
    unsigned hi, lo; __asm__ volatile("lfence\n\trdtsc":"=a"(lo),"=d"(hi)); return ((uint64_t)hi<<32)|lo;
}
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec + t.tv_nsec/1e9; }

int main(int argc, char **argv)
{
    int dl = (argc>1)? atoi(argv[1]) : 512;
    /* calibrate TSC rate over ~300 ms */
    double w0 = now(); uint64_t c0 = rdtsc();
    struct timespec sl = {0, 300000000L}; nanosleep(&sl, NULL);
    uint64_t c1 = rdtsc(); double w1 = now();
    double tsc_hz = (double)(c1 - c0) / (w1 - w0);

    const size_t S[10] = {32,128,512,1024,4096,8192,16384,65536,262144,1048576};
    const size_t MAXN = 1048576;
    unsigned char *msg = (unsigned char*)malloc(MAXN), dig[128];
    for (size_t i=0;i<MAXN;i++) msg[i]=(unsigned char)(i*131+7);

    printf("# QSH-%d  TSC_GHz=%.4f\n", dl, tsc_hz/1e9);
    printf("variant,size_bytes,iters,avg_cycles,cyc_per_byte,throughput_MBps\n");
    for (int s=0;s<10;s++){
        size_t n = S[s];
        CryptHash(dl,msg,(unsigned long long)n*8,dig);                 /* warm */
        /* probe ~50 ms to estimate per-hash time */
        double p0 = now(); long pc = 0;
        while (now() - p0 < 0.05 && pc < 2000000) { CryptHash(dl,msg,(unsigned long long)n*8,dig); pc++; }
        double per = (now() - p0) / (pc > 0 ? pc : 1);
        long iters = (long)(0.25 / per);
        if (iters < 100) iters = 100;            /* guideline: >= 100 */
        if (iters > 20000000) iters = 20000000;
        /* measured run */
        uint64_t cyc0 = rdtsc(); double t0 = now();
        for (long i=0;i<iters;i++) CryptHash(dl,msg,(unsigned long long)n*8,dig);
        double t1 = now(); uint64_t cyc1 = rdtsc();
        double cyc = (double)(cyc1 - cyc0), secs = t1 - t0;
        printf("QSH-%d,%zu,%ld,%.1f,%.2f,%.1f\n", dl, n, iters,
               cyc/iters, cyc/((double)n*iters), (double)n*iters/secs/1e6);
        fflush(stdout);
    }
    free(msg);
    return 0;
}
