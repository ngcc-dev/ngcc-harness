#include <stdio.h>
#include <string.h>
#include "registry.h"

static algorithm_t g_algs[MAX_ALG];
static size_t g_count = 0;

int registry_algorithm(const algorithm_t *alg)
{
    if (alg == NULL || alg->id == NULL || alg->hash == NULL || g_count >= MAX_ALG)
        return -1;
    g_algs[g_count++] = *alg;
    return 0;
}

const algorithm_t *get_algorithm_by_id(const char *id)
{
    size_t i;
    for (i = 0; i < g_count; i++) {
        if (strcmp(g_algs[i].id, id) == 0)
            return &g_algs[i];
    }
    return NULL;
}

size_t registry_count(void) { return g_count; }
const algorithm_t *registry_at(size_t index) { return index < g_count ? &g_algs[index] : NULL; }
