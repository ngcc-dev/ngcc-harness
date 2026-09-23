#ifndef LOONG_GABIDULIN_H
#define LOONG_GABIDULIN_H

#include "rbc_elt.h"
#include "qpoly.h"

typedef struct {
	const rbc_elt *g;
	unsigned int k;
	unsigned int n;
} gabidulin_code;

int gabidulin_code_init(gabidulin_code *code, const rbc_elt *g,
                        unsigned int k, unsigned int n);
int gabidulin_code_encode(rbc_elt *c, unsigned int c_size,
                          const gabidulin_code *code, const rbc_elt *m,
                          unsigned int m_size);
int gabidulin_code_decode_no_error(rbc_elt *m, unsigned int m_size,
                                   const gabidulin_code *code,
                                   const rbc_elt *y, unsigned int y_size);
int gabidulin_code_decode_with_annihilator(rbc_elt *m, unsigned int m_size,
                                           const gabidulin_code *code,
                                           const rbc_elt *y,
                                           unsigned int y_size,
                                           const qpoly *annihilator);
int gabidulin_code_decode(rbc_elt *m, unsigned int m_size,
                          const gabidulin_code *code, const rbc_elt *y,
                          unsigned int y_size);

#endif
