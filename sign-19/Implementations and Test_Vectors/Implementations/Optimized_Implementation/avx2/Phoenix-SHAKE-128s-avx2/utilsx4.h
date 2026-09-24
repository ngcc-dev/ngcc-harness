/* Phoenix 4-way parallel treehash interface */

#ifndef SPX_UTILSX4_H
#define SPX_UTILSX4_H

#include <stdint.h>
#include "params.h"

/* Compute Merkle root and auth path using 4-way AVX2 parallel TreeHash */
#define treehashx4 SPX_NAMESPACE(treehashx4)
void treehashx4(unsigned char *root, unsigned char *auth_path,
                const spx_ctx *ctx,
                uint32_t leaf_idx, uint32_t idx_offset, uint32_t tree_height,
                void (*gen_leafx4)(
                   unsigned char* /* Where to write the leaves */,
                   const spx_ctx* /* ctx */,
                   uint32_t addr_idx, void *info),
                uint32_t tree_addrx4[4*8], void *info);

#endif /* SPX_UTILSX4_H */