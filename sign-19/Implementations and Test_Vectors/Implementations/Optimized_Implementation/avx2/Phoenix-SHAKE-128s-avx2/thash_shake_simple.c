/* Phoenix thash implementation using SHAKE256
 *
 * Tweakable hash function for Phoenix signature scheme.
 */

#include <stdint.h>
#include <string.h>

#include "thash.h"
#include "address.h"
#include "params.h"
#include "utils.h"

#include "fips202.h"

/* Compute tweakable hash on input blocks using SHAKE256 */
void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8])
{
    uint64_t shake_state[26];

    memcpy(shake_state, ctx->state_seeded_shake, sizeof(shake_state));
    shake256_inc_absorb(shake_state, (const unsigned char *)addr, SPX_ADDR_BYTES);
    shake256_inc_absorb(shake_state, in, inblocks * SPX_N);
    shake256_inc_finalize(shake_state);
    shake256_inc_squeeze(out, SPX_N, shake_state);
}

/* Initialize bitmask for thash (noop for simple variant) */
void thash_init_bitmask(unsigned char *bitmask_out, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8])
{
    (void) bitmask_out;
    (void) inblocks;
    (void) ctx;
    (void) addr;
}

/* Finalize thash with bitmask (equivalent to thash for simple variant) */
void thash_fin(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const spx_ctx *ctx, uint32_t addr[8], const unsigned char *bitmask)
{
    (void) bitmask;
    thash(out, in, inblocks, ctx, addr);
}