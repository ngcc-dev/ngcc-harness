#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "KEM_AlgorithmInstance.h"
#include "params.h"
#include "drng.h"

DRNG_ctx drng_algorithm;


#ifdef __APPLE__

#include <mach/mach_time.h>

static inline uint64_t cpucycles(void) {
  return mach_absolute_time();
}

/* macOS: 返回当前时间（纳秒） */
static inline uint64_t get_time_ns(void)
{
  static mach_timebase_info_data_t tb = {0};
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return mach_absolute_time() * tb.numer / tb.denom;
}

#else

#include <sys/time.h>

static inline uint64_t cpucycles(void) {
  uint64_t result;

  __asm__ volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
    : "=a" (result) : : "%rdx");

  return result;
}

/* Linux: 返回当前时间（纳秒） */
static inline uint64_t get_time_ns(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000000000ULL + (uint64_t)tv.tv_usec * 1000ULL;
}

#endif

uint64_t cpucycles_overhead(void) {
  uint64_t t0, t1, overhead = -1LL;
  unsigned int i;

  for(i=0;i<100000;i++) {
    t0 = cpucycles();
    __asm__ volatile ("");
    t1 = cpucycles();
    if(t1 - t0 < overhead)
      overhead = t1 - t0;
  }

  return overhead;
}


#define NTESTS 1000
uint64_t t[NTESTS];


static int cmp_uint64(const void *a, const void *b) {
  if(*(uint64_t *)a < *(uint64_t *)b) return -1;
  if(*(uint64_t *)a > *(uint64_t *)b) return 1;
  return 0;
}

static uint64_t median(uint64_t *l, size_t llen) {
  qsort(l,llen,sizeof(uint64_t),cmp_uint64);

  if(llen%2) return l[llen/2];
  else return (l[llen/2-1]+l[llen/2])/2;
}

static uint64_t average(uint64_t *t, size_t tlen) {
  size_t i;
  uint64_t acc=0;

  for(i=0;i<tlen;i++)
    acc += t[i];

  return acc/tlen;
}

void print_results(const char *s, uint64_t *t, size_t tlen) {
  size_t i;
  static uint64_t overhead = -1;

  if(tlen < 2) {
    fprintf(stderr, "ERROR: Need a least two cycle counts!\n");
    return;
  }

  if(overhead  == (uint64_t)-1)
    overhead = cpucycles_overhead();

  tlen--;
  for(i=0;i<tlen;++i)
    t[i] = t[i+1] - t[i] - overhead;

  printf("%s\n", s);
  printf("median: %llu cycles/ticks\n", (unsigned long long)median(t, tlen));
  printf("average: %llu cycles/ticks\n", (unsigned long long)average(t, tlen));
  printf("\n");
}

/* 打印吞吐量（ops/s） */
static void print_throughput(const char *s, unsigned int n, uint64_t ns)
{
  double sec = (double)ns / 1e9;
  printf("%s throughput: %.2f ops/s\n", s, n / sec);
  printf("\n");
}

void test_speed(void)
{
  printf("\n");

  printf("DTRU-%d-%d-KEM\n\n", DTRU_N, DTRU_Q);
  unsigned int i;
  unsigned char k1[DTRU_SHAREDKEYBYTES], k2[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;
  uint64_t t0, elapsed_ns;

  t0 = get_time_ns();
  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
  }
  elapsed_ns = get_time_ns() - t0;
  print_results("dtru_kem_keygen: ", t, NTESTS);
  print_throughput("dtru_kem_keygen", NTESTS, elapsed_ns);

  t0 = get_time_ns();
  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_enc(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts);
  }
  elapsed_ns = get_time_ns() - t0;
  print_results("dtru_kem_encaps: ", t, NTESTS);
  print_throughput("dtru_kem_encaps", NTESTS, elapsed_ns);

  t0 = get_time_ns();
  for (i = 0; i < NTESTS; i++)
  {
    t[i] = cpucycles();
    kem_dec(sk, sk_byts, ct, ct_byts, k2, &ss_byts2);
  }
  elapsed_ns = get_time_ns() - t0;
  print_results("dtru_kem_decaps: ", t, NTESTS);
  print_throughput("dtru_kem_decaps", NTESTS, elapsed_ns);

}

int main(void)
{
  test_speed();
  return 0;
}
