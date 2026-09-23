#include "afs_sbox64.h"

/* Function rotr32: rotates a 32-bit word right by a constant-count modulo 32 amount. */
static uint32_t rotr32(uint32_t x, unsigned n)
{
    n &= 31U;
    return (uint32_t)((x >> n) | (x << ((32U - n) & 31U)));
}

/* Function afs64_t5_k2: computes the AFS64_t5_k2 ARX S-box for one 64-bit lane. */
uint64_t afs64_t5_k2(uint64_t in, uint32_t c)
{
    uint32_t x = (uint32_t)(in >> 32);
    uint32_t y = (uint32_t)in;

    /* AFS-64, A8=11000011, K8=[17,24,1,1,16,31,24,0]. */
    x = (uint32_t)(x + rotr32(y, 17)); x ^= c;
    y = (uint32_t)(y + rotr32(x, 24)); y ^= c;
    x ^= rotr32(y, 1);
    y ^= rotr32(x, 1);
    x ^= rotr32(y, 16);
    y ^= rotr32(x, 31);
    x = (uint32_t)(x + rotr32(y, 24)); x ^= c;
    y = (uint32_t)(y + rotr32(x, 0));  y ^= c;

    return (((uint64_t)x) << 32) | (uint64_t)y;
}
