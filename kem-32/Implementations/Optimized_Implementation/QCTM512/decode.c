#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "goppa.h"
#include "scheme_api.h"
#include "../qctm_portable.h"

static int scheme_decode_twist_row(int t)
{
    return t > 0 ? t - 1 : 0;
}

static double qctm_decode_now_ms(void)
{
    return qctm_now_ms();
}


void poly_syndrome_patterson(poly_t f, gf_t b, poly_t g) {
    int i, t;
    gf_t a, c;

    t = poly_deg(g);
	if (gf_is_zero(b)) {
		gf_inv(a, poly_coeff(g, 0));
		for (i = 0; i < t; i++) {
			gf_mul_fast(c, a, poly_coeff(g, i + 1));
			poly_set_coeff(f, i, c);
		}
	}
	else {
		poly_set_coeff_to_unit(f, t - 1);
		for (i = t - 2; i >= 0; i--) {
			gf_mul_fast(a, b, poly_coeff(f, i + 1));
			gf_add(a, a, poly_coeff(g, i + 1));
			poly_set_coeff(f, i, a);
		}
		gf_mul_fast(a, b, poly_coeff(f, 0));
		gf_add(c, a, poly_coeff(g, 0));

		gf_inv(a, c);
		for (i = 0; i < t; i++) {
			gf_mul_fast(c, a, poly_coeff(f, i));
			poly_set_coeff(f, i, c);
		}
	}

}

static void gf_pow_small(gf_t out, gf_t x, int e)
{
    int i;

    gf_set_to_unit(out);
    for (i = 0; i < e; i++) {
        gf_mul(out, out, x);
    }
}

static void poly_free_if(poly_t p)
{
    if (p != NULL) {
        poly_free(p);
    }
}

static void poly_eval_safe(gf_t res, poly_t p, gf_t a)
{
    if (p == NULL || poly_deg(p) < 0) {
        gf_set_to_zero(res);
        return;
    }
    poly_eval(res, p, a);
}

static int goppa_locate_error_twisted_berl(poly_t sigma, int *e,
                                           goppa_t gamma, int target,
                                           int trace_dec)
{
    int i;
    int d;
    int weight = 0;
    int profile_dec = getenv("QCTM_PROFILE_DEC") != NULL;
    double t0 = 0.0;
    double t_roots = 0.0;
    gfelt_t *roots;

    roots = (gfelt_t *) malloc(target * sizeof(gfelt_t));
    if (roots == NULL) {
        return -1;
    }

    if (profile_dec) {
        t0 = qctm_decode_now_ms();
    }
    d = roots_berl(sigma, roots);
    if (profile_dec) {
        t_roots = qctm_decode_now_ms();
    }
    if (d != target) {
        if (trace_dec) {
            fprintf(stderr, "decode: roots_berl returned %d roots, target=%d\n",
                    d, target);
        }
        free(roots);
        return -1;
    }

    for (i = 0; i < d; i++) {
        gfindex_t idx = gf_to_index(roots + i);
        int pos = gamma->Linv[idx];

        if (pos < 0 || pos >= gamma->length ||
            !gf_eq(gamma->L + pos, roots + i)) {
            if (trace_dec) {
                fprintf(stderr, "decode: root outside support i=%d idx=%u pos=%d\n",
                        i, (unsigned)idx, pos);
            }
            free(roots);
            return -1;
        }

        e[weight++] = pos;
    }

    free(roots);
    if (profile_dec) {
        double t_end = qctm_decode_now_ms();
        fprintf(stderr,
                "qctm_locate_profile_ms roots_berl=%.3f forney=%.3f roots=%d\n",
                t_roots > 0.0 ? t_roots - t0 : 0.0,
                t_roots > 0.0 ? t_end - t_roots : 0.0,
                weight);
    }
    return weight == target ? weight : -1;
}

void poly_syndrome_twisted(poly_t f, gf_t b, poly_t g, gf_t eta)
{
    int t;
    gf_t gb, inv_gb, b_pow, twist;

    /*
     * Sui--Yue Definition 2.1 column:
     *   (x-b)^(-1) - eta*b^t/g(b) mod g(x).
     * The coefficient matrix built from these columns is row-equivalent to
     * their Proposition 2.2 parity-check matrix, whose last row is
     * (b^(t-1) + eta*b^t)/g(b).
     */
    poly_syndrome_patterson(f, b, g);
    t = poly_deg(g);
    poly_eval(gb, g, b);
    if (gf_is_zero(gb)) {
        poly_set_to_zero(f);
        return;
    }
    gf_inv(inv_gb, gb);
    gf_pow_small(b_pow, b, t);
    gf_mul(twist, b_pow, inv_gb);
    gf_mul(twist, twist, eta);
    poly_addto_coeff(f, 0, twist);
    poly_calcule_deg(f);
}

void poly_syndrome_twisted_explicit(poly_t f, gf_t b, poly_t g, gf_t eta)
{
    poly_syndrome_twisted_explicit_row(f, b, g, eta,
                                       scheme_decode_twist_row(poly_deg(g)));
}

void poly_syndrome_twisted_explicit_row(poly_t f, gf_t b, poly_t g,
                                        gf_t eta, int twist_row)
{
    int i;
    int t;
    gf_t gb;
    gf_t inv_gb;
    gf_t b_pow;
    gf_t value;

    t = poly_deg(g);
    if (twist_row < 0 || twist_row >= t) {
        twist_row = t - 1;
    }
    poly_eval(gb, g, b);
    if (gf_is_zero(gb)) {
        poly_set_to_zero(f);
        return;
    }
    if (f->size > t) {
        memset(f->coeff + t, 0, (size_t)(f->size - t) * sizeof(*f->coeff));
    }

    gf_inv(inv_gb, gb);
    gf_set_to_unit(b_pow);
    for (i = 0; i < t; i++) {
        gf_mul(value, b_pow, inv_gb);
        poly_set_coeff(f, i, value);
        gf_mul(b_pow, b_pow, b);
    }

    gf_mul(value, b_pow, eta);
    gf_mul(value, value, inv_gb);
    poly_addto_coeff(f, twist_row, value);
    poly_calcule_deg(f);
}

static poly_t poly_derivative_compact(poly_t p)
{
    int i;
    int dmax = poly_deg(p) > 0 ? (poly_deg(p) - 1) / 2 : 0;
    poly_t d = poly_alloc(dmax);

    if (d == NULL) {
        return NULL;
    }
    for (i = 1; i <= poly_deg(p); i += 2) {
        poly_set_coeff(d, (i - 1) / 2, poly_coeff(p, i));
    }
    poly_calcule_deg(d);
    return d;
}

static int twisted_forney_polynomial_check(poly_t sigma, poly_t tau,
                                           poly_t sigma_der)
{
    int i;
    int max_deg = poly_deg(tau);
    poly_t check;
    gf_t scale;
    gf_t term;

    if (poly_deg(sigma) < 0 || sigma_der == NULL) {
        return 0;
    }
    if (poly_deg(sigma_der) >= 0 && 2 * poly_deg(sigma_der) > max_deg) {
        max_deg = 2 * poly_deg(sigma_der);
    }
    if (poly_deg(sigma) > max_deg) {
        max_deg = poly_deg(sigma);
    }
    check = poly_alloc(max_deg);
    if (check == NULL) {
        return 0;
    }
    poly_set(check, tau);
    for (i = 0; i <= poly_deg(sigma_der); i++) {
        poly_addto_coeff(check, 2 * i, poly_coeff(sigma_der, i));
    }
    poly_calcule_deg(check);
    if (poly_deg(check) < 0) {
        poly_free(check);
        return 1;
    }
    if (poly_deg(check) != poly_deg(sigma)) {
        poly_free(check);
        return 0;
    }

    gf_div(scale, poly_coeff(check, poly_deg(check)),
           poly_coeff(sigma, poly_deg(sigma)));
    for (i = 0; i <= poly_deg(sigma); i++) {
        gf_mul(term, scale, poly_coeff(sigma, i));
        if (!gf_eq(term, poly_coeff(check, i))) {
            poly_free(check);
            return 0;
        }
    }
    poly_free(check);
    return 1;
}

static int twisted_euclidean(poly_t g, poly_t S, int target,
                             poly_t *r_prev_out, poly_t *r_cur_out,
                             poly_t *w_prev_out, poly_t *w_cur_out)
{
    int work_degree;
    poly_t r_prev = NULL;
    poly_t r_cur = NULL;
    poly_t w_prev = NULL;
    poly_t w_cur = NULL;

    work_degree = poly_deg(g);
    r_prev = poly_copy(g);
    r_cur = poly_copy(S);
    w_prev = poly_alloc(work_degree);
    w_cur = poly_alloc(work_degree);
    if (r_prev == NULL || r_cur == NULL || w_prev == NULL || w_cur == NULL) {
        goto fail;
    }
    poly_set_coeff_to_unit(w_cur, 0);
    poly_set_deg(w_cur, 0);

    while (poly_deg(r_cur) > target) {
        int i;
        int j;
        int r_cur_deg;
        int w_cur_deg;
        gf_t inv_lc;
        gf_t q;
        gf_t term;
        poly_t tmp;

        if (poly_deg(r_cur) < 0) {
            goto fail;
        }
        r_cur_deg = poly_deg(r_cur);
        w_cur_deg = poly_deg(w_cur);
        gf_inv(inv_lc, poly_coeff(r_cur, r_cur_deg));

        for (j = poly_deg(r_prev) - r_cur_deg; j >= 0; j--) {
            gf_mul_fast(q, inv_lc, poly_coeff(r_prev, r_cur_deg + j));
            if (gf_is_zero(q)) {
                continue;
            }
            for (i = 0; i <= r_cur_deg; i++) {
                gf_mul_fast(term, q, poly_coeff(r_cur, i));
                poly_addto_coeff(r_prev, i + j, term);
            }
            if (w_cur_deg + j >= w_prev->size) {
                goto fail;
            }
            for (i = 0; i <= w_cur_deg; i++) {
                gf_mul_fast(term, q, poly_coeff(w_cur, i));
                poly_addto_coeff(w_prev, i + j, term);
            }
            if (w_cur_deg + j > poly_deg(w_prev)) {
                poly_set_deg(w_prev, w_cur_deg + j);
            }
        }
        poly_set_deg(r_prev, r_cur_deg - 1);
        poly_adjust_deg(r_prev);
        poly_adjust_deg(w_prev);

        tmp = r_prev;
        r_prev = r_cur;
        r_cur = tmp;
        tmp = w_prev;
        w_prev = w_cur;
        w_cur = tmp;

        if (poly_deg(w_cur) > work_degree) {
            goto fail;
        }
    }

    *r_prev_out = r_prev;
    *r_cur_out = r_cur;
    *w_prev_out = w_prev;
    *w_cur_out = w_cur;
    return 1;

fail:
    poly_free_if(r_prev);
    poly_free_if(r_cur);
    poly_free_if(w_prev);
    poly_free_if(w_cur);
    return 0;
}

int partition(int * tableau, int gauche, int droite, int pivot) {
    int i, temp;
    for (i = gauche; i < droite; i++) {
        if (tableau[i] <= pivot) {
            temp = tableau[i];
            tableau[i] = tableau[gauche];
            tableau[gauche] = temp;
            ++gauche;
        }
	}
    return gauche;
}

void quickSort(int * tableau, int gauche, int droite, int min, int max) {
  if (gauche < droite - 1) {
    int milieu = partition(tableau, gauche, droite, (max + min) / 2);
    quickSort(tableau, gauche, milieu, min, (max + min) / 2);
    quickSort(tableau, milieu, droite, (max + min) / 2, max);
  }
}

poly_t goppa_keyequation_patterson(poly_t R, goppa_t gamma) {
    int i, j;
    poly_t u, v, h, sigma, S, aux;
    gf_t a, b;





    if (poly_calcule_deg(R) < 0) {
	    sigma = poly_alloc(gamma->degree);
		poly_set_coeff_to_unit(sigma, 0);
		poly_set_deg(sigma, 0);
		return sigma;
	}
	
    poly_eeaux(&h ,&aux, R, gamma->g, 1);
    gf_inv(a, poly_coeff(aux,0));
    for (i = 0; i <= poly_deg(h); ++i) {
	    gf_mul_fast(b, a, poly_coeff(h, i));
        poly_set_coeff(h, i, b);
	}
    poly_free(aux);


	gf_set_to_unit(a);
    poly_addto_coeff(h, 1, a);


  S = poly_alloc(gamma->degree - 1);
  for (i = 0; i < gamma->degree; i++) {
		gf_sqrt(a, poly_coeff(h, i));
		if (i & 1) {
#ifndef SQRT_NO_PRECOMP
			aux = gamma->sqrtmod[i / 2];
#else
			if (i == 1) {
				aux = poly_copy(gamma->sqrtzmod);
			}
			else {
				poly_shiftmod(aux, gamma->g);
			}
#endif
			if (!gf_is_zero(a)) {
				for (j = 0; j < gamma->degree; j++) {
					gf_mul_fast(b, a, poly_coeff(aux, j));
					poly_addto_coeff(S, j, b);
				}
			}
		}
		else {
			poly_addto_coeff(S, i / 2, a);
		}
  }
  poly_calcule_deg(S);
  poly_free(h);

#ifdef SQRT_NO_PRECOMP
	poly_free(aux);
#endif


  poly_eeaux(&v, &u, S, gamma->g, gamma->degree / 2 + 1);
  poly_free(S);


  sigma = poly_alloc(gamma->degree);
  for (i = 0; i <= poly_deg(u); ++i) {
		gf_square(b, poly_coeff(u, i));
        poly_set_coeff(sigma, 2 * i, b);
  }
  for (i = 0; i <= poly_deg(v); ++i) {
		gf_square(b, poly_coeff(v, i));
        poly_set_coeff(sigma, 2 * i + 1, b);
    }
    poly_free(u);
    poly_free(v);

    poly_calcule_deg(sigma);

  return sigma;
}

void poly_to_bin_addto(poly_t p, unsigned long * pt, int degree) {
	int i, j, k, l;
	unsigned long c;
	for (i = 0, l = 0; l < degree; ++l, i += gf_extd()) {
		k = i / __WORDSIZE;
		j = i % __WORDSIZE;
		c = gf_to_index(poly_coeff(p, l));
		pt[k] ^=  c << j;
		if (__WORDSIZE - j < gf_extd()) {
			pt[k + 1] ^= c >> (__WORDSIZE - j);
		}
	}
}

void bin_to_poly(unsigned long * pt, poly_t p, int degree) {
	int i, j, k, l;
	unsigned long u;
	gf_t a;

	for (i = 0, l = 0; l < degree; ++l, i += gf_extd()) {
		k = i / __WORDSIZE;
		j = i % __WORDSIZE;
		u = pt[k] >> j;
		if (__WORDSIZE - j < gf_extd()) {
			u ^= pt[k + 1] << (__WORDSIZE - j);
		}
    u &= ((1UL << gf_extd()) - 1);
		gf_from_index(a, (gfindex_t) u);
    poly_set_coeff(p, l, a);
	}
}

static void bytes_to_poly(const unsigned char *pt, poly_t p, int degree)
{
	int i, j, l;
	gfindex_t u;
	gf_t a;

	for (i = 0, l = 0; l < degree; ++l, i += gf_extd()) {
		u = 0;
		for (j = 0; j < gf_extd(); j++) {
			if ((pt[(i + j) / 8] >> ((i + j) % 8)) & 1U) {
				u ^= (gfindex_t)(1U << j);
			}
		}
		gf_from_index(a, u);
		poly_set_coeff(p, l, a);
	}
	poly_calcule_deg(p);
}

static poly_t base_explicit_to_definition(poly_t S, poly_t g)
{
	int i, j, t;
	poly_t R;
	gf_t term, coeff;

	t = poly_deg(g);
	R = poly_alloc(t - 1);
	if (R == NULL) {
		return NULL;
	}

	for (i = 0; i < t; i++) {
		gf_set_to_zero(coeff);
		for (j = i + 1; j <= t; j++) {
			gf_mul(term, poly_coeff(g, j), poly_coeff(S, j - 1 - i));
			gf_add(coeff, coeff, term);
		}
		poly_set_coeff(R, i, coeff);
	}
	poly_calcule_deg(R);
	return R;
}

static poly_t twisted_explicit_to_definition(poly_t S, poly_t g, gf_t eta,
                                             int twist_row, int error_weight)
{
	int j, t;
	poly_t B;
	poly_t R;
	gf_t c, denom, inv_denom, one, term, sum;

	t = poly_deg(g);
	if (twist_row < 0 || twist_row >= t) {
		twist_row = t - 1;
	}

	gf_set_to_zero(sum);
	if ((error_weight & 1) != 0) {
		gf_set_to_unit(sum);
	}
	for (j = 0; j < t; j++) {
		gf_mul(term, poly_coeff(g, j), poly_coeff(S, j));
		gf_add(sum, sum, term);
	}

	gf_mul(c, eta, sum);
	gf_mul(denom, eta, poly_coeff(g, twist_row));
	gf_set_to_unit(one);
	gf_add(denom, denom, one);
	if (gf_is_zero(denom)) {
		return NULL;
	}
	gf_inv(inv_denom, denom);
	gf_mul(c, c, inv_denom);

	B = poly_copy(S);
	if (B == NULL) {
		return NULL;
	}
	poly_addto_coeff(B, twist_row, c);
	poly_calcule_deg(B);

	R = base_explicit_to_definition(B, g);
	poly_free(B);
	if (R == NULL) {
		return NULL;
	}
	poly_addto_coeff(R, 0, c);
	poly_calcule_deg(R);
	return R;
}


void goppa_decode_init(goppa_t gamma) {
    (void)gamma;
#ifndef PARITY_NO_PRECOMP
	if (gamma->parity == NULL) {
		if (gamma->parity) {
			free(gamma->parity[0]);
			free(gamma->parity);
		}

		int i, nb_synd, alloc_size, degree;
		unsigned long * synd;
		poly_t p;
		gfelt_t * L;

		degree = gamma->degree;
		alloc_size = 1 + (degree * gf_extd() - 1) / __WORDSIZE;

		L = gamma->L + (gamma->length - gamma->degree * gf_extd());
		nb_synd = gamma->degree * gf_extd();		

		gamma->parity = (unsigned long **) malloc(nb_synd * sizeof (unsigned long *));

		synd = (unsigned long *) calloc(nb_synd * alloc_size, sizeof (unsigned long));
		p = poly_alloc(degree - 1);
		for (i = 0; i < nb_synd; i++, synd += alloc_size) {
			poly_syndrome_twisted_explicit(p, L + i, gamma->g, gamma->eta);

			poly_to_bin_addto(p, synd, degree);
			gamma->parity[i] = synd;
		}
		poly_free(p);
	}
#endif
}

poly_t goppa_syndrome(const unsigned char * s, goppa_t gamma) {
	int i, alloc_size, nb_synd, degree;
	poly_t R;
	unsigned long * synd;

#ifdef PARITY_NO_PRECOMP
	gfelt_t * L;
#endif

	goppa_decode_init(gamma);

	degree = gamma->degree;
	alloc_size = 1 + (degree * gf_extd() - 1) / __WORDSIZE;

	if (gamma->explicit_syndrome_input == 2) {
		R = poly_alloc(degree);
		if (R == NULL) {
			return NULL;
		}
		bytes_to_poly(s, R, degree);
		return R;
	}

	if (gamma->explicit_syndrome_input) {
		poly_t converted;

		R = poly_alloc(degree);
		if (R == NULL) {
			return NULL;
		}
		bytes_to_poly(s, R, degree);
		converted = twisted_explicit_to_definition(R, gamma->g, gamma->eta,
		                                           scheme_decode_twist_row(degree),
		                                           ERROR_WEIGHT);
		poly_free(R);
		return converted;
	}



#ifdef PARITY_NO_PRECOMP
	L = gamma->L + (gamma->length - gamma->degree * gf_extd());
#endif
	nb_synd = gamma->degree * gf_extd();


	synd = (unsigned long *) calloc(alloc_size, sizeof (unsigned long));
	R = poly_alloc(degree );
	for (i = 0; i < nb_synd; i++) {
		if ((s[i / 8] >> (i % 8)) & 1) {
#ifndef PARITY_NO_PRECOMP
			xor_long(synd, gamma->parity[i], alloc_size);
#else
		poly_syndrome_twisted_explicit(R, L + i, gamma->g, gamma->eta);
			poly_to_bin_addto(R, synd, degree);
#endif
		}
	}
	bin_to_poly(synd, R, degree);
	poly_calcule_deg(R);
	free(synd);

	{
		poly_t converted = twisted_explicit_to_definition(R, gamma->g,
		                                                  gamma->eta,
		                                                  scheme_decode_twist_row(degree),
		                                                  ERROR_WEIGHT);
		poly_free(R);
		R = converted;
	}

  return R;
}


int goppa_locate_error(poly_t sigma, int * e, goppa_t gamma) {
	int i, d;
	gfelt_t * roots;

	roots = (gfelt_t *) malloc(gamma->degree * sizeof (gfelt_t));
	d = roots_berl(sigma, roots);

  for (i = 0; i < d; ++i) {
    e[i] = gamma->Linv[gf_to_index(roots + i)];
	}
	free(roots);


	quickSort(e, 0, d, 0, gamma->length);
  return d;
}



int goppa_decode(const unsigned char * b, int * e, goppa_t gamma) {
    int j;
    int weight = 0;
    int target = ERROR_WEIGHT;
    int trace_dec = getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL;
    int profile_dec = getenv("QCTM_PROFILE_DEC") != NULL;
    double t0 = 0.0;
    double t_syndrome = 0.0;
    double t_ee = 0.0;
    double t_derivative = 0.0;
    double t_locate = 0.0;
    gf_t eval_sigma;
    poly_t R = NULL;
    poly_t r_prev = NULL;
    poly_t r_cur = NULL;
    poly_t w_prev = NULL;
    poly_t w_cur = NULL;
    poly_t sigma_der = NULL;

    if (target <= 0) {
        return -1;
    }

    if (profile_dec) {
        t0 = qctm_decode_now_ms();
    }
    R = goppa_syndrome(b, gamma);
    if (profile_dec) {
        t_syndrome = qctm_decode_now_ms();
    }
    if (R == NULL || poly_deg(R) < 0) {
        if (trace_dec) {
            fprintf(stderr, "decode: empty syndrome deg=%d\n",
                    R == NULL ? -999 : poly_deg(R));
        }
        poly_free_if(R);
        return -1;
    }
    if (trace_dec) {
        fprintf(stderr, "decode: syndrome_deg=%d target=%d\n",
                poly_deg(R), target);
        fflush(stderr);
    }

    if (!twisted_euclidean(gamma->g, R, target,
                           &r_prev, &r_cur, &w_prev, &w_cur)) {
        if (trace_dec) {
            fprintf(stderr, "decode: twisted_euclidean failed\n");
        }
        goto cleanup;
    }
    if (profile_dec) {
        t_ee = qctm_decode_now_ms();
    }
    if (trace_dec) {
        fprintf(stderr, "decode: deg_r_cur=%d deg_w_cur=%d\n",
                poly_deg(r_cur), poly_deg(w_cur));
        fflush(stderr);
    }
    if (poly_deg(w_cur) < 0 || poly_deg(w_cur) > target) {
        if (trace_dec) {
            fprintf(stderr, "decode: locator degree out of range\n");
        }
        goto cleanup;
    }

    sigma_der = poly_derivative_compact(w_cur);
    if (sigma_der == NULL) {
        if (trace_dec) {
            fprintf(stderr, "decode: derivative allocation failed\n");
        }
        goto cleanup;
    }
    if (profile_dec) {
        t_derivative = qctm_decode_now_ms();
    }

    if (!twisted_forney_polynomial_check(w_cur, r_cur, sigma_der)) {
        if (trace_dec) {
            fprintf(stderr, "decode: forney polynomial check failed\n");
        }
        goto cleanup;
    }

    if (getenv("QCTM_LOCATE_SCAN") == NULL) {
        weight = goppa_locate_error_twisted_berl(w_cur, e, gamma, target,
                                                 trace_dec);
        if (weight < 0 && getenv("QCTM_LOCATE_BERL") != NULL) {
            goto cleanup;
        }
    }
    if (weight < 0 || getenv("QCTM_LOCATE_SCAN") != NULL) {
        weight = 0;
        for (j = 0; j < gamma->length; j++) {
            poly_eval_safe(eval_sigma, w_cur, gamma->L + j);
            if (!gf_is_zero(eval_sigma)) {
                continue;
            }
            if (weight >= target) {
                if (trace_dec) {
                    fprintf(stderr, "decode: too many roots at j=%d\n", j);
                }
                goto cleanup;
            }
            e[weight++] = j;
        }
    }
    if (profile_dec) {
        t_locate = qctm_decode_now_ms();
    }
    if (trace_dec) {
        fprintf(stderr, "decode: located_roots=%d\n", weight);
        fflush(stderr);
    }

cleanup:
    if (profile_dec) {
        double t_end = qctm_decode_now_ms();
        fprintf(stderr,
                "qctm_goppa_decode_profile_ms syndrome=%.3f ee=%.3f derivative=%.3f locate=%.3f cleanup_total=%.3f roots=%d\n",
                t_syndrome > 0.0 ? t_syndrome - t0 : 0.0,
                t_ee > 0.0 ? t_ee - t_syndrome : 0.0,
                t_derivative > 0.0 ? t_derivative - t_ee : 0.0,
                t_locate > 0.0 ? t_locate - t_derivative : 0.0,
                t_end - t0,
                weight);
    }
    if (weight == target) {
        quickSort(e, 0, weight, 0, gamma->length);
    } else {
        weight = -1;
    }
    poly_free_if(R);
    poly_free_if(r_prev);
    poly_free_if(r_cur);
    poly_free_if(w_prev);
    poly_free_if(w_cur);
    poly_free_if(sigma_der);
    return weight;
}
