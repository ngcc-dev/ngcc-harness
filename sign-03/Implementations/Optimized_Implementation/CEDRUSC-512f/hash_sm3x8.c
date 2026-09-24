#include <stdint.h>
#include <string.h>

#include "address.h"
#include "auxfunc.h"
#include "hashx8.h"
#include "params.h"

static int same_addr(const uint32_t a[8], const uint32_t b[8])
{
    return memcmp(a, b, SPX_ADDR_BYTES) == 0;
}

static int prf_addr_lane(unsigned char *out,
                         const spx_ctx *ctx,
                         const uint32_t addr[8])
{
    unsigned char buf[2 * SPX_N + SPX_ADDR_BYTES];

    memcpy(buf, ctx->pub_seed, SPX_N);
    memcpy(buf + SPX_N, addr, SPX_ADDR_BYTES);
    memcpy(buf + SPX_N + SPX_ADDR_BYTES, ctx->sk_seed, SPX_N);

    if (SPX_N <= 32) {
        unsigned char temp[32];
        int rc = sm3hash(256, buf,
                         (unsigned long long)sizeof(buf) * 8u, temp);

        if (rc == 0) {
            memcpy(out, temp, SPX_N);
        }
        return rc;
    }

    return pseudoXOF((unsigned long long)SPX_N * 8u,
                     buf, (unsigned long long)sizeof(buf) * 8u, out);
}

void prf_addrx8(unsigned char *out0,
                unsigned char *out1,
                unsigned char *out2,
                unsigned char *out3,
                unsigned char *out4,
                unsigned char *out5,
                unsigned char *out6,
                unsigned char *out7,
                const spx_ctx *ctx,
                const uint32_t addrx8[8*8])
{
    unsigned char *out[8] = {out0, out1, out2, out3, out4, out5, out6, out7};
    int lane_rc[8] = {0};

    for (unsigned int i = 0; i < 8; i++) {
        const uint32_t *addr = addrx8 + 8 * i;
        unsigned int src = i;

        for (unsigned int j = 0; j < i; j++) {
            if (lane_rc[j] == 0 && same_addr(addr, addrx8 + 8 * j)) {
                src = j;
                break;
            }
        }

        if (src != i) {
            if (out[i] != out[src]) {
                memcpy(out[i], out[src], SPX_N);
            }
            continue;
        }

        lane_rc[i] = prf_addr_lane(out[i], ctx, addr);
    }
}

void prf_addrx8_tree_index(unsigned char *out0,
                           unsigned char *out1,
                           unsigned char *out2,
                           unsigned char *out3,
                           unsigned char *out4,
                           unsigned char *out5,
                           unsigned char *out6,
                           unsigned char *out7,
                           const spx_ctx *ctx,
                           const uint32_t addr_base[8],
                           uint32_t tree_index)
{
    uint32_t addrx8[8 * 8];

    for (unsigned int j = 0; j < 8; j++) {
        memcpy(addrx8 + 8 * j, addr_base, 8 * sizeof(uint32_t));
        set_tree_index(addrx8 + 8 * j, tree_index + j);
    }

    prf_addrx8(out0, out1, out2, out3,
               out4, out5, out6, out7,
               ctx, addrx8);
}
