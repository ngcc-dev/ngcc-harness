#include "laurus_1024.h"

#define NROUNDS 16
#define ROL(a, offset) ((a << offset) ^ (a >> (64 - offset)))
#define ROR(a, offset) ((a >> offset) ^ (a << (64 - offset)))

#define rp 512
#define block_hout_bit 1088

 /*************************************************
 * Description: Load 8 bytes into 64-bit integer in big-endian order
 **************************************************/
#define load64(y)	((uint64_t)((y)[0] & 0xFF) << 56) | ((uint64_t)((y)[1] & 0xFF) << 48) | \
					((uint64_t)((y)[2] & 0xFF) << 40) | ((uint64_t)((y)[3] & 0xFF) << 32) | \
					((uint64_t)((y)[4] & 0xFF) << 24) | ((uint64_t)((y)[5] & 0xFF) << 16) | \
					((uint64_t)((y)[6] & 0xFF) << 8) |  ((uint64_t)((y)[7] & 0xFF))


/*************************************************
* Description: Store a 64-bit integer to array of 8 bytes in big-endian order
**************************************************/
#define store64(y, x) { (y)[0] = ((x) >> 56) & 0xFF; (y)[1] = ((x) >> 48) & 0xFF;\
					    (y)[2] = ((x) >> 40) & 0xFF; (y)[3] = ((x) >> 32) & 0xFF;\
                        (y)[4] = ((x) >> 24) & 0xFF; (y)[5] = ((x) >> 16) & 0xFF;\
					    (y)[6] = ((x) >> 8) & 0xFF; (y)[7] = (x) & 0xFF; }


/* round constants */
static const uint64_t RC[NROUNDS] = 
{
    0xb7e151628aed2a6a,
    0xaf82a3cd15dad4d7,
    0x9f4546922bb529ad,
    0xfeca8c2c576ad359,
    0x3dd51950aed526b1,
    0x7baa32a15daa4d62,
    0xf7546542bb549ac4,
    0x2ee8cb8d76a9b58b,
    0x5dd1971aed536b16,
    0xbba32e35daa6d62c,
    0xb7065d63b54d2c5b,
    0xae4cbbcf6a9ad8b5,
    0x9cd97696d5353169,
    0xf9f2ec25aa6ae2d1,
    0x33a5d94354d545a1,
    0x674bb286a9aa8b42
};

/*************************************************
* Name:        F1600_StatePermute
*
* Description: The F1600 Permutation
*
* Arguments:   - uint64_t *state: pointer to input/output state
**************************************************/
static void F1600_StatePermute(uint64_t state[25])
{
    int round;

    uint64_t P0, P1, P2, P3, P4;
    uint64_t Q0, Q1, Q2, Q3, Q4;
    uint64_t A00, A01, A02, A03, A04;
    uint64_t A10, A11, A12, A13, A14;
    uint64_t A20, A21, A22, A23, A24;
    uint64_t A30, A31, A32, A33, A34;
    uint64_t A40, A41, A42, A43, A44;

    for (round = 0; round < NROUNDS; round++)
    {
        // add RoundConstant --> iota
        state[0] ^= RC[round];

        // non-linear transform --> chi
        P4 = state[20] & state[15];
        P3 = state[15] & state[10];
        P2 = state[10] & state[ 5];
        A40 = P4 ^ state[ 0];
        A30 = P3 ^ state[20];
        A20 = P2 ^ state[15];
        P1 = A40 & state[ 5];
        P0 = A40 & A30;
        A10 = P1 ^ state[10];
        A00 = P0 ^ state[ 5];

        P4 = state[21] & state[16];
        P3 = state[16] & state[11];
        P2 = state[11] & state[ 6];
        A41 = P4 ^ state[ 1];
        A31 = P3 ^ state[21];
        A21 = P2 ^ state[16];
        P1 = A41 & state[ 6];
        P0 = A41 & A31;
        A11 = P1 ^ state[11];
        A01 = P0 ^ state[ 6];

        P4 = state[22] & state[17];
        P3 = state[17] & state[12];
        P2 = state[12] & state[ 7];
        A42 = P4 ^ state[ 2];
        A32 = P3 ^ state[22];
        A22 = P2 ^ state[17];
        P1 = A42 & state[ 7];
        P0 = A42 & A32;
        A12 = P1 ^ state[12];
        A02 = P0 ^ state[ 7];

        P4 = state[23] & state[18];
        P3 = state[18] & state[13];
        P2 = state[13] & state[ 8];
        A43 = P4 ^ state[ 3];
        A33 = P3 ^ state[23];
        A23 = P2 ^ state[18];
        P1 = A43 & state[ 8];
        P0 = A43 & A33;
        A13 = P1 ^ state[13];
        A03 = P0 ^ state[ 8];

        P4 = state[24] & state[19];
        P3 = state[19] & state[14];
        P2 = state[14] & state[ 9];
        A44 = P4 ^ state[ 4];
        A34 = P3 ^ state[24];
        A24 = P2 ^ state[19];
        P1 = A44 & state[ 9];
        P0 = A44 & A34;
        A14 = P1 ^ state[14];
        A04 = P0 ^ state[ 9];

        // linear transform --> theta
        P0 = A00 ^ A01 ^ A02 ^ A03 ^ A04;
        P1 = A10 ^ A11 ^ A12 ^ A13 ^ A14;
        P2 = A20 ^ A21 ^ A22 ^ A23 ^ A24;
        P3 = A30 ^ A31 ^ A32 ^ A33 ^ A34;
        P4 = A40 ^ A41 ^ A42 ^ A43 ^ A44;
        Q0 = ROL(A00, 31) ^ ROL(A01,  1) ^ ROL(A02,  4) ^ ROL(A03, 21) ^ ROL(A04, 22);
        Q1 = ROL(A10, 19) ^ ROL(A11, 12) ^ ROL(A12, 15) ^ ROL(A13, 34) ^ ROL(A14, 13);
        Q2 = ROL(A20, 28) ^ ROL(A21, 35) ^ ROL(A22, 58) ^ ROL(A23,  9) ^ ROL(A24, 23);
        Q3 = ROL(A30, 55) ^ ROL(A31, 18) ^ ROL(A32,  5) ^ ROL(A33, 27) ^ ROL(A34, 44);
        Q4 = ROL(A40, 14) ^ ROL(A41, 43) ^ ROL(A42, 53) ^ ROL(A43, 20) ^ ROL(A44, 25);

        // with shifting transform --> pi
        state[ 0] = A00 ^ ROR(P0, 31) ^ Q0;
        state[23] = A01 ^ ROR(P0,  1) ^ Q0;
        state[16] = A02 ^ ROR(P0,  4) ^ Q0;
        state[14] = A03 ^ ROR(P0, 21) ^ Q0;
        state[ 7] = A04 ^ ROR(P0, 22) ^ Q0;

        state[21] = A10 ^ ROR(P1, 19) ^ Q1;
        state[19] = A11 ^ ROR(P1, 12) ^ Q1;
        state[12] = A12 ^ ROR(P1, 15) ^ Q1;
        state[ 5] = A13 ^ ROR(P1, 34) ^ Q1;
        state[ 3] = A14 ^ ROR(P1, 13) ^ Q1;

        state[17] = A20 ^ ROR(P2, 28) ^ Q2;
        state[10] = A21 ^ ROR(P2, 35) ^ Q2;
        state[ 8] = A22 ^ ROR(P2, 58) ^ Q2;
        state[ 1] = A23 ^ ROR(P2,  9) ^ Q2;
        state[24] = A24 ^ ROR(P2, 23) ^ Q2;

        state[13] = A30 ^ ROR(P3, 55) ^ Q3;
        state[ 6] = A31 ^ ROR(P3, 18) ^ Q3;
        state[ 4] = A32 ^ ROR(P3,  5) ^ Q3;
        state[22] = A33 ^ ROR(P3, 27) ^ Q3;
        state[15] = A34 ^ ROR(P3, 44) ^ Q3;

        state[ 9] = A40 ^ ROR(P4, 14) ^ Q4;
        state[ 2] = A41 ^ ROR(P4, 43) ^ Q4;
        state[20] = A42 ^ ROR(P4, 53) ^ Q4;
        state[18] = A43 ^ ROR(P4, 20) ^ Q4;
        state[11] = A44 ^ ROR(P4, 25) ^ Q4;
    }
}

/*************************************************
* Name:        laurus_init
*
* Arguments:   - laurus_state *state: pointer to  state
*              - uint64_t c:          security parameter in bits (e.g., 512 for laurus_512)
*              - uint64_t fid:        id of hash function, input 1 if xof, otherwise 0
*
* Arguments:   - laurus_state *state: pointer to  state
**************************************************/
static void laurus_init(laurus_state* state, uint64_t c, uint64_t fid)
{
    int i;
    for (i = 0; i < 24; i++)
    {
        state->A[i] = 0;
        state->S[i] = 0;
    }
    state->A[24] = 0;
    state->S[23] = (fid << 63) + c;
    state->sec = c;
    state->pos = 0;
    state->counter = 0;
}

/*************************************************
* Name:        laurus_absorb
*
* Description: Absorb step of Laurus.
*
* Arguments:   - laurus_state* state:   pointer to  state
*              - const uint8_t *m:      pointer to input to be absorbed into s
*              - uint64_t mlen:         length of input in bytes
*
**************************************************/
static void laurus_absorb(laurus_state* state,
                          const uint8_t* m,
                          uint64_t mlen)
{
    uint8_t t[8] = { 0 };
    uint64_t i, r = (1536 - state->sec) / 8, rb = r / 8, cb = 24 - rb;

    if (state->pos & 7) {
        i = state->pos & 7;
        while (i < 8 && mlen > 0) {
            t[i++] = *m++;
            mlen--;
            state->pos++;
        }
        state->A[(state->pos - i) / 8 + 1] ^= load64(t);
    }

    if (state->pos && mlen > r - state->pos) {
        for (i = 1; i <= (r - state->pos) / 8; i++)
            state->A[state->pos / 8 + i] = load64(m + 8 * (i - 1));
        m += r - state->pos;
        mlen -= r - state->pos;
        state->pos = 0;
        state->A[0] = state->counter++;
        for (i = rb + 1; i < 25; i++)
            state->A[i] = state->S[i - rb - 1];
        for (i = 0; i < rb; i++)
            state->S[i] = state->S[i + cb];
        for (; i < 24; i++)
            state->S[i] = state->A[i - rb + 1];
        F1600_StatePermute(state->A);
        for (i = 0; i < 24; i++)
            state->S[i] ^= state->A[i + 1];
    }

    while (mlen > r) {
        for (i = 0; i < rb; i++)
            state->A[i + 1] = load64(m + 8 * i);
        m += r;
        mlen -= r;
        state->A[0] = state->counter++;
        for (i = rb + 1; i < 25; i++)
            state->A[i] = state->S[i - rb - 1];
        for (i = 0; i < rb; i++)
            state->S[i] = state->S[i + cb]; // <<<
        for (; i < 24; i++)
            state->S[i] = state->A[i - rb + 1];
        F1600_StatePermute(state->A);
        for (i = 0; i < 24; i++)
            state->S[i] ^= state->A[i + 1];
    }

    for (i = 0; i < mlen / 8; i++)
        state->A[state->pos / 8 + i + 1] = load64(m + 8 * i);
    m += 8 * i;
    mlen -= 8 * i;
    state->pos += 8 * i;

    if (mlen) {
        for (i = 0; i < 8; i++)
            t[i] = 0;
        for (i = 0; i < mlen; i++)
            t[i] = m[i];
        state->A[state->pos / 8 + 1] = load64(t);
        state->pos += mlen;
    }
}

/*************************************************
* Name:        laurus_finalize
*
* Description: Finalize absorb step.
*
* Arguments:   - laurus_state* state:   pointer to  state
* 
**************************************************/
static void laurus_finalize(laurus_state* state)
{
    uint64_t i, rb = (1536 - state->sec) / 64, cb = 24 - rb;

    if (state->pos > rp / 8)
    {
        if (state->pos & 7)
            state->A[(state->pos >> 3) + 1] ^= 0x80ULL << 8 * (7 - (state->pos & 7));
        else
            state->A[(state->pos >> 3) + 1] = 0x8000000000000000ULL;
        for (i = (state->pos >> 3) + 2; i <= rb; i++) { state->A[i] = 0; }

        state->A[0] = state->counter++;
        for (i = rb + 1; i < 25; i++)
            state->A[i] = state->S[i - rb - 1];
        for (i = 0; i < rb; i++)
            state->S[i] = state->S[i + cb]; // <<<
        for (; i < 24; i++)
            state->S[i] = state->A[i - rb + 1];
        F1600_StatePermute(state->A);
        for (i = 0; i < 16; i++)
            state->S[i] = state->S[i + 8] ^ state->A[i + 9];
        for (; i < 24; i++)
            state->S[i] = 0;
        if (state->pos == rb * 8)
            state->S[16] = 0x8000000000000000ULL;

        state->counter = 1ULL << 63;
    }
    else if (state->pos == rp / 8)
    {
        for (i = 0; i < 16; i++)
            state->S[i] = state->S[i + 8];
        for (; i < 24; i++)
            state->S[i] = state->A[i - 15];

        state->counter = 3ULL << 62;
    }
    else
    {
        for (i = 0; i < 16; i++)
            state->S[i] = state->S[i + 8];
        for (i = 0; i <= state->pos / 8; i++)
            state->S[i + 16] = state->A[i + 1];
        if (state->pos & 7)
            state->S[state->pos / 8 + 16] ^= 0x80ULL << 8 * (7 - (state->pos & 7));
        else
            state->S[state->pos / 8 + 16] = 0x8000000000000000ULL;
        for (i = state->pos / 8 + 17; i < 24; i++) { state->S[i] = 0; }

        state->counter = 1ULL << 63;
    }

    state->pos = 0;
}

/*************************************************
* Name:        laurus_squeezeblocks
*
* Description: Squeeze step of . Squeezes full blocks of r bytes each.
*              Modifies the state. Can be called multiple times to keep
*              squeezing, i.e., is incremental. Assumes zero bytes of current
*              block have already been squeezed.
*
* Arguments:   - uint8_t *out:        pointer to output blocks
*              - int nblocks:         number of blocks to be squeezed (written to out)
*              - laurus_state* state: pointer to input/output  state
* 
**************************************************/
static void laurus_squeezeblocks(uint8_t* out,
                                 int nblocks,
                                 laurus_state* state)
{
    int i;

    while (nblocks > 0) {
        state->A[0] = state->counter++;
        for (i = 1; i < 25; i++)
            state->A[i] = state->S[i - 1];
        F1600_StatePermute(state->A);
        for (i = 0; i < block_hout_bit / 64; i++) {
            state->A[i + 8] ^= state->S[i + 7];
            store64(out + 8 * i, state->A[i + 8]);
        }
        out += block_hout_bit / 8;
        nblocks--;
    }
}

/*************************************************
* Name:        laurus_squeeze
*
* Description: Squeeze step of . Squeezes arbitratrily many bytes.
*              Modifies the state. Can be called multiple times to keep
*              squeezing, i.e., is incremental.
*
* Arguments:   - uint8_t *out:        pointer to output
*              - uint64_t outlen:     number of bytes to be squeezed (written to out)
*              - uint64_t r:          rate in bytes (e.g., 128 for laurus_512)
*              - uint64_t h:          block output bytes (e.g., 64 for laurus_512)
*              - laurus_state* state: pointer to input/output  state
*
**************************************************/
static void laurus_squeeze(uint8_t* out,
                           uint64_t outlen,
                           laurus_state* state)
{
    uint8_t t[8];
    int i, h = block_hout_bit / 8;

    if (state->pos & 7) {
        store64(t, state->A[state->pos / 8]);
        i = state->pos & 7;
        while (i < 8 && outlen > 0) {
            *out++ = t[i++];
            outlen--;
            state->pos++;
        }
    }

    if (state->pos && outlen >= h - state->pos) {
        for (i = 0; i < (h - state->pos) / 8; i++)
            store64(out + 8 * i, state->A[state->pos / 8 + i]);
        out += h - state->pos;
        outlen -= h - state->pos;
        state->pos = 0;
    }

    while (outlen >= h) {
        state->A[0] = state->counter++;
        for (i = 1; i < 25; i++)
            state->A[i] = state->S[i - 1];
        F1600_StatePermute(state->A);
        for (i = 0; i < h / 8; i++) {
            state->A[i] = state->A[i + 8] ^ state->S[i + 7];
            store64(out + 8 * i, state->A[i]);
        }
        out += h;
        outlen -= h;
    }

    if (!outlen)
        return;
    else if (!state->pos)
    {
        state->A[0] = state->counter++;
        for (i = 1; i < 25; i++)
            state->A[i] = state->S[i - 1];
        F1600_StatePermute(state->A);
        for (i = 0; i < h / 8; i++)
            state->A[i] = state->A[i + 8] ^ state->S[i + 7];
    }

    for (i = 0; i < outlen / 8; i++)
        store64(out + 8 * i, state->A[state->pos / 8 + i]);
    out += 8 * i;
    outlen -= 8 * i;
    state->pos += 8 * i;

    store64(t, state->A[state->pos / 8]);
    for (i = 0; i < outlen; i++)
        out[i] = t[i];
    state->pos += outlen;
}

/*************************************************
* Name:        laurus_1024_init
*
* Description: Laurus-1024 Initilizes
*
* Arguments:   - laurus_state *state: pointer to (uninitialized) state
**************************************************/
void laurus_1024_init(laurus_state* state)
{
    laurus_init(state, 1024, 0);
}

/*************************************************
* Name:        laurus_1024_absorb
*
* Description: Absorb step of the laurus_1024.
*
* Arguments:   - laurus_state *state: pointer to (initialized) output state
*              - const uint8_t *in:   pointer to input to be absorbed into s
*              - uint64_t inlen:      length of input in bytes
**************************************************/
void laurus_1024_absorb(laurus_state* state, const uint8_t* in, uint64_t inlen)
{
    laurus_absorb(state, in, inlen);
}

/*************************************************
* Name:        laurus_1024_finalize
*
* Description: Finalize absorb step of the laurus_1024.
*
* Arguments:   - laurus_state *state: pointer to state
**************************************************/
void laurus_1024_finalize(laurus_state* state)
{
    laurus_finalize(state);
}

/*************************************************
* Name:        laurus_1024_squeeze
*
* Description: Squeeze step of laurus_1024. Squeezes 128
*              bytes. Can be called multiple times to keep squeezing.
*
* Arguments:   - uint8_t *out:        pointer to output blocks
*              - uint64_t outlen :    number of bytes to be squeezed
*                                     (written to output)
*              - laurus_state *state: pointer to input/output state
**************************************************/
void laurus_1024_squeeze(uint8_t* out, uint64_t outlen, laurus_state* state)
{
    laurus_squeeze(out, outlen, state);
}

/*************************************************
* Name:        laurus_1024
*
* Description: laurus-1024 with non-incremental API
*
* Arguments:   - uint8_t *h:        pointer to output (128 bytes)
*              - const uint8_t *in: pointer to input
*              - uint64_t inlen:    length of input in bytes
**************************************************/
void laurus_1024(uint8_t h[128], const uint8_t* in, uint64_t inlen)
{
    laurus_state state;

    laurus_1024_init(&state);
    laurus_1024_absorb(&state, in, inlen);
    laurus_1024_finalize(&state);
    laurus_1024_squeeze(h, 128, &state);
}