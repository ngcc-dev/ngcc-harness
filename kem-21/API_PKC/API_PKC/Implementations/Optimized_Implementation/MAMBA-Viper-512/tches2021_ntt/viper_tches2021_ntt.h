#ifndef VIPER_TCHES2021_NTT_H
#define VIPER_TCHES2021_NTT_H

#include "../viper.h"

void poly_mul_tches2021_ntt_avx(vpoly c, const vpoly a, const vpoly b);
void matvec_tches2021_ntt_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void matTvec_tches2021_ntt_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void dot_tches2021_ntt_avx(vpoly out, const vpolyvec a, const vpolyvec b);

#endif
