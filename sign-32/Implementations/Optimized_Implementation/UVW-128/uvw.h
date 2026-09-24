#ifndef UVW_H
#define UVW_H

#include <stdbool.h>
#include <stdint.h>
#include "fq_arithmetic/vf3.h"
#include "fq_arithmetic/mf3.h"
#include "fq_arithmetic/dp.h"
#include "gauss.h"

#ifdef USE_API_PKC
    #include "auxfunc.h"
    #include "drng.h"

    typedef struct {
        unsigned char *msg;
        unsigned long long msg_len;
        unsigned char *xof_buf;
        unsigned long long xof_len;
        unsigned long long xof_pos;
    } uvw_hash_ctx;

    #define shake256_ctx uvw_hash_ctx
#else
    #include "keccak/shake256.h"
#endif

#define MF3_DBG_PRINT(x) do { \
    DBG("\n%s:", #x); \
    mf3_print(x); \
} while (0)

#define VF3_DBG_PRINT(x) do { \
    DBG("\n%s:", #x); \
    vf3_print(x); \
} while (0)

#define DP_DBG_PRINT(x) do { \
    DBG("\n%s:", #x); \
    dp_print(x); \
} while (0)

#define ARRAY_SIZE_T_DBG_PRINT(x, len) do { \
    DBG("\n%s:", #x); \
    for (size_t i = 0; i < (len); i++) { \
        fprintf(stderr, "%lu,", x[i]); \
    } \
    fprintf(stderr, "\n"); \
} while (0)

typedef struct {
    size_t lambda;
    size_t n;
    size_t k;
    size_t k1;
    size_t k2;
    size_t w;
} uvw_param;

typedef struct {
    uvw_param param;
    mf3_e *r;
} uvw_public_key;

typedef struct {
    uvw_param param;
    mf3_e *sti;
    mf3_e *h_x;
    mf3_e *h_y;
    mf3_e *h_z;
    dp_e *d;
} uvw_secret_key;

typedef struct {
    uvw_secret_key sk;
    uvw_public_key pk;
} uvw_keypair;

typedef struct {
    uvw_param param;
    uint8_t *r;
    vf3_e *e;
} uvw_signature;

uvw_param uvw_param_from_level(const size_t level);

uvw_keypair uvw_keygen(const uvw_param param);

uvw_signature uvw_sign(const uvw_secret_key sk, shake256_ctx *ctx);

bool uvw_verify(const uvw_public_key pk, shake256_ctx *ctx, const uvw_signature sign);

void uvw_free_secret_key(uvw_secret_key sk);
void uvw_free_public_key(uvw_public_key pk);
void uvw_free_signature(uvw_signature sign);

void uvw_write_secret_key(const uvw_secret_key sk, FILE *f);
void uvw_write_public_key(const uvw_public_key pk, FILE *f);
void uvw_write_signature(const uvw_signature sign, FILE *f);

uvw_secret_key uvw_read_secret_key(FILE *f);
uvw_public_key uvw_read_public_key(FILE *f);
uvw_signature uvw_read_signature(FILE *f);

#ifdef USE_API_PKC
void uvw_hash_init(shake256_ctx *ctx);
void uvw_hash_update(shake256_ctx *ctx, const unsigned char *data, unsigned long long len);
void uvw_hash_read(shake256_ctx *ctx, unsigned char *out, unsigned long long len);
void uvw_hash_free(shake256_ctx *ctx);
#endif

#endif
