/*
Copyright (c) 2026 Ying Liu.
File Description: Selects the KEM randomness backend without modifying drng.c/h.
*/

#ifndef KEM_RANDOM_H
#define KEM_RANDOM_H

#include "drng.h"

#if BIT_USE_SHAKE
int get_random_number_fips(unsigned char *random_number,
                           unsigned long long random_number_len_bits);
#endif

int kem_get_random_number(DRNG_ctx *drng,
                          unsigned char *random_number,
                          unsigned long long random_number_len_bits);

#endif
