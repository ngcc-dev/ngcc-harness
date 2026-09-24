/* Phoenix-GWOTSx1 interface
 *
 * Internal leaf generation interface for 1-way GWOTS operations.
 */

#if !defined( GWOTSX1_H_ )
#define GWOTSX1_H_

#include <string.h>

/* Leaf info for GWOTS signature generation and PK computation */
struct leaf_info_x1 {
    unsigned char *gwots_sig;
    uint32_t gwots_sign_leaf;
    uint32_t *gwots_steps;
    uint32_t leaf_addr[8];
    uint32_t pk_addr[8];
};

/* Initialize leaf_info for timing-equivalent benchmark mode */
#define INITIALIZE_LEAF_INFO_X1(info, addr, step_buffer) { \
    info.gwots_sig = 0;             \
    info.gwots_sign_leaf = ~0u;      \
    info.gwots_steps = step_buffer; \
    memcpy( &info.leaf_addr[0], addr, 32 ); \
    memcpy( &info.pk_addr[0], addr, 32 ); \
}

#define gwots_gen_leafx1 SPX_NAMESPACE(gwots_gen_leafx1)
void gwots_gen_leafx1(unsigned char *dest,
                   const spx_ctx *ctx,
                   uint32_t leaf_idx, void *v_info);

#endif /* GWOTSX1_H_ */
