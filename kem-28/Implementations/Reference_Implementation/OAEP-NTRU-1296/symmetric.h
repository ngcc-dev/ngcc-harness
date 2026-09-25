#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

void hash_f(uint8_t *buf, const uint8_t *msg);
void hash_g(uint8_t *buf, const uint8_t *msg);
void hash_h_prime(uint8_t *buf, const uint8_t *msg);

#endif
