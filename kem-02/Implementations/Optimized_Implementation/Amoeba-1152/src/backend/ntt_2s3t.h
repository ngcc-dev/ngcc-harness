#ifndef NTT_H
#define NTT_H

#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M RLWE_M
#define N RLWE_N
#define Q RLWE_Q
#define S 5
#define T 3

#define Pos2inv (int32_t)1729       //(Q+1)/2
#define Neg3inv (int32_t)1152       //(Q-1)/3
#define InvFactor (int32_t) - 2281  // MontR * (M/2)^{-1} mod Q

#define Root 9

#define MontR (int32_t)0x10000
#define MontMask (int32_t)0xffff  // R-1
#define MontRmodQ (int32_t)3310   // R%Q
#define MontQinv (int32_t)12929   // Q^{-1} mod R

#define BarrR (int32_t)0x10000  // 2^{16}
#define BarrF (int32_t)19       // round(R/Q)

#define montgomery_reduce(a) (((a) - (int16_t)((int16_t)(a) * MontQinv) * Q) >> 16)
#define barrett_reduce(a) ((a) - ((BarrF * (a)) >> 16) * Q)

extern int16_t forward_zetas[566];
extern int16_t inverse_zetas[566];

void ForwardNTT(int16_t* f);
void InverseNTT(int16_t* f);
void NTTMul(int16_t* r, const int16_t* a, const int16_t* b);
void NTTAdd(int16_t* r, const int16_t* a, const int16_t* b);
void NTTMod(int16_t* r, const int16_t* a);

#ifdef __cplusplus
}
#endif

#endif  // NTT_H