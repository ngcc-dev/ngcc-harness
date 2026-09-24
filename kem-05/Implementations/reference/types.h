/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef __TYPES_H_INCLUDED__
#define __TYPES_H_INCLUDED__

#include "defs.h"
#include <stdint.h>
#include <stddef.h>

/* type definitions used in backflip decoder: */
typedef uint8_t bit_t;
typedef uint16_t index_t;
typedef struct ring_buffer *ring_buffer_t;
typedef struct parameters *parameters_t;
typedef struct decoder *decoder_t;

/* Implement the backflip flipping queue as a ring buffer */
struct ring_buffer {
    index_t *raw_ptr_index;
    index_t *start_ptr_index;
    index_t *raw_ptr_position;
    index_t *start_ptr_position;
    /* Extra information on a position that might be useful */
    int *raw_ptr_extra;
    int *start_ptr_extra;
    size_t size;
    size_t start_idx;
    size_t length;
};

typedef struct uint128_s
{
    union
    {
        uint8_t bytes[16];
        uint32_t dwords[4];
        uint64_t qwords[2];
    };
} uint128_t;

//For clarity of the code.
#define IN
#define OUT

//Bit manipulations
#define BIT(len) (1ULL << (len))
#define MASK(len) (BIT(len) - 1ULL)

#define _INLINE_ static inline

//Make sure no compiler optimizations.
#pragma pack(push, 1)

typedef struct pk_buffer
{
    union
    {
      struct
      {
        uint8_t val[R_SIZE];
      };
      uint8_t raw[R_SIZE];
    };
} pk_buffer_t;

typedef struct ct_buffer
{
    union
    {
      struct
      {
        uint8_t val0[R_SIZE];
        uint8_t val1[ELL_SIZE];
      };
      uint8_t raw[R_SIZE+ELL_SIZE];
    };
} ct_buffer_t;

typedef ct_buffer_t ct_t;

typedef pk_buffer_t pk_t;

typedef struct sk_buffer
{
    union
    {
      struct
      {
        uint8_t val0[R_SIZE];
        uint8_t val1[R_SIZE];
      };
      uint8_t raw[2*R_SIZE];
    };
    uint8_t sigma[ELL_SIZE];
} sk_buffer_t;

typedef sk_buffer_t sk_t;

typedef struct ss_s
{
    uint8_t raw[ELL_SIZE];
} ss_t;

typedef struct syndrome_s
{
    uint8_t raw[R_BITS];
} syndrome_t;

enum _seed_id
{
    G_SEED = 0,
    H_SEED = 1,
    M_SEED = 2,
    E_SEED = 3
};

typedef struct seed_s
{
    union {
        uint8_t  raw[ELL_SIZE];
        uint64_t qwords[ELL_SIZE / sizeof(uint64_t)];
    };
} seed_t;

//Both keygen and encaps require double seed.
typedef struct double_seed_s
{
    union {
        struct {
            seed_t s1;
            seed_t s2;
        };
        uint8_t raw[sizeof(seed_t) * 2ULL];
    };
} double_seed_t;

//////////////////////////////
//   Error handling
/////////////////////////////

//This convention will work all over the code.
#define ERR(v) {res = v; goto EXIT;}
#define CHECK_STATUS(stat) {if(stat != SUCCESS) {goto EXIT;}}

enum _status
{
    SUCCESS                          = 0,
    E_FAIL_TO_DECODE                 = 1,
    E_OSSL_FAILURE                   = 2,
    E_FAIL_TO_PERFORM_CYCLIC_PRODUCT = 3,
    E_FAIL_TO_PERFORM_ADD            = 4,
    E_FAIL_TO_SPLIT                  = 5,
    E_AES_SET_KEY_FAIL               = 6,
    E_ERROR_WEIGHT_IS_NOT_T          = 7,
    E_DECODING_FAILURE               = 8,
    E_AES_CTR_PRF_INIT_FAIL          = 9,
    E_AES_OVER_USED                  = 10,
    E_HASH_FAIL                      = 11,
    E_XOF_FAIL                       = 12,
    E_XOF_OVER_USED                  = 13
};

typedef enum _status status_t;

#pragma pack(pop)

#endif //__TYPES_H_INCLUDED__
