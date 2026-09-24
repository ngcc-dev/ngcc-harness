#include "rbc_poly.h"

void rbc_poly_init(rbc_poly* p, int32_t max_degree) {
  *p = (rbc_poly) malloc(sizeof(rbc_poly_struct));
  if(*p == NULL) {
    exit(EXIT_FAILURE);
  }
  (*p)->degree = -1;
  (*p)->max_degree = max_degree;
  rbc_vec_init(&((*p)->v), (uint32_t) max_degree + 1);
}

void rbc_poly_clear(rbc_poly p) {
  if(p != NULL) {
    rbc_vec_clear(p->v);
    free(p);
  }
}

void rbc_poly_sparse_init(rbc_poly_sparse* p, uint32_t coeffs_nb, const uint32_t* coeffs) {
  *p = (rbc_poly_sparse) malloc(sizeof(rbc_poly_sparse_struct));
  if(*p == NULL) {
    exit(EXIT_FAILURE);
  }

  (*p)->coeffs_nb = coeffs_nb;
  (*p)->coeffs = (uint32_t*) malloc((size_t) coeffs_nb * sizeof(uint32_t));
  if((*p)->coeffs == NULL) {
    free(*p);
    exit(EXIT_FAILURE);
  }
  memcpy((*p)->coeffs, coeffs, (size_t) coeffs_nb * sizeof(uint32_t));
}

void rbc_poly_sparse_clear(rbc_poly_sparse p) {
  if(p != NULL) {
    free(p->coeffs);
    free(p);
  }
}

void rbc_poly_update_degree(rbc_poly p, int32_t position) {
  if(position > p->max_degree) {
    position = p->max_degree;
  }

  for(int32_t i = position; i >= 0; --i) {
    if(!rbc_elt_is_zero(p->v[i])) {
      p->degree = i;
      return;
    }
  }

  p->degree = -1;
}

void rbc_poly_set_zero(rbc_poly p) {
  rbc_vec_set_zero(p->v, (uint32_t) p->max_degree + 1);
  p->degree = -1;
}

void rbc_poly_set(rbc_poly o, const rbc_poly p) {
  rbc_poly_set_zero(o);
  const int32_t degree = p->degree < o->max_degree ? p->degree : o->max_degree;
  for(int32_t i = 0; i <= degree; ++i) {
    rbc_elt_set(o->v[i], p->v[i]);
  }
  rbc_poly_update_degree(o, degree);
}

void rbc_poly_set_coefficient(rbc_poly p, uint32_t position, const rbc_elt e) {
  if(position > (uint32_t) p->max_degree) {
    return;
  }

  rbc_elt_set(p->v[position], e);
  if(!rbc_elt_is_zero(e) && (int32_t) position > p->degree) {
    p->degree = (int32_t) position;
  }
  else if((int32_t) position == p->degree && rbc_elt_is_zero(e)) {
    rbc_poly_update_degree(p, p->degree);
  }
}

void rbc_poly_get_coefficient(rbc_elt o, const rbc_poly p, uint32_t position) {
  if(position > (uint32_t) p->max_degree) {
    rbc_elt_set_zero(o);
  }
  else {
    rbc_elt_set(o, p->v[position]);
  }
}

uint8_t rbc_poly_is_equal_to(const rbc_poly p1, const rbc_poly p2) {
  if(p1->degree != p2->degree) {
    return 0;
  }

  for(int32_t i = 0; i <= p1->degree; ++i) {
    if(!rbc_elt_is_equal_to(p1->v[i], p2->v[i])) {
      return 0;
    }
  }

  return 1;
}

void rbc_poly_add(rbc_poly o, const rbc_poly p1, const rbc_poly p2) {
  const int32_t degree = p1->degree > p2->degree ? p1->degree : p2->degree;
  rbc_poly_set_zero(o);
  for(int32_t i = 0; i <= degree && i <= o->max_degree; ++i) {
    rbc_elt_add(o->v[i], p1->v[i], p2->v[i]);
  }
  rbc_poly_update_degree(o, degree);
}

void rbc_poly_scalar_mul(rbc_poly o, const rbc_poly p, const rbc_elt e) {
  rbc_poly_set_zero(o);
  if(rbc_elt_is_zero(e) || p->degree < 0) {
    return;
  }

  for(int32_t i = 0; i <= p->degree && i <= o->max_degree; ++i) {
    rbc_elt_mul(o->v[i], p->v[i], e);
  }
  rbc_poly_update_degree(o, p->degree);
}

void rbc_poly_mul(rbc_poly o, const rbc_poly p1, const rbc_poly p2) {
  rbc_poly tmp;
  rbc_poly_init(&tmp, o->max_degree);

  if(p1->degree >= 0 && p2->degree >= 0) {
    for(int32_t i = 0; i <= p1->degree; ++i) {
      for(int32_t j = 0; j <= p2->degree; ++j) {
        if(i + j <= o->max_degree) {
          rbc_elt product;
          rbc_elt_mul(product, p1->v[i], p2->v[j]);
          rbc_elt_add(tmp->v[i + j], tmp->v[i + j], product);
        }
      }
    }
    rbc_poly_update_degree(tmp, p1->degree + p2->degree);
  }

  rbc_poly_set(o, tmp);
  rbc_poly_clear(tmp);
}

void rbc_poly_mulmod_sparse(rbc_poly o, const rbc_poly p1, const rbc_poly p2, const rbc_poly_sparse modulus) {
  if(modulus == NULL || modulus->coeffs_nb == 0 || p1->degree < 0 || p2->degree < 0) {
    rbc_poly_set_zero(o);
    return;
  }

  uint32_t modulus_degree = 0;
  for(uint32_t i = 0; i < modulus->coeffs_nb; ++i) {
    if(modulus->coeffs[i] > modulus_degree) {
      modulus_degree = modulus->coeffs[i];
    }
  }
  if(modulus_degree == 0) {
    rbc_poly_set_zero(o);
    return;
  }

  const int32_t product_degree = p1->degree + p2->degree;
  rbc_poly product;
  rbc_poly_init(&product, product_degree);
  for(int32_t i = 0; i <= p1->degree; ++i) {
    for(int32_t j = 0; j <= p2->degree; ++j) {
      rbc_elt term;
      rbc_elt_mul(term, p1->v[i], p2->v[j]);
      rbc_elt_add(product->v[i + j], product->v[i + j], term);
    }
  }
  rbc_poly_update_degree(product, product_degree);

  for(int32_t position = product->degree; position >= (int32_t) modulus_degree; --position) {
    if(!rbc_elt_is_zero(product->v[position])) {
      rbc_elt carry;
      rbc_elt_set(carry, product->v[position]);
      rbc_elt_set_zero(product->v[position]);
      for(uint32_t i = 0; i < modulus->coeffs_nb; ++i) {
        const uint32_t exponent = modulus->coeffs[i];
        if(exponent != modulus_degree) {
          const uint32_t target = (uint32_t) (position - (int32_t) modulus_degree) + exponent;
          rbc_elt_add(product->v[target], product->v[target], carry);
        }
      }
    }
  }

  rbc_poly_set_zero(o);
  const uint32_t output_size = modulus_degree < (uint32_t) o->max_degree + 1
    ? modulus_degree
    : (uint32_t) o->max_degree + 1;
  rbc_vec_set(o->v, product->v, output_size);
  rbc_poly_update_degree(o, (int32_t) output_size - 1);
  rbc_poly_clear(product);
}

void rbc_poly_print(const rbc_poly p) {
  printf("degree=%d [ ", p->degree);
  for(int32_t i = 0; i <= p->degree; ++i) {
    rbc_elt_print(p->v[i]);
    printf(" ");
  }
  printf("]\n");
}
