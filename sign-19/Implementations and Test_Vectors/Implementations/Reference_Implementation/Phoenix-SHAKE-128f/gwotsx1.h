#if !defined( GWOTSX1_H_ )
#define GWOTSX1_H_ 

#include <string.h>

/*
 * This is here to provide an interface to the internal gwots_gen_leafx1
 * routine.  While this routine is not referenced in the package outside of
 * gwots.c, it is called from the stand-alone benchmark code to characterize
 * the performance
 */
struct leaf_info_x1 {
    unsigned char *gwots_sig;
    uint32_t gwots_sign_leaf; /* The index of the GWOTS we're using to sign */
    uint32_t *gwots_steps;
    uint32_t leaf_addr[8];
    uint32_t pk_addr[8];
};

/* Macro to set the leaf_info to something 'benign', that is, it would */
/* run with the same time as it does during the real signing process */
/* Used only by the benchmark code */
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



