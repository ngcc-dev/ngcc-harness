/**
 * \file rbc_isometry.h
 * \brief Interface isometries over Fq^m
 */

#ifndef RBC_127_ISOMETRY_H
#define RBC_127_ISOMETRY_H

#include "rbc_127.h"
#include "rbc_elt.h"
#include "rbc_vec.h"
#include "rbc_mat_fq.h"
#include "seedexpander.h"

void rbc_isometry_init(rbc_isometry* o, uint32_t n);
void rbc_isometry_clear(rbc_isometry* o);
void rbc_isometry_set_zero(rbc_isometry *iso, uint32_t n);
void rbc_isometry_set(rbc_isometry *o, const rbc_mat_fq P, const rbc_mat_fq Q, uint32_t n);
void rbc_isometry_set_random(random_source* ctx, rbc_isometry *o, uint32_t n);
void rbc_isometry_set_random_from_xof(rbc_isometry *o,
                                      uint32_t n,
                                      void (*xof)(uint8_t *, size_t, const uint8_t *, size_t),
                                      const uint8_t *xof_input,
                                      uint32_t xof_size);
void rbc_isometry_apply(rbc_vec o, const rbc_vec v, uint32_t n, const rbc_isometry *iso);
void rbc_isometry_compose(rbc_isometry *o, const rbc_isometry *iso1, const rbc_isometry *iso2);
#endif

