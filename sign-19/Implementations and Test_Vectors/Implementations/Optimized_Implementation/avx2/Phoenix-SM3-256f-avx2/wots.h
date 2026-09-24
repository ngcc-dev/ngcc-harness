#ifndef SPX_GWOTS_H
#define SPX_GWOTS_H

#include <stdint.h>

#include "params.h"
#include "context.h"

/**
 * Takes a GWOTS signature and an n-byte message, computes a GWOTS public key.
 *
 * Writes the computed public key to 'pk'.
 */
#define gwots_pk_from_sig SPX_NAMESPACE(gwots_pk_from_sig)
void gwots_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const spx_ctx *ctx, uint32_t addr[8], uint32_t counter);

/*
 * Compute the chain lengths needed for a given message hash
 */
#define chain_lengths SPX_NAMESPACE(chain_lengths)
unsigned int chain_lengths(unsigned int *lengths, const unsigned char *msg);

#endif



