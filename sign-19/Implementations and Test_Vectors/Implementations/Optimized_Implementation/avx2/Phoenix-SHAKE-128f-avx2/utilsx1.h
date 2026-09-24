/* Phoenix single-lane treehash interface */

#ifndef SPX_UTILSX1_H
#define SPX_UTILSX1_H

#include <stdint.h>
#include "params.h"
#include "context.h"

/* Compute Merkle root and auth path using single-lane TreeHash */
#define treehashx1 SPX_NAMESPACE(treehashx1)
void treehashx1(unsigned char *root, unsigned char *auth_path,
                const spx_ctx* ctx,
                uint32_t leaf_idx, uint32_t idx_offset, uint32_t tree_height,
                void (*gen_leaf)(
                   unsigned char* /* Where to write the leaf */,
                   const spx_ctx* /* ctx */,
                   uint32_t addr_idx, void *info),
                uint32_t tree_addr[8], void *info);

#endif /* SPX_UTILSX1_H */