/* Phoenix 4-way parallel thash interface
 *
 * Parallel tweakable hash function with AVX2 optimization.
 */

#ifndef SPX_THASHX4_H
#define SPX_THASHX4_H

#include <stdint.h>
#include "context.h"
#include "params.h"

/* Compute 4-way parallel tweakable hash on input blocks */
#define thashx4 SPX_NAMESPACE(thashx4)
void thashx4(unsigned char *out0,
             unsigned char *out1,
             unsigned char *out2,
             unsigned char *out3,
             const unsigned char *in0,
             const unsigned char *in1,
             const unsigned char *in2,
             const unsigned char *in3, unsigned int inblocks,
             const spx_ctx *ctx, uint32_t addrx4[4*8]);

#endif /* SPX_THASHX4_H */