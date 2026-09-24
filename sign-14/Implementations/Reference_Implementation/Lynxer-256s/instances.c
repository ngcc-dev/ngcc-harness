/*
 *  SPDX-License-Identifier: MIT
 */

#include "instances.h"
#include "parameters.h"

const char* sig_get_param_name(sig_paramid_t paramid) {
  switch (paramid) {
  case PARAMETER_SET_INVALID:
    return "PARAMETER_SET_INVALID";
  case LYNXER_160S:
    return "LYNXER_160S";
  case LYNXER_160F:
    return "LYNXER_160F";
  case LYNXER_256S:
    return "LYNXER_256S";
  case LYNXER_256F:
    return "LYNXER_256F";
  case LYNXER_384S:
    return "LYNXER_384S";
  case LYNXER_384F:
    return "LYNXER_384F";
  case LYNXER_512S:
    return "LYNXER_512S";
  case LYNXER_512F:
    return "LYNXER_512F";
  default:
    return "PARAMETER_SET_MAX_INDEX";
  }
}

#define CALC_TAU1(name) ((name##_CSP - name##_POW_LEVEL) % name##_TAU)
#define CALC_TAU0(name) (name##_TAU - CALC_TAU1(name))
#define CALC_L(name)                                                                               \
  (CALC_TAU1(name) * (1 << CALC_K(name)) + CALC_TAU0(name) * (1 << (CALC_K(name) - 1)))
#define CALC_K(name) (((name##_CSP - name##_POW_LEVEL) / (name##_TAU)) + 1)

#define PARAMS(name)                                                                               \
  {                                                                                                \
      name##_CSP,                                                                               \
      name##_TAU,                                                                                  \
      name##_POW_LEVEL,                                                                              \
      name##_T_OPEN,                                                                               \
      name##_LENWIT,                                                                                  \
      CALC_K(name),                                                                                \
      CALC_TAU0(name),                                                                             \
      CALC_TAU1(name),                                                                             \
      CALC_L(name),                                                                                \
      name##_SIG_SIZE,                                                                             \
      name##_OWF_INPUT_SIZE,                                                                       \
      name##_OWF_OUTPUT_SIZE,                                                                      \
  }

#define LYNXER_160S_PARAMS PARAMS(LYNXER_160S)
#define LYNXER_160F_PARAMS PARAMS(LYNXER_160F)
#define LYNXER_256S_PARAMS PARAMS(LYNXER_256S)
#define LYNXER_256F_PARAMS PARAMS(LYNXER_256F)
#define LYNXER_384S_PARAMS PARAMS(LYNXER_384S)
#define LYNXER_384F_PARAMS PARAMS(LYNXER_384F)
#define LYNXER_512S_PARAMS PARAMS(LYNXER_512S)
#define LYNXER_512F_PARAMS PARAMS(LYNXER_512F)

#define CASE_PARAM(P)                                                                              \
  case P: {                                                                                        \
    static const sig_paramset_t params = P##_PARAMS;                                             \
    return &params;                                                                                \
  }

const sig_paramset_t* sig_get_paramset(sig_paramid_t paramid) {
  switch (paramid) {
    CASE_PARAM(LYNXER_160S)
    CASE_PARAM(LYNXER_160F)
    CASE_PARAM(LYNXER_256S)
    CASE_PARAM(LYNXER_256F)
    CASE_PARAM(LYNXER_384S)
    CASE_PARAM(LYNXER_384F)
    CASE_PARAM(LYNXER_512S)
    CASE_PARAM(LYNXER_512F)
  default:
    return NULL;
  }
}
