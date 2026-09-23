/**
 * @file Garnet_1024.h
 * @brief Garnet-1024 hash function portable C99 reference implementation.
 */

#ifndef GARNET1024_H
#define GARNET1024_H

#include <stdint.h>
#include <stddef.h>

#define STATE_4x4 16
#define DIGEST_1024 1024

typedef struct {
    uint64_t v[2];
} garnet_u128_t;


void Garnet_1024(const uint8_t* message, uint64_t mlength, uint8_t* digest, garnet_u128_t state[STATE_4x4]);


static void pad_message(unsigned char* msg, uint64_t bit_len);

#endif /* GARNET1024_H */
