#include "gauss.h"
#include "macros.h"

size_t gauss_jordan_elimination(mf3_e const *mat, mf3_e **result, mf3_e **transform, dp_e **systematic) {
    *result = mf3_copy(mat);
    if (transform) {
        *transform = mf3_alloc(mat->n_row, mat->n_row);
        mf3_set_to_identity(*transform);
    }
    if (systematic) *systematic = dp_alloc(mat->n_col);

    size_t pivot[] = { 0, 0 };
    size_t rank = 0;

    while (pivot[0] < mat->n_row && pivot[1] < mat->n_col) {
        // 查找当前列下一个非零值所在的那一行
        size_t nonzero_index_in_col = SIZE_MAX;
        for (size_t i = pivot[0]; i < mat->n_row; i++) {
            if (mf3_coeff(*result, i, pivot[1])) {
                nonzero_index_in_col = i;
                break;
            }
        }
        // DBG(
        //     "non-zero index in col %lld from row %lld to %lld: %lld",
        //     pivot[1],
        //     pivot[0],
        //     mat->n_row - 1,
        //     nonzero_index_in_col
        // );
        // 如果当前列全是0则直接处理下一列
        if (nonzero_index_in_col == SIZE_MAX) {
            pivot[1]++;
            continue;
        }

        // 当前行与非零值所在那一行互换
        if (pivot[0] != nonzero_index_in_col) {
            if (transform) mf3_swap_rows(*transform, pivot[0], nonzero_index_in_col);
            mf3_swap_rows(*result, pivot[0], nonzero_index_in_col);
        }
        // 当前行除以pivot指向的值，使pivot为1
        if (mf3_coeff(*result, pivot[0], pivot[1]) != 1) {
            const uint8_t inv = mf3_coeff(*result, pivot[0], pivot[1]);
            if (transform) vf3_vector_scalarmul_nonzero_inplace(&(*transform)->rows[pivot[0]], inv);
            vf3_vector_scalarmul_nonzero_inplace(&(*result)->rows[pivot[0]], inv);
        }
        // DBG("set1\npivot: (%lld, %lld) step: ", pivot[0], pivot[1]);
        // mf3_print(*result);

        // 其他行减去当前行的倍数，使其他行的这一列为0
        vf3_e *transform_row = transform ? vf3_alloc(mat->n_row) : NULL;
        vf3_e *result_row = vf3_alloc(mat->n_col);
        for (size_t i = 0; i < mat->n_row; i++) {
            if (i == pivot[0]) continue;
            const uint8_t scalar = mf3_coeff(*result, i, pivot[1]);
            if (transform) {
                vf3_vector_scalarmul(transform_row, scalar, &(*transform)->rows[pivot[0]]);
                vf3_vector_sub_inplace(&(*transform)->rows[i], transform_row);
            }
            vf3_vector_scalarmul(result_row, scalar, &(*result)->rows[pivot[0]]);
            vf3_vector_sub_inplace(&(*result)->rows[i], result_row);
        }
        free(result_row);
        if (transform) free(transform_row);
        // DBG("other0\npivot: (%lld, %lld) step: ", pivot[0], pivot[1]);
        // mf3_print(*result);

        // 准备处理下一列
        if (systematic && pivot[0] != pivot[1]) {
            dp_swap_columns(*systematic, pivot[0], pivot[1]);
        }
        rank++;
        pivot[0]++;
        pivot[1]++;
    }

    return rank;
}

bool inverse_matrix(mf3_e *inv, mf3_e const *mat) {
    if (mat->n_row != mat->n_col) {
        return false;
        // PANIC("Cannot inverse, matrix shape mismatch (%lld, %lld)", mat->n_row, mat->n_col);
    }
    // DBG("inverse_matrix size = %lu", mat->n_row);
    // TIMING_START("inverse_matrix");
    // 在原矩阵右边增加一个单位矩阵
    mf3_e *extend = mf3_alloc(mat->n_row, mat->n_col * 2);
    for (size_t i = 0; i < mat->n_row; i++) {
        vf3_vector_cat_zero(&extend->rows[i], &mat->rows[i]);
        vf3_set_coeff(mat->n_col + i, &extend->rows[i], 1);
    }
    mf3_e *result;
    gauss_jordan_elimination(extend, &result, NULL, NULL);
    // 消去后判断左半边是否为单位矩阵，是的话取右半部分为逆矩阵
    // 如果不是则矩阵不是满秩的，不能求逆
    if (mf3_is_identity_loose(result)) {
        vf3_e *dummy = vf3_alloc(mat->n_col);
        for (size_t i = 0; i < mat->n_row; i++) {
            vf3_vector_split(dummy, &inv->rows[i], &result->rows[i]);
        }
        vf3_free(dummy);
        mf3_free(result);
        // TIMING_END();
        return true;
    } else {
        mf3_free(result);
        // TIMING_END();
        return false;
    }
}