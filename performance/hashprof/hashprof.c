/* Hash/XOF/DRNG profile of one API operation on an instrumented library.
 *
 *   hashprof LIB OP BYTES ITERS WARMUPS
 *
 * LIB must be relinked with wrap.c (see relink.sh). The deterministic setup is
 * ngcc_perf.c's, so the profiled inputs are those that the benchmark times.
 * Output (tab-separated): META lines, TOTAL <ops> <ticks>, and one line per
 * distinct call shape: CALL <fn> <in_bits> <out_bits> <calls> <ticks>.
 */
#define main ngcc_perf_main
#include "../ngcc_perf.c"
#undef main

typedef struct { uint32_t fn; uint64_t in_bits, out_bits, calls, ticks; } entry_t;
static const char *FN[] = {"pseudohash", "pseudoXOF", "sm3hash", "drng", "sha3"};
static void (*prof_enable)(int);
static uint64_t (*prof_ticks)(void);
static uint64_t step_t0, step_ticks;

static void hook(int on)
{
    if (on) { step_t0 = prof_ticks(); prof_enable(1); }
    else { prof_enable(0); step_ticks += prof_ticks() - step_t0; }
}

int main(int argc, char **argv)
{
    if (argc != 6) { fprintf(stderr, "usage: %s LIB OP BYTES ITERS WARMUPS\n", argv[0]); return 2; }
    long long iters = atoll(argv[4]);
    int warmups = atoi(argv[5]);
    if (iters < 1 || warmups < 0) { fprintf(stderr, "ITERS >= 1, WARMUPS >= 0\n"); return 2; }
    context_t c = {0};
    c.operation = argv[2];
    c.input_bytes = (size_t)strtoull(argv[3], NULL, 10);
    c.library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!c.library) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }
    const ngcc_meta_t *(*metadata)(void);
    const char *(*tick_source)(void);
    void (*reset)(void);
    size_t (*entries)(entry_t *, size_t, uint64_t *);
    LOAD_FN(metadata, c.library, "ngcc_meta");
    LOAD_FN(tick_source, c.library, "ngcc_hashprof_tick_source");
    LOAD_FN(prof_ticks, c.library, "ngcc_hashprof_ticks");
    LOAD_FN(prof_enable, c.library, "ngcc_hashprof_enable");
    LOAD_FN(reset, c.library, "ngcc_hashprof_reset");
    LOAD_FN(entries, c.library, "ngcc_hashprof_entries");
    c.meta = metadata();
    if (!c.meta || c.meta->magic != NGCC_META_MAGIC || c.meta->abi != NGCC_LINK_ABI)
        { fprintf(stderr, "invalid harness metadata\n"); return 2; }
    prepare(&c);
    int step_mode = c.meta->type == NGCC_TYPE_KEX && c.kex_step >= 0;
    c.perf_fd = -1;
    for (int i = 0; i < warmups; i++)
        if (operate(&c)) { fprintf(stderr, "warm-up operation failed\n"); return 3; }

    /* tick rate, for converting to seconds; shares are ratios of ticks */
    uint64_t n0 = now_ns(), k0 = prof_ticks();
    while (now_ns() - n0 < 100000000u) { }
    double tick_hz = (double)(prof_ticks() - k0) / ((double)(now_ns() - n0) / 1e9);

    reset();
    uint64_t total = 0;
    if (step_mode) step_hook = hook;
    for (long long i = 0; i < iters; i++) {
        if (step_mode) {
            if (operate(&c)) { fprintf(stderr, "measured operation failed\n"); return 3; }
        } else {
            prof_enable(1);
            uint64_t t0 = prof_ticks();
            int r = operate(&c);
            uint64_t t1 = prof_ticks();
            prof_enable(0);
            if (r) { fprintf(stderr, "measured operation failed\n"); return 3; }
            total += t1 - t0;
        }
    }
    if (step_mode) total = step_ticks;
    static entry_t e[8192];
    uint64_t overflow = 0;
    size_t n = entries(e, sizeof e / sizeof e[0], &overflow);
    printf("META\tid\t%s\nMETA\tinstance\t%s\nMETA\toperation\t%s\n", c.meta->id, c.meta->instance, c.operation);
    printf("META\ttick_source\t%s\nMETA\ttick_hz\t%.0f\nMETA\twarmups\t%d\n", tick_source(), tick_hz, warmups);
    printf("META\toverflow_calls\t%" PRIu64 "\n", overflow);
    printf("TOTAL\t%lld\t%" PRIu64 "\n", iters, total);
    for (size_t i = 0; i < n && i < sizeof e / sizeof e[0]; i++)
        printf("CALL\t%s\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\n",
               FN[e[i].fn], e[i].in_bits, e[i].out_bits, e[i].calls, e[i].ticks);
    printf("STATUS\tcomplete\n");
    return 0;
}
