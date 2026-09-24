#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../hash.h"
#include "../api.h"
#include "../tfors.h"
#include "../params.h"
#include "../randombytes.h"
#include "../address.h"
#include "cycles.h"

#define NTESTS 1

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
    printf(",%03llu", n % 1000);
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

static void display_result(double result, unsigned long long *l, size_t llen, unsigned long long mul, const char *stage)
{
    unsigned long long med;

    result /= NTESTS;
    delta(l, NTESTS + 1);
    med = median(l, llen);
    printf("%s avg. %11.2lf us (%2.2lf sec); median ", stage, result, result / 1e6);
    printfalignedcomma(med, 12);
    printf(" cycles,  %5llux: ", mul);
    printfalignedcomma(mul*med, 12);
    printf(" cycles\n");
}

#define MEASURE_GENERIC(TEXT, MUL, FNCALL, CORR, STAGE)\
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
    display_result(result, t, NTESTS, MUL, STAGE);

#define MEASURT(TEXT, MUL, FNCALL, STAGE)\
    MEASURE_GENERIC(\
        TEXT, MUL,\
        do {\
          for (int j = 0; j < 1000; j++) {\
            FNCALL;\
          }\
        } while (0);,\
    1000, STAGE);

#define MEASURE(TEXT, MUL, FNCALL, STAGE) MEASURE_GENERIC(TEXT, MUL, FNCALL, 1, STAGE)

static void tfors_gen_sk(unsigned char *sk, const spx_ctx *ctx,
                        uint32_t tfors_leaf_addr[8])
{
    prf_addr(sk, ctx, tfors_leaf_addr);
}

int main(void)
{
    /* Make stdout buffer more responsive. */
    setbuf(stdout, NULL);
    init_cpucycles();

    spx_ctx ctx;
    
    /* Init context and set seeds */
    randombytes(ctx.pub_seed, SPX_N);
    randombytes(ctx.sk_seed, SPX_N);
    initialize_hash_function(&ctx);

    unsigned char tfors_pk[SPX_TFORS_PK_BYTES];
    unsigned char tfors_sk[SPX_N * SPX_TFORS_K];
    unsigned char tfors_m[SPX_TFORS_MSG_BYTES];
    unsigned char tfors_sig[SPX_TFORS_BYTES];
    uint32_t tfors_addr[8] = {0};
    uint32_t indices[SPX_TFORS_K];

    unsigned long long t[NTESTS+1];
    struct timespec start, stop;
    double result;
    int i;

    randombytes(tfors_m, SPX_TFORS_MSG_BYTES);

    printf("Parameters: n = %d, h = %d, d = %d, b = %d, k = %d, w = %d\n",
           SPX_N, SPX_FULL_HEIGHT, SPX_D, SPX_TFORS_A, SPX_TFORS_K,
           SPX_GWOTS_W1);
    printf("  - Tree height: %d\n", SPX_TFORS_HEIGHT);
    printf("  - Total leaves: %d\n", (1 << SPX_TFORS_HEIGHT));
    printf("  - Max signature bytes: %d\n", (int)SPX_TFORS_BYTES);
    printf("\nRunning %d iterations.\n\n", NTESTS);

    MEASURE("TFORS key generation..  ", 1, {
        for (i = 0; i < SPX_TFORS_K; i++) {
            set_tree_height(tfors_addr, 0);
            set_tree_index(tfors_addr, i);
            set_type(tfors_addr, SPX_ADDR_TYPE_TFORSPRF);
            tfors_gen_sk(tfors_sk + i * SPX_N, &ctx, tfors_addr);
        }
    }, "TFORS Key Generation");

    message_to_indices(indices, tfors_m, &ctx);
    MEASURE("TFORS signing..         ", 1, 
            tfors_sign(tfors_sig, tfors_pk, indices, &ctx, tfors_addr), 
            "TFORS Signing");

    MEASURE("TFORS verification..    ", 1, 
            tfors_pk_from_sig(tfors_pk, tfors_sig, tfors_m, &ctx, tfors_addr), 
            "TFORS Verification");

    printf("\n=== Results ===\n");
    printf("TFORS signature size: %d bytes (%.2f KiB)\n", 
           SPX_TFORS_BYTES, SPX_TFORS_BYTES / 1024.0);
    printf("TFORS public key size: %d bytes (%.2f KiB)\n", 
           SPX_TFORS_PK_BYTES, SPX_TFORS_PK_BYTES / 1024.0);

    return 0;
}