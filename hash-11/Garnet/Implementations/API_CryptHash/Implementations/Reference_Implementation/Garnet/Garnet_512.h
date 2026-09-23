/**
 * @file Garnet_512.h
 * @brief Garnet-512 hash function portable C99 reference implementation.
 */

#ifndef GARNET512_H
#define GARNET512_H

#include <stdint.h>
#include <stddef.h>

#define STATE_4x4 16
#define DIGEST_512 512

typedef struct {
    uint64_t v[2];  
} garnet_u128_t;


void garnet_512(const uint8_t* message, uint64_t mlength, uint8_t* digest, garnet_u128_t state[STATE_4x4]);


void pad_message(unsigned char* msg, uint64_t bit_len);

#endif /* GARNET512_H */