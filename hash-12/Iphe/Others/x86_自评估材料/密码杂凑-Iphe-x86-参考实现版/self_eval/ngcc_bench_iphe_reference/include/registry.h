#ifndef REGISTRY_H
#define REGISTRY_H

#include <stddef.h>

#define MAX_ALG 8

typedef int (*iphe_hash_fn)(int, const unsigned char *, unsigned long long, unsigned char *);

typedef struct {
    const char *id;
    const char *name;
    const char *library_path;
    int digest_bits;
    size_t block_bytes;
    iphe_hash_fn hash;
} algorithm_t;

int registry_algorithm(const algorithm_t *alg);
const algorithm_t *get_algorithm_by_id(const char *id);
size_t registry_count(void);
const algorithm_t *registry_at(size_t index);
void register_iphe_algorithms(void);
int run_hash_bench(const algorithm_t *alg, unsigned int times, size_t len_bytes);

#endif
