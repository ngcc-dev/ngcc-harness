#ifndef GAUSS_H
#define GAUSS_H

#include <stdbool.h>
#include <stdint.h>

#include "fq_arithmetic/mf3.h"
#include "fq_arithmetic/dp.h"

/**
 * 对矩阵进行高斯消去。
 *
 * transform、result、systematic 的 `*_alloc` 在这个函数内完成，不需要自行操作
 *
 * 各矩阵的大小：
 *   * mat (mat->n_row, mat->n_col)
 *   * transform (mat->n_row, mat->n_row)
 *   * result (mat->n_row, mat->n_col)
 *   * systematic (mat->n_col, mat->n_col)
 *
 * @param mat 需要进行高斯消去的矩阵
 * @param result 保存得到的行最简型矩阵
 * @param transform 保存一个矩阵，高斯消去的结果等效于在原矩阵上左乘这个矩阵，如果不需要的话可以设为 `NULL`
 * @param systematic 保存一个矩阵，在行最简型矩阵上右乘这个矩阵，得到的矩阵左侧为单位矩阵，如果不需要的话可以设为 `NULL`
 * @return 输入矩阵的秩
 */
size_t gauss_jordan_elimination(const mf3_e *mat, mf3_e **result, mf3_e **transform, dp_e **systematic);

/**
 * 使用高斯消去求矩阵的逆矩阵。
 *
 * @param inv 逆矩阵
 * @param mat 输入矩阵
 * @return `true` 表示存在逆矩阵，否则返回 `false`
 */
bool inverse_matrix(mf3_e *inv, const mf3_e *mat);

#endif