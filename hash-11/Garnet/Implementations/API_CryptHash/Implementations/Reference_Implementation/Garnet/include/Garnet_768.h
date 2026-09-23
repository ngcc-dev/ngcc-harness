/**
 * @file Garnet_1024.h
 * @brief Garnet-1024 hash function portable C99 reference implementation.
 */

#ifndef GARNET768_H
#define GARNET768_H

#include <stdint.h>
#include <stddef.h>

#define STATE_4x4 16
#define DIGEST_768 768


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct garnet_u128_t 
{
    u64 v[2];
 };


void Garnet_768(const u8* message, u64 mlength, u8* digest, struct garnet_u128_t state[STATE_4x4]);


static void pad_message(unsigned char* msg, uint64_t bit_len);

#endif /* GARNET768_H */
