#ifndef DAC_H
#define DAC_H

#include "ec.h"
#include "curve_extras.h"

typedef void (*xMUL_pointer_t)(ec_point_t* Q, ec_point_t const *P, ec_point_t const* A24);

extern const xMUL_pointer_t torsionscalar_func[];
extern const xMUL_pointer_t mscalar_func_s[];
extern const xMUL_pointer_t mscalar_func_t[];
extern const xMUL_pointer_t stra_scalar_func_s[];
extern const xMUL_pointer_t stra_scalar_func_t[];

#endif