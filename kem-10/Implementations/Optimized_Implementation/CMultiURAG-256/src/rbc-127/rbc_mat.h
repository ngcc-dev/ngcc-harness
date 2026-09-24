/**
 * \file rbc_mat.h
 * \brief Interface for matrices over Fq^m
 */

#ifndef RBC_127_MAT_H
#define RBC_127_MAT_H

#include "rbc_127.h"
#include "rbc_elt.h"
#include "rbc_vec.h"
#include "seedexpander_shake.h"

void rbc_mat_init(rbc_mat* m, uint32_t rows, uint32_t columns);
void rbc_mat_clear(rbc_mat m);
void rbc_mat_set_zero(rbc_mat m, uint32_t rows, uint32_t columns);
void rbc_mat_set(rbc_mat o, const rbc_mat m, uint32_t rows, uint32_t columns);
void rbc_mat_set_random(random_source* ctx, rbc_mat o, uint32_t rows, uint32_t columns);
void rbc_mat_set_random_tmp(seedexpander_shake_t* ctx, rbc_mat o, uint32_t rows, uint32_t columns);
void rbc_mat_set_random_from_support(random_source* ctx, rbc_mat o, uint32_t rows, uint32_t columns, const rbc_vspace support, uint32_t support_size, uint8_t copy_flag);
void rbc_mat_add(rbc_mat o, const rbc_mat m1, const rbc_mat m2, uint32_t rows, uint32_t columns);
void rbc_mat_mul(rbc_mat o, const rbc_mat m1, const rbc_mat m2, uint32_t rows1, uint32_t columns1_rows2, uint32_t columns2);
void rbc_mat_vec_mul(rbc_vec o, const rbc_mat m, const rbc_vec v, uint32_t rows, uint32_t columns);
void rbc_vec_mat_mul(rbc_vec o, const rbc_mat m, const rbc_vec v, uint32_t rows, uint32_t columns);
void rbc_mat_trans(rbc_mat o, const rbc_mat m, uint32_t rows, uint32_t columns);
void rbc_mat_fold(rbc_mat o, const rbc_vec v, uint32_t rows, uint32_t columns);
void rbc_mat_unfold(rbc_vec o, const rbc_mat m, uint32_t rows, uint32_t columns);
uint8_t rbc_mat_is_equal_to(const rbc_mat m1, const rbc_mat m2, uint32_t rows, uint32_t columns);
void rbc_mat_to_string(uint8_t* str, const rbc_mat m, uint32_t rows, uint32_t columns);
void rbc_mat_from_string(rbc_mat m, uint32_t rows, uint32_t columns, const uint8_t* str);
void rbc_mat_print(const rbc_mat m, uint32_t rows, uint32_t columns);
#endif

