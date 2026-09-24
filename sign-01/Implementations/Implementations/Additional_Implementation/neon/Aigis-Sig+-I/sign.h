#ifndef SIGN_H
#define SIGN_H

#include "params.h"
#include "poly.h"
#include "polyvec.h"

int32_t msig_keygen(uint8_t *pk, uint8_t *sk);

int32_t msig_sign(uint8_t *sk,
	uint8_t *m, int32_t mlen,
	uint8_t *sm, int32_t *smlen);

int32_t msig_verf(uint8_t *pk,
	uint8_t *sm, int32_t smlen,
	uint8_t *m, int32_t mlen);

#endif
