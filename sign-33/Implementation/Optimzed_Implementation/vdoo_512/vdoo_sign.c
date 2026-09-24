#include "vdoo_sign.h"
#include "vdoo_keypair.h"
#include "gf_config.h"
#include "utils.h"
#include <string.h>

static int solve_diagonal_eq(uint8_t *y, int diag_eq_idx,
                              const uint8_t *t, const sk_t *sk)
{
    int i = diag_eq_idx;
    int diag_var_idx = VDOO_V1 + i;

    uint8_t A_coeff = 0;
    uint8_t B_const = 0;

    // VD, linear terms (A)
    for (int j = 0; j < diag_var_idx; j++)
    {
        // Index of the monomial t_j * t_diag_var_idx
        int monom_idx = j * VDOO_N + diag_var_idx - j * (j + 1) / 2;
        uint8_t coeff = gfv_get_ele(sk->F, monom_idx * VDOO_M + i);
        if (coeff != 0)
            A_coeff ^= gfv_mul(coeff, gfv_get_ele(y, j));
    }

    // VV, constant terms (B)
    for (int j = 0; j < diag_var_idx; j++)
    {
        for (int k = j; k < diag_var_idx; k++)
        {
            // Index of the monomial t_j * t_k
            int monom_idx = j * VDOO_N + k - j * (j + 1) / 2;
            uint8_t coeff = gfv_get_ele(sk->F, monom_idx * VDOO_M + i);
            if (coeff != 0)
                B_const ^= gfv_mul(coeff, gfv_mul(gfv_get_ele(y, j), gfv_get_ele(y, k)));
        }
    }

    // A * t_diag + B = y_i
    uint8_t yi = gfv_get_ele(t, i);
    // if A = 0, B = y_i
    if (A_coeff == 0)
    {
        // if B != y_i, it fails, we retry
        if (B_const != yi)
            return -1;

        // if B = y_i, t_diag can have a random value
        uint8_t val;

        get_randombytes(&val, 1);
        if (VDOO_Q == 16)
            val &= 0x0f;

        gfv_set_ele(y, diag_var_idx, val);
    }
    else
        // If A =/= 0, t_diag = (yi - B) * A⁻¹
        // In GF(2^l), substraction <=> addition <=> XOR
        // So, t_diag = (yi ^ B) * A⁻¹
        gfv_set_ele(y, diag_var_idx, gfv_mul(yi ^ B_const, gfv_inv(A_coeff)));

    return 0;
}

static void substitute_fixed_vars(uint8_t *A_oil, uint8_t *b_oil,
                                   const uint8_t *y, const uint8_t *t,
                                   int eq_start, int oil_start,
                                   int n_oil, int n_fixed, const sk_t *sk)
{
    for (int j = 0; j < n_oil; j++)
    {
        int eq = eq_start + j;

        // linear coeffs, construction of the line j of the matrix A
        // XOR for each k of coeff(i, oil_k) * x_i*x_oil_k
        //                 = coeff(i, oil_k) * y[i]*x_oil_k (y[i] known)
        for (int k = 0; k < n_oil; k++)
        {
            int oil_k = oil_start + k;
            uint8_t coeff = 0;
            for (int i = 0; i < n_fixed; i++)
            {
                int monom_idx = i * VDOO_N + oil_k - i * (i + 1) / 2;
                uint8_t c = gfv_get_ele(sk->F, monom_idx * VDOO_M + eq);
                if (c != 0)
                    coeff ^= gfv_mul(c, gfv_get_ele(y, i));
            }
            A_oil[j * n_oil + k] = coeff;
        }

        // constant term b[j]
        uint8_t constant = 0;
        for (int i = 0; i < n_fixed; i++)
        {
            for (int k = i; k < n_fixed; k++)
            {
                // XOR for each k of coeff(i, k) * x_i*x_k
                //                 = coeff(i, k) * y[i]*y[k] (y[i] and y[ k] known)
                int monom_idx = i * VDOO_N + k - i * (i + 1) / 2;
                uint8_t c = gfv_get_ele(sk->F, monom_idx * VDOO_M + eq);
                if (c != 0)
                    constant ^= gfv_mul(c,
                                    gfv_mul(gfv_get_ele(y, i),
                                            gfv_get_ele(y, k)));
            }
        }
        // b[i] = t[i] - C = t[i] + C = t[i] ^ C, in GF(2^l)
        b_oil[j] = gfv_get_ele(t, eq) ^ constant;
    }
}

static int gauss_elimination(uint8_t *y, int y_offset,
                             uint8_t *A, uint8_t *b, int n_oil)
{
    // Construction the augmented matrix M = [ A | b ]
    uint8_t M[n_oil][n_oil + 1];

    for (int row = 0; row < n_oil; row++)
    {
        for (int col = 0; col < n_oil; col++)
            M[row][col] = A[row * n_oil + col];

        M[row][n_oil] = b[row];
    }

    // Elimination
    for (int i = 0; i < n_oil; i++)
    {
        // Find pivot
        if (M[i][i] == 0)
        {
            for (int j = i + 1; j < n_oil; j++)
            {
                for (int k = i; k <= n_oil; k++)
                    // Add if pivot == 0, with a mask to be constant-time
                        M[i][k] ^= M[j][k];
            }
        }

        // Verifiy that the matrix is inversible
        if (M[i][i] == 0)
            return -1;

        // normalize the pivot line
        uint8_t inv = gfv_inv(M[i][i]);
        for (int k = i; k <= n_oil; k++)
            M[i][k] = gfv_mul(M[i][k], inv);

        // Eliminate the column i in all the other lines
        for (int j = 0; j < n_oil; j++)
        {
            if (j != i)
            {
                uint8_t factor = M[j][i];
                for (int k = i; k <= n_oil; k++)
                    M[j][k] ^= gfv_mul(factor, M[i][k]);
            }
        }
    }

    // Extract the solution The last column of M
    for (int i = 0; i < n_oil; i++)
        gfv_set_ele(y, y_offset + i, M[i][n_oil]);

    return 0;
}

// VDOOCentPoly_Inversion
static int vdoo_inverse_F(uint8_t *y, const sk_t *sk, const uint8_t *t)
{
    uint8_t A1[VDOO_O1 * VDOO_O1];
    uint8_t b1[VDOO_O1];
    uint8_t A2[VDOO_O2 * VDOO_O2];
    uint8_t b2[VDOO_O2];

    int max_attemps = 100;
    for (int attempt = 0; attempt < max_attemps; attempt++)
    {
        memset(y, 0, VDOO_N_BYTE);
        for (int i = 0; i < VDOO_V1; i++)
        {
            uint8_t val;
            get_randombytes(&val, 1);
            if (VDOO_Q == 16) val
                &= 0x0f;
            gfv_set_ele(y, i, val);
        }

        // Resolution of the diagonal layer
        int ok = 1;
        for (int i = 0; i < VDOO_D && ok; i++)
            if (solve_diagonal_eq(y, i, t, sk) != 0)
                ok = 0;
        if (!ok)
            continue;


        memset(A1, 0, sizeof(A1));
        memset(b1, 0, sizeof(b1));

        // Substitution
        substitute_fixed_vars(A1, b1, y, t,
                              VDOO_D, VDOO_V2, VDOO_O1, VDOO_V2, sk);

        // Gaussian Elimination
        if (gauss_elimination(y, VDOO_V2, A1, b1, VDOO_O1) != 0)
            continue;


        memset(A2, 0, sizeof(A2));
        memset(b2, 0, sizeof(b2));

        // Substitution
        substitute_fixed_vars(A2, b2, y, t,
                              VDOO_D + VDOO_O1, VDOO_V3, VDOO_O2, VDOO_V3, sk);

        // Gaussian Elimination
        if (gauss_elimination(y, VDOO_V3, A2, b2, VDOO_O2) != 0)
            continue;

        return 0;
    }
    return -1;
}

// t = invS * d
static void apply_invS(uint8_t *t, const unsigned char *invS, const uint8_t *d)
{
    memset(t, 0, VDOO_M_BYTE);

    for (int row = 0; row < VDOO_M; row++)
    {
        uint8_t sum = 0;
        for (int col = 0; col < VDOO_M; col++)
        {
            uint8_t invs_rc = gfv_get_ele(invS + col * VDOO_M_BYTE, row);
            uint8_t d_c = gfv_get_ele(d, col);
            sum ^= gfv_mul(invs_rc, d_c);
        }
        gfv_set_ele(t, row, sum);
    }
}

// s = invT * y
static void apply_invT(uint8_t *s, const unsigned char *invT, const uint8_t *y)
{
    memset(s, 0, VDOO_N_BYTE);

    for (int i = 0; i < VDOO_N; i++)
    {
        uint8_t sum = 0;
        for (int j = 0; j < VDOO_N; j++)
        {
            uint8_t invt_ij = gfv_get_ele(invT + j * VDOO_N_BYTE, i);
            uint8_t y_j = gfv_get_ele(y, j);
            sum ^= gfv_mul(invt_ij, y_j);
        }
        gfv_set_ele(s, i, sum);
    }
}

int vdoo_sign(uint8_t *signature, const sk_t *sk, const uint8_t *_digest)
{
    uint8_t salt[SALT_BYTES];
    uint8_t digest_salt[HASH_LEN + SALT_BYTES];

    uint8_t d[VDOO_M_BYTE];
    uint8_t t[VDOO_M_BYTE];

    uint8_t y[VDOO_N_BYTE];
    uint8_t s[VDOO_N_BYTE];

    int max_attempts = 100;
    for (int attempt = 0; attempt < max_attempts; attempt++)
    {
        get_randombytes(salt, SALT_BYTES);

        memcpy(digest_salt, _digest, HASH_LEN);
        memcpy(digest_salt + HASH_LEN, salt, SALT_BYTES);
        hash_msg(d, VDOO_M_BYTE, digest_salt, HASH_LEN + SALT_BYTES);

        // t = invS * d
        apply_invS(t, sk->invS, d);

        // y = F⁻¹(t)
        if (vdoo_inverse_F(y, sk, t) != 0)
            continue;

        // s = invT * y
        apply_invT(s, sk->invT, y);

        // make signature (s, salt)
        memcpy(signature, s, VDOO_N_BYTE);
        memcpy(signature + VDOO_N_BYTE, salt, SALT_BYTES);

        return 0;
    }

    return -1;
}

