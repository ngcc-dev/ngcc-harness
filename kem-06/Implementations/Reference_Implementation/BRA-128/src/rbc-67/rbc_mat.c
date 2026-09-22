/**
 * \file rbc_mat.c
 * \brief Implementation of rbc_mat.h
 */

#include "rbc_mat.h"




/**
 * \fn void rbc_mat_init(rbc_mat* m, uint32_t rows, uint32_t columns)
 * \brief This function allocates the memory for a rbc_mat.
 *
 * \param[out] m Pointer to the allocated rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_init(rbc_mat* m, uint32_t rows, uint32_t columns) {
    *m = calloc(rows, sizeof(rbc_vec *));
    (*m)[0] = calloc(rows * columns, sizeof(rbc_elt));
    for(size_t i = 0; i < rows; ++i) {
        (*m)[i] = (*m)[0] + (i * columns);
    }
    if (m == NULL) exit(EXIT_FAILURE);
}




/**
 * \fn void rbc_mat_clear(rbc_mat m)
 * \brief This function clears a rbc_mat element.
 *
 * \param[out] m rbc_mat
 */
void rbc_mat_clear(rbc_mat m) {
    free(m[0]);
    free(m);
}



/**
 * \fn void rbc_mat_set_zero(rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief This function sets a matrix of finite elements to zero.
 *
 * \param[out] m rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_set_zero(rbc_mat m, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        rbc_vec_set_zero(m[i], columns);
    }
}



/**
 * \fn void rbc_mat_set(rbc_mat o, const rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief This function copies a matrix of finite field elements to another one.
 *
 * \param[out] o rbc_mat
 * \param[in] m rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_set(rbc_mat o, const rbc_mat m, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        rbc_vec_set(o[i], m[i], columns);
    }
}



/**
 * \fn void rbc_mat_set_random(random_source* ctx, rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief This function sets a matrix of finite field elements with random values using NGCC seed expander.
 *
 * \param[out] ctx NGCC seed expander
 * \param[out] m rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_set_random(random_source* ctx, rbc_mat m, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        rbc_vec_set_random(ctx, m[i], columns);
    }
}



/**
 * \fn void rbc_mat_set_random(seedexpander_shake* ctx, rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief This function sets a matrix of finite field elements with random values using NGCC seed expander.
 *
 * \param[out] ctx Seed expander
 * \param[out] m rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_set_random_tmp(seedexpander_shake_t* ctx, rbc_mat m, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        rbc_vec_set_random_tmp(ctx, m[i], columns);
    }
}



/**
 * \fn void rbc_mat_set_random_from_support(random_source* ctx, rbc_mat o, uint32_t rows, uint32_t columns, const rbc_vspace support, uint32_t support_size, uint8_t copy_flag)
 * \brief Sets a matrix with random entries whose support is included in the
 * one given as input, using the NGCC seed expander.
 *
 * Internally, this samples a length rows*columns vector via
 * rbc_vec_set_random_from_support and folds it into a rows x columns
 * matrix. When <b>copy_flag</b> is non-zero, the support is copied into
 * support_size random coordinates of the resulting flat vector to
 * guarantee that the matrix has full support.
 *
 * \param[out] ctx Random source
 * \param[out] o rbc_mat (rows x columns)
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 * \param[in] support Support to draw entries from
 * \param[in] support_size Size of the support
 * \param[in] copy_flag If not 0, the support is copied into random coordinates of the resulting matrix
 */
void rbc_mat_set_random_from_support(random_source* ctx, rbc_mat o, uint32_t rows, uint32_t columns, const rbc_vspace support, uint32_t support_size, uint8_t copy_flag) {
    uint32_t size = rows * columns;

    for(uint32_t i = 0 ; i < rows ; ++i) {
        for(uint32_t j = 0 ; j < columns ; ++j) {
            rbc_elt_set_zero(o[i][j]);
        }
    }

    if(copy_flag) {
        uint32_t random1_size = 4 * support_size;
        uint8_t random1[random1_size];
        random_source_get_bytes(ctx, random1, random1_size);

        uint32_t i = 0;
        uint32_t j = 0;
        uint32_t cutoff = 65536u - (65536u % size);
        while(i != support_size) {
            uint32_t position = ((uint32_t)random1[j]) | (((uint32_t)random1[j + 1]) << 8);
            j += 2;
            if(j + 1 >= random1_size) {
                random_source_get_bytes(ctx, random1, random1_size);
                j = 0;
            }
            if(position >= cutoff) continue;
            position = position % size;
            uint32_t r = position / columns;
            uint32_t c = position % columns;
            if(rbc_elt_is_zero(o[r][c])) {
                rbc_elt_set(o[r][c], support[i]);
                ++i;
            }
        }

        uint32_t random2_size = (support_size * (size - support_size)) / 8 + 1;
        uint8_t random2[random2_size];
        random_source_get_bytes(ctx, random2, random2_size);

        uint32_t k = 0;
        uint32_t l = 0;
        for(uint32_t r = 0 ; r < rows ; ++r) {
            for(uint32_t c = 0 ; c < columns ; ++c) {
                if(rbc_elt_is_zero(o[r][c])) {
                    for(uint32_t s = 0 ; s < support_size ; ++s) {
                        if(random2[k] & 0x1) {
                            rbc_elt_add(o[r][c], support[s], o[r][c]);
                        }
                        random2[k] >>= 1;
                        if(++l == 8) {
                            l = 0;
                            ++k;
                        }
                    }
                }
            }
        }
    }
    else {
        uint32_t random_size = support_size * size / 8 + 1;
        uint8_t random[random_size];
        random_source_get_bytes(ctx, random, random_size);

        uint32_t k = 0;
        uint32_t l = 0;
        for(uint32_t r = 0 ; r < rows ; ++r) {
            for(uint32_t c = 0 ; c < columns ; ++c) {
                for(uint32_t s = 0 ; s < support_size ; ++s) {
                    if(random[k] & 0x1) {
                        rbc_elt_add(o[r][c], o[r][c], support[s]);
                    }
                    random[k] >>= 1;
                    if(++l == 8) {
                        l = 0;
                        ++k;
                    }
                }
            }
        }
    }
}




/**
 * \fn void rbc_mat_add(rbc_mat o, const rbc_mat m1, const rbc_mat m2, uint32_t rows, uint32_t columns)
 * \brief This functions adds matrices of finite field elements.
 *
 * \param[out] o rbc_mat equal to \f$ m1 \oplus m2 \f$
 * \param[in] m1 rbc_mat
 * \param[in] m2 rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_add(rbc_mat o, const rbc_mat m1, const rbc_mat m2, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        for(size_t j = 0 ; j < columns ; ++j) {
            rbc_elt_add(o[i][j], m1[i][j], m2[i][j]);
        }
    }
}



/**
 * \fn void rbc_mat_mul(rbc_mat o, const rbc_mat m1, const rbc_mat m2, uint32_t rows1, uint32_t columns1_rows2, uint32_t columns2)
 * \brief This functions multiplies matrices of finite field elements.
 *
 * \param[out] o rbc_mat equal to \f$ m1 \times m2 \f$
 * \param[in] m1 rbc_mat
 * \param[in] m2 rbc_mat
 * \param[in] rows1 Row size of m1
 * \param[in] columns1_rows2 Column and row sizes of m1 and m2, respectively
 * \param[in] columns2 Column size of m2
 */
void rbc_mat_mul(rbc_mat o, const rbc_mat m1, const rbc_mat m2, uint32_t rows1, uint32_t columns1_rows2, uint32_t columns2) {
    rbc_elt tmp, acc;
    rbc_mat_set_zero(o, rows1, columns2);
    for(size_t i = 0 ; i < rows1 ; ++i) {
        for(size_t j = 0 ; j < columns2 ; ++j) {
            rbc_elt_set_zero(acc);
            for(size_t k = 0 ; k < columns1_rows2 ; ++k) {
                rbc_elt_mul(tmp, m1[i][k], m2[k][j]);
                rbc_elt_add(acc, acc, tmp);
            }
            rbc_elt_set(o[i][j], acc);
        }
    }
}



/**
 * \fn void rbc_mat_vec_mul(rbc_vec o, const rbc_mat m, const rbc_vec v, uint32_t rows, uint32_t columns)
 * \brief This functions multiplies a matrix of finite field elements by a vector.
 *
 * \param[out] o rbc_vec equal to \f$ m \times v \f$
 * \param[in] m rbc_mat
 * \param[in] v rbc_vec
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat, and Row size of the rbd_vec
 */
void rbc_mat_vec_mul(rbc_vec o, const rbc_mat m, const rbc_vec v, uint32_t rows, uint32_t columns) {
    rbc_elt tmp, acc;
    for(size_t i = 0 ; i < rows ; ++i) {
        rbc_elt_set_zero(acc);
        for(size_t j = 0 ; j < columns ; ++j) {
            rbc_elt_mul(tmp, m[i][j], v[j]);
            rbc_elt_add(acc, acc, tmp);
        }
        rbc_elt_set(o[i], acc);
    }
}



/**
 * \fn void rbc_vec_mat_mul(rbc_vec o, const rbc_mat m, const rbc_vec v, uint32_t rows, uint32_t columns)
 * \brief This functions multiplies a matrix of finite field elements by a vector.
 *
 * \param[out] o rbc_vec equal to \f$ v \times m \f$
 * \param[in] m rbc_mat
 * \param[in] v rbc_vec
 * \param[in] rows Row size of the rbc_mat, and Row size of the rbd_vec
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_vec_mat_mul(rbc_vec o, const rbc_mat m, const rbc_vec v, uint32_t rows, uint32_t columns) {
    rbc_elt tmp, acc;
    for(size_t i = 0 ; i < columns ; ++i) {
        rbc_elt_set_zero(acc);
        for(size_t j = 0 ; j < rows ; ++j) {
            rbc_elt_mul(tmp, m[j][i], v[j]);
            rbc_elt_add(acc, acc, tmp);
        }
        rbc_elt_set(o[i], acc);
    }
}



/**
 * \fn void rbc_mat_trans(rbc_mat o, const rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief This function transposes a matrix of finite field elements.
 *
 * The output matrix <b>o</b> has dimensions <b>columns</b> x <b>rows</b>
 * (i.e. the transpose of the <b>rows</b> x <b>columns</b> input matrix).
 *
 * \param[out] o rbc_mat (columns x rows) equal to the transpose of <b>m</b>
 * \param[in] m rbc_mat (rows x columns) to transpose
 * \param[in] rows Row size of the input <b>m</b>
 * \param[in] columns Column size of the input <b>m</b>
 */
void rbc_mat_trans(rbc_mat o, const rbc_mat m, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        for(size_t j = 0 ; j < columns ; ++j) {
            rbc_elt_set(o[j][i], m[i][j]);
        }
    }
}




/**
 * \fn void rbc_mat_fold(rbc_mat o, const rbc_vec v, uint32_t rows, uint32_t columns)
 * \brief Reshape a length rows*columns vector into a rows x columns matrix.
 *
 * Entry (i, j) of the output matrix is set to v[i * columns + j].
 *
 * \param[out] o rbc_mat (rows x columns)
 * \param[in] v rbc_vec of length rows * columns
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_fold(rbc_mat o, const rbc_vec v, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        for(size_t j = 0 ; j < columns ; ++j) {
            rbc_elt_set(o[i][j], v[i * columns + j]);
        }
    }
}




/**
 * \fn void rbc_mat_unfold(rbc_vec o, const rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief Flatten a rows x columns matrix into a length rows*columns vector.
 *
 * Entry i*columns + j of the output vector is set to m[i][j], i.e. the
 * inverse of rbc_mat_fold.
 *
 * \param[out] o rbc_vec of length rows * columns
 * \param[in] m rbc_mat (rows x columns)
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_unfold(rbc_vec o, const rbc_mat m, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        for(size_t j = 0 ; j < columns ; ++j) {
            rbc_elt_set(o[i * columns + j], m[i][j]);
        }
    }
}




/**
 * \fn uint8_t rbc_mat_is_equal_to(const rbc_mat m1, const rbc_mat m2, uint32_t rows, uint32_t columns)
 * \brief This function tests the equality of two matrices of finite field elements.
 *
 * \param[in] m1 rbc_mat
 * \param[in] m2 rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 * \return 1 if m1 == m2, 0 otherwise
 */
uint8_t rbc_mat_is_equal_to(const rbc_mat m1, const rbc_mat m2, uint32_t rows, uint32_t columns) {
    for(size_t i = 0 ; i < rows ; ++i) {
        for(size_t j = 0 ; j < columns ; ++j) {
            if(rbc_elt_is_equal_to(m1[i][j], m2[i][j]) == 0) {
                return 0;
            }
        }
    }
    return 1;
}




/**
 * \fn void rbc_mat_to_string(uint8_t* str, const rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief This function parses a matrix of finite field elements into a string.
 *
 * \param[out] str Output string
 * \param[in] m rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_to_string(uint8_t* str, const rbc_mat m, uint32_t rows, uint32_t columns) {
  rbc_vec t;
  rbc_vec_init(&t, rows * columns);
  for(size_t i = 0 ; i < rows ; i++) {
    for(size_t j = 0 ; j < columns ; j++) {
      rbc_elt_set(t[i * columns + j], m[i][j]);
    }
  }
  rbc_vec_to_string(str, t, rows * columns);
  rbc_vec_clear(t);
}



/**
 * \fn void rbc_mat_from_string(rbc_mat m, uint32_t rows, uint32_t columns, const uint8_t* str)
 * \brief This function parses a string into a matrix of finite field elements.
 *
 * \param[out] m rbc_mat
 * \param[in] size Size of the matrix
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_from_string(rbc_mat m, uint32_t rows, uint32_t columns, const uint8_t* str) {
  rbc_vec t;
  rbc_vec_init(&t, rows * columns);
  rbc_vec_from_string(t, rows * columns, str);
  for(size_t i = 0 ; i < rows ; i++) {
    for(size_t j = 0 ; j < columns ; j++) {
      rbc_elt_set(m[i][j], t[i * columns + j]);
    }
  }
  rbc_vec_clear(t);
}



/**
 * \fn void rbc_mat_print(rbc_mat m, uint32_t rows, uint32_t columns)
 * \brief Display an rbc_mat element.
 *
 * \param[out] m rbc_mat
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_print(rbc_mat m, uint32_t rows, uint32_t columns) {
    printf("[\n");
    for(size_t i = 0 ; i < rows ; ++i) {
        printf("[\t");
        for(size_t j = 0 ; j < columns ; ++j) {
            rbc_elt_print(m[i][j]);
        }
        printf("\t]\n");
    }
    printf("]\n");
}
