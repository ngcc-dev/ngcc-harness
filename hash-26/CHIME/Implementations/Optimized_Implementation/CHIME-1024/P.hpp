#ifndef P_HPP
#define P_HPP

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <immintrin.h>

/*
 * Provides 64-byte aligned allocation helpers for the optimized CHIME-1024 implementation.
 * 为 CHIME-1024 优化实现提供 64 字节对齐的内存分配辅助函数。
 */
#ifndef _MSC_VER
#include <mm_malloc.h>
static inline void* am64(size_t n) { return _mm_malloc(n, 64); }
static inline void  af64(void* p)  { _mm_free(p); }
#else
#include <malloc.h>
#include <intrin.h>
static inline void* am64(size_t n) { return _aligned_malloc(n, 64); }
static inline void  af64(void* p)  { _aligned_free(p); }
#endif

/*
 * Stores the six-lane permutation state used by CHIME-1024.
 * 存储 CHIME-1024 使用的六个 lane 置换状态。
 */
struct alignas(64) S {
  __m256i A0, A1, B0, B1, C0, C1;
};

/*
 * Provides the canonical state alias used throughout the optimized code.
 * 提供优化代码中统一使用的状态类型别名。
 */
using State = S;

/*
 * Enables 64-byte alignment hints for GCC-style compilers.
 * 为 GCC 风格编译器启用 64 字节对齐提示。
 */
#if defined(__GNUC__)
#define P2A64() __asm__ __volatile__(".p2align 6, 0x90\n\t" ::: "memory")
#define PA __attribute__((aligned(64), hot))
#define NI __attribute__((noinline))
#else
#define P2A64()
#define PA
#define NI
#endif

/*
 * Applies a 256-bit lane permutation over four 64-bit lanes.
 * 输入：
 *     x：输入向量
 *     i：重排立即数
 * 输出：
 *     返回值：重排后的向量
 *
 * 对 256 位向量的 4 个 64 位 lane 执行重排。
 * 输入：
 *     x：输入向量
 *     i：重排立即数
 * 输出：
 *     返回值：重排后的向量
 */
#define PQ(x, i)   _mm256_permute4x64_epi64((x), (i))
/*
 * Applies a 32-bit shuffle within each 128-bit half.
 * 输入：
 *     x：输入向量
 *     i：shuffle 立即数
 * 输出：
 *     返回值：重排后的向量
 *
 * 在每个 128 位半区内执行 32 位字重排。
 * 输入：
 *     x：输入向量
 *     i：shuffle 立即数
 * 输出：
 *     返回值：重排后的向量
 */
#define SH(x, i)   _mm256_shuffle_epi32((x), (i))
/*
 * Applies a 128-bit half permutation within the same 256-bit vector.
 * 输入：
 *     x：输入向量
 *     i：重排立即数
 * 输出：
 *     返回值：重排后的向量
 *
 * 在同一个 256 位向量内执行 128 位半区重排。
 * 输入：
 *     x：输入向量
 *     i：重排立即数
 * 输出：
 *     返回值：重排后的向量
 */
#define P1(x, i)   _mm256_permute2x128_si256((x), (x), (i))
/*
 * Applies a 128-bit half permutation within the same 256-bit vector.
 * 输入：
 *     x：输入向量
 *     i：重排立即数
 * 输出：
 *     返回值：重排后的向量
 *
 * 在同一个 256 位向量内执行 128 位半区重排。
 * 输入：
 *     x：输入向量
 *     i：重排立即数
 * 输出：
 *     返回值：重排后的向量
 */
#define P128(x, i) _mm256_permute2x128_si256((x), (x), (i))
/*
 * Computes bitwise XOR of two 256-bit vectors.
 * 输入：
 *     a：第一个输入向量
 *     b：第二个输入向量
 * 输出：
 *     返回值：按位异或结果
 *
 * 计算两个 256 位向量的按位异或。
 * 输入：
 *     a：第一个输入向量
 *     b：第二个输入向量
 * 输出：
 *     返回值：按位异或结果
 */
#define X(a, b)    _mm256_xor_si256((a), (b))
/*
 * Computes bitwise AND-NOT of two 256-bit vectors.
 * 输入：
 *     a：按位取反后作为掩码的输入向量
 *     b：第二个输入向量
 * 输出：
 *     返回值：按位与非结果
 *
 * 计算两个 256 位向量的按位与非。
 * 输入：
 *     a：按位取反后作为掩码的输入向量
 *     b：第二个输入向量
 * 输出：
 *     返回值：按位与非结果
 */
#define AN(a, b)   _mm256_andnot_si256((a), (b))

/*
 * Exposes the global round-constant table used by the permutation layer.
 * 暴露置换层使用的全局轮常量向量表。
 */
extern __m256i ARC[20];

/*
 * Initializes the global round-constant table ARC.
 * Inputs:
 *     none
 * Outputs:
 *     ARC: populated in place
 *
 * 初始化全局轮常量表 ARC。
 * 输入：
 *     无
 * 输出：
 *     ARC：原地填充完成
 */
void init_arc();

/*
 * Stores the 64-bit round constants used to derive the vector table.
 * 存储用于生成向量表的 64 位轮常量。
 */
const uint64_t ARC64[20] = {
    0x409AC7567F4A7C15ULL,
    0xE2C34D9DFE94F82BULL,
    0x040BD3C37DDF743DULL,
    0xA670580AFD29F057ULL,
    0xC9B8DE707C746C6DULL,
    0x6BE164B7FBBEE87BULL,
    0x8D29EAFD7B096495ULL,
    0x2F167324FA53E0AFULL,
    0x515EF96A799E5CB5ULL,
    0xF0877FD1F8E8D8DBULL,
    0x12CF8417783354EDULL,
    0xB4340A5EF77DD0F7ULL,
    0xD67C908476C84D1DULL,
    0x79A516CBF612C92BULL,
    0x9BED9F31755D4535ULL,
    0x3DDA2578F4A7C15FULL,
    0x5F02ABBE73F23D75ULL,
    0xC14B31E5F33CB96BULL,
    0x60B3B62B7287359DULL,
    0x82F83C92F1D1B1B7ULL
};

// ============================= Common helpers =============================

/*
 * Rotates each 64-bit lane left by k bits.
 * Inputs:
 *     x: input 256-bit vector
 * Outputs:
 *     return value: lane-wise left-rotated vector
 *
 * 将每个 64 位 lane 循环左移 k 位。
 * 输入：
 *     x：输入 256 位向量
 * 输出：
 *     返回值：按 lane 循环左移后的向量
 */
template<int k>
static inline __m256i ROL(__m256i x) {
  return _mm256_or_si256(_mm256_slli_epi64(x, k), _mm256_srli_epi64(x, 64 - k));
}

/*
 * Rotates each 64-bit lane left by k bits using 64-bit lane semantics.
 * Inputs:
 *     x: input 256-bit vector
 * Outputs:
 *     return value: lane-wise left-rotated vector
 *
 * 按 64 位 lane 语义将每个 lane 循环左移 k 位。
 * 输入：
 *     x：输入 256 位向量
 * 输出：
 *     返回值：按 lane 循环左移后的向量
 */
template<int k>
static inline __m256i ROL64(__m256i x) {
  return _mm256_or_si256(_mm256_slli_epi64(x, k), _mm256_srli_epi64(x, 64 - k));
}

/*
 * Applies chi non-linear mixing to three 256-bit vectors in place.
 * Inputs:
 *     A: first input/output vector
 *     B: second input/output vector
 *     C: third input/output vector
 * Outputs:
 *     A: updated first vector
 *     B: updated second vector
 *     C: updated third vector
 *
 * 对三个 256 位向量执行 chi 非线性运算并原地更新。
 * 输入：
 *     A：第一个输入/输出向量
 *     B：第二个输入/输出向量
 *     C：第三个输入/输出向量
 * 输出：
 *     A：更新后的第一向量
 *     B：更新后的第二向量
 *     C：更新后的第三向量
 */
static inline void chi(__m256i &A, __m256i &B, __m256i &C) {
  __m256i a = A, b = B;
  A = X(a, AN(b, C));
  B = X(b, AN(C, a));
  C = X(C, AN(a, b));
}

/*
 * Applies the linear layer to the permutation state.
 * Inputs:
 *     s: state to transform
 * Outputs:
 *     s: updated state after the linear layer
 *
 * 对置换状态执行线性层。
 * 输入：
 *     s：待变换状态
 * 输出：
 *     s：线性层后的更新状态
 */
static inline void L_M39(S &s) {
  s.B1 = X(s.B1, SH(s.C1, 0x39)); s.B0 = X(s.B0, s.A1);
  s.C0 = X(s.C0, ROL<29>(s.A0)); s.B1 = X(s.B1, ROL<31>(s.A1));
  s.C1 = X(s.C1, ROL<13>(s.A0)); s.C0 = X(s.C0, s.B1);
  s.A0 = PQ(s.A0, 0x93); __m256i T = SH(s.C0, 0x4E);
  s.C1 = X(s.C1, SH(s.B1, 0x39)); s.A1 = P1(s.A1, 0x01);
  s.A0 = X(s.A0, T); s.A1 = X(s.A1, SH(s.B0, 0x39));
  s.B1 = X(s.B1, _mm256_slli_epi64(s.A0, 7));
  s.C1 = X(s.C1, _mm256_srli_epi64(s.A1, 5));
}

/*
 * Performs the forward branch permutation on the state.
 * Inputs:
 *     s: state to permute
 * Outputs:
 *     s: updated state after forward permutation
 *
 * 对状态执行前向分支置换。
 * 输入：
 *     s：待置换状态
 * 输出：
 *     s：前向置换后的更新状态
 */
static inline void RP_fwd(S &s) {
  __m256i t = s.A0;
  s.A0 = s.C1; s.C1 = s.C0; s.C0 = s.B1; s.B1 = s.B0; s.B0 = s.A1; s.A1 = t;
}

/*
 * Performs the reverse branch permutation on the state.
 * Inputs:
 *     s: state to permute
 * Outputs:
 *     s: updated state after reverse permutation
 *
 * 对状态执行反向分支置换。
 * 输入：
 *     s：待置换状态
 * 输出：
 *     s：反向置换后的更新状态
 */
static inline void RP_rev(S &s) {
  __m256i t = s.A0;
  s.A0 = s.A1; s.A1 = s.B0; s.B0 = s.B1; s.B1 = s.C0; s.C0 = s.C1; s.C1 = t;
}

/*
 * Applies chi on the even-round branch schedule.
 * Inputs:
 *     s: state to transform
 * Outputs:
 *     s: updated state after even-round chi
 *
 * 按偶数轮分支调度执行 chi。
 * 输入：
 *     s：待变换状态
 * 输出：
 *     s：偶数轮 chi 后的更新状态
 */
static inline void chi_e(S &s) {
  chi(s.A0, s.B0, s.C0);
  chi(s.A1, s.B1, s.C1);
}

/*
 * Applies chi on the odd-round branch schedule.
 * Inputs:
 *     s: state to transform
 * Outputs:
 *     s: updated state after odd-round chi
 *
 * 按奇数轮分支调度执行 chi。
 * 输入：
 *     s：待变换状态
 * 输出：
 *     s：奇数轮 chi 后的更新状态
 */
static inline void chi_o(S &s) {
  chi(s.A0, s.B1, s.C1);
  chi(s.A1, s.B0, s.C0);
}

/*
 * Executes one permutation round with the linear layer, round constant, and chi step.
 * Inputs:
 *     s: state for the round
 *     r: round index
 * Outputs:
 *     s: updated state after one round
 *
 * 执行一轮置换，包括线性层、轮常量和 chi 步骤。
 * 输入：
 *     s：本轮状态
 *     r：轮索引
 * 输出：
 *     s：一轮后的更新状态
 */
static inline void rp(S &s, int r) {
  L_M39(s);
  s.A0 = X(s.A0, ARC[r]);
  if (r % 2 == 0) chi_e(s); else chi_o(s);
}

/*
 * Declares the 15-round and 20-round permutation drivers.
 * Inputs:
 *     s: state to permute
 * Outputs:
 *     s: updated state after the requested number of rounds
 *
 * 声明 15 轮和 20 轮置换驱动函数。
 * 输入：
 *     s：待置换状态
 * 输出：
 *     s：按请求轮数更新后的状态
 */
void P_15(S &s);
void P_20(S &s);

#endif