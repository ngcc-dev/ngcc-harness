#include "symmetric.h"
#include "bits-hints.h"
#include "conversion.h"

void SampleInBall(poly* c, int32_t* coordinates, const uint8_t rho[HBYTES]);

void ExpandA(poly A[k][l], const uint8_t rho[SEEDBYTES]);

void ExpandS(poly* s0, poly* e, const uint8_t rho[HBYTES]);

void ExpandMask(poly* y, const uint8_t rho[SEEDBYTES], uint16_t mu);

int SampleSign(poly* y, poly* c, int32_t* coordinates, const poly* s, const uint8_t seed[], uint16_t mu);
