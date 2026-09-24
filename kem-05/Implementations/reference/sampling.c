/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include "sampling.h"

_INLINE_ uint32_t count_ones(IN const uint8_t* a,
        IN const uint32_t len)
{
    uint32_t count = 0;

    for(uint32_t i = 0; i < len; i++)
    {
        count += __builtin_popcountll(a[i]);
    }

    return count;
}


status_t get_rand_mod_len(OUT uint32_t* rand_pos,
        IN const uint32_t len,
        IN OUT xof_prng_state_t* prf_state)
{
    status_t res = SUCCESS;

    // Generate 32 random bits
    res = xof_prng_generate((uint8_t*) rand_pos, prf_state, sizeof(*rand_pos));
    CHECK_STATUS(res);

    // the result of the multiplication by len may exceed 32 bits
    // so we cast (*rand_pos) on a 64 bits word
    uint64_t temp = *rand_pos;
    *rand_pos = (temp * len) >> 32; // 0 <= *rand_pos < len

    EXIT:
    return res;
}

void setZero(uint8_t * r, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
        r[i] = 0;
}
int CHECK_BIT(uint8_t * tmp, int position) {
    int index = position/8;
    int pos = position%8;
    return ((tmp[index] >> (pos))  & 0x01);
}
void SET_BIT(uint8_t * tmp, int position) {
    int index = position/8;
    int pos = position%8;
    tmp[index] |= 1UL << (pos);
}


status_t generate_sparse_rep(OUT uint8_t * r,
        IN  const uint32_t weight,
        IN  const uint32_t len,
        IN OUT xof_prng_state_t *prf_state)
{
    uint32_t rand_pos = 0;
    status_t res = SUCCESS;

    //Ensure r is zero.
    setZero(r, DIVIDE_AND_CEIL(len, 8ULL));

    for (int32_t i = weight - 1; i >= 0; i--)
    {
        res = get_rand_mod_len(&rand_pos, len - i, prf_state);
        CHECK_STATUS(res);

        rand_pos += i; // now i <= rand_pos < len

        if (CHECK_BIT(r, rand_pos))
        {
            // If collision, then select index i instead of rand_pos
            rand_pos = i;
        }
        SET_BIT(r, rand_pos);
    }

    EXIT:
    return res;
}
