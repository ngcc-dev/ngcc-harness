#include "vdoo_keypair.h"

#include "parallel_matrix_op.h"
#include "rng.h"

static void invert_lower_triangular(uint8_t *invL, const uint8_t *L, int n)
{
    int nb_tri = n * (n + 1) / 2;
    memset(invL, 0, nb_tri);

    for (int i = 0; i < n; i++) {
        int diag_idx = i * (i + 1) / 2 + i;
        invL[diag_idx] = gfv_inv(L[diag_idx]);
    }

    // Forward substitution
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < i; j++)
        {
            int inv_idx = i * (i + 1) / 2 + j;
            uint8_t sum = 0;
            for (int k = j; k < i; k++)
            {
                int l_idx = i * (i + 1) / 2 + k;
                int invk_idx = k * (k + 1) / 2 + j;
                sum ^= gfv_mul(L[l_idx], invL[invk_idx]);
            }
            invL[inv_idx] = gfv_mul(sum, invL[i * (i + 1) / 2 + i]);
        }
    }
}

static void invert_upper_triangular(uint8_t *invU, const uint8_t *U, int n)
{
    int nb_tri = n * (n + 1) / 2;
    memset(invU, 0, nb_tri);

    for (int i = 0; i < n; i++) {
        int diag_idx = i * (2 * n - i + 1) / 2;
        invU[diag_idx] = gfv_inv(U[diag_idx]);
    }

    // Backward substitution
    for (int i = n - 1; i >= 0; i--)
    {
        for (int j = i + 1; j < n; j++)
        {
            int inv_idx = i * (2 * n - i + 1) / 2 + (j - i);
            uint8_t sum = 0;
            for (int k = i + 1; k <= j; k++)
            {
                int u_idx = i * (2 * n - i + 1) / 2 + (k - i);
                int invk_idx = k * (2 * n - k + 1) / 2 + (j - k);
                sum ^= gfv_mul(U[u_idx], invU[invk_idx]);
            }
            invU[inv_idx] = gfv_mul(sum, invU[i * (2 * n - i + 1) / 2]);
        }
    }
}

static void multiply_invU_invL(unsigned char *invM, const uint8_t *U, const uint8_t *L, int n, int n_byte)
{
    memset(invM, 0, n * n_byte);

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            uint8_t sum = 0;
            // k = min(i,j)
            int kmin = (i > j) ? i : j;
            for (int k = kmin; k < n; k++)
            {
                int u_idx = i * (2 * n - i + 1) / 2 + (k - i);
                int l_idx = k * (k + 1) / 2 + j;

                sum ^= gfv_mul(U[u_idx], L[l_idx]);
            }
            gfv_set_ele(invM + j * n_byte, i, sum);
        }
    }
}

static void multiply_L_U(unsigned char *M, const uint8_t *L, const uint8_t *U, int n, int n_byte)
{
    memset(M, 0, n * n_byte);

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            uint8_t sum = 0;
            // k = min(i,j)
            int kmax = (i < j) ? i : j;
            for (int k = 0; k <= kmax; k++)
            {
                int l_idx = i * (i + 1) / 2 + k;
                int u_idx = k * (2 * n - k + 1) / 2 + (j - k);

                sum ^= gfv_mul(L[l_idx], U[u_idx]);
            }
            gfv_set_ele(M + j * n_byte, i, sum);
        }
    }
}

static void generate_M_invM(unsigned char *M, unsigned char *invM, int n, int n_byte)
{
    int nb_tri = SUM_K(n);

    unsigned char L[nb_tri];
    unsigned char U[nb_tri];

    // L (lower triangular matrix with diagonal elements != 0)
    //     [a 0 0]
    // L = [b c 0]
    //     [d e f]
    // stocked as an array: L = [a, b, c, d, e, f]
    int idx = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= i; j++) {
            uint8_t val;
            do
            {
                get_randombytes(&val, 1);
                if (VDOO_Q == 16)
                    val &= 0x0f;
            } while (i == j && val == 0);

            L[idx++] = val;
        }
    }

    // U (upper triangular matriw with diagonal elements != 0)
    //     [a' b' c']
    // U = [0  d' e']
    //     [0  0  f']
    // stocked as an array: U = [a', b', c', d', e', f']
    idx = 0;
    for (int i = 0; i < n; i++)
    {
        for (int j = i; j < n; j++)
        {
            uint8_t val;
            do
            {
                get_randombytes(&val, 1);
                if (VDOO_Q == 16)
                    val &= 0x0f;
            } while (i == j && val == 0);

            U[idx++] = val;
        }
    }

    // M = L * U (invertible matrix thanks to L and U structure)
    // Stocked in column-major, to use with batch operations :
    //     [x00 x01 x02]
    // M = [x10 x11 x12]
    //     [x20 x21 x22]
    // M = [x00, x10, x20, x01, x11, x21, x02, x12, x22]
    multiply_L_U(M, L, U, n, n_byte);

    // invL and invU
    unsigned char invL[nb_tri];
    unsigned char invU[nb_tri];

    invert_lower_triangular(invL, L, n);
    invert_upper_triangular(invU, U, n);

    // invM = invU * invL
    // stocked in column-major
    multiply_invU_invL(invM, invU, invL, n, n_byte);
}

static int is_allowed_diag(int poly, int i, int j)
{
    int diag_var_idx = VDOO_V1 + poly;

    // Vinegar × Vinegar
    if (i < diag_var_idx && j < diag_var_idx)
        return 1;

    // Vinegar × Diagonal
    if (i < diag_var_idx && j == diag_var_idx)
        return 1;

    return 0;
}

static int is_allowed_oil1(int poly, int i, int j)
{
    int oil_var_idx = VDOO_V2 + poly;
    int vinegar_max = VDOO_V2;

    // Vinegar × Vinegar
    if (i < vinegar_max && j < vinegar_max)
        return 1;

    // Vinegar × Oil
    if (i < vinegar_max && j == oil_var_idx)
        return 1;

    return 0;
}

static int is_allowed_oil2(int poly, int i, int j)
{
    int oil_var_idx = VDOO_V3 + poly;
    int vinegar_max = VDOO_V3;

    // Vinegar × Vinegar
    if (i < vinegar_max && j < vinegar_max)
        return 1;

    // Vinegar × Oil
    if (i < vinegar_max && j == oil_var_idx)
        return 1;

    return 0;
}

static int is_term_allowed(int poly, int i, int j)
{
    if (poly < VDOO_D)
        return is_allowed_diag(poly, i, j);
    if (poly < VDOO_D + VDOO_O1)
        return is_allowed_oil1(poly - VDOO_D, i, j);
    if (poly < VDOO_D + VDOO_O1 + VDOO_O2)
        return is_allowed_oil2(poly - VDOO_D - VDOO_O1, i, j);

    return -1;
}

static void set_coeff_to_zero(unsigned char *F, int poly, int monom_idx, int m)
{
    int coeff_idx = monom_idx * m + poly;
    gfv_set_ele(F, coeff_idx, 0);
}

static void set_zero_forbidden_terms(unsigned char *F)
{
    for (int poly = 0; poly < VDOO_M; poly++)
    {
        for (int i = 0; i < VDOO_N; i++)
        {
            for (int j = i; j < VDOO_N; j++)
            {
                int monom_idx = i * VDOO_N - i*(i-1)/2 + j - i;
                if (!is_term_allowed(poly, i, j))
                    set_coeff_to_zero(F, poly, monom_idx, VDOO_M);
            }
        }
    }
}

static void generate_central_map(sk_t *sk)
{
    get_randombytes(sk->F, PK_BYTES);
    set_zero_forbidden_terms(sk->F);
}


static void generate_private_key(sk_t *sk, const unsigned char *sk_seed, unsigned char *S, unsigned char *T)
{
    memcpy(sk->sk_seed, sk_seed, LEN_SKSEED);

    // S and invS
    unsigned char invS[VDOO_M_BYTE * VDOO_M];
    generate_M_invM(S, invS, VDOO_M, VDOO_M_BYTE);

    // T and invT
    unsigned char invT[VDOO_N_BYTE * VDOO_N];
    generate_M_invM(T, invT, VDOO_N, VDOO_N_BYTE);

    // Only invS and invT are stocked, as T and S are only use to generate the central map
    memcpy(sk->invS, invS, VDOO_M_BYTE * VDOO_M);
    memcpy(sk->invT, invT, VDOO_N_BYTE * VDOO_N);

    generate_central_map(sk);

    memset(invS, 0, VDOO_M_BYTE * VDOO_M);
    memset(invT, 0, VDOO_N_BYTE * VDOO_N);
}

// each polynomial can be written such as:
// f_k(T*w) = (T*w)^T A_k (T*w)
//        = (w^T * T^T) A_k (T * w)
//        = w^T (T^T * A_k * T) w
// Thus, we need to compute A_k' = (T^T * A_k * T) for each polynomial in
// order to find A' the matrix with the coefficients of the monomials of F(T*w).
// We use batch matrix functions to compute A_k' for every polynomial at the same time.
void apply_T_to_F(unsigned char *FT, const unsigned char *F,
                          const unsigned char *T)
{
    static unsigned char tmp[VDOO_N * VDOO_N * VDOO_M_BYTE];
    static unsigned char A_prime[VDOO_N * VDOO_N * VDOO_M_BYTE];

    memset(tmp, 0, sizeof(tmp));
    memset(A_prime, 0, sizeof(A_prime));

    // tmp = F * T
    batch_trimat_madd(tmp, F, T, VDOO_N, VDOO_N_BYTE, VDOO_N, VDOO_M_BYTE);
    // A' = T^T * tmp
    batch_matTr_madd(A_prime, T, VDOO_N, VDOO_N_BYTE, VDOO_N, tmp, VDOO_N, VDOO_M_BYTE);

    // "Trianglize" the obtained matrix A'
    static unsigned char A_prime_tri[VDOO_M_BYTE * SUM_K(VDOO_N)];
    memset(A_prime_tri, 0, sizeof(A_prime_tri));
    UpperTrianglize(A_prime_tri, A_prime, VDOO_N, VDOO_M_BYTE);
    memcpy(FT, A_prime_tri, VDOO_M_BYTE * SUM_K(VDOO_N));

    memset(tmp, 0, VDOO_N * VDOO_N * VDOO_M_BYTE);
    memset(A_prime, 0, VDOO_N * VDOO_N * VDOO_M_BYTE);
}

// TODO Create batch function to speed up the ocmputation
static void apply_S_to_FT(unsigned char *P, const unsigned char *FT, const unsigned char *S)
{
    memset(P, 0, PK_BYTES);
    batch_S_FT_madd(P, S, FT, VDOO_M, VDOO_M_BYTE, SUM_K(VDOO_N));
}

static void generate_public_key(pk_t *pk, const sk_t *sk, unsigned char *S, unsigned char *T)
{
    static unsigned char FT[PK_BYTES];

    // FT = F ° T
    // FT w = F(T w)
    apply_T_to_F(FT, sk->F, T);

    // P = S ° FT (= S ° F ° T)
    // P w = S(FT w)
    apply_S_to_FT(pk->pk, FT, S);

    memset(FT, 0, PK_BYTES);
}

int generate_keypair(pk_t *pk, sk_t *sk, const unsigned char *sk_seed)
{
    if (!pk || !sk || !sk_seed)
        return -1;

    unsigned char S[VDOO_M_BYTE * VDOO_M];
    unsigned char T[VDOO_N_BYTE * VDOO_N];

    generate_private_key(sk, sk_seed, S, T);
    generate_public_key(pk, sk, S, T);

    memset(S, 0, VDOO_M_BYTE * VDOO_M);
    memset(T, 0, VDOO_N_BYTE * VDOO_N);

    return 0;
}