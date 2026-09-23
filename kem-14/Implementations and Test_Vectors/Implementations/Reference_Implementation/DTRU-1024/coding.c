#include <stdint.h>
#include "params.h"
#include "reduce.h"
#include "coding.h"

static uint32_t sqr(int16_t x)
{
    return x * x;
}

static int16_t abs_q(int16_t x)
{
    uint16_t mask = -(((DTRU_Q - 1 - (x << 1)) >> 15) & 1);
    return (mask & (DTRU_Q - x)) | ((~mask) & x);
}

static uint8_t decode_d8_00(uint32_t *cost, uint32_t tmp_cost[8][2])
{
    uint8_t m[2] = {0, 0}, xor_sum = 0, res;
    uint32_t min_diff = ~0U >> 2, r;
    uint32_t min_i = 0;
    int i;

    *cost = 0;
    for (i = 0; i < 4; i++)
    {
        uint32_t c[2];
        c[0] = tmp_cost[i << 1][0] + tmp_cost[i << 1 | 1][0];
        c[1] = tmp_cost[i << 1][1] + tmp_cost[i << 1 | 1][1];
        r = ((c[1] - c[0]) >> 31) & 1;
        m[0] |= r << i;
        xor_sum ^= r;

        uint32_t tmp = ((-(r ^ 1)) & (uint32_t)c[0]) ^ ((-(r & 1)) & (uint32_t)c[1]);
        uint32_t tmp_xor = ((-(r & 1)) & (uint32_t)c[0]) ^ ((-(r ^ 1)) & (uint32_t)c[1]);

        *cost += tmp;
        uint32_t diff = tmp_xor - tmp;

        r = ((diff - min_diff) >> 31) & 1;
        min_diff = ((-r) & diff) | ((-(r ^ 1)) & min_diff);
        min_i = ((-r) & i) | ((-(r ^ 1)) & min_i);
    }
    m[1] = m[0] ^ (1 << min_i);
    res = ((-(xor_sum ^ 1)) & (uint32_t)m[0]) ^ ((-(xor_sum & 1)) & (uint32_t)m[1]);
    *cost += (-xor_sum) & min_diff;
    return res;
}

static uint8_t decode_d8_10(uint32_t *cost, uint32_t tmp_cost[8][2])
{
    uint8_t m[2] = {0, 0}, xor_sum = 0, res;
    uint32_t min_diff = ~0U >> 2, r;
    uint32_t min_i = 0;
    int i;

    *cost = 0;
    for (i = 0; i < 4; i++)
    {
        uint32_t c[2];
        c[0] = tmp_cost[i << 1][1] + tmp_cost[i << 1 | 1][0];
        c[1] = tmp_cost[i << 1][0] + tmp_cost[i << 1 | 1][1];
        r = ((c[1] - c[0]) >> 31) & 1;
        m[0] |= r << i;
        xor_sum ^= r;

        uint32_t tmp = ((-(r ^ 1)) & (uint32_t)c[0]) ^ ((-(r & 1)) & (uint32_t)c[1]);
        uint32_t tmp_xor = ((-(r & 1)) & (uint32_t)c[0]) ^ ((-(r ^ 1)) & (uint32_t)c[1]);

        *cost += tmp;
        uint32_t diff = tmp_xor - tmp;
        r = ((diff - min_diff) >> 31) & 1;
        min_diff = ((-r) & diff) | ((-(r ^ 1)) & min_diff);
        min_i = ((-r) & i) | ((-(r ^ 1)) & min_i);
    }
    m[1] = m[0] ^ (1 << min_i);
    res = ((-(xor_sum ^ 1)) & (uint32_t)m[0]) ^ ((-(xor_sum & 1)) & (uint32_t)m[1]);
    *cost += (-xor_sum) & min_diff;
    return res;
}

uint8_t decode_e8(int16_t vec[16])
{
    uint32_t cost[2], r;
    uint8_t m[2], res;
    uint32_t tmp_cost[8][2];
    int i;

    for (i = 0; i < 8; i++)
    {
        tmp_cost[i][0] = sqr(abs_q(vec[i])) + sqr(abs_q(vec[i + 8]));
        tmp_cost[i][1] = sqr(abs_q(fqcsubq(vec[i    ] - ((DTRU_Q + 1) >> 1)))) +
                         sqr(abs_q(fqcsubq(vec[i + 8] - ((DTRU_Q + 1) >> 1))));
    }

    m[0] = decode_d8_00(cost + 0, tmp_cost);
    m[1] = decode_d8_10(cost + 1, tmp_cost);
    r = ((cost[1] - cost[0]) >> 31) & 1;

    res = ((-(r ^ 1)) & (uint32_t)m[0]) ^ ((-(r & 1)) & (uint32_t)m[1]);
    res = ((((res ^ (res << 1)) & 0x3) | ((res >> 1) & 4)) << 1) | r;

    return res;
}

uint8_t encode_e8(uint8_t m)
{
    uint8_t a0 = m & 1;
    uint8_t a1 = (m >> 1) & 1;
    uint8_t a2 = (m >> 2) & 1;
    uint8_t a3 = (m >> 3) & 1;
    return ((-a1) & 0x0F) ^ ((-a2) & 0x3c) ^ ((-a3) & 0xF0) ^ ((-a0) & 0x55);
}