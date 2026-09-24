#ifndef SPX_THASH_H
#define SPX_THASH_H

#include "context.h"
#include "params.h"

#include <stdint.h>

#define thash SPX_NAMESPACE(thash)
void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8]);

void thash_init_bitmask(unsigned char *bitmask_out, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8]);

void thash_fin(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8], const unsigned char *bitmask);
#endif



