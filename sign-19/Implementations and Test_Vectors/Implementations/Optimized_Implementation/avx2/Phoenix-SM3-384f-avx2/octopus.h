#ifndef OCTOPUS_H
#define OCTOPUS_H

#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "hash.h"
#include "thash.h"
#include "address.h"
#include "context.h"

typedef struct {
        uint32_t index;
        unsigned char hash[SPX_N];
    } node_entry;

// Octopus authentication entry (only indices)
typedef struct {
    uint32_t level;
    uint32_t index;
} octopus_entry;

// Octopus authentication structure (only indices)
typedef struct {
    octopus_entry entries[SPX_TFORS_K * SPX_TFORS_HEIGHT];
    uint32_t count;
} octopus_auth;

void octopus_compute_auth_count(const uint32_t *indices, uint32_t len, uint32_t *auth_count);

// generate auth indices
void octopus_compute(octopus_auth *auth,
                     const uint32_t *indices);

// generate auth paths
void octopus_compute_auth_paths(unsigned char root[SPX_N],
                                unsigned char *sig,
                                const uint32_t *indices,
                                const spx_ctx *ctx,
                                uint32_t tree_addr[8]);

void octopus_compute_auth_pathsx4(unsigned char root[SPX_N],
                                  unsigned char *sig,
                                  const uint32_t *indices,
                                  const spx_ctx *ctx,
                                  uint32_t tree_addr[8]);

// recompute root from auth paths
void octopus_recompute_root(unsigned char root[SPX_N],
                           const unsigned char *sig,
                           const uint32_t *indices,
                           uint32_t leaf_count,
                           const octopus_auth *auth,
                           const unsigned char *leaf_hashes,
                           const spx_ctx *ctx,
                           uint32_t tree_addr[8]);

#endif /* OCTOPUS_H */