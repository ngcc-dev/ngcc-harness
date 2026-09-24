/* C-Hash (C-Hash-512 / C-Hash-1024) 的 AVX2 优化实现 v2 (纯 C 版本)
 *
 * 编译:
 *   gcc   -O3 -mavx2 -march=native -std=c11 CryptHash_AlgorithmInstance.c -o c_hash_v2_bench
 *   clang -O3 -mavx2 -march=native -std=c11 CryptHash_AlgorithmInstance.c -o c_hash_v2_bench
 */
#include "CryptHash_AlgorithmInstance.h"
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
  #define ALIGN32 __attribute__((aligned(32)))
  #define FINL __attribute__((always_inline)) static inline
  #define RESTRICT __restrict__
#else
  #define ALIGN32 __declspec(align(32))
  #define FINL __forceinline static
  #define RESTRICT __restrict
#endif

/* S 盒表 */
static const uint8_t SBOX_TABLE[16] = {
    0x1, 0x4, 0x0, 0xc, 0x3, 0x2, 0x5, 0xb,
    0xa, 0x8, 0x6, 0xf, 0x7, 0x9, 0xd, 0xe
};

static const uint8_t COL_CONST_NIB[6] = { 0xC, 0x9, 0x0, 0x8, 0xB, 0x2 };

/* AVX2 常量（在 ensure_init 中进行运行时初始化） */
static __m256i g_SBOX_AVX;
static __m256i g_CON0F;
static __m256i g_FUSED_SBOX[6];
static __m256i g_RC256[28];

static const uint32_t RC_U32_TABLE[28] = {
    0x243F6A88u, 0x85A308D3u, 0x13198A2Eu, 0x03707344u,
    0xA4093822u, 0x299F31D0u, 0x082EFA98u, 0xEC4E6C89u,
    0x452821E6u, 0x38D01377u, 0xBE5466CFu, 0x34E90C6Cu,
    0xC0AC29B7u, 0xC97C50DDu, 0x3F84D5B5u, 0xB5470917u,
    0x9216D5D9u, 0x8979FB1Bu, 0xD1310BA6u, 0x98DFB5ACu,
    0x2FFD72DBu, 0xD01ADFB7u, 0xB8E1AFEDu, 0x6A267E96u,
    0xBA7C9045u, 0xF12C7F99u, 0x24A19947u, 0xB3916CF7u
};

/**
 * @brief 由一个 32-bit 轮常数 x 构造 __m256i（高 8 个字节填 nibble, 其余填 0）。
 */
static __m256i rc_from_u32(uint32_t x) {
    return _mm256_setr_epi8(
        (char)((x >> 28) & 0xF), (char)((x >> 24) & 0xF),
        (char)((x >> 20) & 0xF), (char)((x >> 16) & 0xF),
        (char)((x >> 12) & 0xF), (char)((x >>  8) & 0xF),
        (char)((x >>  4) & 0xF), (char)((x      ) & 0xF),
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0);
}

/* 用于 nib32_to_bytes16 的 shuffle 控制向量（函数内初始化） */
static __m128i g_NIB_EVEN;
static __m128i g_NIB_ODD;

/**
 * @brief 运行时初始化 AVX2 全局常量、融合 S 盒表与轮常数表。
 */
static void init_fused_sbox(void) {
    int col, x, r;

    g_SBOX_AVX = _mm256_setr_epi8(
        0x1, 0x4, 0x0, 0xc, 0x3, 0x2, 0x5, 0xb, 0xa, 0x8, 0x6, 0xf, 0x7, 0x9, 0xd, 0xe,
        0x1, 0x4, 0x0, 0xc, 0x3, 0x2, 0x5, 0xb, 0xa, 0x8, 0x6, 0xf, 0x7, 0x9, 0xd, 0xe);

    g_CON0F = _mm256_set1_epi8(0x0f);

    g_NIB_EVEN = _mm_setr_epi8(
        0, 2, 4, 6, 8, 10, 12, 14,
        (char)0x80, (char)0x80, (char)0x80, (char)0x80,
        (char)0x80, (char)0x80, (char)0x80, (char)0x80);
    g_NIB_ODD = _mm_setr_epi8(
        1, 3, 5, 7, 9, 11, 13, 15,
        (char)0x80, (char)0x80, (char)0x80, (char)0x80,
        (char)0x80, (char)0x80, (char)0x80, (char)0x80);

    for (col = 0; col < 6; ++col) {
        uint8_t c = COL_CONST_NIB[col];
        ALIGN32 uint8_t tbl[32];
        for (x = 0; x < 16; ++x) {
            tbl[x]      = SBOX_TABLE[x ^ c];
            tbl[16 + x] = SBOX_TABLE[x ^ c];
        }
        g_FUSED_SBOX[col] = _mm256_load_si256((const __m256i*)tbl);
    }

    for (r = 0; r < 28; ++r) {
        g_RC256[r] = rc_from_u32(RC_U32_TABLE[r]);
    }
}

/**
 * @brief 将 32 字节数据展开为 64 个 4-bit nibble, 分别存入两个 __m256i。
 */
FINL void expand32_to_nibbles_2ymm(const uint8_t* RESTRICT src,
                                   __m256i* out_lo, __m256i* out_hi) {
    __m256i x  = _mm256_loadu_si256((const __m256i*)src);
    __m256i lo = _mm256_and_si256(x, g_CON0F);
    __m256i hi = _mm256_and_si256(_mm256_srli_epi16(x, 4), g_CON0F);

    __m256i t0 = _mm256_unpacklo_epi8(hi, lo);
    __m256i t1 = _mm256_unpackhi_epi8(hi, lo);
    *out_lo = _mm256_permute2x128_si256(t0, t1, 0x20);
    *out_hi = _mm256_permute2x128_si256(t0, t1, 0x31);
}

/**
 * @brief 将 32 个 nibble 打包回 16 字节数据写到 dst16。
 */
FINL void nib32_to_bytes16(uint8_t* RESTRICT dst16, __m256i v32nib) {
    __m128i a = _mm256_castsi256_si128(v32nib);
    __m128i b = _mm256_extracti128_si256(v32nib, 1);

    __m128i ae = _mm_shuffle_epi8(a, g_NIB_EVEN);
    __m128i ao = _mm_shuffle_epi8(a, g_NIB_ODD);
    __m128i ap;
    __m128i be, bo, bp;

    ae = _mm_slli_epi16(ae, 4);
    ap = _mm_or_si128(ae, ao);

    be = _mm_shuffle_epi8(b, g_NIB_EVEN);
    bo = _mm_shuffle_epi8(b, g_NIB_ODD);
    be = _mm_slli_epi16(be, 4);
    bp = _mm_or_si128(be, bo);

    _mm_storel_epi64((__m128i*)(dst16 + 0), ap);
    _mm_storel_epi64((__m128i*)(dst16 + 8), bp);
}

/**
 * @brief 将 (lo, hi) 两个 __m256i（共 64 个 nibble）打包回 32 字节。
 */
FINL void dump_vec32bytes(uint8_t* RESTRICT dst32, __m256i lo, __m256i hi) {
    nib32_to_bytes16(dst32 + 0, lo);
    nib32_to_bytes16(dst32 + 16, hi);
}

/* 四种字节级循环移位（以 128 位车道为单位，再靠 alignr 拼接） */
FINL __m256i rotate_left_15_s(__m256i v, __m256i swap) { return _mm256_alignr_epi8(swap, v, 15); }
FINL __m256i rotate_left_7_s (__m256i v, __m256i swap) { return _mm256_alignr_epi8(swap, v, 7);  }
FINL __m256i rotate_left_23_s(__m256i v, __m256i swap) { return _mm256_alignr_epi8(v, swap, 7);  }
FINL __m256i rotate_right_1_s(__m256i v, __m256i swap) { return _mm256_alignr_epi8(v, swap, 15); }

#define XO2(dest, src) do { \
    state[(dest)*2+0] = _mm256_xor_si256(state[(dest)*2+0], state[(src)*2+0]); \
    state[(dest)*2+1] = _mm256_xor_si256(state[(dest)*2+1], state[(src)*2+1]); \
} while(0)

/**
 * @brief BigU-1536 单轮置换（AVX2 实现）：S 盒、行混合、列常量融合 S 盒、混合旋转、轮常数注入。
 */
FINL void BigU_1536_Round_opt(__m256i state[12], int r) {
    int i;
    /* 第一次 S 盒替换 */
    for (i = 0; i < 12; ++i) {
        state[i] = _mm256_shuffle_epi8(g_SBOX_AVX, state[i]);
    }
    /* 行混合矩阵 */
    XO2(3, 2); XO2(4, 0); XO2(5, 1);
    XO2(0, 3); XO2(1, 4); XO2(2, 5);
    XO2(3, 1); XO2(4, 2); XO2(5, 0);
    /* 融合列常量 + 第二次 S 盒（每行一个融合表） */
    for (i = 0; i < 6; ++i) {
        __m256i tbl = g_FUSED_SBOX[i];
        state[2*i]   = _mm256_shuffle_epi8(tbl, state[2*i]);
        state[2*i+1] = _mm256_shuffle_epi8(tbl, state[2*i+1]);
    }
    /* 每行 4 次 L/R 混合旋转 */
    for (i = 0; i < 6; ++i) {
        __m256i L = state[2*i];
        __m256i R = state[2*i+1];
        __m256i T1, swapT;

        T1    = _mm256_xor_si256(L, R);
        swapT = _mm256_permute2x128_si256(T1, T1, 0x01);
        T1    = rotate_left_15_s(T1, swapT);
        L     = _mm256_xor_si256(L, T1);
        {
            __m256i rt = _mm256_xor_si256(R, T1);
            __m256i sw = _mm256_permute2x128_si256(rt, rt, 0x01);
            R = rotate_right_1_s(rt, sw);
        }

        T1    = _mm256_xor_si256(L, R);
        swapT = _mm256_permute2x128_si256(T1, T1, 0x01);
        T1    = rotate_left_7_s(T1, swapT);
        L     = _mm256_xor_si256(L, T1);
        {
            __m256i rt = _mm256_xor_si256(R, T1);
            __m256i sw = _mm256_permute2x128_si256(rt, rt, 0x01);
            R = rotate_right_1_s(rt, sw);
        }

        T1    = _mm256_xor_si256(L, R);
        swapT = _mm256_permute2x128_si256(T1, T1, 0x01);
        T1    = rotate_left_23_s(T1, swapT);
        L     = _mm256_xor_si256(L, T1);
        {
            __m256i rt = _mm256_xor_si256(R, T1);
            __m256i sw = _mm256_permute2x128_si256(rt, rt, 0x01);
            R = rotate_right_1_s(rt, sw);
        }

        T1    = _mm256_xor_si256(L, R);
        T1    = _mm256_permute2x128_si256(T1, T1, 0x01);
        L     = _mm256_xor_si256(L, T1);
        {
            __m256i rt = _mm256_xor_si256(R, T1);
            __m256i sw = _mm256_permute2x128_si256(rt, rt, 0x01);
            R = rotate_right_1_s(rt, sw);
        }
        state[2*i]   = L;
        state[2*i+1] = R;
    }
    /* 轮常数注入到第 0 向量 */
    state[0] = _mm256_xor_si256(state[0], g_RC256[r]);
}

/**
 * @brief 对 __m256i state[12] 连续执行 28 轮置换。
 */
FINL void C_Engine1536_Perm28_ymm(__m256i state[12]) {
    int r;
    for (r = 0; r < 28; ++r) {
        BigU_1536_Round_opt(state, r);
    }
}

/**
 * @brief 以 192 字节输入/输出的方式调用 AVX2 置换（字节 <-> nibble 双向展开）。
 */
static void C_Engine1536_Perm28_bytes(uint8_t state_bytes[192]) {
    __m256i state[12];
    int i;
    for (i = 0; i < 6; ++i) {
        expand32_to_nibbles_2ymm(state_bytes + i * 32, &state[i*2], &state[i*2+1]);
    }
    C_Engine1536_Perm28_ymm(state);
    for (i = 0; i < 6; ++i) {
        dump_vec32bytes(state_bytes + i * 32, state[i*2], state[i*2+1]);
    }
}

/* C-Hash 常量 */
#define CH_B_BITS       ((size_t)1536)
#define CH_W_BITS       ((size_t)64)
#define CH_N_BITS       ((size_t)1472)
#define CH_HALF_N_BITS  ((size_t)736)

#define CH_B_BYTES      (CH_B_BITS / 8)
#define CH_N_BYTES      (CH_N_BITS / 8)
#define CH_HALF_N_BYTES (CH_HALF_N_BITS / 8)
#define CH_W_BYTES      (CH_W_BITS / 8)


typedef enum CHashMode {
    CH_CTR_PERM = 0,
    CH_CTR_FUNC = 1
} CHashMode;

typedef struct CHashVariant {
    int       hashlen_bits;
    uint8_t   iv[CH_HALF_N_BYTES];
    size_t    h_bits;
    CHashMode mode;
    uint8_t   lane_inject;
    uint8_t   lane_gen;
    uint8_t   lane_ldrh;
} CHashVariant;

/**
 * @brief 构建 IV：后两个字节写入 hashlen_bits 的大端表示。
 */
static void build_IV(int hashlen_bits, uint8_t out[CH_HALF_N_BYTES]) {
    memset(out, 0, CH_HALF_N_BYTES);
    out[CH_HALF_N_BYTES - 2] = (uint8_t)((hashlen_bits >> 8) & 0xFF);
    out[CH_HALF_N_BYTES - 1] = (uint8_t)( hashlen_bits       & 0xFF);
}

/**
 * @brief 根据摘要长度选择 C-Hash 变体参数（512 或 1024）。
 */
static int make_variant(int hashlen_bits, CHashVariant* v) {
    if (hashlen_bits == 512) {
        v->hashlen_bits = 512;  v->h_bits = 736;  v->mode = CH_CTR_PERM;
        v->lane_inject = 0x01;  v->lane_gen = 0x02;  v->lane_ldrh = 0x03;
    } else if (hashlen_bits == 1024) {
        v->hashlen_bits = 1024; v->h_bits = 1472; v->mode = CH_CTR_FUNC;
        v->lane_inject = 0x81;  v->lane_gen = 0x82;  v->lane_ldrh = 0x83;
    } else {
        return 0;
    }
    build_IV(hashlen_bits, v->iv);
    return 1;
}


/**
 * @brief 将 32 字节原始数据展开为 2×__m256i nibble 表示（与 expand32 同义封装）。
 */
FINL void bytes32_to_nibymm(const uint8_t* src32, __m256i* lo, __m256i* hi) {
    expand32_to_nibbles_2ymm(src32, lo, hi);
}


/**
 * @brief 将 1472 位 (184 字节) 的 x_n 展开到 12 个 __m256i 的状态向量。
 *        最后一段只有 24 字节有效, 剩余 8 字节填 0。
 */
FINL void expand_1472bits_to_state(const uint8_t x_n[CH_N_BYTES], __m256i state[12]) {
    int i;
    ALIGN32 uint8_t tmp[32];
    for (i = 0; i < 5; ++i) {
        expand32_to_nibbles_2ymm(x_n + i * 32, &state[i*2], &state[i*2+1]);
    }
    memcpy(tmp, x_n + 160, 24);
    memset(tmp + 24, 0, 8);
    expand32_to_nibbles_2ymm(tmp, &state[10], &state[11]);
}

/**
 * @brief 将 8 字节计数器以 nibble 形式注入状态的最后一个向量 state[11]。
 */
FINL void inject_ctr_into_state(const uint8_t ctr_w[CH_W_BYTES], __m256i state[12]) {
    ALIGN32 uint8_t tmp[32];
    __m256i lo, hi;
    memset(tmp, 0, 16);
    memset(tmp + 16, 0, 8);
    memcpy(tmp + 24, ctr_w, 8);

    expand32_to_nibbles_2ymm(tmp, &lo, &hi);
    state[11] = _mm256_xor_si256(state[11], hi);
    (void)lo;
}


/**
 * @brief 将 12 个 __m256i 状态回写为 184 字节的 x_n 缓冲区。
 */
FINL void state_to_1472bytes(const __m256i state[12], uint8_t out_n[CH_N_BYTES]) {
    int i;
    ALIGN32 uint8_t tmp[32];
    for (i = 0; i < 5; ++i) {
        dump_vec32bytes(out_n + i * 32, state[i*2], state[i*2+1]);
    }
    dump_vec32bytes(tmp, state[10], state[11]);
    memcpy(out_n + 160, tmp, 24);
}



/**
 * @brief 将 laneID 与 56 位计数器合成为 8 字节大端 CTR。
 */
FINL void make_CTR_bytes(uint8_t laneID, uint64_t i, uint8_t out[8]) {
    uint64_t ctr = ((uint64_t)laneID << 56) | (i & ((1ULL << 56) - 1));
    uint64_t be;
#if defined(__GNUC__) || defined(__clang__)
    be = __builtin_bswap64(ctr);
#else
    be = _byteswap_uint64(ctr);
#endif
    memcpy(out, &be, 8);
}

/**
 * @brief 使用 AVX2/SSE 批量按 32/16/8/4/1 字节块就地异或 src 到 dst。
 */
FINL void xor_inplace(uint8_t* RESTRICT dst, const uint8_t* RESTRICT src, size_t n) {
    size_t k = 0;
    for (; k + 32 <= n; k += 32) {
        __m256i a = _mm256_loadu_si256((const __m256i*)(dst + k));
        __m256i b = _mm256_loadu_si256((const __m256i*)(src + k));
        _mm256_storeu_si256((__m256i*)(dst + k), _mm256_xor_si256(a, b));
    }
    if (k + 16 <= n) {
        __m128i a = _mm_loadu_si128((const __m128i*)(dst + k));
        __m128i b = _mm_loadu_si128((const __m128i*)(src + k));
        _mm_storeu_si128((__m128i*)(dst + k), _mm_xor_si128(a, b));
        k += 16;
    }
    if (k + 8 <= n) {
        uint64_t a, b;
        memcpy(&a, dst + k, 8);
        memcpy(&b, src + k, 8);
        a ^= b;
        memcpy(dst + k, &a, 8);
        k += 8;
    }
    if (k + 4 <= n) {
        uint32_t a, b;
        memcpy(&a, dst + k, 4);
        memcpy(&b, src + k, 4);
        a ^= b;
        memcpy(dst + k, &a, 4);
        k += 4;
    }
    for (; k < n; ++k) dst[k] ^= src[k];
}

/**
 * @brief 根据模式应用 P 函数：注入 CTR -> 置换 -> 清空尾部 -> （CTR_FUNC 模式再异或原状态）。
 */
FINL void apply_P_ymm(CHashMode mode,
                      __m256i h_state[12],
                      const uint8_t ctr_w[CH_W_BYTES]) {
    __m256i saved[12];
    int i;
    if (mode == CH_CTR_FUNC) {
        for (i = 0; i < 12; ++i) saved[i] = h_state[i];
    }

    inject_ctr_into_state(ctr_w, h_state);
    C_Engine1536_Perm28_ymm(h_state);
    {
        __m128i low = _mm256_castsi256_si128(h_state[11]);
        h_state[11] = _mm256_zextsi128_si256(low);
    }
    if (mode == CH_CTR_FUNC) {
        for (i = 0; i < 12; ++i) {
            h_state[i] = _mm256_xor_si256(h_state[i], saved[i]);
        }
    }
}

/**
 * @brief 对消息进行填充：'1' 比特 + K 个 '0' 比特，使总长是 HALF_N 位的整数倍。
 * @param[out] blocks_out 分配的缓冲区（调用者释放）
 * @return 填充后的块数
 */
static size_t pad_message(const uint8_t* msg, uint64_t msg_len_bits,
                          uint8_t** blocks_out, size_t* bytes_out) {
    uint64_t rem = msg_len_bits % CH_HALF_N_BITS;
    uint64_t K   = (CH_HALF_N_BITS - 1 - rem) % CH_HALF_N_BITS;
    uint64_t total_bits = msg_len_bits + 1 + K;
    size_t   l          = (size_t)(total_bits / CH_HALF_N_BITS);
    size_t   total_bytes = l * CH_HALF_N_BYTES;
    uint8_t* blocks;
    uint64_t full_bytes;
    uint32_t tail_bits;
    uint64_t one_bit_pos;
    size_t   one_bit_byte_idx;
    uint32_t one_bit_in_byte;

    blocks = (uint8_t*)calloc(total_bytes ? total_bytes : 1, 1);
    *blocks_out = blocks;
    *bytes_out = total_bytes;

    full_bytes = msg_len_bits / 8;
    tail_bits  = (uint32_t)(msg_len_bits % 8);
    if (full_bytes) memcpy(blocks, msg, (size_t)full_bytes);
    if (tail_bits > 0) {
        uint8_t mask = (uint8_t)(0xFFu << (8 - tail_bits));
        blocks[(size_t)full_bytes] = (uint8_t)(msg[full_bytes] & mask);
    }

    one_bit_pos      = msg_len_bits;
    one_bit_byte_idx = (size_t)(one_bit_pos / 8);
    one_bit_in_byte  = (uint32_t)(one_bit_pos % 8);
    blocks[one_bit_byte_idx] |= (uint8_t)(0x80u >> one_bit_in_byte);

    return l;
}

/**
 * @brief 处理短消息(< N 位)的 LDRH 模式：对同一 x 用计数器 1、2 各做一次
 *        CTR-Perm，输出相异或后截断为摘要。
 *        x   = M || 1 || 0^(N-1-|M|)
 *        y1  = LMB(N, P(x || [LaneID||1]))
 *        y2  = LMB(N, P(x || [LaneID||2]))
 *        y   = y1 XOR y2
 *        优化点：x 在两次置换中完全相同, 故只展开一次得到公共基态 st0,
 *        随后分别注入两个计数器、各跑一次置换, 复用现有 AVX2 算子。
 */
static size_t C_HASH_LDRH(const CHashVariant* v,
                          const uint8_t* msg, uint64_t msg_len_bits,
                          uint8_t* digest_out) {
    ALIGN32 uint8_t x_1472[CH_N_BYTES];
    ALIGN32 uint8_t ctr1[CH_W_BYTES], ctr2[CH_W_BYTES];
    ALIGN32 uint8_t y1_1472[CH_N_BYTES], y2_1472[CH_N_BYTES];
    ALIGN32 uint8_t tmp[32];
    __m256i st0[12];   /* 公共基础状态: x 展开为 nibble, state[11] 高半区(计数器)清零 */
    __m256i st[12];
    uint64_t full_bytes;
    uint32_t tail_bits;
    size_t hashlen_bytes;
    int i;

    memset(x_1472, 0, CH_N_BYTES);

    full_bytes = msg_len_bits / 8;
    tail_bits  = (uint32_t)(msg_len_bits % 8);
    if (full_bytes > 0) {
        memcpy(x_1472, msg, (size_t)full_bytes);
    }
    if (tail_bits > 0) {
        uint8_t mask = (uint8_t)(0xFFu << (8 - tail_bits));
        x_1472[(size_t)full_bytes] = (uint8_t)(msg[full_bytes] & mask);
    }
    x_1472[(size_t)(msg_len_bits / 8)] |= (uint8_t)(0x80u >> (msg_len_bits % 8));
    /* 公共基础状态：仅展开一次 x；尾部 8 字节(计数器位置)填 0, 待逐个注入 */
    expand_1472bits_to_state(x_1472, st0);

    /* 两个计数器：[LaneID||1] 与 [LaneID||2] */
    make_CTR_bytes(v->lane_ldrh, 1, ctr1);
    make_CTR_bytes(v->lane_ldrh, 2, ctr2);

    /* 第一次 CTR-Perm：y1 = LMB(N, P(x || ctr1)) */
    for (i = 0; i < 12; ++i) st[i] = st0[i];
    inject_ctr_into_state(ctr1, st);
    C_Engine1536_Perm28_ymm(st);
    for (i = 0; i < 5; ++i) {
        dump_vec32bytes(y1_1472 + i * 32, st[i*2], st[i*2+1]);
    }
    dump_vec32bytes(tmp, st[10], st[11]);
    memcpy(y1_1472 + 160, tmp, 24);

    /* 第二次 CTR-Perm：y2 = LMB(N, P(x || ctr2)) */
    for (i = 0; i < 12; ++i) st[i] = st0[i];
    inject_ctr_into_state(ctr2, st);
    C_Engine1536_Perm28_ymm(st);
    for (i = 0; i < 5; ++i) {
        dump_vec32bytes(y2_1472 + i * 32, st[i*2], st[i*2+1]);
    }
    dump_vec32bytes(tmp, st[10], st[11]);
    memcpy(y2_1472 + 160, tmp, 24);

    /* y = y1 XOR y2 */
    xor_inplace(y1_1472, y2_1472, CH_N_BYTES);

    hashlen_bytes = (size_t)(v->hashlen_bits / 8);
    memcpy(digest_out, y1_1472, hashlen_bytes);
    return hashlen_bytes;
}

/* ===================================================================== *
 *  LDXOF —— 短消息可扩展输出模式（AVX2 参考实现，独立函数）
 *
 *  注意：本函数仅作为 C-Hash “短消息可扩展输出模式 (XOF)” 的参考实现，
 *        供需要 XOF 能力的使用者参考调用；它【不】被标准 CryptHash 流程
 *        使用，也不参与 VIL 注入 / FIL 收尾。LDXOF 始终采用 CTR-Perm
 *        形式——即便是 C-Hash-1024 也不使用 CTR-Func、不做 XOR x。
 *
 *  仅处理短消息：要求 |M| < 1472 bit（即 < 184 字节）。
 *  LaneID：C-Hash-512 → 0x04，C-Hash-1024 → 0x84。
 *
 *  64 位计数器布局（大端序列化）：
 *      [ LaneID(8) | OutputByteLen(24) | Reserved(8)=0 | Index(24) ]
 *
 *  流程：
 *      x   = M || 1 || 0^(1471-|M|)                  （1472-bit 块）
 *      y0  = LMB(1472, P(x || ctr0))                 （i=0 基线）
 *      ri  = y0 XOR LMB(1472, P(x || ctri))          （i=1..block_count）
 *      R   = r1 || r2 || ... || r_block_count
 *      out = LMB(OutputByteLen*8, R)
 *
 *  优化点：x 在所有置换实例中完全相同，故只用 expand_1472bits_to_state
 *          展开一次公共基态 st0；每个计数器拷贝基态、注入、置换、dump，
 *          全程复用现有 AVX2 算子。
 * ===================================================================== */

/* 前向声明：LDXOF 为独立入口，需自行确保 AVX2 全局常量已初始化 */
static void ensure_init(void);

/**
 * @brief 构建 LDXOF 的 64 位计数器并以 8 字节大端写出。
 * @param lane_id         LDXOF 通道号（0x04 或 0x84）
 * @param output_byte_len 用户请求的输出字节数（24 bit）
 * @param index           当前置换实例编号 i（24 bit）
 * @param out             输出的 8 字节大端计数器
 */
static void make_ldxof_counter(uint8_t lane_id, uint32_t output_byte_len,
                               uint32_t index, uint8_t out[CH_W_BYTES]) {
    uint64_t ctr = ((uint64_t)lane_id << 56)
                 | ((uint64_t)(output_byte_len & 0xFFFFFFu) << 32)
                 | ((uint64_t)0x00u << 24)
                 | ((uint64_t)(index & 0xFFFFFFu));
    uint64_t be;
#if defined(__GNUC__) || defined(__clang__)
    be = __builtin_bswap64(ctr);
#else
    be = _byteswap_uint64(ctr);
#endif
    memcpy(out, &be, 8);
}

/**
 * @brief LDXOF 短消息可扩展输出（AVX2 参考实现，标准 CryptHash 流程不调用）。
 * @param variant         512 或 1024
 * @param msg             短消息指针
 * @param msg_len_bits    消息位长，要求 < 1472
 * @param output_byte_len 期望输出字节数，要求 < 2^24
 * @param out             输出缓冲区（至少 output_byte_len 字节）
 * @return 实际写出的字节数；参数非法返回 (size_t)-1
 */
size_t C_HASH_LDXOF(int variant,
                    const uint8_t* msg, uint64_t msg_len_bits,
                    uint32_t output_byte_len,
                    uint8_t* out) {
    uint8_t  lane_id;
    ALIGN32 uint8_t x_1472[CH_N_BYTES];
    ALIGN32 uint8_t ctr[CH_W_BYTES];
    ALIGN32 uint8_t y0[CH_N_BYTES], yi[CH_N_BYTES];
    __m256i  st0[12];   /* 公共基础状态：x 展开为 nibble，计数器位置待注入 */
    __m256i  st[12];
    uint64_t full_bytes;
    uint32_t tail_bits;
    size_t   block_count, idx, written;
    int      j;

    /* 仅处理短消息，且输出长度需落在 24 位范围内 */
    if (msg_len_bits >= CH_N_BITS)     return (size_t)-1;
    if (output_byte_len >= (1u << 24)) return (size_t)-1;
    if (variant == 512)       lane_id = 0x04;
    else if (variant == 1024) lane_id = 0x84;
    else                      return (size_t)-1;

    /* LDXOF 为独立入口，确保 AVX2 全局常量（S 盒/轮常数等）已初始化 */
    ensure_init();

    /* 短消息 padding：x = M || 1 || 0^(1471-|M|) */
    memset(x_1472, 0, CH_N_BYTES);
    full_bytes = msg_len_bits / 8;
    tail_bits  = (uint32_t)(msg_len_bits % 8);
    if (full_bytes > 0) memcpy(x_1472, msg, (size_t)full_bytes);
    if (tail_bits > 0) {
        uint8_t mask = (uint8_t)(0xFFu << (8 - tail_bits));
        x_1472[(size_t)full_bytes] = (uint8_t)(msg[full_bytes] & mask);
    }
    x_1472[(size_t)(msg_len_bits / 8)] |= (uint8_t)(0x80u >> (msg_len_bits % 8));

    /* 公共基础状态：仅展开一次 x */
    expand_1472bits_to_state(x_1472, st0);

    /* y0：i=0  */      
    make_ldxof_counter(lane_id, output_byte_len, 0, ctr);
    for (j = 0; j < 12; ++j) st[j] = st0[j];
    inject_ctr_into_state(ctr, st);
    C_Engine1536_Perm28_ymm(st);
    state_to_1472bytes(st, y0);

    /* block_count = ceil(OutputByteLen*8 / 1472) = ceil(OutputByteLen / 184) */
    block_count = (size_t)((output_byte_len + CH_N_BYTES - 1) / CH_N_BYTES);

    /* 逐块：ri = y0 XOR LMB(N, P(x||ctri))，拼接后截断为 OutputByteLen */
    written = 0;
    for (idx = 1; idx <= block_count; ++idx) {
        size_t copy_len;
        make_ldxof_counter(lane_id, output_byte_len, (uint32_t)idx, ctr);
        for (j = 0; j < 12; ++j) st[j] = st0[j];
        inject_ctr_into_state(ctr, st);
        C_Engine1536_Perm28_ymm(st);
        state_to_1472bytes(st, yi);
        xor_inplace(yi, y0, CH_N_BYTES);   /* ri = y0 XOR yi */

        copy_len = CH_N_BYTES;
        if (written + copy_len > (size_t)output_byte_len) {
            copy_len = (size_t)output_byte_len - written;
        }
        memcpy(out + written, yi, copy_len);
        written += copy_len;
    }
    return written;
}


/**
 * @brief 把消息块的左半 (92 字节) 异或到状态的前 6 个向量。
 */
FINL void xor_leftHalf_bytes_to_state(const uint8_t bytes92[CH_HALF_N_BYTES],
                                      __m256i h_state[12]) {
    __m256i lo, hi;
    ALIGN32 uint8_t tmp[32];

    expand32_to_nibbles_2ymm(bytes92 + 0, &lo, &hi);
    h_state[0] = _mm256_xor_si256(h_state[0], lo);
    h_state[1] = _mm256_xor_si256(h_state[1], hi);

    expand32_to_nibbles_2ymm(bytes92 + 32, &lo, &hi);
    h_state[2] = _mm256_xor_si256(h_state[2], lo);
    h_state[3] = _mm256_xor_si256(h_state[3], hi);

    memcpy(tmp, bytes92 + 64, 16);
    memcpy(tmp + 16, bytes92 + 80, 12);
    memset(tmp + 28, 0, 4);
    expand32_to_nibbles_2ymm(tmp, &lo, &hi);
    h_state[4] = _mm256_xor_si256(h_state[4], lo);
    h_state[5] = _mm256_xor_si256(h_state[5], hi);
}

/**
 * @brief 把消息块的右半 (92 字节) 异或到状态的后 6 个向量（跨 state[5] 末尾对齐）。
 */
FINL void xor_rightHalf_bytes_to_state(const uint8_t bytes92[CH_HALF_N_BYTES],
                                       __m256i h_state[12]) {
    __m256i lo, hi;
    ALIGN32 uint8_t tmp[32];

    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp + 12, bytes92, 4);
    expand32_to_nibbles_2ymm(tmp, &lo, &hi);
    h_state[5] = _mm256_xor_si256(h_state[5], lo);

    expand32_to_nibbles_2ymm(bytes92 + 4, &lo, &hi);
    h_state[6] = _mm256_xor_si256(h_state[6], lo);
    h_state[7] = _mm256_xor_si256(h_state[7], hi);

    expand32_to_nibbles_2ymm(bytes92 + 36, &lo, &hi);
    h_state[8] = _mm256_xor_si256(h_state[8], lo);
    h_state[9] = _mm256_xor_si256(h_state[9], hi);

    memcpy(tmp, bytes92 + 68, 16);
    memcpy(tmp + 16, bytes92 + 84, 8);
    memset(tmp + 24, 0, 8);
    expand32_to_nibbles_2ymm(tmp, &lo, &hi);
    h_state[10] = _mm256_xor_si256(h_state[10], lo);
    h_state[11] = _mm256_xor_si256(h_state[11], hi);
}

/**
 * @brief C-Hash 主流程（AVX2 版本）：长消息分块压缩 + 生成阶段输出。
 */
static size_t C_HASH(const CHashVariant* v,
                     const uint8_t* msg, uint64_t msg_len_bits,
                     uint8_t* digest_out) {
    ALIGN32 uint8_t x0_1472[CH_N_BYTES];
    ALIGN32 uint8_t ctr[CH_W_BYTES];
    ALIGN32 uint8_t y_final[CH_N_BYTES];
    __m256i st[12];
    uint8_t* blocks = NULL;
    size_t blocks_bytes = 0;
    size_t l, i;
    size_t hashlen_bytes;

    if (msg_len_bits < CH_N_BITS) {
        return C_HASH_LDRH(v, msg, msg_len_bits, digest_out);
    }

    memcpy(x0_1472, v->iv, CH_HALF_N_BYTES);
    memset(x0_1472 + CH_HALF_N_BYTES, 0, CH_HALF_N_BYTES);

    expand_1472bits_to_state(x0_1472, st);

    make_CTR_bytes(v->lane_inject, 0, ctr);
    apply_P_ymm(v->mode, st, ctr);
    xor_rightHalf_bytes_to_state(v->iv, st);

    l = pad_message(msg, msg_len_bits, &blocks, &blocks_bytes);

    for (i = 1; i <= l; ++i) {
        const uint8_t* m_i = blocks + (i - 1) * CH_HALF_N_BYTES;
        xor_leftHalf_bytes_to_state(m_i, st);
        make_CTR_bytes(v->lane_inject, (uint64_t)i, ctr);
        apply_P_ymm(v->mode, st, ctr);
        xor_rightHalf_bytes_to_state(m_i, st);
    }
    make_CTR_bytes(v->lane_gen, 1, ctr);
    apply_P_ymm(v->mode, st, ctr);

    state_to_1472bytes(st, y_final);

    hashlen_bytes = (size_t)(v->hashlen_bits / 8);
    memcpy(digest_out, y_final, hashlen_bytes);

    free(blocks);
    return hashlen_bytes;
}

/**
 * @brief 一次性初始化 AVX2 全局常量；线程安全取决于调用时机。
 */
static void ensure_init(void) {
    static int inited = 0;
    if (!inited) {
        init_fused_sbox();
        inited = 1;
    }
}

/**
 * @brief 对外导出的 CryptHash 接口（AVX2 优化 C 版本）。
 */
int CryptHash(int digest_len_bits,
              const unsigned char* msg,
              unsigned long long msg_len_bits,
              unsigned char* digest) {
    CHashVariant v;
    if (digest == NULL) return -2;
    if (msg == NULL && msg_len_bits > 0) return -3;
    ensure_init();
    if (!make_variant(digest_len_bits, &v)) return -1;
    C_HASH(&v, (const uint8_t*)msg, (uint64_t)msg_len_bits, (uint8_t*)digest);
    return 0;
}
