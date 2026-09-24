#include "registry.h"

static ALGORITHM registered_algs[MAX_ALG];
static size_t g_alg_count = 0;

int registry_algorithm(ALGORITHM *alg) {
    if(alg->method == NULL)
        return -1;
    if (g_alg_count >= MAX_ALG) {
        fprintf(stderr, "The number of registered algorithm reached limit.\n");
        return -1;
    }
    if(alg->type != ALG_KEM && alg->type != ALG_SIG && alg->type != ALG_KEX &&
        alg->type != ALG_HASH && alg->type != ALG_CIPHER) {
        fprintf(stderr, "Unknown ALG_TYPE.\n");
        return -1;
    }
    registered_algs[g_alg_count].type = alg->type;
    registered_algs[g_alg_count].method = alg->method;
    registered_algs[g_alg_count].alg_id = alg->alg_id;
    registered_algs[g_alg_count].lib_name = alg->lib_name;
    registered_algs[g_alg_count].alg_name = alg->alg_name;
    registered_algs[g_alg_count].author_name = alg->author_name;
    g_alg_count++;
    return 0;
}

const ALGORITHM *get_algorithm_by_id(const uint32_t alg_id) {
    for(size_t i = 0; i < g_alg_count; i++) {
        if (registered_algs[i].alg_id == alg_id) {
            return &registered_algs[i];
        }
    }
    return NULL;
}