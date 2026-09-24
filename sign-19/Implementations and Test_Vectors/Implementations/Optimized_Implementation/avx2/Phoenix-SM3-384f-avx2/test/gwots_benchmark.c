#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../hash.h"
#include "../merkle.h"
#include "../thash.h"
#include "../gwots.h"
#include "../gwotsx1.h"
#include "../params.h"
#include "../randombytes.h"
#include "../address.h"
#include "../utils.h"
#include "cycles.h"

#define NTESTS 10

static void gwots_gen_pkx1(unsigned char *pk, const spx_ctx *ctx,
                          uint32_t addr[8]);

static int cmp_llu(const void *a, const void *b)
{
    if (*(unsigned long long *)a < *(unsigned long long *)b) return -1;
    if (*(unsigned long long *)a > *(unsigned long long *)b) return 1;
    return 0;
}

static unsigned long long median(unsigned long long *l, size_t llen)
{
    qsort(l, llen, sizeof(unsigned long long), cmp_llu);

    if (llen % 2) return l[llen / 2];
    else return (l[llen / 2 - 1] + l[llen / 2]) / 2;
}

static void delta(unsigned long long *l, size_t llen)
{
    unsigned int i;
    for (i = 0; i < llen - 1; i++) {
        l[i] = l[i + 1] - l[i];
    }
}

static void printfcomma(unsigned long long n)
{
    if (n < 1000) {
        printf("%llu", n);
        return;
    }
    printfcomma(n / 1000);
    printf(",%03llu", n % 1000);
}

static void printfalignedcomma(unsigned long long n, int len)
{
    unsigned long long ncopy = n;
    int i = 0;

    while (ncopy > 9) {
        len -= 1;
        ncopy /= 10;
        i += 1;
    }
    i = i / 3 - 1;
    for (; i < len; i++) {
        printf(" ");
    }
    printfcomma(n);
}

static void display_result(double result, unsigned long long *l, size_t llen,
                           unsigned long long mul, const char *stage)
{
    unsigned long long med;

    result /= NTESTS;
    delta(l, NTESTS + 1);
    med = median(l, llen);
    printf("%s avg. %11.2lf us (%2.2lf sec); median ", stage, result, result / 1e6);
    printfalignedcomma(med, 12);
    printf(" cycles,  %5llux: ", mul);
    printfalignedcomma(mul * med, 12);
    printf(" cycles\n");
}

#define MEASURE_GENERIC(TEXT, MUL, FNCALL, CORR, STAGE) \
    printf(TEXT);                                        \
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);    \
    for (i = 0; i < NTESTS; i++) {                      \
        t[i] = cpucycles() / CORR;                      \
        FNCALL;                                          \
    }                                                    \
    t[NTESTS] = cpucycles();                             \
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);     \
    result = ((stop.tv_sec - start.tv_sec) * 1e6 +      \
        (stop.tv_nsec - start.tv_nsec) / 1e3) / (double)CORR; \
    display_result(result, t, NTESTS, MUL, STAGE);

#define MEASURE(TEXT, MUL, FNCALL, STAGE) MEASURE_GENERIC(TEXT, MUL, FNCALL, 1, STAGE)

int main(void)
{
    setbuf(stdout, NULL);
    init_cpucycles();

    spx_ctx ctx;

    randombytes(ctx.pub_seed, SPX_N);
    randombytes(ctx.sk_seed, SPX_N);
    initialize_hash_function(&ctx);

    uint32_t gwots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};
    uint32_t idx_leaf = 3;

    set_type(gwots_addr, SPX_ADDR_TYPE_GWOTS);
    set_keypair_addr(gwots_addr, idx_leaf);
    set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);

    unsigned char message[SPX_N];
    unsigned char root[SPX_N];
    unsigned char sig[SPX_GWOTS_BYTES + SPX_TREE_HEIGHT * SPX_N + COUNTER_SIZE];
    unsigned char recovered_pk[SPX_GWOTS_PK_BYTES];
    unsigned char recovered_leaf[SPX_N];
    unsigned char computed_root[SPX_N];
    unsigned char gwots_pk[SPX_GWOTS_PK_BYTES];
    uint32_t counter;

    unsigned long long t[NTESTS + 1];
    struct timespec start, stop;
    double result;
    int i;

    randombytes(message, SPX_N);

    printf("=== GWOTS+C Merkle Benchmark ===\n");
    printf("Parameters: n = %d, h = %d, d = %d, w1 = %d, w2 = %d\n",
           SPX_N, SPX_FULL_HEIGHT, SPX_D, SPX_GWOTS_W1, SPX_GWOTS_W2);
    printf("GWOTS parameters:\n");
    printf("  - W1 chains: %d (logW1=%d)\n", SPX_GWOTS_W1_LEN, SPX_GWOTS_LOGW1);
    printf("  - W2 chains: %d (logW2=%d)\n", SPX_GWOTS_W2_LEN, SPX_GWOTS_LOGW2);
    printf("  - Checksum chains: %d (W=%d)\n", SPX_GWOTS_LEN2, SPX_GWOTS_CHECKSUM_W);
    printf("  - Total chains (LEN): %d\n", SPX_GWOTS_LEN);
    printf("  - GWOTS signature size: %d bytes\n", SPX_GWOTS_BYTES);
    printf("  - Tree height: %d\n", SPX_TREE_HEIGHT);
    printf("\nRunning %d iterations.\n\n", NTESTS);

    MEASURE("GWOTS pk gen..           ", 1,
            gwots_gen_pkx1(gwots_pk, &ctx, (uint32_t *)gwots_addr),
            "GWOTS PK Gen");

    MEASURE("merkle_sign..           ", 1, {
        memcpy(root, message, SPX_N);
        set_type(gwots_addr, SPX_ADDR_TYPE_GWOTS);
        set_keypair_addr(gwots_addr, idx_leaf);
        set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);
        merkle_sign(sig, root, &ctx, gwots_addr, tree_addr, idx_leaf, &counter);
    }, "Merkle Sign (with counter search)");

    printf("  Last counter found: %u\n\n", counter);

    MEASURE("gwots_pk_from_sig..      ", 1, {
        set_type(gwots_addr, SPX_ADDR_TYPE_GWOTS);
        set_keypair_addr(gwots_addr, idx_leaf);
        gwots_pk_from_sig(recovered_pk, sig, message, &ctx, gwots_addr, counter);
    }, "GWOTS PK from Sig");

    MEASURE("Full verify path..      ", 1, {
        uint32_t wa[8] = {0};
        uint32_t ta[8] = {0};
        uint32_t pka[8] = {0};
        set_type(wa, SPX_ADDR_TYPE_GWOTS);
        set_keypair_addr(wa, idx_leaf);
        gwots_pk_from_sig(recovered_pk, sig, message, &ctx, wa, counter);
        copy_keypair_addr(pka, wa);
        set_type(pka, SPX_ADDR_TYPE_GWOTSPK);
        thash(recovered_leaf, recovered_pk, SPX_GWOTS_LEN, &ctx, pka);
        set_type(ta, SPX_ADDR_TYPE_HASHTREE);
        compute_root(computed_root, recovered_leaf, idx_leaf, 0,
                     sig + SPX_GWOTS_BYTES, SPX_TREE_HEIGHT, &ctx, ta);
    }, "Full Verify (pk_from_sig + leaf + root)");

    printf("\n=== Results ===\n");
    printf("GWOTS signature size: %d bytes (%.2f KiB)\n",
           SPX_GWOTS_BYTES, SPX_GWOTS_BYTES / 1024.0);
    printf("GWOTS + auth path size: %d bytes (%.2f KiB)\n",
           (int)(SPX_GWOTS_BYTES + SPX_TREE_HEIGHT * SPX_N),
           (SPX_GWOTS_BYTES + SPX_TREE_HEIGHT * SPX_N) / 1024.0);

    return 0;
}

static void gwots_gen_pkx1(unsigned char *pk, const spx_ctx *ctx,
                          uint32_t addr[8])
{
    struct leaf_info_x1 leaf;
    unsigned steps[SPX_GWOTS_LEN] = {0};
    INITIALIZE_LEAF_INFO_X1(leaf, addr, steps);
    gwots_gen_leafx1(pk, ctx, 0, &leaf);
}
