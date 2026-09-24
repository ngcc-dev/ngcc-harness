#ifndef ROUNDING_H
#define ROUNDING_H

#include <stdint.h>
#include "params.h"

#if COMPASS_SIG_MODE == 128
  #define DECOMPOSE_M  264143
  #define DECOMPOSE_A  270848
  #define DECOMPOSE_S  36
#elif COMPASS_SIG_MODE == 256
  #define DECOMPOSE_M  264143
  #define DECOMPOSE_A  270848
  #define DECOMPOSE_S  38
#elif COMPASS_SIG_MODE == 384
  #define DECOMPOSE_M  1049601
  #define DECOMPOSE_A  8192
  #define DECOMPOSE_S  41
#elif COMPASS_SIG_MODE == 512
  #define DECOMPOSE_M  1049601
  #define DECOMPOSE_A  8192
  #define DECOMPOSE_S  42
#endif

#define power2round COMPASS_SIG_NAMESPACE(power2round)
int32_t power2round(int32_t *a0, int32_t a);

#define decompose COMPASS_SIG_NAMESPACE(decompose)
int32_t decompose(int32_t *a0, int32_t a);

#endif
