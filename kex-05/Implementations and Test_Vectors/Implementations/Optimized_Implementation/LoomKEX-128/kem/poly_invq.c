/**
 * poly_invq.c — WEAVER-Inv: Randomized Lifting via Inv_q
 *
 * Lemire 拒绝采样：coeffs 已由 polyvec_fromcompressed_pk 填入桶编号 y，
 * 用 PRF 随机字节提升为 Z_q 代表元 bucket_lo[y] + t。
 */

#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "invq.h"
#include "symmetric.h"

#if (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 8 / 8))
#define WEAVER_DT 8
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 9 / 8))
#define WEAVER_DT 9
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 10 / 8))
#define WEAVER_DT 10
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 11 / 8))
#define WEAVER_DT 11
#else
#error "Unsupported public-key compression width"
#endif

#if WEAVER_DT == 9
#include "invq_table_d9.h"
#define BUCKET_LO invq_d9_bucket_lo
#define BUCKET_SZ invq_d9_bucket_size
#elif WEAVER_DT == 10
#include "invq_table_d10.h"
#define BUCKET_LO invq_d10_bucket_lo
#define BUCKET_SZ invq_d10_bucket_size
#elif WEAVER_DT == 11
#include "invq_table_d11.h"
#define BUCKET_LO invq_d11_bucket_lo
#define BUCKET_SZ invq_d11_bucket_size
#else
#error "Inv_q table not available for this compression width"
#endif

#if WEAVER_N == 128
#define GEN_INVQ_RAND_BYTES SHAKE256_RATE
#elif WEAVER_N == 256
#define GEN_INVQ_RAND_BYTES SHAKE256_RATE
#elif WEAVER_N == 512
#define GEN_INVQ_RAND_BYTES (2 * SHAKE256_RATE)
#else
#error "Unsupported WEAVER_N for invq PRF buffer"
#endif

static unsigned int rej_uniform(int16_t *r,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen)
{
    unsigned int ctr, pos, j;
    uint8_t t[8];

    ctr = pos = 0;
    while(ctr < len && pos + 3 <= buflen) {
        t[0] = (buf[pos + 0] >> 0) & 0x7;
        t[1] = (buf[pos + 0] >> 3) & 0x7;
        t[2] = ((buf[pos + 0] >> 6) | (buf[pos + 1] << 2)) & 0x7;
        t[3] = (buf[pos + 1] >> 1) & 0x7;
        t[4] = (buf[pos + 1] >> 4) & 0x7;
        t[5] = ((buf[pos + 1] >> 7) | (buf[pos + 2] << 1)) & 0x7;
        t[6] = (buf[pos + 2] >> 2) & 0x7;
        t[7] = (buf[pos + 2] >> 5) & 0x7;
        pos += 3;

        for(j = 0; j < 8; j++) {
            if(ctr >= len)
                break;
            if(t[j] < BUCKET_SZ[r[ctr]]) {
                unsigned int c = ctr;
                int16_t bidx = r[c];
                r[c] = (int16_t)(BUCKET_LO[bidx] + t[j]);
                ctr = c + 1;
            }
        }
    }

    return ctr;
}

void polyvec_invq(polyvec *v,
                  const uint8_t seed[WEAVER_SYMBYTES],
                  uint8_t nonce)
{
    unsigned int ctr, i;
    unsigned int buflen;
    uint8_t buf[GEN_INVQ_RAND_BYTES];

    for(i = 0; i < WEAVER_K; i++) {
        prf(buf, GEN_INVQ_RAND_BYTES, seed, nonce++);
        buflen = GEN_INVQ_RAND_BYTES;
        ctr = rej_uniform(v->vec[i].coeffs, WEAVER_N, buf, buflen);

        while(ctr < WEAVER_N) {
            prf(buf, GEN_INVQ_RAND_BYTES, seed, nonce++);
            buflen = GEN_INVQ_RAND_BYTES;
            ctr += rej_uniform(v->vec[i].coeffs + ctr, WEAVER_N - ctr, buf, buflen);
        }
    }
}
