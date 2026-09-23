#ifndef FPRIME_H
#define FPRIME_H

#include "parameters.h"

#ifdef __cplusplus
extern "C"
{
#endif

  int32_t montgomery_mul32(int32_t a, int32_t b, int32_t prime, int32_t primeinv);
  int32_t add32(int32_t a, int32_t b, int32_t prime);
  int32_t sub32(int32_t a, int32_t b, int32_t prime);
  int32_t conditional_reduce32(int32_t r, int32_t prime);
  int32_t conditional_final_reduce32(int32_t r, int32_t prime);

#ifdef __cplusplus
}
#endif

#endif