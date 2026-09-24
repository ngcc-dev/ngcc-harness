#ifndef SPX_UTILSX4_H
#define SPX_UTILSX4_H

#include <stdint.h>
#include <string.h>

#include "context.h"
#include "params.h"

struct leaf_info_x4 {
    unsigned char *gwots_sig;
    uint32_t gwots_sign_leaf;
    uint32_t *gwots_steps;
    uint32_t leaf_addr[4 * 8];
    uint32_t pk_addr[4 * 8];
};

#define INITIALIZE_LEAF_INFO_X4(info, addr, step_buffer) { \
    (info).gwots_sig = 0; \
    (info).gwots_sign_leaf = ~0u; \
    (info).gwots_steps = (step_buffer); \
    for (int i = 0; i < 4; i++) { \
        memcpy(&(info).leaf_addr[8 * i], (addr), 32); \
        memcpy(&(info).pk_addr[8 * i], (addr), 32); \
    } \
}

#define chainx4 SPX_NAMESPACE(chainx4)
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
              uint32_t w);

#define gen_leafx4 SPX_NAMESPACE(gen_leafx4)
void gen_leafx4(unsigned char *dest,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                void *v_info);

#define treehashx4 SPX_NAMESPACE(treehashx4)
void treehashx4(unsigned char *root,
                unsigned char *auth_path,
                const spx_ctx *ctx,
                uint32_t leaf_idx,
                uint32_t idx_offset,
                uint32_t tree_height,
                void (*gen_leafx4)(
                    unsigned char *,
                    const spx_ctx *,
                    uint32_t,
                    void *),
                uint32_t tree_addrx4[4 * 8],
                void *info);

#endif