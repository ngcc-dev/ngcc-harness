#ifndef LOONG_SUPPORT_H
#define LOONG_SUPPORT_H

#include "loong_xof_reader.h"
#include "rbc_elt.h"

int loong_support_sample_pair(rbc_elt *v1, unsigned int v1_rank,
                              rbc_elt *v2, unsigned int v2_rank,
                              unsigned int intersection_dim,
                              int require_one_in_v2,
                              loong_xof_reader *reader);
int loong_support_span_contains_one(const rbc_elt *basis, unsigned int size);

#endif
