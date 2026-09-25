#ifndef SIG_IMPL_H
#define SIG_IMPL_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

void vistrutith_sign_opt_d3_512f(const params_t* params, uint8_t* sig, const uint8_t* msg,
                                 size_t msglen, const uint8_t* owf_key,
                                 const uint8_t* owf_input, const uint8_t* owf_output,
                                 const uint8_t* witness, const uint8_t* rho, size_t rholen);

int vistrutith_verify_opt_d3_512f(const params_t* params, const uint8_t* msg, size_t msglen,
                                  const uint8_t* sig, const uint8_t* owf_input,
                                  const uint8_t* owf_output);

/* control verbosity of internal status prints (1 = on, 0 = off) */
void sig_set_verbose(int v);

#endif
