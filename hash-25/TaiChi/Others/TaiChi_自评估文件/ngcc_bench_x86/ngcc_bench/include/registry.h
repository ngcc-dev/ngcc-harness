#ifndef REGISTRY_H
#define REGISTRY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_ALG 32

typedef enum
{
    ALG_HASH
} ALG_TYPE;

enum {
    TAICHI_512_REF,
    TAICHI_768_REF,
    TAICHI_1024_REF,

    TAICHI_512_OP_PER,
    TAICHI_768_OP_PER,
    TAICHI_1024_OP_PER,

    TAICHI_512_OP_RES,
    TAICHI_768_OP_RES,
    TAICHI_1024_OP_RES
};

typedef struct
{
    ALG_TYPE type;
    uint32_t alg_id;
    const char *alg_name;
    const char *author_name;
    const char *lib_name;
    const void *method;
} ALGORITHM;

int registry_algorithm(ALGORITHM *alg);

const ALGORITHM *get_algorithm_by_id(uint32_t alg_id);

#endif