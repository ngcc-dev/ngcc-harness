#include "config.h"

#include "aes.h"
#include "greatwall.h"
#include "faest_details.h"
#include "owf_proof.h"
#include "owf_alpha.h"
#include "quicksilver.h"
#include <string.h>

#if 0
static void debug_print_bytes(const char* tag, const void* data, size_t len)
{
    const unsigned char* p = (const unsigned char*)data;
    printf("%s = ", tag);
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", p[i]);
    }
    printf("\n");
}

static void debug_print_poly_secpar(const char* tag, poly_secpar_vec x)
{
    debug_print_bytes(tag, &x, sizeof(x));
}

static void debug_print_vec_gfsecpar(const char* tag, quicksilver_vec_gfsecpar x, bool verifier)
{
    char buf[128];

    if (!verifier) {
        snprintf(buf, sizeof(buf), "%s.value", tag);
        debug_print_poly_secpar(buf, x.value);
    }

    snprintf(buf, sizeof(buf), "%s.mac", tag);
    debug_print_poly_secpar(buf, x.mac);
}

static int debug_poly_secpar_eq(poly_secpar_vec a, poly_secpar_vec b)
{
    return memcmp(&a, &b, sizeof(a)) == 0;
}
#endif


#define NUM_COLS (OWF_BLOCK_SIZE / 4)
#define N_WD (SECURITY_PARAM / 32)
    #define S_ENC OWF_ROUNDS

static inline void gf137_shift_reduce(
    qs_gf137_bits* out,
    const qs_gf137_bits* y,
    size_t shift,
    quicksilver_state* state)
{
    quicksilver_vec_gf2 tmp[273];

    for (size_t i = 0; i < 273; ++i) {
        tmp[i] = quicksilver_zero_gf2();
    }

    for (size_t j = 0; j < 137; ++j) {
        tmp[j + shift] =
            quicksilver_add_gf2(
                state,
                tmp[j + shift],
                y->bit[j]
            );
    }


    for (int pos = 272; pos >= 137; --pos) {
        quicksilver_vec_gf2 t = tmp[pos];

        tmp[pos] = quicksilver_zero_gf2(state);

        tmp[pos - 137 + 21] =
            quicksilver_add_gf2(state, tmp[pos - 137 + 21], t);

        tmp[pos - 137] =
            quicksilver_add_gf2(state, tmp[pos - 137], t);
    }

    for (size_t i = 0; i < 137; ++i) {
        out->bit[i] = tmp[i];
    }
}

static inline void gf197_shift_reduce(
    qs_gf197_bits* out,
    const qs_gf197_bits* y,
    size_t shift,
    quicksilver_state* state)
{
    quicksilver_vec_gf2 tmp[393];

    for (size_t i = 0; i < 393; ++i) {
        tmp[i] = quicksilver_zero_gf2();
    }

    for (size_t j = 0; j < 197; ++j) {
        tmp[j + shift] =
            quicksilver_add_gf2(
                state,
                tmp[j + shift],
                y->bit[j]
            );
    }

    for (int pos = 392; pos >= 197; --pos) {
        quicksilver_vec_gf2 t = tmp[pos];

        tmp[pos] = quicksilver_zero_gf2();

        tmp[pos - 197 + 21] =
            quicksilver_add_gf2(state, tmp[pos - 197 + 21], t);
        tmp[pos - 197 + 2] =
            quicksilver_add_gf2(state, tmp[pos - 197 + 2], t);
        tmp[pos - 197 + 1] =
            quicksilver_add_gf2(state, tmp[pos - 197 + 1], t);
        tmp[pos - 197] =
            quicksilver_add_gf2(state, tmp[pos - 197], t);
    }

    for (size_t i = 0; i < 197; ++i) {
        out->bit[i] = tmp[i];
    }
}

static inline void gf263_shift_reduce(
    qs_gf263_bits* out,
    const qs_gf263_bits* y,
    size_t shift,
    quicksilver_state* state)
{
    quicksilver_vec_gf2 tmp[525];

    for (size_t i = 0; i < 525; ++i) {
        tmp[i] = quicksilver_zero_gf2();
    }

    for (size_t j = 0; j < 263; ++j) {
        tmp[j + shift] =
            quicksilver_add_gf2(
                state,
                tmp[j + shift],
                y->bit[j]
            );
    }

    for (int pos = 524; pos >= 263; --pos) {
        quicksilver_vec_gf2 t = tmp[pos];

        tmp[pos] = quicksilver_zero_gf2();

        tmp[pos - 263 + 93] =
            quicksilver_add_gf2(
                state,
                tmp[pos - 263 + 93],
                t
            );

        tmp[pos - 263] =
            quicksilver_add_gf2(
                state,
                tmp[pos - 263],
                t
            );
    }

    for (size_t i = 0; i < 263; ++i) {
        out->bit[i] = tmp[i];
    }
}

static inline void gf521_shift_reduce(qs_gf521_bits* out, const qs_gf521_bits* y,
                                      size_t shift, quicksilver_state* state)
{
    quicksilver_vec_gf2 tmp[1041];
    for (size_t i=0;i<1041;i++) tmp[i]=quicksilver_zero_gf2();
    for (size_t i=0;i<521;i++) tmp[i+shift]=quicksilver_add_gf2(state,tmp[i+shift],y->bit[i]);
    for (int pos=1040;pos>=521;--pos) {
        quicksilver_vec_gf2 t=tmp[pos];
        tmp[pos]=quicksilver_zero_gf2();
        tmp[pos-521+32]=quicksilver_add_gf2(state,tmp[pos-521+32],t);
        tmp[pos-521]=quicksilver_add_gf2(state,tmp[pos-521],t);
    }
    for(size_t i=0;i<521;i++) out->bit[i]=tmp[i];
}

static void qs_gf137_mul_constraint(
    quicksilver_state* state,
    const qs_gf137_bits* x,
    const qs_gf137_bits* y,
    const qs_gf137_bits* z)
{
    quicksilver_vec_gfsecpar x_bits[137];
    quicksilver_vec_gfsecpar y_bits[137];
    for (size_t i = 0; i < 137; ++i) {
        x_bits[i] = quicksilver_combine_1_bit(state, x->bit[i]);
        y_bits[i] = quicksilver_combine_1_bit(state, y->bit[i]);
    }

    quicksilver_vec_deg2 product[273];
    for (size_t i = 0; i < 273; ++i) {
        product[i] = quicksilver_zero_deg2();
    }

    for (size_t i = 0; i < 137; ++i) {
        for (size_t j = 0; j < 137; ++j) {
            product[i + j] = quicksilver_add_deg2(
                state,
                product[i + j],
                quicksilver_mul(state, x_bits[i], y_bits[j])
            );
        }
    }

    for (int pos = 272; pos >= 137; --pos) {
        const quicksilver_vec_deg2 t = product[pos];
        product[pos - 137 + 21] =
            quicksilver_add_deg2(state, product[pos - 137 + 21], t);
        product[pos - 137] =
            quicksilver_add_deg2(state, product[pos - 137], t);
    }

    for (size_t bit = 0; bit < 137; ++bit) {
        const quicksilver_vec_gfsecpar z_bit =
            quicksilver_combine_1_bit(state, z->bit[bit]);
        const quicksilver_vec_deg2 z_term =
            quicksilver_mul(state, z_bit, quicksilver_one_gfsecpar(state));
        quicksilver_constraint(
            state,
            quicksilver_add_deg2(state, product[bit], z_term)
        );
    }
}

#if SECURITY_PARAM == 192
static void qs_gf197_mul_constraint(
    quicksilver_state* state,
    const qs_gf197_bits* x,
    const qs_gf197_bits* y,
    const qs_gf197_bits* z)
{
    quicksilver_vec_gfsecpar x_bits[197];
    quicksilver_vec_gfsecpar y_bits[197];
    for (size_t i = 0; i < 197; ++i) {
        x_bits[i] = quicksilver_combine_1_bit(state, x->bit[i]);
        y_bits[i] = quicksilver_combine_1_bit(state, y->bit[i]);
    }

    quicksilver_vec_deg2 product[393];
    for (size_t i = 0; i < 393; ++i) {
        product[i] = quicksilver_zero_deg2();
    }

    for (size_t i = 0; i < 197; ++i) {
        for (size_t j = 0; j < 197; ++j) {
            product[i + j] = quicksilver_add_deg2(
                state,
                product[i + j],
                quicksilver_mul(state, x_bits[i], y_bits[j])
            );
        }
    }

    for (int pos = 392; pos >= 197; --pos) {
        const quicksilver_vec_deg2 t = product[pos];
        product[pos - 197 + 21] =
            quicksilver_add_deg2(state, product[pos - 197 + 21], t);
        product[pos - 197 + 2] =
            quicksilver_add_deg2(state, product[pos - 197 + 2], t);
        product[pos - 197 + 1] =
            quicksilver_add_deg2(state, product[pos - 197 + 1], t);
        product[pos - 197] =
            quicksilver_add_deg2(state, product[pos - 197], t);
    }

    for (size_t bit = 0; bit < 197; ++bit) {
        const quicksilver_vec_gfsecpar z_bit =
            quicksilver_combine_1_bit(state, z->bit[bit]);
        const quicksilver_vec_deg2 z_term =
            quicksilver_mul(state, z_bit, quicksilver_one_gfsecpar(state));
        quicksilver_constraint(
            state,
            quicksilver_add_deg2(state, product[bit], z_term)
        );
    }
}
#endif

#if SECURITY_PARAM == 256
static void qs_gf263_mul_constraint(
    quicksilver_state* state,
    const qs_gf263_bits* x,
    const qs_gf263_bits* y,
    const qs_gf263_bits* z)
{
    quicksilver_vec_gfsecpar x_bits[263];
    quicksilver_vec_gfsecpar y_bits[263];
    for (size_t i = 0; i < 263; ++i) {
        x_bits[i] = quicksilver_combine_1_bit(state, x->bit[i]);
        y_bits[i] = quicksilver_combine_1_bit(state, y->bit[i]);
    }

    quicksilver_vec_deg2 product[525];
    for (size_t i = 0; i < 525; ++i) {
        product[i] = quicksilver_zero_deg2();
    }

    for (size_t i = 0; i < 263; ++i) {
        for (size_t j = 0; j < 263; ++j) {
            product[i + j] = quicksilver_add_deg2(
                state,
                product[i + j],
                quicksilver_mul(state, x_bits[i], y_bits[j])
            );
        }
    }

    for (int pos = 524; pos >= 263; --pos) {
        const quicksilver_vec_deg2 t = product[pos];
        product[pos - 263 + 93] =
            quicksilver_add_deg2(state, product[pos - 263 + 93], t);
        product[pos - 263] =
            quicksilver_add_deg2(state, product[pos - 263], t);
    }

    for (size_t bit = 0; bit < 263; ++bit) {
        const quicksilver_vec_gfsecpar z_bit =
            quicksilver_combine_1_bit(state, z->bit[bit]);
        const quicksilver_vec_deg2 z_term =
            quicksilver_mul(state, z_bit, quicksilver_one_gfsecpar(state));
        quicksilver_constraint(
            state,
            quicksilver_add_deg2(state, product[bit], z_term)
        );
    }
}
#endif

#if SECURITY_PARAM == 512
static void qs_gf521_mul_constraint(
    quicksilver_state* state,
    const qs_gf521_bits* x,
    const qs_gf521_bits* y,
    const qs_gf521_bits* z)
{
    quicksilver_vec_gfsecpar x_bits[521];
    quicksilver_vec_gfsecpar y_bits[521];
    for (size_t i = 0; i < 521; ++i) {
        x_bits[i] = quicksilver_combine_1_bit(state, x->bit[i]);
        y_bits[i] = quicksilver_combine_1_bit(state, y->bit[i]);
    }

    quicksilver_vec_deg2 product[1041];
    for (size_t i = 0; i < 1041; ++i) {
        product[i] = quicksilver_zero_deg2();
    }

    for (size_t i = 0; i < 521; ++i) {
        for (size_t j = 0; j < 521; ++j) {
            product[i + j] = quicksilver_add_deg2(
                state,
                product[i + j],
                quicksilver_mul(state, x_bits[i], y_bits[j])
            );
        }
    }

    for (int pos = 1040; pos >= 521; --pos) {
        const quicksilver_vec_deg2 t = product[pos];
        product[pos - 521 + 32] =
            quicksilver_add_deg2(state, product[pos - 521 + 32], t);
        product[pos - 521] =
            quicksilver_add_deg2(state, product[pos - 521], t);
    }

    for (size_t bit = 0; bit < 521; ++bit) {
        const quicksilver_vec_gfsecpar z_bit =
            quicksilver_combine_1_bit(state, z->bit[bit]);
        const quicksilver_vec_deg2 z_term =
            quicksilver_mul(state, z_bit, quicksilver_one_gfsecpar(state));
        quicksilver_constraint(
            state,
            quicksilver_add_deg2(state, product[bit], z_term)
        );
    }
}
#endif

static ALWAYS_INLINE void load_rc137_bits(
    quicksilver_state* state,
    qs_gf137_bits* out,
    const uint64_t rc_words[3])
{
    for (size_t i = 0; i < 137; ++i) {
        uint8_t b = (rc_words[i >> 6] >> (i & 63)) & 1;

        out->bit[i] = b
            ? quicksilver_one_gf2(state)
            : quicksilver_zero_gf2(state);
    }
}

static ALWAYS_INLINE void load_rc197_bits(
    quicksilver_state* state,
    qs_gf197_bits* out,
    const uint64_t rc_words[4])
{
    for (size_t i = 0; i < 197; ++i) {
        uint8_t b = (rc_words[i >> 6] >> (i & 63)) & 1;

        out->bit[i] = b
            ? quicksilver_one_gf2(state)
            : quicksilver_zero_gf2(state);
    }
}

static ALWAYS_INLINE void load_rc263_bits(
    quicksilver_state* state,
    qs_gf263_bits* out,
    const uint64_t rc_words[5])
{
    for (size_t i = 0; i < 263; ++i) {
        uint8_t b = (rc_words[i >> 6] >> (i & 63)) & 1;

        out->bit[i] = b
            ? quicksilver_one_gf2(state)
            : quicksilver_zero_gf2(state);
    }
}

static ALWAYS_INLINE void load_rc521_bits(
    quicksilver_state* state, 
    qs_gf521_bits* out,
    const uint64_t rc_words[9])
{
    for(size_t i = 0; i < 521; ++i){
        uint8_t b = (rc_words[i >> 6] >> (i & 63)) & 1;
        
        out->bit[i] = b
            ? quicksilver_one_gf2(state)
            : quicksilver_zero_gf2(state);
    }
}

static ALWAYS_INLINE uint8_t mat137_get_bit(
    const uint64_t matrix[137][3],
    size_t row,
    size_t col)
{
    uint64_t word = matrix[row][col >> 6];
    return (uint8_t)((word >> (col & 63)) & 1ULL);
}

static ALWAYS_INLINE uint8_t mat197_get_bit(
    const uint64_t matrix[197][4],
    size_t row,
    size_t col)
{
    uint64_t word = matrix[row][col >> 6];
    return (uint8_t)((word >> (col & 63)) & 1ULL);
}

static ALWAYS_INLINE uint8_t mat263_get_bit(
    const uint64_t matrix[263][5],
    size_t row,
    size_t col)
{
    uint64_t word = matrix[row][col >> 6];
    return (uint8_t)((word >> (col & 63)) & 1ULL);
}

static ALWAYS_INLINE uint8_t mat521_get_bit(
    const uint64_t matrix[521][9],
    size_t row,
    size_t col)
{
    uint64_t word = matrix[row][col >> 6];
    return (uint8_t)((word >> (col & 63)) & 1ULL);
}




static ALWAYS_INLINE void gf521_add(quicksilver_state* state, qs_gf521_bits* out,
                      const qs_gf521_bits* a, const qs_gf521_bits* b)
{
    for(size_t i=0;i<521;i++) out->bit[i]=quicksilver_add_gf2(state,a->bit[i],b->bit[i]);
}

static ALWAYS_INLINE void gf521_matmul(quicksilver_state* state, qs_gf521_bits* out,
                         const uint64_t matrix[521][9], const qs_gf521_bits* in)
{
    for(size_t row=0;row<521;row++) {
        out->bit[row]=quicksilver_zero_gf2();
        for(size_t col=0;col<521;col++) if((matrix[row][col>>6]>>(col&63))&1)
            out->bit[row]=quicksilver_add_gf2(state,out->bit[row],in->bit[col]);
    }
}

#define DEFINE_OWF_DOT_CONSTRAINT(function_name, bits_type, block_type, field_bits)       \
static void function_name(                                                               \
    quicksilver_state* state, const bits_type* y, const block_type* alpha)               \
{                                                                                         \
    quicksilver_vec_gf2 dot = quicksilver_zero_gf2();                                    \
    for (size_t bit = 0; bit < field_bits; ++bit) {                                      \
        if ((alpha->data[bit >> 6] >> (bit & 63)) & 1ULL)                                \
            dot = quicksilver_add_gf2(state, dot, y->bit[bit]);                          \
    }                                                                                     \
    quicksilver_inverse_constraint(                                                       \
        state, quicksilver_combine_1_bit(state, dot), quicksilver_one_gfsecpar(state));   \
}

DEFINE_OWF_DOT_CONSTRAINT(qs_gf137_dot_constraint, qs_gf137_bits, block137, 137)
#if SECURITY_PARAM == 192
DEFINE_OWF_DOT_CONSTRAINT(qs_gf197_dot_constraint, qs_gf197_bits, block197, 197)
#endif
#if SECURITY_PARAM == 256
DEFINE_OWF_DOT_CONSTRAINT(qs_gf263_dot_constraint, qs_gf263_bits, block263, 263)
#endif
#if SECURITY_PARAM == 512
DEFINE_OWF_DOT_CONSTRAINT(qs_gf521_dot_constraint, qs_gf521_bits, block521, 521)
#endif

#undef DEFINE_OWF_DOT_CONSTRAINT

static ALWAYS_INLINE void enc_constraints(quicksilver_state* state, owf_block out, owf_block owf_iv) {
    owf_block owf_alpha0 = {{0}};
    owf_block owf_alpha1 = {{0}};
    (void)derive_owf_alpha(&owf_alpha0, &owf_iv, 0);
    (void)derive_owf_alpha(&owf_alpha1, &owf_iv, 1);
    
#if SECURITY_PARAM == 128

        qs_gf137_bits inv_inputs[S_ENC];
        qs_gf137_bits inv_outputs[S_ENC];
        qs_gf137_bits pow_outputs[S_ENC];
    
        
        qs_gf137_bits sk_bits;
        for (size_t bit_j = 0; bit_j < 137; ++bit_j){
            sk_bits.bit[bit_j] = quicksilver_get_witness_vec(state, bit_j);
            inv_outputs[0].bit[bit_j] = quicksilver_get_witness_vec(state, bit_j + 144);
        }

        qs_gf137_bits rc0_bits, rc1_bits, owf_iv_bits;

        load_rc137_bits(state, &rc0_bits, greatwall_rc_137[0]);
        load_rc137_bits(state, &rc1_bits, greatwall_rc_137[1]);
        load_rc137_bits(state, &owf_iv_bits, owf_iv.data);

        qs_gf137_bits matrix0_output, matrix1_output, matrix2_output;
        for (size_t matrow = 0; matrow < 137; matrow++) {
            matrix0_output.bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_mat_137_0, matrow, matcol);
                if (bit) {
                    matrix0_output.bit[matrow] = quicksilver_add_gf2(state, matrix0_output.bit[matrow], sk_bits.bit[matcol]);
                }
            }
        }

        for (size_t i = 0; i < 137; ++i) {
            inv_inputs[0].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix0_output.bit[i],
                    rc0_bits.bit[i]
                );
        }

        for (size_t matrow = 0; matrow < 137; matrow++) {
            pow_outputs[0].bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_pow_mat_137_70, matrow, matcol);
                if (bit) {
                    pow_outputs[0].bit[matrow] = quicksilver_add_gf2(state, pow_outputs[0].bit[matrow], inv_outputs[0].bit[matcol]);
                }
            }
        }
        
        qs_gf137_bits tmp;

        for (size_t i = 0; i < 137; ++i) {
            tmp.bit[i] = quicksilver_add_gf2(
                    state,
                    inv_outputs[0].bit[i],
                    sk_bits.bit[i]
                );
        }

        for (size_t matrow = 0; matrow < 137; matrow++) {
            matrix1_output.bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_mat_137_1, matrow, matcol);
                if (bit) {
                    matrix1_output.bit[matrow] = quicksilver_add_gf2(state, matrix1_output.bit[matrow], tmp.bit[matcol]);
                }
            }
        }

        for (size_t i = 0; i < 137; ++i) {
            inv_inputs[1].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix1_output.bit[i],
                    owf_iv_bits.bit[i]
                );
        }

        qs_gf137_bits out_bits;
        for (size_t i = 0; i < 137; ++i) {
            uint8_t b = (out.data[i >> 6] >> (i & 63)) & 1;

            out_bits.bit[i] = b
                ? quicksilver_one_gf2(state)
                : quicksilver_zero_gf2();
        }

        for (size_t i = 0; i < 137; ++i) {
            matrix2_output.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_inputs[1].bit[i],
                    quicksilver_add_gf2(
                        state,
                        rc1_bits.bit[i],
                        quicksilver_add_gf2(
                            state,
                            sk_bits.bit[i],
                            out_bits.bit[i]
                        )
                    )
                );
        }

        for (size_t matrow = 0; matrow < 137; matrow++) {
            inv_outputs[1].bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_mat_137_2_inv, matrow, matcol);
                if (bit) {
                    inv_outputs[1].bit[matrow] = quicksilver_add_gf2(state, inv_outputs[1].bit[matrow], matrix2_output.bit[matcol]);
                }
            }
        }
        
        for (size_t matrow = 0; matrow < 137; matrow++) {
            pow_outputs[1].bit[matrow] = quicksilver_zero_gf2();
            for (size_t matcol = 0; matcol < 137; matcol++) {
                uint8_t bit = mat137_get_bit(greatwall_pow_mat_137_75, matrow, matcol);
                if (bit) {
                    pow_outputs[1].bit[matrow] = quicksilver_add_gf2(state, pow_outputs[1].bit[matrow], inv_outputs[1].bit[matcol]);
                }
            }
        }
        for(int i = 0; i < S_ENC; i++)qs_gf137_mul_constraint(state, &inv_inputs[i], &inv_outputs[i], &pow_outputs[i]);
        qs_gf137_dot_constraint(state, &inv_outputs[0], &owf_alpha0);
        qs_gf137_dot_constraint(state, &inv_outputs[1], &owf_alpha1);

#elif SECURITY_PARAM == 192

        qs_gf197_bits inv_inputs[S_ENC];
        qs_gf197_bits inv_outputs[S_ENC];
        qs_gf197_bits pow_outputs[S_ENC];

        qs_gf197_bits sk_bits;

        for (size_t bit_j = 0; bit_j < 197; ++bit_j) {
            sk_bits.bit[bit_j] = quicksilver_get_witness_vec(state, bit_j);

            inv_outputs[0].bit[bit_j] =
                quicksilver_get_witness_vec(state, bit_j + 200);
        }

        qs_gf197_bits rc0_bits, rc1_bits, owf_iv_bits;

        load_rc197_bits(state, &rc0_bits, greatwall_rc_197[0]);
        load_rc197_bits(state, &rc1_bits, greatwall_rc_197[1]);
        load_rc197_bits(state, &owf_iv_bits, owf_iv.data);

        qs_gf197_bits matrix0_output, matrix1_output, matrix2_output;


        for (size_t matrow = 0; matrow < 197; matrow++) {
            matrix0_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 197; matcol++) {
                uint8_t bit = mat197_get_bit(greatwall_mat_197_0, matrow, matcol);

                if (bit) {
                    matrix0_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix0_output.bit[matrow],
                            sk_bits.bit[matcol]
                        );
                }
            }
        }


        for (size_t i = 0; i < 197; ++i) {
            inv_inputs[0].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix0_output.bit[i],
                    rc0_bits.bit[i]
                );
        }


        for (size_t matrow = 0; matrow < 197; matrow++) {
            pow_outputs[0].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 197; matcol++) {
                uint8_t bit =
                    mat197_get_bit(greatwall_pow_mat_197_94, matrow, matcol);

                if (bit) {
                    pow_outputs[0].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[0].bit[matrow],
                            inv_outputs[0].bit[matcol]
                        );
                }
            }
        }


        qs_gf197_bits tmp;

        for (size_t i = 0; i < 197; ++i) {
            tmp.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_outputs[0].bit[i],
                    sk_bits.bit[i]
                );
        }


        for (size_t matrow = 0; matrow < 197; matrow++) {
            matrix1_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 197; matcol++) {
                uint8_t bit = mat197_get_bit(greatwall_mat_197_1, matrow, matcol);

                if (bit) {
                    matrix1_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix1_output.bit[matrow],
                            tmp.bit[matcol]
                        );
                }
            }
        }


        for (size_t i = 0; i < 197; ++i) {
            inv_inputs[1].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix1_output.bit[i],
                    owf_iv_bits.bit[i]
                );
        }


        qs_gf197_bits out_bits;

        for (size_t i = 0; i < 197; ++i) {
            uint8_t b = (out.data[i >> 6] >> (i & 63)) & 1;

            out_bits.bit[i] = b
                ? quicksilver_one_gf2(state)
                : quicksilver_zero_gf2();
        }

        for (size_t i = 0; i < 197; ++i) {
            matrix2_output.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_inputs[1].bit[i],
                    quicksilver_add_gf2(
                        state,
                        rc1_bits.bit[i],
                        quicksilver_add_gf2(
                            state,
                            sk_bits.bit[i],
                            out_bits.bit[i]
                        )
                    )
                );
        }


        for (size_t matrow = 0; matrow < 197; matrow++) {
            inv_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 197; matcol++) {
                uint8_t bit =
                    mat197_get_bit(greatwall_mat_197_2_inv, matrow, matcol);

                if (bit) {
                    inv_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            inv_outputs[1].bit[matrow],
                            matrix2_output.bit[matcol]
                        );
                }
            }
        }


        for (size_t matrow = 0; matrow < 197; matrow++) {
            pow_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 197; matcol++) {
                uint8_t bit =
                    mat197_get_bit(greatwall_pow_mat_197_105, matrow, matcol);

                if (bit) {
                    pow_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[1].bit[matrow],
                            inv_outputs[1].bit[matcol]
                        );
                }
            }
        }
        for(int i = 0; i < S_ENC; i++)qs_gf197_mul_constraint(state, &inv_inputs[i], &inv_outputs[i], &pow_outputs[i]);
        qs_gf197_dot_constraint(state, &inv_outputs[0], &owf_alpha0);
        qs_gf197_dot_constraint(state, &inv_outputs[1], &owf_alpha1);

#elif SECURITY_PARAM == 256

        qs_gf263_bits inv_inputs[S_ENC];
        qs_gf263_bits inv_outputs[S_ENC];
        qs_gf263_bits pow_outputs[S_ENC];

        qs_gf263_bits sk_bits;

        for (size_t bit_j = 0; bit_j < 263; ++bit_j) {
            sk_bits.bit[bit_j] =
                quicksilver_get_witness_vec(state, bit_j);

            inv_outputs[0].bit[bit_j] =
                quicksilver_get_witness_vec(state, bit_j + 264);
        }

        qs_gf263_bits rc0_bits, rc1_bits, owf_iv_bits;

        load_rc263_bits(state, &rc0_bits, greatwall_rc_263[0]);
        load_rc263_bits(state, &rc1_bits, greatwall_rc_263[1]);
        load_rc263_bits(state, &owf_iv_bits, owf_iv.data);

        qs_gf263_bits matrix0_output, matrix1_output, matrix2_output;


        for (size_t matrow = 0; matrow < 263; matrow++) {
            matrix0_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 263; matcol++) {
                uint8_t bit =
                    mat263_get_bit(greatwall_mat_263_0, matrow, matcol);

                if (bit) {
                    matrix0_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix0_output.bit[matrow],
                            sk_bits.bit[matcol]
                        );
                }
            }
        }


        for (size_t i = 0; i < 263; ++i) {
            inv_inputs[0].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix0_output.bit[i],
                    rc0_bits.bit[i]
                );
        }


        for (size_t matrow = 0; matrow < 263; matrow++) {
            pow_outputs[0].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 263; matcol++) {
                uint8_t bit =
                    mat263_get_bit(greatwall_pow_mat_263_129,
                                   matrow,
                                   matcol);

                if (bit) {
                    pow_outputs[0].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[0].bit[matrow],
                            inv_outputs[0].bit[matcol]
                        );
                }
            }
        }


        qs_gf263_bits tmp;

        for (size_t i = 0; i < 263; ++i) {
            tmp.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_outputs[0].bit[i],
                    sk_bits.bit[i]
                );
        }


        for (size_t matrow = 0; matrow < 263; matrow++) {
            matrix1_output.bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 263; matcol++) {
                uint8_t bit =
                    mat263_get_bit(greatwall_mat_263_1, matrow, matcol);

                if (bit) {
                    matrix1_output.bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            matrix1_output.bit[matrow],
                            tmp.bit[matcol]
                        );
                }
            }
        }


        for (size_t i = 0; i < 263; ++i) {
            inv_inputs[1].bit[i] =
                quicksilver_add_gf2(
                    state,
                    matrix1_output.bit[i],
                    owf_iv_bits.bit[i]
                );
        }


        qs_gf263_bits out_bits;

        for (size_t i = 0; i < 263; ++i) {
            uint8_t b = (out.data[i >> 6] >> (i & 63)) & 1;

            out_bits.bit[i] = b
                ? quicksilver_one_gf2(state)
                : quicksilver_zero_gf2(state);
        }

        for (size_t i = 0; i < 263; ++i) {
            matrix2_output.bit[i] =
                quicksilver_add_gf2(
                    state,
                    inv_inputs[1].bit[i],
                    quicksilver_add_gf2(
                        state,
                        rc1_bits.bit[i],
                        quicksilver_add_gf2(
                            state,
                            sk_bits.bit[i],
                            out_bits.bit[i]
                        )
                    )
                );
        }


        for (size_t matrow = 0; matrow < 263; matrow++) {
            inv_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 263; matcol++) {
                uint8_t bit =
                    mat263_get_bit(greatwall_mat_263_2_inv,
                                   matrow,
                                   matcol);

                if (bit) {
                    inv_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            inv_outputs[1].bit[matrow],
                            matrix2_output.bit[matcol]
                        );
                }
            }
        }


        for (size_t matrow = 0; matrow < 263; matrow++) {
            pow_outputs[1].bit[matrow] = quicksilver_zero_gf2();

            for (size_t matcol = 0; matcol < 263; matcol++) {
                uint8_t bit =
                    mat263_get_bit(greatwall_pow_mat_263_136,
                                   matrow,
                                   matcol);

                if (bit) {
                    pow_outputs[1].bit[matrow] =
                        quicksilver_add_gf2(
                            state,
                            pow_outputs[1].bit[matrow],
                            inv_outputs[1].bit[matcol]
                        );
                }
            }
        }

        for (int i = 0; i < S_ENC; i++) {
            qs_gf263_mul_constraint(
                state,
                &inv_inputs[i],
                &inv_outputs[i],
                &pow_outputs[i]
            );
        }
        qs_gf263_dot_constraint(state, &inv_outputs[0], &owf_alpha0);
        qs_gf263_dot_constraint(state, &inv_outputs[1], &owf_alpha1);

#elif SECURITY_PARAM == 512
        qs_gf521_bits inv_inputs[S_ENC], inv_outputs[S_ENC], pow_outputs[S_ENC];
        qs_gf521_bits sk_bits, rc0, rc1, owf_iv_bits, tmp, matout, public_out;

        for(size_t i=0;i<521;i++) {
            sk_bits.bit[i]=quicksilver_get_witness_vec(state,i);
            inv_outputs[0].bit[i]=quicksilver_get_witness_vec(state,i+528);
            public_out.bit[i]=((out.data[i>>6]>>(i&63))&1)
                ? quicksilver_one_gf2(state) : quicksilver_zero_gf2();
        }
        load_rc521_bits(state,&rc0,greatwall_rc_521[0]);
        load_rc521_bits(state,&rc1,greatwall_rc_521[1]);
        load_rc521_bits(state,&owf_iv_bits,owf_iv.data);

        gf521_matmul(state,&matout,greatwall_mat_521_0,&sk_bits);
        gf521_add(state,&inv_inputs[0],&matout,&rc0);
        gf521_matmul(state,&pow_outputs[0],greatwall_pow_mat_521_248,&inv_outputs[0]);

        gf521_add(state,&tmp,&inv_outputs[0],&sk_bits);
        gf521_matmul(state,&matout,greatwall_mat_521_1,&tmp);
        gf521_add(state,&inv_inputs[1],&matout,&owf_iv_bits);

        gf521_add(state,&tmp,&inv_inputs[1],&rc1);
        gf521_add(state,&matout,&tmp,&sk_bits);
        gf521_add(state,&tmp,&matout,&public_out);
        gf521_matmul(state,&inv_outputs[1],greatwall_mat_521_2_inv,&tmp);
        gf521_matmul(state,&pow_outputs[1],greatwall_pow_mat_521_273,&inv_outputs[1]);

        for(int i=0;i<S_ENC;i++) qs_gf521_mul_constraint(state,&inv_inputs[i],&inv_outputs[i],&pow_outputs[i]);
        qs_gf521_dot_constraint(state, &inv_outputs[0], &owf_alpha0);
        qs_gf521_dot_constraint(state, &inv_outputs[1], &owf_alpha1);
#endif










    
}

static ALWAYS_INLINE void owf_constraints(quicksilver_state* state, const public_key* pk)
{
    enc_constraints(state, pk->owf_output, pk->owf_iv);
}

void owf_constraints_prover(quicksilver_state* state, const public_key* pk)
{
	assert(!state->verifier);
	state->verifier = false;
	owf_constraints(state, pk);
}

void owf_constraints_verifier(quicksilver_state* state, const public_key* pk)
{
	assert(state->verifier);
	state->verifier = true;
	owf_constraints(state, pk);
}


