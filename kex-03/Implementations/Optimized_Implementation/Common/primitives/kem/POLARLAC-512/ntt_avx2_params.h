#ifndef POLARLAC_NTT_AVX2_PARAMS_H
#define POLARLAC_NTT_AVX2_PARAMS_H

#include <stdint.h>
#include "params.h"

#if RL_KEM_Q != 257 || RL_KEM_N != 1024
#error "ntt_avx2_params.h is specialized for RL_KEM_Q=257 and RL_KEM_N=1024."
#endif

#define INVERSE_N 255
#define INVERSE_N_BETA 255
#define INVERSE_N_BETA2 255
#define INVERSE_Q (-255)
#define Normal 225
#define B_Q 1

static int16_t __attribute__((aligned(64))) poly_basemul_f2[(RL_KEM_N / 8)]={
3,209
,-3,-209
,192,12
,-192,-12
,233,127
,-233,-127
,6,161
,-6,-161
,151,154
,-151,-154
,155,90
,-155,-90
,77,53
,-77,-53
,45,51
,-45,-51
,243,224
,-243,-224
,132,201
,-132,-201
,112,7
,-112,-7
,229,191
,-229,-191
,152,138
,-152,-138
,219,94
,-219,-94
,69,181
,-69,-181
,47,19
,-47,-19
,27,82
,-27,-82
,186,108
,-186,-108
,41,115
,-41,-115
,54,164
,-54,-164
,74,101
,-74,-101
,110,39
,-110,-39
,179,220
,-179,-220
,148,202
,-148,-202
,131,217
,-131,-217
,160,10
,-160,-10
,237,63
,-237,-63
,5,177
,-5,-177
,83,214
,-83,-214
,172,75
,-172,-75
,107,87
,-107,-87
,166,171
,-166,-171
};

#endif
