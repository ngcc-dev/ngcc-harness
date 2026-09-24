#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "utilsx1.h"
#include "hash.h"
#include "hashx8.h"
#include "thash.h"
#include "thashx8.h"
#include "wots.h"
#include "wotsx1.h"
#include "wots_avx2.h"
#include "address.h"
#include "params.h"

const unsigned int wots_w[]    = SPX_WOTS_W_ARRAY;
const unsigned int wots_wlog[] = SPX_WOTS_LOGW_ARRAY;

/**
 * Takes a message and derives the matching chain lengths.
 */
unsigned int chain_lengths(unsigned int *lengths, const unsigned char *msg)
{
    int bitbuf = 0;
    unsigned int bits_in_buf = 0;
    unsigned int csum = 0;
    const unsigned char *in = msg;

    for (unsigned int i = 0; i < SPX_WOTS_LEN; i++) {
        while (bits_in_buf < wots_wlog[i]) {
            bitbuf = (bitbuf << 8) | *in++;
            bits_in_buf += 8;
        }
        bits_in_buf -= wots_wlog[i];
        lengths[i] = (unsigned int)(bitbuf >> bits_in_buf) & (wots_w[i] - 1u);
        csum += wots_w[i] - 1u - lengths[i];
    }

    return csum;
}

static void wots_chains_from_sig_x8(unsigned char *pk,
                                    const unsigned char *sig,
                                    const unsigned int *lengths,
                                    const spx_ctx *ctx,
                                    const uint32_t addr[8])
{
    unsigned int pos[SPX_WOTS_LEN];
    unsigned int end[SPX_WOTS_LEN];
    unsigned int active[SPX_WOTS_LEN];
    unsigned int active_len = 0;

    memcpy(pk, sig, SPX_WOTS_BYTES);

    for (unsigned int i = 0; i < SPX_WOTS_LEN; i++) {
        pos[i] = lengths[i];
        end[i] = wots_w[i] - 1;
        if (pos[i] < end[i]) {
            active[active_len++] = i;
        }
    }

    while (active_len != 0) {
        unsigned int lanes = active_len < 8 ? active_len : 8;
        unsigned int c[8];
        uint32_t addrx8[8 * 8];

        for (unsigned int j = 0; j < 8; j++) {
            c[j] = active[j < lanes ? j : 0];
            memcpy(addrx8 + 8 * j, addr, 8 * sizeof(uint32_t));
            set_chain_addr(addrx8 + 8 * j, c[j]);
            set_hash_addr(addrx8 + 8 * j, pos[c[j]]);
        }

        thashx8(pk + c[0] * SPX_N,
                pk + c[1] * SPX_N,
                pk + c[2] * SPX_N,
                pk + c[3] * SPX_N,
                pk + c[4] * SPX_N,
                pk + c[5] * SPX_N,
                pk + c[6] * SPX_N,
                pk + c[7] * SPX_N,
                pk + c[0] * SPX_N,
                pk + c[1] * SPX_N,
                pk + c[2] * SPX_N,
                pk + c[3] * SPX_N,
                pk + c[4] * SPX_N,
                pk + c[5] * SPX_N,
                pk + c[6] * SPX_N,
                pk + c[7] * SPX_N,
                1, ctx, addrx8);

        for (unsigned int j = 0; j < lanes; j++) {
            pos[active[j]]++;
        }

        for (unsigned int j = 0; j < active_len;) {
            unsigned int idx = active[j];
            if (pos[idx] >= end[idx]) {
                active[j] = active[--active_len];
            } else {
                j++;
            }
        }
    }
}

/**
 * Takes a WOTS signature and an n-byte message, computes a WOTS public key.
 *
 * Writes the computed public key to 'pk'.
 */
void wots_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig,
                      const unsigned char *msg,
                      const spx_ctx *ctx, uint32_t addr[8], uint32_t counter)
{
    unsigned int lengths[SPX_WOTS_LEN];
    uint32_t mask = (~0U << (8 - WOTS_ZERO_BITS));
    unsigned char bitmask[SPX_N];
    int csum;
    unsigned char digest[SPX_N];
    uint32_t wots_pk_addr[8] = {0};

    set_type(wots_pk_addr, SPX_ADDR_TYPE_COMPRESS_WOTS);
    copy_keypair_addr(wots_pk_addr, addr);
    thash_init_bitmask(bitmask, 1, ctx, wots_pk_addr);
    ull_to_bytes(((unsigned char *)wots_pk_addr) + SPX_OFFSET_COUNTER,
                 COUNTER_SIZE, counter);
    thash_fin(digest, msg, 1, ctx, wots_pk_addr, bitmask);

    csum = chain_lengths(lengths, digest);

    if (csum != WANTED_CHECKSUM || ((digest[SPX_N - 1] & mask) != 0)) {
        memset(pk, 0, SPX_WOTS_BYTES);
        return;
    }

    wots_chains_from_sig_x8(pk, sig, lengths, ctx, addr);
}

/*
 * This generates four sequential WOTS public keys.  If leaf_info points to the
 * WOTS key being signed, it also copies the selected chain values into the
 * WOTS signature.
 */
void wots_gen_leaf_avx2(unsigned char *dest,
                        const spx_ctx *ctx,
                        uint32_t leaf_idx, void *v_info)
{
    struct leaf_info_avx2 *info = v_info;
    uint32_t *leaf_addr = info->leaf_addr;
    uint32_t *pk_addr = info->pk_addr;
    unsigned int i, j, k;
    unsigned char pk_buffer[4 * SPX_WOTS_BYTES];
    unsigned int wots_offset = SPX_WOTS_BYTES;
    unsigned char *buffer;
    uint32_t wots_k_mask;
    unsigned int wots_sign_index;

    if (((leaf_idx ^ info->wots_sign_leaf) & ~3U) == 0) {
        wots_k_mask = 0;
        wots_sign_index = info->wots_sign_leaf & 3U;
    } else {
        wots_k_mask = ~0U;
        wots_sign_index = 0;
    }

    for (j = 0; j < 4; j++) {
        set_keypair_addr(leaf_addr + j*8, leaf_idx + j);
        set_keypair_addr(pk_addr + j*8, leaf_idx + j);
    }

    for (i = 0, buffer = pk_buffer; i < SPX_WOTS_LEN;) {
        if (i + 1u < SPX_WOTS_LEN && wots_w[i] == wots_w[i + 1u]) {
            unsigned char *buffer0 = buffer;
            unsigned char *buffer1 = buffer + SPX_N;
            uint32_t addrx8[8 * 8];
            uint32_t wots_k0 = info->wots_steps[i] | wots_k_mask;
            uint32_t wots_k1 = info->wots_steps[i + 1u] | wots_k_mask;

            for (j = 0; j < 4; j++) {
                memcpy(addrx8 + j * 8, leaf_addr + j * 8, 8 * sizeof(uint32_t));
                set_chain_addr(addrx8 + j * 8, i);
                set_hash_addr(addrx8 + j * 8, 0);
                set_type(addrx8 + j * 8, SPX_ADDR_TYPE_WOTSPRF);

                memcpy(addrx8 + (j + 4u) * 8, leaf_addr + j * 8, 8 * sizeof(uint32_t));
                set_chain_addr(addrx8 + (j + 4u) * 8, i + 1u);
                set_hash_addr(addrx8 + (j + 4u) * 8, 0);
                set_type(addrx8 + (j + 4u) * 8, SPX_ADDR_TYPE_WOTSPRF);
            }

            prf_addrx8(buffer0 + 0 * wots_offset,
                       buffer0 + 1 * wots_offset,
                       buffer0 + 2 * wots_offset,
                       buffer0 + 3 * wots_offset,
                       buffer1 + 0 * wots_offset,
                       buffer1 + 1 * wots_offset,
                       buffer1 + 2 * wots_offset,
                       buffer1 + 3 * wots_offset,
                       ctx, addrx8);

            for (j = 0; j < 8; j++) {
                set_type(addrx8 + j * 8, SPX_ADDR_TYPE_WOTS);
            }

            for (k = 0;; k++) {
                if (k == wots_k0) {
                    memcpy(info->wots_sig + i * SPX_N,
                           buffer0 + wots_sign_index * wots_offset, SPX_N);
                }
                if (k == wots_k1) {
                    memcpy(info->wots_sig + (i + 1u) * SPX_N,
                           buffer1 + wots_sign_index * wots_offset, SPX_N);
                }

                if (k == wots_w[i] - 1u) {
                    break;
                }

                for (j = 0; j < 8; j++) {
                    set_hash_addr(addrx8 + j * 8, k);
                }

                thashx8(buffer0 + 0 * wots_offset,
                        buffer0 + 1 * wots_offset,
                        buffer0 + 2 * wots_offset,
                        buffer0 + 3 * wots_offset,
                        buffer1 + 0 * wots_offset,
                        buffer1 + 1 * wots_offset,
                        buffer1 + 2 * wots_offset,
                        buffer1 + 3 * wots_offset,
                        buffer0 + 0 * wots_offset,
                        buffer0 + 1 * wots_offset,
                        buffer0 + 2 * wots_offset,
                        buffer0 + 3 * wots_offset,
                        buffer1 + 0 * wots_offset,
                        buffer1 + 1 * wots_offset,
                        buffer1 + 2 * wots_offset,
                        buffer1 + 3 * wots_offset,
                        1, ctx, addrx8);
            }

            i += 2u;
            buffer += 2u * SPX_N;
            continue;
        }
        uint32_t wots_k = info->wots_steps[i] | wots_k_mask;

        for (j = 0; j < 4; j++) {
            set_chain_addr(leaf_addr + j * 8, i);
            set_hash_addr(leaf_addr + j * 8, 0);
            set_type(leaf_addr + j * 8, SPX_ADDR_TYPE_WOTSPRF);
            prf_addr(buffer + j * wots_offset, ctx, leaf_addr + j * 8);
        }

        for (j = 0; j < 4; j++) {
            set_type(leaf_addr + j * 8, SPX_ADDR_TYPE_WOTS);
        }

        for (k = 0;; k++) {
            if (k == wots_k) {
                memcpy(info->wots_sig + i * SPX_N,
                       buffer + wots_sign_index * wots_offset, SPX_N);
            }

            if (k == wots_w[i] - 1) {
                break;
            }

            for (j = 0; j < 4; j++) {
                set_hash_addr(leaf_addr + j * 8, k);
                thash(buffer + j * wots_offset,
                      buffer + j * wots_offset, 1, ctx, leaf_addr + j * 8);
            }
        }

        i++;
        buffer += SPX_N;
    }

    {
        for (j = 0; j < 4; j++) {
            thash(dest + j * SPX_N,
                  pk_buffer + j * wots_offset,
                  SPX_WOTS_LEN, ctx, pk_addr + j * 8);
        }
    }
}
