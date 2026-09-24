#include <string.h>
#include <stdlib.h>

#include "sample.h"
#include "drng.h"

// Absolute value of modulo operation for 32-bit unsigned integers in the range (-HALF_R, HALF_R).
static inline uint32_t lmod32_r(uint32_t x)
{
    uint32_t y = x - (-(x >= PARAM_R) & PARAM_R);
    uint32_t mask = -(y >= HALF_R);
    return (y & ~mask) | ((PARAM_R - y) & mask);
}

// Modulo operation for 32-bit unsigned integers in the range [0, PARAM_R).
static inline uint32_t mod32_r(uint32_t x)
{
    return x - (-(x >= PARAM_R) & PARAM_R);
}

// Tests if the generated indices form a weak key.
static inline uint8_t weak_key_test(const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2)
{
    uint8_t *dist = (uint8_t *)calloc(PARAM_R, sizeof(uint8_t));
    uint32_t *cnt = (uint32_t *)calloc(PARAM_D + 1, sizeof(uint32_t));
    uint8_t weak = 0;
    for (size_t i = 1; i < PARAM_D; i++)
    {
        for (size_t j = 0; j < i; j++)
        {
            dist[lmod32_r(idx0[j] + PARAM_R - idx0[i])]++;
        }
    }
    for (size_t i = 0; i < HALF_R; i++)
    {
        cnt[dist[i]]++;
        dist[i] = 0;
    }
    uint64_t res = 0;
    for (uint64_t i = 2, weight = 1; i < PARAM_D; weight += i++)
    {
        res += weight * cnt[i];
        cnt[i] = 0;
    }
    weak |= (res > PARAM_S);
    for (size_t i = 1; i < PARAM_D; i++)
    {
        for (size_t j = 0; j < i; j++)
        {
            dist[lmod32_r(idx1[j] + PARAM_R - idx1[i])]++;
        }
    }
    for (size_t i = 0; i < HALF_R; i++)
    {
        cnt[dist[i]]++;
        dist[i] = 0;
    }
    res = 0;
    for (uint64_t i = 2, weight = 1; i < PARAM_D; weight += i++)
    {
        res += weight * cnt[i];
        cnt[i] = 0;
    }
    weak |= (res > PARAM_S);
    for (size_t i = 1; i < PARAM_D; i++)
    {
        for (size_t j = 0; j < i; j++)
        {
            dist[lmod32_r(idx2[j] + PARAM_R - idx2[i])]++;
        }
    }
    for (size_t i = 0; i < HALF_R; i++)
    {
        cnt[dist[i]]++;
        dist[i] = 0;
    }
    res = 0;
    for (uint64_t i = 2, weight = 1; i < PARAM_D; weight += i++)
    {
        res += weight * cnt[i];
        cnt[i] = 0;
    }
    weak |= (res > PARAM_S);
    for (size_t i = 0; i < PARAM_D; i++)
    {
        for (size_t j = 0; j < PARAM_D; j++)
        {
            dist[mod32_r(idx1[j] + PARAM_R - idx0[i])]++;
        }
    }
    for (size_t i = 0; i < PARAM_R; i++)
    {
        cnt[dist[i]]++;
        dist[i] = 0;
    }
    res = 0;
    for (uint64_t i = 2, weight = 1; i <= PARAM_D; weight += i++)
    {
        res += weight * cnt[i];
        cnt[i] = 0;
    }
    weak |= (res > PARAM_SS);
    for (size_t i = 0; i < PARAM_D; i++)
    {
        for (size_t j = 0; j < PARAM_D; j++)
        {
            dist[mod32_r(idx2[j] + PARAM_R - idx1[i])]++;
        }
    }
    for (size_t i = 0; i < PARAM_R; i++)
    {
        cnt[dist[i]]++;
        dist[i] = 0;
    }
    res = 0;
    for (uint64_t i = 2, weight = 1; i <= PARAM_D; weight += i++)
    {
        res += weight * cnt[i];
        cnt[i] = 0;
    }
    weak |= (res > PARAM_SS);
    for (size_t i = 0; i < PARAM_D; i++)
    {
        for (size_t j = 0; j < PARAM_D; j++)
        {
            dist[mod32_r(idx0[j] + PARAM_R - idx2[i])]++;
        }
    }
    for (size_t i = 0; i < PARAM_R; i++)
    {
        cnt[dist[i]]++;
        dist[i] = 0;
    }
    res = 0;
    for (uint64_t i = 2, weight = 1; i <= PARAM_D; weight += i++)
    {
        res += weight * cnt[i];
        cnt[i] = 0;
    }
    weak |= (res > PARAM_SS);
    free(dist);
    free(cnt);
    return weak;
}

// Generates w distinct random indices in the range [0, len).
void generate_random_idx(uint32_t *idx, uint32_t len, uint32_t w, DRNG_ctx *drng)
{
    uint32_t pos = w;

    while (pos-- > 0)
    {
        uint32_t random_number = 0;
        get_random_number(drng, (uint8_t *)&random_number, 32);
        random_number = pos + (uint32_t)((uint64_t)random_number * (len - pos) >> 32);

        uint32_t mask = 0;
        for (uint32_t j = pos + 1; j < w; j++)
        {
            mask |= - (uint32_t)(idx[j] == random_number);
        }

        idx[pos] = (pos & mask) | (random_number & ~mask);
    }
}

// Generates a sparse binary vector with hamming weight w.
void generate_random_vector(uint8_t *vec, uint32_t len, uint32_t w, DRNG_ctx *drng)
{
    uint32_t idx[w];

    generate_random_idx(idx, len, w, drng);

    for (uint32_t i = 0; i < w; i++)
    {
        vec[idx[i] >> 3] |= (1 << (idx[i] & 7));
    }
}

// Generates the three secret key components from a seed.
void generate_secret_key(uint8_t *h0, uint8_t *h1, uint8_t *h2, uint32_t *h0_idx, uint32_t *h1_idx, uint32_t *h2_idx, const uint8_t *seed)
{
    DRNG_ctx local_drng;
    init_random_number(&local_drng, seed, M_SIZE_BYTES);

    generate_random_idx(h0_idx, PARAM_R, PARAM_D, &local_drng);
    generate_random_idx(h1_idx, PARAM_R, PARAM_D, &local_drng);
    generate_random_idx(h2_idx, PARAM_R, PARAM_D, &local_drng);
    while(weak_key_test(h0_idx, h1_idx, h2_idx))
    {
        generate_random_idx(h0_idx, PARAM_R, PARAM_D, &local_drng);
        generate_random_idx(h1_idx, PARAM_R, PARAM_D, &local_drng);
        generate_random_idx(h2_idx, PARAM_R, PARAM_D, &local_drng);
    }
    for (size_t i = 0; i < PARAM_D; i++)
    {
        h0[h0_idx[i] >> 3] |= (1 << (h0_idx[i] & 7));
        h1[h1_idx[i] >> 3] |= (1 << (h1_idx[i] & 7));
        h2[h2_idx[i] >> 3] |= (1 << (h2_idx[i] & 7));
    }
}

// Adjusts the vector so its parity bit matches the target parity.
void set_hamming_weight(uint8_t *vec, uint8_t parity)
{
    size_t low_bits = (PARAM_R - 1) & 7;
    vec[R_SIZE_BYTES - 1] &= ((1 << low_bits) - 1);
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        parity ^= __builtin_popcount(vec[i]) & 1;
    }
    vec[R_SIZE_BYTES - 1] |= (parity << low_bits);
}

// Generates t1, t2, and r1 from a hash-based DRNG stream.
void generate_hash_vectors(uint8_t *t1, uint8_t *t2, uint8_t *r1, const uint8_t *seed)
{
    DRNG_ctx local_drng;
    init_random_number(&local_drng, seed, M_SIZE_BYTES);

    get_random_number(&local_drng, t1, R_SIZE_BYTES << 3);
    set_hamming_weight(t1, 0);
    get_random_number(&local_drng, t2, R_SIZE_BYTES << 3);
    set_hamming_weight(t2, 0);
    get_random_number(&local_drng, r1, R_SIZE_BYTES << 3);
    set_hamming_weight(r1, 1);
}

// Generates the error vector from the message and r2 value.
void generate_error_vector(uint8_t *e0, uint8_t *e1, uint8_t *e2, const uint8_t *msg, const uint8_t *r2)
{
    uint8_t *hash_input = (uint8_t *)calloc(M_SIZE_BYTES + R_SIZE_BYTES, sizeof(uint8_t));
    if (hash_input == NULL)return;
    
    memcpy(hash_input, msg, M_SIZE_BYTES);
    memcpy(hash_input + M_SIZE_BYTES, r2, R_SIZE_BYTES);

    DRNG_ctx local_drng;
    init_random_number(&local_drng, hash_input, M_SIZE_BYTES + R_SIZE_BYTES);

    uint32_t idx[PARAM_T];
    generate_random_idx(idx, PARAM_N, PARAM_T, &local_drng);

    for (uint32_t i = 0; i < PARAM_T; i++)
    {
        uint32_t pos = idx[i];
        uint32_t mask0 = -(pos < PARAM_R);
        uint32_t mask1 = -(pos < 2 * PARAM_R) & ~mask0;
        uint32_t mask2 = ~(mask0 | mask1);
        pos = pos % PARAM_R;

        e0[pos >> 3] |= (mask0 & (1 << (pos & 7)));
        e1[pos >> 3] |= (mask1 & (1 << (pos & 7)));
        e2[pos >> 3] |= (mask2 & (1 << (pos & 7)));
    }

    free(hash_input);
}