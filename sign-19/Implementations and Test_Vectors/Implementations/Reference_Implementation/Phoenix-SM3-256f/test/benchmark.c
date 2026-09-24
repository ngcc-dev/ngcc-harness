#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#include "../hash.h"
#include "../thash.h"
#include "../api.h"
#include "../tfors.h"
#include "../gwotsx1.h"
#include "../params.h"
#include "../randombytes.h"
#include "../randombytes.h"
#include "cycles.h"

#define SPX_MLEN 32
#define NTESTS 10

static void gwots_gen_pkx1(unsigned char *pk, const spx_ctx* ctx,
                uint32_t addr[8]);

static int cmp_llu(const void *a, const void*b)
{
  if(*(unsigned long long *)a < *(unsigned long long *)b) return -1;
  if(*(unsigned long long *)a > *(unsigned long long *)b) return 1;
  return 0;
}

static unsigned long long median(unsigned long long *l, size_t llen)
{
  qsort(l,llen,sizeof(unsigned long long),cmp_llu);

  if(llen%2) return l[llen/2];
  else return (l[llen/2-1]+l[llen/2])/2;
}

static void delta(unsigned long long *l, size_t llen)
{
    unsigned int i;
    for(i = 0; i < llen - 1; i++) {
        l[i] = l[i+1] - l[i];
    }
}


static void printfcomma (unsigned long long n)
{
    if (n < 1000) {
        printf("%llu", n);
        return;
    }
    printfcomma(n / 1000);
    printf (",%03llu", n % 1000);
}

static void printfalignedcomma (unsigned long long n, int len)
{
    unsigned long long ncopy = n;
    int i = 0;

    while (ncopy > 9) {
        len -= 1;
        ncopy /= 10;
        i += 1;
    }
    i = i/3 - 1;
    for (; i < len; i++) {
        printf(" ");
    }
    printfcomma(n);
}

static void display_result(double result, unsigned long long *l, size_t llen, unsigned long long mul)
{
    unsigned long long med;

    result /= NTESTS;
    delta(l, NTESTS + 1);
    med = median(l, llen);
    printf("avg. %11.2lf us (%2.2lf sec); median ", result, result / 1e6);
    printfalignedcomma(med, 12);
    printf(" cycles,  %5llux: ", mul);
    printfalignedcomma(mul*med, 12);
    printf(" cycles\n");
}

#define MEASURE_GENERIC(TEXT, MUL, FNCALL, CORR)\
    printf(TEXT);\
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);\
    for(i = 0; i < NTESTS; i++) {\
        t[i] = cpucycles() / CORR;\
        FNCALL;\
    }\
    t[NTESTS] = cpucycles();\
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);\
    result = ((stop.tv_sec - start.tv_sec) * 1e6 + \
        (stop.tv_nsec - start.tv_nsec) / 1e3) / (double)CORR;\
    display_result(result, t, NTESTS, MUL);
#define MEASURT(TEXT, MUL, FNCALL)\
    MEASURE_GENERIC(\
        TEXT, MUL,\
        do {\
          for (int j = 0; j < 1000; j++) {\
            FNCALL;\
          }\
        } while (0);,\
    1000);
#define MEASURE(TEXT, MUL, FNCALL) MEASURE_GENERIC(TEXT, MUL, FNCALL, 1)



int main(void)
{
    setbuf(stdout, NULL);
    init_cpucycles();

    spx_ctx ctx;

    unsigned char pk[SPX_PK_BYTES];
    unsigned char sk[SPX_SK_BYTES];
    unsigned char *m = malloc(SPX_MLEN);
    unsigned char *sm = malloc(SPX_BYTES + SPX_MLEN);
    unsigned char *mout = malloc(SPX_BYTES + SPX_MLEN);

    unsigned char tfors_pk[SPX_TFORS_PK_BYTES];
    unsigned char tfors_m[SPX_TFORS_MSG_BYTES];
    unsigned char tfors_sig[SPX_TFORS_BYTES];
    uint32_t addr[8];
    unsigned char block[SPX_N];

    unsigned char gwots_pk[SPX_GWOTS_PK_BYTES];

    unsigned long long smlen;
    unsigned long long mlen;
    unsigned long long slen;
    unsigned long long tfslen;
    unsigned long long t[NTESTS+1];
    struct timespec start, stop;
    double result;
    int i;

    randombytes(m, SPX_MLEN);
    randombytes((unsigned char *)addr, SPX_ADDR_BYTES);
    randombytes(tfors_m, SPX_TFORS_MSG_BYTES);

    randombytes(ctx.pub_seed, SPX_N);
    randombytes(ctx.sk_seed, SPX_N);
    initialize_hash_function(&ctx);

    uint32_t tfors_indices[SPX_TFORS_K];
    message_to_indices(tfors_indices, tfors_m, &ctx);

    printf("===============================================================\n");
    printf("Phoenix End-to-End Benchmark\n");
    printf("Parameter set: %s\n", PARAMNAME);
    printf("Parameters: n = %d, h = %d, d = %d, a = %d, k = %d, w = %d\n",
           SPX_N, SPX_FULL_HEIGHT, SPX_D, SPX_TFORS_A, SPX_TFORS_K,
           SPX_GWOTS_W1);

    printf("Running %d iterations.\n", NTESTS);

    MEASURT("thash                ", 1, thash(block, block, 1, &ctx, addr));
    MEASURE("Generating keypair.. ", 1, crypto_sign_keypair(pk, sk));
    MEASURE("  - GWOTS pk gen..    ", (1 << SPX_TREE_HEIGHT), gwots_gen_pkx1(gwots_pk, &ctx, addr));
    MEASURE("Signing..            ", 1, crypto_sign(sm, &smlen, &slen, &tfslen, m, SPX_MLEN, sk));
    MEASURE("  - GWOTS pk gen..    ", SPX_D * (1 << SPX_TREE_HEIGHT), gwots_gen_pkx1(gwots_pk, &ctx, addr));
    MEASURE("Verifying..          ", 1, crypto_sign_open(mout, &mlen, &slen, &tfslen, sm, smlen, pk));

    printf("Signature size: %llu bytes\n", smlen);
    printf("Public key size: %d bytes\n", SPX_PK_BYTES);
    printf("Secret key size: %d bytes\n", SPX_SK_BYTES);

    free(m);
    free(sm);
    free(mout);

    return 0;
}

static void gwots_gen_pkx1(unsigned char *pk, const spx_ctx *ctx,
                  uint32_t addr[8]) {
    struct leaf_info_x1 leaf;
    unsigned steps[ SPX_GWOTS_LEN ] = { 0 };
    INITIALIZE_LEAF_INFO_X1(leaf, addr, steps);
    gwots_gen_leafx1(pk, ctx, 0, &leaf);
}