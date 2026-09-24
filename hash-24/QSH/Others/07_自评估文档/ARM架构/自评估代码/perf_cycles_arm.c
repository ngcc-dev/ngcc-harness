/* AArch64 performance measurement for QSH (guideline 1-1, metric 3.1):
 *   "运算吞吐量" (MB/s) from wall-clock time (clock_gettime), and
 *   "时钟周期数" (CPU cycles).  Real cycles are read with perf_event_open
 *   (PERF_COUNT_HW_CPU_CYCLES) when the kernel permits userspace access
 *   (perf_event_paranoid <= 2, the common default).  If that is unavailable,
 *   cycles are derived as time x frequency, where the frequency comes from the
 *   optional GHz argument, else /sys cpufreq; such rows are marked "derived".
 *
 * Iteration count is ADAPTIVE: a ~50 ms probe estimates per-hash time, then the
 * measured loop targets ~0.25 s (always >= 100 iterations).  Reports S1..S8
 * (32 B..64 KiB) + two long-message points (256 KiB, 1 MiB) for the rate r1.
 *
 *   gcc <FLAGS> -I<impl> perf_cycles_arm.c <impl>/CryptHash_AlgorithmInstance.c -o perf
 *   ./perf <variant> [cpu_GHz]      # variant 512/768/1024; GHz only for fallback
 *
 * CSV: variant,size_bytes,iters,avg_cycles,cyc_per_byte,throughput_MBps,cyc_src
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);

static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec + t.tv_nsec/1e9; }

static int pe_open(void){
    struct perf_event_attr a; int fd;
    memset(&a,0,sizeof a);
    a.type=PERF_TYPE_HARDWARE; a.size=sizeof a; a.config=PERF_COUNT_HW_CPU_CYCLES; a.disabled=1;
    a.exclude_kernel=1; a.exclude_hv=1;
    fd=(int)syscall(__NR_perf_event_open,&a,0,-1,-1,0);
    if(fd==-1){ a.exclude_kernel=0; a.exclude_hv=0;        /* some virtualized PMUs reject exclusion */
        fd=(int)syscall(__NR_perf_event_open,&a,0,-1,-1,0); }
    return fd;
}

static double read_sys_hz(void){
    const char *paths[2] = {
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq",
        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq" };
    for (int i=0;i<2;i++){ FILE *f=fopen(paths[i],"r"); if(f){ long khz=0; if(fscanf(f,"%ld",&khz)==1){ fclose(f); if(khz>0) return (double)khz*1000.0; } fclose(f); } }
    return 0.0;
}

#if defined(__aarch64__)
/* Self-calibrate the CPU clock by timing a long dependency chain of integer
 * ADDs. ADD has 1-cycle latency on every Cortex-A/Neoverse core, so a chain
 * retires at exactly 1 add/cycle and frequency ~= (#adds)/elapsed. Loop control
 * (subs/bne every 64 adds) adds ~2-3% overhead; we take the fastest of 3 runs.
 * Used only when perf_event has no PMU (common on virtualized ARM hosts). */
static double calib_hz(void){
    const unsigned long iters = 40000000UL;     /* x64 adds = 2.56e9 adds/run */
    double best = 0.0;
    for (int rep=0; rep<3; rep++){
        uint64_t x = 0; unsigned long cnt = iters;
        double t0 = now();
        __asm__ volatile(
            "1:\n\t"
            ".rept 64\n\t add %0, %0, #1\n\t .endr\n\t"
            "subs %1, %1, #1\n\t"
            "bne 1b\n\t"
            : "+r"(x), "+r"(cnt) :: "cc");
        double dt = now() - t0;
        if (dt > 0){ double hz = (double)iters*64.0/dt; if (hz > best) best = hz; }
        if (x == 0xdeadbeef) best += 0;          /* keep x live */
    }
    return best;
}
#endif

/* fallback frequency (Hz): explicit GHz arg, else self-calibration (aarch64),
 * else /sys cpufreq, else 0. */
static double fallback_hz(double ghz_arg){
    if (ghz_arg > 0.0) return ghz_arg*1e9;
    double hz;
#if defined(__aarch64__)
    hz = calib_hz(); if (hz > 0.0) return hz;
#endif
    hz = read_sys_hz(); if (hz > 0.0) return hz;
    return 0.0;
}

int main(int argc, char **argv){
    int dl = (argc>1)? atoi(argv[1]) : 512;
    double ghz_arg = (argc>2)? atof(argv[2]) : 0.0;
    int fd = pe_open();
    /* only spend time on the frequency fallback when perf_event has no PMU */
    double fb_hz = (fd!=-1) ? (ghz_arg>0 ? ghz_arg*1e9 : 0.0) : fallback_hz(ghz_arg);
    const char *src = (fd!=-1) ? "perf_event" : (fb_hz>0 ? "derived" : "none");

    const size_t S[10] = {32,128,512,1024,4096,8192,16384,65536,262144,1048576};
    const size_t MAXN = 1048576;
    unsigned char *msg = (unsigned char*)malloc(MAXN), dig[128];
    for (size_t i=0;i<MAXN;i++) msg[i]=(unsigned char)(i*131+7);

    printf("# QSH-%d  cyc_src=%s  freq=%.3fGHz%s\n", dl, src, fb_hz/1e9,
           (fd!=-1) ? " (perf_event=real cycles; freq shown for reference)" :
           (ghz_arg>0 ? " (from CPU_GHZ)" : " (self-calibrated)"));
    printf("variant,size_bytes,iters,avg_cycles,cyc_per_byte,throughput_MBps,cyc_src\n");
    for (int s=0;s<10;s++){
        size_t n = S[s];
        CryptHash(dl,msg,(unsigned long long)n*8,dig);                 /* warm */
        double p0 = now(); long pc = 0;
        while (now()-p0 < 0.05 && pc < 2000000){ CryptHash(dl,msg,(unsigned long long)n*8,dig); pc++; }
        double per = (now()-p0)/(pc>0?pc:1);
        long iters = (long)(0.25/per); if (iters<100) iters=100; if (iters>20000000) iters=20000000;

        uint64_t cyc_pe = 0;
        if (fd!=-1){ ioctl(fd,PERF_EVENT_IOC_RESET,0); ioctl(fd,PERF_EVENT_IOC_ENABLE,0); }
        double t0 = now();
        for (long i=0;i<iters;i++) CryptHash(dl,msg,(unsigned long long)n*8,dig);
        double t1 = now();
        if (fd!=-1){ ioctl(fd,PERF_EVENT_IOC_DISABLE,0); if (read(fd,&cyc_pe,sizeof cyc_pe)!=sizeof cyc_pe) cyc_pe=0; }

        double secs = t1-t0;
        double cyc = (fd!=-1 && cyc_pe>0) ? (double)cyc_pe : secs*fb_hz;
        double mbps = (double)n*iters/secs/1e6;
        if (cyc > 0.0)
            printf("QSH-%d,%zu,%ld,%.1f,%.2f,%.1f,%s\n", dl, n, iters,
                   cyc/iters, cyc/((double)n*iters), mbps, src);
        else
            printf("QSH-%d,%zu,%ld,,,%.1f,none\n", dl, n, iters, mbps);
        fflush(stdout);
    }
    if (fd!=-1) close(fd);
    free(msg);
    return 0;
}
