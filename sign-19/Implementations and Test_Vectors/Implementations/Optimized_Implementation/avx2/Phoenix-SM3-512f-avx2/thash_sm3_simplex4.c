#include <stdint.h>
#include <string.h>

#include "thashx4.h"
#include "address.h"
#include "params.h"
#include "utils.h"
#include "hash_sm3.h"
#include "pseudoXOF_avx2.h"

void thashx4(unsigned char *out0,
              unsigned char *out1,
              unsigned char *out2,
              unsigned char *out3,
              const unsigned char *in0,
              const unsigned char *in1,
              const unsigned char *in2,
              const unsigned char *in3,
              unsigned int inblocks,
              const spx_ctx *ctx,
              uint32_t addrx4[4 * 8])
{
    SPX_VLA(uint8_t, buf0, SPX_SM3_ADDR_BYTES + inblocks * SPX_N);
    SPX_VLA(uint8_t, buf1, SPX_SM3_ADDR_BYTES + inblocks * SPX_N);
    SPX_VLA(uint8_t, buf2, SPX_SM3_ADDR_BYTES + inblocks * SPX_N);
    SPX_VLA(uint8_t, buf3, SPX_SM3_ADDR_BYTES + inblocks * SPX_N);

    memcpy(buf0, addrx4 + 0 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf1, addrx4 + 1 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf2, addrx4 + 2 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf3, addrx4 + 3 * 8, SPX_SM3_ADDR_BYTES);

    memcpy(buf0 + SPX_SM3_ADDR_BYTES, in0, inblocks * SPX_N);
    memcpy(buf1 + SPX_SM3_ADDR_BYTES, in1, inblocks * SPX_N);
    memcpy(buf2 + SPX_SM3_ADDR_BYTES, in2, inblocks * SPX_N);
    memcpy(buf3 + SPX_SM3_ADDR_BYTES, in3, inblocks * SPX_N);

    pseudoXOFx4_avx2_aligned(
        out0, out1, out2, out3,
        SPX_N * 8,
        buf0, buf1, buf2, buf3,
        (SPX_SM3_ADDR_BYTES + inblocks * SPX_N) * 8);
}