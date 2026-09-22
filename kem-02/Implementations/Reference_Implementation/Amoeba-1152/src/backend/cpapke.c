#include <stdlib.h>
#include <string.h>

#include "cpapke.h"
#include "hamming.h"
#include "poly.h"
#include "randombytes.h"

#include "auxfunc.h"
#include "drng.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
DRNG_ctx drng_algorithm;

void CPAPKE_Init() {
    uint8_t rnd[64];
    CryptoRandomBytes(rnd, 64);
    init_random_number(&drng_algorithm, rnd, 64);
}

typedef uint8_t MESG[RLWE_ECC_L];
typedef uint8_t PTXT[RLWE_ECC_N];

typedef int16_t POLY[RLWE_K * RLWE_N];
typedef uint8_t SEED[RLWE_SEED_LEN];

typedef struct {
    POLY s;
} CPASK;

typedef struct {
    SEED seed;
    POLY b;
} CPAPK;

typedef struct {
    POLY c1;
    POLY c2;
} CTXT;

static void CBD(POLY f, int32_t eta, const SEED coin, uint8_t label) {
    // state = [coin, label, count]
    uint8_t state[RLWE_SEED_LEN + 2];
    memcpy(state, coin, RLWE_SEED_LEN);
    state[RLWE_SEED_LEN] = label;
    state[RLWE_SEED_LEN + 1] = 0;

    uint8_t arr1[RLWE_K * RLWE_M >> 4];
    uint8_t arr2[RLWE_K * RLWE_M >> 4];
    int8_t F[RLWE_K * RLWE_M >> 1];

    const int32_t bitLen = RLWE_K * RLWE_M >> 1;
    memset(F, 0, bitLen);

    for (int32_t i = 0; i < eta; i++) {
        pseudoXOF(bitLen, state, (RLWE_SEED_LEN + 2) << 3, arr1);
        state[RLWE_SEED_LEN + 1]++;
        pseudoXOF(bitLen, state, (RLWE_SEED_LEN + 2) << 3, arr2);
        state[RLWE_SEED_LEN + 1]++;

        for (int32_t j = 0; j < bitLen; j += 8) {
            int32_t k = j >> 3;
            int8_t r1 = arr1[k], r2 = arr2[k];
            F[j + 0] += (r1 & 0x01) - (r2 & 0x01);
            r1 >>= 1;
            r2 >>= 1;
            F[j + 1] += (r1 & 0x01) - (r2 & 0x01);
            r1 >>= 1;
            r2 >>= 1;
            F[j + 2] += (r1 & 0x01) - (r2 & 0x01);
            r1 >>= 1;
            r2 >>= 1;
            F[j + 3] += (r1 & 0x01) - (r2 & 0x01);
            r1 >>= 1;
            r2 >>= 1;
            F[j + 4] += (r1 & 0x01) - (r2 & 0x01);
            r1 >>= 1;
            r2 >>= 1;
            F[j + 5] += (r1 & 0x01) - (r2 & 0x01);
            r1 >>= 1;
            r2 >>= 1;
            F[j + 6] += (r1 & 0x01) - (r2 & 0x01);
            r1 >>= 1;
            r2 >>= 1;
            F[j + 7] += (r1 & 0x01) - (r2 & 0x01);
        }
    }

    const int32_t n = bitLen / 3;
    for (int32_t i = 0; i < n; i++) {
        int16_t E = F[i + 2 * n];
        f[i] = F[i] - E;
        f[i + n] = F[i + n] + E;
    }
}

static void Parse(POLY a, const SEED seed) {
    uint8_t state[RLWE_SEED_LEN + 1];
    memcpy(state, seed, RLWE_SEED_LEN);
    state[RLWE_SEED_LEN] = 0;

    int32_t i = 0;
    uint16_t x1, x2;
    uint8_t arr[192];  // 3 * 64 Bytes = 1536 bits

    while (i < RLWE_N * RLWE_K) {
        pseudoXOF(1536, state, (RLWE_SEED_LEN + 1) << 3, arr);
        state[RLWE_SEED_LEN]++;

        for (int32_t j = 0; j <= 189; j += 3) {
            if (i >= RLWE_N * RLWE_K)
                break;

            x1 = arr[j] + ((uint16_t)(arr[j + 1] & 0x0f) << 8);
            x2 = (arr[j + 1] >> 4) + ((uint16_t)arr[j + 2] << 8);

            if (x1 < Q) {
                a[i] = x1;
                i++;
            }
            if ((i < RLWE_N * RLWE_K) && (x2 < Q)) {
                a[i] = x2;
                i++;
            }
        }
    }
}

static void int2bin(MESG m, const uint8_t msg[RLWE_MSG_LEN]) {
    int32_t i = 0;
    for (; i < RLWE_ECC_L - 7; i += 8) {
        uint8_t t = msg[i >> 3];
        m[i] = t & 0x01;
        m[i + 1] = (t >> 1) & 0x01;
        m[i + 2] = (t >> 2) & 0x01;
        m[i + 3] = (t >> 3) & 0x01;
        m[i + 4] = (t >> 4) & 0x01;
        m[i + 5] = (t >> 5) & 0x01;
        m[i + 6] = (t >> 6) & 0x01;
        m[i + 7] = (t >> 7) & 0x01;
    }

    if(i != RLWE_ECC_L){
        uint8_t t = msg[RLWE_MSG_LEN - 1];
        for(int32_t j=0;i+j<RLWE_ECC_L;j++){
            m[i+j] = (t >> j) & 0x01;
        }
    }
}

static void bin2int(uint8_t msg[RLWE_MSG_LEN], const MESG m) {
    int32_t i = 0;
    for (; i < RLWE_ECC_L - 7; i += 8) {
        msg[i >> 3] = m[i] + (m[i + 1] << 1) + (m[i + 2] << 2) + (m[i + 3] << 3) + (m[i + 4] << 4) + (m[i + 5] << 5) + (m[i + 6] << 6) + (m[i + 7] << 7);
    }

    if(i != RLWE_ECC_L){
        msg[RLWE_MSG_LEN - 1] = 0;
        for(int j=0;i+j<RLWE_ECC_L;j++){
            msg[RLWE_MSG_LEN - 1] += m[i+j] << j;
        }
    }
}

static void encode_MSB(POLY c, const PTXT m) {
    const int16_t delta = RLWE_Q >> 1;
    for (int32_t i = 0; i < RLWE_ECC_N; i += 4) {
        c[i] = delta & (-m[i]);
        c[i + 1] = delta & (-m[i + 1]);
        c[i + 2] = delta & (-m[i + 2]);
        c[i + 3] = delta & (-m[i + 3]);
    }
}

static void decode_MSB(PTXT m, const POLY c) {
    const int16_t delta = RLWE_Q >> 1;
    for (int32_t i = 0; i < RLWE_ECC_N; i += 4) {
        m[i] = (((c[i] << 1) + delta) / RLWE_Q) & 0x01;
        m[i + 1] = (((c[i + 1] << 1) + delta) / RLWE_Q) & 0x01;
        m[i + 2] = (((c[i + 2] << 1) + delta) / RLWE_Q) & 0x01;
        m[i + 3] = (((c[i + 3] << 1) + delta) / RLWE_Q) & 0x01;
    }
}

static void CPAPKE_KeyGen_raw(CPASK* sk, CPAPK* pk, SEED coin) {
    uint8_t state[RLWE_SEED_LEN + 1];
    memcpy(state, coin, RLWE_SEED_LEN);
    state[RLWE_SEED_LEN] = 0;

    POLY a, b, s, e;
    pseudoXOF(RLWE_SEED_LEN << 3, state, (RLWE_SEED_LEN + 1) << 3, pk->seed);
    state[RLWE_SEED_LEN]++;
    Parse(a, pk->seed);

    SEED seed;
    pseudoXOF(RLWE_SEED_LEN << 3, state, (RLWE_SEED_LEN + 1) << 3, seed);
    state[RLWE_SEED_LEN]++;
    CBD(s, RLWE_ETA1, seed, 0x00);
    CBD(e, RLWE_ETA2, seed, 0x01);
    ForwardNussbamuer(s, s);
    PolyMod(sk->s, s);  // NTT form

    PolyMul(b, a, s);
    InverseNussbamuer(b, b);
    PolyAdd(b, b, e);
    PolyMod(pk->b, b);  // b=as+e

    // Compress PK
}

static void CPAPKE_Enc_raw(CTXT* ct, const CPAPK* pk, const PTXT pt, const SEED coin) {
    // Decompress PK

    POLY a, r, e1, e2;
    Parse(a, pk->seed);

    CBD(r, RLWE_ETA1, coin, 0x02);
    CBD(e1, RLWE_ETA2, coin, 0x04);
    CBD(e2, RLWE_ETA2, coin, 0x08);

    POLY b, u, v;
    ForwardNussbamuer(r, r);
    ForwardNussbamuer(b, pk->b);
    PolyMul(u, r, a);
    PolyMul(v, r, b);
    InverseNussbamuer(u, u);
    InverseNussbamuer(v, v);
    PolyAdd(u, u, e1);  // u=ra+e1
    PolyAdd(v, v, e2);  // v=rb+e2

    POLY c;
    encode_MSB(c, pt);
    PolyAdd(v, v, c);

    PolyMod(ct->c1, u);
    PolyMod(ct->c2, v);

    // Compress CT
}

static void CPAPKE_Dec_raw(PTXT pt, const CPASK* sk, const CTXT* ct) {
    // Decompress CT

    POLY c1;
    ForwardNussbamuer(c1, ct->c1);
    PolyMul(c1, c1, sk->s);
    InverseNussbamuer(c1, c1);

    PolySub(c1, ct->c2, c1);
    PolyMod(c1, c1);
    decode_MSB(pt, c1);
}

static void poly_pack(const int16_t* poly, size_t poly_len, int d, uint8_t* stream) {
    memset(stream, 0, ((poly_len * d) + 7) / 8);

    size_t bit_pos = 0;  // 全局比特偏移
    const uint16_t mask = (1U << d) - 1;

    for (size_t i = 0; i < poly_len; i++) {
        uint16_t val = poly[i] & mask;
        int remain_bits = d;

        while (remain_bits > 0) {
            size_t byte_idx = bit_pos >> 3;
            int bit_in_byte = bit_pos & 0x07;
            int write_bits = (remain_bits < (8 - bit_in_byte)) ? remain_bits : (8 - bit_in_byte);

            uint8_t shift_val = (val >> (remain_bits - write_bits)) & ((1U << write_bits) - 1);
            stream[byte_idx] |= (shift_val << (8 - bit_in_byte - write_bits));

            bit_pos += write_bits;
            remain_bits -= write_bits;
        }
    }
}

static void poly_unpack(int16_t* poly, size_t poly_len, int d, const uint8_t* stream) {
    size_t bit_pos = 0;
    const uint16_t mask = (1U << d) - 1;

    for (size_t i = 0; i < poly_len; i++) {
        uint16_t val = 0;
        int read_bits = d;

        while (read_bits > 0) {
            size_t byte_idx = bit_pos >> 3;
            int bit_in_byte = bit_pos & 0x07;
            int avail_bits = 8 - bit_in_byte;
            int take_bits = (read_bits < avail_bits) ? read_bits : avail_bits;

            uint8_t byte_data = stream[byte_idx];
            uint8_t slice = (byte_data >> (8 - bit_in_byte - take_bits)) & ((1U << take_bits) - 1);

            val = (val << take_bits) | slice;
            bit_pos += take_bits;
            read_bits -= take_bits;
        }
        poly[i] = (int16_t)(val & mask);
    }
}

static void mod_down(int16_t* poly_out, const int16_t* poly_in, size_t poly_len, int16_t q, int32_t d) {
    if ((1 << d) >= q) {
        memcpy(poly_out, poly_in, poly_len*sizeof(int16_t));
        return;
    }

    int16_t half_q = q / 2;
    int32_t i = 0;
    for (; i < poly_len - 7; i += 4) {
        poly_out[i] = (((int32_t)poly_in[i] << d) + half_q) / q;
        poly_out[i + 1] = (((int32_t)poly_in[i + 1] << d) + half_q) / q;
        poly_out[i + 2] = (((int32_t)poly_in[i + 2] << d) + half_q) / q;
        poly_out[i + 3] = (((int32_t)poly_in[i + 3] << d) + half_q) / q;
    }
    for (; i < poly_len; i++) {
        poly_out[i] = (((int32_t)poly_in[i] << d) + half_q) / q;
    }
}

static void mod_up(int16_t* poly_out, const int16_t* poly_in, size_t poly_len, int32_t q, int32_t d) {
    if ((1 << d) >= q) {
        memcpy(poly_out, poly_in, poly_len*sizeof(int16_t));
        return;
    }

    int16_t half_p = 1 << (d - 1);
    int32_t i = 0;
    for (; i < poly_len - 7; i += 4) {
        poly_out[i] = (poly_in[i] * q + half_p) >> d;
        poly_out[i + 1] = (poly_in[i + 1] * q + half_p) >> d;
        poly_out[i + 2] = (poly_in[i + 2] * q + half_p) >> d;
        poly_out[i + 3] = (poly_in[i + 3] * q + half_p) >> d;
    }
    for (; i < poly_len; i++) {
        poly_out[i] = (poly_in[i] * q + half_p) >> d;
    }
}

static int Compress_poly(uint8_t* stm, int32_t stm_len, const int16_t* ploy, int32_t ploy_len, int32_t q, int32_t d) {
    if (stm_len < (ploy_len * d + 7) / 8) {
        return -1;
    }

    int16_t* tmp = (int16_t*)malloc(ploy_len * sizeof(int16_t));
    memset(tmp, 0, ploy_len * sizeof(int16_t));
    mod_down(tmp, ploy, ploy_len, q, d);
    poly_pack(tmp, ploy_len, d, stm);
    free(tmp);
    return 0;
}

static int Decompress_poly(const uint8_t* stm, int32_t stm_len, int16_t* ploy, int32_t ploy_len, int32_t q, int32_t d) {
    if (stm_len < (ploy_len * d + 7) / 8) {
        return -1;
    }

    int16_t* tmp = (int16_t*)malloc(ploy_len * sizeof(int16_t));
    memset(tmp, 0, ploy_len * sizeof(int16_t));
    poly_unpack(tmp, ploy_len, d, stm);
    mod_up(ploy, tmp, ploy_len, q, d);
    free(tmp);
    return 0;
}

static void Compress_SK(uint8_t dst[RLWE_CPA_SK_LEN], CPASK* sk) {
    Compress_poly(dst, RLWE_CPA_SK_LEN, sk->s, RLWE_K * RLWE_N, RLWE_Q, RLWE_LogQ);
}

static void Decompress_SK(CPASK* sk, uint8_t src[RLWE_CPA_SK_LEN]) {
    Decompress_poly(src, RLWE_CPA_SK_LEN, sk->s, RLWE_K * RLWE_N, RLWE_Q, RLWE_LogQ);
}

static void Compress_PK(uint8_t dst[RLWE_CPA_PK_LEN], CPAPK* pk) {
    memcpy(dst, pk->seed, RLWE_SEED_LEN);
    dst += RLWE_SEED_LEN;
    Compress_poly(dst, BIT_TO_BYTE(RLWE_D0 * RLWE_K * RLWE_N), pk->b, RLWE_K * RLWE_N, RLWE_Q, RLWE_D0);
}

static void Decompress_PK(CPAPK* pk, uint8_t src[RLWE_CPA_PK_LEN]) {
    memcpy(pk->seed, src, RLWE_SEED_LEN);
    src += RLWE_SEED_LEN;
    Decompress_poly(src, BIT_TO_BYTE(RLWE_D0 * RLWE_K * RLWE_N), pk->b, RLWE_K * RLWE_N, RLWE_Q, RLWE_D0);
}

static void Compress_CT(uint8_t dst[RLWE_CPA_CT_LEN], CTXT* ct) {
    Compress_poly(dst, BIT_TO_BYTE(RLWE_D1 * RLWE_K * RLWE_N), ct->c1, RLWE_K * RLWE_N, RLWE_Q, RLWE_D1);
    dst += BIT_TO_BYTE(RLWE_D1 * RLWE_K * RLWE_N);
    Compress_poly(dst, BIT_TO_BYTE(RLWE_D2 * RLWE_ECC_N), ct->c2, RLWE_ECC_N, RLWE_Q, RLWE_D2);
}

static void Decompress_CT(CTXT* ct, uint8_t src[RLWE_CPA_CT_LEN]) {
    Decompress_poly(src, BIT_TO_BYTE(RLWE_D1 * RLWE_K * RLWE_N), ct->c1, RLWE_K * RLWE_N, RLWE_Q, RLWE_D1);
    src += BIT_TO_BYTE(RLWE_D1 * RLWE_K * RLWE_N);
    Decompress_poly(src, BIT_TO_BYTE(RLWE_D2 * RLWE_ECC_N), ct->c2, RLWE_ECC_N, RLWE_Q, RLWE_D2);
}

void CPAPKE_KeyGen(uint8_t sk[RLWE_CPA_SK_LEN], uint8_t pk[RLWE_CPA_PK_LEN]) {
    memset(sk, 0, RLWE_CPA_SK_LEN);
    memset(pk, 0, RLWE_CPA_PK_LEN);

    CPASK s;
    CPAPK p;
    SEED seed;

    get_random_number(&drng_algorithm, seed, RLWE_SEED_LEN * 8);
    CPAPKE_KeyGen_raw(&s, &p, seed);

    Compress_SK(sk, &s);
    Compress_PK(pk, &p);
}

void CPAPKE_Encrypt(uint8_t ct[RLWE_CPA_CT_LEN], const uint8_t pk[RLWE_CPA_PK_LEN], const uint8_t msg[RLWE_MSG_LEN], const uint8_t coin[RLWE_SEED_LEN]) {
    memset(ct, 0, RLWE_CPA_CT_LEN);

    CPAPK p;
    CTXT c;
    PTXT pt;
    MESG m;

    int2bin(m, msg);
    encode_ECC(m, pt);

    Decompress_PK(&p, pk);
    CPAPKE_Enc_raw(&c, &p, pt, coin);
    Compress_CT(ct, &c);
}

int CPAPKE_Decrypt(uint8_t msg[RLWE_MSG_LEN],
                   const uint8_t sk[RLWE_CPA_SK_LEN],
                   const uint8_t ct[RLWE_CPA_CT_LEN]) {
    memset(msg, 0, RLWE_MSG_LEN);

    CPASK s;
    CTXT c;
    PTXT pt;
    MESG m;

    Decompress_SK(&s, sk);
    Decompress_CT(&c, ct);
    CPAPKE_Dec_raw(pt, &s, &c);

    if (decode_ECC(pt, m) > 1) {
        return -1;
    }
    bin2int(msg, m);
    return 0;
}
