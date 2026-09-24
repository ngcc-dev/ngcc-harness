#include "sig_core.h"

#include "auxfunc.h"
#include "rsencode.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern DRNG_ctx drng_algorithm;

enum
{
    GF_REDUCTION_POLY = 0x002D
};

/*
 * SIG256_OPT uses the sparse extension modulus
 *   y^16 + y^11 + y + x
 * over F_(2^16), where x is the base-field indeterminate.
 * This keeps reduction to two xor folds plus one multiply-by-x.
 */

static uint16_t gf_log_table[1u << 16];
static uint16_t gf_exp_table[(1u << 16) - 1u];
static int gf_tables_ready = 0;
static int rs_tables_ready = 0;
static int g_polydef_ready = 0;
static int g_zero_merkle_ready = 0;
static RawGF2EX g_polydef_coeffs[7];
static unsigned char g_zero_merkle_nodes[MERKLE_NODE_COUNT][HASH_DIGEST_LENGTH];
static uint8_t g_merkle_live_nodes[MERKLE_NODE_COUNT];
static int g_merkle_recompute_nodes[MERKLE_NODE_COUNT];
static int g_merkle_recompute_count = 0;

#ifdef SIG_PROFILE_STEPS
SigStepProfile g_sig_step_profile;

void sig_profile_reset(void)
{
    memset(&g_sig_step_profile, 0, sizeof(g_sig_step_profile));
}

void sig_profile_snapshot(SigStepProfile *out)
{
    if (out != NULL)
    {
        *out = g_sig_step_profile;
    }
}
#endif

typedef struct
{
    unsigned total_elements;
    unsigned elements_per_block;
    unsigned k;
    unsigned recovery_block_count;
    unsigned work_count;
    uint16_t *original_buffer;
    const void **original_ptrs;
    uint16_t *work_storage;
    void **work_ptrs;
} RSWorkspace;

typedef struct
{
    unsigned char (*hash_nodes)[HASH_DIGEST_LENGTH];
    unsigned char *root_hash;
    const RawGF2EX *e_hat_group;
    const RawGF2EX *v_group;
#ifdef SIG_PROFILE_STEPS
    uint64_t profile_leaf_hash_ns;
    uint64_t profile_tree_hash_ns;
#endif
} MerkleBuildTask;

typedef struct
{
    pthread_t threads[total_groups];
    MerkleBuildTask tasks[total_groups];
    int launched[total_groups];
#ifdef SIG_PROFILE_STEPS
    uint64_t profile_thread_launch_ns;
    uint64_t profile_join_ns;
#endif
} MerkleBuildState;

typedef struct
{
    RSWorkspace *workspace;
    RawGF2EX *encoded;
    const RawGF2EX *prime;
#ifdef SIG_PROFILE_STEPS
    uint64_t profile_encode_ns;
#endif
} RSEncodeTask;

static RSWorkspace g_rs_workspaces[2];

static void sig_abort(const char *message)
{
    fprintf(stderr, "%s\n", message);
    abort();
}

static void *sig_malloc_zero(size_t bytes)
{
    void *ptr = calloc(1u, bytes);
    if (ptr == NULL)
    {
        sig_abort("ERROR: allocation failed");
    }
    return ptr;
}

static void sig_get_random_bytes(unsigned char *out, size_t len)
{
    if (get_random_number(&drng_algorithm, out, (unsigned long long)len * 8ULL) != 0)
    {
        sig_abort("ERROR: get_random_number failed");
    }
}

static uint16_t sig_random_u16(void)
{
    unsigned char bytes[2];
    sig_get_random_bytes(bytes, sizeof(bytes));
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static void sig_hash_bytes(const unsigned char *input, size_t input_len, unsigned char out[HASH_DIGEST_LENGTH])
{
    if (pseudohash(512, input, ((unsigned long long)input_len) * 8ULL, out) != 0)
    {
        sig_abort("ERROR: pseudohash failed");
    }
}

/* Keep the historical workspace-init hook without re-zeroing the full cached signer state. */
void sig_init_prover_workspace(ProverWorkspace *ws)
{
    (void)ws;
}

/* Keep the historical workspace-init hook without re-zeroing the full cached verifier state. */
void sig_init_verifier_workspace(VerifierWorkspace *ws)
{
    (void)ws;
}

static void sig_hash_ctx_init(SIG_HASH_CTX *ctx)
{
    ctx->len = 0;
}

static void sig_hash_ctx_update(SIG_HASH_CTX *ctx, const unsigned char *data, size_t len)
{
    if (ctx->len + len > HASH_CTX_CAPACITY)
    {
        sig_abort("ERROR: hash buffer overflow");
    }
    memcpy(ctx->buf + ctx->len, data, len);
    ctx->len += len;
}

static void sig_hash_ctx_final(const SIG_HASH_CTX *ctx, unsigned char out[HASH_DIGEST_LENGTH])
{
    sig_hash_bytes(ctx->buf, ctx->len, out);
}

static void sig_hash_ctx_copy(SIG_HASH_CTX *dst, const SIG_HASH_CTX *src)
{
    dst->len = src->len;
    memcpy(dst->buf, src->buf, src->len);
}

static uint16_t gf_mul_basic(uint16_t a, uint16_t b)
{
    int i;
    uint16_t result = 0;

    for (i = 0; i < 16; ++i)
    {
        if ((b & 1u) != 0u)
        {
            result ^= a;
        }
        if ((a & 0x8000u) != 0u)
        {
            a = (uint16_t)(a << 1);
            a ^= GF_REDUCTION_POLY;
        }
        else
        {
            a = (uint16_t)(a << 1);
        }
        b = (uint16_t)(b >> 1);
    }

    return result;
}

static void gf_init_tables(void)
{
    uint16_t x = 1;
    uint32_t i;

    if (gf_tables_ready)
    {
        return;
    }

    for (i = 0; i < ((1u << 16) - 1u); ++i)
    {
        gf_exp_table[i] = x;
        gf_log_table[x] = (uint16_t)i;
        x = gf_mul_basic(x, 2u);
    }
    gf_log_table[0] = 0;
    gf_tables_ready = 1;
}

static uint16_t mul_gf2_16(uint16_t a, uint16_t b)
{
    uint32_t sum;

    if (a == 0u || b == 0u)
    {
        return 0u;
    }
    if (!gf_tables_ready)
    {
        gf_init_tables();
    }
    sum = (uint32_t)gf_log_table[a] + (uint32_t)gf_log_table[b];
    if (sum >= ((1u << 16) - 1u))
    {
        sum -= ((1u << 16) - 1u);
    }
    return gf_exp_table[sum];
}

static uint16_t mul_gf2_16_by_log(uint16_t value, uint16_t log_factor)
{
    uint32_t sum;

    if (value == 0u)
    {
        return 0u;
    }
    sum = (uint32_t)gf_log_table[value] + (uint32_t)log_factor;
    if (sum >= ((1u << 16) - 1u))
    {
        sum -= ((1u << 16) - 1u);
    }
    return gf_exp_table[sum];
}

static uint16_t mul_gf2_16_x(uint16_t value)
{
    if ((value & 0x8000u) != 0u)
    {
        return (uint16_t)((value << 1) ^ GF_REDUCTION_POLY);
    }
    return (uint16_t)(value << 1);
}

static void add_raw(RawGF2EX *r, const RawGF2EX *a, const RawGF2EX *b)
{
    int i;
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        r->coeffs[i] = (uint16_t)(a->coeffs[i] ^ b->coeffs[i]);
    }
}

static uint8_t raw_collect_nonzero_terms(const RawGF2EX *poly, RawNZTerms *terms)
{
    uint8_t count = 0u;
    int i;

    if (!gf_tables_ready)
    {
        gf_init_tables();
    }

    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        uint16_t coeff = poly->coeffs[i];
        if (coeff != 0u)
        {
            terms->index[count] = (uint8_t)i;
            terms->log_value[count] = gf_log_table[coeff];
            ++count;
        }
    }
    terms->count = count;
    return count;
}

void precompute_H_hat_terms(RawNZTerms H_hat_terms[ROWS][COLS], const RawGF2EX H_hat_raw[ROWS][COLS])
{
    int row;

    for (row = 0; row < ROWS; ++row)
    {
        int col;
        for (col = 0; col < COLS; ++col)
        {
            raw_collect_nonzero_terms(&H_hat_raw[row][col], &H_hat_terms[row][col]);
        }
    }
}

static uint16_t gf_mul_from_logs(uint16_t log_a, uint16_t log_b)
{
    uint32_t sum = (uint32_t)log_a + (uint32_t)log_b;

    if (sum >= ((1u << 16) - 1u))
    {
        sum -= ((1u << 16) - 1u);
    }
    return gf_exp_table[sum];
}

static void reduced_term_xor(uint16_t out[RAW_POLY_LEN], unsigned degree, uint16_t value)
{
    if (degree < RAW_POLY_LEN)
    {
        out[degree] ^= value;
        return;
    }

    reduced_term_xor(out, degree - 5u, value);
    out[degree - 15u] ^= value;
    out[degree - 16u] ^= mul_gf2_16_x(value);
}

static void mul_reduce_low_terms_into_coeffs(uint16_t out[RAW_POLY_LEN], const RawNZTerms *h_terms, const RawNZTerms *poly_terms)
{
    uint8_t ih;

    for (ih = 0u; ih < h_terms->count; ++ih)
    {
        uint8_t ip;
        unsigned h_degree = (unsigned)h_terms->index[ih];
        uint16_t log_h = h_terms->log_value[ih];

        for (ip = 0u; ip < poly_terms->count; ++ip)
        {
            unsigned degree = h_degree + (unsigned)poly_terms->index[ip];
            uint16_t value = gf_mul_from_logs(log_h, poly_terms->log_value[ip]);

            if (degree < RAW_POLY_LEN)
            {
                out[degree] ^= value;
            }
            else
            {
                out[degree - 5u] ^= value;
                out[degree - 15u] ^= value;
                out[degree - 16u] ^= mul_gf2_16_x(value);
            }
        }
    }
}

static void mul_raw(RawGF2EX_Product *r, const RawGF2EX *a, const RawGF2EX *b)
{
    memset(r->coeffs, 0, sizeof(r->coeffs));
    {
        RawNZTerms a_terms;
        RawNZTerms b_terms;
        uint8_t ia;

        if (raw_collect_nonzero_terms(a, &a_terms) == 0u || raw_collect_nonzero_terms(b, &b_terms) == 0u)
        {
            return;
        }

        for (ia = 0u; ia < a_terms.count; ++ia)
        {
            uint8_t ib;
            unsigned base_degree = (unsigned)a_terms.index[ia];
            uint16_t log_a = a_terms.log_value[ia];

            for (ib = 0u; ib < b_terms.count; ++ib)
            {
                r->coeffs[base_degree + (unsigned)b_terms.index[ib]] ^=
                    gf_mul_from_logs(log_a, b_terms.log_value[ib]);
            }
        }
    }
}

static void build_mul_reduce_map(RawNZTerms map[RAW_POLY_LEN], const RawNZTerms *b_terms)
{
    int input_degree;

    for (input_degree = 0; input_degree < RAW_POLY_LEN; ++input_degree)
    {
        RawGF2EX row;
        uint8_t ib;

        memset(row.coeffs, 0, sizeof(row.coeffs));
        for (ib = 0u; ib < b_terms->count; ++ib)
        {
            reduced_term_xor(
                row.coeffs,
                (unsigned)input_degree + (unsigned)b_terms->index[ib],
                gf_exp_table[b_terms->log_value[ib]]);
        }
        raw_collect_nonzero_terms(&row, &map[input_degree]);
    }
}

static void mul_reduce_raw_by_map(RawGF2EX *r, const RawGF2EX *a, const RawNZTerms map[RAW_POLY_LEN])
{
    int input_degree;

    memset(r->coeffs, 0, sizeof(r->coeffs));
    for (input_degree = 0; input_degree < RAW_POLY_LEN; ++input_degree)
    {
        uint16_t value = a->coeffs[input_degree];
        const RawNZTerms *terms;
        uint16_t log_value;
        uint8_t i;

        if (value == 0u)
        {
            continue;
        }
        log_value = gf_log_table[value];
        terms = &map[input_degree];
        for (i = 0u; i < terms->count; ++i)
        {
            r->coeffs[terms->index[i]] ^= gf_mul_from_logs(log_value, terms->log_value[i]);
        }
    }
}

static void rem_raw(RawGF2EX *r, const RawGF2EX_Product *c)
{
    uint16_t tmp[RAW_PRODUCT_LEN];
    int i;

    memcpy(tmp, c->coeffs, sizeof(tmp));
    for (i = RAW_PRODUCT_LEN - 1; i >= RAW_POLY_LEN; --i)
    {
        uint16_t lead = tmp[i];

        if (lead == 0u)
        {
            continue;
        }

        /* y^16 = y^11 + y + x in characteristic two. */
        tmp[i - 5] ^= lead;
        tmp[i - 15] ^= lead;
        tmp[i - 16] ^= mul_gf2_16_x(lead);
        tmp[i] = 0u;
    }
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        r->coeffs[i] = tmp[i];
    }
}

static void scalar_mul_raw(RawGF2EX *r, uint16_t s, const RawGF2EX *a)
{
    int i;
    uint16_t log_s;

    if (s == 0u)
    {
        memset(r->coeffs, 0, sizeof(r->coeffs));
        return;
    }
    if (!gf_tables_ready)
    {
        gf_init_tables();
    }
    log_s = gf_log_table[s];
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        uint16_t ai = a->coeffs[i];
        r->coeffs[i] = (ai == 0u) ? 0u : gf_mul_from_logs(log_s, gf_log_table[ai]);
    }
}

static void scalar_mul_add_raw(RawGF2EX *r, uint16_t s, const RawGF2EX *a)
{
    int i;
    uint16_t log_s;

    if (s == 0u)
    {
        return;
    }
    if (!gf_tables_ready)
    {
        gf_init_tables();
    }
    log_s = gf_log_table[s];
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        uint16_t ai = a->coeffs[i];
        if (ai != 0u)
        {
            r->coeffs[i] ^= gf_mul_from_logs(log_s, gf_log_table[ai]);
        }
    }
}

static void assign_raw(RawGF2EX *dest, const RawGF2EX *src)
{
    memcpy(dest->coeffs, src->coeffs, sizeof(dest->coeffs));
}

static int raw_equals(const RawGF2EX *a, const RawGF2EX *b)
{
    return memcmp(a->coeffs, b->coeffs, sizeof(a->coeffs)) == 0;
}

/* Compute syndrome bits y = H * e over GF(2). */
void generate_y(unsigned long y[embded_num], unsigned long e[M], unsigned long H[embded_num][M])
{
    int i;

    for (i = 0; i < embded_num; ++i)
    {
        int j;
        y[i] = 0;
        for (j = 0; j < M; ++j)
        {
            y[i] = (y[i] + H[i][j] * e[j]) % 2u;
        }
    }
}

#define SIG_XOF_LABEL_H "SIG256:H"
#define SIG_XOF_LABEL_E "SIG256:e"
#define SIG_XOF_LABEL_COMMITMENT "SIG256:commitment"
#define SIG_XOF_LABEL_INTERCHECK "SIG256:intercheck"
#define SIG_XOF_LABEL_POLYRES "SIG256:polyres"
#define SIG_XOF_LABEL_MASK "SIG256:mask"
#define SIG_XOF_LABEL_OPEN "SIG256:open"

static unsigned char *derive_xof_bytes(const char *label, const unsigned char *material, size_t material_len, unsigned long long output_len_bits)
{
    size_t label_len = strlen(label);
    size_t input_len = label_len + material_len;
    size_t output_len = (size_t)((output_len_bits + 7ULL) / 8ULL);
    unsigned char input_stack[256];
    unsigned char *input = input_stack;
    unsigned char *output = (unsigned char *)malloc(output_len);

    if (input_len > sizeof(input_stack))
    {
        input = (unsigned char *)malloc(input_len);
    }
    if (input == NULL || output == NULL)
    {
        if (input != input_stack)
        {
            free(input);
        }
        free(output);
        sig_abort("ERROR: XOF allocation failed");
    }
    memcpy(input, label, label_len);
    memcpy(input + label_len, material, material_len);
    if (pseudoXOF(output_len_bits, input, ((unsigned long long)input_len) * 8ULL, output) != 0)
    {
        if (input != input_stack)
        {
            free(input);
        }
        free(output);
        sig_abort("ERROR: pseudoXOF failed");
    }
    if (input != input_stack)
    {
        free(input);
    }
    return output;
}

static uint16_t load_u16_be(const unsigned char *bytes)
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | (uint16_t)bytes[1]);
}

static void fill_random_raw_poly(RawGF2EX *poly)
{
    int i;
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        poly->coeffs[i] = sig_random_u16();
    }
}

static void fill_random_raw_poly_array(RawGF2EX *polys, size_t count)
{
    size_t total_coeffs = count * (size_t)RAW_POLY_LEN;
    size_t total_bytes = total_coeffs * 2u;
    unsigned char *bytes;
    size_t i;

    if (count == 0u)
    {
        return;
    }

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    if (sizeof(RawGF2EX) == (size_t)RAW_POLY_BYTES)
    {
        sig_get_random_bytes((unsigned char *)polys, total_bytes);
        return;
    }
#endif

    bytes = (unsigned char *)malloc(total_bytes);
    if (bytes == NULL)
    {
        sig_abort("ERROR: random batch allocation failed");
    }
    sig_get_random_bytes(bytes, total_bytes);
    for (i = 0; i < total_coeffs; ++i)
    {
        polys[i / (size_t)RAW_POLY_LEN].coeffs[i % (size_t)RAW_POLY_LEN] =
            (uint16_t)(bytes[i * 2u] | ((uint16_t)bytes[i * 2u + 1u] << 8));
    }
    free(bytes);
}

/* Expand the public seed into the binary parity-check matrix H. */
void random_H(unsigned long H[embded_num][M], const unsigned char seed[SEED_BYTES])
{
    unsigned long long bit_count = (unsigned long long)embded_num * (unsigned long long)M;
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_H, seed, SEED_BYTES, bit_count);
    size_t bit_index = 0;
    int row;

    for (row = 0; row < embded_num; ++row)
    {
        unsigned long *row_ptr = H[row];
        int col;

        for (col = 0; col < M; ++col)
        {
            row_ptr[col] = (unsigned long)((stream[bit_index / 8u] >> (7u - (bit_index % 8u))) & 1u);
            ++bit_index;
        }
    }
    free(stream);
}

/* Expand the secret seed into the sparse error vector e. */
void random_e(unsigned long e[M], const unsigned char seed[SEED_BYTES])
{
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_E, seed, SEED_BYTES, (unsigned long long)(M / 6) * 16ULL);
    int i;

    memset(e, 0, sizeof(unsigned long) * M);
    for (i = 0; i < M / 6; ++i)
    {
        uint16_t rnd = load_u16_be(stream + ((size_t)i * 2u));
        e[i * 6 + (rnd % 6u)] = 1;
    }
    free(stream);
}

/* Pack e into the RawGF2EX representation used by the protocol. */
void embed_e_raw(RawGF2EX e_hat_prime_raw[N_PRIME_LEN], const unsigned long e[M])
{
    int col;

    for (col = 0; col < COLS; ++col)
    {
        int offset;
        memset(e_hat_prime_raw[col].coeffs, 0, sizeof(e_hat_prime_raw[col].coeffs));
        for (offset = 0; offset < 6; ++offset)
        {
            if (e[col * 6 + offset] != 0u)
            {
                e_hat_prime_raw[col].coeffs[offset] = 1;
            }
        }
    }
    for (col = COLS; col < N_PRIME_LEN; ++col)
    {
        memset(e_hat_prime_raw[col].coeffs, 0, sizeof(e_hat_prime_raw[col].coeffs));
    }
}

/* Pack H into RawGF2EX blocks for prover and verifier arithmetic. */
void embed_H_raw(RawGF2EX H_hat_raw[ROWS][COLS], const unsigned long H[embded_num][M])
{
    int row;

    for (row = 0; row < ROWS; ++row)
    {
        int row_offset = row * 16;
        int col;

        for (col = 0; col < COLS; ++col)
        {
            RawGF2EX entry;
            int col_base = col * 6;
            int offset;

            memset(entry.coeffs, 0, sizeof(entry.coeffs));
            for (offset = 0; offset < 6; ++offset)
            {
                const unsigned long *col_ptr = &H[row_offset][col_base + offset];
                uint16_t coeff = 0;
                int bit;

                for (bit = 0; bit < 16; ++bit)
                {
                    coeff |= (uint16_t)((col_ptr[bit * M] & 1u) << bit);
                }
                entry.coeffs[5 - offset] = coeff;
            }
            H_hat_raw[row][col] = entry;
        }
    }
}

/* Pack y into 16-bit scalars for the interactive consistency checks. */
void embed_y_raw(uint16_t y_hat_scalars[ROWS], const unsigned long y[embded_num])
{
    int row;

    for (row = 0; row < ROWS; ++row)
    {
        uint16_t packed = 0;
        int bit;

        for (bit = 0; bit < 16; ++bit)
        {
            if (y[row * 16 + bit] != 0u)
            {
                packed |= (uint16_t)(1u << bit);
            }
        }
        y_hat_scalars[row] = packed;
    }
}

/* Sample tau polynomials from the template DRNG. */
void random_tau_raw(RawGF2EX tau_raw[], int count)
{
    int i;
    for (i = 0; i < count; ++i)
    {
        fill_random_raw_poly(&tau_raw[i]);
    }
}

/* Sample the scalar zeta from the template DRNG. */
uint16_t random_zeta_raw(void)
{
    return sig_random_u16();
}

static unsigned round_up_multiple(unsigned value, unsigned multiple)
{
    unsigned rem;

    if (multiple == 0u)
    {
        return value;
    }
    rem = value % multiple;
    return rem ? (value + multiple - rem) : value;
}

static void rs_ensure_ready(void)
{
    if (!rs_tables_ready)
    {
        if (rs_init() != Rsencode_Success)
        {
            sig_abort("ERROR: rs_init failed");
        }
        rs_tables_ready = 1;
    }
}

static void rs_workspace_ensure(
    RSWorkspace *workspace,
    unsigned total_elements,
    unsigned elements_per_block,
    unsigned k,
    unsigned recovery_block_count,
    unsigned work_count)
{
    if (workspace->original_buffer == NULL)
    {
        workspace->total_elements = total_elements;
        workspace->elements_per_block = elements_per_block;
        workspace->k = k;
        workspace->recovery_block_count = recovery_block_count;
        workspace->work_count = work_count;
        workspace->original_buffer = (uint16_t *)sig_malloc_zero((size_t)k * elements_per_block * sizeof(uint16_t));
        workspace->original_ptrs = (const void **)sig_malloc_zero((size_t)k * sizeof(void *));
        workspace->work_storage = (uint16_t *)sig_malloc_zero((size_t)work_count * elements_per_block * sizeof(uint16_t));
        workspace->work_ptrs = (void **)sig_malloc_zero((size_t)work_count * sizeof(void *));
    }
    else if (workspace->total_elements != total_elements ||
             workspace->elements_per_block != elements_per_block ||
             workspace->k != k ||
             workspace->recovery_block_count != recovery_block_count ||
             workspace->work_count != work_count)
    {
        sig_abort("ERROR: RS workspace does not match current parameters");
    }
}

static void rs_encode_with_workspace(RSWorkspace *workspace, RawGF2EX e_hat_encoded[N], const RawGF2EX e_hat_prime[N_PRIME_LEN])
{
    const unsigned logical_input_size = N_PRIME_LEN;
    const unsigned padded_n_logical = round_up_multiple(logical_input_size, 32u);
    const unsigned chunk_count = 32u;
    const unsigned symbols_per_chunk = padded_n_logical / chunk_count;
    const uint64_t buffer_bytes = RAW_POLY_LEN * 64u;
    const unsigned elements_per_block = (unsigned)(buffer_bytes / sizeof(uint16_t));
    const unsigned k = symbols_per_chunk;
    const unsigned recovery_block_count = 8u * k;
    const unsigned work_count = rs_encode_work_count(k, recovery_block_count);
    unsigned chunk;
    unsigned block;
    RsencodeResult result;

    rs_workspace_ensure(workspace, padded_n_logical * RAW_POLY_LEN, elements_per_block, k, recovery_block_count, work_count);

    for (chunk = 0; chunk < chunk_count; ++chunk)
    {
        unsigned lane_base = chunk * RAW_POLY_LEN;
        for (block = 0; block < symbols_per_chunk; ++block)
        {
            unsigned logical_index = chunk * symbols_per_chunk + block;
            if (logical_index < logical_input_size)
            {
                int coeff;
                for (coeff = 0; coeff < RAW_POLY_LEN; ++coeff)
                {
                    workspace->original_buffer[(size_t)block * elements_per_block + lane_base + (unsigned)coeff] =
                        e_hat_prime[logical_index].coeffs[coeff];
                }
            }
            else
            {
                int coeff;
                for (coeff = 0; coeff < RAW_POLY_LEN; ++coeff)
                {
                    workspace->original_buffer[(size_t)block * elements_per_block + lane_base + (unsigned)coeff] = 0u;
                }
            }
        }
    }
    for (block = 0; block < k; ++block)
    {
        workspace->original_ptrs[block] = (const void *)(workspace->original_buffer + (size_t)block * elements_per_block);
    }
    for (block = 0; block < work_count; ++block)
    {
        workspace->work_ptrs[block] = (void *)(workspace->work_storage + (size_t)block * elements_per_block);
    }

    result = rs_encode(buffer_bytes, k, recovery_block_count, work_count, workspace->original_ptrs, workspace->work_ptrs);
    if (result != Rsencode_Success)
    {
        sig_abort("ERROR: rs_encode failed");
    }

    {
        const unsigned logical_output_size = N;
        for (chunk = 0; chunk < chunk_count; ++chunk)
        {
            unsigned lane_base = chunk * RAW_POLY_LEN;
            unsigned recovery;

            for (recovery = 0; recovery < recovery_block_count; ++recovery)
            {
                unsigned logical_index = chunk * recovery_block_count + recovery;
                if (logical_index < logical_output_size)
                {
                    int coeff;
                    for (coeff = 0; coeff < RAW_POLY_LEN; ++coeff)
                    {
                        e_hat_encoded[logical_index].coeffs[coeff] =
                            workspace->work_storage[(size_t)recovery * elements_per_block + lane_base + (unsigned)coeff];
                    }
                }
            }
        }
    }
}

static void *rs_encode_thread(void *arg)
{
    RSEncodeTask *task = (RSEncodeTask *)arg;
#ifdef SIG_PROFILE_STEPS
    uint64_t t0 = sig_profile_now_ns();
#endif

    rs_encode_with_workspace(task->workspace, task->encoded, task->prime);
#ifdef SIG_PROFILE_STEPS
    task->profile_encode_ns = sig_profile_now_ns() - t0;
#endif
    return NULL;
}

/* Reed-Solomon encode the N' input blocks into the N transmitted blocks. */
void RS_encode(RawGF2EX e_hat_encoded[N], const RawGF2EX e_hat_prime[N_PRIME_LEN])
{
    rs_ensure_ready();
    rs_encode_with_workspace(&g_rs_workspaces[0], e_hat_encoded, e_hat_prime);
}

static void raw_poly_to_bytes(unsigned char result[RAW_POLY_BYTES], const RawGF2EX *poly)
{
    memcpy(result, poly->coeffs, RAW_POLY_BYTES);
}

static void raw_poly_pair_to_bytes(unsigned char result[RAW_POLY_BYTES * 2], const RawGF2EX *left, const RawGF2EX *right)
{
    raw_poly_to_bytes(result, left);
    raw_poly_to_bytes(result + RAW_POLY_BYTES, right);
}

static const unsigned char *merkle_node_hash_ptr(const unsigned char (*hash_nodes)[HASH_DIGEST_LENGTH], int node_index)
{
    return g_merkle_live_nodes[node_index] ? hash_nodes[node_index] : g_zero_merkle_nodes[node_index];
}

static void rs_evaluate_output_position(RawGF2EX *out, const RawGF2EX e_hat_prime[N_PRIME_LEN], unsigned logical_index)
{
    const unsigned logical_input_size = N_PRIME_LEN;
    const unsigned padded_n_logical = round_up_multiple(logical_input_size, 32u);
    const unsigned chunk_count = 32u;
    const unsigned symbols_per_chunk = padded_n_logical / chunk_count;
    const unsigned recovery_block_count = 8u * symbols_per_chunk;
    const unsigned chunk = logical_index / recovery_block_count;
    const unsigned recovery = logical_index % recovery_block_count;
    const unsigned chunk_base = chunk * symbols_per_chunk;
    const uint16_t eval_log = (uint16_t)(recovery + 1u);
    int coeff;

    if (!gf_tables_ready)
    {
        gf_init_tables();
    }

    for (coeff = 0; coeff < RAW_POLY_LEN; ++coeff)
    {
        uint16_t acc = 0u;
        int block;

        for (block = (int)symbols_per_chunk - 1; block >= 0; --block)
        {
            unsigned input_index = chunk_base + (unsigned)block;
            uint16_t value = 0u;

            if (input_index < logical_input_size)
            {
                value = e_hat_prime[input_index].coeffs[coeff];
            }
            acc = mul_gf2_16_by_log(acc, eval_log);
            acc ^= value;
        }
        out->coeffs[coeff] = acc;
    }
}

/* Keep the historical helper name while routing hashing through pseudohash(512). */
void compute_sha256_vec(unsigned char *hash_out, unsigned char *data, unsigned long data_len)
{
    sig_hash_bytes(data, (size_t)data_len, hash_out);
}

static void ensure_zero_merkle_cache(void)
{
    unsigned char combined_node[HASH_DIGEST_LENGTH * 2];
    unsigned char zero_leaf_hash[HASH_DIGEST_LENGTH];
    unsigned char zero_leaf_bytes[RAW_POLY_BYTES * 2];
    uint8_t impacted[MERKLE_NODE_COUNT];
    int j;

    if (g_zero_merkle_ready)
    {
        return;
    }

    memset(zero_leaf_bytes, 0, sizeof(zero_leaf_bytes));
    compute_sha256_vec(zero_leaf_hash, zero_leaf_bytes, (unsigned long)sizeof(zero_leaf_bytes));
    for (j = 0; j < MERKLE_LEAF_COUNT; ++j)
    {
        memcpy(g_zero_merkle_nodes[MERKLE_LEAF_OFFSET + j], zero_leaf_hash, HASH_DIGEST_LENGTH);
    }
    for (j = MERKLE_LEAF_OFFSET - 1; j >= 0; --j)
    {
        memcpy(combined_node, g_zero_merkle_nodes[j * 2 + 2], HASH_DIGEST_LENGTH);
        memcpy(combined_node + HASH_DIGEST_LENGTH, g_zero_merkle_nodes[j * 2 + 3], HASH_DIGEST_LENGTH);
        compute_sha256_vec(g_zero_merkle_nodes[j], combined_node, (unsigned long)sizeof(combined_node));
    }

    memset(impacted, 0, sizeof(impacted));
    memset(g_merkle_live_nodes, 0, sizeof(g_merkle_live_nodes));
    for (j = 0; j < group_size; ++j)
    {
        impacted[MERKLE_LEAF_OFFSET + j] = 1;
        g_merkle_live_nodes[MERKLE_LEAF_OFFSET + j] = 1;
    }
    g_merkle_recompute_count = 0;
    for (j = MERKLE_LEAF_OFFSET - 1; j >= 0; --j)
    {
        int left = j * 2 + 2;
        int right = left + 1;
        impacted[j] = impacted[left] || impacted[right];
        if (impacted[j])
        {
            g_merkle_live_nodes[j] = 1;
            g_merkle_recompute_nodes[g_merkle_recompute_count++] = j;
        }
    }

    g_zero_merkle_ready = 1;
}

static void merkle_build_group(MerkleBuildTask *task)
{
    unsigned char combined[RAW_POLY_BYTES * 2];
    unsigned char combined_node[HASH_DIGEST_LENGTH * 2];
#ifdef SIG_PROFILE_STEPS
    uint64_t t0 = sig_profile_now_ns();
#endif
    int j;

    for (j = 0; j < group_size; ++j)
    {
        int node_index = MERKLE_LEAF_OFFSET + j;
        raw_poly_pair_to_bytes(combined, &task->e_hat_group[j], &task->v_group[j]);
        compute_sha256_vec(task->hash_nodes[node_index], combined, (unsigned long)sizeof(combined));
    }

#ifdef SIG_PROFILE_STEPS
    task->profile_leaf_hash_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    for (j = 0; j < g_merkle_recompute_count; ++j)
    {
        int node = g_merkle_recompute_nodes[j];
        const unsigned char *left_hash = merkle_node_hash_ptr(task->hash_nodes, node * 2 + 2);
        const unsigned char *right_hash = merkle_node_hash_ptr(task->hash_nodes, node * 2 + 3);

        memcpy(combined_node, left_hash, HASH_DIGEST_LENGTH);
        memcpy(combined_node + HASH_DIGEST_LENGTH, right_hash, HASH_DIGEST_LENGTH);
        compute_sha256_vec(task->hash_nodes[node], combined_node, (unsigned long)sizeof(combined_node));
    }

    memcpy(combined_node, merkle_node_hash_ptr(task->hash_nodes, 0), HASH_DIGEST_LENGTH);
    memcpy(combined_node + HASH_DIGEST_LENGTH, merkle_node_hash_ptr(task->hash_nodes, 1), HASH_DIGEST_LENGTH);
    compute_sha256_vec(task->root_hash, combined_node, (unsigned long)sizeof(combined_node));
#ifdef SIG_PROFILE_STEPS
    task->profile_tree_hash_ns += sig_profile_now_ns() - t0;
#endif
}

static void *merkle_build_group_thread(void *arg)
{
    merkle_build_group((MerkleBuildTask *)arg);
    return NULL;
}

static void merkle_launch(MerkleBuildState *state, unsigned char root_Hash[total_groups][HASH_DIGEST_LENGTH], unsigned char Hash[total_groups][MERKLE_NODE_COUNT][HASH_DIGEST_LENGTH], const RawGF2EX e_hat_encoded[], const RawGF2EX v_encoded[])
{
    int i;

    ensure_zero_merkle_cache();
    memset(state->launched, 0, sizeof(state->launched));
#ifdef SIG_PROFILE_STEPS
    state->profile_thread_launch_ns = 0;
    state->profile_join_ns = 0;
#endif

    for (i = 0; i < total_groups; ++i)
    {
        int create_result;
#ifdef SIG_PROFILE_STEPS
        uint64_t t0;
#endif
        state->tasks[i].hash_nodes = Hash[i];
        state->tasks[i].root_hash = root_Hash[i];
        state->tasks[i].e_hat_group = e_hat_encoded + (size_t)i * group_size;
        state->tasks[i].v_group = v_encoded + (size_t)i * group_size;
#ifdef SIG_PROFILE_STEPS
        state->tasks[i].profile_leaf_hash_ns = 0;
        state->tasks[i].profile_tree_hash_ns = 0;
        t0 = sig_profile_now_ns();
#endif

        create_result = pthread_create(&state->threads[i], NULL, merkle_build_group_thread, &state->tasks[i]);
#ifdef SIG_PROFILE_STEPS
        state->profile_thread_launch_ns += sig_profile_now_ns() - t0;
#endif
        if (create_result == 0)
        {
            state->launched[i] = 1;
        }
        else
        {
            merkle_build_group(&state->tasks[i]);
        }
    }
}

static void merkle_join(MerkleBuildState *state)
{
#ifdef SIG_PROFILE_STEPS
    uint64_t leaf_hash_ns = 0;
    uint64_t tree_hash_ns = 0;
#endif
    int i;

    for (i = 0; i < total_groups; ++i)
    {
        if (state->launched[i])
        {
#ifdef SIG_PROFILE_STEPS
            uint64_t t0 = sig_profile_now_ns();
#endif
            pthread_join(state->threads[i], NULL);
#ifdef SIG_PROFILE_STEPS
            state->profile_join_ns += sig_profile_now_ns() - t0;
#endif
        }
#ifdef SIG_PROFILE_STEPS
        leaf_hash_ns += state->tasks[i].profile_leaf_hash_ns;
        tree_hash_ns += state->tasks[i].profile_tree_hash_ns;
#endif
    }
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_commitment_merkle_leaf_hash_ns += leaf_hash_ns;
    g_sig_step_profile.sign_commitment_merkle_tree_hash_ns += tree_hash_ns;
    g_sig_step_profile.sign_commitment_merkle_thread_launch_ns += state->profile_thread_launch_ns;
    g_sig_step_profile.sign_commitment_merkle_join_ns += state->profile_join_ns;
#endif
}

/* Build one Merkle tree per group from the encoded commitment leaves. */
void merkle(unsigned char root_Hash[total_groups][HASH_DIGEST_LENGTH], unsigned char Hash[total_groups][MERKLE_NODE_COUNT][HASH_DIGEST_LENGTH], const RawGF2EX e_hat_encoded[], const RawGF2EX v_encoded[])
{
    MerkleBuildState state;

    merkle_launch(&state, root_Hash, Hash, e_hat_encoded, v_encoded);
    merkle_join(&state);
}

static int collect_required_merkle_nodes(const uint8_t opened[MERKLE_LEAF_COUNT], uint8_t required[MERKLE_NODE_COUNT])
{
    uint8_t known[MERKLE_NODE_COUNT];
    int parent;
    int count = 0;
    int i;

    memset(required, 0, sizeof(uint8_t) * MERKLE_NODE_COUNT);
    memset(known, 0, sizeof(known));

    for (i = 0; i < MERKLE_LEAF_COUNT; ++i)
    {
        known[MERKLE_LEAF_OFFSET + i] = opened[i];
    }
    for (parent = MERKLE_LEAF_OFFSET - 1; parent >= 0; --parent)
    {
        int left = parent * 2 + 2;
        int right = left + 1;
        int left_known = known[left];
        int right_known = known[right];

        if (left_known && !right_known)
        {
            required[right] = 1;
            known[right] = 1;
        }
        else if (!left_known && right_known)
        {
            required[left] = 1;
            known[left] = 1;
        }
        if (known[left] && known[right])
        {
            known[parent] = 1;
        }
    }
    if (known[0] && !known[1])
    {
        required[1] = 1;
        known[1] = 1;
    }
    else if (!known[0] && known[1])
    {
        required[0] = 1;
        known[0] = 1;
    }
    for (i = 0; i < MERKLE_NODE_COUNT; ++i)
    {
        if (required[i])
        {
            ++count;
        }
    }
    return count;
}

/* Sample commitment vectors, RS-encode them, and build the Merkle roots. */
void Commitment_Raw(unsigned char Hash[total_groups][MERKLE_NODE_COUNT][HASH_DIGEST_LENGTH], RawGF2EX e_hat_prime_raw[N_PRIME_LEN], RawGF2EX v_hat_prime_raw[N_PRIME_LEN], RawGF2EX v_encoded[N], RawGF2EX e_hat_encoded[N], unsigned char root_Hash[total_groups][HASH_DIGEST_LENGTH], MerkleBuildState *merkle_state)
{
    RawGF2EX augmentation_raw[TAIL_LEN];
    pthread_t rs_thread;
    RSEncodeTask rs_task;
    int rs_thread_launched = 0;
    int create_result;
#ifdef SIG_PROFILE_STEPS
    uint64_t t0 = sig_profile_now_ns();
#endif
    int i;

    fill_random_raw_poly_array(augmentation_raw, TAIL_LEN);
    for (i = 0; i < SIGMA_REPETITIONS; ++i)
    {
        augmentation_raw[i].coeffs[5] = 0;
    }

    for (i = 0; i < TAIL_LEN; ++i)
    {
        e_hat_prime_raw[COLS + i] = augmentation_raw[i];
    }
    fill_random_raw_poly_array(v_hat_prime_raw, N_PRIME_LEN);

#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_commitment_random_fill_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    rs_ensure_ready();
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_commitment_rs_prepare_ns += sig_profile_now_ns() - t0;
#endif
    rs_task.workspace = &g_rs_workspaces[1];
    rs_task.encoded = v_encoded;
    rs_task.prime = v_hat_prime_raw;
#ifdef SIG_PROFILE_STEPS
    rs_task.profile_encode_ns = 0;
    t0 = sig_profile_now_ns();
#endif
    create_result = pthread_create(&rs_thread, NULL, rs_encode_thread, &rs_task);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_commitment_rs_v_launch_ns += sig_profile_now_ns() - t0;
#endif
    if (create_result == 0)
    {
        rs_thread_launched = 1;
    }
    else
    {
#ifdef SIG_PROFILE_STEPS
        t0 = sig_profile_now_ns();
#endif
        rs_encode_with_workspace(rs_task.workspace, rs_task.encoded, rs_task.prime);
#ifdef SIG_PROFILE_STEPS
        g_sig_step_profile.sign_commitment_rs_v_encode_ns += sig_profile_now_ns() - t0;
#endif
    }

#ifdef SIG_PROFILE_STEPS
    t0 = sig_profile_now_ns();
#endif
    rs_encode_with_workspace(&g_rs_workspaces[0], e_hat_encoded, e_hat_prime_raw);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_commitment_rs_e_encode_ns += sig_profile_now_ns() - t0;
#endif
    if (rs_thread_launched)
    {
#ifdef SIG_PROFILE_STEPS
        t0 = sig_profile_now_ns();
#endif
        pthread_join(rs_thread, NULL);
#ifdef SIG_PROFILE_STEPS
        g_sig_step_profile.sign_commitment_rs_v_join_ns += sig_profile_now_ns() - t0;
        g_sig_step_profile.sign_commitment_rs_v_encode_ns += rs_task.profile_encode_ns;
#endif
    }

    if (merkle_state != NULL)
    {
#ifdef SIG_PROFILE_STEPS
        t0 = sig_profile_now_ns();
#endif
        merkle_launch(merkle_state, root_Hash, Hash, e_hat_encoded, v_encoded);
#ifdef SIG_PROFILE_STEPS
        g_sig_step_profile.sign_commitment_merkle_launch_ns += sig_profile_now_ns() - t0;
#endif
    }
    else
    {
        merkle(root_Hash, Hash, e_hat_encoded, v_encoded);
    }
}

/* Evaluate the committed e/v polynomials at the Fiat-Shamir point z. */
static void Commitment_Evals_Raw(const RawGF2EX e_hat_prime_raw[N_PRIME_LEN], const RawGF2EX v_hat_prime_raw[N_PRIME_LEN], const RawGF2EX *z, RawGF2EX *e_eval, RawGF2EX *v_eval)
{
    RawNZTerms z_terms;
    RawNZTerms z_map[RAW_POLY_LEN];
    RawGF2EX temp_e;
    RawGF2EX temp_v;
    int i;

    raw_collect_nonzero_terms(z, &z_terms);
    build_mul_reduce_map(z_map, &z_terms);
    memset(e_eval->coeffs, 0, sizeof(e_eval->coeffs));
    memset(v_eval->coeffs, 0, sizeof(v_eval->coeffs));

    for (i = N_PRIME_LEN - 1; i >= 0; --i)
    {
        mul_reduce_raw_by_map(&temp_e, e_eval, z_map);
        add_raw(e_eval, &temp_e, &e_hat_prime_raw[i]);

        mul_reduce_raw_by_map(&temp_v, v_eval, z_map);
        add_raw(v_eval, &temp_v, &v_hat_prime_raw[i]);
    }
}

/* Collect the Merkle authentication data opened by the signer. */
size_t Commitment_Open_Prover_Raw(unsigned char result[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH], RawGF2EX e_hat_encoded_output[opennum], const int trees[opennum], const int choices[opennum], const unsigned char Hash[total_groups][MERKLE_NODE_COUNT][HASH_DIGEST_LENGTH], const RawGF2EX e_hat_encoded[N])
{
    int16_t opening_index[total_groups * group_size];
    size_t hash_out = 0;
    int i;

    memset(opening_index, -1, sizeof(opening_index));

    for (i = 0; i < opennum; ++i)
    {
        int leaf_index = choices[i] - MERKLE_LEAF_OFFSET;
        assign_raw(&e_hat_encoded_output[i], &e_hat_encoded[trees[i] * group_size + leaf_index]);
        opening_index[trees[i] * group_size + leaf_index] = i;
    }

    for (i = 0; i < total_groups; ++i)
    {
        uint8_t opened[MERKLE_LEAF_COUNT];
        int has_opening = 0;
        int leaf;

        memset(opened, 0, sizeof(opened));
        for (leaf = 0; leaf < group_size; ++leaf)
        {
            if (opening_index[i * group_size + leaf] >= 0)
            {
                opened[leaf] = 1;
                has_opening = 1;
            }
        }
        if (has_opening)
        {
            uint8_t required[MERKLE_NODE_COUNT];
            int idx;

            collect_required_merkle_nodes(opened, required);
            for (idx = 0; idx < MERKLE_NODE_COUNT; ++idx)
            {
                if (required[idx])
                {
                    const unsigned char *node_hash = g_merkle_live_nodes[idx] ? Hash[i][idx] : g_zero_merkle_nodes[idx];

                    if (hash_out >= MERKLE_MAX_PROOF_HASHES)
                    {
                        sig_abort("ERROR: commitment open proof exceeds buffer size");
                    }
                    memcpy(result[hash_out], node_hash, HASH_DIGEST_LENGTH);
                    ++hash_out;
                }
            }
        }
    }
    return hash_out;
}

/* Verify opened Merkle paths and the masked codeword relation. */
int Commitment_Open_Verifier_Raw(RawGF2EX e_hat_encoded_iutput[opennum], const int trees[opennum], const unsigned char hash[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH], size_t hash_count, const int choices[opennum], const unsigned char root_Hash[total_groups][HASH_DIGEST_LENGTH], const RawGF2EX w_hat_prime_raw[N_PRIME_LEN], const RawGF2EX *z, const RawGF2EX *e_eval, const RawGF2EX *v_eval, const RawGF2EX *delta_raw)
{
    RawGF2EX w_eval;
    RawGF2EX_Product prod;
    RawGF2EX temp;
    RawGF2EX_Product prod_rhs;
    RawGF2EX delta_times_e;
    RawGF2EX rhs;
    RawGF2EX w_encoded_open[opennum];
    int16_t opening_index[total_groups * group_size];
    size_t hash_offset = 0;
    int i;

    memset(&w_eval, 0, sizeof(w_eval));

    for (i = N_PRIME_LEN - 1; i >= 0; --i)
    {
        mul_raw(&prod, &w_eval, z);
        rem_raw(&temp, &prod);
        add_raw(&w_eval, &temp, &w_hat_prime_raw[i]);
    }

    mul_raw(&prod_rhs, delta_raw, e_eval);
    rem_raw(&delta_times_e, &prod_rhs);
    add_raw(&rhs, &delta_times_e, v_eval);

    for (i = 0; i < opennum; ++i)
    {
        int idx = trees[i] * group_size + choices[i] - MERKLE_LEAF_OFFSET;
        rs_evaluate_output_position(&w_encoded_open[i], w_hat_prime_raw, (unsigned)idx);
    }

    memset(opening_index, -1, sizeof(opening_index));
    for (i = 0; i < opennum; ++i)
    {
        int leaf_index = choices[i] - MERKLE_LEAF_OFFSET;
        opening_index[trees[i] * group_size + leaf_index] = i;
    }

    for (i = 0; i < total_groups; ++i)
    {
        uint8_t opened[MERKLE_LEAF_COUNT];
        int has_opening = 0;
        uint8_t required[MERKLE_NODE_COUNT];
        unsigned char node_hash[MERKLE_NODE_COUNT][HASH_DIGEST_LENGTH];
        uint8_t node_known[MERKLE_NODE_COUNT];
        int leaf;

        memset(opened, 0, sizeof(opened));
        memset(node_known, 0, sizeof(node_known));

        for (leaf = 0; leaf < group_size; ++leaf)
        {
            if (opening_index[i * group_size + leaf] >= 0)
            {
                opened[leaf] = 1;
                has_opening = 1;
            }
        }
        if (!has_opening)
        {
            continue;
        }

        collect_required_merkle_nodes(opened, required);

        for (leaf = 0; leaf < group_size; ++leaf)
        {
            int open_idx = opening_index[i * group_size + leaf];
            if (open_idx >= 0)
            {
                RawGF2EX recovered_v;
                unsigned char combined[RAW_POLY_BYTES * 2];

                mul_raw(&prod, &e_hat_encoded_iutput[open_idx], delta_raw);
                rem_raw(&temp, &prod);
                add_raw(&recovered_v, &w_encoded_open[open_idx], &temp);
                raw_poly_pair_to_bytes(combined, &e_hat_encoded_iutput[open_idx], &recovered_v);
                compute_sha256_vec(node_hash[MERKLE_LEAF_OFFSET + leaf], combined, (unsigned long)sizeof(combined));
                node_known[MERKLE_LEAF_OFFSET + leaf] = 1;
            }
        }

        {
            int idx;
            for (idx = 0; idx < MERKLE_NODE_COUNT; ++idx)
            {
                if (required[idx])
                {
                    if (hash_offset >= hash_count || node_known[idx])
                    {
                        return 0;
                    }
                    memcpy(node_hash[idx], hash[hash_offset], HASH_DIGEST_LENGTH);
                    node_known[idx] = 1;
                    ++hash_offset;
                }
            }
        }

        {
            unsigned char hash_temp[HASH_DIGEST_LENGTH];
            unsigned char combined_node[HASH_DIGEST_LENGTH * 2];
            int parent;

            for (parent = MERKLE_LEAF_OFFSET - 1; parent >= 0; --parent)
            {
                int left = parent * 2 + 2;
                int right = left + 1;
                if (node_known[left] && node_known[right])
                {
                    memcpy(combined_node, node_hash[left], HASH_DIGEST_LENGTH);
                    memcpy(combined_node + HASH_DIGEST_LENGTH, node_hash[right], HASH_DIGEST_LENGTH);
                    compute_sha256_vec(hash_temp, combined_node, (unsigned long)sizeof(combined_node));
                    if (node_known[parent])
                    {
                        if (memcmp(hash_temp, node_hash[parent], HASH_DIGEST_LENGTH) != 0)
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        memcpy(node_hash[parent], hash_temp, HASH_DIGEST_LENGTH);
                        node_known[parent] = 1;
                    }
                }
            }

            if (!node_known[0] || !node_known[1])
            {
                return 0;
            }
            memcpy(combined_node, node_hash[0], HASH_DIGEST_LENGTH);
            memcpy(combined_node + HASH_DIGEST_LENGTH, node_hash[1], HASH_DIGEST_LENGTH);
            compute_sha256_vec(hash_temp, combined_node, (unsigned long)sizeof(combined_node));
            if (memcmp(hash_temp, root_Hash[i], HASH_DIGEST_LENGTH) != 0)
            {
                return 0;
            }
        }
    }
    if (hash_offset != hash_count)
    {
        return 0;
    }

    return raw_equals(&w_eval, &rhs);
}

/* Build the fixed polynomial family used in the final response relation. */
void PolyDef_Raw(RawGF2EX coeffs[7])
{
    RawGF2EX x_var;
    RawGF2EX roots[6];
    int i;

    memset(&x_var, 0, sizeof(x_var));
    memset(roots, 0, sizeof(roots));
    x_var.coeffs[1] = 1;
    roots[0].coeffs[0] = 1;

    for (i = 1; i < 6; ++i)
    {
        RawGF2EX_Product prod;
        mul_raw(&prod, &roots[i - 1], &x_var);
        rem_raw(&roots[i], &prod);
    }

    memset(coeffs, 0, sizeof(RawGF2EX) * 7);
    coeffs[0].coeffs[0] = 1;

    for (i = 0; i < 6; ++i)
    {
        int k;
        for (k = i; k >= 0; --k)
        {
            RawGF2EX_Product prod;
            RawGF2EX temp;
            RawGF2EX sum;

            add_raw(&sum, &coeffs[k + 1], &coeffs[k]);
            coeffs[k + 1] = sum;
            mul_raw(&prod, &roots[i], &coeffs[k]);
            rem_raw(&temp, &prod);
            coeffs[k] = temp;
        }
    }
}

static void ensure_polydef_cache(void)
{
    if (!g_polydef_ready)
    {
        PolyDef_Raw(g_polydef_coeffs);
        g_polydef_ready = 1;
    }
}

static void horner_eval_Raw(RawGF2EX *result, const RawGF2EX *x_val, const RawGF2EX effective_coeffs[], int degree)
{
    int k;

    *result = effective_coeffs[degree];
    for (k = degree - 1; k >= 0; --k)
    {
        RawGF2EX_Product prod;
        RawGF2EX sum;

        mul_raw(&prod, result, x_val);
        rem_raw(result, &prod);
        add_raw(&sum, result, &effective_coeffs[k]);
        *result = sum;
    }
}

/* Compute the verifier-side polynomial challenge value. */
void Polynomial_Challenge_Raw(RawGF2EX *vc, long n, const RawGF2EX tau[], uint16_t scalar, const RawGF2EX w[], const RawGF2EX wr[], const RawGF2EX f_coeffs[], const RawGF2EX *delta)
{
    RawGF2EX delta_pows[7];
    RawGF2EX effective_coeffs[7];
    RawGF2EX tau_acc;
    RawGF2EX wr_poly;
    RawGF2EX scaled_wr;
    int i;

    memset(delta_pows, 0, sizeof(delta_pows));
    memset(effective_coeffs, 0, sizeof(effective_coeffs));
    memset(&tau_acc, 0, sizeof(tau_acc));
    memset(&wr_poly, 0, sizeof(wr_poly));
    memset(&scaled_wr, 0, sizeof(scaled_wr));

    delta_pows[0].coeffs[0] = 1;
    delta_pows[1] = *delta;
    for (i = 2; i <= 6; ++i)
    {
        RawGF2EX_Product prod;
        mul_raw(&prod, &delta_pows[i - 1], delta);
        rem_raw(&delta_pows[i], &prod);
    }
    for (i = 0; i <= 6; ++i)
    {
        RawGF2EX_Product prod;
        mul_raw(&prod, &f_coeffs[i], &delta_pows[6 - i]);
        rem_raw(&effective_coeffs[i], &prod);
    }
    for (i = 0; i < n; ++i)
    {
        RawGF2EX f_prime_eval;
        RawGF2EX term;
        RawGF2EX sum;
        RawGF2EX_Product prod;

        horner_eval_Raw(&f_prime_eval, &w[i], effective_coeffs, 6);
        mul_raw(&prod, &tau[i], &f_prime_eval);
        rem_raw(&term, &prod);
        add_raw(&sum, &tau_acc, &term);
        tau_acc = sum;
    }
    for (i = 0; i < 5; ++i)
    {
        RawGF2EX_Product prod;
        RawGF2EX term;
        RawGF2EX sum;

        mul_raw(&prod, &wr[i], &delta_pows[4 - i]);
        rem_raw(&term, &prod);
        add_raw(&sum, &wr_poly, &term);
        wr_poly = sum;
    }
    scalar_mul_raw(&scaled_wr, scalar, &wr_poly);
    add_raw(vc, &tau_acc, &scaled_wr);
}

static void get_gi_coeffs_from_formula_Raw(RawGF2EX gi_coeffs[6], const RawGF2EX *e_i, const RawGF2EX *v_i, const RawGF2EX f_coeffs[7])
{
    RawGF2EX e_pows[5];
    RawGF2EX v_pows[7];
    RawGF2EX term1;
    RawGF2EX term2;
    RawGF2EX sum;
    RawGF2EX_Product temp_prod;
    int j;

    memset(e_pows, 0, sizeof(e_pows));
    memset(v_pows, 0, sizeof(v_pows));
    memset(gi_coeffs, 0, sizeof(RawGF2EX) * 6);

    e_pows[1] = *e_i;
    mul_raw(&temp_prod, &e_pows[1], e_i);
    rem_raw(&e_pows[2], &temp_prod);
    mul_raw(&temp_prod, &e_pows[2], &e_pows[2]);
    rem_raw(&e_pows[4], &temp_prod);

    v_pows[1] = *v_i;
    for (j = 2; j <= 6; ++j)
    {
        mul_raw(&temp_prod, &v_pows[j - 1], v_i);
        rem_raw(&v_pows[j], &temp_prod);
    }

    mul_raw(&temp_prod, &f_coeffs[3], &e_pows[2]);
    rem_raw(&term1, &temp_prod);
    mul_raw(&temp_prod, &f_coeffs[5], &e_pows[4]);
    rem_raw(&term2, &temp_prod);
    add_raw(&sum, &f_coeffs[1], &term1);
    add_raw(&sum, &sum, &term2);
    mul_raw(&temp_prod, &sum, &v_pows[1]);
    rem_raw(&gi_coeffs[5], &temp_prod);

    mul_raw(&temp_prod, &f_coeffs[3], &e_pows[1]);
    rem_raw(&term1, &temp_prod);
    mul_raw(&temp_prod, &f_coeffs[6], &e_pows[4]);
    rem_raw(&term2, &temp_prod);
    add_raw(&sum, &f_coeffs[2], &term1);
    add_raw(&sum, &sum, &term2);
    mul_raw(&temp_prod, &sum, &v_pows[2]);
    rem_raw(&gi_coeffs[4], &temp_prod);

    mul_raw(&temp_prod, &f_coeffs[3], &v_pows[3]);
    rem_raw(&gi_coeffs[3], &temp_prod);

    mul_raw(&temp_prod, &f_coeffs[5], &e_pows[1]);
    rem_raw(&term1, &temp_prod);
    mul_raw(&temp_prod, &f_coeffs[6], &e_pows[2]);
    rem_raw(&term2, &temp_prod);
    add_raw(&sum, &f_coeffs[4], &term1);
    add_raw(&sum, &sum, &term2);
    mul_raw(&temp_prod, &sum, &v_pows[4]);
    rem_raw(&gi_coeffs[2], &temp_prod);

    mul_raw(&temp_prod, &f_coeffs[5], &v_pows[5]);
    rem_raw(&gi_coeffs[1], &temp_prod);
    mul_raw(&temp_prod, &f_coeffs[6], &v_pows[6]);
    rem_raw(&gi_coeffs[0], &temp_prod);
}

/* Compute the prover's final polynomial response coefficients. */
void ProverPolynomialResponse_Final_Raw(RawGF2EX G_coeffs[6], long n, const RawGF2EX e[], const RawGF2EX v[], const RawGF2EX f_coeffs[7], const RawGF2EX tau[], const RawGF2EX r_hat[], const RawGF2EX v_hat[], uint16_t scalar)
{
    RawGF2EX E_coeffs[6];
    RawGF2EX F_coeffs[6];
    int j;
    long i;

    memset(E_coeffs, 0, sizeof(E_coeffs));
    memset(F_coeffs, 0, sizeof(F_coeffs));
    memset(G_coeffs, 0, sizeof(RawGF2EX) * 6);

    for (i = 0; i < n; ++i)
    {
        RawGF2EX gi_coeffs[6];
        get_gi_coeffs_from_formula_Raw(gi_coeffs, &e[i], &v[i], f_coeffs);
        for (j = 0; j < 6; ++j)
        {
            RawGF2EX_Product prod;
            RawGF2EX term;
            RawGF2EX sum;

            mul_raw(&prod, &tau[i], &gi_coeffs[j]);
            rem_raw(&term, &prod);
            add_raw(&sum, &E_coeffs[j], &term);
            E_coeffs[j] = sum;
        }
    }

    F_coeffs[0] = v_hat[4];
    add_raw(&F_coeffs[1], &v_hat[3], &r_hat[4]);
    add_raw(&F_coeffs[2], &v_hat[2], &r_hat[3]);
    add_raw(&F_coeffs[3], &v_hat[1], &r_hat[2]);
    add_raw(&F_coeffs[4], &v_hat[0], &r_hat[1]);
    F_coeffs[5] = r_hat[0];

    for (j = 0; j < 6; ++j)
    {
        RawGF2EX scaled_F_j;
        RawGF2EX sum;
        scalar_mul_raw(&scaled_F_j, scalar, &F_coeffs[j]);
        add_raw(&sum, &E_coeffs[j], &scaled_F_j);
        G_coeffs[j] = sum;
    }
}

/* Evaluate the final response polynomial at the masking point. */
void VerifierCheck_Raw(RawGF2EX *vc, const RawGF2EX G_coeffs[6], const RawGF2EX *evaluation_point)
{
    horner_eval_Raw(vc, evaluation_point, G_coeffs, 5);
}

static void Masking_Setup_Raw(RawGF2EX *delta, const RawGF2EX e_hat_prime[], const RawGF2EX v_hat_prime[], RawGF2EX w_hat_prime[])
{
    int has_delta = 0;
    int i;

    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        if (delta->coeffs[i] != 0u)
        {
            has_delta = 1;
            break;
        }
    }
    if (!has_delta)
    {
        for (i = 0; i < RAW_POLY_LEN; ++i)
        {
            delta->coeffs[i] = sig_random_u16();
        }
    }
    for (i = 0; i < N_PRIME_LEN; ++i)
    {
        RawGF2EX_Product prod;
        RawGF2EX term;

        mul_raw(&prod, &e_hat_prime[i], delta);
        rem_raw(&term, &prod);
        add_raw(&w_hat_prime[i], &term, &v_hat_prime[i]);
    }
}

static void compute_projected_products(RawGF2EX out[ROWS], const RawNZTerms H_hat_terms[ROWS][COLS], const RawGF2EX projected_raw[N_PRIME_LEN])
{
    RawNZTerms projected_terms[COLS];
    int i;

    for (i = 0; i < COLS; ++i)
    {
        raw_collect_nonzero_terms(&projected_raw[i], &projected_terms[i]);
    }

    for (i = 0; i < ROWS; ++i)
    {
        RawGF2EX acc;
        int j;

        memset(&acc, 0, sizeof(acc));
        for (j = 0; j < COLS; ++j)
        {
            if (H_hat_terms[i][j].count != 0u && projected_terms[j].count != 0u)
            {
                mul_reduce_low_terms_into_coeffs(acc.coeffs, &H_hat_terms[i][j], &projected_terms[j]);
            }
        }
        out[i] = acc;
    }
}

/* Precompute Y*E terms shared by the interactive consistency checks. */
void Compute_YE_Raw(RawGF2EX ye_hat[ROWS], const RawNZTerms H_hat_terms[ROWS][COLS], const RawGF2EX e_hat_prime_raw[N_PRIME_LEN])
{
    compute_projected_products(ye_hat, H_hat_terms, e_hat_prime_raw);
}

/* Precompute Y*V terms needed by the prover. */
void Initial_Computation_Prover_Raw(RawGF2EX yv_hat[ROWS], const RawNZTerms H_hat_terms[ROWS][COLS], const RawGF2EX v_hat_prime_raw[N_PRIME_LEN])
{
    compute_projected_products(yv_hat, H_hat_terms, v_hat_prime_raw);
}

static void Initial_Computation_Verify_Raw(RawGF2EX yw_hat[ROWS], const RawNZTerms H_hat_terms[ROWS][COLS], const RawGF2EX w_hat_prime_raw[N_PRIME_LEN])
{
    compute_projected_products(yw_hat, H_hat_terms, w_hat_prime_raw);
}

/* Produce the prover side of the eight interactive consistency checks. */
void Interactive_Check_Raw_Prover(RawGF2EX p_poly_out[SIGMA_REPETITIONS], RawGF2EX rv_out[SIGMA_REPETITIONS], const RawGF2EX ye_hat[ROWS], const RawGF2EX yv_hat[ROWS], const RawGF2EX v_hat_prime[N_PRIME_LEN], const RawGF2EX e_hat_prime[N_PRIME_LEN], const uint16_t y_hat[ROWS], const uint16_t rand_challenges[SIGMA_REPETITIONS][ROWS], const uint16_t gamma_vec[SIGMA_REPETITIONS])
{
    int s;

    for (s = 0; s < SIGMA_REPETITIONS; ++s)
    {
        RawGF2EX p1_acc;
        RawGF2EX rv_acc;
        uint16_t p0 = 0;
        int i;

        memset(&p1_acc, 0, sizeof(p1_acc));
        memset(&rv_acc, 0, sizeof(rv_acc));

        for (i = 0; i < ROWS; ++i)
        {
            uint16_t rand_elem = rand_challenges[s][i];
            if (rand_elem == 0u)
            {
                continue;
            }
            p0 ^= mul_gf2_16(rand_elem, y_hat[i]);
            scalar_mul_add_raw(&p1_acc, rand_elem, &ye_hat[i]);
            scalar_mul_add_raw(&rv_acc, rand_elem, &yv_hat[i]);
        }

        {
            RawGF2EX p0_embedded;
            RawGF2EX gamma_e;
            RawGF2EX gamma_v;
            RawGF2EX p_poly;
            RawGF2EX rv;
            uint16_t gamma = gamma_vec[s];

            memset(&p0_embedded, 0, sizeof(p0_embedded));
            memset(&gamma_e, 0, sizeof(gamma_e));
            memset(&gamma_v, 0, sizeof(gamma_v));
            p0_embedded.coeffs[5] = p0;

            scalar_mul_raw(&gamma_e, gamma, &e_hat_prime[TAIL_INTERACTIVE_OFFSET + s]);
            scalar_mul_raw(&gamma_v, gamma, &v_hat_prime[TAIL_INTERACTIVE_OFFSET + s]);

            p_poly = p1_acc;
            add_raw(&p_poly, &p_poly, &p0_embedded);
            add_raw(&p_poly, &p_poly, &gamma_e);

            rv = rv_acc;
            add_raw(&rv, &rv, &gamma_v);

            p_poly_out[s] = p_poly;
            rv_out[s] = rv;
        }
    }
}

/* Verify the interactive consistency relations derived from Fiat-Shamir. */
int Interactive_Check_Raw_verify(const RawGF2EX p_poly[SIGMA_REPETITIONS], const RawGF2EX rv_out[SIGMA_REPETITIONS], const RawGF2EX yw_hat[ROWS], const RawGF2EX w_hat_prime[N_PRIME_LEN], const RawGF2EX *delta, const uint16_t y_hat[ROWS], const uint16_t rand_challenges[SIGMA_REPETITIONS][ROWS], const uint16_t gamma_vec[SIGMA_REPETITIONS])
{
    int s;

    for (s = 0; s < SIGMA_REPETITIONS; ++s)
    {
        RawGF2EX c0_acc;
        uint16_t p0 = 0;
        int i;

        memset(&c0_acc, 0, sizeof(c0_acc));
        for (i = 0; i < ROWS; ++i)
        {
            uint16_t rand_elem = rand_challenges[s][i];
            if (rand_elem == 0u)
            {
                continue;
            }
            p0 ^= mul_gf2_16(rand_elem, y_hat[i]);
            scalar_mul_add_raw(&c0_acc, rand_elem, &yw_hat[i]);
        }

        {
            RawGF2EX p0_embedded;
            RawGF2EX p1_plus_gamma;
            RawGF2EX_Product prod_delta;
            RawGF2EX delta_term;
            RawGF2EX gamma_w;
            RawGF2EX temp;
            uint16_t gamma = gamma_vec[s];

            memset(&p0_embedded, 0, sizeof(p0_embedded));
            memset(&gamma_w, 0, sizeof(gamma_w));
            p0_embedded.coeffs[5] = p0;

            p1_plus_gamma = p_poly[s];
            add_raw(&p1_plus_gamma, &p1_plus_gamma, &p0_embedded);

            mul_raw(&prod_delta, &p1_plus_gamma, delta);
            rem_raw(&delta_term, &prod_delta);

            scalar_mul_raw(&gamma_w, gamma, &w_hat_prime[TAIL_INTERACTIVE_OFFSET + s]);

            temp = c0_acc;
            add_raw(&temp, &temp, &delta_term);
            add_raw(&temp, &temp, &gamma_w);

            if (!raw_equals(&temp, &rv_out[s]) || p_poly[s].coeffs[5] != 0u)
            {
                return 0;
            }
        }
    }
    return 1;
}

/* Debug helper retained from the original code path. */
void show_hash(const unsigned char hash[HASH_DIGEST_LENGTH])
{
    int i;
    for (i = 0; i < HASH_DIGEST_LENGTH; ++i)
    {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

static void hash_to_rand_commitment(RawGF2EX *z_commit, const unsigned char hash_value[HASH_DIGEST_LENGTH])
{
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_COMMITMENT, hash_value, HASH_DIGEST_LENGTH, (unsigned long long)RAW_POLY_LEN * 16ULL);
    int coeff;
    for (coeff = 0; coeff < RAW_POLY_LEN; ++coeff)
    {
        z_commit->coeffs[coeff] = load_u16_be(stream + ((size_t)coeff * 2u));
    }
    free(stream);
}

static void hash_to_rand_InterCheck(uint16_t rand_challenges[SIGMA_REPETITIONS][ROWS], uint16_t gamma_vec[SIGMA_REPETITIONS], const unsigned char hash_value[HASH_DIGEST_LENGTH])
{
    unsigned long long word_count = (unsigned long long)SIGMA_REPETITIONS * (unsigned long long)(ROWS + 1);
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_INTERCHECK, hash_value, HASH_DIGEST_LENGTH, word_count * 16ULL);
    size_t word_index = 0;
    int i;

    for (i = 0; i < SIGMA_REPETITIONS; ++i)
    {
        int j;
        gamma_vec[i] = load_u16_be(stream + (word_index++ * 2u));
        for (j = 0; j < ROWS; ++j)
        {
            rand_challenges[i][j] = load_u16_be(stream + (word_index++ * 2u));
        }
    }
    free(stream);
}

static void hash_to_rand_PolyRes(RawGF2EX tau_vec_raw[COLS], uint16_t *zeta_raw, const unsigned char hash_value[HASH_DIGEST_LENGTH])
{
    unsigned long long word_count = ((unsigned long long)COLS * (unsigned long long)RAW_POLY_LEN) + 1ULL;
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_POLYRES, hash_value, HASH_DIGEST_LENGTH, word_count * 16ULL);
    size_t word_index = 0;
    int i;

    for (i = 0; i < COLS; ++i)
    {
        int j;
        for (j = 0; j < RAW_POLY_LEN; ++j)
        {
            tau_vec_raw[i].coeffs[j] = load_u16_be(stream + (word_index++ * 2u));
        }
    }
    *zeta_raw = load_u16_be(stream + (word_index * 2u));
    free(stream);
}

static void hash_to_rand_Mask(RawGF2EX *delta_raw, const unsigned char hash_value[HASH_DIGEST_LENGTH])
{
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_MASK, hash_value, HASH_DIGEST_LENGTH, (unsigned long long)RAW_POLY_LEN * 16ULL);
    int i;
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        delta_raw->coeffs[i] = load_u16_be(stream + ((size_t)i * 2u));
    }
    free(stream);
}

static void hash_to_rand_Open(int commitment_open_trees[opennum], int commitment_open_choices[opennum], const unsigned char hash_value[HASH_DIGEST_LENGTH])
{
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_OPEN, hash_value, HASH_DIGEST_LENGTH, (unsigned long long)opennum * 16ULL);
    const int total_slots = total_groups * group_size;
    uint16_t pool[total_groups * group_size];
    int i;

    if (total_slots < opennum)
    {
        sig_abort("ERROR: not enough unique openings");
    }

    for (i = 0; i < total_slots; ++i)
    {
        pool[i] = i;
    }
    for (i = 0; i < opennum; ++i)
    {
        int remaining = total_slots - i;
        int j = load_u16_be(stream + ((size_t)i * 2u)) % remaining;
        uint16_t value = pool[i + j];
        pool[i + j] = pool[i];
        pool[i] = value;
        commitment_open_trees[i] = value / group_size;
        commitment_open_choices[i] = (value % group_size) + MERKLE_LEAF_OFFSET;
    }
    free(stream);
}

/* Rebuild all public and embedded state from the serialized key seeds. */
void sig_build_public_state(const unsigned char H_seed[SEED_BYTES], const unsigned char e_seed[SEED_BYTES], unsigned long H_bits[embded_num][M], unsigned long e_bits[M], unsigned long y_bits[embded_num], RawGF2EX H_hat_raw[ROWS][COLS], RawNZTerms H_hat_terms[ROWS][COLS], RawGF2EX e_hat_prime_raw[N_PRIME_LEN], uint16_t y_hat_prime_raw[ROWS], RawGF2EX ye_raw[ROWS])
{
    random_H(H_bits, H_seed);
    random_e(e_bits, e_seed);
    generate_y(y_bits, e_bits, H_bits);
    embed_H_raw(H_hat_raw, H_bits);
    precompute_H_hat_terms(H_hat_terms, H_hat_raw);
    embed_e_raw(e_hat_prime_raw, e_bits);
    embed_y_raw(y_hat_prime_raw, y_bits);
    Compute_YE_Raw(ye_raw, H_hat_terms, e_hat_prime_raw);
}

void sig_build_public_state_direct(const unsigned char H_seed[SEED_BYTES], const unsigned char e_seed[SEED_BYTES], unsigned long e_bits[M], unsigned long y_bits[embded_num], RawGF2EX H_hat_raw[ROWS][COLS], RawNZTerms H_hat_terms[ROWS][COLS], RawGF2EX e_hat_prime_raw[N_PRIME_LEN], uint16_t y_hat_prime_raw[ROWS], RawGF2EX ye_raw[ROWS])
{
    unsigned long long bit_count = (unsigned long long)embded_num * (unsigned long long)M;
    unsigned char *stream = derive_xof_bytes(SIG_XOF_LABEL_H, H_seed, SEED_BYTES, bit_count);
    size_t bit_index = 0;
    int row;

    random_e(e_bits, e_seed);
    memset(y_bits, 0, sizeof(unsigned long) * embded_num);
    memset(H_hat_raw, 0, sizeof(RawGF2EX) * ROWS * COLS);

    for (row = 0; row < embded_num; ++row)
    {
        int row_block = row / 16;
        int row_bit = row % 16;
        int row_is_embedded = row_block < ROWS;
        int col;

        for (col = 0; col < M; ++col)
        {
            unsigned long bit = (unsigned long)((stream[bit_index / 8u] >> (7u - (bit_index % 8u))) & 1u);
            if (bit != 0u)
            {
                if (row_is_embedded)
                {
                    int col_block = col / 6;
                    int col_offset = col % 6;
                    H_hat_raw[row_block][col_block].coeffs[5 - col_offset] |= (uint16_t)(1u << row_bit);
                }
                if (e_bits[col] != 0u)
                {
                    y_bits[row] ^= 1u;
                }
            }
            ++bit_index;
        }
    }

    free(stream);
    precompute_H_hat_terms(H_hat_terms, H_hat_raw);
    embed_e_raw(e_hat_prime_raw, e_bits);
    embed_y_raw(y_hat_prime_raw, y_bits);
    Compute_YE_Raw(ye_raw, H_hat_terms, e_hat_prime_raw);
}

/* Run the full signing transcript for one message. */
void Prover(ProverWorkspace *ws, const unsigned char *message, size_t message_len, unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES], unsigned long e_bits[M], const RawNZTerms H_hat_terms[ROWS][COLS], uint16_t y_hat_prime_raw[ROWS], RawGF2EX e_hat_prime_raw[N_PRIME_LEN], const RawGF2EX ye_raw[ROWS], RawGF2EX *z_commit, RawGF2EX *e_eval_commit, RawGF2EX *v_eval_commit, RawGF2EX *delta_raw, ProverOutputs *outputs)
{
    MerkleBuildState merkle_state;
    uint16_t (*rand_challenges)[ROWS];
    uint16_t zeta_raw = 0;
#ifdef SIG_PROFILE_STEPS
    uint64_t t0 = 0;
#endif

    (void)e_bits;

    if (ws == NULL || message == NULL || z_commit == NULL || e_eval_commit == NULL ||
        v_eval_commit == NULL || delta_raw == NULL || outputs == NULL)
    {
        sig_abort("ERROR: Prover received NULL input");
    }

#ifdef SIG_PROFILE_STEPS
    t0 = sig_profile_now_ns();
#endif
    sig_hash_bytes(message, (size_t)message_len, ws->message_hash);

    sig_hash_ctx_init(&ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->message_prefix_ctx, ws->message_hash, HASH_DIGEST_LENGTH);

    memcpy(fs_commitment_input, ws->message_hash, HASH_DIGEST_LENGTH);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_message_hash_and_fs_commitment_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    Commitment_Raw(ws->commitment_hash_storage, e_hat_prime_raw, ws->v_hat_prime_raw, ws->v_encoded, ws->e_hat_encoded, ws->root_hash_storage, &merkle_state);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_commitment_launch_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif
    Initial_Computation_Prover_Raw(ws->yv_raw, H_hat_terms, ws->v_hat_prime_raw);
    merkle_join(&merkle_state);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_projection_and_merkle_join_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_init(&ws->sha_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, fs_commitment_input, FS_COMMITMENT_INPUT_BYTES);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)ws->root_hash_storage, total_groups * HASH_DIGEST_LENGTH);
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_commitment);

    hash_to_rand_commitment(z_commit, ws->fs_value_commitment);
    Commitment_Evals_Raw(e_hat_prime_raw, ws->v_hat_prime_raw, z_commit, e_eval_commit, v_eval_commit);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_message_hash_and_fs_commitment_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_commitment, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)e_eval_commit, sizeof(RawGF2EX));
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)v_eval_commit, sizeof(RawGF2EX));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_intercheck);

    rand_challenges = (uint16_t (*)[ROWS])ws->rand_challenges_flat;
    hash_to_rand_InterCheck(rand_challenges, ws->gamma_vec, ws->fs_value_intercheck);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_fs_intercheck_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    Interactive_Check_Raw_Prover(ws->p_poly_interactive, ws->rv_out_interactive, ye_raw, ws->yv_raw, ws->v_hat_prime_raw, e_hat_prime_raw, y_hat_prime_raw, rand_challenges, ws->gamma_vec);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_interactive_check_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    ensure_polydef_cache();

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_intercheck, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)ws->p_poly_interactive, sizeof(ws->p_poly_interactive));
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)ws->rv_out_interactive, sizeof(ws->rv_out_interactive));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_polyres);

    hash_to_rand_PolyRes(ws->tau_vec_raw, &zeta_raw, ws->fs_value_polyres);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_fs_polyres_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    ProverPolynomialResponse_Final_Raw(ws->G_coeffs_raw, COLS, e_hat_prime_raw, ws->v_hat_prime_raw, g_polydef_coeffs, ws->tau_vec_raw, e_hat_prime_raw + TAIL_POLY_OFFSET, ws->v_hat_prime_raw + TAIL_POLY_OFFSET, zeta_raw);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_poly_response_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_polyres, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)ws->G_coeffs_raw, sizeof(ws->G_coeffs_raw));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_mask);

    hash_to_rand_Mask(delta_raw, ws->fs_value_mask);
    Masking_Setup_Raw(delta_raw, e_hat_prime_raw, ws->v_hat_prime_raw, ws->w_hat_prime_raw);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_fs_mask_and_masking_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_polyres, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)ws->w_hat_prime_raw, sizeof(ws->w_hat_prime_raw));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_open);

    hash_to_rand_Open(ws->commitment_open_trees, ws->commitment_open_choices, ws->fs_value_open);
    ws->commitment_open_hash_count = Commitment_Open_Prover_Raw(ws->commitment_open_hashes, ws->e_hat_encoded_commitment_open_output, ws->commitment_open_trees, ws->commitment_open_choices, ws->commitment_hash_storage, ws->e_hat_encoded);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_fs_open_and_commitment_open_ns += sig_profile_now_ns() - t0;
#endif

    outputs->root_hash_ptr = ws->root_hash_storage;
    outputs->e_eval_commit_ptr = e_eval_commit;
    outputs->v_eval_commit_ptr = v_eval_commit;
    outputs->p_poly_interactive_ptr = ws->p_poly_interactive;
    outputs->rv_out_interactive_ptr = ws->rv_out_interactive;
    outputs->G_coeffs_raw_ptr = ws->G_coeffs_raw;
    outputs->w_hat_prime_raw_ptr = ws->w_hat_prime_raw;
    outputs->commitment_open_hashes_ptr = ws->commitment_open_hashes;
    outputs->commitment_open_hash_count = ws->commitment_open_hash_count;
    outputs->e_hat_encoded_commitment_open_output_ptr = ws->e_hat_encoded_commitment_open_output;
}

/* Run the full verifier transcript and accept only if all checks pass. */
int Verifier(VerifierWorkspace *ws, const unsigned char *message, size_t message_len, unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES], const RawNZTerms H_hat_terms[ROWS][COLS], uint16_t y_hat_prime_raw[ROWS], const RawGF2EX *e_eval_commit, const RawGF2EX *v_eval_commit, const unsigned char root_hash[total_groups][HASH_DIGEST_LENGTH], const RawGF2EX p_poly_interactive[SIGMA_REPETITIONS], const RawGF2EX rv_out_interactive[SIGMA_REPETITIONS], const RawGF2EX G_coeffs_raw[6], const RawGF2EX w_hat_prime_raw[N_PRIME_LEN], const unsigned char commitment_open_hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH], size_t commitment_open_hash_count, RawGF2EX e_hat_encoded_commitment_open_output[opennum], RawGF2EX *z_commit, RawGF2EX *delta_raw, RawGF2EX *vc_poly_challenge_raw, RawGF2EX *vc_verifier_check_raw)
{
    uint16_t (*rand_challenges)[ROWS];
    uint16_t zeta_raw = 0;
#ifdef SIG_PROFILE_STEPS
    uint64_t t0 = 0;
#endif

    if (ws == NULL || message == NULL || z_commit == NULL || delta_raw == NULL ||
        vc_poly_challenge_raw == NULL || vc_verifier_check_raw == NULL)
    {
        return 0;
    }

#ifdef SIG_PROFILE_STEPS
    t0 = sig_profile_now_ns();
#endif
    sig_hash_bytes(message, (size_t)message_len, ws->message_hash);

    sig_hash_ctx_init(&ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->message_prefix_ctx, ws->message_hash, HASH_DIGEST_LENGTH);

    memcpy(fs_commitment_input, ws->message_hash, HASH_DIGEST_LENGTH);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_message_hash_and_fs_commitment_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_init(&ws->sha_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, fs_commitment_input, FS_COMMITMENT_INPUT_BYTES);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)root_hash, total_groups * HASH_DIGEST_LENGTH);
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_commitment);

    hash_to_rand_commitment(z_commit, ws->fs_value_commitment);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_message_hash_and_fs_commitment_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_commitment, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)e_eval_commit, sizeof(RawGF2EX));
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)v_eval_commit, sizeof(RawGF2EX));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_intercheck);

    rand_challenges = (uint16_t (*)[ROWS])ws->rand_challenges_flat;
    hash_to_rand_InterCheck(rand_challenges, ws->gamma_vec, ws->fs_value_intercheck);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_fs_intercheck_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_intercheck, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)p_poly_interactive, SIGMA_REPETITIONS * sizeof(RawGF2EX));
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)rv_out_interactive, SIGMA_REPETITIONS * sizeof(RawGF2EX));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_polyres);

    hash_to_rand_PolyRes(ws->tau_vec_raw, &zeta_raw, ws->fs_value_polyres);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_fs_polyres_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_polyres, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)G_coeffs_raw, 6 * sizeof(RawGF2EX));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_mask);

    hash_to_rand_Mask(delta_raw, ws->fs_value_mask);

    sig_hash_ctx_copy(&ws->sha_ctx, &ws->message_prefix_ctx);
    sig_hash_ctx_update(&ws->sha_ctx, ws->fs_value_polyres, HASH_DIGEST_LENGTH);
    sig_hash_ctx_update(&ws->sha_ctx, (const unsigned char *)w_hat_prime_raw, N_PRIME_LEN * sizeof(RawGF2EX));
    sig_hash_ctx_final(&ws->sha_ctx, ws->fs_value_open);

    hash_to_rand_Open(ws->derived_trees, ws->derived_choices, ws->fs_value_open);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_fs_mask_and_fs_open_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif
    if (!Commitment_Open_Verifier_Raw(e_hat_encoded_commitment_open_output, ws->derived_trees, commitment_open_hashes, commitment_open_hash_count, ws->derived_choices, root_hash, w_hat_prime_raw, z_commit, e_eval_commit, v_eval_commit, delta_raw))
    {
#ifdef SIG_PROFILE_STEPS
        g_sig_step_profile.verify_commitment_open_with_overlap_ns += sig_profile_now_ns() - t0;
#endif
        return 0;
    }

#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_commitment_open_with_overlap_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif
    Initial_Computation_Verify_Raw(ws->yw_raw, H_hat_terms, w_hat_prime_raw);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_initial_computation_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    if (!Interactive_Check_Raw_verify(p_poly_interactive, rv_out_interactive, ws->yw_raw, w_hat_prime_raw, delta_raw, y_hat_prime_raw, rand_challenges, ws->gamma_vec))
    {
#ifdef SIG_PROFILE_STEPS
        g_sig_step_profile.verify_interactive_check_ns += sig_profile_now_ns() - t0;
#endif
        return 0;
    }
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_interactive_check_ns += sig_profile_now_ns() - t0;
    t0 = sig_profile_now_ns();
#endif

    ensure_polydef_cache();
    Polynomial_Challenge_Raw(vc_poly_challenge_raw, COLS, ws->tau_vec_raw, zeta_raw, w_hat_prime_raw, w_hat_prime_raw + TAIL_POLY_OFFSET, g_polydef_coeffs, delta_raw);
    VerifierCheck_Raw(vc_verifier_check_raw, G_coeffs_raw, delta_raw);
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_final_poly_check_ns += sig_profile_now_ns() - t0;
#endif
    return raw_equals(vc_poly_challenge_raw, vc_verifier_check_raw);
}

/* Initialize arithmetic backends used by both signer and verifier. */
int sig_run_self_test(void)
{
    if (!gf_tables_ready)
    {
        gf_init_tables();
    }
    if (!rs_tables_ready)
    {
        int rc = rs_init();
        if (rc != Rsencode_Success)
        {
            return rc;
        }
        rs_tables_ready = 1;
    }
    return 0;
}
