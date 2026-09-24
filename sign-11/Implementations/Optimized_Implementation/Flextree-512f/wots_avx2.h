#if !defined(WOTS_AVX2_H_)
#define WOTS_AVX2_H_

#include <string.h>
#include "params.h"
#include "context.h"

struct leaf_info_avx2 {
    unsigned char *wots_sig;
    uint32_t wots_sign_leaf;
    uint32_t *wots_steps;
    uint32_t leaf_addr[4*8];
    uint32_t pk_addr[4*8];
};

#define INITIALIZE_LEAF_INFO_AVX2(info, addr, step_buffer) { \
    info.wots_sig = 0;             \
    info.wots_sign_leaf = ~0;      \
    info.wots_steps = step_buffer; \
    int i;                         \
    for (i=0; i<4; i++) {          \
        memcpy( &info.leaf_addr[8*i], addr, 32 ); \
        memcpy( &info.pk_addr[8*i], addr, 32 ); \
    } \
}

void wots_gen_leaf_avx2(unsigned char *dest,
                        const spx_ctx *ctx,
                        uint32_t leaf_idx, void *v_info);

#endif /* WOTS_AVX2_H_ */
