#ifndef DP_H
#define DP_H

#include <stdio.h>
#include <stdbool.h>
#include "types_f3.h"

/**
 * 表示一个置换对角矩阵。
 * `p[i] = j` 表示第 i 行第 j 列的元素为 `d[i]。`
 * 需要自行保证 `p[d->size]` 只包含 0 到 `d->size - 1` 且不重复的元素。
 */
typedef struct {
    /** 对角（每一行）上的元素 */
    vf3_e *d;
    /** 置换位置，从 0 开始 */
    size_t *p;
} dp_e;

/**
 * 初始化一个置换对角矩阵，元素均为 1，置换为恒等置换。
 * @param dp 需要初始化的置换对角矩阵
 * @param length 置换对角矩阵的大小
 */
void dp_init(dp_e *dp, const size_t length);

/**
 * 使用 dp_init 返回一个初始化的置换对角矩阵，元素均为 1，置换为恒等置换
 * @param length 置换对角矩阵的大小
 */
dp_e *dp_alloc(const size_t length);

/**
 * 释放一个置换对角矩阵。
 * @param dp 置换对角矩阵
 */
void dp_free(dp_e *dp);

/**
 * 设置置换对角矩阵的对角上的元素。
 * @param dp 置换对角矩阵
 * @param data 对角上的元素 {0,1,2}，长度和 `dp->d->size` 相同
 */
void dp_set_diag(dp_e *dp, const uint8_t *data);

/**
 * 设置置换对角矩阵的置换位置。
 * @param dp 置换对角矩阵
 * @param data 置换位置，长度和 `dp->d->size` 相同
 */
void dp_set_perm(dp_e *dp, const size_t *data);

/**
 * 输出一个置换对角矩阵
 * @param dp 置换对角矩阵
 */
void dp_print(dp_e *dp);

/**
 * 将置换对角矩阵的元素设为随机值。
 * @param dp 置换对角矩阵
 */
void dp_random(dp_e *dp);

/**
 * 将置换对角矩阵的元素设为非 0 随机值。
 * @param dp 置换对角矩阵
 */
void dp_random_non_zero(dp_e *dp);

/**
 * 将置换对角矩阵的置换位置随机打乱。
 * @param dp 置换对角矩阵
 */
void dp_shuffle(dp_e *dp);

/**
 * 判断两个置换对角矩阵是否相等。
 * @param a 置换对角矩阵
 * @param b 置换对角矩阵
 * @return `true` 表示相等，`false` 表示不相等。
 */
bool dp_equal(const dp_e *a, const dp_e *b);

/**
 * 将两个置换对角矩阵相乘（行变换）：c = a b
 * 需要保证 `a->d->size = b->d->size = c->d->size`
 * @param c 表示计算结果的置换对角矩阵
 * @param a 置换对角矩阵
 * @param b 置换对角矩阵
 */
void dp_product(dp_e *c, const dp_e *a, const dp_e *b);

/**
 * 将置换对角矩阵左乘到一个矩阵上（行变换）：r = dp m
 * 需要保证 `r->n_row = m->n_row = dp->d->size` 和 `r->n_col = m->n_col`
 * @param r 表示计算结果的矩阵
 * @param dp 置换对角矩阵
 * @param m 矩阵
 */
void dp_left_product_matrix(mf3_e *r, const dp_e *dp, const mf3_e *m);

/**
 * 将置换对角矩阵右乘到一个矩阵上（列变换）：r = m dp
 * 需要保证 `r->n_row = m->n_row` 和 `r->n_col = m->n_col = dp->d->size`
 * @param r 表示计算结果的矩阵
 * @param dp 置换对角矩阵
 * @param m 矩阵
 */
void dp_right_product_matrix(mf3_e *r, const dp_e *dp, const mf3_e *m);

/**
 * 将置换对角矩阵右乘到一个向量上（列变换）：y = x dp
 * 需要保证 `y->size = x->size = dp->d->size`
 * @param r 表示计算结果的向量
 * @param dp 置换对角矩阵
 * @param m 向量
 */
void dp_right_product_vector(vf3_e *y, const dp_e *dp, const vf3_e *x);

/**
 * 交换置换对角矩阵的两行
 *
 * @param dp 置换对角矩阵
 * @param i 交换第 i 行和第 j 行
 * @param j 交换第 i 行和第 j 行
 */
void dp_swap_rows(dp_e *dp, const size_t i, const size_t j);

/**
 * 交换置换对角矩阵的两列
 *
 * @param dp 置换对角矩阵
 * @param i 交换第 i 列和第 j 列
 * @param j 交换第 i 列和第 j 列
 */
void dp_swap_columns(dp_e *dp, const size_t i, const size_t j);

/**
 * 将置换对角矩阵转置
 * 需要保证 `dpt->d->size = dp->d->size`
 *
 * @param dpt 转置后的置换对角矩阵
 * @param dp 置换对角矩阵
 */
void dp_transpose_and_copy(dp_e *dpt, const dp_e *dp);

/**
 * 从文件读取置换对角矩阵
 *
 * @param dp 置换对角矩阵
 * @param f 文件
 * @return 读取的大小
 */
size_t dp_read(dp_e *dp, FILE *f);

/**
 * 将置换对角矩阵写入到文件
 *
 * @param dp 置换对角矩阵
 * @param f 文件
 * @return 写入的大小
 */
size_t dp_write(dp_e *dp, FILE *f);

#endif
