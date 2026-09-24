#ifndef SPX_UTILSX8_H
#define SPX_UTILSX8_H

#include <stdint.h>
#include <string.h>

#include "context.h"
#include "params.h"

struct leaf_info_x8 {
    unsigned char *gwots_sig;
    uint32_t gwots_sign_leaf;
    uint32_t *gwots_steps;
    uint32_t leaf_addr[8 * 8];
    uint32_t pk_addr[8 * 8];
};

#define INITIALIZE_LEAF_INFO_X8(info, addr, step_buffer) { \
    (info).gwots_sig = 0; \
    (info).gwots_sign_leaf = ~0u; \
    (info).gwots_steps = (step_buffer); \
    for (int i = 0; i < 8; i++) { \
        memcpy(&(info).leaf_addr[8 * i], (addr), 32); \
        memcpy(&(info).pk_addr[8 * i], (addr), 32); \
    } \
}

#define chainx8 SPX_NAMESPACE(chainx8)
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
             uint32_t w);

#define gen_leafx8 SPX_NAMESPACE(gen_leafx8)
void gen_leafx8(unsigned char *dest,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                void *v_info);

#define treehashx8 SPX_NAMESPACE(treehashx8)
void treehashx8(unsigned char *root,
                unsigned char *auth_path,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                uint32_t idx_offset,
                uint32_t tree_height,
                void (*gen_leafx8)(
                    unsigned char *,
                    const spx_ctx *,
                    uint32_t,
                    void *),
                uint32_t tree_addrx8[8 * 8],
                void *info);

#endif