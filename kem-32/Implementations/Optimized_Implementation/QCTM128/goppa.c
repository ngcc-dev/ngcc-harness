#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "goppa.h"
#include "scheme_api.h"
#include "qctm_portable.h"

static int qctm_profile_keygen_active = 0;

static int qctm_profile_keygen_enabled(void)
{
    const char *profile = getenv("QCTM_PROFILE_KEYGEN");

    return profile != NULL && profile[0] != '\0' && profile[0] != '0';
}

static double qctm_profile_now_ms(void)
{
    return qctm_now_ms();
}




goppa_t goppa_init(gfelt_t * L, poly_t g, int length, int degree,
                   int order, poly_t sqrtzmod, gf_t eta) {
    int i;
    goppa_t gamma = malloc(sizeof (struct goppa));

    gamma->length = length;
    gamma->degree = degree;
    gamma->order = order;
    gamma->L = L;
    gamma->g = g;
    gamma->explicit_syndrome_input = 0;
    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(gamma->eta, eta);
    } else {
        gf_set_to_unit(gamma->eta);
    }
    gamma->Linv = (gfindex_t *) calloc(gf_card(), sizeof (gfindex_t));
    for (i = 0; i < length; ++i)
        gamma->Linv[gf_to_index(L + i)] = i;
    gamma->sqrtzmod = sqrtzmod;
    
#ifndef SQRT_NO_PRECOMP
    gamma->sqrtmod = NULL;
#endif
#ifndef PARITY_NO_PRECOMP
    gamma->parity = NULL;
#endif
    
    return gamma;
}



void free_goppa(goppa_t gamma) {
    if (gamma->L)
        free(gamma->L);
    if (gamma->g)
        poly_free(gamma->g);
    if (gamma->Linv)
        free(gamma->Linv);
    if (gamma->sqrtzmod)
        poly_free(gamma->sqrtzmod);
#ifndef SQRT_NO_PRECOMP
    if (gamma->sqrtmod) {     
        for (int i = 0; i < gamma->degree / 2; ++i)
            poly_free(gamma->sqrtmod[i]);
        free(gamma->sqrtmod);
    }
#endif
#ifndef PARITY_NO_PRECOMP
    if (gamma->parity) {
        free(gamma->parity[0]);
        free(gamma->parity);
    }
#endif
    free(gamma);    
}


int goppa_fixed_eta(int l, gf_t eta)
{
    (void)l;

    if (eta == NULL) {
        return 0;
    }

    do {
        gf_set_rand(eta);
    } while (gf_is_zero(eta));

    return 1;
}


static int binomial_odd(unsigned n, unsigned k) {
    return (k & ~n) == 0;
}

static void gf_pow_nonnegative(gf_t out, gf_t x, int e)
{
    int i;

    gf_set_to_unit(out);
    for (i = 0; i < e; i++) {
        gf_mul(out, out, x);
    }
}

static poly_t x_plus_c_pow_l(int l, gf_t c) {
    int i;
    gf_t coeff;
    poly_t p;

    if (l <= 0) {
        return NULL;
    }
    p = poly_alloc(l);
    if (p == NULL) {
        return NULL;
    }

    for (i = 0; i <= l; i++) {
        if (binomial_odd((unsigned)l, (unsigned)i)) {
            gf_pow_nonnegative(coeff, c, l - i);
            poly_set_coeff(p, i, coeff);
        }
    }
    poly_calcule_deg(p);
    return p;
}



gfelt_t * goppa_rand_support(int n, int l, gf_t eta) {

    int i, j, k, s, nbr_o = n/l;
    gf_t a;
    gf_t shift;
    gfelt_t *L;
    int tab[gf_card()-1];
    index_t * perm_orb = malloc(nbr_o*sizeof(index_t));
    
    if (eta == NULL || gf_is_zero(eta)) {
        free(perm_orb);
        return NULL;
    }

    L = malloc(n * sizeof(gfelt_t));
    
    for (i = 0; i < (gf_card()-1)/l; i++){
        for (k = 0; k < l; k++){
            tab[i*l+k] = (i + k*(gf_card()-1)/l);
        }
    }
    
    generate_permutation(perm_orb, (index_t) nbr_o);
    gf_inv(shift, eta);
    
    for (k = 0; k < nbr_o; k++) {
        i = (int) perm_orb[k];
        randindex_init(l);
        j = (int) uniform((index_t)l);

        for(s = 0; s < l; s++) {
            a[0] = gf_exp[tab[i*l + ((j + s)%l)]];
            gf_add(a, a, shift);
            gf_set(L + k*l + s, a);
        }
    }
    
    free(perm_orb);
    
    return L;
}


void xor_long(unsigned long * a, unsigned long * b, int n) {
    int i;
    for (i = 0; i < n; ++i)
	a[i] ^= b[i];
}

static void matrix_xor_gf_bits(unsigned long **M, int row_base, int col,
                               gfindex_t x)
{
    unsigned long mask = 1UL << (col % __WORDSIZE);
    unsigned long word = (unsigned long)x;
    int col_word = col / __WORDSIZE;

    while (word != 0UL) {
        int k = __builtin_ctzl(word);

        M[row_base + k][col_word] ^= mask;
        word &= word - 1UL;
    }
}

static unsigned long bit_window_mask(int bits)
{
    return (1UL << bits) - 1UL;
}

static unsigned long row_load_bits(const unsigned long *row, int bit,
                                   int bits)
{
    int word = bit / __WORDSIZE;
    int off = bit % __WORDSIZE;
    unsigned long value = row[word] >> off;

    if (off + bits > __WORDSIZE) {
        value |= row[word + 1] << (__WORDSIZE - off);
    }
    return value & bit_window_mask(bits);
}

static void row_store_bits(unsigned long *row, int bit, int bits,
                           unsigned long value)
{
    int word = bit / __WORDSIZE;
    int off = bit % __WORDSIZE;
    unsigned long mask = bit_window_mask(bits);

    value &= mask;
    if (off + bits <= __WORDSIZE) {
        unsigned long shifted_mask = mask << off;

        row[word] = (row[word] & ~shifted_mask) | (value << off);
    } else {
        int low_bits = __WORDSIZE - off;
        int high_bits = bits - low_bits;
        unsigned long low_mask = ~0UL << off;
        unsigned long high_mask = bit_window_mask(high_bits);

        row[word] = (row[word] & ~low_mask) | (value << off);
        row[word + 1] = (row[word + 1] & ~high_mask) |
                        (value >> low_bits);
    }
}

static void row_rotate_l_blocks(unsigned long *dst, const unsigned long *src,
                                int n, int l, int rwdcnt)
{
    int block;
    unsigned long mask = bit_window_mask(l);

    memset(dst, 0, (size_t)rwdcnt * sizeof(*dst));
    for (block = 0; block < n; block += l) {
        unsigned long chunk = row_load_bits(src, block, l);

        chunk = ((chunk << 1) & mask) | (chunk >> (l - 1));
        row_store_bits(dst, block, l, chunk);
    }
}

static void eliminate_with_pivot_block(unsigned long **M, int row,
                                       int pivot_base, int pivot_col,
                                       int l, int rwdcnt)
{
    unsigned long bits = row_load_bits(M[row], pivot_col, l);

    while (bits != 0UL) {
        int s = __builtin_ctzl(bits);

        xor_long(M[row], M[pivot_base + s], rwdcnt);
        bits &= bits - 1UL;
    }
}



void swap_block_rows(unsigned long ** M, int i, int k, int l) {
    int u;
    for(u = 0; u < l; u++)
        swaprows(M, i + u, k + u);
}

static int scheme_trace_keygen_enabled(void)
{
    const char *trace = getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_KEYGEN");

    return trace != NULL && trace[0] != '\0' && trace[0] != '0';
}

static int scheme_selected_twist_row(int t)
{
    return t > 0 ? t - 1 : 0;
}

static int scheme_matgen_field_rows(int t)
{
    return t > 0 ? t - 1 : 0;
}

static int scheme_matgen_binary_rows(int m, int t)
{
    return m * scheme_matgen_field_rows(t);
}

static int scheme_matgen_qc_aligned(int n, int l, int r)
{
    return l > 0 && r > 0 && (r % l) == 0 && ((n - r) % l) == 0;
}

static void map_swap_columns(unsigned long **W, int rows, int a, int b)
{
    int i;
    unsigned long ma;
    unsigned long mb;

    if (W == NULL || a == b) {
        return;
    }
    ma = 1LU << (a % __WORDSIZE);
    mb = 1LU << (b % __WORDSIZE);
    for (i = 0; i < rows; i++) {
        int ba = coeff(W, i, a);
        int bb = coeff(W, i, b);
        if (ba != bb) {
            W[i][a / __WORDSIZE] ^= ma;
            W[i][b / __WORDSIZE] ^= mb;
        }
    }
}

static void map_add_dst_column_to_src(unsigned long **W, int rows,
                                      int dst, int src)
{
    int i;
    unsigned long ms;

    if (W == NULL || dst == src) {
        return;
    }
    ms = 1LU << (src % __WORDSIZE);
    for (i = 0; i < rows; i++) {
        if (coeff(W, i, dst)) {
            W[i][src / __WORDSIZE] ^= ms;
        }
    }
}

static void tracked_swaprows(unsigned long **M, int a, int b,
                             unsigned long **W, int map_rows)
{
    swaprows(M, a, b);
    map_swap_columns(W, map_rows, a, b);
}

static void tracked_addrowto(unsigned long **M, int src, int dst, int rwdcnt,
                             unsigned long **W, int map_rows)
{
    xor_long(M[dst], M[src], rwdcnt);
    map_add_dst_column_to_src(W, map_rows, dst, src);
}

static int gausselim_aux_tracked(unsigned long **M, int base, int r, int c,
                                 int ind_cols, unsigned long **W,
                                 int map_rows)
{
    int i, j, k, l, s;
    int rwdcnt = 1 + (c - 1) / __WORDSIZE;

    for (j = ind_cols, k = 0, s = 0; (k < r) && (j < c); ++j) {

        for (i = k; i < r; ++i) {
            if (coeff(M, base + i, j)) {
                tracked_swaprows(M, base + i, base + k, W, map_rows);
                break;
            }
        }
        if (i < r) {
            for (l = 0; l < k; ++l)
                if (coeff(M, base + l, j))
                    tracked_addrowto(M, base + k, base + l, rwdcnt, W, map_rows);
            for (l = i + 1; l < r; ++l)
                if (coeff(M, base + l, j))
                    tracked_addrowto(M, base + k, base + l, rwdcnt, W, map_rows);
            ++k;
        } else {
            ++s;
        }
    }

    return k;
}








int gausselim(unsigned long ** M, int r, int c) {
    int i, j, k, l, s;
    int rwdcnt = 1 + (c - 1) / __WORDSIZE;

    for (j = 0, k = 0, s = 0; (k < r) && (j < c); ++j) {
        int word = j / __WORDSIZE;
        unsigned long mask = 1UL << (j % __WORDSIZE);

        for (i = k; i < r; ++i) {
            if ((M[i][word] & mask) != 0UL) {
                swaprows(M, i, k);
                break;
            }
        }
        if (i < r) {
            for (l = 0; l < k; ++l)
                if ((M[l][word] & mask) != 0UL)
                    addrowto(M, k, l);
            for (l = i + 1; l < r; ++l)
                if ((M[l][word] & mask) != 0UL)
                    addrowto(M, k, l);
            ++k;
        } else {
            ++s;
        }
    }
    
    return k;
}


int gausselim_aux(unsigned long ** M, int r, int c, int ind_cols) {
    int i, j, k, l, s;
    int rwdcnt = 1 + (c - 1) / __WORDSIZE;

    for (j = ind_cols, k = 0, s = 0; (k < r) && (j < c); ++j) {
        int word = j / __WORDSIZE;
        unsigned long mask = 1UL << (j % __WORDSIZE);

        for (i = k; i < r; ++i) {
            if ((M[i][word] & mask) != 0UL) {
                swaprows(M, i, k);
                break;
            }
        }
        if (i < r) {
            for (l = 0; l < k; ++l)
                if ((M[l][word] & mask) != 0UL)
                    addrowto(M, k, l);
            for (l = i + 1; l < r; ++l)
                if ((M[l][word] & mask) != 0UL)
                    addrowto(M, k, l);
            ++k;
        } else {
            ++s;
        }
    }
    
    return k;
}



 
int rank_block(unsigned long ** M, int i, int j, int l) {

    int k, s, rk;
    unsigned long ** Bl;

    if (l > 0 && l <= __WORDSIZE) {
        unsigned long rows[__WORDSIZE];
        int rank = 0;
        int col;

        for (k = 0; k < l; k++) {
            rows[k] = row_load_bits(M[i + k], j, l);
        }
        for (col = 0; col < l && rank < l; col++) {
            unsigned long mask = 1UL << col;
            int pivot = -1;
            int row;

            for (row = rank; row < l; row++) {
                if ((rows[row] & mask) != 0UL) {
                    pivot = row;
                    break;
                }
            }
            if (pivot < 0) {
                continue;
            }
            if (pivot != rank) {
                unsigned long tmp = rows[pivot];

                rows[pivot] = rows[rank];
                rows[rank] = tmp;
            }
            for (row = rank + 1; row < l; row++) {
                if ((rows[row] & mask) != 0UL) {
                    rows[row] ^= rows[rank];
                }
            }
            rank++;
        }
        return rank;
    }

    Bl = malloc(l * sizeof (unsigned long *));
    for (k = 0; k < l; k++) {
        Bl[k] = calloc(1 + (l - 1) / __WORDSIZE, sizeof (unsigned long));
        for (s = 0; s < l; s++) {
            if (coeff(M, i + k, j + s)) {
                Bl[k][s / __WORDSIZE] ^= (1LU << (s % __WORDSIZE));
            }
        }
    }
    
    rk = gausselim(Bl, l, l);
    for (k = 0; k < l; k++)
        free(Bl[k]);
    free(Bl);
    
    return(rk);
}



int qc_systematic_form(unsigned long ** M, int r, int n, int l, int * perm) {
    
    int i, j, k, block_inv, ind_cols, rk, tmp;
    unsigned long ** M_bis;
    unsigned long *M_storage;
    size_t row_count;
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;
    int profile = qctm_profile_keygen_active;
    double t_start = 0.0;
    double t_after_alloc = 0.0;
    double t_after_expand = 0.0;
    double t_pivot = 0.0;
    double t_aux = 0.0;
    double t_elim = 0.0;
    double t_copy_free_start = 0.0;
    double t_end = 0.0;
    unsigned long long rank_calls = 0;
    unsigned long long pivot_blocks = 0;

    if (profile) {
        t_start = qctm_profile_now_ms();
    }
    
    row_count = (size_t)r * (size_t)l;
    M_bis = malloc(row_count * sizeof(*M_bis));
    M_storage = calloc(row_count * (size_t)rwdcnt, sizeof(*M_storage));
    if (M_bis == NULL || M_storage == NULL) {
        free(M_bis);
        free(M_storage);
        return 0;
    }
    for (i = 0; i < r*l; i++) {
        M_bis[i] = M_storage + (size_t)i * (size_t)rwdcnt;
    }
    if (profile) {
        t_after_alloc = qctm_profile_now_ms();
    }
    
    for (i = 0; i < r; i++) {
        memcpy(M_bis[i * l], M[i],
               (size_t)rwdcnt * sizeof(**M_bis));

        for (k = 0; k < l - 1; k++) {
            row_rotate_l_blocks(M_bis[i * l + k + 1],
                                M_bis[i * l + k], n, l, rwdcnt);
        }
    }
    if (profile) {
        t_after_expand = qctm_profile_now_ms();
    }
    
    
    for (i = 0; i < n; i++)
        perm[i] = i;

    ind_cols = 0;
    
    for (j = 0; j < r ; j += l) {

                
        block_inv = FALSE; 
        
        while (!block_inv && ind_cols < n) { 

                       
            rk = 0;
            i = j;
            if (profile) {
                double t0 = qctm_profile_now_ms();

                while (rk != l && i < r*l) {
                    rk = rank_block(M_bis, i, ind_cols, l);
                    rank_calls++;
                    i += l;
                }
                t_pivot += qctm_profile_now_ms() - t0;
            } else {
            while (rk != l && i < r*l) {
                rk = rank_block(M_bis, i, ind_cols, l);
                i += l;
            }
            }
            
            if (rk == l) {
                                
                block_inv = TRUE;
                pivot_blocks++;

                if (j != (i-l))
                    for (k = 0; k < l; k++)
                        swaprows(M_bis, i - l + k, j + k);

                if (profile) {
                    double t0 = qctm_profile_now_ms();

                    gausselim_aux(M_bis + j, l, n, ind_cols);
                    t_aux += qctm_profile_now_ms() - t0;
                } else {
                gausselim_aux(M_bis + j, l, n, ind_cols);
                }
                
                if (profile) {
                    double t0 = qctm_profile_now_ms();

                    for (k = 0; k < j; k++) {
                        eliminate_with_pivot_block(M_bis, k, j, ind_cols, l,
                                                   rwdcnt);
                    }

                    for (k = j + l; k < r*l; k++) {
                        eliminate_with_pivot_block(M_bis, k, j, ind_cols, l,
                                                   rwdcnt);
                    }
                    t_elim += qctm_profile_now_ms() - t0;
                } else {
                for (k = 0; k < j; k++) {
                    eliminate_with_pivot_block(M_bis, k, j, ind_cols, l,
                                               rwdcnt);
                }

                for (k = j + l; k < r*l; k++) {
                    eliminate_with_pivot_block(M_bis, k, j, ind_cols, l,
                                               rwdcnt);
                }
                }
                       
                
                if (ind_cols != j)
                    for (k = 0; k < l; k++) {
                        tmp = perm[j + k];
                        perm[j + k] = perm[ind_cols + k];
                        perm[ind_cols + k] = tmp;
                    }
            }
            ind_cols += l;
        }
    }
    if (profile) {
        t_copy_free_start = qctm_profile_now_ms();
    }
    
    for (i = 0; i < r; i++) {
        for (k = 0; k < rwdcnt; k++)
            M[i][k] = M_bis[i][k];
    }
    free(M_storage);
    free(M_bis);
    if (profile) {
        t_end = qctm_profile_now_ms();
        fprintf(stderr,
                "qctm_keygen_profile qc_systematic_ms alloc=%.3f expand=%.3f pivot=%.3f aux=%.3f elim=%.3f copy_free=%.3f total=%.3f rank_calls=%llu pivot_blocks=%llu\n",
                t_after_alloc - t_start,
                t_after_expand - t_after_alloc,
                t_pivot,
                t_aux,
                t_elim,
                t_end - t_copy_free_start,
                t_end - t_start,
                rank_calls,
                pivot_blocks);
        fflush(stderr);
    }
    

    if (ind_cols >= n){
        
        return 0;
    }

    return j;
}

static int qc_systematic_form_fixed_columns(unsigned long **M, int r, int n,
                                            int l)
{
    int i;
    int j;
    int k;
    int rk;
    unsigned long **M_bis;
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;

    M_bis = malloc((size_t)r * (size_t)l * sizeof(*M_bis));
    if (M_bis == NULL) {
        return 0;
    }
    for (i = 0; i < r * l; i++) {
        M_bis[i] = calloc((size_t)rwdcnt, sizeof(**M_bis));
        if (M_bis[i] == NULL) {
            while (--i >= 0) {
                free(M_bis[i]);
            }
            free(M_bis);
            return 0;
        }
    }

    for (i = 0; i < r; i++) {
        memcpy(M_bis[i * l], M[i],
               (size_t)rwdcnt * sizeof(**M_bis));
        for (k = 0; k < l - 1; k++) {
            row_rotate_l_blocks(M_bis[i * l + k + 1],
                                M_bis[i * l + k], n, l, rwdcnt);
        }
    }

    for (j = 0; j < r; j += l) {
        int pivot_block = -1;

        for (i = j; i < r * l; i += l) {
            rk = rank_block(M_bis, i, j, l);
            if (rk == l) {
                pivot_block = i;
                break;
            }
        }
        if (pivot_block < 0) {
            for (i = 0; i < r * l; i++) {
                free(M_bis[i]);
            }
            free(M_bis);
            return j;
        }
        if (pivot_block != j) {
            for (k = 0; k < l; k++) {
                swaprows(M_bis, pivot_block + k, j + k);
            }
        }

        gausselim_aux(M_bis + j, l, n, j);

        for (k = 0; k < j; k++) {
            eliminate_with_pivot_block(M_bis, k, j, j, l, rwdcnt);
        }
        for (k = j + l; k < r * l; k++) {
            eliminate_with_pivot_block(M_bis, k, j, j, l, rwdcnt);
        }
    }

    for (i = 0; i < r; i++) {
        for (k = 0; k < rwdcnt; k++) {
            M[i][k] = M_bis[i][k];
        }
    }
    for (i = 0; i < r * l; i++) {
        free(M_bis[i]);
    }
    free(M_bis);
    return r;
}

static void swapcols(unsigned long **M, int rows, int a, int b)
{
    int i;
    unsigned long ma;
    unsigned long mb;

    if (a == b) {
        return;
    }
    ma = 1LU << (a % __WORDSIZE);
    mb = 1LU << (b % __WORDSIZE);
    for (i = 0; i < rows; i++) {
        int ba = coeff(M, i, a);
        int bb = coeff(M, i, b);
        if (ba != bb) {
            M[i][a / __WORDSIZE] ^= ma;
            M[i][b / __WORDSIZE] ^= mb;
        }
    }
}

static int systematic_form(unsigned long **M, int r, int n, int *perm)
{
    int i, j, k, tmp;
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;

    for (i = 0; i < n; i++) {
        perm[i] = i;
    }

    for (k = 0; k < r; k++) {
        int pivot_row = -1;
        int pivot_col = -1;

        for (j = k; j < n && pivot_row < 0; j++) {
            for (i = k; i < r; i++) {
                if (coeff(M, i, j)) {
                    pivot_row = i;
                    pivot_col = j;
                    break;
                }
            }
        }
        if (pivot_row < 0) {
            return k;
        }

        if (pivot_col != k) {
            swapcols(M, r, k, pivot_col);
            tmp = perm[k];
            perm[k] = perm[pivot_col];
            perm[pivot_col] = tmp;
        }
        if (pivot_row != k) {
            swaprows(M, pivot_row, k);
        }

        for (i = 0; i < r; i++) {
            if (i != k && coeff(M, i, k)) {
                addrowto(M, k, i);
            }
        }
    }

    return r;
}

static int pivot_column(unsigned long **M, int r, int n, int col, int rank)
{
    int i;
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;

    for (i = rank; i < r; i++) {
        if (coeff(M, i, col)) {
            break;
        }
    }
    if (i == r) {
        return rank;
    }

    if (i != rank) {
        swaprows(M, i, rank);
    }
    for (i = 0; i < r; i++) {
        if (i != rank && coeff(M, i, col)) {
            addrowto(M, rank, i);
        }
    }

    return rank + 1;
}

static int block_systematic_form(unsigned long **M, int r, int n, int l,
                                 int *perm)
{
    int block;
    int i;
    int rank = 0;
    int selected_blocks = 0;
    int total_blocks;
    int rwdcnt;
    unsigned long *backup_data;
    unsigned long **backup;
    unsigned char *selected;
    int *order;
    const char *seed_text;
    char *end = NULL;
    unsigned long shuffle_state = 0;

    if (M == NULL || perm == NULL || l <= 0 || (r % l) != 0 ||
        (n % l) != 0) {
        return 0;
    }

    total_blocks = n / l;
    rwdcnt = 1 + (n - 1) / __WORDSIZE;
    backup_data = malloc((size_t)r * (size_t)rwdcnt * sizeof(unsigned long));
    backup = malloc((size_t)r * sizeof(*backup));
    selected = calloc((size_t)total_blocks, 1);
    order = malloc((size_t)total_blocks * sizeof(*order));
    if (backup_data == NULL || backup == NULL || selected == NULL ||
        order == NULL) {
        free(backup_data);
        free(backup);
        free(selected);
        free(order);
        return 0;
    }
    for (i = 0; i < r; i++) {
        backup[i] = backup_data + (size_t)i * (size_t)rwdcnt;
    }
    for (i = 0; i < total_blocks; i++) {
        order[i] = i;
    }

    seed_text = getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_BLOCK_ORDER_SEED");
    if (seed_text != NULL && seed_text[0] != '\0') {
        shuffle_state = strtoul(seed_text, &end, 10);
        if (end == seed_text) {
            shuffle_state = 0;
        }
    }
    if (shuffle_state != 0) {
        for (i = total_blocks - 1; i > 0; i--) {
            int j;
            int tmp;

            shuffle_state ^= shuffle_state << 13;
            shuffle_state ^= shuffle_state >> 7;
            shuffle_state ^= shuffle_state << 17;
            j = (int)(shuffle_state % (unsigned long)(i + 1));
            tmp = order[i];
            order[i] = order[j];
            order[j] = tmp;
        }
    }

    for (int order_pos = 0; order_pos < total_blocks && rank < r; order_pos++) {
        block = order[order_pos];
        int saved_rank = rank;
        int col;

        for (int row = 0; row < r; row++) {
            memcpy(backup[row], M[row],
                   (size_t)rwdcnt * sizeof(unsigned long));
        }

        for (col = block * l; col < (block + 1) * l; col++) {
            rank = pivot_column(M, r, n, col, rank);
        }

        if (rank == saved_rank + l) {
            selected[block] = 1;
            for (col = 0; col < l; col++) {
                perm[(size_t)selected_blocks * (size_t)l + (size_t)col] =
                    block * l + col;
            }
            selected_blocks++;
        } else {
            for (int row = 0; row < r; row++) {
                memcpy(M[row], backup[row],
                       (size_t)rwdcnt * sizeof(unsigned long));
            }
            rank = saved_rank;
        }
    }

    if (rank == r) {
        int out = r;

        for (block = 0; block < total_blocks; block++) {
            if (selected[block] == 0) {
                int col;

                for (col = 0; col < l; col++) {
                    perm[out++] = block * l + col;
                }
            }
        }
    }

    free(backup_data);
    free(backup);
    free(selected);
    free(order);
    return rank;
}

static int matrix_rank_copy(unsigned long **M, int rows, int cols)
{
    int i;
    int j;
    int rank;
    int rwdcnt = 1 + (cols - 1) / __WORDSIZE;
    unsigned long **A;

    A = malloc((size_t)rows * sizeof(*A));
    if (A == NULL) {
        return -1;
    }
    for (i = 0; i < rows; i++) {
        A[i] = malloc((size_t)rwdcnt * sizeof(unsigned long));
        if (A[i] == NULL) {
            int j;

            for (j = 0; j < i; j++) {
                free(A[j]);
            }
            free(A);
            return -1;
        }
        memcpy(A[i], M[i], (size_t)rwdcnt * sizeof(unsigned long));
    }

    rank = 0;
    for (j = 0; j < cols && rank < rows; j++) {
        rank = pivot_column(A, rows, cols, j, rank);
    }
    for (i = 0; i < rows; i++) {
        free(A[i]);
    }
    free(A);
    return rank;
}

static int selected_column_rank(unsigned long **M, int rows, int n,
                                const int *selected_cols, int selected_count)
{
    int i;
    int j;
    int rank;
    int rwdcnt = 1 + (selected_count - 1) / __WORDSIZE;
    unsigned long **A;

    A = malloc((size_t)rows * sizeof(*A));
    if (A == NULL) {
        return -1;
    }
    for (i = 0; i < rows; i++) {
        A[i] = calloc((size_t)rwdcnt, sizeof(unsigned long));
        if (A[i] == NULL) {
            int k;

            for (k = 0; k < i; k++) {
                free(A[k]);
            }
            free(A);
            return -1;
        }
    }

    for (j = 0; j < selected_count; j++) {
        int src_col = selected_cols[j];

        if (src_col < 0 || src_col >= n) {
            continue;
        }
        for (i = 0; i < rows; i++) {
            if (coeff(M, i, src_col)) {
                A[i][j / __WORDSIZE] ^= (1LU << (j % __WORDSIZE));
            }
        }
    }

    rank = 0;
    for (j = 0; j < selected_count && rank < rows; j++) {
        rank = pivot_column(A, rows, selected_count, j, rank);
    }
    for (i = 0; i < rows; i++) {
        free(A[i]);
    }
    free(A);
    return rank;
}

static unsigned long row_weight(unsigned long **M, int row, int cols)
{
    int j;
    unsigned long weight = 0;

    for (j = 0; j < cols; j++) {
        if (coeff(M, row, j)) {
            weight++;
        }
    }
    return weight;
}

static unsigned long column_weight(unsigned long **M, int rows, int col)
{
    int i;
    unsigned long weight = 0;

    for (i = 0; i < rows; i++) {
        if (coeff(M, i, col)) {
            weight++;
        }
    }
    return weight;
}

static void print_row_bits(FILE *fp, unsigned long **M, int row, int cols,
                           int max_cols)
{
    int j;
    int limit = cols < max_cols ? cols : max_cols;

    for (j = 0; j < limit; j++) {
        fputc(coeff(M, row, j) ? '1' : '0', fp);
    }
    if (limit < cols) {
        fprintf(fp, "...");
    }
    fputc('\n', fp);
}

static void tex_print_escaped(FILE *fp, const char *s)
{
    while (s != NULL && *s != '\0') {
        if (*s == '_' || *s == '%' || *s == '&' || *s == '#' ||
            *s == '$') {
            fputc('\\', fp);
        }
        fputc(*s, fp);
        s++;
    }
}

static void tex_print_t_chunks(FILE *fp, unsigned long **M, int r, int n,
                               int l, const int *perm, int chunk_cols)
{
    int psi_rows = r / l;
    int t_cols = n - r;
    int row_block;
    int max_x = chunk_cols;

    if (max_x <= 0 || max_x > t_cols) {
        max_x = t_cols;
    }
    fprintf(fp, "\\begin{tikzpicture}[x=0.035cm,y=0.035cm]\n");
    fprintf(fp, "\\fill[black!3] (0,0) rectangle (%d,%d);\n",
            max_x, psi_rows);
    for (row_block = 0; row_block < psi_rows; row_block++) {
        int row = row_block * l;
        int x;

        for (x = 0; x < max_x; x++) {
            int start = (int)(((long long)x * (long long)t_cols) / max_x);
            int end = (int)(((long long)(x + 1) * (long long)t_cols) / max_x);
            int total = end - start;
            int count = 0;
            int col;
            const char *color;

            for (col = start; col < end; col++) {
                if (coeff(M, row, perm[r + col])) {
                    count++;
                }
            }
            if (count == 0 || total <= 0) {
                continue;
            }
            color = "heatA";
            if (count * 5 >= total * 4) {
                color = "heatE";
            } else if (count * 5 >= total * 3) {
                color = "heatD";
            } else if (count * 5 >= total * 2) {
                color = "heatC";
            } else if (count * 5 >= total) {
                color = "heatB";
            }
            fprintf(fp, "\\fill[%s] (%d.5,%d.5) circle[radius=0.38];\n",
                    color, x, psi_rows - 1 - row_block);
        }
    }
    fprintf(fp, "\\draw[black!45] (0,0) rectangle (%d,%d);\n",
            max_x, psi_rows);
    fprintf(fp, "\\node[anchor=west,scale=0.7] at (%d.8,%d) {$%d$ representative rows};\n",
            max_x, psi_rows - 1, psi_rows);
    fprintf(fp, "\\node[anchor=west,scale=0.7] at (%d.8,%d) {$%d$ T columns, grouped into %d bins};\n",
            max_x, psi_rows - 4 > 0 ? psi_rows - 4 : 0, t_cols, max_x);
    fprintf(fp, "\\end{tikzpicture}\n");
}

static void tex_print_permuted_row_chunks(FILE *fp, unsigned long **M, int row,
                                          int n, const int *perm,
                                          int chunk_cols)
{
    int max_x = chunk_cols;
    int x;

    if (max_x <= 0 || max_x > n) {
        max_x = n;
    }
    fprintf(fp, "\\begin{tikzpicture}[x=0.035cm,y=0.3cm]\n");
    fprintf(fp, "\\fill[black!3] (0,0) rectangle (%d,1);\n", max_x);
    for (x = 0; x < max_x; x++) {
        int start = (int)(((long long)x * (long long)n) / max_x);
        int end = (int)(((long long)(x + 1) * (long long)n) / max_x);
        int total = end - start;
        int count = 0;
        int col;
        const char *color;

        for (col = start; col < end; col++) {
            if (coeff(M, row, perm[col])) {
                count++;
            }
        }
        if (count == 0 || total <= 0) {
            continue;
        }
        color = "heatA";
        if (count * 5 >= total * 4) {
            color = "heatE";
        } else if (count * 5 >= total * 3) {
            color = "heatD";
        } else if (count * 5 >= total * 2) {
            color = "heatC";
        } else if (count * 5 >= total) {
            color = "heatB";
        }
        fprintf(fp, "\\fill[%s] (%d.5,0.5) circle[radius=0.38];\n",
                color, x);
    }
    fprintf(fp, "\\draw[black!45] (0,0) rectangle (%d,1);\n", max_x);
    fprintf(fp, "\\node[anchor=west,scale=0.7] at (%d.8,0.5) {row %d, %d permuted columns grouped into %d bins};\n",
            max_x, row, n, max_x);
    fprintf(fp, "\\end{tikzpicture}\n");
}

static int write_prefix_t_tex_report(const char *path, unsigned long **M,
                                     int r_full, int r_prefix, int n, int l,
                                     int t, int matrix_rows,
                                     const int *perm, int full_rank,
                                     int prefix_full_rank,
                                     int prefix_block_rank,
                                     int prefix_l_cyclic)
{
    FILE *fp;

    if (path == NULL || path[0] == '\0' || M == NULL || perm == NULL ||
        r_prefix <= 0 || n <= r_prefix) {
        return FALSE;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        return FALSE;
    }

    fprintf(fp, "\\documentclass[9pt]{article}\n");
    fprintf(fp, "\\usepackage[a4paper,landscape,margin=8mm]{geometry}\n");
    fprintf(fp, "\\usepackage{xcolor}\n");
    fprintf(fp, "\\usepackage{tikz}\n");
    fprintf(fp, "\\definecolor{heatA}{RGB}{43,131,186}\n");
    fprintf(fp, "\\definecolor{heatB}{RGB}{127,191,123}\n");
    fprintf(fp, "\\definecolor{heatC}{RGB}{255,255,153}\n");
    fprintf(fp, "\\definecolor{heatD}{RGB}{253,174,97}\n");
    fprintf(fp, "\\definecolor{heatE}{RGB}{215,25,28}\n");
    fprintf(fp, "\\setlength{\\parindent}{0pt}\n");
    fprintf(fp, "\\begin{document}\n");
    fprintf(fp, "\\section*{");
    tex_print_escaped(fp, CRYPTO_ALGNAME);
    fprintf(fp, " prefix QC $T$ report}\n");
    fprintf(fp, "\\textbf{Parameters.} $m=%d$, $n=%d$, $\\ell=%d$, $t=%d$, field rows kept $=%d$, full binary rows $=%d$, prefix rows $=%d$.\\\\\n",
            gf_extd(), n, l, t, matrix_rows, r_full, r_prefix);
    fprintf(fp, "\\textbf{Ranks.} full dropped-row matrix rank $=%d/%d$, prefix rank $=%d/%d$, prefix QC rank $=%d/%d$.\\\\\n",
            full_rank, r_full, prefix_full_rank, r_prefix,
            prefix_block_rank, r_prefix);
    fprintf(fp, "\\textbf{Prefix $T$ is $\\ell$-cyclic:} %s. The one leftover binary row is printed after $\\psi(T)$.\\\\\n",
            prefix_l_cyclic ? "yes" : "no");
    fprintf(fp, "\\section*{Compressed $\\psi(T)$ visualization}\n");
    fprintf(fp, "Each dot is a column-bin of one representative row of $\\psi(T)$; color encodes the density of ones in that bin. Full $T$ is recovered by $\\ell$-cyclic shifts inside each row block.\\\\[1ex]\n");
    fprintf(fp, "\\begin{tikzpicture}[x=0.55cm,y=0.4cm,baseline=-0.5ex]\n");
    fprintf(fp, "\\fill[heatA] (0,0) circle[radius=0.12]; \\node[anchor=west] at (0.25,0) {$0<d<0.2$};\n");
    fprintf(fp, "\\fill[heatB] (3,0) circle[radius=0.12]; \\node[anchor=west] at (3.25,0) {$0.2\\le d<0.4$};\n");
    fprintf(fp, "\\fill[heatC] (6.4,0) circle[radius=0.12]; \\node[anchor=west] at (6.65,0) {$0.4\\le d<0.6$};\n");
    fprintf(fp, "\\fill[heatD] (10,0) circle[radius=0.12]; \\node[anchor=west] at (10.25,0) {$0.6\\le d<0.8$};\n");
    fprintf(fp, "\\fill[heatE] (13.4,0) circle[radius=0.12]; \\node[anchor=west] at (13.65,0) {$0.8\\le d\\le1$};\n");
    fprintf(fp, "\\end{tikzpicture}\\\\[1ex]\n");
    tex_print_t_chunks(fp, M, r_prefix, n, l, perm, 240);
    if (r_prefix < r_full) {
        fprintf(fp, "\\section*{Leftover Binary Row}\n");
        fprintf(fp, "This row is outside the $\\ell$-row block structure when using $m(t-1)$ rows. It is shown as one aggregated strip over the permuted columns.\\\\[1ex]\n");
        tex_print_permuted_row_chunks(fp, M, r_prefix, n, perm, 240);
    }
    fprintf(fp, "\\end{document}\n");
    fclose(fp);
    return TRUE;
}

static int l_cyclic_public_part(unsigned long **M, int r, int l, int *perm,
                                int n);

static unsigned long **alloc_binary_matrix(int rows, int cols)
{
    int i;
    int rwdcnt = 1 + (cols - 1) / __WORDSIZE;
    unsigned long **M;

    M = malloc((size_t)rows * sizeof(*M));
    if (M == NULL) {
        return NULL;
    }
    for (i = 0; i < rows; i++) {
        M[i] = calloc((size_t)rwdcnt, sizeof(unsigned long));
        if (M[i] == NULL) {
            int j;

            for (j = 0; j < i; j++) {
                free(M[j]);
            }
            free(M);
            return NULL;
        }
    }
    return M;
}

static void free_binary_matrix(unsigned long **M, int rows)
{
    int i;

    if (M == NULL) {
        return;
    }
    for (i = 0; i < rows; i++) {
        free(M[i]);
    }
    free(M);
}

static void audit_full_hole_candidate(FILE *fp, unsigned long **full_H,
                                      int full_rows, int n, int l, int t,
                                      int hole_start)
{
    int src;
    int dst;
    int r_cut = full_rows - l;
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;
    int twist_start = (t - 1) * gf_extd();
    int twist_end = t * gf_extd();
    int touches_twist;
    int full_rank;
    int block_rank = 0;
    int public_l_cyclic = FALSE;
    int *perm;
    unsigned long **H;

    if (hole_start < 0 || hole_start + l > full_rows) {
        return;
    }

    touches_twist = (hole_start < twist_end && hole_start + l > twist_start);
    H = alloc_binary_matrix(r_cut, n);
    perm = malloc((size_t)n * sizeof(*perm));
    if (H == NULL || perm == NULL) {
        free_binary_matrix(H, r_cut);
        free(perm);
        fprintf(fp, "hole rows [%d,%d): allocation failed\n",
                hole_start, hole_start + l);
        return;
    }

    dst = 0;
    for (src = 0; src < full_rows; src++) {
        if (src >= hole_start && src < hole_start + l) {
            continue;
        }
        memcpy(H[dst], full_H[src], (size_t)rwdcnt * sizeof(unsigned long));
        dst++;
    }

    full_rank = matrix_rank_copy(H, r_cut, n);
    gausselim(H, r_cut, n);
    block_rank = qc_systematic_form(H, r_cut, n, l, perm);
    if (block_rank == r_cut) {
        public_l_cyclic = l_cyclic_public_part(H, r_cut, l, perm, n);
    }

    fprintf(fp,
            "hole rows [%d,%d): preserves_twist=%s rank=%d/%d QC-rank=%d/%d l-cyclic=%s\n",
            hole_start, hole_start + l, touches_twist ? "no" : "yes",
            full_rank, r_cut, block_rank, r_cut,
            public_l_cyclic ? "yes" : "no");

    free_binary_matrix(H, r_cut);
    free(perm);
}

static void audit_full_hole_rows(gfelt_t *L, poly_t g, int n, int l, int t,
                                 gf_t eta, FILE *fp)
{
    int i, j, k;
    int full_rows = t * gf_extd();
    int candidates[4];
    int candidate_count = 0;
    unsigned long **full_H;
    poly_t col;
    gfindex_t x;

    full_H = alloc_binary_matrix(full_rows, n);
    col = poly_alloc(t - 1);
    if (full_H == NULL || col == NULL) {
        fprintf(fp, "full-row hole audit: allocation failed\n");
        free_binary_matrix(full_H, full_rows);
        poly_free(col);
        return;
    }

    for (i = 0; i < n; i++) {
        poly_syndrome_twisted_explicit_row(col, L + i, g, eta,
                                           scheme_selected_twist_row(t));
        for (j = 0; j < t; j++) {
            x = gf_to_index(poly_coeff(col, j));
            for (k = 0; k < gf_extd(); k++) {
                if (x & (1 << k)) {
                    full_H[j * gf_extd() + k][i / __WORDSIZE] ^=
                        (1UL << (i % __WORDSIZE));
                }
            }
        }
    }
    poly_free(col);

    fprintf(fp, "Full mt-row l-hole audit\n");
    fprintf(fp, "full rows=%d, full rank=%d of %d, twist binary rows=[%d,%d)\n",
            full_rows, matrix_rank_copy(full_H, full_rows, n), full_rows,
            (t - 1) * gf_extd(), t * gf_extd());

    candidates[candidate_count++] = 0;
    candidates[candidate_count++] = (full_rows / (2 * l)) * l;
    candidates[candidate_count++] = full_rows - 2 * l;
    candidates[candidate_count++] = full_rows - l;
    for (i = 0; i < candidate_count; i++) {
        int duplicate = FALSE;

        if (candidates[i] < 0 || candidates[i] + l > full_rows) {
            continue;
        }
        for (j = 0; j < i; j++) {
            if (candidates[j] == candidates[i]) {
                duplicate = TRUE;
            }
        }
        if (!duplicate) {
            audit_full_hole_candidate(fp, full_H, full_rows, n, l, t,
                                      candidates[i]);
        }
    }

    free_binary_matrix(full_H, full_rows);
}

int goppa_matgen_audit(gfelt_t * L, poly_t g, int n, int l, int t,
                       gf_t eta, FILE *fp)
{
    int i, j, k, r;
    int matrix_rows;
    int qc_aligned;
    int block_rank = 0;
    int full_rank;
    int partial_block_rank = 0;
    int partial_l_cyclic = FALSE;
    int public_l_cyclic = FALSE;
    int *perm = NULL;
    int selected_blocks = 0;
    int selected_cols_count = 0;
    int *selected_cols = NULL;
    unsigned long **H = NULL;
    poly_t col = NULL;
    gf_t eta_local;
    gfindex_t x;

    if (fp == NULL) {
        fp = stdout;
    }
    if (L == NULL || g == NULL || !goppa_check_params(n, gf_extd(), l, t)) {
        fprintf(fp, "audit: invalid input parameters\n");
        return 0;
    }

    matrix_rows = scheme_matgen_field_rows(t);
    r = scheme_matgen_binary_rows(gf_extd(), t);
    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(eta_local, eta);
    } else {
        gf_set_to_unit(eta_local);
    }

    audit_full_hole_rows(L, g, n, l, t, eta_local, fp);

    H = malloc((size_t)r * sizeof(*H));
    col = poly_alloc(t - 1);
    if (H == NULL || col == NULL) {
        free(H);
        poly_free(col);
        fprintf(fp, "audit: allocation failed\n");
        return 0;
    }
    for (i = 0; i < r; i++) {
        H[i] = calloc((size_t)(1 + (n - 1) / __WORDSIZE),
                      sizeof(unsigned long));
        if (H[i] == NULL) {
            int u;

            for (u = 0; u < i; u++) {
                free(H[u]);
            }
            free(H);
            poly_free(col);
            fprintf(fp, "audit: allocation failed\n");
            return 0;
        }
    }

    for (i = 0; i < n; i++) {
        poly_syndrome_twisted_explicit_row(col, L + i, g, eta_local,
                                           scheme_selected_twist_row(t));
        for (j = 0; j < matrix_rows; j++) {
            x = gf_to_index(poly_coeff(col, j));
            for (k = 0; k < gf_extd(); k++) {
                if (x & (1 << k)) {
                    H[j * gf_extd() + k][i / __WORDSIZE] ^=
                        (1UL << (i % __WORDSIZE));
                }
            }
        }
    }
    poly_free(col);

    full_rank = matrix_rank_copy(H, r, n);
    qc_aligned = scheme_matgen_qc_aligned(n, l, r);

    fprintf(fp, "MatGen audit\n");
    fprintf(fp, "parameters: m=%d n=%d l=%d t0=%d t=%d matrix_rows=%d r=m*(t-1)=%d twist_row=%d dropped_row=%d\n",
            gf_extd(), n, l, t / l, t, matrix_rows, r,
            scheme_selected_twist_row(t), t - 1);
    fprintf(fp, "full binary rank(H_T without dropped row)=%d of %d\n",
            full_rank, r);
    if (!qc_aligned) {
        int r_prefix = r - (r % l);

        fprintf(fp, "QC-systematic rank=not run; r mod l=%d, (n-r) mod l=%d\n",
                r % l, (n - r) % l);
        fprintf(fp, "l-cyclic public T=no\n");
        fprintf(fp, "max complete l-row prefix: r'=%d, extra binary rows outside l-blocks=%d\n",
                r_prefix, r - r_prefix);
        if (r_prefix > 0 && ((n - r_prefix) % l) == 0) {
            int prefix_full_rank = matrix_rank_copy(H, r_prefix, n);
            int *prefix_perm = malloc((size_t)n * sizeof(*prefix_perm));

            fprintf(fp, "prefix full binary rank=%d of %d\n",
                    prefix_full_rank, r_prefix);
            if (prefix_perm != NULL) {
                int prefix_block_rank;
                int prefix_l_cyclic = FALSE;

                gausselim(H, r_prefix, n);
                prefix_block_rank = qc_systematic_form(H, r_prefix, n, l,
                                                       prefix_perm);
                if (prefix_block_rank == r_prefix) {
                    prefix_l_cyclic = l_cyclic_public_part(H, r_prefix, l,
                                                           prefix_perm, n);
                }
                partial_block_rank = prefix_block_rank;
                partial_l_cyclic = prefix_l_cyclic;
                fprintf(fp, "prefix QC-systematic rank=%d of %d\n",
                        prefix_block_rank, r_prefix);
                fprintf(fp, "prefix l-cyclic public T=%s\n",
                        prefix_l_cyclic ? "yes" : "no");
                if (prefix_l_cyclic) {
                    const char *tex_path = getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_PREFIX_T_TEX");

                    if (tex_path != NULL && tex_path[0] != '\0') {
                        int wrote_tex = write_prefix_t_tex_report(
                            tex_path, H, r, r_prefix, n, l, t, matrix_rows,
                            prefix_perm, full_rank, prefix_full_rank,
                            prefix_block_rank, prefix_l_cyclic);

                        fprintf(fp, "prefix T TeX report=%s (%s)\n",
                                tex_path, wrote_tex ? "written" : "failed");
                    }
                }
                free(prefix_perm);
            }
        }
        if (r_prefix < r) {
            fprintf(fp, "leftover row %d weight=%lu sample: ",
                    r_prefix, row_weight(H, r_prefix, n));
            print_row_bits(fp, H, r_prefix, n, 160);
        }
        goto cleanup;
    }

    perm = malloc((size_t)n * sizeof(*perm));
    if (perm == NULL) {
        fprintf(fp, "audit: allocation failed\n");
        goto cleanup;
    }
    gausselim(H, r, n);
    block_rank = qc_systematic_form(H, r, n, l, perm);
    if (block_rank == r) {
        public_l_cyclic = l_cyclic_public_part(H, r, l, perm, n);
    }
    selected_blocks = block_rank / l;
    selected_cols_count = selected_blocks * l;
    selected_cols = malloc((size_t)(selected_cols_count > 0 ? selected_cols_count : 1) *
                           sizeof(*selected_cols));
    if (selected_cols != NULL) {
        for (i = 0; i < selected_cols_count; i++) {
            selected_cols[i] = perm[i];
        }
    }

    fprintf(fp, "QC-systematic rank=%d of %d\n", block_rank, r);
    fprintf(fp, "l-cyclic public T=%s\n",
            public_l_cyclic ? "yes" : "no");
    fprintf(fp, "selected block columns=%d of %d required\n",
            selected_blocks, r / l);
    fprintf(fp, "missing rank=%d, missing blocks=%d\n",
            r - block_rank, (r - block_rank) / l);
    fprintf(fp, "selected pivot block indices:");
    for (i = 0; i < selected_blocks && i < 40; i++) {
        fprintf(fp, " %d", perm[i * l] / l);
    }
    if (selected_blocks > 40) {
        fprintf(fp, " ...");
    }
    fprintf(fp, "\n");

    if (selected_cols != NULL && selected_cols_count > 0) {
        int selected_rank = selected_column_rank(H, r, n, selected_cols,
                                                selected_cols_count);

        fprintf(fp, "rank(selected pivot columns after elimination)=%d of %d\n",
                selected_rank, selected_cols_count);
    }

    fprintf(fp, "first row weights:");
    for (i = 0; i < 12 && i < r; i++) {
        fprintf(fp, " row%d=%lu", i, row_weight(H, i, n));
    }
    fprintf(fp, "\n");
    fprintf(fp, "first block column weights:");
    for (j = 0; j < 3 * l && j < n; j++) {
        fprintf(fp, " col%d=%lu", j, column_weight(H, r, j));
    }
    fprintf(fp, "\n");
    fprintf(fp, "sample transformed rows (first 160 columns):\n");
    for (i = block_rank > 6 ? block_rank - 6 : 0; i < r && i < block_rank + 6; i++) {
        fprintf(fp, "row %d: ", i);
        print_row_bits(fp, H, i, n, 160);
    }

cleanup:
    free(selected_cols);
    free(perm);
    if (H != NULL) {
        for (i = 0; i < r; i++) {
            free(H[i]);
        }
        free(H);
    }
    if (block_rank == r && public_l_cyclic) {
        return block_rank;
    }
    if (partial_l_cyclic) {
        return partial_block_rank;
    }
    return 0;
}

static int l_cyclic_public_part(unsigned long **M, int r, int l, int *perm,
                                int n)
{
    int block;
    int shift;
    int row;

    if (M == NULL || perm == NULL || l <= 1 || r <= 0 || n <= r ||
        (r % l) != 0 || ((n - r) % l) != 0) {
        return FALSE;
    }

    for (block = 0; block < n - r; block += l) {
        int base_col = perm[r + block];

        for (shift = 1; shift < l; shift++) {
            int col = perm[r + block + shift];

            for (row = 0; row < r; row++) {
                int expected_row = (row / l) * l + ((row + l - shift) % l);
                int expected = coeff(M, expected_row, base_col);
                int got = coeff(M, row, col);

                if (expected != got) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

static int hybrid_total_rows(int m, int t)
{
    return m * scheme_matgen_field_rows(t) + 1;
}

static int hybrid_tail_rows(int m, int t, int l)
{
    return hybrid_total_rows(m, t) % l;
}

static int hybrid_qc_rows(int m, int t, int l)
{
    return hybrid_total_rows(m, t) - hybrid_tail_rows(m, t, l);
}

static void set_decode_map_bit(unsigned char *decode_map, int row, int col)
{
    decode_map[(size_t)row * BITS_TO_BYTES(CODIMENSION) + (size_t)col / 8U] |=
        (unsigned char)(1U << (col % 8));
}

static int sun_syndrome_to_sui_definition(poly_t R, poly_t S, poly_t g)
{
    int i;
    int j;
    int t;
    gf_t coeff;
    gf_t term;

    if (R == NULL || S == NULL || g == NULL) {
        return FALSE;
    }
    t = poly_deg(g);
    if (t <= 0) {
        return FALSE;
    }

    poly_set_to_zero(R);
    for (i = 0; i < t; i++) {
        gf_set_to_zero(coeff);
        for (j = i + 1; j <= t; j++) {
            gf_mul(term, poly_coeff(g, j), poly_coeff(S, j - 1 - i));
            gf_add(coeff, coeff, term);
        }
        poly_set_coeff(R, i, coeff);
    }
    poly_calcule_deg(R);
    return TRUE;
}

static int build_hybrid_decode_map(unsigned char *decode_map,
                                   unsigned long **full_H,
                                   unsigned long **hybrid_H,
                                   int n, int *perm,
                                   int qc_rows, int total_rows)
{
    int full_rows = GOPPA_DEGREE * gf_extd();
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;
    int row;
    int i;
    unsigned long *residual = NULL;

    if (decode_map == NULL || total_rows != CODIMENSION ||
        total_rows - qc_rows != SYSTEMATIC_TAIL_ROWS) {
        return FALSE;
    }

    residual = calloc((size_t)rwdcnt, sizeof(*residual));
    if (residual == NULL) {
        return FALSE;
    }

    memset(decode_map, 0,
           (size_t)FULL_SYNDROME_LENGTH * BITS_TO_BYTES(CODIMENSION));
    for (row = 0; row < full_rows; row++) {
        int c0;
        int c1;
        int found = FALSE;

        memcpy(residual, full_H[row], (size_t)rwdcnt * sizeof(*residual));
        for (i = 0; i < qc_rows; i++) {
            if (coeff(full_H, row, perm[i])) {
                set_decode_map_bit(decode_map, row, i);
                xor_long(residual, hybrid_H[i], rwdcnt);
            }
        }

        for (c0 = 0; c0 < 2 && !found; c0++) {
            for (c1 = 0; c1 < 2 && !found; c1++) {
                int word;
                int ok = TRUE;

                for (word = 0; word < rwdcnt; word++) {
                    unsigned long tail = 0;

                    if (c0) {
                        tail ^= hybrid_H[qc_rows][word];
                    }
                    if (c1) {
                        tail ^= hybrid_H[qc_rows + 1][word];
                    }
                    if (residual[word] != tail) {
                        ok = FALSE;
                        break;
                    }
                }
                if (ok) {
                    if (c0) {
                        set_decode_map_bit(decode_map, row, qc_rows);
                    }
                    if (c1) {
                        set_decode_map_bit(decode_map, row, qc_rows + 1);
                    }
                    found = TRUE;
                }
            }
        }
        if (!found) {
            free(residual);
            return FALSE;
        }
    }
    free(residual);
    return TRUE;
}

static int verify_hybrid_decode_map(unsigned char *decode_map,
                                    unsigned long **full_H,
                                    unsigned long **hybrid_H,
                                    int n, int total_rows)
{
    int row;
    int hrow;
    int word;
    int full_rows = GOPPA_DEGREE * gf_extd();
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;
    int map_row_bytes = BITS_TO_BYTES(CODIMENSION);
    unsigned long *acc = calloc((size_t)rwdcnt, sizeof(*acc));

    if (acc == NULL) {
        return FALSE;
    }
    for (row = 0; row < full_rows; row++) {
        memset(acc, 0, (size_t)rwdcnt * sizeof(*acc));
        for (hrow = 0; hrow < total_rows; hrow++) {
            if ((decode_map[(size_t)row * map_row_bytes + hrow / 8] >>
                 (hrow % 8)) & 1U) {
                xor_long(acc, hybrid_H[hrow], rwdcnt);
            }
        }
        for (word = 0; word < rwdcnt; word++) {
            if (acc[word] != full_H[row][word]) {
                fprintf(stderr,
                        "decode_map verification failed at full row %d word %d\n",
                        row, word);
                free(acc);
                return FALSE;
            }
        }
    }
    free(acc);
    return TRUE;
}

static void pk_set_bit(unsigned char *pk, int bit)
{
    pk[(size_t)bit / 8U] ^= (unsigned char)(1U << (bit % 8));
}

static int shifted_public_position(int public_pos, int shift, int l)
{
    int block = public_pos / l;
    int in_block = public_pos % l;

    return block * l + ((in_block + l - shift) % l);
}

static int hybrid_dump_t1_enabled(void)
{
    const char *dump = getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_DUMP_T1");

    return dump != NULL && dump[0] != '\0' && dump[0] != '0';
}

static void hybrid_print_public_block_bits(unsigned long **M, int row,
                                         int public_block, int l, int qc_rows,
                                         int *perm)
{
    int c;

    for (c = 0; c < l; c++) {
        int public_pos = public_block * l + c;

        fputc(coeff(M, row, perm[qc_rows + public_pos]) ? '1' : '0',
              stderr);
    }
}

static void hybrid_print_expected_block_bits(unsigned long **M, int base_row,
                                           int row_shift, int public_block,
                                           int l, int qc_rows, int *perm)
{
    int c;

    for (c = 0; c < l; c++) {
        int src_public_pos = public_block * l +
            ((c + l - row_shift) % l);

        fputc(coeff(M, base_row, perm[qc_rows + src_public_pos]) ? '1' : '0',
              stderr);
    }
}

static void hybrid_dump_t1_blocks(unsigned long **M, int qc_rows,
                                int tail_rows, int l, int *perm, int n)
{
    int row_block;
    int public_block;
    int row_shift;
    int c;
    int printed = 0;
    int total_mismatches = 0;
    int bad_blocks = 0;
    int qc_blocks = qc_rows / l;
    int public_blocks = (n - qc_rows) / l;

    fprintf(stderr,
            "T1 block dump: qc_blocks=%d public_blocks=%d tail_rows=%d T1_cols=%d l=%d\n",
            qc_blocks, public_blocks, tail_rows, n - qc_rows, l);

    for (row_block = 0; row_block < qc_blocks; row_block++) {
        int row_block_mismatches = 0;

        for (public_block = 0; public_block < public_blocks; public_block++) {
            int block_mismatches = 0;

            for (row_shift = 0; row_shift < l; row_shift++) {
                int row = row_block * l + row_shift;
                int base_row = row_block * l;

                for (c = 0; c < l; c++) {
                    int public_pos = public_block * l + c;
                    int src_public_pos = public_block * l +
                        ((c + l - row_shift) % l);
                    int got = coeff(M, row, perm[qc_rows + public_pos]);
                    int expected = coeff(M, base_row,
                                         perm[qc_rows + src_public_pos]);

                    if (got != expected) {
                        block_mismatches++;
                    }
                }
            }
            if (block_mismatches != 0) {
                bad_blocks++;
                row_block_mismatches += block_mismatches;
                if (printed < 6) {
                    fprintf(stderr,
                            "T1 bad block row_block=%d public_block=%d mismatches=%d",
                            row_block, public_block, block_mismatches);
                    fputc('\n', stderr);
                    for (row_shift = 0; row_shift < l; row_shift++) {
                        int row = row_block * l + row_shift;
                        int base_row = row_block * l;

                        fprintf(stderr, "  row+%02d got=", row_shift);
                        hybrid_print_public_block_bits(M, row, public_block, l,
                                                     qc_rows, perm);
                        fprintf(stderr, " expected=");
                        hybrid_print_expected_block_bits(M, base_row, row_shift,
                                                       public_block, l,
                                                       qc_rows, perm);
                        fputc('\n', stderr);
                    }
                    printed++;
                }
            }
            total_mismatches += block_mismatches;
        }
        if (row_block_mismatches != 0 && row_block < 12) {
            fprintf(stderr, "T1 row_block=%d total_mismatches=%d\n",
                    row_block, row_block_mismatches);
        }
    }
    fprintf(stderr, "T1 mismatch summary: bad_blocks=%d total_mismatches=%d\n",
            bad_blocks, total_mismatches);
}

static int hybrid_t1_l_cyclic_public_part(unsigned long **M, int qc_rows,
                                        int tail_rows, int l, int *perm,
                                        int n)
{
    int public_cols = n - qc_rows;
    int row;
    int col;

    if (M == NULL || perm == NULL || qc_rows <= 0 || tail_rows <= 0 ||
        (qc_rows % l) != 0 || n <= qc_rows + tail_rows) {
        return FALSE;
    }

    for (row = 0; row < qc_rows; row++) {
        int base_row = (row / l) * l;
        int shift = row % l;

        for (col = 0; col < public_cols; col++) {
            int public_pos = col;
            int src_public_pos = shifted_public_position(public_pos, shift, l);
            int expected = coeff(M, base_row, perm[qc_rows + src_public_pos]);
            int got = coeff(M, row, perm[qc_rows + col]);

            if (got != expected) {
                if (hybrid_dump_t1_enabled()) {
                    hybrid_dump_t1_blocks(M, qc_rows, tail_rows, l, perm, n);
                }
                return FALSE;
            }
        }
    }
    return TRUE;
}

static int public_key_bit(const unsigned char *pk, int bit)
{
    return (pk[(size_t)bit / 8U] >> (bit % 8)) & 1U;
}

static int public_key_t_bit(const unsigned char *pk, int row, int col,
                            int l, int qc_rows, int public_cols)
{
    if (row < qc_rows) {
        int base_block = row / l;
        int shift = row % l;
        int src_public_pos = shifted_public_position(col, shift, l);

        return public_key_bit(pk, base_block * public_cols + src_public_pos);
    }
    return public_key_bit(pk, (qc_rows / l) * public_cols +
                          (row - qc_rows) * public_cols + col);
}

static void matrix_set_bit(unsigned long **M, int row, int col)
{
    M[row][col / __WORDSIZE] |= (1UL << (col % __WORDSIZE));
}

static int syndrome_lsb_bit(const unsigned char *syndrome, int bit)
{
    return (syndrome[(size_t)bit / 8U] >> (bit % 8)) & 1U;
}

static void syndrome_lsb_set(unsigned char *syndrome, int bit)
{
    syndrome[(size_t)bit / 8U] |= (unsigned char)(1U << (bit % 8));
}

static void syndrome_lsb_toggle(unsigned char *syndrome, int bit)
{
    syndrome[(size_t)bit / 8U] ^= (unsigned char)(1U << (bit % 8));
}

static void syndrome_xor_bytes(unsigned char *dst, const unsigned char *src)
{
    size_t i = 0;

    for (; i + sizeof(uint64_t) <= (size_t)FULL_SYNDROME_BYTES;
         i += sizeof(uint64_t)) {
        uint64_t a;
        uint64_t b;

        memcpy(&a, dst + i, sizeof(a));
        memcpy(&b, src + i, sizeof(b));
        a ^= b;
        memcpy(dst + i, &a, sizeof(a));
    }
    for (; i < (size_t)FULL_SYNDROME_BYTES; i++) {
        dst[i] ^= src[i];
    }
}

static int xor_sun_column(unsigned char *out, gfelt_t *alpha, poly_t g,
                          gf_t eta_local, poly_t col, poly_t sun_col,
                          int matrix_rows, int t)
{
    int j;
    int k;

    if (out == NULL || alpha == NULL || g == NULL || col == NULL ||
        sun_col == NULL) {
        return FALSE;
    }

    poly_syndrome_twisted_explicit_row(col, alpha, g, eta_local,
                                       scheme_selected_twist_row(t));
    if (matrix_rows == t - 1 && sun_col->size == t) {
        for (j = 0; j < matrix_rows; j++) {
            poly_set_coeff(sun_col, j, poly_coeff(col, j));
        }
        poly_set_coeff_to_unit(sun_col, t - 1);
        poly_set_deg(sun_col, t - 1);
    } else {
        poly_set_to_zero(sun_col);
        for (j = 0; j < matrix_rows; j++) {
            poly_set_coeff(sun_col, j, poly_coeff(col, j));
        }
        poly_set_coeff_to_unit(sun_col, t - 1);
        poly_calcule_deg(sun_col);
    }

    for (j = 0; j < t; j++) {
        gfindex_t x = gf_to_index(poly_coeff(sun_col, j));

        for (k = 0; k < gf_extd(); k++) {
            if ((x >> k) & 1U) {
                syndrome_lsb_toggle(out, j * gf_extd() + k);
            }
        }
    }

    return TRUE;
}

static int compute_sun_column(unsigned char *out, gfelt_t *alpha, poly_t g,
                              gf_t eta_local, poly_t col, poly_t sun_col,
                              int matrix_rows, int t)
{
    memset(out, 0, FULL_SYNDROME_BYTES);
    return xor_sun_column(out, alpha, g, eta_local, col, sun_col,
                          matrix_rows, t);
}

static int find_t2_pivots(const unsigned char *pk, int l, int qc_rows,
                          int public_cols, int *p0, int *p1, int *a,
                          int *b, int *c, int *d)
{
    int col0;
    int col1;

    for (col0 = 0; col0 < public_cols; col0++) {
        int a0 = public_key_t_bit(pk, qc_rows, col0, l, qc_rows,
                                  public_cols);
        int c0 = public_key_t_bit(pk, qc_rows + 1, col0, l, qc_rows,
                                  public_cols);

        if (!a0 && !c0) {
            continue;
        }
        for (col1 = col0 + 1; col1 < public_cols; col1++) {
            int b0 = public_key_t_bit(pk, qc_rows, col1, l, qc_rows,
                                      public_cols);
            int d0 = public_key_t_bit(pk, qc_rows + 1, col1, l, qc_rows,
                                      public_cols);

            if ((a0 & d0) ^ (b0 & c0)) {
                *p0 = col0;
                *p1 = col1;
                *a = a0;
                *b = b0;
                *c = c0;
                *d = d0;
                return TRUE;
            }
        }
    }

    return FALSE;
}

static int goppa_public_syndrome_to_sun_syndrome_fast(
    gfelt_t *L, poly_t g, int n, int l, int t, gf_t eta,
    const unsigned char *pk, const unsigned char *syndrome,
    unsigned char *sun_syndrome)
{
    int i;
    int qc_rows = hybrid_qc_rows(gf_extd(), t, l);
    int tail_rows = hybrid_tail_rows(gf_extd(), t, l);
    int r = hybrid_total_rows(gf_extd(), t);
    int matrix_rows = scheme_matgen_field_rows(t);
    int public_cols = n - qc_rows;
    int p0 = -1;
    int p1 = -1;
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    gf_t eta_local;
    poly_t col = NULL;
    poly_t sun_col = NULL;
    unsigned char tmp[FULL_SYNDROME_BYTES];
    unsigned char r0[FULL_SYNDROME_BYTES];
    unsigned char r1[FULL_SYNDROME_BYTES];
    unsigned char x0[FULL_SYNDROME_BYTES];
    unsigned char x1[FULL_SYNDROME_BYTES];
    int ok = FALSE;

    if (L == NULL || g == NULL || pk == NULL || syndrome == NULL ||
        sun_syndrome == NULL || r != CODIMENSION ||
        qc_rows != SYSTEMATIC_QC_ROWS ||
        tail_rows != SYSTEMATIC_TAIL_ROWS || public_cols != PUBLIC_T_COLUMNS ||
        (qc_rows % l) != 0 || (public_cols % l) != 0 ||
        tail_rows != 2) {
        return FALSE;
    }

    if (!find_t2_pivots(pk, l, qc_rows, public_cols, &p0, &p1,
                        &a, &b, &c, &d)) {
        return FALSE;
    }

    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(eta_local, eta);
    } else {
        gf_set_to_unit(eta_local);
    }

    col = poly_alloc(t - 1);
    sun_col = poly_alloc(t - 1);
    if (col == NULL || sun_col == NULL) {
        goto cleanup;
    }

    memset(sun_syndrome, 0, FULL_SYNDROME_BYTES);
    memset(r0, 0, sizeof(r0));
    memset(r1, 0, sizeof(r1));

    for (i = 0; i < qc_rows; i++) {
        int use_c = syndrome_lsb_bit(syndrome, i);
        int use_p0 = public_key_t_bit(pk, i, p0, l, qc_rows, public_cols);
        int use_p1 = public_key_t_bit(pk, i, p1, l, qc_rows, public_cols);

        if (!use_c && !use_p0 && !use_p1) {
            continue;
        }
        if (!compute_sun_column(tmp, L + i, g, eta_local, col, sun_col,
                                matrix_rows, t)) {
            goto cleanup;
        }
        if (use_c) {
            syndrome_xor_bytes(sun_syndrome, tmp);
        }
        if (use_p0) {
            syndrome_xor_bytes(r0, tmp);
        }
        if (use_p1) {
            syndrome_xor_bytes(r1, tmp);
        }
    }

    if (!compute_sun_column(tmp, L + qc_rows + p0, g, eta_local, col,
                            sun_col, matrix_rows, t)) {
        goto cleanup;
    }
    syndrome_xor_bytes(r0, tmp);
    if (!compute_sun_column(tmp, L + qc_rows + p1, g, eta_local, col,
                            sun_col, matrix_rows, t)) {
        goto cleanup;
    }
    syndrome_xor_bytes(r1, tmp);

    memset(x0, 0, sizeof(x0));
    memset(x1, 0, sizeof(x1));
    if (d) {
        syndrome_xor_bytes(x0, r0);
    }
    if (c) {
        syndrome_xor_bytes(x0, r1);
    }
    if (b) {
        syndrome_xor_bytes(x1, r0);
    }
    if (a) {
        syndrome_xor_bytes(x1, r1);
    }

    if (syndrome_lsb_bit(syndrome, qc_rows)) {
        syndrome_xor_bytes(sun_syndrome, x0);
    }
    if (syndrome_lsb_bit(syndrome, qc_rows + 1)) {
        syndrome_xor_bytes(sun_syndrome, x1);
    }

    ok = TRUE;

cleanup:
    if (col != NULL) {
        poly_free(col);
    }
    if (sun_col != NULL) {
        poly_free(sun_col);
    }
    return ok;
}

static int apply_cached_syndrome_map(unsigned char *sun_syndrome,
                                     const unsigned char *map_cols,
                                     const unsigned char *syndrome, int r)
{
    int i;

    memset(sun_syndrome, 0, FULL_SYNDROME_BYTES);
    for (i = 0; i < r; i++) {
        if (syndrome_lsb_bit(syndrome, i)) {
            syndrome_xor_bytes(sun_syndrome,
                               map_cols + (size_t)i * FULL_SYNDROME_BYTES);
        }
    }
    return TRUE;
}

static int build_cached_syndrome_map(unsigned char *map_cols, gfelt_t *L,
                                     poly_t g, int n, int l, int t, gf_t eta,
                                     const unsigned char *pk)
{
    int i;
    int qc_rows = hybrid_qc_rows(gf_extd(), t, l);
    int tail_rows = hybrid_tail_rows(gf_extd(), t, l);
    int r = hybrid_total_rows(gf_extd(), t);
    int matrix_rows = scheme_matgen_field_rows(t);
    int public_cols = n - qc_rows;
    int p0 = -1;
    int p1 = -1;
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    gf_t eta_local;
    poly_t col = NULL;
    poly_t sun_col = NULL;
    unsigned char r0[FULL_SYNDROME_BYTES];
    unsigned char r1[FULL_SYNDROME_BYTES];
    unsigned char *dst = NULL;
    int ok = FALSE;

    if (map_cols == NULL || L == NULL || g == NULL || pk == NULL ||
        r != CODIMENSION || qc_rows != SYSTEMATIC_QC_ROWS ||
        tail_rows != SYSTEMATIC_TAIL_ROWS || public_cols != PUBLIC_T_COLUMNS ||
        tail_rows != 2) {
        return FALSE;
    }
    if (!find_t2_pivots(pk, l, qc_rows, public_cols, &p0, &p1,
                        &a, &b, &c, &d)) {
        return FALSE;
    }

    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(eta_local, eta);
    } else {
        gf_set_to_unit(eta_local);
    }

    col = poly_alloc(t - 1);
    sun_col = poly_alloc(t - 1);
    if (col == NULL || sun_col == NULL) {
        goto cleanup;
    }

    memset(map_cols, 0, (size_t)r * FULL_SYNDROME_BYTES);
    memset(r0, 0, sizeof(r0));
    memset(r1, 0, sizeof(r1));

    for (i = 0; i < qc_rows; i++) {
        dst = map_cols + (size_t)i * FULL_SYNDROME_BYTES;
        if (!compute_sun_column(dst, L + i, g, eta_local, col, sun_col,
                                matrix_rows, t)) {
            goto cleanup;
        }
        if (public_key_t_bit(pk, i, p0, l, qc_rows, public_cols)) {
            syndrome_xor_bytes(r0, dst);
        }
        if (public_key_t_bit(pk, i, p1, l, qc_rows, public_cols)) {
            syndrome_xor_bytes(r1, dst);
        }
    }

    dst = map_cols + (size_t)qc_rows * FULL_SYNDROME_BYTES;
    if (!compute_sun_column(dst, L + qc_rows + p0, g, eta_local, col,
                            sun_col, matrix_rows, t)) {
        goto cleanup;
    }
    syndrome_xor_bytes(r0, dst);
    dst = map_cols + (size_t)(qc_rows + 1) * FULL_SYNDROME_BYTES;
    if (!compute_sun_column(dst, L + qc_rows + p1, g, eta_local, col,
                            sun_col, matrix_rows, t)) {
        goto cleanup;
    }
    syndrome_xor_bytes(r1, dst);

    dst = map_cols + (size_t)qc_rows * FULL_SYNDROME_BYTES;
    memset(dst, 0, FULL_SYNDROME_BYTES);
    if (d) {
        syndrome_xor_bytes(dst, r0);
    }
    if (c) {
        syndrome_xor_bytes(dst, r1);
    }

    dst = map_cols + (size_t)(qc_rows + 1) * FULL_SYNDROME_BYTES;
    memset(dst, 0, FULL_SYNDROME_BYTES);
    if (b) {
        syndrome_xor_bytes(dst, r0);
    }
    if (a) {
        syndrome_xor_bytes(dst, r1);
    }

    ok = TRUE;

cleanup:
    if (col != NULL) {
        poly_free(col);
    }
    if (sun_col != NULL) {
        poly_free(sun_col);
    }
    return ok;
}

static int goppa_public_syndrome_to_sun_syndrome_cached(
    gfelt_t *L, poly_t g, int n, int l, int t, gf_t eta,
    const unsigned char *pk, const unsigned char *syndrome,
    unsigned char *sun_syndrome)
{
    static unsigned char *cached_map_cols = NULL;
    static unsigned char *cached_pk = NULL;
    static int cached_n = 0;
    static int cached_l = 0;
    static int cached_t = 0;
    static int cached_valid = FALSE;
    int r = hybrid_total_rows(gf_extd(), t);
    int cache_hit;

    if (getenv("QCTM_DISABLE_DEC_CACHE") != NULL) {
        return goppa_public_syndrome_to_sun_syndrome_fast(
            L, g, n, l, t, eta, pk, syndrome, sun_syndrome);
    }
    if (r != CODIMENSION || pk == NULL || syndrome == NULL ||
        sun_syndrome == NULL) {
        return FALSE;
    }

    if (cached_map_cols == NULL) {
        cached_map_cols = malloc((size_t)r * FULL_SYNDROME_BYTES);
        if (cached_map_cols == NULL) {
            return FALSE;
        }
        cached_valid = FALSE;
    }
    if (cached_pk == NULL) {
        cached_pk = malloc(PUBLICKEY_BYTES);
        if (cached_pk == NULL) {
            return FALSE;
        }
        cached_valid = FALSE;
    }

    cache_hit = cached_valid && cached_n == n && cached_l == l &&
                cached_t == t &&
                memcmp(cached_pk, pk, PUBLICKEY_BYTES) == 0;
    if (!cache_hit) {
        if (!build_cached_syndrome_map(cached_map_cols, L, g, n, l, t,
                                       eta, pk)) {
            cached_valid = FALSE;
            return FALSE;
        }
        memcpy(cached_pk, pk, PUBLICKEY_BYTES);
        cached_n = n;
        cached_l = l;
        cached_t = t;
        cached_valid = TRUE;
    }

    return apply_cached_syndrome_map(sun_syndrome, cached_map_cols,
                                     syndrome, r);
}

static int apply_hybrid_decode_rows_direct(unsigned char *sun_syndrome,
                                           unsigned long **full_H,
                                           unsigned long **hybrid_H,
                                           const unsigned char *syndrome,
                                           int n, int *perm,
                                           int qc_rows, int total_rows)
{
    int full_rows = GOPPA_DEGREE * gf_extd();
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;
    int row;
    int i;
    unsigned long *residual = NULL;

    if (sun_syndrome == NULL || full_H == NULL || hybrid_H == NULL ||
        syndrome == NULL || perm == NULL || total_rows != CODIMENSION ||
        total_rows - qc_rows != SYSTEMATIC_TAIL_ROWS) {
        return FALSE;
    }

    residual = calloc((size_t)rwdcnt, sizeof(*residual));
    if (residual == NULL) {
        return FALSE;
    }

    memset(sun_syndrome, 0, FULL_SYNDROME_BYTES);
    for (row = 0; row < full_rows; row++) {
        int c0;
        int c1;
        int found = FALSE;
        int acc = 0;

        memcpy(residual, full_H[row], (size_t)rwdcnt * sizeof(*residual));
        for (i = 0; i < qc_rows; i++) {
            if (coeff(full_H, row, perm[i])) {
                acc ^= syndrome_lsb_bit(syndrome, i);
                xor_long(residual, hybrid_H[i], rwdcnt);
            }
        }

        for (c0 = 0; c0 < 2 && !found; c0++) {
            for (c1 = 0; c1 < 2 && !found; c1++) {
                int word;
                int ok = TRUE;

                for (word = 0; word < rwdcnt; word++) {
                    unsigned long tail = 0;

                    if (c0) {
                        tail ^= hybrid_H[qc_rows][word];
                    }
                    if (c1) {
                        tail ^= hybrid_H[qc_rows + 1][word];
                    }
                    if (residual[word] != tail) {
                        ok = FALSE;
                        break;
                    }
                }
                if (ok) {
                    if (c0) {
                        acc ^= syndrome_lsb_bit(syndrome, qc_rows);
                    }
                    if (c1) {
                        acc ^= syndrome_lsb_bit(syndrome, qc_rows + 1);
                    }
                    found = TRUE;
                }
            }
        }
        if (!found) {
            free(residual);
            return FALSE;
        }
        if (acc) {
            syndrome_lsb_set(sun_syndrome, row);
        }
    }
    free(residual);
    return TRUE;
}

int goppa_decode_map_from_public_key(gfelt_t *L, poly_t g, int n, int l,
                                     int t, gf_t eta,
                                     const unsigned char *pk,
                                     unsigned char *decode_map)
{
    int i;
    int j;
    int k;
    int matrix_rows = scheme_matgen_field_rows(t);
    int qc_rows = hybrid_qc_rows(gf_extd(), t, l);
    int tail_rows = hybrid_tail_rows(gf_extd(), t, l);
    int r = hybrid_total_rows(gf_extd(), t);
    int public_cols = n - qc_rows;
    int *identity_perm = NULL;
    gf_t eta_local;
    gfindex_t x;
    unsigned long **hybrid_H = NULL;
    unsigned long **full_H = NULL;
    poly_t col = NULL;
    poly_t sun_col = NULL;
    int ok = FALSE;
    const char *fail_stage = "entry";

    if (L == NULL || g == NULL || pk == NULL || decode_map == NULL ||
        r != CODIMENSION || qc_rows != SYSTEMATIC_QC_ROWS ||
        tail_rows != SYSTEMATIC_TAIL_ROWS || public_cols != PUBLIC_T_COLUMNS ||
        (qc_rows % l) != 0 || (public_cols % l) != 0 ||
        tail_rows != 2) {
        fail_stage = "parameter check";
        if (getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL) {
            fprintf(stderr,
                    "decode_map_from_pk failed: %s r=%d CODIMENSION=%d qc=%d/%d tail=%d/%d public=%d/%d\n",
                    fail_stage, r, CODIMENSION, qc_rows, SYSTEMATIC_QC_ROWS,
                    tail_rows, SYSTEMATIC_TAIL_ROWS, public_cols,
                    PUBLIC_T_COLUMNS);
        }
        return FALSE;
    }

    full_H = alloc_binary_matrix(gf_extd() * t, n);
    hybrid_H = alloc_binary_matrix(r, n);
    identity_perm = malloc((size_t)n * sizeof(*identity_perm));
    if (full_H == NULL || hybrid_H == NULL || identity_perm == NULL) {
        fail_stage = "allocation";
        goto cleanup;
    }

    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(eta_local, eta);
    } else {
        gf_set_to_unit(eta_local);
    }
    col = poly_alloc(t - 1);
    sun_col = poly_alloc(t - 1);
    if (col == NULL || sun_col == NULL) {
        fail_stage = "poly allocation";
        goto cleanup;
    }

    for (i = 0; i < n; i++) {
        poly_syndrome_twisted_explicit_row(col, L + i, g, eta_local,
                                           scheme_selected_twist_row(t));

        /* Map ciphertext syndromes back to the compact Sun basis; KEM
         * converts that basis to the Sui definition before decoding. */
        poly_set_to_zero(sun_col);
        for (j = 0; j < matrix_rows; j++) {
            poly_set_coeff(sun_col, j, poly_coeff(col, j));
        }
        poly_set_coeff_to_unit(sun_col, t - 1);
        poly_calcule_deg(sun_col);
        for (j = 0; j < t; j++) {
            x = gf_to_index(poly_coeff(sun_col, j));
            for (k = 0; k < gf_extd(); k++) {
                if (x & (1 << k)) {
                    int full_row = j * gf_extd() + k;

                    matrix_set_bit(full_H, full_row, i);
                }
            }
        }
    }

    for (i = 0; i < qc_rows; i++) {
        matrix_set_bit(hybrid_H, i, i);
        for (j = 0; j < public_cols; j++) {
            if (public_key_t_bit(pk, i, j, l, qc_rows, public_cols)) {
                matrix_set_bit(hybrid_H, i, qc_rows + j);
            }
        }
    }
    for (i = 0; i < tail_rows; i++) {
        int row = qc_rows + i;

        for (j = 0; j < public_cols; j++) {
            if (public_key_t_bit(pk, row, j, l, qc_rows, public_cols)) {
                matrix_set_bit(hybrid_H, row, qc_rows + j);
            }
        }
    }
    for (i = 0; i < n; i++) {
        identity_perm[i] = i;
    }

    if (!build_hybrid_decode_map(decode_map, full_H, hybrid_H, n,
                                 identity_perm, qc_rows, r)) {
        fail_stage = "build_hybrid_decode_map";
        goto cleanup;
    }
    if (getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_VERIFY_DECODE_MAP") != NULL &&
        !verify_hybrid_decode_map(decode_map, full_H, hybrid_H, n, r)) {
        fail_stage = "verify_hybrid_decode_map";
        goto cleanup;
    }
    ok = TRUE;

cleanup:
    if (col != NULL) {
        poly_free(col);
    }
    if (sun_col != NULL) {
        poly_free(sun_col);
    }
    free_binary_matrix(full_H, gf_extd() * t);
    free_binary_matrix(hybrid_H, r);
    free(identity_perm);
    if (!ok &&
        getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL) {
        fprintf(stderr, "decode_map_from_pk failed: %s\n", fail_stage);
    }
    return ok;
}

int goppa_public_syndrome_to_sun_syndrome(gfelt_t *L, poly_t g, int n,
                                          int l, int t, gf_t eta,
                                          const unsigned char *pk,
                                          const unsigned char *syndrome,
                                          unsigned char *sun_syndrome)
{
    int i;
    int j;
    int k;
    int matrix_rows = scheme_matgen_field_rows(t);
    int qc_rows = hybrid_qc_rows(gf_extd(), t, l);
    int tail_rows = hybrid_tail_rows(gf_extd(), t, l);
    int r = hybrid_total_rows(gf_extd(), t);
    int public_cols = n - qc_rows;
    int *identity_perm = NULL;
    gf_t eta_local;
    gfindex_t x;
    unsigned long **hybrid_H = NULL;
    unsigned long **full_H = NULL;
    poly_t col = NULL;
    poly_t sun_col = NULL;
    int ok = FALSE;
    const char *fail_stage = "entry";

    if (getenv("QCTM_USE_OLD_DIRECTDEC") == NULL) {
        return goppa_public_syndrome_to_sun_syndrome_cached(
            L, g, n, l, t, eta, pk, syndrome, sun_syndrome);
    }

    if (L == NULL || g == NULL || pk == NULL || syndrome == NULL ||
        sun_syndrome == NULL || r != CODIMENSION ||
        qc_rows != SYSTEMATIC_QC_ROWS ||
        tail_rows != SYSTEMATIC_TAIL_ROWS || public_cols != PUBLIC_T_COLUMNS ||
        (qc_rows % l) != 0 || (public_cols % l) != 0 ||
        tail_rows != 2) {
        fail_stage = "parameter check";
        if (getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL) {
            fprintf(stderr,
                    "direct_syndrome_from_pk failed: %s r=%d CODIMENSION=%d qc=%d/%d tail=%d/%d public=%d/%d\n",
                    fail_stage, r, CODIMENSION, qc_rows, SYSTEMATIC_QC_ROWS,
                    tail_rows, SYSTEMATIC_TAIL_ROWS, public_cols,
                    PUBLIC_T_COLUMNS);
        }
        return FALSE;
    }

    full_H = alloc_binary_matrix(gf_extd() * t, n);
    hybrid_H = alloc_binary_matrix(r, n);
    identity_perm = malloc((size_t)n * sizeof(*identity_perm));
    if (full_H == NULL || hybrid_H == NULL || identity_perm == NULL) {
        fail_stage = "allocation";
        goto cleanup;
    }

    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(eta_local, eta);
    } else {
        gf_set_to_unit(eta_local);
    }
    col = poly_alloc(t - 1);
    sun_col = poly_alloc(t - 1);
    if (col == NULL || sun_col == NULL) {
        fail_stage = "poly allocation";
        goto cleanup;
    }

    for (i = 0; i < n; i++) {
        poly_syndrome_twisted_explicit_row(col, L + i, g, eta_local,
                                           scheme_selected_twist_row(t));
        poly_set_to_zero(sun_col);
        for (j = 0; j < matrix_rows; j++) {
            poly_set_coeff(sun_col, j, poly_coeff(col, j));
        }
        poly_set_coeff_to_unit(sun_col, t - 1);
        poly_calcule_deg(sun_col);
        for (j = 0; j < t; j++) {
            x = gf_to_index(poly_coeff(sun_col, j));
            for (k = 0; k < gf_extd(); k++) {
                if (x & (1 << k)) {
                    matrix_set_bit(full_H, j * gf_extd() + k, i);
                }
            }
        }
    }

    for (i = 0; i < qc_rows; i++) {
        matrix_set_bit(hybrid_H, i, i);
        for (j = 0; j < public_cols; j++) {
            if (public_key_t_bit(pk, i, j, l, qc_rows, public_cols)) {
                matrix_set_bit(hybrid_H, i, qc_rows + j);
            }
        }
    }
    for (i = 0; i < tail_rows; i++) {
        int row = qc_rows + i;

        for (j = 0; j < public_cols; j++) {
            if (public_key_t_bit(pk, row, j, l, qc_rows, public_cols)) {
                matrix_set_bit(hybrid_H, row, qc_rows + j);
            }
        }
    }
    for (i = 0; i < n; i++) {
        identity_perm[i] = i;
    }

    if (!apply_hybrid_decode_rows_direct(sun_syndrome, full_H, hybrid_H,
                                         syndrome, n, identity_perm,
                                         qc_rows, r)) {
        fail_stage = "apply_hybrid_decode_rows_direct";
        goto cleanup;
    }
    ok = TRUE;

cleanup:
    if (col != NULL) {
        poly_free(col);
    }
    if (sun_col != NULL) {
        poly_free(sun_col);
    }
    free_binary_matrix(full_H, gf_extd() * t);
    free_binary_matrix(hybrid_H, r);
    free(identity_perm);
    if (!ok &&
        getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL) {
        fprintf(stderr, "direct_syndrome_from_pk failed: %s\n", fail_stage);
    }
    return ok;
}

int goppa_decode_map_from_systematic_support(gfelt_t *L, poly_t g, int n,
                                             int l, int t, gf_t eta,
                                             unsigned char *pk,
                                             unsigned char *decode_map)
{
    int i;
    int j;
    int k;
    int matrix_rows = scheme_matgen_field_rows(t);
    int qc_rows = hybrid_qc_rows(gf_extd(), t, l);
    int tail_rows = hybrid_tail_rows(gf_extd(), t, l);
    int r = hybrid_total_rows(gf_extd(), t);
    int public_cols = n - qc_rows;
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;
    int *identity_perm = NULL;
    gf_t eta_local;
    gfindex_t x;
    unsigned long **H = NULL;
    unsigned long **full_H = NULL;
    poly_t col = NULL;
    poly_t sun_col = NULL;
    const char *fail_stage = "entry";
    int pk_bit;
    int ok = FALSE;

    if (L == NULL || g == NULL || decode_map == NULL ||
        r != CODIMENSION || qc_rows != SYSTEMATIC_QC_ROWS ||
        tail_rows != SYSTEMATIC_TAIL_ROWS || public_cols != PUBLIC_T_COLUMNS ||
        (qc_rows % l) != 0 || (public_cols % l) != 0 ||
        tail_rows != 2) {
        return FALSE;
    }

    H = calloc((size_t)r, sizeof(*H));
    full_H = calloc((size_t)(gf_extd() * t), sizeof(*full_H));
    identity_perm = malloc((size_t)n * sizeof(*identity_perm));
    if (H == NULL || full_H == NULL || identity_perm == NULL) {
        fail_stage = "allocation";
        goto cleanup;
    }
    for (i = 0; i < r; i++) {
        H[i] = calloc((size_t)rwdcnt, sizeof(**H));
        if (H[i] == NULL) {
            fail_stage = "hybrid matrix allocation";
            goto cleanup;
        }
    }
    for (i = 0; i < gf_extd() * t; i++) {
        full_H[i] = calloc((size_t)rwdcnt, sizeof(**full_H));
        if (full_H[i] == NULL) {
            fail_stage = "full matrix allocation";
            goto cleanup;
        }
    }

    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(eta_local, eta);
    } else {
        gf_set_to_unit(eta_local);
    }
    col = poly_alloc(t - 1);
    sun_col = poly_alloc(t - 1);
    if (col == NULL || sun_col == NULL) {
        fail_stage = "poly allocation";
        goto cleanup;
    }

    for (i = 0; i < n; i++) {
        poly_syndrome_twisted_explicit_row(col, L + i, g, eta_local,
                                           scheme_selected_twist_row(t));

        /* Map ciphertext syndromes back to the compact Sun basis; KEM
         * converts that basis to the Sui definition before decoding. */
        poly_set_to_zero(sun_col);
        for (j = 0; j < matrix_rows; j++) {
            poly_set_coeff(sun_col, j, poly_coeff(col, j));
        }
        poly_set_coeff_to_unit(sun_col, t - 1);
        poly_calcule_deg(sun_col);
        for (j = 0; j < t; j++) {
            x = gf_to_index(poly_coeff(sun_col, j));
            for (k = 0; k < gf_extd(); k++) {
                if (x & (1 << k)) {
                    int full_row = j * gf_extd() + k;

                    full_H[full_row][i / __WORDSIZE] ^=
                        (1UL << (i % __WORDSIZE));
                }
            }
        }

        for (j = 0; j < matrix_rows; j++) {
            x = gf_to_index(poly_coeff(col, j));
            for (k = 0; k < gf_extd(); k++) {
                if (x & (1 << k)) {
                    int full_row = j * gf_extd() + k;

                    if (full_row < qc_rows) {
                        H[full_row][i / __WORDSIZE] ^=
                            (1UL << (i % __WORDSIZE));
                    } else {
                        int tail_row = qc_rows + (full_row - qc_rows);

                        H[tail_row][i / __WORDSIZE] ^=
                            (1UL << (i % __WORDSIZE));
                    }
                }
            }
        }
        H[r - 1][i / __WORDSIZE] ^= (1UL << (i % __WORDSIZE));
    }

    gausselim(H, qc_rows, n);
    if (qc_systematic_form_fixed_columns(H, qc_rows, n, l) != qc_rows) {
        fail_stage = "fixed-column systematic form";
        goto cleanup;
    }
    for (i = 0; i < n; i++) {
        identity_perm[i] = i;
    }
    if (!hybrid_t1_l_cyclic_public_part(H, qc_rows, tail_rows, l,
                                        identity_perm, n)) {
        fail_stage = "T1 cyclic check";
        goto cleanup;
    }

    for (j = 0; j < tail_rows; j++) {
        int row = qc_rows + j;

        for (i = 0; i < qc_rows; i++) {
            if (coeff(H, row, i)) {
                xor_long(H[row], H[i], rwdcnt);
            }
        }
    }

    if (!build_hybrid_decode_map(decode_map, full_H, H, n, identity_perm,
                                 qc_rows, r)) {
        fail_stage = "build_hybrid_decode_map";
        goto cleanup;
    }
    if (getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_VERIFY_DECODE_MAP") != NULL &&
        !verify_hybrid_decode_map(decode_map, full_H, H, n, r)) {
        fail_stage = "verify_hybrid_decode_map";
        goto cleanup;
    }

    if (pk != NULL) {
        memset(pk, 0, PUBLICKEY_BYTES);
        pk_bit = 0;
        for (i = 0; i < qc_rows; i += l) {
            for (j = 0; j < public_cols; j++) {
                if (coeff(H, i, qc_rows + j)) {
                    pk_set_bit(pk, pk_bit);
                }
                pk_bit++;
            }
        }
        for (i = 0; i < tail_rows; i++) {
            for (j = 0; j < public_cols; j++) {
                if (coeff(H, qc_rows + i, qc_rows + j)) {
                    pk_set_bit(pk, pk_bit);
                }
                pk_bit++;
            }
        }
    }
    ok = TRUE;

cleanup:
    if (col != NULL) {
        poly_free(col);
    }
    if (sun_col != NULL) {
        poly_free(sun_col);
    }
    if (H != NULL) {
        for (i = 0; i < r; i++) {
            free(H[i]);
        }
    }
    if (full_H != NULL) {
        for (i = 0; i < gf_extd() * t; i++) {
            free(full_H[i]);
        }
    }
    free(H);
    free(full_H);
    free(identity_perm);
    if (!ok &&
        getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL) {
        fprintf(stderr, "decode_map_from_systematic_support failed: %s\n",
                fail_stage);
    }
    return ok;
}

int goppa_keygen(gfelt_t * L, poly_t g, int n, int l, int t,
                 gf_t eta, unsigned char * pk, unsigned char *decode_map) {
    int i;
    int j;
    int k;
    int matrix_rows = scheme_matgen_field_rows(t);
    int qc_rows = hybrid_qc_rows(gf_extd(), t, l);
    int tail_rows = hybrid_tail_rows(gf_extd(), t, l);
    int r = hybrid_total_rows(gf_extd(), t);
    int public_cols = n - qc_rows;
    int rwdcnt = 1 + (n - 1) / __WORDSIZE;
    int *perm = NULL;
    gfelt_t *Laux = NULL;
    gf_t eta_local;
    gfindex_t x;
    unsigned long **H = NULL;
    unsigned long **full_H = NULL;
    unsigned long *H_storage = NULL;
    unsigned long *full_H_storage = NULL;
    poly_t col = NULL;
    poly_t sun_col = NULL;
    poly_t def_col = NULL;
    int pk_bit;
    int profile = qctm_profile_keygen_enabled();
    double t_start = 0.0;
    double t_after_alloc = 0.0;
    double t_after_matgen = 0.0;
    double t_after_gauss = 0.0;
    double t_after_qc = 0.0;
    double t_after_t1 = 0.0;
    double t_after_tail = 0.0;
    double t_after_map = 0.0;
    double t_after_pack = 0.0;
    double t_end = 0.0;

    if (r != CODIMENSION || qc_rows != SYSTEMATIC_QC_ROWS ||
        tail_rows != SYSTEMATIC_TAIL_ROWS || public_cols != PUBLIC_T_COLUMNS ||
        (qc_rows % l) != 0 || (public_cols % l) != 0 ||
        tail_rows != 2) {
        return 0;
    }

    if (scheme_trace_keygen_enabled()) {
        fprintf(stderr,
                "MatGen: building hybrid partial H qc_rows=%d tail_rows=%d public_cols=%d n=%d l=%d\n",
                qc_rows, tail_rows, public_cols, n, l);
    }
    if (profile) {
        qctm_profile_keygen_active = 1;
        t_start = qctm_profile_now_ms();
    }

    H = calloc((size_t)r, sizeof(*H));
    H_storage = calloc((size_t)r * (size_t)rwdcnt, sizeof(*H_storage));
    if (decode_map != NULL) {
        full_H = calloc((size_t)(gf_extd() * t), sizeof(*full_H));
        full_H_storage = calloc((size_t)(gf_extd() * t) * (size_t)rwdcnt,
                                sizeof(*full_H_storage));
    }
    if (H == NULL || H_storage == NULL ||
        (decode_map != NULL && (full_H == NULL || full_H_storage == NULL))) {
        goto fail;
    }
    for (i = 0; i < r; i++) {
        H[i] = H_storage + (size_t)i * (size_t)rwdcnt;
    }
    if (decode_map != NULL) {
        for (i = 0; i < gf_extd() * t; i++) {
            full_H[i] = full_H_storage + (size_t)i * (size_t)rwdcnt;
        }
    }

    if (eta != NULL && !gf_is_zero(eta)) {
        gf_set(eta_local, eta);
    } else {
        gf_set_to_unit(eta_local);
    }
    col = poly_alloc(t - 1);
    if (decode_map != NULL) {
        sun_col = poly_alloc(t - 1);
        def_col = poly_alloc(t - 1);
    }
    if (col == NULL ||
        (decode_map != NULL && (sun_col == NULL || def_col == NULL))) {
        goto fail;
    }
    if (profile) {
        t_after_alloc = qctm_profile_now_ms();
    }

    for (i = 0; i < n; i++) {
        poly_syndrome_twisted_explicit_row(col, L + i, g, eta_local,
                                           scheme_selected_twist_row(t));

        if (decode_map != NULL) {
            poly_set_to_zero(sun_col);
            for (j = 0; j < matrix_rows; j++) {
                poly_set_coeff(sun_col, j, poly_coeff(col, j));
            }
            poly_set_coeff_to_unit(sun_col, t - 1);
            poly_calcule_deg(sun_col);
            if (!sun_syndrome_to_sui_definition(def_col, sun_col, g)) {
                goto fail;
            }
            for (j = 0; j < t; j++) {
                x = gf_to_index(poly_coeff(def_col, j));
                matrix_xor_gf_bits(full_H, j * gf_extd(), i, x);
            }
        }

        for (j = 0; j < matrix_rows; j++) {
            x = gf_to_index(poly_coeff(col, j));
            if (j * gf_extd() < qc_rows) {
                matrix_xor_gf_bits(H, j * gf_extd(), i, x);
            } else {
                matrix_xor_gf_bits(H, qc_rows + (j * gf_extd() - qc_rows),
                                   i, x);
            }
        }
        H[r - 1][i / __WORDSIZE] ^= (1UL << (i % __WORDSIZE));
    }
    if (profile) {
        t_after_matgen = qctm_profile_now_ms();
    }
    poly_free(col);
    col = NULL;
    if (sun_col != NULL) {
        poly_free(sun_col);
        sun_col = NULL;
    }
    if (def_col != NULL) {
        poly_free(def_col);
        def_col = NULL;
    }

    perm = malloc((size_t)n * sizeof(*perm));
    if (perm == NULL) {
        goto fail;
    }

    gausselim(H, qc_rows, n);
    if (profile) {
        t_after_gauss = qctm_profile_now_ms();
    }
    k = qc_systematic_form(H, qc_rows, n, l, perm);
    if (profile) {
        t_after_qc = qctm_profile_now_ms();
    }
    if (k != qc_rows) {
        if (scheme_trace_keygen_enabled()) {
            fprintf(stderr, "MatGen: QC body systematic failed k=%d\n", k);
        }
        goto fail_rank;
    }
    if (!hybrid_t1_l_cyclic_public_part(H, qc_rows, tail_rows, l, perm, n)) {
        if (scheme_trace_keygen_enabled()) {
            fprintf(stderr, "MatGen: T1 cyclic check failed\n");
        }
        goto fail_rank;
    }
    if (profile) {
        t_after_t1 = qctm_profile_now_ms();
    }

    for (j = 0; j < tail_rows; j++) {
        int row = qc_rows + j;

        for (i = 0; i < qc_rows; i++) {
            if (coeff(H, row, perm[i])) {
                xor_long(H[row], H[i], rwdcnt);
            }
        }
    }
    if (profile) {
        t_after_tail = qctm_profile_now_ms();
    }

    if (decode_map != NULL &&
        !build_hybrid_decode_map(decode_map, full_H, H, n, perm,
                                 qc_rows, r)) {
        if (scheme_trace_keygen_enabled()) {
            fprintf(stderr, "MatGen: decode map failed\n");
        }
        goto fail;
    }
    if (decode_map != NULL && getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_VERIFY_DECODE_MAP") != NULL &&
        !verify_hybrid_decode_map(decode_map, full_H, H, n, r)) {
        goto fail;
    }
    if (profile) {
        t_after_map = qctm_profile_now_ms();
    }

    Laux = malloc((size_t)n * sizeof(*Laux));
    if (Laux == NULL) {
        goto fail;
    }
    for (i = 0; i < qc_rows; i++) {
        Laux[i] = L[perm[i]];
    }
    for (i = 0; i < public_cols; i++) {
        Laux[qc_rows + i] = L[perm[qc_rows + i]];
    }
    for (i = 0; i < n; i++) {
        L[i] = Laux[i];
    }

    memset(pk, 0, PUBLICKEY_BYTES);
    pk_bit = 0;
    for (i = 0; i < qc_rows; i += l) {
        for (j = 0; j < public_cols; j++) {
            if (coeff(H, i, perm[qc_rows + j])) {
                pk_set_bit(pk, pk_bit);
            }
            pk_bit++;
        }
    }
    for (i = 0; i < tail_rows; i++) {
        for (j = 0; j < public_cols; j++) {
            if (coeff(H, qc_rows + i, perm[qc_rows + j])) {
                pk_set_bit(pk, pk_bit);
            }
            pk_bit++;
        }
    }
    if (profile) {
        t_after_pack = qctm_profile_now_ms();
    }

    free(H_storage);
    free(full_H_storage);
    free(H);
    free(full_H);
    free(Laux);
    free(perm);
    if (profile) {
        t_end = qctm_profile_now_ms();
        fprintf(stderr,
                "qctm_keygen_profile goppa_keygen_ms alloc=%.3f matgen=%.3f gauss=%.3f qc=%.3f t1=%.3f tail=%.3f map=%.3f pack=%.3f cleanup=%.3f total=%.3f\n",
                t_after_alloc - t_start,
                t_after_matgen - t_after_alloc,
                t_after_gauss - t_after_matgen,
                t_after_qc - t_after_gauss,
                t_after_t1 - t_after_qc,
                t_after_tail - t_after_t1,
                t_after_map - t_after_tail,
                t_after_pack - t_after_map,
                t_end - t_after_pack,
                t_end - t_start);
        fflush(stderr);
        qctm_profile_keygen_active = 0;
    }

    return r;

fail_rank:
    if (scheme_trace_keygen_enabled()) {
        fprintf(stderr, "MatGen: hybrid systematic form failed\n");
    }
fail:
    qctm_profile_keygen_active = 0;
    if (col != NULL) {
        poly_free(col);
    }
    if (sun_col != NULL) {
        poly_free(sun_col);
    }
    if (def_col != NULL) {
        poly_free(def_col);
    }
    free(H_storage);
    free(full_H_storage);
    free(H);
    free(full_H);
    free(Laux);
    free(perm);

    return 0;
}



int goppa_check_params(int n, int m, int l, int t) {
    return ( (n > 0) && (m > 0) && (t > 0) &&
         (n <= (1 << m)) && (t * m < n) &&
         (m == gf_init(m)) &&
         ((gf_card() - 1) % l == 0) && (n % l == 0) &&
         (t % l == 0) );
}



goppa_t goppa_keygen_rand(int n, int l, int m, int t,
                          gf_t eta, unsigned char ** pk,
                          unsigned char *decode_map) {


    if (! goppa_check_params(n, m, l, t))
        return NULL;

    if (eta == NULL || gf_is_zero(eta))
        return NULL;

    gfelt_t * L = goppa_rand_support(n, l, eta);
    
    if (L == NULL)
        return NULL;




		poly_t g = NULL;
    if (l != 1) {
			int smallestFactorDegree=0;
            gf_t shift;
        
            gf_inv(shift, eta);
			poly_t xl = x_plus_c_pow_l(l, shift);
            if (xl == NULL) {
                free(L);
                return NULL;
            }
			while (smallestFactorDegree <t){
				poly_t aux_poly = poly_randgen_irred(t/l);
				g = poly_compose(aux_poly, xl);
				poly_free(aux_poly);
				smallestFactorDegree=poly_degppf(g);
				if (smallestFactorDegree < t)
					poly_free(g);
			}
			poly_free(xl);
    } else {
			g = poly_randgen_irred(t);
    }    

		
    if (g == NULL) {
        free(L);
        return NULL;
    }
    

    int alloc_pk = ((pk) && (*pk == NULL));
    if (alloc_pk) {
        *pk = malloc(PUBLICKEY_BYTES);
        if (*pk == NULL) {
            free(L);
            poly_free(g);
            return NULL;
        }
    }
    
    if (pk) {
        if (goppa_keygen(L, g, n, l, t, eta, *pk, decode_map) != CODIMENSION) {
            free(L);
            poly_free(g);
            if (alloc_pk) {
                free(*pk);
            }
            *pk = NULL;
            return NULL;
        }
    }
    
    goppa_t gamma = goppa_init(L, g, n, t, l, NULL, eta);
    if (gamma == NULL) {
        free(L);
        poly_free(g);
        if (pk) {
            if (alloc_pk) {
                free(*pk);
            }
            *pk = NULL;
        }
        return NULL;
    }
    
    return gamma;
}
