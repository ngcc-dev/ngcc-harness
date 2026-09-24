#include <stdint.h>
#include <string.h>

#include "address.h"
#include "hashx8.h"
#include "params.h"
#include "thashx8.h"
#include "utils.h"
#include "utilsx8.h"

static uint32_t chain_limit(unsigned int start, unsigned int steps, uint32_t w)
{
    uint32_t end = start + steps;

    if (end < start || end > w) {
        end = w;
    }

    return end;
}

void chainx8(unsigned char *out0,
             unsigned char *out1,
             unsigned char *out2,
             unsigned char *out3,
             unsigned char *out4,
             unsigned char *out5,
             unsigned char *out6,
             unsigned char *out7,
             const unsigned char *in0,
             const unsigned char *in1,
             const unsigned char *in2,
             const unsigned char *in3,
             const unsigned char *in4,
             const unsigned char *in5,
             const unsigned char *in6,
             const unsigned char *in7,
             const unsigned int start[8],
             const unsigned int steps[8],
             const spx_ctx *ctx,
             uint32_t addrx8[8 * 8],
             uint32_t w)
{
    uint32_t ends[8];
    uint32_t max_end = 0;
    uint32_t k;
    unsigned char dummy0[SPX_N] = {0};
    unsigned char dummy1[SPX_N] = {0};
    unsigned char dummy2[SPX_N] = {0};
    unsigned char dummy3[SPX_N] = {0};
    unsigned char dummy4[SPX_N] = {0};
    unsigned char dummy5[SPX_N] = {0};
    unsigned char dummy6[SPX_N] = {0};
    unsigned char dummy7[SPX_N] = {0};
    uint32_t step_addrx8[8 * 8];

    memcpy(out0, in0, SPX_N);
    memcpy(out1, in1, SPX_N);
    memcpy(out2, in2, SPX_N);
    memcpy(out3, in3, SPX_N);
    memcpy(out4, in4, SPX_N);
    memcpy(out5, in5, SPX_N);
    memcpy(out6, in6, SPX_N);
    memcpy(out7, in7, SPX_N);

    for (k = 0; k < 8; k++) {
        ends[k] = chain_limit(start[k], steps[k], w);
        if (ends[k] > max_end) {
            max_end = ends[k];
        }
    }

    for (k = 0; k < max_end; k++) {
        unsigned char *lane_out[8] = {dummy0, dummy1, dummy2, dummy3,
                                      dummy4, dummy5, dummy6, dummy7};
        const unsigned char *lane_in[8] = {dummy0, dummy1, dummy2, dummy3,
                                           dummy4, dummy5, dummy6, dummy7};

        memset(step_addrx8, 0, sizeof(step_addrx8));

        for (uint32_t lane = 0; lane < 8; lane++) {
            if (k >= start[lane] && k < ends[lane]) {
                unsigned char *active_out;

                set_hash_addr(addrx8 + lane * 8, k);
                memcpy(step_addrx8 + lane * 8, addrx8 + lane * 8, 8 * sizeof(uint32_t));

                switch (lane) {
                    case 0: active_out = out0; break;
                    case 1: active_out = out1; break;
                    case 2: active_out = out2; break;
                    case 3: active_out = out3; break;
                    case 4: active_out = out4; break;
                    case 5: active_out = out5; break;
                    case 6: active_out = out6; break;
                    default: active_out = out7; break;
                }

                lane_out[lane] = active_out;
                lane_in[lane] = active_out;
            } else {
                memcpy(step_addrx8 + lane * 8, addrx8 + lane * 8, 8 * sizeof(uint32_t));
            }
        }

        thashx8(lane_out[0], lane_out[1], lane_out[2], lane_out[3],
                lane_out[4], lane_out[5], lane_out[6], lane_out[7],
                lane_in[0], lane_in[1], lane_in[2], lane_in[3],
                lane_in[4], lane_in[5], lane_in[6], lane_in[7],
                1, ctx, step_addrx8);
    }
}

void gen_leafx8(unsigned char *dest,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                void *v_info)
{
    struct leaf_info_x8 *info = v_info;
    unsigned char pk0[SPX_GWOTS_BYTES];
    unsigned char pk1[SPX_GWOTS_BYTES];
    unsigned char pk2[SPX_GWOTS_BYTES];
    unsigned char pk3[SPX_GWOTS_BYTES];
    unsigned char pk4[SPX_GWOTS_BYTES];
    unsigned char pk5[SPX_GWOTS_BYTES];
    unsigned char pk6[SPX_GWOTS_BYTES];
    unsigned char pk7[SPX_GWOTS_BYTES];
    unsigned char chain0[SPX_N];
    unsigned char chain1[SPX_N];
    unsigned char chain2[SPX_N];
    unsigned char chain3[SPX_N];
    unsigned char chain4[SPX_N];
    unsigned char chain5[SPX_N];
    unsigned char chain6[SPX_N];
    unsigned char chain7[SPX_N];
    uint32_t gwots_k_mask[8];

    for (uint32_t lane = 0; lane < 8; lane++) {
        uint32_t lane_leaf = leaf_idx + lane;
        uint32_t *leaf_addr = info->leaf_addr + lane * 8;
        uint32_t *pk_addr = info->pk_addr + lane * 8;

        gwots_k_mask[lane] = (lane_leaf == info->gwots_sign_leaf) ? 0u : ~0u;
        set_keypair_addr(leaf_addr, lane_leaf);
        set_keypair_addr(pk_addr, lane_leaf);
    }

    for (uint32_t i = 0; i < SPX_GWOTS_LEN; i++) {
        unsigned int starts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        unsigned int full_steps[8];
        unsigned int sig_steps[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        unsigned char sig0[SPX_N];
        unsigned char sig1[SPX_N];
        unsigned char sig2[SPX_N];
        unsigned char sig3[SPX_N];
        unsigned char sig4[SPX_N];
        unsigned char sig5[SPX_N];
        unsigned char sig6[SPX_N];
        unsigned char sig7[SPX_N];
        uint32_t w;
        int have_sig_lane = 0;

        if (i < SPX_GWOTS_W1_LEN) {
            w = SPX_GWOTS_W1;
        } else if (i < SPX_GWOTS_LEN1) {
            w = SPX_GWOTS_W2;
        } else {
            w = SPX_GWOTS_CHECKSUM_W;
        }

        for (uint32_t lane = 0; lane < 8; lane++) {
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

        prf_addrx8(chain0, chain1, chain2, chain3,
                   chain4, chain5, chain6, chain7,
                   ctx, info->leaf_addr);

        for (uint32_t lane = 0; lane < 8; lane++) {
            set_type(info->leaf_addr + lane * 8, SPX_ADDR_TYPE_GWOTS);
        }

        if (have_sig_lane) {
            chainx8(sig0, sig1, sig2, sig3,
                    sig4, sig5, sig6, sig7,
                    chain0, chain1, chain2, chain3,
                    chain4, chain5, chain6, chain7,
                    starts, sig_steps, ctx, info->leaf_addr, w);

            for (uint32_t lane = 0; lane < 8; lane++) {
                if (gwots_k_mask[lane] == 0u && info->gwots_sig != 0) {
                    const unsigned char *sig_src;

                    switch (lane) {
                        case 0: sig_src = sig0; break;
                        case 1: sig_src = sig1; break;
                        case 2: sig_src = sig2; break;
                        case 3: sig_src = sig3; break;
                        case 4: sig_src = sig4; break;
                        case 5: sig_src = sig5; break;
                        case 6: sig_src = sig6; break;
                        default: sig_src = sig7; break;
                    }

                    memcpy(info->gwots_sig + i * SPX_N, sig_src, SPX_N);
                }
            }
        }

        chainx8(chain0, chain1, chain2, chain3,
                chain4, chain5, chain6, chain7,
                chain0, chain1, chain2, chain3,
                chain4, chain5, chain6, chain7,
                starts, full_steps, ctx, info->leaf_addr, w);

        memcpy(pk0 + i * SPX_N, chain0, SPX_N);
        memcpy(pk1 + i * SPX_N, chain1, SPX_N);
        memcpy(pk2 + i * SPX_N, chain2, SPX_N);
        memcpy(pk3 + i * SPX_N, chain3, SPX_N);
        memcpy(pk4 + i * SPX_N, chain4, SPX_N);
        memcpy(pk5 + i * SPX_N, chain5, SPX_N);
        memcpy(pk6 + i * SPX_N, chain6, SPX_N);
        memcpy(pk7 + i * SPX_N, chain7, SPX_N);
    }

    thashx8(dest + 0 * SPX_N,
            dest + 1 * SPX_N,
            dest + 2 * SPX_N,
            dest + 3 * SPX_N,
            dest + 4 * SPX_N,
            dest + 5 * SPX_N,
            dest + 6 * SPX_N,
            dest + 7 * SPX_N,
            pk0, pk1, pk2, pk3,
            pk4, pk5, pk6, pk7,
            SPX_GWOTS_LEN, ctx, info->pk_addr);
}

void treehashx8(unsigned char *root,
                unsigned char *auth_path,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                uint32_t idx_offset,
                uint32_t tree_height,
                void (*gen_leafx8_fn)(
                    unsigned char *,
                    const spx_ctx *,
                    uint32_t,
                    void *),
                uint32_t tree_addrx8[8 * 8],
                void *info)
{
    SPX_VLA(unsigned char, stackx8, tree_height * 8 * SPX_N);
    uint32_t left_adj = 0;
    uint32_t prev_left_adj = 0;
    uint32_t idx;
    uint32_t max_idx = (uint32_t)((1u << (tree_height - 2)) - 1u);

    for (idx = 0;; idx++) {
        unsigned char current[8 * SPX_N];
        uint32_t internal_idx_offset = idx_offset;
        uint32_t internal_idx = idx;
        uint32_t internal_leaf = leaf_idx;
        uint32_t h;

        gen_leafx8_fn(current, ctx, 8 * idx + idx_offset, info);

        for (h = 0;; h++, internal_idx >>= 1, internal_leaf >>= 1) {
            if (h >= tree_height - 2) {
                if (h == tree_height) {
                    memcpy(root, &current[7 * SPX_N], SPX_N);
                    return;
                }

                prev_left_adj = left_adj;
                left_adj = 8u - (1u << (tree_height - h - 1));
            }

            if ((((internal_idx << 3) ^ internal_leaf) & ~0x7u) == 0) {
                memcpy(&auth_path[h * SPX_N],
                       &current[(((internal_leaf & 0x7u) ^ 0x1u) + prev_left_adj) * SPX_N],
                       SPX_N);
            }

            if ((internal_idx & 1u) == 0u && idx < max_idx) {
                break;
            }

            internal_idx_offset >>= 1;
            for (int j = 0; j < 8; j++) {
                set_tree_height(tree_addrx8 + j * 8, h + 1);
                set_tree_index(tree_addrx8 + j * 8,
                               2u * (internal_idx & ~1u) + (uint32_t)j - left_adj + internal_idx_offset);
            }

            {
                unsigned char *left = &stackx8[h * 8 * SPX_N];
                thashx8(&current[0 * SPX_N],
                        &current[1 * SPX_N],
                        &current[2 * SPX_N],
                        &current[3 * SPX_N],
                        &current[4 * SPX_N],
                        &current[5 * SPX_N],
                        &current[6 * SPX_N],
                        &current[7 * SPX_N],
                        &left[0 * SPX_N],
                        &left[2 * SPX_N],
                        &left[4 * SPX_N],
                        &left[6 * SPX_N],
                        &current[0 * SPX_N],
                        &current[2 * SPX_N],
                        &current[4 * SPX_N],
                        &current[6 * SPX_N],
                        2, ctx, tree_addrx8);
            }
        }

        memcpy(&stackx8[h * 8 * SPX_N], current, 8 * SPX_N);
    }
}