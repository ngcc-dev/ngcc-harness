#ifndef LOONG_AUGABIDULIN_H
#define LOONG_AUGABIDULIN_H

#include "rbc_elt.h"

typedef struct {
	const rbc_elt *g;
	unsigned int k;
	unsigned int n;
	unsigned int n_prime;
} augabidulin_code;

int augabidulin_code_init(augabidulin_code *code, const rbc_elt *g,
                          unsigned int k, unsigned int n,
                          unsigned int n_prime);
int augabidulin_code_encode(rbc_elt *c, unsigned int c_size,
                            const augabidulin_code *code, const rbc_elt *m,
                            unsigned int m_size);
int augabidulin_code_decode_no_error(rbc_elt *m, unsigned int m_size,
                                     const augabidulin_code *code,
                                     const rbc_elt *y, unsigned int y_size);
int augabidulin_code_decode(rbc_elt *m, unsigned int m_size,
                            const augabidulin_code *code, const rbc_elt *y,
                            unsigned int y_size, unsigned int epsilon);

#endif
