/* === QingLuan-128 instance: security level fixed for this submission folder.
 * Per algorithm-requirements (6): each AlgorithmInstance lives in its own folder.
 * Builds with a bare "gcc -Iinclude" (no -DQINGLUAN_xxx needed). === */
#ifndef QINGLUAN_128
#define QINGLUAN_128
#endif

/*
 * QingLuan Digital Signature Scheme
 * params.h - Parameter definitions for all security levels
 *
 * v2 redesign: QingLuan aligns to the CROSS-RSDP protocol (see
 * docs/protocol_reference.md). The base field and restriction are FIXED for
 * every security level (as in CROSS-RSDP): p = 127, z = 7, g = 2,
 * E = <2> = {1,2,4,8,16,32,64}. Security levels differ only in (n, k, t, w).
 */

#ifndef QINGLUAN_PARAMS_H
#define QINGLUAN_PARAMS_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * 开发用玩具参数 / Development-only TOY parameters
 * 仅用于快速 TDD,绝不用于安全用途。与 QINGLUAN_128/256/512 互斥。
 * 结构与 CROSS-RSDP 一致 (小 n、t 轮、固定权重 w),仅规模极小。
 * ============================================================ */
#ifdef QL_TOY
#define QINGLUAN_NAME       "QingLuan-TOY"
#define QINGLUAN_SECURITY    8
#define PARAM_Q              127     /* base field F_p */
#define PARAM_Q_BITS         7       /* bits to represent p-1=126 */
#define PARAM_G              2       /* restricted subgroup generator g */
#define PARAM_Z              7       /* |E| = z, E = <2> mod 127 */
#define PARAM_Z_BITS         3       /* bits to represent z-1=6 */
#define PARAM_N              12      /* code length n (tiny) */
#define PARAM_K              6       /* code dimension k */
#define PARAM_R              6       /* redundancy r = n - k */
#define PARAM_TAU            8       /* number of rounds t */
#define PARAM_W              5       /* weight of second challenge (chall2[i]=1) */
#define PARAM_LAMBDA         128     /* security parameter lambda (toy: nominal) */
#define PARAM_SEED_BYTES     16      /* round-seed length = lambda/8 */
#define PARAM_SALT_BYTES     32      /* salt length = 2*lambda/8 */
#define PARAM_KEYSEED_BYTES  32      /* key seed length = 2*lambda/8 */
#define PARAM_HASH_BYTES     32      /* Hash output = 2*lambda/8 (HASH_PIPES via hash.h) */
#endif /* QL_TOY */

/*
 * 安全等级选择 / Security level selection
 * 编译时定义以下宏之一: QINGLUAN_128, QINGLUAN_256, QINGLUAN_384, QINGLUAN_512
 * 默认: QINGLUAN_128.  切换等级需先 clean 再重新编译。
 *
 * 参数 (n,k,t,w) 由 tools/rsdp_estimator.py 确定 (见 self_test 复现 CROSS
 * Table 5/7)。经典安全取 min(密钥恢复, 伪造); 量子安全保守取 经典/2。
 * 128/256 复用 CROSS-RSDP cat1/cat5 已分析码; 384/512 为线性外推 (含冗余)。
 */
#if !defined(QL_TOY) && !defined(QINGLUAN_128) && !defined(QINGLUAN_256) && \
    !defined(QINGLUAN_384) && !defined(QINGLUAN_512)
#define QINGLUAN_128
#endif

/* Fixed base field + restriction for ALL real levels (CROSS-RSDP). */
#if !defined(QL_TOY)
#define PARAM_Q              127
#define PARAM_Q_BITS         7
#define PARAM_G              2
#define PARAM_Z              7
#define PARAM_Z_BITS         3
#endif

/* ============================================================
 * QingLuan-128: 经典 128 / 量子 >=80 位 (复用 CROSS-RSDP cat1 码)
 *   密钥恢复 143 位经典 (量子 72); 伪造 raw 128.8 位经典 (量子 64)。
 *   量子下限 80 由 NIST cat-1 (AES-128, 有限深度 Grover) 等价给出,
 *   与技术要求 128->80 比值一致。详见 tools/rsdp_estimator.py。
 * ============================================================ */
#if defined(QINGLUAN_128) && !defined(QL_TOY)
#define QINGLUAN_NAME       "QingLuan-128"
#define QINGLUAN_SECURITY    128
#define PARAM_N              127
#define PARAM_K              76
#define PARAM_R              51      /* n - k */
#define PARAM_TAU            256     /* t */
#define PARAM_W              212     /* w (raw forgery 128.8 bit, no +5 credit) */
#define PARAM_LAMBDA         128
#define PARAM_SEED_BYTES     16      /* lambda/8 */
#define PARAM_SALT_BYTES     32      /* 2*lambda/8 */
#define PARAM_KEYSEED_BYTES  32      /* 2*lambda/8 */
#define PARAM_HASH_BYTES     32      /* 2*lambda/8 */
#endif /* QINGLUAN_128 */

/* ============================================================
 * QingLuan-256: 经典 256 / 量子 >=128 位 (复用 CROSS-RSDP cat5 码)
 *   密钥恢复 274 位经典 (量子 137); 伪造 raw 257.2 位经典 (量子 128.6)。
 *   两项量子安全均 >=128 (无需有限深度信用)。
 * ============================================================ */
#if defined(QINGLUAN_256) && !defined(QL_TOY)
#define QINGLUAN_NAME       "QingLuan-256"
#define QINGLUAN_SECURITY    256
#define PARAM_N              251
#define PARAM_K              150
#define PARAM_R              101
#define PARAM_TAU            512
#define PARAM_W              424     /* w (raw forgery 257.2 bit) */
#define PARAM_LAMBDA         256
#define PARAM_SEED_BYTES     32      /* lambda/8 */
#define PARAM_SALT_BYTES     64      /* 2*lambda/8 */
#define PARAM_KEYSEED_BYTES  64
#define PARAM_HASH_BYTES     64      /* 2*lambda/8 */
#endif /* QINGLUAN_256 */

/* ============================================================
 * QingLuan-384 (可选): 经典 384 / 量子 >=192 位 (线性外推, 含冗余)
 *   密钥恢复 400 位经典 (量子 200); 伪造 raw 384.3 位经典 (量子 192.2)。
 *   (n,k) 为 CROSS Table 5 线性外推; 标准化前需用官方 CROSS 估算器复核。
 * ============================================================ */
#if defined(QINGLUAN_384) && !defined(QL_TOY)
#define QINGLUAN_NAME       "QingLuan-384"
#define QINGLUAN_SECURITY    384
#define PARAM_N              370
#define PARAM_K              221
#define PARAM_R              149
#define PARAM_TAU            763
#define PARAM_W              631     /* w (raw forgery 384.3 bit) */
#define PARAM_LAMBDA         384
#define PARAM_SEED_BYTES     48      /* lambda/8 */
#define PARAM_SALT_BYTES     96      /* 2*lambda/8 */
#define PARAM_KEYSEED_BYTES  96
#define PARAM_HASH_BYTES     96      /* 2*lambda/8 (3 SM3 pipes) */
#endif /* QINGLUAN_384 */

/* ============================================================
 * QingLuan-512: 经典 512 / 量子 >=256 位 (线性外推, 含冗余)
 *   密钥恢复 528 位经典 (量子 264); 伪造 raw 512.1 位经典 (量子 256.1)。
 *   (n,k) 为 CROSS Table 5 线性外推; 标准化前需用官方 CROSS 估算器复核。
 * ============================================================ */
#if defined(QINGLUAN_512) && !defined(QL_TOY)
#define QINGLUAN_NAME       "QingLuan-512"
#define QINGLUAN_SECURITY    512
#define PARAM_N              491
#define PARAM_K              293
#define PARAM_R              198     /* n - k */
#define PARAM_TAU            1018    /* t */
#define PARAM_W              842     /* w (raw forgery 512.1 bit) */
#define PARAM_LAMBDA         512
#define PARAM_SEED_BYTES     64
#define PARAM_SALT_BYTES     128
#define PARAM_KEYSEED_BYTES  128
#define PARAM_HASH_BYTES     128
#endif /* QINGLUAN_512 */

/* ============================================================
 * 派生参数 / Derived parameters
 * ============================================================ */

/* 域元素打包: F_p 用 Q_BITS=7 比特, F_z 用 Z_BITS=3 比特, 小端比特打包, 补零到字节 */
#define QINGLUAN_SYNDROME_BYTES  ((PARAM_R * PARAM_Q_BITS + 7) / 8)   /* s: r elems */
#define QINGLUAN_Y_BYTES         ((PARAM_N * PARAM_Q_BITS + 7) / 8)   /* y: n F_p elems */
#define QINGLUAN_V_BYTES         ((PARAM_N * PARAM_Z_BITS + 7) / 8)   /* v: n F_z elems */

/* 密钥大小: sk = Seed_sk (2λ); pk = Seed_pk (2λ) || pack(s) */
#define QINGLUAN_SK_BYTES        (PARAM_KEYSEED_BYTES)
#define QINGLUAN_PK_BYTES        (PARAM_KEYSEED_BYTES + QINGLUAN_SYNDROME_BYTES)

/* 域分离常量 c = 2t - 1 (CROSS 约定) */
#define PARAM_C                  (2 * PARAM_TAU - 1)

/*
 * 签名布局 (fast-style 直接发送, 见 protocol_reference.md §10):
 *   Salt || digest_cmt || digest_chall2
 *   || Path:  w 个 Seed[i]   (chall2[i]=1)
 *   || Proof: w 个 cmt0[i]   (chall2[i]=1)
 *   || resp:  (t-w) 个 { pack(y[i]) || pack(v[i]) || cmt1[i] }  (chall2[i]=0)
 */
#define SIG_RESP0_BYTES   (QINGLUAN_Y_BYTES + QINGLUAN_V_BYTES + PARAM_HASH_BYTES)
#define QINGLUAN_SIG_BYTES \
    (PARAM_SALT_BYTES + 2 * PARAM_HASH_BYTES \
     + PARAM_W * PARAM_SEED_BYTES \
     + PARAM_W * PARAM_HASH_BYTES \
     + (PARAM_TAU - PARAM_W) * SIG_RESP0_BYTES)

/*
 * 域元素类型 / Field element type. p = 127 <= 255 -> uint8_t for all levels.
 */
#if PARAM_Q <= 255
typedef uint8_t fq_t;
#else
typedef uint16_t fq_t;
#endif

/*
 * Barrett 约简常量 / Barrett reduction constants (single-mul result, <= (q-1)^2).
 * 累加器约简使用 fq_arith.c 中的 BARRETT_SUM_M/K (K=32)。
 * p = 127 对所有等级一致。
 */
#define BARRETT_M    129     /* floor(2^14 / 127) */
#define BARRETT_K    14

/* ============================================================
 * 域分离标签 / Domain separation tags (1-byte prefix on every Hash/CSPRNG)
 * 配合各调用附加的 2 字节小端常量 (i+c 等) 区分实例。
 * ============================================================ */
#define DOMAIN_EXPAND_SK    0x00   /* (Seed_e, Seed_pk) <- CSPRNG(Seed_sk) */
#define DOMAIN_MATRIX       0x01   /* V <- CSPRNG(Seed_pk) */
#define DOMAIN_ERROR        0x02   /* eta <- CSPRNG(Seed_e) */
#define DOMAIN_SEEDLEAVES   0x03   /* Seed[1..t] <- CSPRNG(Seed | Salt) */
#define DOMAIN_ROUND        0x04   /* (eta'[i], u'[i]) <- CSPRNG(Seed[i]|Salt|i+c) */
#define DOMAIN_CMT0         0x05   /* cmt0[i] = Hash(s'[i]|v[i]|Salt|i+c) */
#define DOMAIN_CMT1         0x06   /* cmt1[i] = Hash(Seed[i]|Salt|i+c) */
#define DOMAIN_DIGEST_CMT0  0x07   /* digest_cmt0 = Hash(cmt0[1..t]) */
#define DOMAIN_DIGEST_CMT1  0x08   /* digest_cmt1 = Hash(cmt1[1..t]) */
#define DOMAIN_DIGEST_CMT   0x09   /* digest_cmt  = Hash(digest_cmt0|digest_cmt1) */
#define DOMAIN_MSG          0x0A   /* digest_Msg  = Hash(Salt|pk_hash|Msg) (Salt=eTCR randomizer) */
#define DOMAIN_CHALL1       0x0B   /* digest_chall1 = Hash(digest_Msg|digest_cmt|Salt) */
#define DOMAIN_CHALL1_GEN   0x0C   /* chall1 <- CSPRNG(digest_chall1|t+c) */
#define DOMAIN_CHALL2       0x0D   /* digest_chall2 = Hash(y[1..t]|digest_chall1) */
#define DOMAIN_CHALL2_GEN   0x0E   /* chall2 <- CSPRNG(digest_chall2|t+c+1) */
#define DOMAIN_COMMIT       0x0F   /* generic salted TCR commitment (hash_commit) */

#endif /* QINGLUAN_PARAMS_H */
