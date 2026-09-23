#ifndef BUNDLE_DAC_H
#define BUNDLE_DAC_H

#include "bundle_ec.h"
#include "bundle_curve_extras.h"

typedef void (*bundlexMUL_pointer_t)(bundleec_point_t* Q, bundleec_point_t const *P, bundleec_point_t const* A24, int len);

extern const bundlexMUL_pointer_t bundletorsionscalar_func[];
extern const bundlexMUL_pointer_t bundlemscalar_func_s[];
extern const bundlexMUL_pointer_t bundlemscalar_func_t[];
extern const bundlexMUL_pointer_t bundlestra_scalar_func_s[];
extern const bundlexMUL_pointer_t bundlestra_scalar_func_t[];

#endif