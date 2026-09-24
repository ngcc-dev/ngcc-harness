/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#  define FORCE_INLINE __attribute__((always_inline)) inline
#else
#  define FORCE_INLINE inline
#endif

const uint64_t ROUND_CONSTANTS[24] = {
    0x243f6a8885a308d3ULL, 0x13198a2e03707344ULL, 0xa4093822299f31d0ULL, 0x082efa98ec4e6c89ULL,
    0x452821e638d01377ULL, 0xbe5466cf34e90c6cULL, 0xc0ac29b7c97c50ddULL, 0x3f84d5b5b5470917ULL,
    0x9216d5d98979fb1bULL, 0xd1310ba698dfb5acULL, 0x2ffd72dbd01adfb7ULL, 0xb8e1afed6a267e96ULL,
    0xba7c9045f12c7f99ULL, 0x24a19947b3916cf7ULL, 0x0801f2e2858efc16ULL, 0x636920d871574e69ULL,
    0xa458fea3f4933d7eULL, 0x0d95748f728eb658ULL, 0x718bcd5882154aeeULL, 0x7b54a41dc25a59b5ULL,
    0x9c30d5392af26013ULL, 0xc5d1b023286085f0ULL, 0xca417918b8db38efULL, 0x8e79dcb0603a180eULL
};


static FORCE_INLINE void sbox_hw25_bitslice64_withtemp(
    uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4,
    uint64_t* y0, uint64_t* y1, uint64_t* y2, uint64_t* y3, uint64_t* y4)
{
    uint64_t t0, t1, t2, t3, t4, t5;
    t0 = (x0 ^ x1);
    t1 = (x2 ^ x4);
    t2 = (x0 | x2);
    t3 = ~t0;
    t4 = (x3 ^ t3);
    t5 = (t1 ^ (x1 & x4));
    *y0 = ((((x0 ^ x4) & t3) ^ (x4 & t1)) ^ (x2 & t4));
    *y1 = ((t2 ^ (x0 & x4)) ^ (x1 | (x3 ^ t5)));
    *y2 = ((x2 ^ (x3 & t1)) ^ ((x0 ^ x4) | t4));
    *y3 = ((x1 ^ t2) ^ (x3 & t1));
    *y4 = ((x3 ^ t5) ^ (x2 & t4));
}

#define MUL(t0, t1, t2, t3, t4) do { \
        uint64_t mul_tmp = (t4); \
        (t4) = (t3); \
        (t3) = (t2); \
        (t2) = (t1) ^ mul_tmp; \
        (t1) = (t0); \
        (t0) = mul_tmp; \
    } while (0)

#define MDS(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) do { \
        x0  ^= x5;  x1  ^= x6;  x2  ^= x7;  x3  ^= x8;  x4  ^= x9;   \
        x10 ^= x15; x11 ^= x16; x12 ^= x17; x13 ^= x18; x14 ^= x19;  \
        x5  ^= x10; x6  ^= x11; x7  ^= x12; x8  ^= x13; x9  ^= x14;  \
        x15 ^= x4;  x16 ^= x0;  x17 ^= x1 ^ x4;  x18 ^= x2;  x19 ^= x3;   \
        MUL(x5, x6, x7, x8, x9);                     \
        x10 ^= x19; x11 ^= x15; x12 ^= x16 ^ x19; x13 ^= x17; x14 ^= x18;  \
        x0  ^= x5;  x1  ^= x6;  x2  ^= x7;  x3  ^= x8;  x4  ^= x9;   \
        x15 ^= x0; x16 ^= x1; x17 ^= x2; x18 ^= x3; x19 ^= x4;   \
        x5  ^= x10; x6  ^= x11; x7  ^= x12; x8  ^= x13; x9  ^= x14;  \
    } while (0)

#define SR_X(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) do { \
        x5 = ((x5&0x7777777777777777ULL) << 1) | ((x5&0x8888888888888888ULL) >> 3); \
        x6 = ((x6&0x7777777777777777ULL) << 1) | ((x6&0x8888888888888888ULL) >> 3); \
        x7 = ((x7&0x7777777777777777ULL) << 1) | ((x7&0x8888888888888888ULL) >> 3); \
        x8 = ((x8&0x7777777777777777ULL) << 1) | ((x8&0x8888888888888888ULL) >> 3); \
        x9 = ((x9&0x7777777777777777ULL) << 1) | ((x9&0x8888888888888888ULL) >> 3); \
        x10 = ((x10&0x3333333333333333ULL) << 2) | ((x10&0xccccccccccccccccULL) >> 2); \
        x11 = ((x11&0x3333333333333333ULL) << 2) | ((x11&0xccccccccccccccccULL) >> 2); \
        x12 = ((x12&0x3333333333333333ULL) << 2) | ((x12&0xccccccccccccccccULL) >> 2); \
        x13 = ((x13&0x3333333333333333ULL) << 2) | ((x13&0xccccccccccccccccULL) >> 2); \
        x14 = ((x14&0x3333333333333333ULL) << 2) | ((x14&0xccccccccccccccccULL) >> 2); \
        x15 = ((x15&0x1111111111111111ULL) << 3) | ((x15&0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x16 = ((x16&0x1111111111111111ULL) << 3) | ((x16&0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x17 = ((x17&0x1111111111111111ULL) << 3) | ((x17&0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x18 = ((x18&0x1111111111111111ULL) << 3) | ((x18&0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x19 = ((x19&0x1111111111111111ULL) << 3) | ((x19&0xeeeeeeeeeeeeeeeeULL) >> 1); \
    } while (0)

#define SR_X_INV(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) do { \
        x5  = ((x5 &0x1111111111111111ULL) << 3) | ((x5 &0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x6  = ((x6 &0x1111111111111111ULL) << 3) | ((x6 &0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x7  = ((x7 &0x1111111111111111ULL) << 3) | ((x7 &0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x8  = ((x8 &0x1111111111111111ULL) << 3) | ((x8 &0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x9  = ((x9 &0x1111111111111111ULL) << 3) | ((x9 &0xeeeeeeeeeeeeeeeeULL) >> 1); \
        x10 = ((x10&0x3333333333333333ULL) << 2) | ((x10&0xccccccccccccccccULL) >> 2); \
        x11 = ((x11&0x3333333333333333ULL) << 2) | ((x11&0xccccccccccccccccULL) >> 2); \
        x12 = ((x12&0x3333333333333333ULL) << 2) | ((x12&0xccccccccccccccccULL) >> 2); \
        x13 = ((x13&0x3333333333333333ULL) << 2) | ((x13&0xccccccccccccccccULL) >> 2); \
        x14 = ((x14&0x3333333333333333ULL) << 2) | ((x14&0xccccccccccccccccULL) >> 2); \
        x15 = ((x15&0x7777777777777777ULL) << 1) | ((x15&0x8888888888888888ULL) >> 3); \
        x16 = ((x16&0x7777777777777777ULL) << 1) | ((x16&0x8888888888888888ULL) >> 3); \
        x17 = ((x17&0x7777777777777777ULL) << 1) | ((x17&0x8888888888888888ULL) >> 3); \
        x18 = ((x18&0x7777777777777777ULL) << 1) | ((x18&0x8888888888888888ULL) >> 3); \
        x19 = ((x19&0x7777777777777777ULL) << 1) | ((x19&0x8888888888888888ULL) >> 3); \
    } while (0)

#define SR_Y(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) do { \
        x5  = ((x5 &0x0fff0fff0fff0fffULL) << 4)  | ((x5 &0xf000f000f000f000ULL) >> 12); \
        x6  = ((x6 &0x0fff0fff0fff0fffULL) << 4)  | ((x6 &0xf000f000f000f000ULL) >> 12); \
        x7  = ((x7 &0x0fff0fff0fff0fffULL) << 4)  | ((x7 &0xf000f000f000f000ULL) >> 12); \
        x8  = ((x8 &0x0fff0fff0fff0fffULL) << 4)  | ((x8 &0xf000f000f000f000ULL) >> 12); \
        x9  = ((x9 &0x0fff0fff0fff0fffULL) << 4)  | ((x9 &0xf000f000f000f000ULL) >> 12); \
        x10 = ((x10&0x00ff00ff00ff00ffULL) << 8)  | ((x10&0xff00ff00ff00ff00ULL) >> 8); \
        x11 = ((x11&0x00ff00ff00ff00ffULL) << 8)  | ((x11&0xff00ff00ff00ff00ULL) >> 8); \
        x12 = ((x12&0x00ff00ff00ff00ffULL) << 8)  | ((x12&0xff00ff00ff00ff00ULL) >> 8); \
        x13 = ((x13&0x00ff00ff00ff00ffULL) << 8)  | ((x13&0xff00ff00ff00ff00ULL) >> 8); \
        x14 = ((x14&0x00ff00ff00ff00ffULL) << 8)  | ((x14&0xff00ff00ff00ff00ULL) >> 8); \
        x15 = ((x15&0x000f000f000f000fULL) << 12) | ((x15&0xfff0fff0fff0fff0ULL) >> 4); \
        x16 = ((x16&0x000f000f000f000fULL) << 12) | ((x16&0xfff0fff0fff0fff0ULL) >> 4); \
        x17 = ((x17&0x000f000f000f000fULL) << 12) | ((x17&0xfff0fff0fff0fff0ULL) >> 4); \
        x18 = ((x18&0x000f000f000f000fULL) << 12) | ((x18&0xfff0fff0fff0fff0ULL) >> 4); \
        x19 = ((x19&0x000f000f000f000fULL) << 12) | ((x19&0xfff0fff0fff0fff0ULL) >> 4); \
    } while (0)

#define SR_Y_INV(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) do { \
        x5  = ((x5 &0x000f000f000f000fULL) << 12) | ((x5 &0xfff0fff0fff0fff0ULL) >> 4); \
        x6  = ((x6 &0x000f000f000f000fULL) << 12) | ((x6 &0xfff0fff0fff0fff0ULL) >> 4); \
        x7  = ((x7 &0x000f000f000f000fULL) << 12) | ((x7 &0xfff0fff0fff0fff0ULL) >> 4); \
        x8  = ((x8 &0x000f000f000f000fULL) << 12) | ((x8 &0xfff0fff0fff0fff0ULL) >> 4); \
        x9  = ((x9 &0x000f000f000f000fULL) << 12) | ((x9 &0xfff0fff0fff0fff0ULL) >> 4); \
        x10 = ((x10&0x00ff00ff00ff00ffULL) << 8)  | ((x10&0xff00ff00ff00ff00ULL) >> 8); \
        x11 = ((x11&0x00ff00ff00ff00ffULL) << 8)  | ((x11&0xff00ff00ff00ff00ULL) >> 8); \
        x12 = ((x12&0x00ff00ff00ff00ffULL) << 8)  | ((x12&0xff00ff00ff00ff00ULL) >> 8); \
        x13 = ((x13&0x00ff00ff00ff00ffULL) << 8)  | ((x13&0xff00ff00ff00ff00ULL) >> 8); \
        x14 = ((x14&0x00ff00ff00ff00ffULL) << 8)  | ((x14&0xff00ff00ff00ff00ULL) >> 8); \
        x15 = ((x15&0x0fff0fff0fff0fffULL) << 4)  | ((x15&0xf000f000f000f000ULL) >> 12); \
        x16 = ((x16&0x0fff0fff0fff0fffULL) << 4)  | ((x16&0xf000f000f000f000ULL) >> 12); \
        x17 = ((x17&0x0fff0fff0fff0fffULL) << 4)  | ((x17&0xf000f000f000f000ULL) >> 12); \
        x18 = ((x18&0x0fff0fff0fff0fffULL) << 4)  | ((x18&0xf000f000f000f000ULL) >> 12); \
        x19 = ((x19&0x0fff0fff0fff0fffULL) << 4)  | ((x19&0xf000f000f000f000ULL) >> 12); \
    } while (0)

#define SR_Z(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) do { \
        x5  = (x5  << 16) | (x5  >> 48); \
        x6  = (x6  << 16) | (x6  >> 48); \
        x7  = (x7  << 16) | (x7  >> 48); \
        x8  = (x8  << 16) | (x8  >> 48); \
        x9  = (x9  << 16) | (x9  >> 48); \
        x10 = (x10 << 32) | (x10 >> 32); \
        x11 = (x11 << 32) | (x11 >> 32); \
        x12 = (x12 << 32) | (x12 >> 32); \
        x13 = (x13 << 32) | (x13 >> 32); \
        x14 = (x14 << 32) | (x14 >> 32); \
        x15 = (x15 << 48) | (x15 >> 16); \
        x16 = (x16 << 48) | (x16 >> 16); \
        x17 = (x17 << 48) | (x17 >> 16); \
        x18 = (x18 << 48) | (x18 >> 16); \
        x19 = (x19 << 48) | (x19 >> 16); \
    } while (0)

#define SR_Z_INV(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19) do { \
        x5  = (x5  << 48) | (x5  >> 16); \
        x6  = (x6  << 48) | (x6  >> 16); \
        x7  = (x7  << 48) | (x7  >> 16); \
        x8  = (x8  << 48) | (x8  >> 16); \
        x9  = (x9  << 48) | (x9  >> 16); \
        x10 = (x10 << 32) | (x10 >> 32); \
        x11 = (x11 << 32) | (x11 >> 32); \
        x12 = (x12 << 32) | (x12 >> 32); \
        x13 = (x13 << 32) | (x13 >> 32); \
        x14 = (x14 << 32) | (x14 >> 32); \
        x15 = (x15 << 16) | (x15 >> 48); \
        x16 = (x16 << 16) | (x16 >> 48); \
        x17 = (x17 << 16) | (x17 >> 48); \
        x18 = (x18 << 16) | (x18 >> 48); \
        x19 = (x19 << 16) | (x19 >> 48); \
    } while (0)

/* Apply S-box to the 20-word state in-place (four parallel 5-word S-boxes) */
#define APPLY_SBOX20() do { \
    sbox_hw25_bitslice64_withtemp(x0, x1, x2, x3, x4,  &x0,  &x1,  &x2,  &x3,  &x4); \
    sbox_hw25_bitslice64_withtemp(x5, x6, x7, x8, x9,  &x5,  &x6,  &x7,  &x8,  &x9); \
    sbox_hw25_bitslice64_withtemp(x10,x11,x12,x13,x14, &x10, &x11, &x12, &x13, &x14); \
    sbox_hw25_bitslice64_withtemp(x15,x16,x17,x18,x19, &x15, &x16, &x17, &x18, &x19); \
} while (0)

void XRH1280(int R0, int R1, uint64_t *restrict buf)
{
    /* Load state into locals once */
    uint64_t x0 = buf[0],  x1 = buf[1],  x2 = buf[2],  x3 = buf[3],  x4 = buf[4];
    uint64_t x5 = buf[5],  x6 = buf[6],  x7 = buf[7],  x8 = buf[8],  x9 = buf[9];
    uint64_t x10 = buf[10], x11 = buf[11], x12 = buf[12], x13 = buf[13], x14 = buf[14];
    uint64_t x15 = buf[15], x16 = buf[16], x17 = buf[17], x18 = buf[18], x19 = buf[19];

    int phase = R0 % 3; /* equivalent to i % 3 but without modulo */
    for (int i = R0; i < R1; ++i) {
        /* Layer 1: 4 parallel S-boxes */
        APPLY_SBOX20();

        /* MDS */
        MDS(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);

        /* Layer 2: 4 parallel S-boxes */
        APPLY_SBOX20();

        /* SR by phase */
        if (phase == 0) {
            SR_X(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);
        } else if (phase == 1) {
            SR_Y(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);
        } else {
            SR_Z(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);
        }

        /* MDS */
        MDS(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);

        /* inverse SR matching the same phase */
        if (phase == 0) {
            SR_X_INV(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);
        } else if (phase == 1) {
            SR_Y_INV(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);
        } else {
            SR_Z_INV(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19);
        }

        /* Add round constant to x0 */
        x0 ^= ROUND_CONSTANTS[i];

        /* advance phase: 0 -> 1 -> 2 -> 0 */
        phase += 1;
        if (phase == 3) phase = 0;
    }

    /* Store back once */
    buf[0]=x0;  buf[1]=x1;  buf[2]=x2;  buf[3]=x3;  buf[4]=x4;
    buf[5]=x5;  buf[6]=x6;  buf[7]=x7;  buf[8]=x8;  buf[9]=x9;
    buf[10]=x10; buf[11]=x11; buf[12]=x12; buf[13]=x13; buf[14]=x14;
    buf[15]=x15; buf[16]=x16; buf[17]=x17; buf[18]=x18; buf[19]=x19;
}

void XRH_dm_permute(uint64_t state[STATE_LANES]) {
    volatile uint64_t initial_state[STATE_LANES];
    memcpy((void*)initial_state, state, sizeof(uint64_t) * STATE_LANES);
    XRH1280(0, 18, state);
    for (int j = 0; j < STATE_LANES; j++) state[j] ^= initial_state[j];
}

static void XRH_dm_init(XRH_state *state, uint32_t rate_bits) {
    memset(state->state, 0, STATE_LANES * sizeof(uint64_t));
    state->rate_bits = rate_bits;
}


int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    XRH_state state;
    uint8_t *state_bytes = (uint8_t*)state.state;
    XRH_dm_init(&state, STATE_RATE);
    int rate_bytes = state.rate_bits / 8;
    for (size_t i = 0; i < ((msg_len_bits + state.rate_bits) / state.rate_bits); i++) {
        if (msg_len_bits - state.rate_bits*i >= state.rate_bits) {
            for (size_t j = 0; j < rate_bytes; j++) state_bytes[j] ^= msg[j];
            XRH_dm_permute(state.state);
            msg += rate_bytes; 
        }else {
            size_t length = (msg_len_bits - state.rate_bits*i) / 8;
            size_t tail_bits = msg_len_bits & 0b111;
            if (length > 0) {
                for (size_t j = 0; j < length; j++) state_bytes[j] ^= msg[j];
            }
            if (tail_bits > 0) {
                state_bytes[length] ^= msg[length]&(0xFF << (8 - tail_bits));
            }
            state_bytes[length] ^= (0x80 >> tail_bits);
            if (msg_len_bits % state.rate_bits == state.rate_bits - 1) {
                XRH_dm_permute(state.state);
            }
            state_bytes[rate_bytes - 1] ^= 0x01; 
            XRH_dm_permute(state.state);
        }
    }

    size_t bits_out = 0;
    size_t bits_to_squeeze = (digest_len_bits - bits_out < state.rate_bits) ? (digest_len_bits - bits_out) : state.rate_bits;
    memcpy(digest + (bits_out / 8), state_bytes, bits_to_squeeze/8);
    bits_out += bits_to_squeeze;
    while (bits_out < digest_len_bits) {
        XRH1280(0, 18, (state.state));
        size_t bits_to_squeeze = (digest_len_bits - bits_out < state.rate_bits) ? (digest_len_bits - bits_out) : state.rate_bits;
        memcpy(digest + (bits_out / 8), state_bytes, bits_to_squeeze/8);
        bits_out += bits_to_squeeze;
    }

    return 0;
}