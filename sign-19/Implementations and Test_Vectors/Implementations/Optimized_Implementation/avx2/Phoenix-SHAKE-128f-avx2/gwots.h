/* Phoenix-GWOTS interface
 *
 * GWOTS with variable-length chains for Phoenix signature scheme.
 */

#ifndef PH_GWOTS_H
#define PH_GWOTS_H

#include <stdint.h>

#include "params.h"
#include "context.h"

/* Derive chain lengths from message using base-w conversion. */
#define chain_lengths SPX_NAMESPACE(chain_lengths)
unsigned int chain_lengths(unsigned int *lengths, const unsigned char *msg);

/* Reconstruct GWOTS public key from signature using range-constrained chains. */
#define gwots_pk_from_sig SPX_NAMESPACE(gwots_pk_from_sig)
void gwots_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const spx_ctx *ctx, uint32_t addr[8], uint32_t counter);

#endif /* PH_GWOTS_H */
