#include "code_layer.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define RS_MAX_N 130u
#define RS_MAX_VARS 130u
#define GF_POLY 0x11du

#ifdef NSS_HQC_DEBUG_KAT
static int dbg_saved_rs = 0;
static int dbg_suspend_rs_save = 0;
static uint8_t dbg_true_rs[RS_MAX_N];
static uint8_t dbg_true_msg[NSS_HQC_MSG_BYTES];
#endif

static uint8_t gf_exp[512];
static uint8_t gf_log[256];
static uint8_t gf_ready = 0u;

static uint8_t code_bit_get(const uint8_t *a, uint32_t pos)
{
    return (uint8_t)((a[pos >> 3] >> (pos & 7u)) & 1u);
}

static void code_bit_set(uint8_t *a, uint32_t pos, uint8_t v)
{
    uint8_t mask = (uint8_t)(1u << (pos & 7u));
    if (v) {
        a[pos >> 3] |= mask;
    } else {
        a[pos >> 3] &= (uint8_t)~mask;
    }
}

static uint8_t parity8(uint8_t x)
{
    x ^= (uint8_t)(x >> 4);
    x ^= (uint8_t)(x >> 2);
    x ^= (uint8_t)(x >> 1);
    return (uint8_t)(x & 1u);
}

static void gf_init(void)
{
    uint32_t x = 1u;
    uint32_t i;
    if (gf_ready != 0u) {
        return;
    }
    for (i = 0u; i < 255u; i++) {
        gf_exp[i] = (uint8_t)x;
        gf_log[x] = (uint8_t)i;
        x <<= 1;
        if ((x & 0x100u) != 0u) {
            x ^= GF_POLY;
        }
    }
    for (i = 255u; i < 512u; i++) {
        gf_exp[i] = gf_exp[i - 255u];
    }
    gf_ready = 1u;
}

static uint8_t gf_mul(uint8_t a, uint8_t b)
{
    if (a == 0u || b == 0u) {
        return 0u;
    }
    return gf_exp[(uint32_t)gf_log[a] + (uint32_t)gf_log[b]];
}

static uint8_t gf_inv(uint8_t a)
{
    if (a == 0u) {
        return 0u;
    }
    return gf_exp[255u - (uint32_t)gf_log[a]];
}

static uint8_t gf_pow_alpha(uint32_t e)
{
    return gf_exp[e % 255u];
}

static uint8_t poly_eval(const uint8_t *poly, uint32_t len, uint8_t x)
{
    uint32_t i = len;
    uint8_t y = 0u;
    while (i > 0u) {
        i--;
        y = (uint8_t)(gf_mul(y, x) ^ poly[i]);
    }
    return y;
}

static void rs_eval_encode(uint8_t *rs, const uint8_t *m)
{
    uint32_t i;
    gf_init();
    for (i = 0u; i < NSS_HQC_N1; i++) {
        rs[i] = poly_eval(m, NSS_HQC_K1, gf_pow_alpha(i));
    }
}

static void rs_compute_syndromes_eval(uint8_t *syndromes, const uint8_t *received)
{
    uint32_t j;
    gf_init();
    for (j = 0u; j < NSS_HQC_N1 - NSS_HQC_K1; j++) {
        uint32_t i;
        uint8_t s = 0u;
        for (i = 0u; i < NSS_HQC_N1; i++) {
            s ^= gf_mul(received[i], gf_pow_alpha(i * j));
        }
        syndromes[j] = s;
    }
}

static uint8_t rm_bit(uint8_t symbol, uint32_t point)
{
    uint8_t linear = (uint8_t)(symbol >> 1);
    uint8_t constant = (uint8_t)(symbol & 1u);
    return (uint8_t)(constant ^ parity8((uint8_t)(linear & (uint8_t)point)));
}

static void rm_encode_symbol(uint8_t *cw, uint32_t block, uint8_t symbol)
{
    uint32_t base = block * 128u * NSS_HQC_MULT;
    uint32_t point;
    for (point = 0u; point < 128u; point++) {
        uint8_t b = rm_bit(symbol, point);
        uint32_t rep;
        for (rep = 0u; rep < NSS_HQC_MULT; rep++) {
            code_bit_set(cw, base + point * NSS_HQC_MULT + rep, b);
        }
    }
}

#ifdef NSS_HQC_DEBUG_KAT
static void debug_print_code_summary(const char *prefix,
                                     const uint8_t *rs,
                                     const uint8_t *erased,
                                     const uint32_t *reliability_values,
                                     const int32_t *best_corr_values,
                                     const int32_t *second_corr_values)
{
    uint32_t i;
    uint32_t erasures = 0u;
    uint32_t min_rel = 0xffffffffu;
    uint32_t max_rel = 0u;
    uint64_t sum_rel = 0u;
    int32_t min_gap = 1000000;
    int32_t max_gap = -1000000;
    int64_t sum_gap = 0;

    for (i = 0u; i < NSS_HQC_N1; i++) {
        int32_t gap = best_corr_values[i] - second_corr_values[i];
        erasures += erased[i] != 0u ? 1u : 0u;
        if (reliability_values[i] < min_rel) {
            min_rel = reliability_values[i];
        }
        if (reliability_values[i] > max_rel) {
            max_rel = reliability_values[i];
        }
        if (gap < min_gap) {
            min_gap = gap;
        }
        if (gap > max_gap) {
            max_gap = gap;
        }
        sum_rel += reliability_values[i];
        sum_gap += gap;
    }

    fprintf(stderr,
            "[debug] %s erasures=%lu reliability min=%lu max=%lu avg=%lu gap min=%ld max=%ld avg=%ld\n",
            prefix,
            (unsigned long)erasures,
            (unsigned long)min_rel,
            (unsigned long)max_rel,
            (unsigned long)(sum_rel / NSS_HQC_N1),
            (long)min_gap,
            (long)max_gap,
            (long)(sum_gap / (int64_t)NSS_HQC_N1));
    fprintf(stderr, "[debug] %s RS first16 =", prefix);
    for (i = 0u; i < NSS_HQC_N1 && i < 16u; i++) {
        fprintf(stderr, " %02x", rs[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "[debug] %s reliability first16 =", prefix);
    for (i = 0u; i < NSS_HQC_N1 && i < 16u; i++) {
        fprintf(stderr, " %lu", (unsigned long)reliability_values[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "[debug] %s best/second/gap first16 =", prefix);
    for (i = 0u; i < NSS_HQC_N1 && i < 16u; i++) {
        fprintf(stderr, " %ld/%ld/%ld",
                (long)best_corr_values[i],
                (long)second_corr_values[i],
                (long)(best_corr_values[i] - second_corr_values[i]));
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "[debug] %s erased first16 =", prefix);
    for (i = 0u; i < NSS_HQC_N1 && i < 16u; i++) {
        fprintf(stderr, " %u", (unsigned)erased[i]);
    }
    fprintf(stderr, "\n");
}
#endif

static uint8_t rm_decode_symbol(const uint8_t *cw, uint32_t block, uint32_t *reliability
#ifdef NSS_HQC_DEBUG_KAT
                                , int32_t *best_corr, int32_t *second_best_corr
#endif
                                )
{
    uint32_t base = block * 128u * NSS_HQC_MULT;
    int32_t best = -1000000;
    int32_t second = -1000000;
    uint32_t best_symbol = 0u;
    uint32_t symbol;

    for (symbol = 0u; symbol < 256u; symbol++) {
        int32_t corr = 0;
        uint32_t point;
        for (point = 0u; point < 128u; point++) {
            uint8_t expected = rm_bit((uint8_t)symbol, point);
            uint32_t rep;
            for (rep = 0u; rep < NSS_HQC_MULT; rep++) {
                uint8_t got = code_bit_get(cw, base + point * NSS_HQC_MULT + rep);
                corr += (got == expected) ? 1 : -1;
            }
        }
        if (corr > best) {
            second = best;
            best = corr;
            best_symbol = symbol;
        } else if (corr > second) {
            second = corr;
        }
    }
    *reliability = (uint32_t)(best < 0 ? -best : best);
#ifdef NSS_HQC_DEBUG_KAT
    *best_corr = best;
    *second_best_corr = second;
#endif
    return (uint8_t)best_symbol;
}

static int gf_solve(uint8_t solution[RS_MAX_VARS],
                    uint8_t mat[RS_MAX_N][RS_MAX_VARS + 1u],
                    uint32_t rows, uint32_t vars)
{
    uint32_t pivot_col[RS_MAX_VARS];
    uint32_t rank = 0u;
    uint32_t col;
    memset(solution, 0, RS_MAX_VARS);

    for (col = 0u; col < vars && rank < rows; col++) {
        uint32_t pivot = rank;
        uint32_t r;
        while (pivot < rows && mat[pivot][col] == 0u) {
            pivot++;
        }
        if (pivot == rows) {
            continue;
        }
        if (pivot != rank) {
            uint32_t c;
            for (c = col; c <= vars; c++) {
                uint8_t tmp = mat[rank][c];
                mat[rank][c] = mat[pivot][c];
                mat[pivot][c] = tmp;
            }
        }
        {
            uint8_t inv = gf_inv(mat[rank][col]);
            uint32_t c;
            for (c = col; c <= vars; c++) {
                mat[rank][c] = gf_mul(mat[rank][c], inv);
            }
        }
        for (r = 0u; r < rows; r++) {
            if (r != rank && mat[r][col] != 0u) {
                uint8_t factor = mat[r][col];
                uint32_t c;
                for (c = col; c <= vars; c++) {
                    mat[r][c] ^= gf_mul(factor, mat[rank][c]);
                }
            }
        }
        pivot_col[rank] = col;
        rank++;
    }

    {
        uint32_t r;
        for (r = 0u; r < rows; r++) {
            uint32_t c;
            uint8_t nonzero = 0u;
            for (c = 0u; c < vars; c++) {
                nonzero |= mat[r][c];
            }
            if (nonzero == 0u && mat[r][vars] != 0u) {
                return -1;
            }
        }
    }
    {
        uint32_t r;
        for (r = 0u; r < rank; r++) {
            solution[pivot_col[r]] = mat[r][vars];
        }
    }
    return 0;
}

static int rs_divide_message(uint8_t *m, const uint8_t *q, uint32_t q_len,
                             const uint8_t *e, uint32_t e_deg)
{
    uint8_t rem[RS_MAX_VARS];
    int32_t d;
    memset(m, 0, NSS_HQC_MSG_BYTES);
    memset(rem, 0, sizeof(rem));
    memcpy(rem, q, q_len);

    for (d = (int32_t)q_len - 1; d >= (int32_t)e_deg; d--) {
        uint8_t coeff = rem[d];
        uint32_t mpos = (uint32_t)d - e_deg;
        uint32_t j;
        if (mpos >= NSS_HQC_K1) {
            if (coeff != 0u) {
                return -1;
            }
            continue;
        }
        m[mpos] = coeff;
        for (j = 0u; j <= e_deg; j++) {
            rem[mpos + j] ^= gf_mul(coeff, e[j]);
        }
    }
    {
        uint32_t i;
        for (i = 0u; i < e_deg; i++) {
            if (rem[i] != 0u) {
                return -1;
            }
        }
    }
    return 0;
}

static int rs_try_decode(uint8_t *m, const uint8_t *received,
                         const uint8_t *erased, uint32_t erasures, uint32_t errors)
{
    uint8_t mat[RS_MAX_N][RS_MAX_VARS + 1u];
    uint8_t sol[RS_MAX_VARS];
    uint8_t q[RS_MAX_VARS];
    uint8_t e[RS_MAX_VARS];
    uint8_t check[RS_MAX_N];
    uint32_t q_len = NSS_HQC_K1 + errors;
    uint32_t vars = NSS_HQC_K1 + 2u * errors;
    uint32_t row = 0u;
    uint32_t i;

#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr, "[debug] rs_try_decode errors=%lu q_len=%lu vars=%lu\n",
            (unsigned long)errors, (unsigned long)q_len, (unsigned long)vars);
#endif
    if (vars > RS_MAX_VARS || q_len > RS_MAX_VARS) {
#ifdef NSS_HQC_DEBUG_KAT
        fprintf(stderr, "[debug] rs_try_decode size failure\n");
#endif
        return -1;
    }
    memset(mat, 0, sizeof(mat));
    for (i = 0u; i < NSS_HQC_N1; i++) {
        uint8_t a;
        uint8_t apow;
        uint32_t j;
        if (erased[i] != 0u) {
            continue;
        }
        a = gf_pow_alpha(i);
        apow = 1u;
        for (j = 0u; j < q_len; j++) {
            mat[row][j] = apow;
            apow = gf_mul(apow, a);
        }
        apow = 1u;
        for (j = 0u; j < errors; j++) {
            mat[row][q_len + j] = gf_mul(received[i], apow);
            apow = gf_mul(apow, a);
        }
        mat[row][vars] = gf_mul(received[i], apow);
        row++;
    }
#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr, "[debug] rs_try_decode row=%lu vars=%lu q_len=%lu\n",
            (unsigned long)row, (unsigned long)vars, (unsigned long)q_len);
#endif
    if (row < vars) {
#ifdef NSS_HQC_DEBUG_KAT
        fprintf(stderr, "[debug] rs_try_decode row<vars failure\n");
#endif
        return -1;
    }
    {
        int solve_ret = gf_solve(sol, mat, row, vars);
        if (solve_ret != 0) {
#ifdef NSS_HQC_DEBUG_KAT
            fprintf(stderr, "[debug] gf_solve failed ret=%d\n", solve_ret);
#endif
            return -1;
        }
    }
    memset(q, 0, sizeof(q));
    memset(e, 0, sizeof(e));
    memcpy(q, sol, q_len);
    for (i = 0u; i < errors; i++) {
        e[i] = sol[q_len + i];
    }
    e[errors] = 1u;
    {
        int divide_ret = rs_divide_message(m, q, q_len, e, errors);
        if (divide_ret != 0) {
#ifdef NSS_HQC_DEBUG_KAT
            fprintf(stderr, "[debug] rs_divide_message failed ret=%d\n", divide_ret);
#endif
            return -1;
        }
    }
    rs_eval_encode(check, m);
    {
        uint32_t mismatches = 0u;
        uint32_t bound = NSS_HQC_N1 - NSS_HQC_K1;
        for (i = 0u; i < NSS_HQC_N1; i++) {
            if (erased[i] == 0u && check[i] != received[i]) {
                mismatches++;
            }
        }
#ifdef NSS_HQC_DEBUG_KAT
        fprintf(stderr,
                "[debug] rs_try_decode mismatches=%lu metric=%lu bound=%lu erasures=%lu\n",
                (unsigned long)mismatches,
                (unsigned long)(2u * mismatches + erasures),
                (unsigned long)bound,
                (unsigned long)erasures);
#endif
        if (2u * mismatches + erasures > bound) {
#ifdef NSS_HQC_DEBUG_KAT
            fprintf(stderr, "[debug] mismatch bound failure\n");
#endif
            return -1;
        }
    }
    return 0;
#if 0
    if (row < vars || gf_solve(sol, mat, row, vars) != 0) {
        return -1;
    }
    memset(q, 0, sizeof(q));
    memset(e, 0, sizeof(e));
    memcpy(q, sol, q_len);
    for (i = 0u; i < errors; i++) {
        e[i] = sol[q_len + i];
    }
    e[errors] = 1u;
    if (rs_divide_message(m, q, q_len, e, errors) != 0) {
        return -1;
    }
    rs_eval_encode(check, m);
    {
        uint32_t mismatches = 0u;
        for (i = 0u; i < NSS_HQC_N1; i++) {
            if (erased[i] == 0u && check[i] != received[i]) {
                mismatches++;
            }
        }
        if (2u * mismatches + erasures > NSS_HQC_N1 - NSS_HQC_K1) {
            return -1;
        }
    }
    return 0;
#endif
}

static int rs_decode(uint8_t *m, const uint8_t *received, const uint8_t *erased)
{
    uint32_t erasures = 0u;
    uint32_t max_errors;
    int32_t errors;
    uint32_t i;

    for (i = 0u; i < NSS_HQC_N1; i++) {
        erasures += erased[i] != 0u ? 1u : 0u;
    }
#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr, "[debug] rs_decode erasures=%lu redundancy=%lu\n",
            (unsigned long)erasures,
            (unsigned long)(NSS_HQC_N1 - NSS_HQC_K1));
#endif
    if (erasures > NSS_HQC_N1 - NSS_HQC_K1) {
#ifdef NSS_HQC_DEBUG_KAT
        fprintf(stderr, "[debug] rs_decode erasures exceed redundancy\n");
#endif
        return -1;
    }
    max_errors = (NSS_HQC_N1 - NSS_HQC_K1 - erasures) / 2u;
#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr, "[debug] rs_decode max_errors=%lu\n", (unsigned long)max_errors);
#endif
    for (errors = (int32_t)max_errors; errors >= 0; errors--) {
#ifdef NSS_HQC_DEBUG_KAT
        fprintf(stderr, "[debug] rs_decode trying errors=%ld\n", (long)errors);
#endif
        if (rs_try_decode(m, received, erased, erasures, (uint32_t)errors) == 0) {
#ifdef NSS_HQC_DEBUG_KAT
            fprintf(stderr, "[debug] rs_decode success errors=%ld\n", (long)errors);
#endif
            return 0;
        }
    }
#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr, "[debug] rs_decode failed all attempts\n");
#endif
    return -1;
}

#ifdef NSS_HQC_DEBUG_KAT
static uint8_t debug_msg_equal(const uint8_t *a, const uint8_t *b)
{
    uint32_t i;
    uint8_t diff = 0u;
    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }
    return (uint8_t)(diff == 0u);
}

static void debug_compare_rm_to_true_rs(const uint8_t *rs,
                                        const uint8_t *erased,
                                        const uint32_t *reliability_values)
{
    uint32_t i;
    uint32_t rm_err = 0u;
    uint32_t erasure_count = 0u;
    uint32_t nonerased_err = 0u;
    uint32_t erased_err = 0u;
    uint32_t erased_correct = 0u;
    uint32_t nonerased_correct = 0u;

    if (!dbg_saved_rs) {
        fprintf(stderr, "[debug] no saved true RS symbols for comparison\n");
        return;
    }

    for (i = 0u; i < NSS_HQC_N1; i++) {
        uint8_t wrong = (uint8_t)(rs[i] != dbg_true_rs[i]);
        if (wrong != 0u) {
            rm_err++;
        }
        if (erased[i] != 0u) {
            erasure_count++;
            if (wrong != 0u) {
                erased_err++;
            } else {
                erased_correct++;
            }
        } else {
            if (wrong != 0u) {
                nonerased_err++;
            } else {
                nonerased_correct++;
            }
        }
    }

    fprintf(stderr, "[debug] RM symbol errors = %lu / %lu\n",
            (unsigned long)rm_err, (unsigned long)NSS_HQC_N1);
    fprintf(stderr, "[debug] erasures = %lu\n", (unsigned long)erasure_count);
    fprintf(stderr, "[debug] erased_wrong = %lu\n", (unsigned long)erased_err);
    fprintf(stderr, "[debug] erased_correct = %lu\n", (unsigned long)erased_correct);
    fprintf(stderr, "[debug] nonerased_wrong = %lu\n", (unsigned long)nonerased_err);
    fprintf(stderr, "[debug] nonerased_correct = %lu\n", (unsigned long)nonerased_correct);
    fprintf(stderr, "[debug] RS condition current: 2*nonerased_wrong + erasures = %lu\n",
            (unsigned long)(2u * nonerased_err + erasure_count));
    fprintf(stderr, "[debug] RS redundancy = %lu\n",
            (unsigned long)(NSS_HQC_N1 - NSS_HQC_K1));

    for (i = 0u; i < NSS_HQC_N1; i++) {
        fprintf(stderr,
                "[debug][sym %02lu] true=%02x dec=%02x wrong=%u rel=%lu erased=%u\n",
                (unsigned long)i,
                dbg_true_rs[i],
                rs[i],
                (unsigned)(rs[i] != dbg_true_rs[i]),
                (unsigned long)reliability_values[i],
                (unsigned)erased[i]);
    }
}

static void debug_tau_scan(const uint8_t *rs, const uint32_t *reliability_values)
{
    static const uint32_t tau_values[] = {10u, 15u, 20u, 25u, 30u, 35u, 40u, 45u};
    uint32_t t;

    if (!dbg_saved_rs) {
        return;
    }

    for (t = 0u; t < (uint32_t)(sizeof(tau_values) / sizeof(tau_values[0])); t++) {
        uint8_t tmp_erased[RS_MAX_N];
        uint8_t m_tmp[NSS_HQC_MSG_BYTES];
        uint32_t tau = tau_values[t];
        uint32_t erasures = 0u;
        uint32_t nonerased_wrong = 0u;
        uint32_t i;
        int rs_ret;

        memset(tmp_erased, 0, sizeof(tmp_erased));
        memset(m_tmp, 0, sizeof(m_tmp));
        for (i = 0u; i < NSS_HQC_N1; i++) {
            tmp_erased[i] = (uint8_t)(reliability_values[i] * 100u <
                                      tau * 128u * NSS_HQC_MULT);
            if (tmp_erased[i] != 0u) {
                erasures++;
            } else if (rs[i] != dbg_true_rs[i]) {
                nonerased_wrong++;
            }
        }

        rs_ret = rs_decode(m_tmp, rs, tmp_erased);
        fprintf(stderr,
                "[debug] tau=%lu erasures=%lu nonerased_wrong=%lu metric=%lu rs_ret=%d m_equal=%u\n",
                (unsigned long)tau,
                (unsigned long)erasures,
                (unsigned long)nonerased_wrong,
                (unsigned long)(2u * nonerased_wrong + erasures),
                rs_ret,
                (unsigned)debug_msg_equal(m_tmp, dbg_true_msg));
    }
}
#endif

void nss_hqc_code_encode(uint8_t *cw, const uint8_t *m)
{
    uint8_t rs[RS_MAX_N];
    uint32_t i;
    memset(cw, 0, NSS_HQC_NC_BYTES);
    rs_eval_encode(rs, m);
#ifdef NSS_HQC_DEBUG_KAT
    if (!dbg_suspend_rs_save) {
        memcpy(dbg_true_rs, rs, NSS_HQC_N1);
        memcpy(dbg_true_msg, m, NSS_HQC_MSG_BYTES);
        dbg_saved_rs = 1;
        fprintf(stderr, "[debug] saved current encode true RS/msg for comparison\n");
    }
#endif
    for (i = 0u; i < NSS_HQC_N1; i++) {
        rm_encode_symbol(cw, i, rs[i]);
    }
}

int nss_hqc_code_selftest(void)
{
    uint8_t m[NSS_HQC_MSG_BYTES];
    uint8_t out[NSS_HQC_MSG_BYTES];
    uint8_t rs[RS_MAX_N];
    uint8_t synd[RS_MAX_N];
    uint8_t erased[RS_MAX_N];
    uint8_t cw[128u * 5u / 8u];
    static const uint8_t symbols[4] = {0x01u, 0x80u, 0x55u, 0xaau};
    uint32_t i;
    uint8_t diff = 0u;
    uint32_t redundancy = NSS_HQC_N1 - NSS_HQC_K1;
    uint32_t t = redundancy / 2u;

    memset(m, 0, sizeof(m));
    m[0] = 0x5au;
    rs_eval_encode(rs, m);
    for (i = 0u; i < NSS_HQC_N1; i++) {
        if (rs[i] != 0x5au) {
            return 1;
        }
    }

    memset(m, 0, sizeof(m));
    if (NSS_HQC_MSG_BYTES > 1u) {
        m[1] = 1u;
        rs_eval_encode(rs, m);
        for (i = 0u; i < NSS_HQC_N1; i++) {
            if (rs[i] != gf_pow_alpha(i)) {
                return 1;
            }
        }
    }

    memset(m, 0, sizeof(m));
    memset(erased, 0, sizeof(erased));
    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        m[i] = (uint8_t)(0x31u + 17u * i);
    }
    rs_eval_encode(rs, m);
    rs_compute_syndromes_eval(synd, rs);
    rs[1] ^= 0x55u;
    rs[3] ^= 0xa7u;
    erased[5] = 1u;
    erased[7] = 1u;
    if (rs_decode(out, rs, erased) != 0) {
        return 1;
    }
    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        diff |= (uint8_t)(m[i] ^ out[i]);
    }
    if (diff != 0u) {
        return 1;
    }

    /* RS boundary diagnostics: pure errors, pure erasures, mixed, and over-bound. */
    memset(m, 0, sizeof(m));
    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        m[i] = (uint8_t)(0x53u + 11u * i);
    }

    rs_eval_encode(rs, m);
    memset(erased, 0, sizeof(erased));
    for (i = 0u; i < t; i++) {
        rs[i] ^= (uint8_t)(0x21u + i);
    }
    memset(out, 0, sizeof(out));
    if (rs_decode(out, rs, erased) != 0 ||
        memcmp(m, out, NSS_HQC_MSG_BYTES) != 0) {
        return 1;
    }

    rs_eval_encode(rs, m);
    memset(erased, 0, sizeof(erased));
    for (i = 0u; i < redundancy; i++) {
        erased[i] = 1u;
    }
    memset(out, 0, sizeof(out));
    if (rs_decode(out, rs, erased) != 0 ||
        memcmp(m, out, NSS_HQC_MSG_BYTES) != 0) {
        return 1;
    }

    rs_eval_encode(rs, m);
    memset(erased, 0, sizeof(erased));
    for (i = 0u; i < 2u; i++) {
        rs[i] ^= (uint8_t)(0x61u + i);
    }
    for (i = 2u; i < redundancy - 2u; i++) {
        erased[i] = 1u;
    }
    memset(out, 0, sizeof(out));
    if (rs_decode(out, rs, erased) != 0 ||
        memcmp(m, out, NSS_HQC_MSG_BYTES) != 0) {
        return 1;
    }

    rs_eval_encode(rs, m);
    memset(erased, 0, sizeof(erased));
    for (i = 0u; i < t + 1u; i++) {
        rs[i] ^= (uint8_t)(0x41u + i);
    }
    memset(out, 0, sizeof(out));
    if (rs_decode(out, rs, erased) == 0) {
        return 1;
    }

    printf("[diag][rs] redundancy=%lu t=%lu pure_errors=%lu pure_erasures=%lu mixed_metric=%lu over_bound_metric=%lu\n",
           (unsigned long)redundancy,
           (unsigned long)t,
           (unsigned long)t,
           (unsigned long)redundancy,
           (unsigned long)(2u * 2u + (redundancy - 4u)),
           (unsigned long)(2u * (t + 1u)));
    printf("[diag][erasure] equal_rho_cap_tie_break=current implementation removes highest indices first; retained equal-rho erasures favor lowest indices.\n");

    for (i = 0u; i < 4u; i++) {
        uint32_t reliability = 0u;
        memset(cw, 0, sizeof(cw));
        rm_encode_symbol(cw, 0u, symbols[i]);
        if (rm_decode_symbol(cw, 0u, &reliability
#ifdef NSS_HQC_DEBUG_KAT
                             , &(int32_t){0}, &(int32_t){0}
#endif
                             ) != symbols[i]) {
            return 1;
        }
        if (reliability != 128u * NSS_HQC_MULT) {
            return 1;
        }
    }
    return 0;
}

#ifdef NSS_HQC_DEBUG_KAT
static void debug_code_layer_selftest_once(void)
{
    static uint8_t done = 0u;
    uint8_t m[NSS_HQC_MSG_BYTES];
    uint8_t m2[NSS_HQC_MSG_BYTES];
    uint8_t cw[NSS_HQC_NC_BYTES];
    uint8_t rs[RS_MAX_N];
    uint8_t erased[RS_MAX_N];
    uint32_t reliability_values[RS_MAX_N];
    int32_t best_corr_values[RS_MAX_N];
    int32_t second_corr_values[RS_MAX_N];
    uint32_t threshold = NSS_HQC_TAU_NUM * 128u * NSS_HQC_MULT;
    uint32_t i;
    uint8_t diff = 0u;
    int ret;

    if (done != 0u) {
        return;
    }
    done = 1u;

    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        m[i] = (uint8_t)(0xa5u ^ (uint8_t)(17u * i));
    }
    memset(m2, 0, sizeof(m2));
    memset(rs, 0, sizeof(rs));
    memset(erased, 0, sizeof(erased));
    memset(reliability_values, 0, sizeof(reliability_values));
    memset(best_corr_values, 0, sizeof(best_corr_values));
    memset(second_corr_values, 0, sizeof(second_corr_values));

    dbg_suspend_rs_save = 1;
    nss_hqc_code_encode(cw, m);
    dbg_suspend_rs_save = 0;
    for (i = 0u; i < NSS_HQC_N1; i++) {
        uint32_t reliability = 0u;
        rs[i] = rm_decode_symbol(cw, i, &reliability,
                                  &best_corr_values[i], &second_corr_values[i]);
        reliability_values[i] = reliability;
        erased[i] = (uint8_t)(reliability * NSS_HQC_TAU_DEN < threshold);
    }
    debug_print_code_summary("clean code selftest", rs, erased, reliability_values,
                             best_corr_values, second_corr_values);
    ret = rs_decode(m2, rs, erased);
    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        diff |= (uint8_t)(m[i] ^ m2[i]);
    }
    fprintf(stderr, "[debug] clean code selftest ret=%d m_equal=%u\n",
            ret, (unsigned)(diff == 0u));
    fprintf(stderr, "[debug] clean code selftest m[0..15] =");
    for (i = 0u; i < NSS_HQC_MSG_BYTES && i < 16u; i++) {
        fprintf(stderr, " %02x", m[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "[debug] clean code selftest m2[0..15] =");
    for (i = 0u; i < NSS_HQC_MSG_BYTES && i < 16u; i++) {
        fprintf(stderr, " %02x", m2[i]);
    }
    fprintf(stderr, "\n");
}
#endif

int nss_hqc_code_decode(uint8_t *m, const uint8_t *cw)
{
    uint8_t rs[RS_MAX_N];
    uint8_t erased[RS_MAX_N];
    uint32_t reliability_values[RS_MAX_N];
    uint32_t threshold = NSS_HQC_TAU_NUM * 128u * NSS_HQC_MULT;
    uint32_t i;
    uint32_t erasures = 0u;
#ifdef NSS_HQC_DEBUG_KAT
    int32_t best_corr_values[RS_MAX_N];
    int32_t second_corr_values[RS_MAX_N];
    int decode_ret;
#endif

    memset(rs, 0, sizeof(rs));
    memset(erased, 0, sizeof(erased));
    memset(reliability_values, 0, sizeof(reliability_values));
#ifdef NSS_HQC_DEBUG_KAT
    memset(best_corr_values, 0, sizeof(best_corr_values));
    memset(second_corr_values, 0, sizeof(second_corr_values));
    debug_code_layer_selftest_once();
    fprintf(stderr,
            "[debug] params N1=%lu K1=%lu MULT=%lu TAU=%lu/%lu threshold=%lu\n",
            (unsigned long)NSS_HQC_N1,
            (unsigned long)NSS_HQC_K1,
            (unsigned long)NSS_HQC_MULT,
            (unsigned long)NSS_HQC_TAU_NUM,
            (unsigned long)NSS_HQC_TAU_DEN,
            (unsigned long)threshold);
#endif
    for (i = 0u; i < NSS_HQC_N1; i++) {
        uint32_t reliability = 0u;
        rs[i] = rm_decode_symbol(cw, i, &reliability
#ifdef NSS_HQC_DEBUG_KAT
                                  , &best_corr_values[i], &second_corr_values[i]
#endif
                                  );
        erased[i] = (uint8_t)(reliability * NSS_HQC_TAU_DEN < threshold);
        reliability_values[i] = reliability;
        erasures += erased[i] != 0u ? 1u : 0u;
    }
    while (erasures > (NSS_HQC_N1 - NSS_HQC_K1) / 2u) {
        uint32_t best = 0u;
        uint32_t best_rel = 0u;
        for (i = 0u; i < NSS_HQC_N1; i++) {
            if (erased[i] != 0u && reliability_values[i] >= best_rel) {
                best = i;
                best_rel = reliability_values[i];
            }
        }
        erased[best] = 0u;
        erasures--;
    }
#ifdef NSS_HQC_DEBUG_KAT
    debug_print_code_summary("KAT decode input", rs, erased, reliability_values,
                             best_corr_values, second_corr_values);
    debug_compare_rm_to_true_rs(rs, erased, reliability_values);
    debug_tau_scan(rs, reliability_values);
    decode_ret = rs_decode(m, rs, erased);
    fprintf(stderr, "[debug] nss_hqc_code_decode ret=%d\n", decode_ret);
    fprintf(stderr, "[debug] decoded m[0..15] =");
    for (i = 0u; i < NSS_HQC_MSG_BYTES && i < 16u; i++) {
        fprintf(stderr, " %02x", m[i]);
    }
    fprintf(stderr, "\n");
    return decode_ret;
#else
    return rs_decode(m, rs, erased);
#endif
}

