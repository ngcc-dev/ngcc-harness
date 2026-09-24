/* Phoenix tweakable hash (thash) interface
 *
 * Tweakable hash function for Phoenix signature scheme.
 */

#ifndef PH_THASH_H
#define PH_THASH_H

#include "context.h"
#include "params.h"

#include <stdint.h>

/* Compute tweakable hash on input blocks using SHAKE256 */
#define thash SPX_NAMESPACE(thash)
void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8]);

/* Initialize bitmask for thash (noop for simple variant) */
void thash_init_bitmask(unsigned char *bitmask_out, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8]);

/* Finalize thash with bitmask (equivalent to thash for simple variant) */
void thash_fin(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8], const unsigned char *bitmask);

#endif /* PH_THASH_H */