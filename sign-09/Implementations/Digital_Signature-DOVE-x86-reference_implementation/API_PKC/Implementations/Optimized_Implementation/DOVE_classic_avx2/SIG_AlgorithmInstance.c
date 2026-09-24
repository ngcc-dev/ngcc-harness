/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "auxfunc.h"
#include "matrix.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;




int pseudoXOFwithSeedAndCounter(unsigned long long output_len_bytes, const unsigned char *msg, unsigned long long msg_len_bytes, uint32_t counter,unsigned char *output){
    uint8_t *buf = malloc(msg_len_bytes + 2);
    memcpy(buf, msg, msg_len_bytes);
    buf[msg_len_bytes] = counter & 0xff;
    buf[msg_len_bytes + 1] = (counter >> 8) & 0xff;
    pseudoXOF(output_len_bytes*8, buf, (msg_len_bytes+2)*8, output);
    free(buf);
    return 0;
}

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

// ============================================================================
// pk 布局: seed_pk | L*Ak(上三角紧凑) | M*Pi2 | M*Pi3(上三角紧凑)
// Ak 仅存储上三角元素，避免下三角零填充
// ============================================================================
#define AK_COMPACT_BYTES ((size_t)UPPER_SIZE(DOVE_V))
#define PK_AK_BYTES      ((size_t)DOVE_L * AK_COMPACT_BYTES)
#define PK_PI2_BYTES     ((size_t)DOVE_M * DOVE_V * DOVE_O)
#define PK_PI3_BYTES     ((size_t)DOVE_M * UPPER_SIZE(DOVE_O))

#define PK_AK_OFFSET   ((size_t)DOVE_PK_SEED_BYTES)
#define PK_PI2_OFFSET  (PK_AK_OFFSET + PK_AK_BYTES)
#define PK_PI3_OFFSET  (PK_PI2_OFFSET + PK_PI2_BYTES)

// ============================================================================
// sk 布局: seed_sk | seed_pk | T | L*Ak(上三角紧凑) | L*Bk | L*Ck | M*Pi2
// ============================================================================
#define SK_T_BYTES     ((size_t)DOVE_V * DOVE_O)
#define SK_AK_BYTES    ((size_t)DOVE_L * AK_COMPACT_BYTES)
#define SK_BK_BYTES    ((size_t)DOVE_L * DOVE_O * DOVE_V)
#define SK_CK_BYTES    ((size_t)DOVE_L * DOVE_V * DOVE_O)
#define SK_PI2_BYTES   ((size_t)DOVE_M * DOVE_V * DOVE_O)

#define SK_SEED_PK_OFFSET  ((size_t)DOVE_SK_SEED_BYTES)
#define SK_T_OFFSET        (SK_SEED_PK_OFFSET + (size_t)DOVE_PK_SEED_BYTES)
#define SK_AK_OFFSET       (SK_T_OFFSET + SK_T_BYTES)
#define SK_BK_OFFSET       (SK_AK_OFFSET + SK_AK_BYTES)
#define SK_CK_OFFSET       (SK_BK_OFFSET + SK_BK_BYTES)
#define SK_PI2_OFFSET      (SK_CK_OFFSET + SK_CK_BYTES)


unsigned long long sig_get_pk_len_bytes()
{
	return (unsigned long long)(DOVE_PK_SEED_BYTES + PK_AK_BYTES + PK_PI2_BYTES + PK_PI3_BYTES);
}

unsigned long long sig_get_sk_len_bytes()
{
	 return (unsigned long long)(DOVE_SK_SEED_BYTES + DOVE_PK_SEED_BYTES
	                          + SK_T_BYTES + SK_AK_BYTES + SK_BK_BYTES + SK_CK_BYTES + SK_PI2_BYTES);
}

unsigned long long sig_get_sn_len_bytes()
{
	 return DOVE_N + DOVE_SALT_BYTES;
}

//int pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output)

// 从 seed_pk 生成第 k 个上三角矩阵 A_k
static void generate_Ak(gf256_matrix_t *Ak, const uint8_t *seed_pk, uint32_t k) {
    size_t out_len = UPPER_SIZE(DOVE_V);
    uint8_t *out = malloc(out_len);
    pseudoXOFwithSeedAndCounter(out_len,seed_pk, DOVE_PK_SEED_BYTES, k, out);
    matrix_fill_upper(Ak, out);
    free(out);
}

static void generate_Pi2(gf256_matrix_t *Pi2, const uint8_t *seed_pk, uint32_t i) {
    size_t out_len = DOVE_V * DOVE_O;
    uint32_t idx = DOVE_L + i;
    uint8_t *out = malloc(out_len);
	pseudoXOFwithSeedAndCounter(out_len,seed_pk, DOVE_PK_SEED_BYTES,idx,out);
    matrix_fill(Pi2, out);
    free(out);
}

static void derive_seedpk_T(uint8_t *seed_pk, gf256_matrix_t *T, const uint8_t *seed_sk) {
    size_t out_len = DOVE_PK_SEED_BYTES + DOVE_V * DOVE_O;
    uint8_t *out = malloc(out_len);
    pseudoXOF(out_len*8, seed_sk, DOVE_SK_SEED_BYTES*8, out);

    memcpy(seed_pk, out, DOVE_PK_SEED_BYTES);
    matrix_fill(T, out + DOVE_PK_SEED_BYTES);
    free(out);
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
    uint8_t seed_sk[DOVE_SK_SEED_BYTES];
    get_random_number(&drng_algorithm, seed_sk, (unsigned long long)DOVE_SK_SEED_BYTES*8 );
    uint8_t seed_pk[DOVE_PK_SEED_BYTES];
    gf256_matrix_t *T = matrix_create(DOVE_V, DOVE_O);
    if (!T) return -1;
    derive_seedpk_T(seed_pk, T, seed_sk);



        // sign_fprintstr(stdout, "seed_sk = ", seed_sk, DOVE_SK_SEED_BYTES);
        // sign_fprintstr(stdout, "seed_pk = ", seed_pk, DOVE_PK_SEED_BYTES);

    gf256_matrix_t *Ak[DOVE_L];
    gf256_matrix_t *Bk[DOVE_L]; // O×V
    gf256_matrix_t *Ck[DOVE_L]; // V×O
    gf256_matrix_t *Tt = matrix_create(DOVE_O, DOVE_V);
    matrix_transpose(Tt, T);

    for (int k = 0; k < DOVE_L; k++) {
        Ak[k] = matrix_create(DOVE_V, DOVE_V);
        generate_Ak(Ak[k], seed_pk, k);
        Bk[k] = matrix_create(DOVE_O, DOVE_V);
        matrix_mul(Bk[k], Tt, Ak[k]);
        Ck[k] = matrix_create(DOVE_V, DOVE_O);
        matrix_mul(Ck[k], Ak[k], T);
    }

    // ============= 组装公钥 pk = (seed_pk | L*Ak | M*Pi2 | M*Pi3) =============
    uint8_t *pk_ptr = pk;
    memcpy(pk_ptr, seed_pk, DOVE_PK_SEED_BYTES);
    pk_ptr += DOVE_PK_SEED_BYTES;

    // 写 L 个 Ak（上三角紧凑）
    for (int k = 0; k < DOVE_L; k++) {
        matrix_dump_upper(Ak[k], pk_ptr);
        pk_ptr += AK_COMPACT_BYTES;
    }

    // 中转缓冲，用于计算 Pi3
    gf256_matrix_t *Pi2 = matrix_create(DOVE_V, DOVE_O);
    gf256_matrix_t *tmp1 = matrix_create(DOVE_O, DOVE_O);
    gf256_matrix_t *tmp2 = matrix_create(DOVE_O, DOVE_O);
    gf256_matrix_t *Pi3 = matrix_create(DOVE_O, DOVE_O);

    // 写 M 个 Pi2
    for (int i = 0; i < DOVE_M; i++) {
        generate_Pi2(Pi2, seed_pk, i);
        matrix_dump(Pi2, pk_ptr);
        pk_ptr += DOVE_V * DOVE_O;
    }

    // 写 M 个 Pi3（上三角）
    for (int i = 0; i < DOVE_M; i++) {
        generate_Pi2(Pi2, seed_pk, i);

        int k1 = i / DOVE_L;
        int k2 = i % DOVE_L;

        matrix_mul(tmp1, Bk[k1], Ck[k2]);
        matrix_mul(tmp2, Tt, Pi2);
        matrix_add(Pi3, tmp1, tmp2);
        matrix_upper(Pi3);
        matrix_dump_upper(Pi3, pk_ptr);
        pk_ptr += UPPER_SIZE(DOVE_O);
    }

    // ============= 组装私钥 sk = (seed_sk | seed_pk | T | L*Ak | L*Bk | L*Ck | M*Pi2) =============
    uint8_t *sk_ptr = sk;
    memcpy(sk_ptr, seed_sk, DOVE_SK_SEED_BYTES);
    sk_ptr += DOVE_SK_SEED_BYTES;
    memcpy(sk_ptr, seed_pk, DOVE_PK_SEED_BYTES);
    sk_ptr += DOVE_PK_SEED_BYTES;
    matrix_dump(T, sk_ptr);
    sk_ptr += DOVE_V * DOVE_O;

    // 写 L 个 Ak（上三角紧凑）
    for (int k = 0; k < DOVE_L; k++) {
        matrix_dump_upper(Ak[k], sk_ptr);
        sk_ptr += AK_COMPACT_BYTES;
    }
    // 写 L 个 Bk
    for (int k = 0; k < DOVE_L; k++) {
        matrix_dump(Bk[k], sk_ptr);
        sk_ptr += DOVE_O * DOVE_V;
    }
    // 写 L 个 Ck
    for (int k = 0; k < DOVE_L; k++) {
        matrix_dump(Ck[k], sk_ptr);
        sk_ptr += DOVE_V * DOVE_O;
    }
    // 写 M 个 Pi2
    for (int i = 0; i < DOVE_M; i++) {
        generate_Pi2(Pi2, seed_pk, i);
        matrix_dump(Pi2, sk_ptr);
        sk_ptr += DOVE_V * DOVE_O;
    }

    // 释放资源
    matrix_free(T);
    matrix_free(Tt);
    matrix_free(Pi2);
    matrix_free(tmp1);
    matrix_free(tmp2);
    matrix_free(Pi3);
    for (int k = 0; k < DOVE_L; k++) {
        matrix_free(Ak[k]);
        matrix_free(Bk[k]);
        matrix_free(Ck[k]);
    }

    *pk_len_bytes = sig_get_pk_len_bytes();
    *sk_len_bytes = sig_get_sk_len_bytes();

    return 0;
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
    // 从 sk 中直接获取所有需要的预计算值
    const uint8_t *seed_sk  = sk;
    const uint8_t *seed_pk  = sk + SK_SEED_PK_OFFSET;
    const uint8_t *T_data   = sk + SK_T_OFFSET;
    const uint8_t *Ak_data  = sk + SK_AK_OFFSET;
    const uint8_t *Bk_data  = sk + SK_BK_OFFSET;
    const uint8_t *Ck_data  = sk + SK_CK_OFFSET;
    const uint8_t *Pi2_data = sk + SK_PI2_OFFSET;

    gf256_matrix_t *T = matrix_create(DOVE_V, DOVE_O);
    matrix_fill(T, T_data);

    // 计算消息哈希 t
    uint8_t t[DOVE_M];
    size_t input_len = m_len_bytes + DOVE_PK_SEED_BYTES;
    uint8_t *input = malloc(input_len);
    memcpy(input, m, m_len_bytes);
    memcpy(input + m_len_bytes, seed_pk, DOVE_PK_SEED_BYTES);
	pseudoXOF(DOVE_M*8,input, input_len*8, t);
    free(input);

    // 计算 salt
    uint8_t salt[DOVE_SALT_BYTES];
    input_len = DOVE_SK_SEED_BYTES + DOVE_M;
    input = malloc(input_len);
    memcpy(input, seed_sk, DOVE_SK_SEED_BYTES);
    memcpy(input + DOVE_SK_SEED_BYTES, t, DOVE_M);
	pseudoXOF(DOVE_SALT_BYTES*8,input, input_len*8, salt);
    free(input);

    gf256_matrix_t *Tt = matrix_create(DOVE_O, DOVE_V);
    matrix_transpose(Tt, T);

    int ret = -1;
    uint8_t v[DOVE_V];
    gf256_matrix_t *Ak[DOVE_L];
    uint8_t *xk[DOVE_L], *yk[DOVE_L];
    gf256_matrix_t *Bk[DOVE_L], *Ck[DOVE_L];
    gf256_matrix_t *Pi2 = matrix_create(DOVE_V, DOVE_O);

    for (int k = 0; k < DOVE_L; k++) {
        Ak[k] = matrix_create(DOVE_V, DOVE_V);
        matrix_fill_upper(Ak[k], Ak_data + (size_t)k * AK_COMPACT_BYTES);
        xk[k] = malloc(DOVE_V);
        yk[k] = malloc(DOVE_V);
        Bk[k] = matrix_create(DOVE_O, DOVE_V);
        matrix_fill(Bk[k], Bk_data + (size_t)k * DOVE_O * DOVE_V);
        Ck[k] = matrix_create(DOVE_V, DOVE_O);
        matrix_fill(Ck[k], Ck_data + (size_t)k * DOVE_V * DOVE_O);
    }

    gf256_matrix_t *L_mat = matrix_create(DOVE_M, DOVE_O);
    uint8_t w[DOVE_M], t_minus_w[DOVE_M], z[DOVE_M], s_v[DOVE_V];
    uint8_t term1[DOVE_O], term2[DOVE_O], term3[DOVE_O];

    for (int ctr = 0; ctr < 256; ctr++) {
        // 生成醋变量 v
        input_len = DOVE_M + DOVE_SK_SEED_BYTES;
        input = malloc(input_len);
        memcpy(input, t, DOVE_M);
        memcpy(input + DOVE_M, seed_sk, DOVE_SK_SEED_BYTES);
        pseudoXOFwithSeedAndCounter(DOVE_V,input, input_len,ctr,v);
        free(input);

        for (int k = 0; k < DOVE_L; k++) {
            row_vec_mul_matrix(xk[k], v, Ak[k]);
            matrix_mul_col_vec(yk[k], Ak[k], v);
        }

        // 构造线性系统 L * z = t - w
        memset(L_mat->data, 0, DOVE_M * DOVE_O);
        for (int i = 0; i < DOVE_M; i++) {
            // 从 sk 直接获取 Pi2，不再调用 generate_Pi2
            matrix_fill(Pi2, Pi2_data + (size_t)i * DOVE_V * DOVE_O);
            int k1 = i / DOVE_L;
            int k2 = i % DOVE_L;

            w[i] = gf256_dot(xk[k1], yk[k2], DOVE_V);

            row_vec_mul_matrix(term1, v, Pi2);
            row_vec_mul_matrix(term2, xk[k1], Ck[k2]);

            uint8_t tmp[DOVE_O];
            matrix_mul_col_vec(tmp, Bk[k1], yk[k2]);
            memcpy(term3, tmp, DOVE_O);

            uint8_t *row = &L_mat->data[i * DOVE_O];
            for (int j = 0; j < DOVE_O; j++) {
                row[j] = term1[j] ^ term2[j] ^ term3[j];
            }
        }

        // 计算 t - w（必须在 matrix_solve 之前完成）
        for (int i = 0; i < DOVE_M; i++) {
            t_minus_w[i] = t[i] ^ w[i];
        }

        // 求解线性系统 L * z = t - w
        if (matrix_solve(z, L_mat, t_minus_w) == 0) {

            // 构造签名 s = (v + Tz) || z
            uint8_t Tz[DOVE_V];
            matrix_mul_col_vec(Tz, T, z);
            for (int i = 0; i < DOVE_V; i++) {
                s_v[i] = v[i] ^ Tz[i];
            }

            uint8_t *sig_ptr = sn;
            memcpy(sig_ptr, s_v, DOVE_V);
            sig_ptr += DOVE_V;
            memcpy(sig_ptr, z, DOVE_M);
            sig_ptr += DOVE_M;
            memcpy(sig_ptr, salt, DOVE_SALT_BYTES);

            *sn_len_bytes = sig_get_sn_len_bytes();
            ret = 0;
            break;
        }
    }

    // 释放资源
    matrix_free(T);
    matrix_free(Tt);
    matrix_free(L_mat);
    matrix_free(Pi2);
    for (int k = 0; k < DOVE_L; k++) {
        matrix_free(Ak[k]);
        free(xk[k]);
        free(yk[k]);
        matrix_free(Bk[k]);
        matrix_free(Ck[k]);
    }
    return ret;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
    // 从 pk 中直接获取 seed_pk、L*Ak、M*Pi2、M*Pi3
    const uint8_t *seed_pk  = pk;
    const uint8_t *Ak_data  = pk + PK_AK_OFFSET;
    const uint8_t *Pi2_data = pk + PK_PI2_OFFSET;
    const uint8_t *pi3_data = pk + PK_PI3_OFFSET;

    const uint8_t *s = sn;
    const uint8_t *v = s;
    const uint8_t *z = s + DOVE_V;

    // 计算消息哈希 t
    uint8_t t[DOVE_M];
    size_t input_len = m_len_bytes + DOVE_PK_SEED_BYTES;
    uint8_t *input = malloc(input_len);
    memcpy(input, m, m_len_bytes);
    memcpy(input + m_len_bytes, seed_pk, DOVE_PK_SEED_BYTES);
	pseudoXOF(DOVE_M*8,input, input_len*8, t);
    free(input);

    gf256_matrix_t *Ak[DOVE_L];
    uint8_t *xk[DOVE_L], *yk[DOVE_L];
    for (int k = 0; k < DOVE_L; k++) {
        // 从 pk 直接获取 Ak，不再调用 generate_Ak
        Ak[k] = matrix_create(DOVE_V, DOVE_V);
        matrix_fill_upper(Ak[k], Ak_data + (size_t)k * AK_COMPACT_BYTES);
        xk[k] = malloc(DOVE_V);
        yk[k] = malloc(DOVE_V);
        row_vec_mul_matrix(xk[k], v, Ak[k]);
        matrix_mul_col_vec(yk[k], Ak[k], v);
    }

    gf256_matrix_t *Pi2 = matrix_create(DOVE_V, DOVE_O);
    gf256_matrix_t *Pi3 = matrix_create(DOVE_O, DOVE_O);
    uint8_t w[DOVE_M];

    for (int i = 0; i < DOVE_M; i++) {
        // 从 pk 直接获取 Pi2，不再调用 generate_Pi2
        matrix_fill(Pi2, Pi2_data + (size_t)i * DOVE_V * DOVE_O);
        matrix_fill_upper(Pi3, pi3_data + (size_t)i * UPPER_SIZE(DOVE_O));

        int k1 = i / DOVE_L;
        int k2 = i % DOVE_L;

        uint8_t term1 = gf256_dot(xk[k1], yk[k2], DOVE_V);

        uint8_t pi2_z[DOVE_V];
        matrix_mul_col_vec(pi2_z, Pi2, z);
        uint8_t term2 = gf256_dot(v, pi2_z, DOVE_V);

        uint8_t term3 = quadratic_form(z, Pi3);

        w[i] = term1 ^ term2 ^ term3;
    }

    // 验证二次型结果等于消息哈希 t
    int valid = 0;
    for (int i = 0; i < DOVE_M; i++) {
        if (w[i] != t[i]) {
            valid = -1;
            break;
        }
    }

    matrix_free(Pi2);
    matrix_free(Pi3);
    for (int k = 0; k < DOVE_L; k++) {
        matrix_free(Ak[k]);
        free(xk[k]);
        free(yk[k]);
    }
    return valid;
}


//   void sign_fprintlen(FILE *file_output, char *identifier, unsigned long long len)
// {
// 	if (OUTPUT_BLANK_TEST_VECTORS)
// 		fprintf(file_output, "%s\n", identifier);
// 	else
// 		fprintf(file_output, "%s%llu\n", identifier, len);
// }

//   void sign_fprintstr(FILE *file_output, char *identifier, unsigned char *msg, unsigned long long len)
// {
// 	fprintf(file_output, "%s", identifier);
// 	for (unsigned long long i = 0; i < len; i++)
// 		fprintf(file_output, "%02X", msg[i]);
// 	fprintf(file_output, "\n");
// }
