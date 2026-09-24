#ifndef REGISTRY_H
#define REGISTRY_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_ALG 32

typedef enum {
    ALG_SIG
} ALG_TYPE;

typedef struct {
    ALG_TYPE type;
    uint32_t alg_id;
    int security_level;
    const char *alg_name;
    const char *author_name;
    const char *lib_name;
    const void *method;
} ALGORITHM;

int registry_algorithm(ALGORITHM *alg);

const ALGORITHM *get_algorithm_by_id(uint32_t alg_id);

#endif
