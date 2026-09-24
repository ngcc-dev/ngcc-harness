#include <stdint.h>
#include "params.h"
#include "sign.h"
#include "packing.h"
#include "polyvec.h"
#include "poly.h"
#include "randombytes.h"
#include "symmetric.h"
#include "fips202.h"

// ==========================================
// 📊 全局采样统计器 (用于测试与论文数据抓取)
// ==========================================
unsigned long long global_total_cycles = 0; // 总循环次数
unsigned long long global_success_signs = 0; // 成功生成签名的次数
unsigned long long global_fail_w1 = 0;      // 被 w1 漂移拒绝的次数
unsigned long long global_fail_w0 = 0;      // 被 w0 范数拒绝的次数
unsigned long long global_fail_zl2 = 0;       // 被 z 范数/L2 拒绝的次数
unsigned long long global_fail_zl0 = 0;       // 被 z 范数/L2 拒绝的次数
// ==========================================

// #include "stdio.h"
// #include "stdlib.h"
/*************************************************
* Name:        crypto_sign_keypair
*
* Description: Generates public and private key.
*
* Arguments:   - uint8_t *pk: pointer to output public key (allocated
*                             array of CRYPTO_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key (allocated
*                             array of CRYPTO_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
  uint8_t seed[SEEDBYTES];
  uint8_t seedbuf[2*SEEDBYTES + CRHBYTES];
  uint8_t tr[TRBYTES];
  const uint8_t *rho, *rhoprime, *key;
  polyvecl mat[K];
  polyvecl s1, s1hat;
  polyveck s2, t1, t0, t;

  // 1. 获取安全的随机种子并使用 SHAKE256 扩展
  randombytes(seed, SEEDBYTES);
  shake256(seedbuf, 2*SEEDBYTES + CRHBYTES, seed, SEEDBYTES);
  
  rho = seedbuf;
  rhoprime = rho + SEEDBYTES;
  key = rhoprime + CRHBYTES;

  // 2. 展开公钥矩阵 A
  polyvec_matrix_expand(mat, rho);

  // 3. 采样私钥向量 s1 和 s2
  // 必须显式传入 ETA_S 和 ETA_E，利用修改后的 poly_uniform_eta
  polyvecl_uniform_eta_s(&s1,rhoprime,0);
  polyveck_uniform_eta_e(&s2,rhoprime,L);
  // for(unsigned int i = 0; i < L; ++i)
  //   poly_uniform_eta(&s1.vec[i], rhoprime, i, ETA_S);
  // for(unsigned int i = 0; i < K; ++i)
  //   poly_uniform_eta(&s2.vec[i], rhoprime, L + i, ETA_E);

  // 4. 计算 t = A * s1 + s2
  s1hat = s1;
  polyvecl_ntt(&s1hat);
  
  // 矩阵向量乘法
  polyvec_matrix_pointwise_montgomery(&t, mat, &s1hat);
  polyveck_reduce(&t);
  polyveck_invntt_tomont(&t);

  // 加上 s2 误差
  polyveck_add(&t, &t, &s2);
  
  // 条件加 q，将系数规约到正数范围
  polyveck_caddq(&t);

  // 5. 拆分 t 得到公钥 t1 和私钥 t0
  // 内部调用的 power2round 已经通过宏 D 自动适配了不同位移
  polyveck_power2round(&t1, &t0, &t);

  // 6. 打包公钥并计算其哈希 tr
  pack_pk(pk, rho, &t1);
  shake256(tr, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);

  // 7. 打包私钥 (按照修改后的参数顺序)
  pack_sk(sk, rho, tr, key, &s1, &s2, &t0);

  return 0;
}

/*************************************************
* Name:        crypto_sign_signature_internal
*
* Description: Computes signature. Internal API.
*
* Arguments:   - uint8_t *sig:   pointer to output signature (of length CRYPTO_BYTES)
*              - size_t *siglen: pointer to output length of signature
*              - uint8_t *m:     pointer to message to be signed
*              - size_t mlen:    length of message
*              - uint8_t *pre:   pointer to prefix string
*              - size_t prelen:  length of prefix string
*              - uint8_t *rnd:   pointer to random seed
*              - uint8_t *sk:    pointer to bit-packed secret key
*
* Returns 0 (success)
**************************************************/
int crypto_sign_signature_internal(uint8_t *sig,
                                   size_t *siglen,
                                   const uint8_t *m,
                                   size_t mlen,
                                   const uint8_t *pre,
                                   size_t prelen,
                                   const uint8_t rnd[RNDBYTES],
                                   const uint8_t *sk)
{
  // unsigned int n;
  uint8_t seedbuf[2*SEEDBYTES + TRBYTES + 2*CRHBYTES];
  uint8_t *rho, *tr, *key, *mu, *rhoprime;
  uint16_t nonce = 0;
  polyvecl mat[K], s1, y, z;
  polyveck t0, s2, w1, w0, w;
  poly cp;
  keccak_state state;

  // 1. 设置指针并解包私钥 (使用我们修改后的 unpack_sk)
  rho = seedbuf;
  tr = rho + SEEDBYTES;
  key = tr + TRBYTES;
  mu = key + SEEDBYTES;
  rhoprime = mu + CRHBYTES;
  
  // 注意传参顺序与 packing.c 中一致
  unpack_sk(rho, tr, key, &s1, &s2, &t0, sk);

  // printf("\n--- 🔍 私钥解包立即安检 ---\n");
  // printf("t0[0] 前三个系数: %d, %d, %d\n", 
  //        t0.vec[0].coeffs[0], t0.vec[0].coeffs[1], t0.vec[0].coeffs[2]);
  // printf("---------------------------\n");
  
  // for(int i = 0; i < L; ++i) {
  //   for(int j = 0; j < N; ++j) {
  //     if(s1.vec[i].coeffs[j] < 0) s1.vec[i].coeffs[j] += Q;
  //   }
  // }
  // for(int i = 0; i < K; ++i) {
  //   for(int j = 0; j < N; ++j) {
  //     if(s2.vec[i].coeffs[j] < 0) s2.vec[i].coeffs[j] += Q;
  //     if(t0.vec[i].coeffs[j] < 0) t0.vec[i].coeffs[j] += Q;
  //   }
  // }

  // 2. 计算消息的哈希 mu = CRH(tr || M)
  shake256_init(&state);
  shake256_absorb(&state, tr, TRBYTES);
  shake256_absorb(&state, pre, prelen);
  shake256_absorb(&state, m, mlen);
  shake256_finalize(&state);
  shake256_squeeze(mu, CRHBYTES, &state);

  // 3. 计算随机种子 rhoprime = CRH(key || mu)
  shake256_init(&state);
  shake256_absorb(&state, key, SEEDBYTES);
  shake256_absorb(&state, rnd, RNDBYTES);
  shake256_absorb(&state, mu, CRHBYTES);
  shake256_finalize(&state);
  shake256_squeeze(rhoprime, CRHBYTES, &state);

  // 4. 展开公钥矩阵 A
  polyvec_matrix_expand(mat, rho);

  // 5. 将私钥 s1, s2 转换到 NTT 域，以便后续快速乘法
  polyvecl_ntt(&s1);
  polyveck_ntt(&s2);
  polyveck_ntt(&t0);

  // int fail_w1 = 0;
  // int fail_w0_norm = 0;
  // int fail_z_norm = 0;
  // int cycle_count = 0;
  // ==========================================
  // 开始拒绝采样循环 (Rejection Sampling)
  // ==========================================
  // int ii=0;
  while(1) {
    global_total_cycles++;
    // printf("while cycle = %d \n",cycle_count++);
    // if(cycle_count == 1000) {
    //         printf("\n====== 🚨 卡死现场勘探报告 (已采样 1000 次) ======\n");
    //         printf(">>> 模式: K=%d, L=%d, GAMMA2=%d\n", K, L, GAMMA2);
    //         printf("因 w1 漂移被拒绝     : %d 次\n", fail_w1);
    //         printf("因 w0 范数越界被拒绝 : %d 次\n", fail_w0_norm);
    //         printf("因 z 范数/L2 被拒绝  : %d 次\n", fail_z_norm);
    //         printf("==================================================\n");
            
    //         if(fail_w0_norm > 950) {
    //             printf("🎯 结论锁定：几乎 100%% 被 w0 范数拒绝！\n");
    //             printf("👉 原因：您的 decompose 魔法常数 M, A, S 错了！\n");
    //             printf("👉 对策：必须为 GAMMA2=(Q-1)/4 重新算一套魔数！\n");
    //         } else if (fail_w1 > 950) {
    //             printf("🎯 结论锁定：几乎 100%% 被 w1 漂移拒绝！\n");
    //             printf("👉 原因：底层的 polyt0_unpack 可能漏写了 D=5 和 D=6 的分支，导致 t0 全是乱码！\n");
    //         } else {
    //             printf("🎯 结论锁定：拒绝率分布异常，极大可能是 K=7, L=7 导致了数组越界！\n");
    //         }
    //         exit(1); 
    //     }
    // 5.1 采样掩码向量 y
    polyvecl_uniform_gamma1(&y, rhoprime, nonce++);

    // 5.2 计算 w = A * y
    z = y;

    // for(int i = 0; i < L; ++i) {
    //   for(int j = 0; j < N; ++j) {
    //     if(z.vec[i].coeffs[j] < 0) z.vec[i].coeffs[j] += Q;
    //   }
    // }

    polyvecl_ntt(&z);
    polyvec_matrix_pointwise_montgomery(&w, mat, &z);
    polyveck_reduce(&w);
    polyveck_invntt_tomont(&w);
    polyveck_caddq(&w); // 将系数转换到正数 [0, q-1]

    // 【新增】保存原始的 w，用于后续结合 s2 和 t0 进行验签一致性模拟
    polyveck w_original = w; 

    // 5.3 高低位拆分 w = w1 * 2^gamma2 + w0
    polyveck_decompose(&w1, &w0, &w);
    
    // for(int i = 0; i < K; ++i) {
    //   for(int j = 0; j < N; ++j) {
    //     int32_t a = w.vec[i].coeffs[j];
    //     // 纯数学的 Decompose，完美适配任何 Q 和 GAMMA2
    //     int32_t a1 = (a + GAMMA2) / (2 * GAMMA2);
    //     int32_t a0 = a - a1 * 2 * GAMMA2;
    //     if (a1 == (Q - 1) / (2 * GAMMA2)) {
    //       a1 = 0;
    //       a0 = a - Q;
    //     }
    //     w1.vec[i].coeffs[j] = a1;
    //     w0.vec[i].coeffs[j] = a0;
    //   }
    // }

    // 5.4 计算挑战哈希 c = CRH(mu || w1)
    polyveck_pack_w1(sig, &w1); 
    shake256_init(&state);
    shake256_absorb(&state, mu, CRHBYTES);
    shake256_absorb(&state, sig, K*POLYW1_PACKEDBYTES);
    shake256_finalize(&state);
    // 【修复致命错误】改用 squeeze 按字节输出，防止缓冲区严重溢出
    shake256_squeeze(sig, CTILDEBYTES, &state);
    
    // 从哈希中生成挑战多项式 c
    poly_challenge(&cp, sig);
    poly_ntt(&cp);

    // 5.5 计算 z = y + c * s1
    polyvecl_pointwise_poly_montgomery(&z, &cp, &s1);
    polyvecl_invntt_tomont(&z);

    // printf("c*s1[0]: %d\n", z.vec[0].coeffs[0]);

    polyvecl_add(&z, &z, &y);
    polyvecl_reduce(&z);
    
    
    // printf("y[0]: %d, z[0]: %d\n", y.vec[0].coeffs[0], z.vec[0].coeffs[0]);
    // 【检查 1】: z 的无穷范数必须小于 gamma1 - beta
    if(polyvecl_chknorm(&z, GAMMA1 - BETA_Z))
      {
        // printf("【检查 1】not pass\n");
        global_fail_zl0++;
        continue;
      } // 越界，拒绝当前采样，重试

    // 5.6 【核心修复】完整计算验签方将还原的 w_approx 并验证 w1'
    polyveck cs2, ct0, u, u1, u0, w_approx, w1_prime, w0_prime;
    
    // 计算 c * s2
    polyveck_pointwise_poly_montgomery(&cs2, &cp, &s2);
    polyveck_invntt_tomont(&cs2);

    // 计算 u = w_original - c*s2
    polyveck_sub(&u, &w_original, &cs2);
    polyveck_reduce(&u);
    polyveck_caddq(&u);

    // 拆分得到 u0 (对应文档步骤 5)
    polyveck_decompose(&u1, &u0, &u);

    // 【检查 2】: 严格检查 u0 的无穷范数 (对应文档步骤 6)
    if(polyveck_chknorm(&u0, GAMMA2 - BETA_W)) {
        global_fail_w0++; // 建议重命名变量反映这是 u0 失败
        continue; 
    }

    // 计算 c * t0
    polyveck_pointwise_poly_montgomery(&ct0, &cp, &t0);
    polyveck_invntt_tomont(&ct0);

    // w_approx = (w - c*s2) + c*t0 = u + c*t0
    polyveck_add(&w_approx, &u, &ct0);
    polyveck_reduce(&w_approx);
    polyveck_caddq(&w_approx);

    // 拆分得到验签端的 w1_prime 和 w0_prime
    polyveck_decompose(&w1_prime, &w0_prime, &w_approx);
    // polyveck w1_prime_test,w0_prime_test;
    // polyveck_decompose(&w1_prime_test, &w0_prime_test, &w_approx);
    // 【新增防线】检查 w1' 是否与 w1 完全一致
    // 如果不一致，说明验签时会得到错误的哈希，必须拒绝

    int w1_match = 1;
    for(int i = 0; i < K; ++i) {
      for(int j = 0; j < N; ++j) {
        // int32_t val = w_approx.vec[i].coeffs[j];// w_original.vec[i].coeffs[j] - cs2.vec[i].coeffs[j] + ct0.vec[i].coeffs[j];
        // // 4. 数学安全拆分得到 a1 (即 w1_prime) 和 a0 (即 w0_prime)
        // int32_t a1 = (val + GAMMA2) / (2 * GAMMA2);
        // int32_t a0 = val - a1 * 2 * GAMMA2;
        // if (a1 == (Q - 1) / (2 * GAMMA2)) {
        //   a1 = 0;
        //   a0 = val - Q;
        // }
        // w0_prime.vec[i].coeffs[j] = a0;
        // w1_prime.vec[i].coeffs[j]=a1;
        if(w1_prime.vec[i].coeffs[j] != w1.vec[i].coeffs[j]) {
          w1_match = 0;
          break; // 跳出内层循环
        }
      }
      if(!w1_match) break; // 跳出外层循环
    }
    if(!w1_match) {
      global_fail_w1++; // 埋点
      // fail_w1++;
      // printf("【检查 w1】not pass\n");
      continue; // w1 发生漂移，拒绝当前采样
    }
    // for(int i = 0; i < K; ++i) {
    //   for(int j = 0; j < N; ++j) {
    //     if (w0_prime.vec[i].coeffs[j] != w0_prime_test.vec[i].coeffs[j]) {
    //       printf("no. %d : %d, %d, %d ;",j,w_approx.vec[i].coeffs[j],w0_prime.vec[i].coeffs[j],w0_prime_test.vec[i].coeffs[j]);
    //     }
    //   }
    // }
    // int w1_match = 1;
    // for(int i = 0; i < K; ++i) {
    //   for(int j = 0; j < N; ++j) {
    //     int32_t val = w_approx.vec[i].coeffs[j];// w_original.vec[i].coeffs[j] - cs2.vec[i].coeffs[j] + ct0.vec[i].coeffs[j];
    //     // 4. 数学安全拆分得到 a1 (即 w1_prime) 和 a0 (即 w0_prime)
    //     int32_t a1 = (val + GAMMA2) / (2 * GAMMA2);
    //     int32_t a0 = val - a1 * 2 * GAMMA2;
    //     if (a1 == (Q - 1) / (2 * GAMMA2)) {
    //       a1 = 0;
    //       a0 = val - Q;
    //     }

    //     w0_prime.vec[i].coeffs[j] = a0; // 保留供下方的无穷范数和 L2 检查使用

    //     // 5. 立即核对是否相等
    //     if(a1 != w1.vec[i].coeffs[j]) {
    //       w1_match = 0;
    //       break; // 若不等，直接跳出 j 循环
    //     }
    //   }
    //   if(!w1_match) break; // 若不等，直接跳出 i 循环
    // }

    // if(!w1_match) {
    //   continue;
    // }

          // printf("\n【检查 w1】not pass ====== 🚨 致命错误现场勘探 🚨 ======\n");
      // // 1. 寻找具体是哪个系数发生了漂移
      // for(int i = 0; i < K; ++i) {
      //   for(int j = 0; j < N; ++j) {
      //     if(w1_prime.vec[i].coeffs[j] != w1.vec[i].coeffs[j]) {
      //       printf("在向量索引 [%d] 的系数 [%d] 处发生 w1 漂移:\n", i, j);
      //       printf("原始 w  = %d  =>  高位 w1 = %d, 低位 w0 = %d\n", 
      //              w_original.vec[i].coeffs[j], w1.vec[i].coeffs[j], w0.vec[i].coeffs[j]);
            
      //       printf("\n--- 异常的干扰项 --- \n");
      //       printf("c * s2 = %d (正常应该在 -30 到 30 左右)\n", cs2.vec[i].coeffs[j]);
      //       printf("c * t0 = %d (正常应该在 -240 到 240 左右)\n", ct0.vec[i].coeffs[j]);
            
      //       // 计算过程回放
      //       int32_t val = w_original.vec[i].coeffs[j] - cs2.vec[i].coeffs[j] + ct0.vec[i].coeffs[j];
      //       val = val % Q; if(val < 0) val += Q;
            
      //       printf("\n计算出的 w_approx = %d  =>  高位 w1' = %d, 低位 w0' = %d\n", 
      //              val, w1_prime.vec[i].coeffs[j], w0_prime.vec[i].coeffs[j]);
      //       break; // 打印一个就够了
      //     }
      //   }
      //   if(!w1_match) break;
      // }

      // 2. 检查 s2 和 t0 本身是不是已经变成了垃圾数据
      // 需要先逆 NTT 转换回正常域才能看
      // polyveck s2_test = s2;
      // polyveck t0_test = t0;
      // polyveck_invntt_tomont(&s2_test);
      // polyveck_invntt_tomont(&t0_test);
      
      // 强制居中对齐以便阅读
      // for(int i=0; i<K; ++i){
      //   for(int j=0; j<N; ++j){
      //      if(s2_test.vec[i].coeffs[j] > (Q-1)/2) s2_test.vec[i].coeffs[j] -= Q;
      //      else if(s2_test.vec[i].coeffs[j] < -(Q-1)/2) s2_test.vec[i].coeffs[j] += Q;
      //      if(t0_test.vec[i].coeffs[j] > (Q-1)/2) t0_test.vec[i].coeffs[j] -= Q;
      //      else if(t0_test.vec[i].coeffs[j] < -(Q-1)/2) t0_test.vec[i].coeffs[j] += Q;
      //   }
      // }
      
      // printf("\n--- 私钥解包探伤 (极大概率这里全是几十万的乱码) ---\n");
      // printf("s2[0] 的前三个系数: %d, %d, %d (应全部为 -1, 0 或 1)\n", 
      //        s2_test.vec[0].coeffs[0], s2_test.vec[0].coeffs[1], s2_test.vec[0].coeffs[2]);
      // printf("t0[0] 的前三个系数: %d, %d, %d (应全部在 -8 到 8 之间)\n", 
      //        t0_test.vec[0].coeffs[0], t0_test.vec[0].coeffs[1], t0_test.vec[0].coeffs[2]);
      
      // printf("==================================\n\n");
      // exit(1); // 强制停止，等待诊断
    // 【检查 2】: 使用新的 w0' 检查无穷范数
    // if(polyveck_chknorm(&w0_prime, GAMMA2 - BETA_W))
    //   {
    //     // fail_w0_norm++;
    //     global_fail_w0++; // 埋点
    //     // printf("【检查 2】not pass\n");
    //     continue;}

    // 【检查 3】: 欧氏范数 (L2) 检查
    // 提醒：根据您的算法文档，L2 范数如果【太小】(< B) 会拒绝
    // 确保您的 polyvec_check_L2_bound 逻辑是: if (L2_norm < B) return 1;
    if(polyvec_check_L2_bound(&z, &w0_prime) == 1)
      {
        // fail_z_norm++;
        // printf("【检查 3】not pass\n");
        global_fail_zl2++; // 埋点
        continue;}

    // ====== 📸 签名端黄金快照 ======
    // printf("\n--- 📝 签名端黄金快照 ---\n");
    // printf("Signer z[0] 前三项   : %d, %d, %d\n", 
    //        z.vec[0].coeffs[0], z.vec[0].coeffs[1], z.vec[0].coeffs[2]);
    // printf("Signer w_approx 前三项: %d, %d, %d\n", 
    //        w_approx.vec[0].coeffs[0], w_approx.vec[0].coeffs[1], w_approx.vec[0].coeffs[2]);
    // printf("Signer w1' 前三项     : %d, %d, %d\n", 
    //        w1_prime.vec[0].coeffs[0], w1_prime.vec[0].coeffs[1], w1_prime.vec[0].coeffs[2]);
    // printf("Signer w0' 前三项     : %d, %d, %d\n", 
    //        w0_prime.vec[0].coeffs[0], w0_prime.vec[0].coeffs[1], w0_prime.vec[0].coeffs[2]);
    // printf("---------------------------\n");
    
    // 如果所有检查全部通过，跳出循环！

    // 如果所有检查全部通过，跳出循环！
    break;
  }
  // printf("Done! \n");
  // 6. 打包最终签名 (只包含挑战 c 和 向量 z)
  // 使用我们在 packing.c 中重写的 pack_sig
  pack_sig(sig, sig, &z);
  *siglen = CRYPTO_BYTES;
  global_success_signs++; // 成功次数 +1
  // printf("sign finish! \n");
  return 0;
}

/*************************************************
* Name:        crypto_sign_signature
*
* Description: Computes signature.
*
* Arguments:   - uint8_t *sig:   pointer to output signature (of length CRYPTO_BYTES)
*              - size_t *siglen: pointer to output length of signature
*              - uint8_t *m:     pointer to message to be signed
*              - size_t mlen:    length of message
*              - uint8_t *ctx:   pointer to contex string
*              - size_t ctxlen:  length of contex string
*              - uint8_t *sk:    pointer to bit-packed secret key
*
* Returns 0 (success) or -1 (context string too long)
**************************************************/
int crypto_sign_signature(uint8_t *sig,
                          size_t *siglen,
                          const uint8_t *m,
                          size_t mlen,
                          const uint8_t *ctx,
                          size_t ctxlen,
                          const uint8_t *sk)
{
  size_t i;
  uint8_t pre[257];
  uint8_t rnd[RNDBYTES];

  if(ctxlen > 255)
    return -1;

  /* Prepare pre = (0, ctxlen, ctx) */
  pre[0] = 0;
  pre[1] = ctxlen;
  for(i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];

#ifdef COMPASS_SIG_RANDOMIZED_SIGNING
  randombytes(rnd, RNDBYTES);
#else
  for(i=0;i<RNDBYTES;i++)
    rnd[i] = 0;
#endif

  crypto_sign_signature_internal(sig,siglen,m,mlen,pre,2+ctxlen,rnd,sk);
  return 0;
}

/*************************************************
* Name:        crypto_sign
*
* Description: Compute signed message.
*
* Arguments:   - uint8_t *sm: pointer to output signed message (allocated
*                             array with CRYPTO_BYTES + mlen bytes),
*                             can be equal to m
*              - size_t *smlen: pointer to output length of signed
*                               message
*              - const uint8_t *m: pointer to message to be signed
*              - size_t mlen: length of message
*              - const uint8_t *ctx: pointer to context string
*              - size_t ctxlen: length of context string
*              - const uint8_t *sk: pointer to bit-packed secret key
*
* Returns 0 (success) or -1 (context string too long)
**************************************************/
int crypto_sign(uint8_t *sm,
                size_t *smlen,
                const uint8_t *m,
                size_t mlen,
                const uint8_t *ctx,
                size_t ctxlen,
                const uint8_t *sk)
{
  int ret;
  size_t i;

  for(i = 0; i < mlen; ++i)
    sm[CRYPTO_BYTES + mlen - 1 - i] = m[mlen - 1 - i];
  ret = crypto_sign_signature(sm, smlen, sm + CRYPTO_BYTES, mlen, ctx, ctxlen, sk);
  *smlen += mlen;
  return ret;
}

/*************************************************
* Name:        crypto_sign_verify_internal
*
* Description: Verifies signature. Internal API.
*
* Arguments:   - uint8_t *m: pointer to input signature
*              - size_t siglen: length of signature
*              - const uint8_t *m: pointer to message
*              - size_t mlen: length of message
*              - const uint8_t *pre: pointer to prefix string
*              - size_t prelen: length of prefix string
*              - const uint8_t *pk: pointer to bit-packed public key
*
* Returns 0 if signature could be verified correctly and -1 otherwise
**************************************************/
int crypto_sign_verify_internal(const uint8_t *sig,
                                size_t siglen,
                                const uint8_t *m,
                                size_t mlen,
                                const uint8_t *pre,
                                size_t prelen,
                                const uint8_t *pk)
{
  // unsigned int i;
  uint8_t buf[K*POLYW1_PACKEDBYTES];
  uint8_t rho[SEEDBYTES];
  uint8_t mu[CRHBYTES];
  uint8_t c[CTILDEBYTES];
  uint8_t c2[CTILDEBYTES];
  poly cp;
  polyvecl mat[K], z, z_ntt;  // 新增 z_ntt 用于备份
  polyveck t1, w1, w0;
  keccak_state state;

  if(siglen != CRYPTO_BYTES)
    return -1;

  // printf("\n====== 🛡️ 进入验签过程探伤 🛡️ ======\n");

  // 1. 测试解包过程
  // printf(">>> [1] 准备解包签名 (unpack_sig)...\n");
  // 1. 解包公钥和签名
  
  unpack_pk(rho, &t1, pk);

  // printf(">>> [1] 签名解包通过！\n");

  // printf(">>> [2] 准备解包公钥 (unpack_pk)...\n");
  if(unpack_sig(c, &z, sig))
    return -1;
  // printf(">>> [2] 公钥解包通过！\n");
  // 2. 检查 z 的无穷范数 (第一道防线)
  if(polyvecl_chknorm(&z, GAMMA1 - BETA_Z))
    return -1;

  // 3. 计算消息哈希 mu (CRH(CRH(pk) || M))

  /* Compute CRH(H(rho, t1), pre, msg) */
  shake256(mu, TRBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  shake256_init(&state);
  shake256_absorb(&state, mu, TRBYTES);
  shake256_absorb(&state, pre, prelen);
  shake256_absorb(&state, m, mlen);
  shake256_finalize(&state);
  shake256_squeeze(mu, CRHBYTES, &state);

  // 4. 恢复挑战多项式 c
  poly_challenge(&cp, c);
  poly_ntt(&cp);

  // 5. 展开公钥矩阵 A
  polyvec_matrix_expand(mat, rho);

  // 6. 【核心避坑】拷贝一份 z 用于 NTT 乘法，保留原始 z 用于最终 L2 检查
  z_ntt = z;
  polyvecl_ntt(&z_ntt);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &z_ntt);

  // 7. 处理公钥 t1: 左移 d 位后乘以 c
  polyveck_shiftl(&t1);
  polyveck_ntt(&t1);
  polyveck_pointwise_poly_montgomery(&t1, &cp, &t1);

  // 8. 计算 w_approx = A*z - c*t1*2^d
  polyveck_sub(&w1, &w1, &t1);
  polyveck_reduce(&w1);
  polyveck_invntt_tomont(&w1);
  polyveck_caddq(&w1); // 映射到正数域

  // 9. 无 Hint 的纯粹高低位拆分 (w_approx -> w1, w0)
  // 原版的 use_hint 被废弃，这里直接调用拆分
  polyveck_decompose(&w1, &w0, &w1);

  // polyveck w0_prime; 
  // for(int i = 0; i < K; ++i) {
  //   for(int j = 0; j < N; ++j) {
  //     // 规范化到 [0, Q-1]
  //     int32_t val = w1.vec[i].coeffs[j] % Q;
  //     if(val < 0) val += Q;

  //     // 纯数学拆分（绝对忠实于您的设计文档）
  //     int32_t a1 = (val + GAMMA2) / (2 * GAMMA2);
  //     int32_t a0 = val - a1 * 2 * GAMMA2;
  //     if (a1 == (Q - 1) / (2 * GAMMA2)) {
  //       a1 = 0;
  //       a0 = val - Q;
  //     }

  //     w1.vec[i].coeffs[j] = a1;       // 存入 w1 供后续打包并计算哈希
  //     w0_prime.vec[i].coeffs[j] = a0; // 存入 w0_prime 供范数检查
  //   }
  // }
  
  // ====== 📸 验签端黄金快照 =====
  
  // 此时的 w1 数组存的是刚刚拆分出的 a1，我们需要看拆分前的重构多项式
  // 因此我们重新算一下第一个元素的原始 val 打印出来
  // int32_t val0 = w1.vec[0].coeffs[0] % Q; if(val0 < 0) val0 += Q;
  // int32_t val1 = w1.vec[0].coeffs[1] % Q; if(val1 < 0) val1 += Q;
  // int32_t val2 = w1.vec[0].coeffs[2] % Q; if(val2 < 0) val2 += Q;

  // 10. 打包高位 w1 并重新计算挑战哈希 c'
  // 请确保这里传入的 buf 数组定义大小不小于上述打印值！
  polyveck_pack_w1(buf, &w1); 

  shake256_init(&state);
  shake256_absorb(&state, mu, CRHBYTES);
  shake256_absorb(&state, buf, K*POLYW1_PACKEDBYTES);
  shake256_finalize(&state);
  shake256_squeeze(c2, CTILDEBYTES, &state);

  // 11. 校验哈希是否严格一致
  // int hash_match = 1;
  for(int i = 0; i < CTILDEBYTES; ++i) {
    if(c[i] != c2[i]) return -1;
  }
  // if(!hash_match) {
  //   printf("校验哈希是否严格一致 fail \n");
  //   printf("\n>>> [!] 致命错误：哈希值 c2 不等于 c！\n");
  //   printf("这意味着验签端的 w_approx 和签名端不一样！\n");
  //   printf("请立刻去检查 rounding.c 里的 poly_power2round 函数，\n");
  //   printf("确保 KeyGen 阶段生成 t1 和 t0 时，也是用的 D=4 而不是 13！\n");
  //   return -1;
  // } else {
  //   printf("\n>>> [✓] 奇迹发生：哈希完全匹配，w1 分毫不差！\n");
  // }

  // for(int i = 0; i < K; ++i) {
  //   for(int j = 0; j < N; ++j) {
  //     int32_t val = w0_prime.vec[i].coeffs[j];
  //     if(val < 0) val = -val; // 取绝对值
  //     if(val >= GAMMA2 - BETA_W) {
  //       printf(">>> [!] 抓到越界点: i=%d, j=%d, 值=%d, 允许的界限=%d\n", 
  //              i, j, w0_prime.vec[i].coeffs[j], GAMMA2 - BETA_W);
  //       return -1;
  //     }
  //   }
  // }

  // 12. 检查低位 w0' 的无穷范数 (第二道防线)

  // 13. 核心创新检查：欧氏范数 (L2) 综合防线
  // 利用保留的原始 z 和刚刚拆分出的 w0 进行最终校验
  if(polyvec_check_L2_bound(&z, &w0) == 1)
    {
      // printf("核心创新检查：欧氏范数 (L2) fail \n");
      return -1;}
  // printf("🎉 验签完全成功！准备 return 0 退出函数...\n");
  return 0;
}

/*************************************************
* Name:        crypto_sign_verify
*
* Description: Verifies signature.
*
* Arguments:   - uint8_t *m: pointer to input signature
*              - size_t siglen: length of signature
*              - const uint8_t *m: pointer to message
*              - size_t mlen: length of message
*              - const uint8_t *ctx: pointer to context string
*              - size_t ctxlen: length of context string
*              - const uint8_t *pk: pointer to bit-packed public key
*
* Returns 0 if signature could be verified correctly and -1 otherwise
**************************************************/
int crypto_sign_verify(const uint8_t *sig,
                       size_t siglen,
                       const uint8_t *m,
                       size_t mlen,
                       const uint8_t *ctx,
                       size_t ctxlen,
                       const uint8_t *pk)
{
  size_t i;
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  pre[0] = 0;
  pre[1] = ctxlen;
  for(i = 0; i < ctxlen; i++)
    pre[2 + i] = ctx[i];

  return crypto_sign_verify_internal(sig,siglen,m,mlen,pre,2+ctxlen,pk);
}

/*************************************************
* Name:        crypto_sign_open
*
* Description: Verify signed message.
*
* Arguments:   - uint8_t *m: pointer to output message (allocated
*                            array with smlen bytes), can be equal to sm
*              - size_t *mlen: pointer to output length of message
*              - const uint8_t *sm: pointer to signed message
*              - size_t smlen: length of signed message
*              - const uint8_t *ctx: pointer to context tring
*              - size_t ctxlen: length of context string
*              - const uint8_t *pk: pointer to bit-packed public key
*
* Returns 0 if signed message could be verified correctly and -1 otherwise
**************************************************/
int crypto_sign_open(uint8_t *m,
                     size_t *mlen,
                     const uint8_t *sm,
                     size_t smlen,
                     const uint8_t *ctx,
                     size_t ctxlen,
                     const uint8_t *pk)
{
  size_t i;

  if(smlen < CRYPTO_BYTES)
    goto badsig;

  *mlen = smlen - CRYPTO_BYTES;
  if(crypto_sign_verify(sm, CRYPTO_BYTES, sm + CRYPTO_BYTES, *mlen, ctx, ctxlen, pk))
    goto badsig;
  else {
    /* All good, copy msg, return 0 */
    for(i = 0; i < *mlen; ++i)
      m[i] = sm[CRYPTO_BYTES + i];
    return 0;
  }

badsig:
  /* Signature verification failed */
  *mlen = 0;
  for(i = 0; i < smlen; ++i)
    m[i] = 0;

  return -1;
}
