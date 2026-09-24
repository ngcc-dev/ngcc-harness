#include "bch_codec.h"
#include "params.h"

#include <stdlib.h>
#include <string.h>

#ifdef LORE_USE_NEON_CORE
#include <arm_neon.h>
#endif

#define BCH_MAX_M 10
#define BCH_MAX_T 28
#define BCH_NIBBLE_VALUES 16
#define BCH_MAX_ECC_BITS (BCH_MAX_M * BCH_MAX_T)
#define BCH_MAX_ROWS (2 * BCH_MAX_M * BCH_MAX_T)
#define BCH_MAX_WORDS ((BCH_MAX_ECC_BITS + 63) / 64)
#define BCH_MAX_ROW_WORDS ((BCH_MAX_ROWS + 63) / 64)

static unsigned int default_primitive_polynomial(unsigned int m)
{
    switch (m) {
    case 9:
        return 0x211;
    case 10:
        return 0x409;
    default:
        return 0;
    }
}

static unsigned int get_bit(const uint8_t *buf, unsigned int bit)
{
    return (buf[bit >> 3] >> (bit & 7)) & 1U;
}

static void toggle_bit(uint8_t *buf, unsigned int bit)
{
    buf[bit >> 3] ^= (uint8_t)(1U << (bit & 7));
}

static unsigned int bit_word(unsigned int bit)
{
    return bit >> 6;
}

static uint64_t bit_mask(unsigned int bit)
{
    return 1ULL << (bit & 63);
}

static unsigned int matrix_bit(const uint64_t *row, unsigned int bit)
{
    return (unsigned int)((row[bit_word(bit)] >> (bit & 63)) & 1ULL);
}

static void matrix_set(uint64_t *row, unsigned int bit)
{
    row[bit_word(bit)] |= bit_mask(bit);
}

static void row_xor(uint64_t *dst, const uint64_t *src, unsigned int words)
{
    for (unsigned int i = 0; i < words; i++) {
        dst[i] ^= src[i];
    }
}

static unsigned int word_parity(uint64_t x)
{
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    return (0x6996U >> (x & 0xfU)) & 1U;
}

static uint16_t gf_mul(const struct bch_control *bch, uint16_t a, uint16_t b)
{
    int exp;

    if (a == 0 || b == 0) {
        return 0;
    }

    exp = bch->index_of[a] + bch->index_of[b];
    exp %= (int)bch->n;
    return bch->alpha_to[exp];
}

static uint16_t gf_div(const struct bch_control *bch, uint16_t a, uint16_t b)
{
    int exp;

    if (a == 0) {
        return 0;
    }
    if (b == 0) {
        return 0;
    }

    exp = bch->index_of[a] - bch->index_of[b];
    while (exp < 0) {
        exp += (int)bch->n;
    }
    return bch->alpha_to[exp % (int)bch->n];
}

static uint16_t gf_pow_alpha(const struct bch_control *bch, unsigned int exp)
{
    return bch->alpha_to[exp % bch->n];
}

static int build_field(struct bch_control *bch, unsigned int prim_poly)
{
    unsigned int x = 1;

    bch->alpha_to = (uint16_t *)calloc(bch->n + 1, sizeof(uint16_t));
    bch->index_of = (int16_t *)calloc(bch->n + 1, sizeof(int16_t));
    if (bch->alpha_to == NULL || bch->index_of == NULL) {
        return -1;
    }

    for (unsigned int i = 0; i <= bch->n; i++) {
        bch->index_of[i] = -1;
    }

    for (unsigned int i = 0; i < bch->n; i++) {
        bch->alpha_to[i] = (uint16_t)x;
        bch->index_of[x] = (int16_t)i;
        x <<= 1;
        if (x & (1U << bch->m)) {
            x ^= prim_poly;
        }
        x &= bch->n;
    }
    bch->alpha_to[bch->n] = 1;

    return x == 1 ? 0 : -1;
}

static int build_parity_matrix(struct bch_control *bch)
{
    bch->parity_matrix = (uint64_t *)calloc((size_t)bch->rows * bch->words,
                                            sizeof(uint64_t));
    if (bch->parity_matrix == NULL) {
        return -1;
    }

    for (unsigned int root = 1; root <= 2 * bch->t; root++) {
        for (unsigned int col = 0; col < bch->ecc_bits; col++) {
            unsigned int pos = bch->msg_bits + col;
            uint16_t value = gf_pow_alpha(bch, root * pos);

            for (unsigned int bit = 0; bit < bch->m; bit++) {
                if ((value >> bit) & 1U) {
                    unsigned int row = (root - 1) * bch->m + bit;
                    matrix_set(&bch->parity_matrix[(size_t)row * bch->words], col);
                }
            }
        }
    }

    return 0;
}

static int build_parity_solver(const struct bch_control *bch,
                               uint64_t solver[BCH_MAX_ECC_BITS][BCH_MAX_ROW_WORDS],
                               unsigned int row_words)
{
    uint64_t matrix[BCH_MAX_ROWS][BCH_MAX_WORDS];
    uint64_t transform[BCH_MAX_ROWS][BCH_MAX_ROW_WORDS];
    int pivot_for_col[BCH_MAX_ECC_BITS];
    unsigned int rank = 0;

    memset(matrix, 0, sizeof(matrix));
    memset(transform, 0, sizeof(transform));
    memset(solver, 0, sizeof(uint64_t) * BCH_MAX_ECC_BITS * BCH_MAX_ROW_WORDS);
    for (unsigned int i = 0; i < bch->ecc_bits; i++) {
        pivot_for_col[i] = -1;
    }

    for (unsigned int row = 0; row < bch->rows; row++) {
        memcpy(matrix[row],
               &bch->parity_matrix[(size_t)row * bch->words],
               (size_t)bch->words * sizeof(uint64_t));
        matrix_set(transform[row], row);
    }

    for (unsigned int col = 0; col < bch->ecc_bits && rank < bch->rows; col++) {
        unsigned int pivot = rank;

        while (pivot < bch->rows && !matrix_bit(matrix[pivot], col)) {
            pivot++;
        }
        if (pivot == bch->rows) {
            continue;
        }

        if (pivot != rank) {
            uint64_t tmp_row[BCH_MAX_WORDS];
            uint64_t tmp_transform[BCH_MAX_ROW_WORDS];

            memcpy(tmp_row, matrix[pivot], sizeof(tmp_row));
            memcpy(matrix[pivot], matrix[rank], sizeof(tmp_row));
            memcpy(matrix[rank], tmp_row, sizeof(tmp_row));
            memcpy(tmp_transform, transform[pivot], sizeof(tmp_transform));
            memcpy(transform[pivot], transform[rank], sizeof(tmp_transform));
            memcpy(transform[rank], tmp_transform, sizeof(tmp_transform));
        }

        for (unsigned int row = 0; row < bch->rows; row++) {
            if (row != rank && matrix_bit(matrix[row], col)) {
                row_xor(matrix[row], matrix[rank], bch->words);
                row_xor(transform[row], transform[rank], row_words);
            }
        }

        pivot_for_col[col] = (int)rank;
        rank++;
    }

    for (unsigned int col = 0; col < bch->ecc_bits; col++) {
        if (pivot_for_col[col] >= 0) {
            memcpy(solver[col], transform[pivot_for_col[col]],
                   (size_t)row_words * sizeof(uint64_t));
        }
    }

    return 0;
}

static void build_message_rhs(const struct bch_control *bch,
                              unsigned int pos,
                              uint64_t rhs[BCH_MAX_ROW_WORDS])
{
    memset(rhs, 0, sizeof(uint64_t) * BCH_MAX_ROW_WORDS);

    for (unsigned int root = 1; root <= 2 * bch->t; root++) {
        uint16_t value = gf_pow_alpha(bch, root * pos);

        for (unsigned int bit = 0; bit < bch->m; bit++) {
            if ((value >> bit) & 1U) {
                unsigned int row = (root - 1) * bch->m + bit;
                matrix_set(rhs, row);
            }
        }
    }
}

static void solve_rhs_to_words(const struct bch_control *bch,
                               uint64_t solver[BCH_MAX_ECC_BITS][BCH_MAX_ROW_WORDS],
                               unsigned int row_words,
                               const uint64_t rhs[BCH_MAX_ROW_WORDS],
                               uint64_t *ecc_words)
{
    memset(ecc_words, 0, (size_t)bch->words * sizeof(uint64_t));

    for (unsigned int col = 0; col < bch->ecc_bits; col++) {
        unsigned int value = 0;

        for (unsigned int word = 0; word < row_words; word++) {
            value ^= word_parity(solver[col][word] & rhs[word]);
        }
        if (value & 1U) {
            matrix_set(ecc_words, col);
        }
    }
}

static int build_encode_matrix(struct bch_control *bch)
{
    uint64_t solver[BCH_MAX_ECC_BITS][BCH_MAX_ROW_WORDS];
    uint64_t rhs[BCH_MAX_ROW_WORDS];
    unsigned int row_words = (bch->rows + 63U) / 64U;

    bch->encode_matrix = (uint64_t *)calloc((size_t)bch->msg_bits * bch->words,
                                            sizeof(uint64_t));
    if (bch->encode_matrix == NULL) {
        return -1;
    }

    if (build_parity_solver(bch, solver, row_words) != 0) {
        return -1;
    }

    for (unsigned int pos = 0; pos < bch->msg_bits; pos++) {
        build_message_rhs(bch, pos, rhs);
        solve_rhs_to_words(bch, solver, row_words, rhs,
                           &bch->encode_matrix[(size_t)pos * bch->words]);
    }

    return 0;
}

static size_t syndrome_nibble_offset(const struct bch_control *bch,
                                     unsigned int nibble,
                                     unsigned int value)
{
    return ((size_t)nibble * BCH_NIBBLE_VALUES + value) * bch->roots;
}

static int build_syndrome_nibble_table(struct bch_control *bch)
{
    size_t entries = (size_t)bch->code_nibbles * BCH_NIBBLE_VALUES * bch->roots;

    bch->syndrome_nibble_table = (uint16_t *)calloc(entries, sizeof(uint16_t));
    if (bch->syndrome_nibble_table == NULL) {
        return -1;
    }

    for (unsigned int nibble = 0; nibble < bch->code_nibbles; nibble++) {
        unsigned int base_pos = 4U * nibble;

        for (unsigned int value = 0; value < BCH_NIBBLE_VALUES; value++) {
            uint16_t *entry = &bch->syndrome_nibble_table[
                syndrome_nibble_offset(bch, nibble, value)];

            for (unsigned int bit = 0; bit < 4; bit++) {
                unsigned int pos = base_pos + bit;

                if (pos >= bch->code_bits || ((value >> bit) & 1U) == 0) {
                    continue;
                }

                for (unsigned int root = 1; root <= bch->roots; root++) {
                    entry[root - 1] ^= gf_pow_alpha(bch, root * pos);
                }
            }
        }
    }

    return 0;
}

struct bch_control *init_bch(int m, int t, unsigned int prim_poly)
{
    struct bch_control *bch;

    if (m <= 0 || t <= 0 || m > BCH_MAX_M || t > BCH_MAX_T) {
        return NULL;
    }

    bch = (struct bch_control *)calloc(1, sizeof(*bch));
    if (bch == NULL) {
        return NULL;
    }

    bch->m = (unsigned int)m;
    bch->n = (1U << bch->m) - 1U;
    bch->t = (unsigned int)t;
    bch->ecc_bits = bch->m * bch->t;
    bch->ecc_bytes = (bch->ecc_bits + 7U) / 8U;
    bch->msg_bits = LORE_MSG_BYTES * 8U;
    bch->roots = 2U * bch->t;
    bch->code_bits = bch->msg_bits + bch->ecc_bits;
    bch->code_nibbles = (bch->code_bits + 3U) / 4U;
    bch->rows = bch->roots * bch->m;
    bch->words = (bch->ecc_bits + 63U) / 64U;

    if (bch->ecc_bits > BCH_MAX_ECC_BITS || bch->rows > BCH_MAX_ROWS ||
        bch->words > BCH_MAX_WORDS || bch->code_bits > bch->n) {
        free(bch);
        return NULL;
    }

    if (prim_poly == 0) {
        prim_poly = default_primitive_polynomial(bch->m);
    }
    if (prim_poly == 0 || build_field(bch, prim_poly) != 0 ||
        build_parity_matrix(bch) != 0 || build_encode_matrix(bch) != 0 ||
        build_syndrome_nibble_table(bch) != 0) {
        free_bch(bch);
        return NULL;
    }

    return bch;
}

void free_bch(struct bch_control *bch)
{
    if (bch == NULL) {
        return;
    }

    free(bch->alpha_to);
    free(bch->index_of);
    free(bch->parity_matrix);
    free(bch->encode_matrix);
    free(bch->syndrome_nibble_table);
    free(bch);
}

static int solve_parity(const struct bch_control *bch,
                        const uint8_t *data,
                        unsigned int data_bits,
                        uint8_t *ecc)
{
    uint64_t ecc_words[BCH_MAX_WORDS] = {0};

    if (data_bits != bch->msg_bits || bch->encode_matrix == NULL) {
        return -1;
    }

    memset(ecc, 0, bch->ecc_bytes);

    for (unsigned int pos = 0; pos < data_bits; pos++) {
        if (get_bit(data, pos)) {
            row_xor(ecc_words,
                    &bch->encode_matrix[(size_t)pos * bch->words],
                    bch->words);
        }
    }

    for (unsigned int byte = 0; byte < bch->ecc_bytes; byte++) {
        ecc[byte] = (uint8_t)(ecc_words[byte >> 3] >> (8 * (byte & 7)));
    }
    if ((bch->ecc_bits & 7U) != 0) {
        ecc[bch->ecc_bytes - 1] &= (uint8_t)((1U << (bch->ecc_bits & 7U)) - 1U);
    }

    return 0;
}

void encode_bch(struct bch_control *bch, const uint8_t *data,
                unsigned int len, uint8_t *ecc)
{
    if (bch == NULL || data == NULL || ecc == NULL ||
        solve_parity(bch, data, len * 8U, ecc) != 0) {
        if (ecc != NULL && bch != NULL) {
            memset(ecc, 0, bch->ecc_bytes);
        }
    }
}

void encodebits_bch(struct bch_control *bch, const uint8_t *data, uint8_t *ecc)
{
    if (bch == NULL || data == NULL || ecc == NULL ||
        solve_parity(bch, data, bch->msg_bits, ecc) != 0) {
        if (ecc != NULL && bch != NULL) {
            memset(ecc, 0, bch->ecc_bytes);
        }
    }
}

static void xor_syndrome_nibble(uint16_t syndrome[2 * BCH_MAX_T + 1],
                                const uint16_t *entry,
                                unsigned int roots)
{
    unsigned int root = 1;

#ifdef LORE_USE_NEON_CORE
    for (; root + 7U <= roots; root += 8) {
        uint16x8_t s = vld1q_u16(&syndrome[root]);
        uint16x8_t e = vld1q_u16(&entry[root - 1]);

        vst1q_u16(&syndrome[root], veorq_u16(s, e));
    }
#endif
    for (; root <= roots; root++) {
        syndrome[root] ^= entry[root - 1];
    }
}

static void compute_syndromes(const struct bch_control *bch,
                              const uint8_t *data,
                              unsigned int data_bits,
                              const uint8_t *ecc,
                              uint16_t syndrome[2 * BCH_MAX_T + 1])
{
    memset(syndrome, 0, (2 * BCH_MAX_T + 1) * sizeof(uint16_t));

    if (bch->syndrome_nibble_table != NULL && (data_bits & 7U) == 0) {
        unsigned int data_bytes = data_bits / 8U;
        unsigned int data_nibbles = data_bytes * 2U;
        unsigned int ecc_nibbles = (bch->ecc_bits + 3U) / 4U;

        for (unsigned int byte = 0; byte < data_bytes; byte++) {
            unsigned int low = data[byte] & 0x0fU;
            unsigned int high = data[byte] >> 4;

            xor_syndrome_nibble(syndrome,
                &bch->syndrome_nibble_table[
                    syndrome_nibble_offset(bch, 2U * byte, low)],
                bch->roots);
            xor_syndrome_nibble(syndrome,
                &bch->syndrome_nibble_table[
                    syndrome_nibble_offset(bch, 2U * byte + 1U, high)],
                bch->roots);
        }

        for (unsigned int nibble = 0; nibble < ecc_nibbles; nibble++) {
            unsigned int value = (ecc[nibble >> 1] >> (4U * (nibble & 1U))) & 0x0fU;

            xor_syndrome_nibble(syndrome,
                &bch->syndrome_nibble_table[
                    syndrome_nibble_offset(bch, data_nibbles + nibble, value)],
                bch->roots);
        }
        return;
    }

    for (unsigned int pos = 0; pos < data_bits; pos++) {
        if (get_bit(data, pos)) {
            for (unsigned int root = 1; root <= bch->roots; root++) {
                syndrome[root] ^= gf_pow_alpha(bch, root * pos);
            }
        }
    }

    for (unsigned int bit = 0; bit < bch->ecc_bits; bit++) {
        if (get_bit(ecc, bit)) {
            unsigned int pos = data_bits + bit;

            for (unsigned int root = 1; root <= bch->roots; root++) {
                syndrome[root] ^= gf_pow_alpha(bch, root * pos);
            }
        }
    }
}

static unsigned int syndromes_are_zero(const struct bch_control *bch,
                                       const uint16_t syndrome[2 * BCH_MAX_T + 1])
{
    for (unsigned int i = 1; i <= 2 * bch->t; i++) {
        if (syndrome[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static int berlekamp_massey(const struct bch_control *bch,
                            const uint16_t syndrome[2 * BCH_MAX_T + 1],
                            uint16_t locator[BCH_MAX_T + 1],
                            unsigned int *degree)
{
    uint16_t c[2 * BCH_MAX_T + 1] = {0};
    uint16_t b[2 * BCH_MAX_T + 1] = {0};
    unsigned int l = 0;
    unsigned int m = 1;
    uint16_t bb = 1;

    c[0] = 1;
    b[0] = 1;

    for (unsigned int n = 0; n < 2 * bch->t; n++) {
        uint16_t d = syndrome[n + 1];

        for (unsigned int i = 1; i <= l; i++) {
            d ^= gf_mul(bch, c[i], syndrome[n + 1 - i]);
        }

        if (d == 0) {
            m++;
        } else {
            uint16_t tmp[2 * BCH_MAX_T + 1];
            uint16_t coef = gf_div(bch, d, bb);

            memcpy(tmp, c, sizeof(tmp));
            for (unsigned int i = 0; i + m <= 2 * bch->t; i++) {
                c[i + m] ^= gf_mul(bch, coef, b[i]);
            }

            if (2 * l <= n) {
                l = n + 1 - l;
                memcpy(b, tmp, sizeof(b));
                bb = d;
                m = 1;
            } else {
                m++;
            }
        }
    }

    if (l > bch->t) {
        return -1;
    }

    memset(locator, 0, (BCH_MAX_T + 1) * sizeof(uint16_t));
    for (unsigned int i = 0; i <= l; i++) {
        locator[i] = c[i];
    }
    *degree = l;

    return 0;
}

static uint16_t eval_locator(const struct bch_control *bch,
                             const uint16_t locator[BCH_MAX_T + 1],
                             unsigned int degree,
                             uint16_t x)
{
    uint16_t y = locator[degree];

    for (unsigned int i = degree; i > 0; i--) {
        y = gf_mul(bch, y, x) ^ locator[i - 1];
    }

    return y;
}

static int find_error_locations(const struct bch_control *bch,
                                unsigned int code_bits,
                                const uint16_t locator[BCH_MAX_T + 1],
                                unsigned int degree,
                                unsigned int *errloc)
{
    unsigned int found = 0;

    for (unsigned int pos = 0; pos < code_bits && found < degree; pos++) {
        uint16_t x = gf_pow_alpha(bch, bch->n - (pos % bch->n));

        if (eval_locator(bch, locator, degree, x) == 0) {
            errloc[found++] = pos;
        }
    }

    return found == degree ? (int)found : -1;
}

int decode_bch(struct bch_control *bch, const uint8_t *data, unsigned int len,
               const uint8_t *recv_ecc, const uint8_t *calc_ecc,
               const unsigned int *syn, unsigned int *errloc)
{
    uint16_t syndrome[2 * BCH_MAX_T + 1];
    uint16_t locator[BCH_MAX_T + 1];
    unsigned int degree = 0;
    uint8_t local_ecc[(BCH_MAX_ECC_BITS + 7) / 8];
    const uint8_t *ecc = recv_ecc;

    (void)calc_ecc;
    (void)syn;

    if (bch == NULL || data == NULL || errloc == NULL || len * 8U != bch->msg_bits) {
        return -1;
    }

    if (ecc == NULL) {
        encode_bch(bch, data, len, local_ecc);
        ecc = local_ecc;
    }

    compute_syndromes(bch, data, len * 8U, ecc, syndrome);
    if (syndromes_are_zero(bch, syndrome)) {
        return 0;
    }

    if (berlekamp_massey(bch, syndrome, locator, &degree) != 0) {
        return -1;
    }

    return find_error_locations(bch, bch->msg_bits + bch->ecc_bits,
                                locator, degree, errloc);
}

int decodebits_bch(struct bch_control *bch, const uint8_t *data,
                   const uint8_t *recv_ecc, unsigned int *errloc)
{
    if (bch == NULL) {
        return -1;
    }
    return decode_bch(bch, data, bch->msg_bits / 8U, recv_ecc, NULL, NULL, errloc);
}

void correct_bch(struct bch_control *bch, uint8_t *data, unsigned int len,
                 unsigned int *errloc, int nerr)
{
    unsigned int data_bits = len * 8U;

    (void)bch;

    if (data == NULL || errloc == NULL || nerr <= 0) {
        return;
    }

    for (int i = 0; i < nerr; i++) {
        if (errloc[i] < data_bits) {
            toggle_bit(data, errloc[i]);
        }
    }
}

void correctbits_bch(struct bch_control *bch, uint8_t *databits,
                     unsigned int *errloc, int nerr)
{
    if (bch == NULL) {
        return;
    }
    correct_bch(bch, databits, bch->msg_bits / 8U, errloc, nerr);
}
