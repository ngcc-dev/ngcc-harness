#include <stdint.h>
#include <string.h>

#include "address.h"
#include "hashx4.h"
#include "params.h"
#include "thashx4.h"
#include "utils.h"
#include "utilsx4.h"

static uint32_t chain_limit(unsigned int start, unsigned int steps, uint32_t w)
{
    uint32_t end = start + steps;

    if (end < start || end > w) {
        end = w;
    }

    return end;
}

void chainx4(unsigned char *out0,
             unsigned char *out1,
             unsigned char *out2,
             unsigned char *out3,
             const unsigned char *in0,
             const unsigned char *in1,
             const unsigned char *in2,
             const unsigned char *in3,
             const unsigned int start[4],
             const unsigned int steps[4],
             const spx_ctx *ctx,
             uint32_t addrx4[4 * 8],
             uint32_t w)
{
    uint32_t ends[4];
    uint32_t max_end = 0;
    uint32_t k;
    unsigned char dummy0[SPX_N] = {0};
    unsigned char dummy1[SPX_N] = {0};
    unsigned char dummy2[SPX_N] = {0};
    unsigned char dummy3[SPX_N] = {0};
    uint32_t step_addrx4[4 * 8];

    memcpy(out0, in0, SPX_N);
    memcpy(out1, in1, SPX_N);
    memcpy(out2, in2, SPX_N);
    memcpy(out3, in3, SPX_N);

    for (k = 0; k < 4; k++) {
        ends[k] = chain_limit(start[k], steps[k], w);
        if (ends[k] > max_end) {
            max_end = ends[k];
        }
    }

    for (k = 0; k < max_end; k++) {
        unsigned char *lane_out[4] = {dummy0, dummy1, dummy2, dummy3};
        const unsigned char *lane_in[4] = {dummy0, dummy1, dummy2, dummy3};

        memset(step_addrx4, 0, sizeof(step_addrx4));

        for (uint32_t lane = 0; lane < 4; lane++) {
            if (k >= start[lane] && k < ends[lane]) {
                unsigned char *active_out;

                set_hash_addr(addrx4 + lane * 8, k);
                memcpy(step_addrx4 + lane * 8, addrx4 + lane * 8, 8 * sizeof(uint32_t));

                switch (lane) {
                    case 0: active_out = out0; break;
                    case 1: active_out = out1; break;
                    case 2: active_out = out2; break;
                    default: active_out = out3; break;
                }

                lane_out[lane] = active_out;
                lane_in[lane] = active_out;
            } else {
                memcpy(step_addrx4 + lane * 8, addrx4 + lane * 8, 8 * sizeof(uint32_t));
            }
        }

        thashx4(lane_out[0], lane_out[1], lane_out[2], lane_out[3],
                lane_in[0], lane_in[1], lane_in[2], lane_in[3],
                1, ctx, step_addrx4);
    }
}

void gen_leafx4(unsigned char *dest,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                void *v_info)
{
    struct leaf_info_x4 *info = v_info;
    unsigned char pk0[SPX_GWOTS_BYTES];
    unsigned char pk1[SPX_GWOTS_BYTES];
    unsigned char pk2[SPX_GWOTS_BYTES];
    unsigned char pk3[SPX_GWOTS_BYTES];
    unsigned char chain0[SPX_N];
    unsigned char chain1[SPX_N];
    unsigned char chain2[SPX_N];
    unsigned char chain3[SPX_N];
    uint32_t gwots_k_mask[4];

    for (uint32_t lane = 0; lane < 4; lane++) {
        uint32_t lane_leaf = leaf_idx + lane;
        uint32_t *leaf_addr = info->leaf_addr + lane * 8;
        uint32_t *pk_addr = info->pk_addr + lane * 8;

        gwots_k_mask[lane] = (lane_leaf == info->gwots_sign_leaf) ? 0u : ~0u;
        set_keypair_addr(leaf_addr, lane_leaf);
        set_keypair_addr(pk_addr, lane_leaf);
    }

    for (uint32_t i = 0; i < SPX_GWOTS_LEN; i++) {
        unsigned int starts[4] = {0, 0, 0, 0};
        unsigned int full_steps[4];
        unsigned int sig_steps[4] = {0, 0, 0, 0};
        unsigned char sig0[SPX_N];
        unsigned char sig1[SPX_N];
        unsigned char sig2[SPX_N];
        unsigned char sig3[SPX_N];
        uint32_t w;
        int have_sig_lane = 0;

        if (i < SPX_GWOTS_W1_LEN) {
            w = SPX_GWOTS_W1;
        } else if (i < SPX_GWOTS_LEN1) {
            w = SPX_GWOTS_W2;
        } else {
            w = SPX_GWOTS_CHECKSUM_W;
        }

        for (uint32_t lane = 0; lane < 4; lane++) {
            uint32_t *leaf_addr = info->leaf_addr + lane * 8;
            full_steps[lane] = w - 1;
            set_chain_addr(leaf_addr, i);
            set_hash_addr(leaf_addr, 0);
            set_type(leaf_addr, SPX_ADDR_TYPE_GWOTSPRF);

            if (gwots_k_mask[lane] == 0u && info->gwots_sig != 0) {
                sig_steps[lane] = info->gwots_steps[i];
                have_sig_lane = 1;
            }
        }

        prf_addrx4(chain0, chain1, chain2, chain3,
                   ctx, info->leaf_addr);

        for (uint32_t lane = 0; lane < 4; lane++) {
            set_type(info->leaf_addr + lane * 8, SPX_ADDR_TYPE_GWOTS);
        }

        if (have_sig_lane) {
            chainx4(sig0, sig1, sig2, sig3,
                    chain0, chain1, chain2, chain3,
                    starts, sig_steps, ctx, info->leaf_addr, w);

            for (uint32_t lane = 0; lane < 4; lane++) {
                if (gwots_k_mask[lane] == 0u && info->gwots_sig != 0) {
                    const unsigned char *sig_src;

                    switch (lane) {
                        case 0: sig_src = sig0; break;
                        case 1: sig_src = sig1; break;
                        case 2: sig_src = sig2; break;
                        default: sig_src = sig3; break;
                    }

                    memcpy(info->gwots_sig + i * SPX_N, sig_src, SPX_N);
                }
            }
        }

        chainx4(chain0, chain1, chain2, chain3,
                chain0, chain1, chain2, chain3,
                starts, full_steps, ctx, info->leaf_addr, w);

        memcpy(pk0 + i * SPX_N, chain0, SPX_N);
        memcpy(pk1 + i * SPX_N, chain1, SPX_N);
        memcpy(pk2 + i * SPX_N, chain2, SPX_N);
        memcpy(pk3 + i * SPX_N, chain3, SPX_N);
    }

    thashx4(dest + 0 * SPX_N,
            dest + 1 * SPX_N,
            dest + 2 * SPX_N,
            dest + 3 * SPX_N,
            pk0, pk1, pk2, pk3,
            SPX_GWOTS_LEN, ctx, info->pk_addr);
}

void treehashx4(unsigned char *root,
                unsigned char *auth_path,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                uint32_t idx_offset,
                uint32_t tree_height,
                void (*gen_leafx4_fn)(
                    unsigned char *,
                    const spx_ctx *,
                    uint32_t,
                    void *),
                uint32_t tree_addrx4[4 * 8],
                void *info)
{
    SPX_VLA(unsigned char, stackx4, tree_height * 4 * SPX_N);
    uint32_t left_adj = 0;
    uint32_t prev_left_adj = 0;
    uint32_t idx;
    uint32_t max_idx = (uint32_t)((1u << (tree_height - 2)) - 1u);

    for (idx = 0;; idx++) {
        unsigned char current[4 * SPX_N];
        uint32_t internal_idx_offset = idx_offset;
        uint32_t internal_idx = idx;
        uint32_t internal_leaf = leaf_idx;
        uint32_t h;

        gen_leafx4_fn(current, ctx, 4 * idx + idx_offset, info);

        for (h = 0;; h++, internal_idx >>= 1, internal_leaf >>= 1) {
            if (h >= tree_height - 2) {
                if (h == tree_height) {
                    memcpy(root, &current[3 * SPX_N], SPX_N);
                    return;
                }

                prev_left_adj = left_adj;
                left_adj = 4u - (1u << (tree_height - h - 1));
            }

            if ((((internal_idx << 2) ^ internal_leaf) & ~0x3u) == 0) {
                memcpy(&auth_path[h * SPX_N],
                       &current[(((internal_leaf & 0x3u) ^ 0x1u) + prev_left_adj) * SPX_N],
                       SPX_N);
            }

            if ((internal_idx & 1u) == 0u && idx < max_idx) {
                break;
            }

            internal_idx_offset >>= 1;
            for (int j = 0; j < 4; j++) {
                set_tree_height(tree_addrx4 + j * 8, h + 1);
                set_tree_index(tree_addrx4 + j * 8,
                               2u * (internal_idx & ~1u) + (uint32_t)j - left_adj + internal_idx_offset);
            }

            {
                unsigned char *left = &stackx4[h * 4 * SPX_N];
                thashx4(&current[0 * SPX_N],
                        &current[1 * SPX_N],
                        &current[2 * SPX_N],
                        &current[3 * SPX_N],
                        &left[0 * SPX_N],
                        &left[2 * SPX_N],
                        &current[0 * SPX_N],
                        &current[2 * SPX_N],
                        2, ctx, tree_addrx4);
            }
        }

        memcpy(&stackx4[h * 4 * SPX_N], current, 4 * SPX_N);
    }
}