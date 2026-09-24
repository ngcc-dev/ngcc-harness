#ifndef VERIFY_H
#define VERIFY_H

#include <stdio.h>

int verify(const uint8_t *a, const uint8_t *b, size_t len);

void cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b);

int verify32(const uint8_t *a, const uint8_t *b, size_t len);

void cmov32(uint8_t *r, const uint8_t *x, uint8_t b);

#endif
