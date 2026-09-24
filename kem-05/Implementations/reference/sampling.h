/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include "xof_prng.h"

typedef enum
{
    NO_RESTRICTION=0,
    MUST_BE_ODD=1
} must_be_odd_t;

status_t generate_sparse_rep(OUT uint8_t* r,
        IN const uint32_t weight,
        IN const uint32_t len,
        IN OUT xof_prng_state_t *prf_state);

// sample a single number smaller than len.
status_t get_rand_mod_len(OUT uint32_t* rand_pos,
        IN const uint32_t len,
        IN OUT xof_prng_state_t* prf_state);

#endif //_SAMPLE_H_
