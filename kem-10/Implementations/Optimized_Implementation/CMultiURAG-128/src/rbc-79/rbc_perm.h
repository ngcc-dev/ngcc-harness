/**
 * \file rbc_perm.h
 * \brief Interface for permutation matrices over Fq
 */

#ifndef RBC_79_PERM_H
#define RBC_79_PERM_H

#include "rbc_79.h"
#include "rbc_elt.h"
#include "rbc_vec.h"
#include "rbc_mat_fq.h"

void rbc_perm_init(rbc_perm* perm, uint32_t size);
void rbc_perm_clear(rbc_perm m);
void rbc_perm_set_zero(rbc_perm o, uint32_t size);
void rbc_perm_set(rbc_perm o, const rbc_perm perm, uint32_t size);
void rbc_perm_set_random(random_source* ctx, rbc_perm perm, uint32_t size);
void rbc_perm_set_random_from_xof(rbc_perm perm, uint32_t size, void (*xof)(uint8_t *, size_t, const uint8_t *, size_t), const uint8_t *xof_input, uint32_t xof_size);
void rbc_perm_apply(rbc_mat_fq o, const rbc_mat_fq m, const rbc_perm perm, uint32_t size);
#endif

