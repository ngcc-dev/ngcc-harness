#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if !defined(W)
#error "W must be defined before including llh_v2_core.c"
#endif

#define ROWS 24
#define RATE_ROWS 8
#define CAP_ROWS 16

typedef struct {
    uint32_t hi;
    uint64_t lo;
} word_t;

static inline word_t word_not(word_t a)
{
    return (word_t){~a.hi, ~a.lo};
}

static inline word_t word_xor(word_t a, word_t b)
{
    return (word_t){a.hi ^ b.hi, a.lo ^ b.lo};
}

static inline word_t word_and(word_t a, word_t b)
{
    return (word_t){a.hi & b.hi, a.lo & b.lo};
}

/* 96 比特字按 X = hi || lo 表示，等价于 X = (hi << 64) | lo。
 * 这个函数只用于 SIGMA 表中的非零移位量；分支用于避开 C 中
 * 32/64 bit 宽度边界上的非法移位，同时模拟 96-bit 环形左移。
 */
static inline word_t llh_rotl(word_t x, unsigned int n)
{
    if (n < 32u) {
        /* 1..31 位：hi：旧hi左移，从 lo 的高 n 位补；lo：旧lo左移后从 hi 的高 n 位补。 */
        uint32_t hi = (uint32_t)((x.hi << n) | (uint32_t)(x.lo >> (64u - n)));
        uint64_t lo = (x.lo << n) | ((uint64_t)x.hi >> (32u - n));
        return (word_t){hi, lo};
    }

    if (n == 32u) {
        /* 32 位：按 [hi][lo_hi32][lo_lo32] -> [lo_hi32][lo_lo32][hi] 轮换。 */
        uint32_t hi = (uint32_t)(x.lo >> 32u);
        uint64_t lo = (x.lo << 32u) | (uint64_t)x.hi;
        return (word_t){hi, lo};
    }

    if (n < 64u) {
        /* 33..63 位，设 k = n - 32：
         * new_hi 取旧 lo 中移入最高 32 位的部分 32位截断；
         * new_lo 旧 lo 左移n位后剩余部分 | 旧 hi | 旧 lo 高 k 位。
         */
        unsigned int k = n - 32u;
        uint32_t hi = (uint32_t)(x.lo >> (32u - k));
        uint64_t lo = (x.lo << n) |
                      ((uint64_t)x.hi << k) |
                      (x.lo >> (64u - k));
        return (word_t){hi, lo};
    }

    /* 64..95 位，设 k = n - 64：
     * new_hi 旧 lo 的低 32 位左移 k 位 | 旧 hi 的高 k 位；
     * new_lo 旧 hi 的低32-k位 | 旧 lo 右移 32-k 位组成。
     */
    {
        unsigned int k = n - 64u;
        uint32_t hi = (uint32_t)((x.lo << k) |
                                 ((uint64_t)x.hi >> (32u - k)));
        uint64_t lo = ((uint64_t)x.hi << (32u + k)) |
                      (x.lo >> (32u - k));
        return (word_t){hi, lo};
    }
}

#define WORD_NOT(a) word_not(a)
#define WORD_XOR(a, b) word_xor((a), (b))
#define WORD_AND(a, b) word_and((a), (b))


#define RATE_BITS (RATE_ROWS * W)
#define RATE_BYTES (RATE_BITS / 8)
#define OUTPUT_BYTES RATE_BYTES
#define NR_DEFAULT (8 + (W / 32) * 8)

typedef struct {
    word_t row[ROWS];
} State;


static const int SIGMA[ROWS] = {
    2,  11, 28, 53, 19, 34, 57, 88,
    52, 73, 6,  43, 5,  32, 67, 14,
    70, 7,  48, 1,  55, 94, 45, 4
};


static const word_t RC[32] = {
    {0xB7E15162u, 0x8AED2A6ABF715880ull},
    {0x9CF4F3C7u, 0x62E7160F38B4DA56ull},
    {0xA784D904u, 0x5190CFEF324E7738ull},
    {0x926CFBE5u, 0xF4BF8D8D8C31D763ull},
    {0xDA06C80Au, 0xBB1185EB4F7C7B57ull},
    {0x57F59584u, 0x90CFD47D7C19BB42ull},
    {0x158D9554u, 0xF7B46BCED55C4D79ull},
    {0xFD5F24D6u, 0x613C31C3839A2DDFull},
    {0x8A9A276Bu, 0xCFBFA1C877C56284ull},
    {0xDAB79CD4u, 0xC2B3293D20E9E5EAull},
    {0xF02AC60Au, 0xCC93ED874422A52Eull},
    {0xCB238FEEu, 0xE5AB6ADD835FD1A0ull},
    {0x753D0A8Fu, 0x78E537D2B95BB79Dull},
    {0x8DCAEC64u, 0x2C1E9F23B829B5C2ull},
    {0x780BF387u, 0x37DF8BB300D01334ull},
    {0xA0D0BD86u, 0x45CBFA73A6160FFEull},
    {0x393C48CBu, 0xBBCA060F0FF8EC6Dull},
    {0x31BEB5CCu, 0xEED7F2F0BB088017ull},
    {0x163BC60Du, 0xF45A0ECB1BCD289Bull},
    {0x06CBBFEAu, 0x21AD08E1847F3F73ull},
    {0x78D56CEDu, 0x94640D6EF0D3D37Bull},
    {0xE67008E1u, 0x86D1BF275B9B241Dull},
    {0xEB64749Au, 0x47DFDFB96632C3EBull},
    {0x061B6472u, 0xBBF84C26144E49C2ull},
    {0xD04C324Eu, 0xF10DE513D3F5114Bull},
    {0x8B5D374Du, 0x93CB8879C7D52FFDull},
    {0x72BA0AAEu, 0x7277DA7BA1B4AF14ull},
    {0x88D8E836u, 0xAF14865E6C37AB68ull},
    {0x76FE690Bu, 0x571121382AF341AFull},
    {0xE94F77BCu, 0xF06C83B8FF5675F0ull},
    {0x979074ADu, 0x9A787BC5B9BD4B0Cull},
    {0x5937D3EDu, 0xE4C3A79396215EDAull}
};



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
    uint64_t lo = (uint64_t)LLH_LOAD_U32_LE(p) |
                  ((uint64_t)LLH_LOAD_U32_LE(p + 4) << 32);
    uint32_t hi = LLH_LOAD_U32_LE(p + 8);
    return (word_t){hi, lo};
}

static inline void store_word(uint8_t *p, word_t w)
{
    LLH_STORE_U32_LE(p, (uint32_t)w.lo);
    LLH_STORE_U32_LE(p + 4, (uint32_t)(w.lo >> 32));
    LLH_STORE_U32_LE(p + 8, w.hi);
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

    ctx->state.row[23] = (word_t){0u, msg_len_bits};
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
