#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gf.h"
#include "poly.h"

poly_t poly_alloc(int d) {
  poly_t p;

  p = (poly_t) malloc(sizeof (struct polynome));
  p->deg = -1;
  p->size = d + 1;
  p->coeff = (gfelt_t *) calloc(p->size, sizeof (gfelt_t));
  return p;
}


poly_t poly_alloc_from_string(int d, const unsigned char * s) {
  poly_t p;

  p = (poly_t) malloc(sizeof (struct polynome));
  p->deg = -1;
  p->size = d + 1;
  p->coeff = (gfelt_t *) s;
  return p;
}

poly_t poly_copy(poly_t p) {
  poly_t q;

  q = (poly_t) malloc(sizeof (struct polynome));
  q->deg = p->deg;
  q->size = p->size;
  q->coeff = (gfelt_t *) calloc(q->size, sizeof (gfelt_t));
  memcpy(q->coeff, p->coeff, p->size * sizeof (gfelt_t));
  return q;
}

void poly_free(poly_t p) {
  free(p->coeff);
  free(p);
}

void poly_set_to_zero(poly_t p) {
  memset(p->coeff, 0, p->size * sizeof (gfelt_t));
  p->deg = -1;
}

int poly_calcule_deg(poly_t p) {
  int d = p->size - 1;
  while ((d >= 0) && (gf_is_zero(p->coeff + d)))
    --d;
  p->deg = d;
  return d;
}


int poly_adjust_deg(poly_t p) {
  int d = p->deg;
  while ((d >= 0) && (gf_is_zero(p->coeff + d)))
    --d;
  p->deg = d;
  return d;
}


void poly_set(poly_t p, poly_t q) {
  int d = p->size - q->size;
  if (d < 0) {
    memcpy(p->coeff, q->coeff, p->size * sizeof (gfelt_t));
    poly_calcule_deg(p);
  }
  else {
    memcpy(p->coeff, q->coeff, q->size * sizeof (gfelt_t));
    memset(p->coeff + q->size, 0, d * sizeof (gfelt_t));
    p->deg = q->deg;
  }
}


poly_t poly_addto(poly_t p, poly_t q) {
  int i;

  for (i = 0; i < p->size; ++i)
		if (i < q->size) 
			poly_addto_coeff(p, i, poly_coeff(q, i));
  poly_calcule_deg(p);

  return(p);
}

poly_t poly_mul(poly_t p, poly_t q) {
  int i,j,dp,dq;
  poly_t r;
    gf_t a;

  poly_calcule_deg(p);
  poly_calcule_deg(q);
  dp = poly_deg(p);
  dq = poly_deg(q);
  r = poly_alloc(dp + dq);
  for (i = 0; i <= dp; ++i) {
    for (j = 0; j <= dq; ++j) {
            gf_mul(a, poly_coeff(p, i), poly_coeff(q, j));
      poly_addto_coeff(r, i + j, a);
        }
    }
  poly_calcule_deg(r);

  return(r);
}


void poly_eval_aux(gf_t res, gfelt_t * coeff, gf_t a, int d) {
  gf_set(res, coeff + d);
  for (--d; d >= 0; --d) {
        gf_mul_fast(res, a, res);
        gf_add(res, res, coeff + d);
    }
}

void poly_eval(gf_t res, poly_t p, gf_t a) {
    if (gf_is_zero(a)) {
        gf_set(res, poly_coeff(p, 0));
    }
    else {
        poly_eval_aux(res, p->coeff, a, poly_deg(p));
    }
}


poly_t poly_compose(poly_t p, poly_t q) {
    int i,j,dp,dq,maxdeg;
    poly_t r;
    gf_t a;
    poly_t power;
    
    poly_calcule_deg(p);
    poly_calcule_deg(q);
    dp = poly_deg(p);
    dq = poly_deg(q);

    if (dp < 0) {
        return poly_alloc(0);
    }

    if (dq < 0) {
        r = poly_alloc(0);
        poly_set_coeff(r, 0, poly_coeff(p, 0));
        poly_calcule_deg(r);
        return r;
    }

    maxdeg = dp * dq;
    r = poly_alloc(maxdeg > 0 ? maxdeg : 0);
    power = poly_alloc(0);
    poly_set_coeff_to_unit(power, 0);
    poly_set_deg(power, 0);

    for (i = 0; i <= dp; ++i) {
        for (j = 0; j <= poly_deg(power); ++j) {
            gf_mul(a, poly_coeff(p, i), poly_coeff(power, j));
            poly_addto_coeff(r, j, a);
        }
        if (i < dp) {
            poly_t next = poly_mul(power, q);
            poly_free(power);
            power = next;
        }
    }
    poly_free(power);
    poly_calcule_deg(r);
   
    return(r);
}


void poly_rem(poly_t p, poly_t g) {
  int i, j, d;
  gf_t a, b, c;
  int dg;

  dg = poly_deg(g);
  d = poly_deg(p) - dg;
  if (d >= 0) {
    if (gf_is_unit(poly_coeff(g, dg))) {
      gfelt_t *pc = p->coeff;
      gfelt_t *gc = g->coeff;
      gfindex_t *log = gf_log;
      gfelt_t *exp = gf_exp;
      for (i = poly_deg(p); d >= 0; --i, --d) {
        gfelt_t bval = pc[i];
        if (bval != 0) {
          uint32_t lb = log[bval];

          for (j = 0; j < dg; ++j) {
            gfelt_t gj = gc[j];

            if (gj != 0) {
              pc[j + d] ^= exp[_gfelt_modq_1(lb + log[gj])];
            }
          }
          pc[i] = 0;
        }
      }
    } else {
      gf_inv(a, poly_coeff(g, dg));
      for (i = poly_deg(p); d >= 0; --i, --d) {
        if (!gf_is_zero(poly_coeff(p, i))) {
                gf_mul_fast(b, a, poly_coeff(p, i));
          for (j = 0; j < dg; ++j) {
              gf_mul_fast(c, b, poly_coeff(g, j));
              poly_addto_coeff(p, j + d, c);
          }
          poly_set_coeff_to_zero(p, i);
        }
      }
    }
    poly_set_deg(p, dg - 1);
    while ((poly_deg(p) >= 0) && (gf_is_zero(poly_coeff(p, poly_deg(p))))) {
      poly_set_deg(p, poly_deg(p) - 1);
        }
  }
}

void poly_sqmod_init(poly_t g, poly_t * sq) {
  int i, d;

  d = poly_deg(g);

  for (i = 0; 2 * i < d; ++i) {

    poly_set_to_zero(sq[i]);
    poly_set_deg(sq[i], 2 * i);
    poly_set_coeff_to_unit(sq[i], 2 * i);
  }

  for (; i < d; ++i) {

        poly_set_to_zero(sq[i]);
    memcpy(sq[i]->coeff + 2, sq[i - 1]->coeff, d * sizeof (gfelt_t));
    poly_set_deg(sq[i], poly_deg(sq[i - 1]) + 2);
    poly_rem(sq[i], g);
  }
}



void poly_sqmod(poly_t res, poly_t p, poly_t * sq, int d) {
  int i, j;
  gf_t a, b;

  poly_set_to_zero(res);


  for (i = 0; i < d / 2; ++i) {
        gf_square(a, poly_coeff(p, i));
    poly_set_coeff(res, i * 2, a);
    }


  for (; i < d; ++i) {
    if (!gf_is_zero(poly_coeff(p, i))) {
      gf_square(a, poly_coeff(p, i));
      for (j = 0; j < d; ++j) {
                gf_mul_fast(b, a, poly_coeff(sq[i], j));
                poly_addto_coeff(res, j, b);
            }
        }
  }


  poly_set_deg(res, d - 1);
  while ((poly_deg(res) >= 0) && (gf_is_zero(poly_coeff(res, poly_deg(res))))) {
    poly_set_deg(res, poly_deg(res) - 1);
    }
}


poly_t poly_gcd_aux(poly_t p1, poly_t p2) {
  if (poly_deg(p2) == -1)
    return p1;
  else {
    poly_rem(p1, p2);
    return poly_gcd_aux(p2, p1);
  }
}

poly_t poly_gcd(poly_t p1, poly_t p2) {
  poly_t a, b, c;

  a = poly_copy(p1);
  b = poly_copy(p2);
  if (poly_deg(a) < poly_deg(b))
    c = poly_copy(poly_gcd_aux(b, a));
  else
    c = poly_copy(poly_gcd_aux(a, b));
  poly_free(a);
  poly_free(b);
  return c;
}


void poly_quo_aux(poly_t quo, poly_t rem, poly_t d) {
  int i, j, dd;
  gf_t a, b, c;

  dd = poly_deg(d);
  gf_inv(a, poly_coeff(d, dd));
  poly_set_to_zero(quo);
  for (i = poly_deg(rem); i >= dd; --i) {
    gf_mul_fast(b, a, poly_coeff(rem, i));
    poly_set_coeff(quo, i - dd, b);
    if (!gf_is_zero(b)) {
      poly_set_coeff_to_zero(rem, i);
      for (j = i - 1; j >= i - dd; --j) {
                gf_mul_fast(c, b, poly_coeff(d, dd - i + j));
                poly_addto_coeff(rem, j, c);
            }
        }
  }
  poly_set_deg(quo, poly_deg(rem) - dd);
}

poly_t poly_quo(poly_t p, poly_t d) {
  int dd, dp;
  poly_t quo, rem;

  dd = poly_calcule_deg(d);
  dp = poly_calcule_deg(p);
  rem = poly_copy(p);
  quo = poly_alloc(dp - dd);
    poly_quo_aux(quo, rem, d);
  poly_free(rem);

  return quo;
}


int poly_degppf(poly_t g) {
    int i, d, res;
    poly_t *u, p, r, s;
    gf_t one;
    gf_set_to_unit(one);

    d = poly_deg(g);
    if (d == 1)
        return 1;
    
    u = malloc(d * sizeof (poly_t *));
    for (i = 0; i < d; ++i)
        u[i] = poly_alloc(d + 1);
    poly_sqmod_init(g, u);

    p = poly_alloc(d - 1);
    poly_set_deg(p, 1);
    poly_set_coeff_to_unit(p, 1);
    r = poly_alloc(d - 1);
    res = d;
    for (i = 1; i <= (d / 2) * gf_extd(); ++i) {
        poly_sqmod(r, p, u, d);

        if ((i % gf_extd()) == 0) {
            poly_addto_coeff(r, 1, one);
            poly_calcule_deg(r);
            s = poly_gcd(g, r);
            if (poly_deg(s) > 0) {
            poly_free(s);
            res = i / gf_extd();
            break;
            }
            poly_free(s);
            poly_addto_coeff(r, 1, one);
            poly_calcule_deg(r);
        }

        s = p;
        p = r;
        r = s;
    }

    poly_free(p);
    poly_free(r);
    for (i = 0; i < d; ++i) {
        poly_free(u[i]);
    }
    free(u);

    return res;
}

void poly_print(poly_t p) {
    int i;

    poly_calcule_deg(p);
    for (i = 0; i < p->size; ++i) {
        if (gf_is_zero(poly_coeff(p, i))) {
            printf("0");
        }
        else if (gf_is_unit(poly_coeff(p, i))) {
            printf("1");
        }
        else {
            printf("*");
        }
    }
    printf("\n");
}


int poly_ee_aux(poly_t u0, poly_t u1, poly_t r0, poly_t r1, int t) {
  int i, j, count = 0;
  gf_t a, b;
  poly_t aux;

  poly_set_to_zero(u0);
  poly_set_to_zero(u1);
  poly_set_coeff_to_unit(u1, 0);
  poly_set_deg(u1, 0);
  while (r1->deg >= t) {
        for (j = r0->deg - r1->deg; j >= 0; --j) {
      gf_div(a, poly_coeff(r0, r1->deg + j), poly_coeff(r1, r1->deg));
      if (!gf_is_zero(a)) {

                for (i = 0; i <= u1->deg; ++i) {
                    gf_mul_fast(b, a, poly_coeff(u1, i));
                    poly_addto_coeff(u0, i + j, b);
                }

                for (i = 0; i <= r1->deg; ++i) {
                    gf_mul_fast(b, a, poly_coeff(r1, i));
                    poly_addto_coeff(r0, i + j, b);
                }
            }
    }
        u0->deg = u1->deg + r0->deg - r1->deg;
        r0->deg = r1->deg - 1;
        poly_adjust_deg(r0);

    aux = r0; r0 = r1; r1 = aux;
    aux = u0; u0 = u1; u1 = aux;
        count++;
  }

    return count;
}


void poly_eeaux(poly_t * u, poly_t * v, poly_t p, poly_t g, int t) {
  int count, dr;
  poly_t r0, r1, u0, u1;



  dr = poly_deg(g);

  r0 = poly_copy(g);
  r1 = poly_copy(p);
  u0 = poly_alloc(dr);
  u1 = poly_alloc(dr);

    count = poly_ee_aux(u0, u1, r0, r1, t);
    
    if (count & 1) {
        *u = u0;
        *v = r0;
        poly_free(r1);
        poly_free(u1);
    }
    else {
        *u = u1;
        *v = r1;
        poly_free(r0);
        poly_free(u0);
    }
}




poly_t poly_randgen_irred(int t) {

  int i;
  poly_t g;

  g = poly_alloc(t);
  poly_set_deg(g, t);
  poly_set_coeff_to_unit(g, t);

  i = 0;
  do {
      for (i = 0; i < t; ++i) {
        poly_set_coeff_rand(g, i);
      }
  } while (poly_degppf(g) < t);

  return g;
}




void poly_shiftmod(poly_t p, poly_t g) {
  int i, t;
  gf_t a, b;

  t = poly_deg(g);
  gf_div(a, p->coeff + (t - 1), g->coeff + t);
  for (i = t - 1; i > 0; --i) {
        gf_mul(b, a, g->coeff + i);
        gf_add(p->coeff + i, p->coeff + (i - 1), b);
    }
    gf_mul(p->coeff, a, g->coeff);
}


poly_t poly_sqrtmod(poly_t g) {
  int i, t;
  poly_t aux, p, q, * sq_aux;

  t = poly_deg(g);

  sq_aux = malloc(t * sizeof (poly_t));
  for (i = 0; i < t; ++i)
    sq_aux[i] = poly_alloc(t + 1);
  poly_sqmod_init(g, sq_aux);

  q = poly_alloc(t - 1);
  p = poly_alloc(t - 1);
  poly_set_deg(p, 1);

  poly_set_coeff_to_unit(p, 1);

  for (i = 0; i < t * gf_extd() - 1; ++i) {

    poly_sqmod(q, p, sq_aux, t);

    aux = q; q = p; p = aux;
  }


  for (i = 0; i < t; ++i)
    poly_free(sq_aux[i]);
  free(sq_aux);
    poly_free(q);

  poly_calcule_deg(p);
    return p;
}

poly_t * poly_sqrtmod_init(poly_t g) {
  int i, t;
  poly_t * sqrt;

  t = poly_deg(g);

  sqrt = malloc(t * sizeof (poly_t));
    sqrt[0] = poly_alloc(t - 1);
    sqrt[1] = poly_sqrtmod(g);
  for (i = 2; i < t; ++i)
    sqrt[i] = poly_alloc(t - 1);

  for(i = 3; i < t; i += 2) {
    poly_set(sqrt[i], sqrt[i - 2]);
    poly_shiftmod(sqrt[i], g);
    poly_calcule_deg(sqrt[i]);
  }

  for (i = 0; i < t; i += 2) {
    poly_set_to_zero(sqrt[i]);
    gf_set_to_unit(sqrt[i]->coeff + (i / 2));
    sqrt[i]->deg = i / 2;
  }

  return sqrt;
}
