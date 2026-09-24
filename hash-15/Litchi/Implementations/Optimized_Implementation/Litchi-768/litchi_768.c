#include "litchi_768.h"

#define NROUNDS 16
#define ROL(a, offset) ((a << offset) ^ (a >> (64 - offset)))
#define ROR(a, offset) ((a >> offset) ^ (a << (64 - offset)))


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

    uint64_t P, Q;
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
        A40 = state[20] & state[15] ^ state[ 0];
        A30 = state[15] & state[10] ^ state[20];
        A20 = state[10] & state[ 5] ^ state[15];
        A10 = A40 & state[ 5] ^ state[10];
        A00 = A40 & A30 ^ state[ 5];

        A41 = state[21] & state[16] ^ state[ 1];
        A31 = state[16] & state[11] ^ state[21];
        A21 = state[11] & state[ 6] ^ state[16];
        A11 = A41 & state[ 6] ^ state[11];
        A01 = A41 & A31 ^ state[ 6];

        A42 = state[22] & state[17] ^ state[ 2];
        A32 = state[17] & state[12] ^ state[22];
        A22 = state[12] & state[ 7] ^ state[17];
        A12 = A42 & state[ 7] ^ state[12];
        A02 = A42 & A32 ^ state[ 7];

        A43 = state[23] & state[18] ^ state[ 3];
        A33 = state[18] & state[13] ^ state[23];
        A23 = state[13] & state[ 8] ^ state[18];
        A13 = A43 & state[ 8] ^ state[13];
        A03 = A43 & A33 ^ state[ 8];

        A44 = state[24] & state[19] ^ state[ 4];
        A34 = state[19] & state[14] ^ state[24];
        A24 = state[14] & state[ 9] ^ state[19];
        A14 = A44 & state[ 9] ^ state[14];
        A04 = A44 & A34 ^ state[ 9];

        // linear transform with shifting transform --> theta & pi
        P = A00 ^ A01 ^ A02 ^ A03 ^ A04;
        Q = ROL(A00, 31) ^ ROL(A01,  1) ^ ROL(A02,  4) ^ ROL(A03, 21) ^ ROL(A04, 22);
        state[ 0] = A00 ^ ROR(P, 31) ^ Q;
        state[23] = A01 ^ ROR(P,  1) ^ Q;
        state[16] = A02 ^ ROR(P,  4) ^ Q;
        state[14] = A03 ^ ROR(P, 21) ^ Q;
        state[ 7] = A04 ^ ROR(P, 22) ^ Q;

        P = A10 ^ A11 ^ A12 ^ A13 ^ A14;
        Q = ROL(A10, 19) ^ ROL(A11, 12) ^ ROL(A12, 15) ^ ROL(A13, 34) ^ ROL(A14, 13);
        state[21] = A10 ^ ROR(P, 19) ^ Q;
        state[19] = A11 ^ ROR(P, 12) ^ Q;
        state[12] = A12 ^ ROR(P, 15) ^ Q;
        state[ 5] = A13 ^ ROR(P, 34) ^ Q;
        state[ 3] = A14 ^ ROR(P, 13) ^ Q;

        P = A20 ^ A21 ^ A22 ^ A23 ^ A24;
        Q = ROL(A20, 28) ^ ROL(A21, 35) ^ ROL(A22, 58) ^ ROL(A23,  9) ^ ROL(A24, 23);
        state[17] = A20 ^ ROR(P, 28) ^ Q;
        state[10] = A21 ^ ROR(P, 35) ^ Q;
        state[ 8] = A22 ^ ROR(P, 58) ^ Q;
        state[ 1] = A23 ^ ROR(P,  9) ^ Q;
        state[24] = A24 ^ ROR(P, 23) ^ Q;

        P = A30 ^ A31 ^ A32 ^ A33 ^ A34;
        Q = ROL(A30, 55) ^ ROL(A31, 18) ^ ROL(A32,  5) ^ ROL(A33, 27) ^ ROL(A34, 44);
        state[13] = A30 ^ ROR(P, 55) ^ Q;
        state[ 6] = A31 ^ ROR(P, 18) ^ Q;
        state[ 4] = A32 ^ ROR(P,  5) ^ Q;
        state[22] = A33 ^ ROR(P, 27) ^ Q;
        state[15] = A34 ^ ROR(P, 44) ^ Q;

        P = A40 ^ A41 ^ A42 ^ A43 ^ A44;
        Q = ROL(A40, 14) ^ ROL(A41, 43) ^ ROL(A42, 53) ^ ROL(A43, 20) ^ ROL(A44, 25);
        state[ 9] = A40 ^ ROR(P, 14) ^ Q;
        state[ 2] = A41 ^ ROR(P, 43) ^ Q;
        state[20] = A42 ^ ROR(P, 53) ^ Q;
        state[18] = A43 ^ ROR(P, 20) ^ Q;
        state[11] = A44 ^ ROR(P, 25) ^ Q;
    }
}

/*************************************************
* Name:        litchi_init
*
* Description: Initializes the litchi state.
*
* Arguments:   - litchi_state *state: pointer to  state
*              - uint64_t c:          security parameter in bits (e.g., 576 for litchi_512)
*              - uint64_t fid:        control parameter in bits
* 
**************************************************/
static void litchi_init(litchi_state* state, uint64_t c, uint64_t fid)
{
    int i;

    for (i = 0; i < 25; i++) {
        state->A[i] = 0;
        state->S[i] = 0;
    }
    state->A[24] = (fid << 63) + c;
    state->sec = c;
    state->pos = 0;
    state->counter = 1;
}

/*************************************************
* Name:        litchi_absorb
*
* Description: Absorb step of Litchi.
*
* Arguments:   - litchi_state* state:   pointer to  state
*              - const uint8_t *m:      pointer to input to be absorbed into s
*              - uint64_t mlen:         length of input in bytes
*
**************************************************/
static void litchi_absorb(litchi_state* state,
                          const uint8_t* m,
                          uint64_t mlen)
{
    uint8_t t[8] = { 0 };
    uint64_t i, r = 200 - (state->sec / 8), rb = r / 8;

    if (state->pos & 7) {
        i = state->pos & 7;
        while (i < 8 && mlen > 0) {
            t[i++] = *m++;
            mlen--;
            state->pos++;
        }
        state->A[(state->pos - i) / 8] ^= load64(t);
    }

    if (state->pos && mlen >= r - state->pos) {
        for (i = 0; i < (r - state->pos) / 8; i++)
            state->A[state->pos / 8 + i] ^= load64(m + 8 * i);
        m += r - state->pos;
        mlen -= r - state->pos;
        state->pos = 0;
        for (i = 0; i < state->sec / 64; i++)
            state->S[i] = state->A[i + rb];
        F1600_StatePermute(state->A);
        for (i = 0; i < state->sec / 64; i++)
            state->A[i + rb] ^= state->S[i];
    }

    while (mlen >= r) {
        for (i = 0; i < rb; i++)
            state->A[i] ^= load64(m + 8 * i);
        m += r;
        mlen -= r;
        for (i = 0; i < state->sec / 64; i++)
            state->S[i] = state->A[i + rb];
        F1600_StatePermute(state->A);
        for (i = 0; i < state->sec / 64; i++)
            state->A[i + rb] ^= state->S[i];
    }

    for (i = 0; i < mlen / 8; i++)
        state->A[state->pos / 8 + i] ^= load64(m + 8 * i);
    m += 8 * i;
    mlen -= 8 * i;
    state->pos += 8 * i;

    if (mlen) {
        for (i = 0; i < 8; i++)
            t[i] = 0;
        for (i = 0; i < mlen; i++)
            t[i] = m[i];
        state->A[state->pos / 8] ^= load64(t);
        state->pos += mlen;
    }
}

/*************************************************
* Name:        litchi_finalize
*
* Description: Finalize absorb step.
*
* Arguments:   - litchi_state* state:   pointer to  state
* 
**************************************************/
static void litchi_finalize(litchi_state* state)
{
    int i;

    state->A[state->pos >> 3] ^= 0x80ULL << 8 * (7 - (state->pos & 7));
    for (i = 0; i < 25; i++)
        state->S[i] = state->A[i];

    state->pos = 0;
}

/*************************************************
* Name:        litchi_squeezeblocks
*
* Description: Squeeze step of . Squeezes full blocks of r bytes each.
*              Modifies the state. Can be called multiple times to keep
*              squeezing, i.e., is incremental. Assumes zero bytes of current
*              block have already been squeezed.
*
* Arguments:   - uint8_t *out:        pointer to output blocks
*              - int nblocks:         number of blocks to be squeezed (written to out)
*              - litchi_state* state: pointer to input/output  state
* 
**************************************************/
static void litchi_squeezeblocks(uint8_t* out,
                                 int nblocks,
                                 litchi_state* state)
{
    uint64_t i, rb = 25 - (state->sec / 64);

    while (nblocks > 0) {
        state->S[24] ^= state->counter;
        for (i = 0; i < 25; i++)
            state->A[i] = state->S[i];
        F1600_StatePermute(state->A);
        for (i = rb; i < 25; i++) {
            state->A[i] ^= state->S[i];
            store64(out + 8 * (i - rb), state->A[i]);
        }
        state->S[24] ^= state->counter++;
        out += state->sec / 8;
        nblocks--;
    }
}

/*************************************************
* Name:        litchi_squeeze
*
* Description: Squeeze step of . Squeezes arbitratrily many bytes.
*              Modifies the state. Can be called multiple times to keep
*              squeezing, i.e., is incremental.
*
* Arguments:   - uint8_t *out:        pointer to output
*              - uint64_t outlen:     number of bytes to be squeezed (written to out)
*              - litchi_state* state: pointer to input/output  state
*
**************************************************/
static void litchi_squeeze(uint8_t* out,
                           uint64_t outlen,
                           litchi_state* state)
{
    uint8_t t[8];
    uint64_t i, cb = state->sec / 64, rb = 25 - cb, h = state->sec / 8;

    if (state->pos & 7) {
        store64(t, state->A[state->pos / 8 + rb]);
        i = state->pos & 7;
        while (i < 8 && outlen > 0) {
            *out++ = t[i++];
            outlen--;
            state->pos++;
        }
    }

    if (state->pos && outlen >= h - state->pos) {
        for (i = 0; i < (h - state->pos) / 8; i++)
            store64(out + 8 * i, state->A[state->pos / 8 + rb + i]);
        out += h - state->pos;
        outlen -= h - state->pos;
        state->pos = 0;
    }

    while (outlen >= h) {
        state->S[24] ^= state->counter;
        for (i = 0; i < 25; i++)
            state->A[i] = state->S[i];
        F1600_StatePermute(state->A);
        for (i = rb; i < 25; i++) {
            state->A[i] ^= state->S[i];
            store64(out + 8 * (i - rb), state->A[i]);
        }
        state->S[24] ^= state->counter++;
        out += h;
        outlen -= h;
    }

    if (!outlen)
        return;
    else if (!state->pos)
    {
        state->S[24] ^= state->counter;
        for (i = 0; i < 25; i++)
            state->A[i] = state->S[i];
        F1600_StatePermute(state->A);
        for (i = rb; i < 25; i++)
            state->A[i] ^= state->S[i];
        state->S[24] ^= state->counter++;
    }

    for (i = 0; i < outlen / 8; i++)
        store64(out + 8 * i, state->A[state->pos / 8 + rb + i]);
    out += 8 * i;
    outlen -= 8 * i;
    state->pos += 8 * i;

    store64(t, state->A[state->pos / 8 + rb]);
    for (i = 0; i < outlen; i++)
        out[i] = t[i];
    state->pos += outlen;
}

/*************************************************
* Name:        litchi_768_init
*
* Description: Litchi-768 Initilizes
*
* Arguments:   - litchi_state *state: pointer to (uninitialized) state
**************************************************/
void litchi_768_init(litchi_state* state)
{
    litchi_init(state, 832, 0);
}

/*************************************************
* Name:        litchi_768_absorb
*
* Description: Absorb step of the litchi_768.
*
* Arguments:   - litchi_state *state: pointer to (initialized) output state
*              - const uint8_t *in:   pointer to input to be absorbed into s
*              - uint64_t inlen:      length of input in bytes
**************************************************/
void litchi_768_absorb(litchi_state* state, const uint8_t* in, uint64_t inlen)
{
    litchi_absorb(state, in, inlen);
}

/*************************************************
* Name:        litchi_768_finalize
*
* Description: Finalize absorb step of the litchi_768.
*
* Arguments:   - litchi_state *state: pointer to state
**************************************************/
void litchi_768_finalize(litchi_state* state)
{
    litchi_finalize(state);
}

/*************************************************
* Name:        litchi_768_squeeze
*
* Description: Squeeze step of litchi_768. Squeezes 96
*              bytes. Can be called multiple times to keep squeezing.
*
* Arguments:   - uint8_t *out:        pointer to output blocks
*              - uint64_t outlen :    number of bytes to be squeezed
*                                     (written to output)
*              - litchi_state *state: pointer to input/output state
**************************************************/
void litchi_768_squeeze(uint8_t* out, uint64_t outlen, litchi_state* state)
{
    litchi_squeeze(out, outlen, state);
}

/*************************************************
* Name:        litchi_768
*
* Description: litchi-768 with non-incremental API
*
* Arguments:   - uint8_t *h:        pointer to output (96 bytes)
*              - const uint8_t *in: pointer to input
*              - uint64_t inlen:    length of input in bytes
**************************************************/
void litchi_768(uint8_t h[96], const uint8_t* in, uint64_t inlen)
{
    litchi_state state;

    litchi_768_init(&state);
    litchi_768_absorb(&state, in, inlen);
    litchi_768_finalize(&state);
    litchi_768_squeeze(h, 96, &state);
}