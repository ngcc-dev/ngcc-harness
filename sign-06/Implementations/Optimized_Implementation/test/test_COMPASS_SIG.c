#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "../randombytes.h"
#include "../sign.h"

// 引入 sign.c 中定义的全局变量
extern unsigned long long global_total_cycles;
extern unsigned long long global_success_signs;
extern unsigned long long global_fail_w1;
extern unsigned long long global_fail_w0;
extern unsigned long long global_fail_zl2;
extern unsigned long long global_fail_zl0;

#define MLEN 59
#define CTXLEN 14
#define NTESTS 10000

int main(void)
{
  size_t i, j;
  int ret;
  size_t mlen, smlen;
  uint8_t b;
  uint8_t ctx[CTXLEN] = {0};
  uint8_t m[MLEN + CRYPTO_BYTES];
  uint8_t m2[MLEN + CRYPTO_BYTES];
  uint8_t sm[MLEN + CRYPTO_BYTES];
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];

  snprintf((char*)ctx,CTXLEN,"test_dilitium");

  for(i = 0; i < NTESTS; ++i) {
    randombytes(m, MLEN);

    crypto_sign_keypair(pk, sk);
    crypto_sign(sm, &smlen, m, MLEN, ctx, CTXLEN, sk);
    ret = crypto_sign_open(m2, &mlen, sm, smlen, ctx, CTXLEN, pk);

    if(ret) {
      fprintf(stderr, "Verification failed\n");
      return -1;
    }
    if(smlen != MLEN + CRYPTO_BYTES) {
      fprintf(stderr, "Signed message lengths wrong\n");
      return -1;
    }
    if(mlen != MLEN) {
      fprintf(stderr, "Message lengths wrong\n");
      return -1;
    }
    for(j = 0; j < MLEN; ++j) {
      if(m2[j] != m[j]) {
        fprintf(stderr, "Messages don't match\n");
        return -1;
      }
    }

    randombytes((uint8_t *)&j, sizeof(j));
    do {
      randombytes(&b, 1);
    } while(!b);
    sm[j % (MLEN + CRYPTO_BYTES)] += b;
    ret = crypto_sign_open(m2, &mlen, sm, smlen, ctx, CTXLEN, pk);
    if(!ret) {
      fprintf(stderr, "Trivial forgeries possible\n");
      return -1;
    }
  }

  printf("CRYPTO_PUBLICKEYBYTES = %d\n", CRYPTO_PUBLICKEYBYTES);
  printf("CRYPTO_SECRETKEYBYTES = %d\n", CRYPTO_SECRETKEYBYTES);
  printf("CRYPTO_BYTES = %d\n", CRYPTO_BYTES);
  
  printf("\n🎉 恭喜！完美的算法！\n");
  printf("全部 %d 次 [生成 -> 签名 -> 验签 -> 拦截伪造] 测试已成功通过！\n", NTESTS);

  // =======================================================================
  // 📊 拒绝采样硬核统计报告 (用于论文 Evaluation)
  // =======================================================================
  if (global_success_signs > 0) {
      double avg_cycles = (double)global_total_cycles / global_success_signs;
      double rate_w1 = (double)global_fail_w1 / global_total_cycles * 100.0;
      double rate_w0 = (double)global_fail_w0 / global_total_cycles * 100.0;
      double rate_zl2  = (double)global_fail_zl2  / global_total_cycles * 100.0;
      double rate_zl0  = (double)global_fail_zl0  / global_total_cycles * 100.0;
      double rate_success = (double)global_success_signs / global_total_cycles * 100.0;

      printf("\n======================================================\n");
      printf(" 📊 拒绝采样 (Rejection Sampling) 全局统计报告 \n");
      printf("======================================================\n");
      printf(" 🎯 成功生成签名总数   : %llu 次\n", global_success_signs);
      printf(" 🔄 经历的总循环次数   : %llu 次\n", global_total_cycles);
      printf(" ⏱️ 平均生成一个签名需要 : %.2f 次循环\n", avg_cycles);
      printf("------------------------------------------------------\n");
      printf(" 📉 失败原因分布 (占总循环的比例):\n");
      printf("    ❌ 被 zl0范数拒绝  : %llu 次 (%.2f%%)\n", global_fail_zl0, rate_zl0);
      printf("    ❌ 被 zl2 范数拒绝 : %llu 次 (%.2f%%)\n", global_fail_zl2, rate_zl2);
      printf("    ❌ 被 w0 范数拒绝  : %llu 次 (%.2f%%)\n", global_fail_w0, rate_w0);
      printf("    ❌ 被 w1 漂移拒绝  : %llu 次 (%.2f%%)\n", global_fail_w1, rate_w1);
      printf("    ✅ 完美通过的比例  : %.2f%%\n", rate_success);
      printf("======================================================\n\n");
  }
  return 0;
}
