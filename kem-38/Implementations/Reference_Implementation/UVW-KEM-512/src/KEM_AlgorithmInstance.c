/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include "auxfunc.h"
#include "gf_math.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "uvw_constants.h"


// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// =========================================================
//  宏
// =========================================================


#define MASK_Q_BITS ((1U << UVW_Q_BITS) - 1)

// 生成满秩矩阵的最大重试次数 (防止死循环)
#define KEM_KEYGEN_MAX_ATTEMPTS 100

// 定义一次取随机数的字节数
// 2KB 既能装下足够多的随机数，又不会撑爆物理栈，还能塞进 L1 缓存
#define CHUNK_SIZE 2048

// 哈希函数域分离前缀
#define H1_PREFIX 0x01
#define H2_PREFIX 0x02
#define H3_PREFIX 0x03
#define H4_PREFIX 0x04

// =========================================================
//  内部辅助函数声明 (static)
// =========================================================
int get_random_bytes(unsigned char *out, int out_len_bytes);
static void generate_random_matrix(gf_elem_t *matrix, int size, DRNG_ctx *drng);
static void generate_permutation(uint16_t *p, int n, DRNG_ctx *drng);
static void matrix_mul(gf_elem_t *C, const gf_elem_t *A, const gf_elem_t *B, int m, int k, int n);
static void generate_nonzero_random_array(gf_elem_t *array, int size, DRNG_ctx *drng);
static uint16_t get_rand_range(uint16_t limit);
static uint16_t get_rand_range_batched(uint16_t limit, uint8_t *buf, int *pos, DRNG_ctx *drng);
static int compute_rank(gf_elem_t *matrix_copy, int rows, int cols);
static int generate_full_rank_matrix(gf_elem_t *matrix, int rows, int cols, DRNG_ctx *drng);
static void generate_distinct_evaluation_points(gf_elem_t *alpha, int count);
static void generate_rs_matrix(gf_elem_t *G, int rows, int cols);
static int systematic_gaussian_elimination(gf_elem_t *G, int rows, int cols);

static void sample_error_vector(gf_elem_t *e, int n, int w, DRNG_ctx *drng);
void uvw_pke_enc(const unsigned char *pk_bytes, const unsigned char *m, ciphertext_t *ct_struct);
int uvw_pke_dec(const unsigned char *sk_bytes, ciphertext_t *ct, unsigned char *output_m);

static void uvw_pke_enc_deterministic(const unsigned char *pk_bytes, const unsigned char *m, const gf_elem_t *r, const gf_elem_t *e, ciphertext_t *ct_struct);

static int gf_mat_inverse(gf_elem_t *inv, const gf_elem_t *A, int k);
extern int uvw_rs_list_decode(const gf_elem_t *receive_codeword, const gf_elem_t *support_set, gf_elem_t *de_codeword);

static void uvw_pack_pk(unsigned char *pk_bytes, const public_key_t *pk_struct);
static void uvw_unpack_pk(public_key_t *pk_struct, const unsigned char *pk_bytes);

static void uvw_pack_ct(unsigned char *ct_bytes, const kem_ciphertext_t *ct_struct);
static void uvw_unpack_ct(kem_ciphertext_t *ct_struct, const unsigned char *ct_bytes);

static void compress_gf_array(unsigned char *out, const gf_elem_t *array, int total_elements);
static void decompress_gf_array(gf_elem_t *array, const unsigned char *in, int total_elements);

static int get_random_bytes_from(DRNG_ctx *drng, unsigned char *out, int out_len_bytes);
static int expand_GU_KeyGen(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i1,
                        private_key_t *sk_struct);

static int expand_GW_KeyGen(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i2,
                        private_key_t *sk_struct);

static void expand_GU_Dec(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i1,
                        private_key_t *sk_struct);

static void expand_GW_Dec(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i2,
                        private_key_t *sk_struct);

static void expand_D(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i3,
                    private_key_t *sk_struct);

static int kem_hash(unsigned char prefix,
                    const unsigned char *msg1, unsigned long long msg1_bits,
                    const unsigned char *msg2, unsigned long long msg2_bits,
                    const unsigned char *msg3, unsigned long long msg3_bits,
                    unsigned long long out_bits,
                    unsigned char *out);


static int derive_pk_from_seeds(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i1, uint8_t i2, uint8_t i3, gf_elem_t *T);

unsigned long long kem_get_pk_len_bytes()
{
    unsigned long long k = UVW_K;
    unsigned long long t_cols = UVW_N - UVW_K;

    // T 矩阵的元素总数
    unsigned long long total_elements = k * t_cols;

    // 总比特数
    unsigned long long total_bits = total_elements * UVW_Q_BITS;

    // 物理字节数：向上取整除以 8
    unsigned long long total_bytes = (total_bits + 7) / 8;

    return total_bytes;
}

unsigned long long kem_get_sk_len_bytes()
{
    return PRIVATE_KEY_SEED_BYTES + 3;
}

unsigned long long kem_get_ss_len_bytes()
{
    return 64; // 512 bits = 64 bytes
}

unsigned long long kem_get_ct_len_bytes()
{
    unsigned long long ct_c1_compressed_bytes = (((UVW_N)*UVW_Q_BITS + 7) / 8);
    return ct_c1_compressed_bytes + (UVW_M / 8) * 2;
}

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    public_key_t *temp_pk = (public_key_t *)malloc(sizeof(public_key_t));
    private_key_t *temp_sk = (private_key_t *)malloc(sizeof(private_key_t));
    if (!temp_sk || !temp_pk)
        return -1;

    int n = UVW_N;
    int k = UVW_K;
    int k1 = UVW_K1;
    int k2 = UVW_K2;
    int half_n = n / 2;

    unsigned char seed[PRIVATE_KEY_SEED_BYTES];

    //  从全局 drng_algorithm 获取随机 seed
    get_random_bytes_from(&drng_algorithm, seed, PRIVATE_KEY_SEED_BYTES);

    //  由 seed 展开 G_U, G_W
    uint8_t i1 = 1;
    while (expand_GU_KeyGen(seed, i1, temp_sk) != 0)
    {
        i1++;
    }

    uint8_t i2 = 1;
    while (expand_GW_KeyGen(seed, i2, temp_sk) != 0)
    {
        i2++;
    }

    // generate_rs_matrix(sk_struct->G_RS, k2, half_n);
    // 由于 G_RS 是固定的，直接从预定义的常量中复制
    memcpy(temp_sk->G_RS, FIXED_G_RS, k2 * half_n * sizeof(gf_elem_t));

    // --------------------------------------------------------
    // 拼装中心矩阵 (大小 k * n)
    // --------------------------------------------------------

    // 中心矩阵
    gf_elem_t *G_tmp = (gf_elem_t *)malloc(k * n * sizeof(gf_elem_t));
    if (!G_tmp)
        return -1;

    // 提前算好半行的字节数，memcpy 要用到
    size_t half_row_bytes = half_n * sizeof(gf_elem_t);

    // --------------------------------------------------------
    // 填充上半区 (r < k1): 格式为 [ G_U | G_U ]
    // --------------------------------------------------------
    for (int r = 0; r < k1; r++)
    {
        // 定位到目标矩阵的第 r 行开头
        gf_elem_t *dest_row = G_tmp + (r * n);
        // 定位到源矩阵 G_U 的第 r 行开头
        gf_elem_t *src_u_row = temp_sk->G_U + (r * half_n);

        // 左半区：直接拷贝 G_U 的一行
        memcpy(dest_row, src_u_row, half_row_bytes);
        // 右半区：往后偏移 half_n 的位置，再拷贝一次 G_U 的一行
        memcpy(dest_row + half_n, src_u_row, half_row_bytes);
    }

    // --------------------------------------------------------
    // 填充下半区 (r >= k1): 格式为 [ G_V | G_W ]，其中 G_V = G_RS + G_W
    // --------------------------------------------------------
    for (int r = 0; r < k2; r++)
    {
        // 定位到目标矩阵的实际物理行 (k1 + r)
        gf_elem_t *dest_row = G_tmp + ((k1 + r) * n);

        gf_elem_t *src_rs_row = temp_sk->G_RS + (r * half_n);
        gf_elem_t *src_w_row = temp_sk->G_W + (r * half_n);

        // 左半区 [ G_V ]：G_V 是 G_RS 和 G_W 的逐元素相加
        for (int c = 0; c < half_n; c++)
        {
            dest_row[c] = gf_add(src_rs_row[c], src_w_row[c]);
        }

        // 右半区 [ G_W ]：直接拷贝 G_W 的一行
        memcpy(dest_row + half_n, src_w_row, half_row_bytes);
    }

    // --------------------------------------------------------
    // 进入概率循环，寻找能化为系统形式的置换矩阵 D
    // --------------------------------------------------------
    gf_elem_t *G_sk = (gf_elem_t *)malloc(k * n * sizeof(gf_elem_t));
    if (!G_sk)
    {
        free(G_tmp);
        free(temp_sk);
        free(temp_pk);
        return -1;
    }

    uint8_t i3 = 0;
    while (1)
    {
        i3++;
        expand_D(seed, i3, temp_sk);

        // 计算 G_sk = G_tmp * D
        // G_sk 的第 c 列 = (G_tmp 的第 D_perm[c] 列) * D_coeffs[c]
        for (int r = 0; r < k; r++)
        {
            for (int c = 0; c < n; c++)
            {
                int old_col_idx = temp_sk->D_perm[c];
                G_sk[r * n + c] = gf_mul(G_tmp[r * n + old_col_idx], temp_sk->D_coeffs[c]);
            }
        }

        // 截取高斯消元前的左侧 K x K 块 A 存入私钥
        // 这块矩阵就是解密时原本需要求逆的那个 S^-1，直接保存，后续解密不用求逆
        for (int r = 0; r < k; r++)
            memcpy(temp_sk->A + (r * k), G_sk + (r * n), k * sizeof(gf_elem_t));

        // 尝试高斯消元
        if (systematic_gaussian_elimination(G_sk, k, n) == 0)
        {
            break; // 化简为 [I_k | T]，跳出循环
        }
    }

    free(G_tmp);

    // --------------------------------------------------------
    // 将化简后的右半部分 T 存入公钥结构体
    // --------------------------------------------------------
    int t_cols = n - k;
    size_t row_bytes = t_cols * sizeof(gf_elem_t);

    for (int r = 0; r < k; r++)
    {
        memcpy(temp_pk->T + r * t_cols, G_sk + r * n + k, row_bytes);
    }

    free(G_sk);

    // =========================================================
    // 压缩公钥，将 temp_pk 挤压写入外部的 pk 字节数组中
    // =========================================================
    uvw_pack_pk(pk, temp_pk);

    memcpy(sk, seed, PRIVATE_KEY_SEED_BYTES);
    memcpy(sk + PRIVATE_KEY_SEED_BYTES, &i1, 1);
    memcpy(sk + PRIVATE_KEY_SEED_BYTES + 1, &i2, 1);
    memcpy(sk + PRIVATE_KEY_SEED_BYTES + 2, &i3, 1);

    // 设置返回长度
    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();

    free(temp_sk);
    free(temp_pk);

    return 0; // Success
}

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    int k = UVW_K, n = UVW_N, w = UVW_W;

    // 随机选择 m
    unsigned char m[UVW_M / 8];
    get_random_bytes_from(&drng_algorithm, m, sizeof(m));

    // H1(m) → (r, e)：先用 H1 生成 256 比特种子，然后通过 DRNG 扩展
    unsigned char seed_h1[32]; // 256 比特
    if (kem_hash(H1_PREFIX, m, UVW_M, NULL, 0, NULL, 0, 256, seed_h1) != 0)
        return -1;

    DRNG_ctx drng_h1;
    init_random_number(&drng_h1, seed_h1, 32);
    gf_elem_t r[UVW_K];
    generate_random_matrix(r, k, &drng_h1);
    gf_elem_t e[UVW_N];
    sample_error_vector(e, n, w, &drng_h1);

    // 确定性地加密得到 c1
    kem_ciphertext_t *kct = (kem_ciphertext_t *)malloc(sizeof(kem_ciphertext_t));
    if (!kct)
        return -1;
    ciphertext_t pke_ct;
    uvw_pke_enc_deterministic(pk, m, r, e, &pke_ct);
    memcpy(kct->c1, pke_ct.c1, sizeof(pke_ct.c1));

    // H3(r, e) → mask，计算 c2 = m ⊕ mask
    unsigned char mask[UVW_M / 8];
    if (kem_hash(H3_PREFIX,
                 (unsigned char *)r, k * sizeof(gf_elem_t) * 8,
                 (unsigned char *)e, n * sizeof(gf_elem_t) * 8,
                 NULL, 0,
                 UVW_M, mask) != 0)
        return -1;
    for (int i = 0; i < UVW_M / 8; i++)
        kct->c2[i] = m[i] ^ mask[i];

    // H2(m) → d
    if (kem_hash(H2_PREFIX, m, UVW_M, NULL, 0, NULL, 0, UVW_M, kct->d) != 0)
        return -1;

    // H4(m, c1, c2) → 会话密钥 K (512 比特)
    if (kem_hash(H4_PREFIX,
                 m, UVW_M,
                 (unsigned char *)kct->c1, sizeof(kct->c1) * 8,
                 (unsigned char *)kct->c2, sizeof(kct->c2) * 8,
                 512, ss) != 0)
        return -1;

    // 最后将 kct 打包输出到 ct
    uvw_pack_ct(ct, kct);
    free(kct);

    *ss_len_bytes = kem_get_ss_len_bytes();
    *ct_len_bytes = kem_get_ct_len_bytes();
    return 0;
}

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    // 检查输入长度
    if (sk_len_bytes != kem_get_sk_len_bytes() ||
        ct_len_bytes != kem_get_ct_len_bytes())
        return -1;

    int k = UVW_K, n = UVW_N, w = UVW_W, t_cols = n - k;

    // 解包密文
    kem_ciphertext_t *kct = (kem_ciphertext_t *)malloc(sizeof(kem_ciphertext_t));
    if (!kct)
        return -1;
    uvw_unpack_ct(kct, ct);

    // 构造 PKE 密文结构体
    ciphertext_t pke_ct;
    memcpy(pke_ct.c1, kct->c1, sizeof(pke_ct.c1));
    memcpy(pke_ct.c2, kct->c2, sizeof(pke_ct.c2));

    // 调用 PKE 解密得到 m'
    unsigned char m_prime[UVW_M / 8];
    int ret = uvw_pke_dec(sk, &pke_ct, m_prime);
    if (ret != 0)
        return -2; // 解密失败

    // 从种子重建公钥 T（用于重加密验证）
    gf_elem_t *T = (gf_elem_t *)malloc(k * t_cols * sizeof(gf_elem_t));
    if (!T)
        return -3;
    unsigned char seed[PRIVATE_KEY_SEED_BYTES];
    memcpy(seed, sk, PRIVATE_KEY_SEED_BYTES);
    uint8_t i1 = sk[PRIVATE_KEY_SEED_BYTES];
    uint8_t i2 = sk[PRIVATE_KEY_SEED_BYTES + 1];
    uint8_t i3 = sk[PRIVATE_KEY_SEED_BYTES + 2];
    if (derive_pk_from_seeds(seed, i1, i2, i3, T) != 0)
    {
        free(T);
        return -4;
    }

    // 计算 r', e' = H1(m')
    unsigned char seed_h1_prime[32]; // 256 比特种子
    if (kem_hash(H1_PREFIX, m_prime, UVW_M, NULL, 0, NULL, 0, 256, seed_h1_prime) != 0)
    {
        free(T);
        return -5;
    }

    DRNG_ctx drng_h1_prime;
    init_random_number(&drng_h1_prime, seed_h1_prime, sizeof(seed_h1_prime));
    gf_elem_t r_prime[UVW_K];
    generate_random_matrix(r_prime, k, &drng_h1_prime);
    gf_elem_t e_prime[UVW_N];
    sample_error_vector(e_prime, n, UVW_W, &drng_h1_prime);

    // 计算 d' = H2(m')
    unsigned char d_prime[UVW_M / 8];
    if (kem_hash(H2_PREFIX, m_prime, UVW_M, NULL, 0, NULL, 0, UVW_M, d_prime) != 0)
    {
        free(T);
        return -5;
    }

    // 验证 d
    if (memcmp(kct->d, d_prime, sizeof(d_prime)) != 0)
    {
        free(T);
        return -1; // 验证失败
    }

    // 重加密得到 c1' 和 c2'
    // 用临时公钥字节串打包 T，然后进行确定性加密
    unsigned char *pk_bytes = (unsigned char *)malloc(kem_get_pk_len_bytes());
    if (!pk_bytes)
    {
        free(T);
        return -3;
    }

    public_key_t *temp_pk_struct = (public_key_t *)malloc(sizeof(public_key_t));
    if (!temp_pk_struct)
    {
        free(pk_bytes);
        free(T);
        free(kct);
        return -3;
    }
    memcpy(temp_pk_struct->T, T, k * t_cols * sizeof(gf_elem_t));
    uvw_pack_pk(pk_bytes, temp_pk_struct);
    free(temp_pk_struct);

    ciphertext_t pke_ct_prime;
    uvw_pke_enc_deterministic(pk_bytes, m_prime, r_prime, e_prime, &pke_ct_prime);
    free(pk_bytes);

    // 计算 c2' = m' XOR H3(r', e')
    unsigned char mask_prime[UVW_M / 8];
    if (kem_hash(H3_PREFIX,
                 (unsigned char *)r_prime, k * sizeof(gf_elem_t) * 8,
                 (unsigned char *)e_prime, n * sizeof(gf_elem_t) * 8,
                 NULL, 0,
                 UVW_M, mask_prime) != 0)
    {
        free(T);
        return -5;
    }
    unsigned char c2_prime[UVW_M / 8];
    for (int i = 0; i < UVW_M / 8; i++)
        c2_prime[i] = m_prime[i] ^ mask_prime[i];

    // 比较密文
    if (memcmp(pke_ct_prime.c1, kct->c1, sizeof(pke_ct_prime.c1)) != 0 ||
        memcmp(c2_prime, kct->c2, sizeof(c2_prime)) != 0)
    {
        free(T);
        return -1; // 密文不匹配
    }

    // 派生最终会话密钥 K = H4(m, c1, c2) (512 比特)
    if (kem_hash(H4_PREFIX,
                 m_prime, UVW_M,
                 (unsigned char *)kct->c1, sizeof(kct->c1) * 8,
                 (unsigned char *)kct->c2, sizeof(kct->c2) * 8,
                 512, ss) != 0)
    {
        free(T);
        return -6;
    }

    free(kct);

    *ss_len_bytes = kem_get_ss_len_bytes();
    free(T);
    return 0;
}

// 封装 DRNG 调用：把 Bytes 转换为 Bits，返回out_len_bytes长度的随机字节
int get_random_bytes(unsigned char *out, int out_len_bytes)
{
    return get_random_number(&drng_algorithm, out, (unsigned long long)out_len_bytes * 8);
}

// 生成随机矩阵
static void generate_random_matrix(gf_elem_t *matrix, int size, DRNG_ctx *drng)
{
    uint8_t buf[CHUNK_SIZE];    // 在栈上开辟缓冲区
    int elements_generated = 0; // 记录我们已经成功生成了多少个矩阵元素

    // 只要矩阵还没填满，就继续
    while (elements_generated < size)
    {
        // 一次性拿 2048 字节
        get_random_bytes_from(drng, buf, CHUNK_SIZE);

        // 步长为 2
        for (int i = 0; i < CHUNK_SIZE; i += 2)
        {
            // 拼装 16 位整数
            uint16_t raw_val = (buf[i + 1] << 8) | buf[i];

            // 取低 9 位 (0 到 511)
            uint16_t val = raw_val & MASK_Q_BITS;

            // 拒绝采样
            if (val < UVW_Q)
            {
                matrix[elements_generated++] = (gf_elem_t)val;

                if (elements_generated == size)
                {
                    return;
                }
            }
        }
    }
}

// 辅助函数：生成 [0, limit-1] 之间的均匀随机数 (拒绝采样)
static uint16_t get_rand_range(uint16_t limit)
{
    unsigned char rand_buf[2];
    uint16_t val;
    uint16_t mask;

    // 1. 计算覆盖 limit 的最小掩码 (例如 limit=5, mask=7 (111b))
    // 这是一个简单的方法：将 limit 向右填满 1
    mask = limit;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;
    // 此时 mask 是 >= limit 的最小 2^k - 1

    while (1)
    {
        // 2. 获取随机数
        get_random_bytes(rand_buf, 2);
        val = (rand_buf[1] << 8) | rand_buf[0];

        // 3. 应用掩码，缩小范围到接近 limit 的 2 的幂次
        val &= mask;

        // 4. 拒绝采样：如果落在 [0, limit-1] 区间内，就接受
        if (val < limit)
        {
            return val;
        }
        // 否则重试 (拒绝掉多余的部分)
    }
}

// 带有缓冲区的批量随机范围采样
// ctx_buf: 缓冲区指针, pos: 当前读取位置, buf_size: 缓冲区总大小
static uint16_t get_rand_range_batched(uint16_t limit, uint8_t *buf, int *pos, DRNG_ctx *drng)
{
    uint16_t val;
    uint16_t mask;

    // 1. 计算掩码
    mask = limit;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;

    while (1)
    {
        // 2. 检查缓冲区是否够用 (至少需要 2 字节)
        if (*pos + 2 > CHUNK_SIZE)
        {
            get_random_bytes_from(drng, buf, CHUNK_SIZE);
            *pos = 0; // 重置指针，重新打了一桶水
        }

        // 3. 从缓冲区提取 2 字节
        val = (buf[*pos + 1] << 8) | buf[*pos];
        *pos += 2;

        // 4. 应用掩码并拒绝采样
        val &= mask;
        if (val < limit)
        {
            return val;
        }
    }
}

// Knuth Shuffle 生成随机置换向量 (存储在 D_perm 中)
static void generate_permutation(uint16_t *p, int n, DRNG_ctx *drng)
{
    uint8_t rand_pool[CHUNK_SIZE];
    int pool_pos = CHUNK_SIZE; // 初始设为满，触发第一次填装

    // 1. 初始化 
    for (int i = 0; i < n; i++)
        p[i] = (uint16_t)i;

    // 2. 洗牌 (从后往前)
    for (int i = n - 1; i > 0; i--)
    {
        // limit = i + 1
        uint16_t j = get_rand_range_batched(i + 1, rand_pool, &pool_pos, drng);

        // 3. 交换
        uint16_t temp = p[i];
        p[i] = p[j];
        p[j] = temp;
    }
}

// 生成非零随机数数组 (用于 D 的对角系数 )
static void generate_nonzero_random_array(gf_elem_t *array, int size, DRNG_ctx *drng)
{
    uint8_t buf[CHUNK_SIZE];
    int elements_generated = 0;

    while (elements_generated < size)
    {
        // 1. 一次性获取大块随机字节
        get_random_bytes_from(drng, buf, CHUNK_SIZE);

        // 2. 在内存缓冲区中扫描
        for (int i = 0; i < CHUNK_SIZE; i += 2)
        {
            // 拼装 16 位原始随机数
            uint16_t raw_val = (buf[i + 1] << 8) | buf[i];

            // 仅保留低 9 位 (0-511)
            uint16_t val = raw_val & MASK_Q_BITS;

            // 3. 核心拒绝采样逻辑：
            // 必须满足在有限域范围内 ( < 433 ) 且 必须非零 ( != 0 )
            if (val < UVW_Q && val != 0)
            {
                array[elements_generated++] = (gf_elem_t)val;

                // 填满即刻退出
                if (elements_generated == size)
                {
                    return;
                }
            }
        }
    }
}

// 矩阵乘法 C = A * B
//        mxn   mxk   kxn
static void matrix_mul(gf_elem_t *C, const gf_elem_t *A, const gf_elem_t *B, int m, int k, int n)
{
    // 初始化 C 为 0
    memset(C, 0, m * n * sizeof(gf_elem_t));

    for (int i = 0; i < m; i++)
    {
        for (int l = 0; l < k; l++)
        {
            gf_elem_t a = A[i * k + l];
            if (a == 0)
                continue; // 稀疏优化

            const gf_elem_t *B_row = &B[l * n];
            for (int j = 0; j < n; j++)
            {
                // C[i][j] += A[i][l] * B[l][j]
                C[i * n + j] = gf_add(C[i * n + j], gf_mul(a, B_row[j]));
            }
        }
    }
}

// 计算矩阵的秩 (Gaussian Elimination)
// 注意：会破坏 matrix_copy 的数据
static int compute_rank(gf_elem_t *matrix_copy, int rows, int cols)
{
    int rank = 0;

    // 1. 在物理栈上分配“行指针数组”
    // 交换行时只换指针
    gf_elem_t *row_ptrs[rows];
    for (int i = 0; i < rows; i++)
    {
        row_ptrs[i] = matrix_copy + i * cols;
    }

    // 遍历列 (主元列)
    for (int j = 0; j < cols && rank < rows; j++)
    {

        // 找主元
        int pivot_row = rank;
        while (pivot_row < rows && row_ptrs[pivot_row][j] == 0)
        {
            pivot_row++;
        }

        if (pivot_row == rows)
        {
            continue; // 自由变量，跳过
        }

        // 2. 只交换 8 字节的指针
        if (pivot_row != rank)
        {
            gf_elem_t *tmp = row_ptrs[rank];
            row_ptrs[rank] = row_ptrs[pivot_row];
            row_ptrs[pivot_row] = tmp;
        }

        // 获取当前主元行的首地址和主元的值
        gf_elem_t *pivot_row_ptr = row_ptrs[rank];
        gf_elem_t pivot_val = pivot_row_ptr[j];

        // 3. 我们只需算出消元因子所需的逆元
        gf_elem_t inv_pivot = gf_inv(pivot_val);

        // 4. 内层消元
        for (int i = rank + 1; i < rows; i++)
        {
            gf_elem_t *current_row_ptr = row_ptrs[i];
            gf_elem_t target_val = current_row_ptr[j];

            if (target_val != 0)
            {
                // factor = target_val / pivot_val
                gf_elem_t factor = gf_mul(target_val, inv_pivot);

                // 不算直接写 0
                current_row_ptr[j] = 0;

                // k 直接从 j + 1 开始
                for (int k = j + 1; k < cols; k++)
                {
                    gf_elem_t sub_val = gf_mul(factor, pivot_row_ptr[k]);
                    current_row_ptr[k] = gf_sub(current_row_ptr[k], sub_val);
                }
            }
        }

        rank++;
    }

    return rank;
}

// 生成指定大小的满秩矩阵 (带重试机制)
// rows <= cols 时要求行满秩 (秩 = rows)
// rows > cols 时要求列满秩 (秩 = cols) -> 通常我们只用前一种情况
static int generate_full_rank_matrix(gf_elem_t *matrix, int rows, int cols, DRNG_ctx *drng)
{
    int target_rank = (rows < cols) ? rows : cols;

    // 临时缓冲区，用于计算秩（因为计算过程会破坏数据）
    gf_elem_t *temp_buffer = (gf_elem_t *)malloc(rows * cols * sizeof(gf_elem_t));
    if (!temp_buffer)
        return -1; // 内存错误，NULL（0x0）<->false,非NULL地址<->true

    int attempt = 0;
    while (attempt < KEM_KEYGEN_MAX_ATTEMPTS)
    {
        attempt++;

        // 1. 生成随机矩阵
        generate_random_matrix(matrix, rows * cols, drng);

        // 2. 拷贝一份数据到 temp_buffer
        memcpy(temp_buffer, matrix, rows * cols * sizeof(gf_elem_t));

        // 3. 计算秩
        int current_rank = compute_rank(temp_buffer, rows, cols);

        // 4. 检查是否满秩
        if (current_rank == target_rank)
        {
            free(temp_buffer);
            return 0; // 成功！退出循环
        }

        // 否则：循环继续，重新生成
        // ( q >= 401 时，重试概率极低，通常一次过)
    }

    // 运行到这里说明失败了
    free(temp_buffer);
    return -2; // 一直不满秩，返回一个特殊的错误码
}

// 没用到
// 生成互不相同的随机求值点 alpha (长度为 count)，用于 RS 码生成矩阵
// 原理：生成全集 [0..Q-1]，然后洗牌，取前 count 个
static void generate_distinct_evaluation_points(gf_elem_t *alpha, int count)
{
    // 临时池，大小为 Q (例如 401, 797, 1499)，栈上分配完全够用
    gf_elem_t pool[UVW_Q];

    // 初始化池子: [0, 1, 2, ..., Q-1]
    for (int i = 0; i < UVW_Q; i++)
    {
        pool[i] = (gf_elem_t)i;
    }

    // Fisher-Yates 洗牌 (只需要洗前 count 次)
    for (int i = 0; i < count; i++)
    {
        // 在剩余的 [i, Q-1] 里随机选一个下标
        uint16_t range = UVW_Q - i;
        uint16_t rand_offset = get_rand_range(range);
        uint16_t pick_idx = i + rand_offset;

        // 交换 pool[i] 和 pool[pick_idx]
        gf_elem_t temp = pool[i];
        pool[i] = pool[pick_idx];
        pool[pick_idx] = temp;
    }

    // 取出前 count 个作为结果
    for (int i = 0; i < count; i++)
    {
        alpha[i] = pool[i];
    }
}

// 2. 生成 RS 码生成矩阵 (Vandermonde 矩阵)
// rows: k2, cols: n/2
// G[r][c] = alpha[c] ^ r
static void generate_rs_matrix(gf_elem_t *G, int rows, int cols)
{
    // 申请临时内存存放求值点
    gf_elem_t *alpha = (gf_elem_t *)malloc(cols * sizeof(gf_elem_t));
    if (!alpha)
        return; // 错误处理

    // ==========================================
    // 直接固定使用 1 到 cols 作为求值点
    // 因为外部的 D 矩阵会负责打乱和伪装
    // ==========================================
    for (int c = 0; c < cols; c++)
    {
        alpha[c] = (gf_elem_t)(c + 1);
    }

    // 填充矩阵 (这部分保持你原来的优秀逻辑不变)
    for (int c = 0; c < cols; c++)
    {
        // 第 0 行全是 1 (alpha[c]^0 = 1)
        G[0 * cols + c] = 1;

        // 优化：利用上一行的结果递推，避免频繁调用 gf_pow
        if (rows > 1)
        {
            gf_elem_t val = alpha[c]; // 也就是 alpha[c]^1
            G[1 * cols + c] = val;

            for (int r = 2; r < rows; r++)
            {
                val = gf_mul(val, alpha[c]);
                G[r * cols + c] = val;
            }
        }
    }

    free(alpha);
}

/**
 * 采样重量为 W 的错误向量 e ∈ Fq,n
 */
static void sample_error_vector(gf_elem_t *e, int n, int w, DRNG_ctx *drng)
{
    // 1. 初始化全零向量
    memset(e, 0, n * sizeof(gf_elem_t));

    // 2. 在栈上准备一个索引池 [0, 1, ..., n-1]
    // 假设 n 在 1024 左右，这只占用 2KB 栈空间
    uint16_t pool[n];
    for (int i = 0; i < n; i++)
    {
        pool[i] = (uint16_t)i;
    }

    // 3. 准备随机数大水桶
    uint8_t rand_pool[CHUNK_SIZE];
    int pool_pos = CHUNK_SIZE; // 初始设为满，触发第一次填装

    // 4. 循环 W 次
    for (int i = 0; i < w; i++)
    {
        // 随机选出一个不重复的位置
        // 在剩余的 [i, n-1] 范围内选一个下标
        uint16_t range = n - i;
        uint16_t rand_offset = get_rand_range_batched(range, rand_pool, &pool_pos, drng);
        uint16_t pick_idx = i + rand_offset;

        // 交换索引，保证 pool[i] 拿到的是本轮选中的唯一位置
        uint16_t pos = pool[pick_idx];
        pool[pick_idx] = pool[i];
        // pool[i] = pos; // 实际上我们只需要 pos，不需要写回 pool[i]

        // 为该位置生成一个非零随机值
        gf_elem_t val;
        do
        {
            // 同样使用批量采样器获取 [0, Q-1] 的值
            val = (gf_elem_t)get_rand_range_batched(UVW_Q, rand_pool, &pool_pos, drng);
        } while (val == 0); // 确保非零

        // 5. 写入错误向量
        e[pos] = val;
    }
}

/**
 * UVW.PKE.Enc(pk, m) -> c
 * @param pk_bytes  输入公钥字节数组
 * @param m         输入消息 (长度 UVW_M bits)
 * @param ct_struct 输出密文结构体
 */
void uvw_pke_enc(const unsigned char *pk_bytes, const unsigned char *m, ciphertext_t *ct_struct)
{
    int k = UVW_K;
    int n = UVW_N;
    int t_cols = n - k;

    gf_elem_t r[UVW_K];
    gf_elem_t e[UVW_N];
    unsigned char mask[UVW_M / 8];

    // =========================================================
    // 解包公钥，获取矩阵 T
    // =========================================================
    public_key_t *temp_pk = (public_key_t *)malloc(sizeof(public_key_t));
    uvw_unpack_pk(temp_pk, pk_bytes);

    // 1. 随机选择向量 r ← Fq^k
    generate_random_matrix(r, k, &drng_algorithm);

    // 2. 随机选择一个重量为 w 的向量 e ∈ Eq,n
    sample_error_vector(e, n, UVW_W, &drng_algorithm);

    // =========================================================
    // 3. 计算 c1 = r * [I_k | T] + e
    // =========================================================

    // 3.1 左半部分: c1_left = r * I_k + e_left = r + e_left
    // 完全没有乘法开销，直接将 r 和 e 的前半部分对应相加
    for (int i = 0; i < k; i++)
    {
        ct_struct->c1[i] = gf_add(r[i], e[i]);
    }

    // 3.2 右半部分: c1_right = r * T + e_right
    // 仅需计算缩减版矩阵 T 的乘法
    gf_elem_t rT[UVW_N - UVW_K];

    // 参数说明: 输出数组 rT, 输入向量 r, 矩阵 pk_struct->T, 行数 k, 列数 t_cols
    gf_vec_mat_mul(rT, r, temp_pk->T, k, t_cols);

    // 加上 e 的右半部分
    for (int i = 0; i < t_cols; i++)
    {
        ct_struct->c1[k + i] = gf_add(rT[i], e[k + i]);
    }

    // =========================================================
    // 4. 计算 c2 = m ⊕ H(r, e)
    // =========================================================
    // 准备哈希输入：拼接 r 和 e
    int input_len_bytes = (k + n) * sizeof(gf_elem_t);
    unsigned char *hash_input = (unsigned char *)malloc(input_len_bytes);
    if (hash_input)
    {
        memcpy(hash_input, r, k * sizeof(gf_elem_t));
        memcpy(hash_input + (k * sizeof(gf_elem_t)), e, n * sizeof(gf_elem_t));

        // 调用 auxfunc.c 中的 sm3hash 生成掩码
        sm3hash(UVW_M, hash_input, (unsigned long long)input_len_bytes * 8, mask);

        // 异或运算
        for (int i = 0; i < UVW_M / 8; i++)
        {
            ct_struct->c2[i] = m[i] ^ mask[i];
        }
        free(hash_input);
    }
    free(temp_pk);
}

// 矩阵求逆 (使用高斯消元法)
// 返回 0 表示成功，返回 -1 表示奇异矩阵不可逆
static int gf_mat_inverse(gf_elem_t *inv, const gf_elem_t *A, int k)
{
    // 动态分配工作矩阵
    gf_elem_t *tmp_A = (gf_elem_t *)malloc((size_t)k * k * sizeof(gf_elem_t));
    if (!tmp_A) return -1;   // 内存不足

    //  初始化：拷贝 A，并将 inv 初始化为纯净的单位阵 I_k
    for (int i = 0; i < k; i++)
    {
        gf_elem_t *row_A = tmp_A + i * k;
        gf_elem_t *row_inv = inv + i * k;

        // 向量化拷贝 A 的一行
        memcpy(row_A, A + i * k, k * sizeof(gf_elem_t));

        // 清零 inv 的一行，并设置对角线为 1
        memset(row_inv, 0, k * sizeof(gf_elem_t));
        row_inv[i] = 1;
    }

    // 消元
    for (int i = 0; i < k; i++)
    {
        gf_elem_t *row_i_A = tmp_A + i * k;
        gf_elem_t *row_i_inv = inv + i * k;

        // 找主元
        int pivot = i;
        while (pivot < k && tmp_A[pivot * k + i] == 0)
        {
            pivot++;
        }
        if (pivot == k)
            return -1; // 奇异矩阵，不可逆

        // 物理行交换
        if (pivot != i)
        {
            gf_elem_t *row_pivot_A = tmp_A + pivot * k;
            gf_elem_t *row_pivot_inv = inv + pivot * k;

            // 对于 tmp_A：由于 0 到 i-1 绝对是 0，只交换从 i 开始的数据
            for (int j = i; j < k; j++)
            {
                gf_elem_t tmp = row_i_A[j];
                row_i_A[j] = row_pivot_A[j];
                row_pivot_A[j] = tmp;
            }
            // 对于 inv：它的非零数据分布在整行，必须交换整行
            for (int j = 0; j < k; j++)
            {
                gf_elem_t tmp = row_i_inv[j];
                row_i_inv[j] = row_pivot_inv[j];
                row_pivot_inv[j] = tmp;
            }
        }

        // 归一化
        gf_elem_t inv_val = gf_inv(row_i_A[i]);
        row_i_A[i] = 1; // 直接写 1，省去乘法

        // tmp_A：只乘右侧有效数据
        for (int j = i + 1; j < k; j++)
        {
            row_i_A[j] = gf_mul(row_i_A[j], inv_val);
        }
        // inv：必须乘整行
        for (int j = 0; j < k; j++)
        {
            row_i_inv[j] = gf_mul(row_i_inv[j], inv_val);
        }

        // 行消元
        for (int r = 0; r < k; r++)
        {
            if (r == i)
                continue; // 跳过自己

            gf_elem_t *row_r_A = tmp_A + r * k;
            gf_elem_t factor = row_r_A[i];

            if (factor != 0)
            {
                row_r_A[i] = 0; // 直接物理写 0，省去乘减

                gf_elem_t *row_r_inv = inv + r * k;

                // tmp_A：只减右侧有效数据
                for (int j = i + 1; j < k; j++)
                {
                    row_r_A[j] = gf_sub(row_r_A[j], gf_mul(factor, row_i_A[j]));
                }
                // inv：必须减整行
                for (int j = 0; j < k; j++)
                {
                    row_r_inv[j] = gf_sub(row_r_inv[j], gf_mul(factor, row_i_inv[j]));
                }
            }
        }
    }

    free(tmp_A);
    return 0;
}

/**
 * UVW.PKE.Dec(sk, c) -> m
 * @param sk          输入私钥字节数组
 * @param ct          输入密文结构体
 * @param output_m    输出消息 (长度 UVW_M bits)
 * @return            0 表示解密成功，非 0 表示失败
 */
int uvw_pke_dec(const unsigned char *sk_bytes, ciphertext_t *ct, unsigned char *output_m)
{
    int n = UVW_N;
    int half_n = UVW_N / 2;
    int k = UVW_K;
    int k1 = UVW_K1;
    int k2 = UVW_K2;

    unsigned char seed[PRIVATE_KEY_SEED_BYTES];
    memcpy(seed, sk_bytes, PRIVATE_KEY_SEED_BYTES);
    uint8_t i1 = sk_bytes[PRIVATE_KEY_SEED_BYTES];
    uint8_t i2 = sk_bytes[PRIVATE_KEY_SEED_BYTES + 1];
    uint8_t i3 = sk_bytes[PRIVATE_KEY_SEED_BYTES + 2];

    private_key_t *temp_sk = (private_key_t *)malloc(sizeof(private_key_t));

    expand_GU_Dec(seed, i1, temp_sk);
    expand_GW_Dec(seed, i2, temp_sk);
    expand_D(seed, i3, temp_sk);



    // 由于 G_RS 是固定的，直接从预定义的常量中复制
    memcpy(temp_sk->G_RS, FIXED_G_RS, k2 * half_n * sizeof(gf_elem_t));


    // ---------- 重建 A = 左侧 k × k 块 ----------
    gf_elem_t *G_tmp = (gf_elem_t *)malloc(k * n * sizeof(gf_elem_t));
    gf_elem_t *G_sk = (gf_elem_t *)malloc(k * n * sizeof(gf_elem_t));
    if (!G_tmp || !G_sk)
    {
        free(temp_sk);
        free(G_tmp);
        free(G_sk);
        return -1;
    }

    //  拼装 G_tmp = [G_U | G_U; G_V | G_W]
    size_t half_row_bytes = half_n * sizeof(gf_elem_t);
    for (int r = 0; r < k1; r++)
    {
        gf_elem_t *dest = G_tmp + r * n;
        memcpy(dest, temp_sk->G_U + r * half_n, half_row_bytes);
        memcpy(dest + half_n, temp_sk->G_U + r * half_n, half_row_bytes);
    }
    for (int r = 0; r < k2; r++)
    {
        gf_elem_t *dest = G_tmp + (k1 + r) * n;
        for (int c = 0; c < half_n; c++)
            dest[c] = gf_add(temp_sk->G_RS[r * half_n + c], temp_sk->G_W[r * half_n + c]);
        memcpy(dest + half_n, temp_sk->G_W + r * half_n, half_row_bytes);
    }

    //  G_sk = G_tmp * D
    for (int r = 0; r < k; r++)
        for (int c = 0; c < n; c++)
        {
            int old = temp_sk->D_perm[c];
            G_sk[r * n + c] = gf_mul(G_tmp[r * n + old], temp_sk->D_coeffs[c]);
        }

    //  提取左侧 K×K 块
    size_t a_row_bytes = k * sizeof(gf_elem_t);
    for (int r = 0; r < k; r++)
        memcpy(temp_sk->A + r * k, G_sk + r * n, a_row_bytes);

    free(G_tmp);
    free(G_sk);

    gf_elem_t y[UVW_N];
    gf_elem_t c11[UVW_N / 2];
    gf_elem_t c12[UVW_N / 2];

    // =========================================================
    // 计算 c1 * D^-1 = (c11 || c12)
    // =========================================================
    for (int i = 0; i < n; i++)
    {
        int old_col = temp_sk->D_perm[i];
        gf_elem_t inv_d = gf_inv(temp_sk->D_coeffs[i]);
        y[old_col] = gf_mul(ct->c1[i], inv_d);
    }
    memcpy(c11, y, half_n * sizeof(gf_elem_t));
    memcpy(c12, y + half_n, half_n * sizeof(gf_elem_t));

    // =========================================================
    // 计算 r2 和 \bar{e}
    // r2 * G_RS + \bar{e} = c11 - c12
    // =========================================================
    gf_elem_t y_diff[UVW_N / 2];
    for (int i = 0; i < half_n; i++)
        y_diff[i] = gf_sub(c11[i], c12[i]);

    gf_elem_t support_set[UVW_N / 2];
    for (int i = 0; i < half_n; i++)
        support_set[i] = (gf_elem_t)(i + 1);
        
    gf_elem_t r2[UVW_K2];
    int decode_ret = uvw_rs_list_decode(y_diff, support_set, r2);
    if (decode_ret <= 0)
        return -1; // RS 译码失败


    // // ========================================================
    // // 测量 RS 列表译码耗时
    // // ========================================================
    // unsigned long long rs_start_cycles = cpucycles(); // <--- 紧贴译码开始

    // // 执行核心译码函数，并把结果暂存下来
    // int decode_ret = uvw_rs_list_decode(y_diff, support_set, r2);

    // unsigned long long rs_end_cycles = cpucycles();   // <--- 紧贴译码结束
    // unsigned long long rs_cycles = rs_end_cycles - rs_start_cycles;

    // // 打印译码消耗的绝对 CPU 周期
    // printf("RS 列表译码耗时: %llu CPU 周期\n", rs_cycles);
    // // ========================================================

    // // 恢复原有的错误判断逻辑
    // if (decode_ret <= 0) {
    //     return -1; // RS 译码失败
    // }

    // 计算 \bar{e} = (c11 - c12) - r2 * G_RS
    gf_elem_t e_bar[UVW_N / 2];
    gf_elem_t r2_G_RS[UVW_N / 2];
    gf_vec_mat_mul(r2_G_RS, r2, temp_sk->G_RS, k2, half_n);

    for (int i = 0; i < half_n; i++)
        e_bar[i] = gf_sub(y_diff[i], r2_G_RS[i]);

    // 找到无错集合 I = [n/2] \ supp(\bar{e})
    int I_set[UVW_N / 2];
    int num_I = 0;
    for (int i = 0; i < half_n; i++)
    {
        if (e_bar[i] == 0)
            I_set[num_I++] = i;
    }

    // 如果无错位置不够 k1 个，无法构成信息集，直接失败
    if (num_I < k1)
        return -2;

    // 提前计算常量 (c12 - r2 * Gw)
    gf_elem_t r2_G_W[UVW_N / 2];
    gf_elem_t c12_minus_r2Gw[UVW_N / 2];
    gf_vec_mat_mul(r2_G_W, r2, temp_sk->G_W, k2, half_n);
    for (int i = 0; i < half_n; i++)
        c12_minus_r2Gw[i] = gf_sub(c12[i], r2_G_W[i]);

    // =========================================================
    // 提取信息集 I1, 求解 r1，验证重量
    // =========================================================
    int max_attempts = 1000; // 防止无限死循环
    int success = 0;
    gf_elem_t r1[UVW_K1];
    gf_elem_t r[UVW_K];
    gf_elem_t e_final[UVW_N];

    for (int attempt = 0; attempt < max_attempts; attempt++)
    {
        // 随机数池和位置指针
        uint8_t rand_pool[CHUNK_SIZE];
        int pool_pos = CHUNK_SIZE;

        // 随机打乱 I_set 的前部，挑选 k1 个元素构成集合 I1
        int I1[UVW_K1];
        for (int i = 0; i < k1; i++)
        {
            uint16_t limit = (uint16_t)(num_I - i);

            uint16_t rand_offset = get_rand_range_batched(limit, rand_pool, &pool_pos, &drng_algorithm);
            int pick = i + (int)rand_offset;

            int temp = I_set[i];
            I_set[i] = I_set[pick];
            I_set[pick] = temp;
            I1[i] = I_set[i];
        }

        // 构建子矩阵 (G_U)_{I1} (大小为 k1 x k1)
        // 求子矩阵的逆 (G_U)^-1_{I1}
        gf_elem_t *GU_I1 = (gf_elem_t *)malloc((size_t)k1 * k1 * sizeof(gf_elem_t));
        gf_elem_t *GU_I1_inv = (gf_elem_t *)malloc((size_t)k1 * k1 * sizeof(gf_elem_t));
        if (!GU_I1 || !GU_I1_inv) {
            free(GU_I1);
            free(GU_I1_inv);
            continue;
        }
        for (int row = 0; row < k1; row++)
        {
            for (int col = 0; col < k1; col++)
            {
                GU_I1[row * k1 + col] = temp_sk->G_U[row * half_n + I1[col]];
            }
        }


        if (gf_mat_inverse(GU_I1_inv, GU_I1, k1) != 0)
            continue; // 不是满秩，重新挑！

        // 提取对应的向量并求解: r1 = (c12 - r2Gw)_{I1} * (G_U)^-1_{I1}
        gf_elem_t vec_I1[UVW_K1];
        for (int i = 0; i < k1; i++)
            vec_I1[i] = c12_minus_r2Gw[I1[i]];
        gf_vec_mat_mul(r1, vec_I1, GU_I1_inv, k1, k1);

        // 计算完整的 r
        // 直接用私钥里存好的 sk->S (天然等于逆矩阵) 去乘
        gf_elem_t r_prime[UVW_K];
        memcpy(r_prime, r1, k1 * sizeof(gf_elem_t));
        memcpy(r_prime + k1, r2, k2 * sizeof(gf_elem_t));

        gf_vec_mat_mul(r, r_prime, temp_sk->A, k, k);

        // ==========================================
        // 验证重量: e = c1 - r * Gpk
        // r * Gpk = r * S * G_{sk} * D
        //         = (r1 || r2) * [G_U | G_U] * D
        //                        [G_V | G_W]
        //         = (r1 * G_U + r2 * G_V | r1 * G_U + r2 * G_W) * D
        // ==========================================
        gf_elem_t v_left[UVW_N / 2];
        gf_elem_t v_right[UVW_N / 2];

        // 计算 r1 * G_U
        gf_elem_t r1_GU[UVW_N / 2];
        gf_vec_mat_mul(r1_GU, r1, temp_sk->G_U, k1, half_n);

        // 计算左右两半的无错密文
        for (int i = 0; i < half_n; i++)
        {
            v_right[i] = gf_add(r1_GU[i], r2_G_W[i]);
            v_left[i] = gf_add(v_right[i], r2_G_RS[i]);
        }

        int err_weight = 0;
        for (int i = 0; i < n; i++)
        {
            int old_col = temp_sk->D_perm[i];
            gf_elem_t v_val = (old_col < half_n) ? v_left[old_col] : v_right[old_col - half_n];
            gf_elem_t c1_prime_i = gf_mul(v_val, temp_sk->D_coeffs[i]);
            e_final[i] = gf_sub(ct->c1[i], c1_prime_i);

            if (e_final[i] != 0)
                err_weight++;
        }

        if (err_weight <= UVW_W)
        {
            success = 1;
            break;
        }
    }

    free(temp_sk);

    if (!success)
        return -4; // 超过最大尝试次数

    // =========================================================
    // 输出: m = c2 XOR H(r, e)
    // =========================================================
    int input_len_bytes = (k + n) * sizeof(gf_elem_t);
    unsigned char *hash_input = (unsigned char *)malloc(input_len_bytes);
    memcpy(hash_input, r, k * sizeof(gf_elem_t));
    memcpy(hash_input + (k * sizeof(gf_elem_t)), e_final, n * sizeof(gf_elem_t));

    unsigned char mask[UVW_M / 8];
    if (kem_hash(H3_PREFIX,
                 hash_input, input_len_bytes * 8,
                 NULL, 0,
                 NULL, 0,
                 UVW_M, mask) != 0)
    {
        free(hash_input);
        return -5;
    }
    free(hash_input);

    for (int i = 0; i < UVW_M / 8; i++)
    {
        output_m[i] = ct->c2[i] ^ mask[i];
    }

    return 0; // 解密成功
}

// ========================================================
// 将 k x n 矩阵化为系统形式 [I_k | T]
// 返回 0 表示成功，返回 -1 表示前 k 列不满秩
// ========================================================
static int systematic_gaussian_elimination(gf_elem_t *G, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        // 缓存当前主元行的首地址，极大地加速后续内存寻址
        gf_elem_t *row_i = G + i * cols;

        // 1. 找主元 (Pivot)
        int pivot = i;
        while (pivot < rows && G[pivot * cols + i] == 0)
        {
            pivot++;
        }
        if (pivot == rows)
        {
            return -1; // 致命错误：前 k 列不可逆
        }

        // 缓存找到的主元行的首地址
        gf_elem_t *row_pivot = G + pivot * cols;

        // 2. 物理行交换：只交换从 i 开始的有效数据块
        // 因为 0 到 i-1 绝对全是 0，没必要换
        if (pivot != i)
        {
            for (int c = i; c < cols; c++)
            {
                gf_elem_t temp = row_i[c];
                row_i[c] = row_pivot[c];
                row_pivot[c] = temp;
            }
        }

        // 3. 归一化 (主对角线化为 1)
        gf_elem_t inv_val = gf_inv(row_i[i]);
        row_i[i] = 1; // 明确赋值为 1，省去一次乘法

        // 只处理右侧有效载荷
        for (int c = i + 1; c < cols; c++)
        {
            row_i[c] = gf_mul(row_i[c], inv_val);
        }

        // 4. 行消元 (将第 i 列的其他元素化为 0)
        for (int r = 0; r < rows; r++)
        {
            if (r == i)
                continue; // 跳过当前主元行自身

            gf_elem_t *row_r = G + r * cols;
            gf_elem_t factor = row_r[i];

            if (factor != 0)
            {
                row_r[i] = 0; // 明确赋值为 0，省去一次乘减操作

                // 避开已知是 0 的区域
                for (int c = i + 1; c < cols; c++)
                {
                    row_r[c] = gf_sub(row_r[c], gf_mul(factor, row_i[c]));
                }
            }
        }
    }
    return 0; // 成功化简
}

/**
 * 将 16-bit 存储的系统形式公钥 T 压缩为紧凑字节流 (严格遵循 MSB first 规范)
 * 采用 8-to-9 块状无分支压缩
 */
static void uvw_pack_pk(unsigned char *pk_bytes, const public_key_t *pk_struct)
{
    int total_elements = UVW_K * (UVW_N - UVW_K);
    compress_gf_array(pk_bytes, pk_struct->T, total_elements);
}

/**
 * 将 MSB first 规范的紧凑字节流解压恢复为公钥结构体
 * 采用 9-to-8 块状无分支解压
 */
static void uvw_unpack_pk(public_key_t *pk_struct, const unsigned char *pk_bytes)
{
    int total_elements = UVW_K * (UVW_N - UVW_K);
    decompress_gf_array(pk_struct->T, pk_bytes, total_elements);
}

static int get_random_bytes_from(DRNG_ctx *drng, unsigned char *out, int out_len_bytes)
{
    return get_random_number(drng, out, (unsigned long long)out_len_bytes * 8);
}

// -----------------------------------------------------------
// 由 seed 扩展出 G_U, G_W
// -----------------------------------------------------------
static int expand_GU_KeyGen(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i1,
                        private_key_t *sk_struct)
{
    unsigned char gu_seed[PRIVATE_KEY_SEED_BYTES + 2];
    memcpy(gu_seed, seed, PRIVATE_KEY_SEED_BYTES);
    gu_seed[PRIVATE_KEY_SEED_BYTES] = i1;
    gu_seed[PRIVATE_KEY_SEED_BYTES + 1] = 1;

    DRNG_ctx drng_u;
    init_random_number(&drng_u, gu_seed, sizeof(gu_seed));

    int half_n = UVW_N / 2;

    generate_random_matrix(sk_struct->G_U, UVW_K1 * half_n, &drng_u);
    gf_elem_t *temp_buffer = (gf_elem_t *)malloc(UVW_K1 * half_n * sizeof(gf_elem_t));
    if (!temp_buffer)
        return -1;

    memcpy(temp_buffer, sk_struct->G_U, UVW_K1 * half_n * sizeof(gf_elem_t));

    int current_rank = compute_rank(temp_buffer, UVW_K1, half_n);

    if (current_rank == UVW_K1)
        {
            free(temp_buffer);
            return 0;
        }

    return -1;
}

static void expand_GU_Dec(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i1,
                        private_key_t *sk_struct)
{
    unsigned char gu_seed[PRIVATE_KEY_SEED_BYTES + 2];
    memcpy(gu_seed, seed, PRIVATE_KEY_SEED_BYTES);
    gu_seed[PRIVATE_KEY_SEED_BYTES] = i1;
    gu_seed[PRIVATE_KEY_SEED_BYTES + 1] = 1;

    DRNG_ctx drng_u;
    init_random_number(&drng_u, gu_seed, sizeof(gu_seed));

    int half_n = UVW_N / 2;

    generate_random_matrix(sk_struct->G_U, UVW_K1 * half_n, &drng_u);
}

static int expand_GW_KeyGen(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i2,
                        private_key_t *sk_struct)
{
    unsigned char gw_seed[PRIVATE_KEY_SEED_BYTES + 2];
    memcpy(gw_seed, seed, PRIVATE_KEY_SEED_BYTES);
    gw_seed[PRIVATE_KEY_SEED_BYTES] = i2;
    gw_seed[PRIVATE_KEY_SEED_BYTES + 1] = 2;

    DRNG_ctx drng_w;
    init_random_number(&drng_w, gw_seed, sizeof(gw_seed));

    int half_n = UVW_N / 2;

    generate_random_matrix(sk_struct->G_W, UVW_K2 * half_n, &drng_w);
    gf_elem_t *temp_buffer = (gf_elem_t *)malloc(UVW_K2 * half_n * sizeof(gf_elem_t));
    if (!temp_buffer)
        return -1;

    memcpy(temp_buffer, sk_struct->G_W, UVW_K2 * half_n * sizeof(gf_elem_t));

    int current_rank = compute_rank(temp_buffer, UVW_K2, half_n);

    if (current_rank == UVW_K2)
        {
            free(temp_buffer);
            return 0;
        }

    return -1;
}

static void expand_GW_Dec(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i2,
                        private_key_t *sk_struct)
{
    unsigned char gw_seed[PRIVATE_KEY_SEED_BYTES + 2];
    memcpy(gw_seed, seed, PRIVATE_KEY_SEED_BYTES);
    gw_seed[PRIVATE_KEY_SEED_BYTES] = i2;
    gw_seed[PRIVATE_KEY_SEED_BYTES + 1] = 2;

    DRNG_ctx drng_w;
    init_random_number(&drng_w, gw_seed, sizeof(gw_seed));

    int half_n = UVW_N / 2;

    generate_random_matrix(sk_struct->G_W, UVW_K2 * half_n, &drng_w);
}

// -----------------------------------------------------------
// 由 seed 扩展出单项矩阵 D（置换 + 非零对角系数）
// -----------------------------------------------------------
static void expand_D(const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i3,
                    private_key_t *sk_struct)
{
    unsigned char d_seed[PRIVATE_KEY_SEED_BYTES + 2];
    memcpy(d_seed, seed, PRIVATE_KEY_SEED_BYTES);
    d_seed[PRIVATE_KEY_SEED_BYTES] = i3;
    d_seed[PRIVATE_KEY_SEED_BYTES + 1] = 3;

    DRNG_ctx drng_d;
    init_random_number(&drng_d, d_seed, sizeof(d_seed));

    generate_permutation(sk_struct->D_perm, UVW_N, &drng_d);
    generate_nonzero_random_array(sk_struct->D_coeffs, UVW_N, &drng_d);
}

/**
 * UVW.PKE.Enc 确定性版本 (使用外部给定的 r, e)
 * @param pk_bytes  公钥字节数组
 * @param m         消息 (长度 UVW_M bits)
 * @param r         随机向量 (长度 UVW_K)
 * @param e         错误向量 (长度 UVW_N, 重量 UVW_W)
 * @param ct_struct 输出密文结构体 (c1, c2)
 */
static void uvw_pke_enc_deterministic(
    const unsigned char *pk_bytes,
    const unsigned char *m,
    const gf_elem_t *r,
    const gf_elem_t *e,
    ciphertext_t *ct_struct)
{
    int k = UVW_K;
    int n = UVW_N;
    int t_cols = n - k;

    // 解包公钥 T
    public_key_t *temp_pk = (public_key_t *)malloc(sizeof(public_key_t));
    uvw_unpack_pk(temp_pk, pk_bytes);

    // =========================================================
    // 计算 c1 = r * [I_k | T] + e
    // =========================================================
    // 左半部分: r + e_left
    for (int i = 0; i < k; i++)
        ct_struct->c1[i] = gf_add(r[i], e[i]);

    // 右半部分: r * T
    gf_elem_t rT[UVW_N - UVW_K];
    gf_vec_mat_mul(rT, r, temp_pk->T, k, t_cols);
    for (int i = 0; i < t_cols; i++)
        ct_struct->c1[k + i] = gf_add(rT[i], e[k + i]);

    free(temp_pk);

    // =========================================================
    // 计算 c2 = m ⊕ H3(r, e)   (注意：这里用 H3，但在 KEM 外部已计算)
    // 实际上在 KEM 中 c2 是通过 H3 得到的，而这个函数只负责计算 c1，
    // c2 由调用者自己异或。但为了完整性，PKE 的 c2 逻辑可以保留，
    // 但由于 KEM 的 c2 = m ⊕ H3(r, e)，且之后不会再用这个函数来异或，
    // 我们选择让函数返回前只填充 c1，c2 留给 KEM 层处理。
    // 因此这里不再计算 c2，我们在 kem_enc 中自己异或。
}

// =========================================================
// 内部辅助哈希函数：自动拼接前缀 + 最多三个消息段，调用 pseudoXOF
// msg1/msg2/msg3 可以为 NULL，对应的 len_bits 为 0
// =========================================================
static int kem_hash(
    unsigned char prefix,
    const unsigned char *msg1, unsigned long long msg1_bits,
    const unsigned char *msg2, unsigned long long msg2_bits,
    const unsigned char *msg3, unsigned long long msg3_bits,
    unsigned long long out_bits,
    unsigned char *out)
{
    // 计算总字节数（含前缀 1 字节）
    unsigned long long bytes1 = (msg1_bits + 7) / 8;
    unsigned long long bytes2 = (msg2_bits + 7) / 8;
    unsigned long long bytes3 = (msg3_bits + 7) / 8;
    unsigned long long total_bytes = 1 + bytes1 + bytes2 + bytes3;

    unsigned char *buf = (unsigned char *)malloc(total_bytes);
    if (!buf)
        return -1;

    buf[0] = prefix;
    unsigned long long pos = 1;
    if (msg1 && msg1_bits)
    {
        memcpy(buf + pos, msg1, bytes1);
        pos += bytes1;
    }
    if (msg2 && msg2_bits)
    {
        memcpy(buf + pos, msg2, bytes2);
        pos += bytes2;
    }
    if (msg3 && msg3_bits)
    {
        memcpy(buf + pos, msg3, bytes3);
    }

    int ret = pseudoXOF(out_bits, buf, total_bytes * 8, out);
    free(buf);
    return ret;
}

/**
 * 从种子重建公钥 T
 * @param seed  私钥种子 (PRIVATE_KEY_SEED_BYTES 字节)
 * @param i     索引
 * @param T      输出：公钥矩阵 (UVW_K × (UVW_N-UVW_K))，由调用者分配
 * @return 0 成功，非0 失败
 */
static int derive_pk_from_seeds(
    const unsigned char seed[PRIVATE_KEY_SEED_BYTES], uint8_t i1, uint8_t i2, uint8_t i3,
    gf_elem_t *T)
{
    int n = UVW_N;
    int k = UVW_K;
    int k1 = UVW_K1;
    int k2 = UVW_K2;
    int half_n = n / 2;
    int t_cols = n - k;

    // 用种子展开私钥结构（临时）
    private_key_t *temp_sk = (private_key_t *)malloc(sizeof(private_key_t));
    if (!temp_sk)
        return -1;

    // 展开 G_U, G_W
    expand_GU_Dec(seed, i1, temp_sk);
    expand_GW_Dec(seed, i2, temp_sk);

    // 展开 D
    expand_D(seed, i3, temp_sk);

    // 固定矩阵 G_RS
    memcpy(temp_sk->G_RS, FIXED_G_RS, k2 * half_n * sizeof(gf_elem_t));

    // 拼装 G_tmp = [ G_U | G_U ; G_V | G_W ]
    gf_elem_t *G_tmp = (gf_elem_t *)malloc(k * n * sizeof(gf_elem_t));
    if (!G_tmp)
    {
        free(temp_sk);
        return -4;
    }

    size_t half_row_bytes = half_n * sizeof(gf_elem_t);
    // 上半区 (k1 行)
    for (int r = 0; r < k1; r++)
    {
        gf_elem_t *dest = G_tmp + r * n;
        gf_elem_t *src = temp_sk->G_U + r * half_n;
        memcpy(dest, src, half_row_bytes);
        memcpy(dest + half_n, src, half_row_bytes);
    }
    // 下半区 (k2 行): G_V = G_RS + G_W, G_W
    for (int r = 0; r < k2; r++)
    {
        gf_elem_t *dest = G_tmp + (k1 + r) * n;
        gf_elem_t *src_rs = temp_sk->G_RS + r * half_n;
        gf_elem_t *src_w = temp_sk->G_W + r * half_n;
        for (int c = 0; c < half_n; c++)
        {
            dest[c] = gf_add(src_rs[c], src_w[c]);
        }
        memcpy(dest + half_n, src_w, half_row_bytes);
    }

    // 计算 G_sk = G_tmp * D
    gf_elem_t *G_sk = (gf_elem_t *)malloc(k * n * sizeof(gf_elem_t));
    if (!G_sk)
    {
        free(G_tmp);
        free(temp_sk);
        return -5;
    }
    for (int r = 0; r < k; r++)
    {
        for (int c = 0; c < n; c++)
        {
            int old_col = temp_sk->D_perm[c];
            G_sk[r * n + c] = gf_mul(G_tmp[r * n + old_col], temp_sk->D_coeffs[c]);
        }
    }
    free(G_tmp); // G_tmp 不再需要

    // 高斯消元化为系统形式 [I_k | T]
    if (systematic_gaussian_elimination(G_sk, k, n) != 0)
    {
        free(G_sk);
        free(temp_sk);
        return -6;
    }

    // 提取右半部分 T (k × t_cols)
    for (int r = 0; r < k; r++)
    {
        memcpy(T + r * t_cols, G_sk + r * n + k, t_cols * sizeof(gf_elem_t));
    }

    // 清理
    free(G_sk);
    free(temp_sk);
    return 0;
}

static void uvw_pack_ct(unsigned char *ct_bytes, const kem_ciphertext_t *ct_struct)
{
    int pos = 0;

    // 压缩 c1
    compress_gf_array(ct_bytes + pos, ct_struct->c1, UVW_N);
    pos += (((UVW_N)*UVW_Q_BITS + 7) / 8);

    // 复制 c2
    memcpy(ct_bytes + pos, ct_struct->c2, UVW_M / 8);
    pos += UVW_M / 8;

    // 复制 d
    memcpy(ct_bytes + pos, ct_struct->d, UVW_M / 8);
}

static void uvw_unpack_ct(kem_ciphertext_t *ct_struct, const unsigned char *ct_bytes)
{
    int pos = 0;

    // 解压缩 c1
    decompress_gf_array(ct_struct->c1, ct_bytes + pos, UVW_N);
    pos += (((UVW_N)*UVW_Q_BITS + 7) / 8);

    // 复制 c2
    memcpy(ct_struct->c2, ct_bytes + pos, UVW_M / 8);
    pos += UVW_M / 8;

    // 复制 d
    memcpy(ct_struct->d, ct_bytes + pos, UVW_M / 8);
}

static void compress_gf_array(unsigned char *out, const gf_elem_t *array, int total_elements)
{
    uint32_t bit_buffer = 0;
    int bits_in_buffer = 0;
    int byte_idx = 0;

    for (int i = 0; i < total_elements; i++) {
        bit_buffer = (bit_buffer << UVW_Q_BITS) | (array[i] & MASK_Q_BITS);
        bits_in_buffer += UVW_Q_BITS;

        while (bits_in_buffer >= 8) {
            bits_in_buffer -= 8;
            out[byte_idx++] = (unsigned char)((bit_buffer >> bits_in_buffer) & 0xFFU);
        }
    }

    if (bits_in_buffer > 0) {
        out[byte_idx++] = (unsigned char)((bit_buffer << (8 - bits_in_buffer)) & 0xFFU);
    }
}

static void decompress_gf_array(gf_elem_t *array, const unsigned char *in, int total_elements)
{
    uint32_t bit_buffer = 0;
    int bits_in_buffer = 0;
    int byte_idx = 0;

    for (int i = 0; i < total_elements; i++) {
        while (bits_in_buffer < UVW_Q_BITS) {
            bit_buffer = (bit_buffer << 8) | in[byte_idx++];
            bits_in_buffer += 8;
        }

        bits_in_buffer -= UVW_Q_BITS;
        array[i] = (gf_elem_t)((bit_buffer >> bits_in_buffer) & MASK_Q_BITS);
    }
}
