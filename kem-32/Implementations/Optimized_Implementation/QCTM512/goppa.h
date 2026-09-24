#ifndef GOPPA_H
#define GOPPA_H
#define GOPPA_NO_PRECOMP

#include <limits.h>
#include <stdio.h>

#include "gf.h"
#include "poly.h"
#include "permutation.h"

#ifndef __WORDSIZE
#if ULONG_MAX == 0xffffffffffffffffUL
#define __WORDSIZE 64
#else
#define __WORDSIZE 32
#endif
#endif

#ifdef GOPPA_NO_PRECOMP
#define SQRT_NO_PRECOMP
#define PARITY_NO_PRECOMP
#endif



typedef struct goppa {
    
    int length, degree, order;
    poly_t g;
    gfelt_t * L;
    gf_t eta;
    int explicit_syndrome_input;
    
    gfindex_t * Linv;
    
    poly_t sqrtzmod;
#ifndef SQRT_NO_PRECOMP
    poly_t * sqrtmod;
#endif
#ifndef PARITY_NO_PRECOMP
    
    unsigned long ** parity;
#endif
} * goppa_t;


#define coeff(M, i, j) (M[i][(j) / __WORDSIZE] >> ((j) % __WORDSIZE) & 1)
#define addrowto(M, i, l) xor_long(M[l], M[i], rwdcnt)
#define swaprows(M, i, k) {unsigned long * pt = M[i]; M[i] = M[k]; M[k] = pt;}

goppa_t goppa_init(gfelt_t * L, poly_t g, int length, int degree,
                   int order, poly_t sqrtzmod, gf_t eta);
void free_goppa(goppa_t goppa);
int goppa_check_params(int n, int m, int l, int t);
int goppa_fixed_eta(int l, gf_t eta);
gfelt_t * goppa_rand_support(int n, int l, gf_t eta);
int gausselim(unsigned long ** M, int r, int c);
int gausselim_aux(unsigned long ** M, int r, int c, int ind_cols);
int gausselim_perm(unsigned long ** M, int r, int c, int * perm);
int qc_systematic_form(unsigned long ** M, int r, int n, int l, int * perm);
goppa_t goppa_keygen_rand(int n, int l, int m, int t,
                          gf_t eta, unsigned char ** pk,
                          unsigned char *decode_map);
int goppa_keygen(gfelt_t * L, poly_t g, int n, int l, int t,
                 gf_t eta, unsigned char * pk, unsigned char *decode_map);
int goppa_decode_map_from_public_key(gfelt_t * L, poly_t g, int n, int l,
                                     int t, gf_t eta,
                                     const unsigned char *pk,
                                     unsigned char *decode_map);
int goppa_public_syndrome_to_sun_syndrome(gfelt_t *L, poly_t g, int n,
                                          int l, int t, gf_t eta,
                                          const unsigned char *pk,
                                          const unsigned char *syndrome,
                                          unsigned char *sun_syndrome);
int goppa_decode_map_from_systematic_support(gfelt_t * L, poly_t g, int n,
                                             int l, int t, gf_t eta,
                                             unsigned char *pk,
                                             unsigned char *decode_map);
int goppa_matgen_audit(gfelt_t * L, poly_t g, int n, int l, int t,
                       gf_t eta, FILE *fp);


void xor_long(unsigned long * a, unsigned long * b, int n);
int goppa_decode(const unsigned char * s, int * e, goppa_t gamma);
void poly_syndrome_twisted(poly_t f, gf_t b, poly_t g, gf_t eta);
void poly_syndrome_twisted_explicit(poly_t f, gf_t b, poly_t g, gf_t eta);
void poly_syndrome_twisted_explicit_row(poly_t f, gf_t b, poly_t g,
                                        gf_t eta, int twist_row);


void roots_init();
void roots_clear();
int roots_berl(poly_t sigma, gfelt_t * res);

#endif
