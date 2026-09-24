#include "sampler.h"
#include "fixpoint.h"
#include "symmetric.h"
#include <stdint.h>

/*************************************************
 * Name:        rej_uniform
 *
 * Description: Sample uniformly random coefficients in [0, Q-1] by
 *              performing rejection sampling on array of random bytes.
 *
 * Arguments:   - int32_t *a: pointer to output array (allocated)
 *              - unsigned int len: number of coefficients to be sampled
 *              - const uint8_t *buf: array of random bytes
 *              - unsigned int buflen: length of array of random bytes
 *
 * Returns number of sampled coefficients. Can be smaller than len 
 * if not enough random bytes were given.
 **************************************************/
unsigned int rej_uniform(int32_t *a, unsigned int len,
                         const uint8_t *buf, unsigned int buflen)
{
    unsigned int ctr = 0, pos = 0;

#if Q_BITS == 18
    while (ctr < len && pos + 9 <= buflen) {
        uint32_t t0 =  (uint32_t)buf[pos]
                      | ((uint32_t)buf[pos+1] << 8)
                      | (((uint32_t)buf[pos+2] & 0x03) << 16);

        uint32_t t1 =  ((uint32_t)buf[pos+2] >> 2)
                      | ((uint32_t)buf[pos+3] << 6)
                      | (((uint32_t)buf[pos+4] & 0x0F) << 14);

        uint32_t t2 =  ((uint32_t)buf[pos+4] >> 4)
                      | ((uint32_t)buf[pos+5] << 4)
                      | (((uint32_t)buf[pos+6] & 0x3F) << 12);

        uint32_t t3 =  ((uint32_t)buf[pos+6] >> 6)
                      | ((uint32_t)buf[pos+7] << 2)
                      | ((uint32_t)buf[pos+8] << 10);

        pos += 9;

        if (t0 < Q && ctr < len) a[ctr++] = (int32_t)t0;
        if (t1 < Q && ctr < len) a[ctr++] = (int32_t)t1;
        if (t2 < Q && ctr < len) a[ctr++] = (int32_t)t2;
        if (t3 < Q && ctr < len) a[ctr++] = (int32_t)t3;
    }

#elif Q_BITS == 17
    while (ctr < len && pos + 17 <= buflen) {
        const uint8_t *b = buf + pos;
        uint32_t t[8];

        t[0] =  (uint32_t)b[0]        | ((uint32_t)b[1]  << 8) | (((uint32_t)b[2]  & 0x01) << 16);
        t[1] = ((uint32_t)b[2]  >> 1) | ((uint32_t)b[3]  << 7) | (((uint32_t)b[4]  & 0x03) << 15);
        t[2] = ((uint32_t)b[4]  >> 2) | ((uint32_t)b[5]  << 6) | (((uint32_t)b[6]  & 0x07) << 14);
        t[3] = ((uint32_t)b[6]  >> 3) | ((uint32_t)b[7]  << 5) | (((uint32_t)b[8]  & 0x0F) << 13);
        t[4] = ((uint32_t)b[8]  >> 4) | ((uint32_t)b[9]  << 4) | (((uint32_t)b[10] & 0x1F) << 12);
        t[5] = ((uint32_t)b[10] >> 5) | ((uint32_t)b[11] << 3) | (((uint32_t)b[12] & 0x3F) << 11);
        t[6] = ((uint32_t)b[12] >> 6) | ((uint32_t)b[13] << 2) | (((uint32_t)b[14] & 0x7F) << 10);
        t[7] = ((uint32_t)b[14] >> 7) | ((uint32_t)b[15] << 1) | ((uint32_t)b[16]  << 9);

        pos += 17;

        for (unsigned int i = 0; i < 8 && ctr < len; i++) {
            if (t[i] < Q)
                a[ctr++] = (int32_t)t[i];
        }
    }

#else
    #error "Q_BITS must be 17 or 18"
#endif
    {
        uint32_t acc = 0;
        unsigned int acc_bits = 0;
        const uint32_t mask = (1u << Q_BITS) - 1u;

        while (ctr < len) {
            while (acc_bits < Q_BITS && pos < buflen) {
                acc |= ((uint32_t)buf[pos++]) << acc_bits;
                acc_bits += 8;
            }
            if (acc_bits < Q_BITS) break;

            uint32_t t = acc & mask;
            acc >>= Q_BITS;
            acc_bits -= Q_BITS;

            if (t < Q)
                a[ctr++] = (int32_t)t;
        }
    }

    return ctr;
}

/*************************************************
 * Name:        rej_p
 *
 * Description: Sample coefficients in {-1,0,1} with
 *              Pr[X = 1] = Pr[X = -1] = P, Pr[X = 0] = 1 - 2P
 *              by performing rejection sampling on array of random bytes.
 *
 * Arguments:   - int32_t *a: pointer to output array (allocated)
 *              - unsigned int len: number of coefficients to be sampled
 *              - const uint8_t *buf: array of random bytes
 *              - unsigned int buflen: length of array of random bytes
 *
 * Returns number of sampled coefficients. Can be smaller than len 
 * if not enough random bytes were given.
 **************************************************/
static inline int32_t floor10(uint16_t t) {
    return (int32_t)((t * 205u) >> 11);
}

static inline int32_t mod10(uint16_t t) {
    int32_t r;
    r = (t & 31) + ((t >> 5) << 1);
    r = (r & 31) + ((r >> 5) << 1);
    r = (r & 15) + ((r >> 4) * 6);
    r -= 10 * (r >= 10);
    return r - 10 * (r >= 10);
}

unsigned int rej_p(int32_t *a, unsigned int len,
                   const uint8_t *buf, unsigned int buflen)
{
    unsigned int ctr = 0, pos = 0, acc_bits = 0;
    uint32_t acc = 0;

    if (P == 0.15 || P == 0.2)
    {
        const int32_t thresh = (P == 0.15) ? 3 : 4;

        while (ctr < len) {
            while (acc_bits < 13 && pos < buflen) {
                acc |= ((uint32_t)buf[pos++]) << acc_bits;
                acc_bits += 8;
            }
            if (acc_bits < 13) break;

            uint16_t t = acc & 0x3FFu;
            acc >>= 10;
            acc_bits -= 10;

            if (t >= 1000u) continue;

            int32_t nz, s;

            nz = (int32_t)(mod10(t) < thresh);
            s  = (int32_t)(acc & 1u) * 2 - 1;
            a[ctr++] = nz * s;
            if (ctr >= len) break;
            acc >>= (nz & 1);
            acc_bits -= (nz & 1);
            t = floor10(t);

            nz = (int32_t)(mod10(t) < thresh);
            s  = (int32_t)(acc & 1u) * 2 - 1;
            a[ctr++] = nz * s;
            if (ctr >= len) break;
            acc >>= (nz & 1);
            acc_bits -= (nz & 1);
            t = floor10(t);

            nz = (int32_t)(t < thresh);
            s  = (int32_t)(acc & 1u) * 2 - 1;
            a[ctr++] = nz * s;
            if (ctr >= len) break;
            acc >>= (nz & 1);
            acc_bits -= (nz & 1);
        }
    }
    else if (P == 0.3125)
    {
        while (ctr < len) {
            while (acc_bits < 15 && pos < buflen) {
                acc |= ((uint32_t)buf[pos++]) << acc_bits;
                acc_bits += 8;
            }
            if (acc_bits < 15) break;

            uint32_t t = acc & 0xFFFu;
            acc >>= 12;
            acc_bits -= 12;

            int32_t nz, s;
            uint32_t d;

            d  = t & 0xFu;
            nz = (int32_t)(d < 10u);
            s  = (int32_t)(acc & 1u) * 2 - 1;
            a[ctr++] = nz * s;
            if (ctr >= len) break;
            acc >>= (nz & 1);
            acc_bits -= (nz & 1);

            d  = (t >> 4) & 0xFu;
            nz = (int32_t)(d < 10u);
            s  = (int32_t)(acc & 1u) * 2 - 1;
            a[ctr++] = nz * s;
            if (ctr >= len) break;
            acc >>= (nz & 1);
            acc_bits -= (nz & 1);

            d  = (t >> 8) & 0xFu;
            nz = (int32_t)(d < 10u);
            s  = (int32_t)(acc & 1u) * 2 - 1;
            a[ctr++] = nz * s;
            if (ctr >= len) break;
            acc >>= (nz & 1);
            acc_bits -= (nz & 1);
        }
    }

    return ctr;
}





static uint64_t approx_exp(const uint64_t x) {
    int64_t result;
    result = -0x0000B6C6340925AELL;
    result = ((smulh48(result, x) + (1LL << 2)) >> 3) + 0x0000B4BD4DF85227LL;
    result = ((smulh48(result, x) + (1LL << 2)) >> 3) - 0x0000887F727491E2LL;
    result = ((smulh48(result, x) + (1LL << 1)) >> 2) + 0x0000AAAA643C7E8DLL;
    result = ((smulh48(result, x) + (1LL << 1)) >> 2) - 0x0000AAAAA98179E6LL;
    result = ((smulh48(result, x) + 1LL) >> 1) + 0x0000FFFFFFFB2E7ALL;
    result = ((smulh48(result, x) + 1LL) >> 1) - 0x0000FFFFFFFFF85FLL;
    result = ((smulh48(result, x))) + 0x0000FFFFFFFFFFFCLL;
    return result;
}

#define CDTLEN 64
static const uint32_t CDT[CDTLEN] = {
 3266,  6520,  9748, 12938, 16079, 19159, 22168, 25096,
27934, 30674, 33309, 35833, 38241, 40531, 42698, 44742,
46663, 48460, 50135, 51690, 53128, 54454, 55670, 56781,
57794, 58712, 59541, 60287, 60956, 61554, 62085, 62556,
62972, 63337, 63657, 63936, 64178, 64388, 64569, 64724,
64857, 64970, 65066, 65148, 65216, 65273, 65321, 65361,
65394, 65422, 65444, 65463, 65478, 65490, 65500, 65508,
65514, 65519, 65523, 65527, 65529, 65531, 65533, 65534
};


static uint64_t sample_gauss16(const uint64_t rand16) {
    unsigned int i;
    uint64_t r = 0;
    for (i = 0; i < CDTLEN; i++) {
        r += (((uint64_t)CDT[i] - rand16) >> 63) & 1;
    }
    return r;
}

#define GAUSS_RAND (72 + 16 + 48)
#define GAUSS_RAND_BYTES ((GAUSS_RAND + 7) / 8)
static int sample_gauss_sigma76(uint64_t *r, fp96_76 *sqr,
                                const uint8_t rand[GAUSS_RAND_BYTES]) {
    const uint64_t rand_gauss16 = rand[0] | (((uint64_t) rand[1]) << 8); 
    const uint64_t rand_rej = rand[2] | (((uint64_t) rand[3]) << 8) | (((uint64_t) rand[4]) << 16) | (((uint64_t) rand[5]) << 24)
     | (((uint64_t) rand[6]) << 32) | (((uint64_t) rand[7]) << 40);
    uint64_t x, exp_in;
    fp96_76 y;

    // sample x
    x = sample_gauss16(rand_gauss16);

    // y := append x to y
    // leave 16 bit for carries
    y.limb48[0] = rand[8] | ((uint64_t)rand[9] << 8) |
                  ((uint64_t)rand[10] << 16) | ((uint64_t)rand[11] << 24) |
                  ((uint64_t)rand[12] << 32) | ((uint64_t)rand[13] << 40);
    y.limb48[1] =
        rand[14] | ((uint64_t)rand[15] << 8) | ((uint64_t)rand[16] << 16) |
        (x << 24);

    // r := round y 
    *r = (y.limb48[0] >> 15) ^ (y.limb48[1] << 33);
    *r += 1; // rounding
    *r >>= 1;

    // sqr := y*y
    fixpoint_square(sqr, &y);

    // sqr[1] = y^2 >> (76+48)                // 34 bit
    // sqr[0] = (y^2 >> 76) & ((1UL<<48)-1)   // 48 bit
    // exp_in := sqr - ((x*x) << 68)
    exp_in = sqr->limb48[1] - ((x*x) << (68 - 48));
    exp_in <<= 20;
    exp_in |= sqr->limb48[0] >> 28;
    exp_in += 1; // rounding
    exp_in >>= 1;

    return ((((int64_t)(rand_rej ^
                        (rand_rej & 1)) // set lowest bit to zero in order to
                                        // use it for rejection if sample==0
              - (int64_t)approx_exp(exp_in)) >>
             63) // reject with prob 1-approx_exp(exp_in)
            & (((*r | -*r) >> 63) | rand_rej)) &
           1; // if the sample is zero, clear the return value with prob 1/2
}

int sample_gauss(uint64_t *r, fp96_76 *sqsum, const uint8_t *buf, const size_t buflen, const size_t len, const int dont_write_last)
{
    const uint8_t *pos = buf;
    fp96_76 sqr;
    size_t bytecnt = buflen, coefcnt = 0, cnt = 0;
    int accepted;
    uint64_t dummy;
    
    while (coefcnt < len) {
        if (bytecnt < GAUSS_RAND_BYTES) {
          renormalize(sqsum);
          return coefcnt;
        }

        if (dont_write_last && coefcnt == len-1)
        {
          accepted = sample_gauss_sigma76(&dummy, &sqr, pos);
        } else {
          accepted = sample_gauss_sigma76(&r[coefcnt], &sqr, pos);
        }
        cnt += 1;
        coefcnt += accepted;
        pos += GAUSS_RAND_BYTES;
        bytecnt -= GAUSS_RAND_BYTES;

        sqsum->limb48[0] += sqr.limb48[0] & -(int64_t)accepted;
        sqsum->limb48[1] += sqr.limb48[1] & -(int64_t)accepted;
    }

    renormalize(sqsum);
    return len;
}

#define POLY_HYPERBALL_BUFLEN (GAUSS_RAND_BYTES * N)
#define POLY_HYPERBALL_NBLOCKS ((POLY_HYPERBALL_BUFLEN + SM3_XOF_BLOCKBYTES - 1) / SM3_XOF_BLOCKBYTES)

/*************************************************
 * Name:        sample_gauss_N
 *
 * Description: Sample len coefficients from discrete Gaussian with
 *              standard deviation sigma=76/2^13
 *              using output of a SHAKE256(seed|nonce).
 *              Also compute the squared norm of the sampled coefficients.
 *
 * Arguments:   - uint64_t *r: pointer to output array of sampled coefficients
 *              - uint8_t *signs: pointer to output array of sign bits
 *              - fp96_76 *sqsum: pointer to output squared norm
 *              - const uint8_t seed[CRHBYTES]: byte array with seed of length CRHBYTES
 *              - const uint16_t nonce: 2-byte nonce
 *              - const size_t len: number of coefficients to be sampled
 **************************************************/
void sample_gauss_N(uint64_t *r, uint8_t *signs, fp96_76 *sqsum,
                    const uint8_t seed[CRHBYTES], const uint16_t nonce,
                    const size_t len) {
    uint8_t buf[POLY_HYPERBALL_NBLOCKS * SM3_XOF_BLOCKBYTES];
    size_t bytecnt, coefcnt, firstflag = 1;
    sm3_xof_state state;
    sm3_xof_stream_init(&state, seed, CRHBYTES, nonce);

    sm3_xof_squeezeblocks(buf, POLY_HYPERBALL_NBLOCKS, &state);
    for (size_t i = 0; i < len / 8; i++) {
        signs[i] = buf[i];
    }
    bytecnt = POLY_HYPERBALL_NBLOCKS * SM3_XOF_BLOCKBYTES - len / 8;
    coefcnt = sample_gauss(r, sqsum, buf + len / 8, bytecnt, len, len%N);
    while (coefcnt < len) {
        size_t off = bytecnt % GAUSS_RAND_BYTES;
        for (size_t i = 0; i < off; i++) {
            buf[i] = buf[bytecnt + len/8*firstflag - off + i];
        }
        sm3_xof_squeezeblocks(buf + off, 1, &state);
        bytecnt = SM3_XOF_BLOCKBYTES + off;

        coefcnt += sample_gauss(r + coefcnt, sqsum, buf, bytecnt, len - coefcnt, len%N);
        firstflag = 0;
    }
}