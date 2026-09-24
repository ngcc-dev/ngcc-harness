#ifndef V8DAC_H
#define V8DAC_H

#include "v8ec.h"
#include "v8curve_extras.h"

typedef void (*v8xMUL_pointer_t)(v8ec_point_t* Q, v8ec_point_t const *P, v8ec_point_t const* A24);

extern const v8xMUL_pointer_t v8torsionscalar_func[];
extern const v8xMUL_pointer_t v8mscalar_func_s[];
extern const v8xMUL_pointer_t v8mscalar_func_t[];
extern const v8xMUL_pointer_t v8stra_scalar_func_s[];
extern const v8xMUL_pointer_t v8stra_scalar_func_t[];

#endif