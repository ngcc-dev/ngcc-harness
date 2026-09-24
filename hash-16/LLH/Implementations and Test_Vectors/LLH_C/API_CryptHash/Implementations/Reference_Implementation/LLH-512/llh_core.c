#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if !defined(W)
#error "W must be defined before including llh_v2_core.c"
#endif

#define ROWS 24
#define RATE_ROWS 8
#define CAP_ROWS 16

typedef uint64_t word_t;

static inline word_t llh_rotl(word_t x, unsigned int n)
{
    return (word_t)((x << n) | (x >> (64u - n)));
}

#define WORD_NOT(a) (~(a))
#define WORD_XOR(a, b) ((a) ^ (b))
#define WORD_AND(a, b) ((a) & (b))


#define RATE_BITS (RATE_ROWS * W)
#define RATE_BYTES (RATE_BITS / 8)
#define OUTPUT_BYTES RATE_BYTES
#define NR_DEFAULT (8 + (W / 32) * 8)

typedef struct {
    word_t row[ROWS];
} State;


static const int SIGMA[ROWS] = {
    2,  25, 20, 51, 15, 40, 37, 6,
    36, 63, 62, 33, 1,  30, 31, 4,
    38, 5,  8,  47, 19, 52, 57, 34
};


#define LLH_W64_RC_LIST(X)                 \
    X(B7E151628AED2A6A)                    \
    X(BF7158809CF4F3C7)                    \
    X(62E7160F38B4DA56)                    \
    X(A784D9045190CFEF)                    \
    X(324E7738926CFBE5)                    \
    X(F4BF8D8D8C31D763)                    \
    X(DA06C80ABB1185EB)                    \
    X(4F7C7B5757F59584)                    \
    X(90CFD47D7C19BB42)                    \
    X(158D9554F7B46BCE)                    \
    X(D55C4D79FD5F24D6)                    \
    X(613C31C3839A2DDF)                    \
    X(8A9A276BCFBFA1C8)                    \
    X(77C56284DAB79CD4)                    \
    X(C2B3293D20E9E5EA)                    \
    X(F02AC60ACC93ED87)                    \
    X(4422A52ECB238FEE)                    \
    X(E5AB6ADD835FD1A0)                    \
    X(753D0A8F78E537D2)                    \
    X(B95BB79D8DCAEC64)                    \
    X(2C1E9F23B829B5C2)                    \
    X(780BF38737DF8BB3)                    \
    X(00D01334A0D0BD86)                    \
    X(45CBFA73A6160FFE)

#define LLH_W64_RC_ENTRY_WORD(v) 0x##v##ull,

static const word_t RC[24] = {
    LLH_W64_RC_LIST(LLH_W64_RC_ENTRY_WORD)
};


#undef LLH_W64_RC_ENTRY_WORD
#undef LLH_W64_RC_LIST


#define LLH_LOAD_U32_LE(src)                                                  \
    ((uint32_t)((src)[0]) | ((uint32_t)((src)[1]) << 8) |                    \
     ((uint32_t)((src)[2]) << 16) | ((uint32_t)((src)[3]) << 24))

#define LLH_STORE_U32_LE(dst, value)                                          \
    do {                                                                      \
        uint8_t *llh_dst_ = (dst);                                            \
        uint32_t llh_value_ = (uint32_t)(value);                              \
        llh_dst_[0] = (uint8_t)llh_value_;                                    \
        llh_dst_[1] = (uint8_t)(llh_value_ >> 8);                             \
        llh_dst_[2] = (uint8_t)(llh_value_ >> 16);                            \
        llh_dst_[3] = (uint8_t)(llh_value_ >> 24);                            \
    } while (0)

static inline word_t load_word(const uint8_t *p)
{
    return (word_t)LLH_LOAD_U32_LE(p) |
           ((word_t)LLH_LOAD_U32_LE(p + 4) << 32);
}

static inline void store_word(uint8_t *p, word_t w)
{
    LLH_STORE_U32_LE(p, (uint32_t)w);
    LLH_STORE_U32_LE(p + 4, (uint32_t)(w >> 32));
}

typedef struct {
    State state;
    uint8_t buf[RATE_BYTES];
    size_t buf_len;
} HashCtx;

#define LLH_MIX_FAMILY(dst, a, b, c, d)                                 \
    do {                                                                \
        state[(dst) + 0] = WORD_XOR(T[(b)], WORD_XOR(T[(c)], T[(d)])); \
        state[(dst) + 1] = WORD_XOR(T[(a)], WORD_XOR(T[(c)], T[(d)])); \
        state[(dst) + 2] = WORD_XOR(T[(a)], WORD_XOR(T[(b)], T[(d)])); \
        state[(dst) + 3] = WORD_XOR(T[(a)], WORD_XOR(T[(b)], T[(c)])); \
    } while (0)

static void llh_round(word_t *state, word_t rc)
{
    word_t T[ROWS];

    T[0] = WORD_NOT(WORD_XOR(state[0], WORD_XOR(state[8], WORD_AND(state[4], state[12]))));
    T[4] = WORD_NOT(WORD_XOR(state[4], WORD_XOR(state[12], WORD_AND(state[8], state[16]))));
    T[8] = WORD_XOR(state[8], WORD_XOR(state[16], WORD_AND(state[12], state[20])));
    T[12] = WORD_NOT(WORD_XOR(state[12], WORD_XOR(state[20], WORD_AND(state[16], state[0]))));
    T[16] = WORD_NOT(WORD_XOR(state[16], WORD_XOR(state[0], WORD_AND(state[20], state[4]))));
    T[20] = WORD_XOR(state[20], WORD_XOR(state[4], WORD_AND(state[0], state[8])));

    T[1] = WORD_NOT(WORD_XOR(state[1], WORD_XOR(state[9], WORD_AND(state[5], state[13]))));
    T[5] = WORD_NOT(WORD_XOR(state[5], WORD_XOR(state[13], WORD_AND(state[9], state[17]))));
    T[9] = WORD_XOR(state[9], WORD_XOR(state[17], WORD_AND(state[13], state[21])));
    T[13] = WORD_NOT(WORD_XOR(state[13], WORD_XOR(state[21], WORD_AND(state[17], state[1]))));
    T[17] = WORD_NOT(WORD_XOR(state[17], WORD_XOR(state[1], WORD_AND(state[21], state[5]))));
    T[21] = WORD_XOR(state[21], WORD_XOR(state[5], WORD_AND(state[1], state[9])));

    T[2] = WORD_NOT(WORD_XOR(state[2], WORD_XOR(state[10], WORD_AND(state[6], state[14]))));
    T[6] = WORD_NOT(WORD_XOR(state[6], WORD_XOR(state[14], WORD_AND(state[10], state[18]))));
    T[10] = WORD_XOR(state[10], WORD_XOR(state[18], WORD_AND(state[14], state[22])));
    T[14] = WORD_NOT(WORD_XOR(state[14], WORD_XOR(state[22], WORD_AND(state[18], state[2]))));
    T[18] = WORD_NOT(WORD_XOR(state[18], WORD_XOR(state[2], WORD_AND(state[22], state[6]))));
    T[22] = WORD_XOR(state[22], WORD_XOR(state[6], WORD_AND(state[2], state[10])));

    T[3] = WORD_NOT(WORD_XOR(state[3], WORD_XOR(state[11], WORD_AND(state[7], state[15]))));
    T[7] = WORD_NOT(WORD_XOR(state[7], WORD_XOR(state[15], WORD_AND(state[11], state[19]))));
    T[11] = WORD_XOR(state[11], WORD_XOR(state[19], WORD_AND(state[15], state[23])));
    T[15] = WORD_NOT(WORD_XOR(state[15], WORD_XOR(state[23], WORD_AND(state[19], state[3]))));
    T[19] = WORD_NOT(WORD_XOR(state[19], WORD_XOR(state[3], WORD_AND(state[23], state[7]))));
    T[23] = WORD_XOR(state[23], WORD_XOR(state[7], WORD_AND(state[3], state[11])));

    LLH_MIX_FAMILY(0, 4, 9, 18, 23);
    LLH_MIX_FAMILY(4, 8, 1, 22, 15);
    LLH_MIX_FAMILY(8, 12, 17, 6, 3);
    LLH_MIX_FAMILY(12, 20, 5, 10, 19);
    LLH_MIX_FAMILY(16, 0, 21, 14, 11);
    LLH_MIX_FAMILY(20, 16, 13, 2, 7);

    state[0] = llh_rotl(state[0], SIGMA[0]);
    state[1] = llh_rotl(state[1], SIGMA[1]);
    state[2] = llh_rotl(state[2], SIGMA[2]);
    state[3] = llh_rotl(state[3], SIGMA[3]);
    state[4] = llh_rotl(state[4], SIGMA[4]);
    state[5] = llh_rotl(state[5], SIGMA[5]);
    state[6] = llh_rotl(state[6], SIGMA[6]);
    state[7] = llh_rotl(state[7], SIGMA[7]);
    state[8] = llh_rotl(state[8], SIGMA[8]);
    state[9] = llh_rotl(state[9], SIGMA[9]);
    state[10] = llh_rotl(state[10], SIGMA[10]);
    state[11] = llh_rotl(state[11], SIGMA[11]);
    state[12] = llh_rotl(state[12], SIGMA[12]);
    state[13] = llh_rotl(state[13], SIGMA[13]);
    state[14] = llh_rotl(state[14], SIGMA[14]);
    state[15] = llh_rotl(state[15], SIGMA[15]);
    state[16] = llh_rotl(state[16], SIGMA[16]);
    state[17] = llh_rotl(state[17], SIGMA[17]);
    state[18] = llh_rotl(state[18], SIGMA[18]);
    state[19] = llh_rotl(state[19], SIGMA[19]);
    state[20] = llh_rotl(state[20], SIGMA[20]);
    state[21] = llh_rotl(state[21], SIGMA[21]);
    state[22] = llh_rotl(state[22], SIGMA[22]);
    state[23] = llh_rotl(state[23], SIGMA[23]);
    state[23] = WORD_XOR(state[23], rc);
}

static void permutation(State *s)
{
    for (int round = 0; round < NR_DEFAULT; round++) {
        llh_round(s->row, RC[round]);
    }
}

static void xor_rate(State *s, const uint8_t *block)
{
    const int row_bytes = W / 8;

    for (int i = 0; i < RATE_ROWS; i++) {
        s->row[i] = WORD_XOR(s->row[i], load_word(block + i * row_bytes));
    }
}

static void squeeze_rate(const State *s, uint8_t *out)
{
    const int row_bytes = W / 8;

    for (int i = 0; i < RATE_ROWS; i++) {
        store_word(out + i * row_bytes, s->row[i]);
    }
}

static void hash_process_block(HashCtx *ctx, const uint8_t *block)
{
    State prev = ctx->state;

    permutation(&ctx->state);
    for (int i = RATE_ROWS; i < ROWS; i++) {
        ctx->state.row[i] = WORD_XOR(ctx->state.row[i], prev.row[i]);
    }
    xor_rate(&ctx->state, block);
}

void hash_init(HashCtx *ctx, uint64_t msg_len_bits)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->state.row[23] = msg_len_bits;
}

void hash_absorb(HashCtx *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        ctx->buf[ctx->buf_len++] = data[i];
        if (ctx->buf_len == RATE_BYTES) {
            hash_process_block(ctx, ctx->buf);
            ctx->buf_len = 0;
        }
    }
}

void hash_final(HashCtx *ctx, uint8_t *out)
{
    uint8_t final_block[RATE_BYTES];

    if (ctx->buf_len != 0) {
        memcpy(final_block, ctx->buf, ctx->buf_len);
        memset(final_block + ctx->buf_len, 0xFF, RATE_BYTES - ctx->buf_len);
        hash_process_block(ctx, final_block);
        ctx->buf_len = 0;
    }

    permutation(&ctx->state);
    squeeze_rate(&ctx->state, out);
}

void hash(const uint8_t *msg, size_t msg_len, uint8_t *out)
{
    HashCtx ctx;

    hash_init(&ctx, (uint64_t)msg_len * 8ULL);
    hash_absorb(&ctx, msg, msg_len);
    hash_final(&ctx, out);
}
