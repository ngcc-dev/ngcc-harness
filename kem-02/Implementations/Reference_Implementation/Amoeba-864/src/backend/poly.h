#ifndef POLYMUL_H
#define POLYMUL_H

#include "ntt_2s3t.h"
#include "params.h"

#ifdef __cplusplus
extern "C" {
#endif

#define K RLWE_K

extern int16_t NTTofY[288];

void ForwardNussbamuer(int16_t* out, const int16_t* in);

void InverseNussbamuer(int16_t* out, const int16_t* in);

void PolyMul(int16_t* r, const int16_t* f, const int16_t* g);

void PolyAdd(int16_t* r, const int16_t* f, const int16_t* g);

void PolySub(int16_t* r, const int16_t* f, const int16_t* g);

void PolyMod(int16_t* r, const int16_t* f);

void PolyClear(int16_t* f);

#ifdef __cplusplus
}
#endif

#endif  // POLYMUL_H