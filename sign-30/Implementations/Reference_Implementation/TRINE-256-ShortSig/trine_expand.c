#include "trine_expand.h"

#include <stdint.h>
#include <string.h>

#include "hashkdf.h"
#include "matrixelim.h"
#include "triform.h"
#include "util.h"

enum
{
  TRINE_PURPOSE_PUBLIC_SEED = 1,
  TRINE_PURPOSE_BASE_FORM = 2,
  TRINE_PURPOSE_MATRIX_A = 3,
  TRINE_PURPOSE_MATRIX_B = 4,
  TRINE_PURPOSE_MATRIX_C = 5
};

static const uint8_t TRINE_EXPAND_PREFIX[] =
    "MEDS2END-TRINE-EXPAND-v1";

static int role_to_purpose(
    trine_matrix_role_t role,
    uint8_t *purpose)
{
  switch (role)
  {
    case TRINE_MATRIX_A:
      *purpose = TRINE_PURPOSE_MATRIX_A;
      return 0;
    case TRINE_MATRIX_B:
      *purpose = TRINE_PURPOSE_MATRIX_B;
      return 0;
    case TRINE_MATRIX_C:
      *purpose = TRINE_PURPOSE_MATRIX_C;
      return 0;
    default:
      return -1;
  }
}

static void store_u32_le(
    uint8_t out[4],
    uint32_t value)
{
  out[0] = (uint8_t)(value & 0xffu);
  out[1] = (uint8_t)((value >> 8) & 0xffu);
  out[2] = (uint8_t)((value >> 16) & 0xffu);
  out[3] = (uint8_t)((value >> 24) & 0xffu);
}

static int init_domain_xof(
    trine_xof_state *xof,
    uint8_t purpose,
    uint32_t index,
    const uint8_t *seed,
    size_t seed_len)
{
  uint8_t index_le[4];

  store_u32_le(index_le, index);

  if (trine_xof_init(xof) != 0 ||
      trine_xof_absorb(
          xof,
          TRINE_EXPAND_PREFIX,
          sizeof(TRINE_EXPAND_PREFIX) - 1u) != 0 ||
      trine_xof_absorb(xof, &purpose, 1u) != 0 ||
      trine_xof_absorb(xof, index_le, sizeof(index_le)) != 0 ||
      trine_xof_absorb(xof, seed, seed_len) != 0 ||
      trine_xof_finalize(xof) != 0)
  {
    trine_xof_release(xof);
    return -1;
  }

  return 0;
}

static int ranges_overlap(
    const Fq *a,
    const Fq *b,
    size_t count)
{
  if (a == NULL || b == NULL)
    return 0;

  const uintptr_t a_begin = (uintptr_t)a;
  const uintptr_t a_end = a_begin + count * sizeof(*a);
  const uintptr_t b_begin = (uintptr_t)b;
  const uintptr_t b_end = b_begin + count * sizeof(*b);

  return a_begin < b_end && b_begin < a_end;
}

int trine_expand_public_seed(
    uint8_t out_public_seed[TRINE_public_seed_bytes],
    const uint8_t secret_seed[TRINE_secret_seed_bytes])
{
  if (out_public_seed == NULL || secret_seed == NULL)
    return -1;

  uint8_t tmp[TRINE_public_seed_bytes];
  trine_xof_state xof;

  if (init_domain_xof(
      &xof,
      TRINE_PURPOSE_PUBLIC_SEED,
      0,
      secret_seed,
      TRINE_secret_seed_bytes) != 0)
    return -1;

  if (trine_xof_squeeze(&xof, tmp, sizeof(tmp)) != 0)
  {
    trine_xof_release(&xof);
    return -1;
  }
  trine_xof_release(&xof);

  memcpy(out_public_seed, tmp, sizeof(tmp));
  return 0;
}

int trine_expand_base_form(
    Fq *out_base_form,
    const uint8_t public_seed[TRINE_public_seed_bytes],
    int n)
{
  if (out_base_form == NULL || public_seed == NULL)
    return -1;

  if (n < 1 || n > TRINE_n)
    return -1;

  trine_xof_state xof;

  if (init_domain_xof(
      &xof,
      TRINE_PURPOSE_BASE_FORM,
      0,
      public_seed,
      TRINE_public_seed_bytes) != 0)
    return -1;

  for (size_t i = 0; i < triform_element_count(n); i++)
  {
    if (rnd_GF(&out_base_form[i], &xof) != 0)
    {
      trine_xof_release(&xof);
      return -1;
    }
  }

  trine_xof_release(&xof);
  return 0;
}

int trine_expand_secret_matrix_pair_vartime(
    Fq *out_matrix,
    Fq *out_inverse,
    const uint8_t secret_seed[TRINE_secret_seed_bytes],
    trine_matrix_role_t role,
    uint32_t index,
    int n)
{
  if (secret_seed == NULL)
    return -1;

  if (out_matrix == NULL && out_inverse == NULL)
    return -1;

  if (n < 1 || n > TRINE_n)
    return -1;

  if (index >= (uint32_t)TRINE_X)
    return -1;

  const size_t matrix_elements = (size_t)n * (size_t)n;
  if (ranges_overlap(out_matrix, out_inverse, matrix_elements))
    return -1;

  uint8_t purpose;
  if (role_to_purpose(role, &purpose) != 0)
    return -1;

  uint8_t matrix_seed[TRINE_secret_seed_bytes];
  trine_xof_state xof;
  Fq matrix_tmp[matrix_elements];
  Fq inverse_tmp[matrix_elements];

  if (init_domain_xof(&xof, purpose, index, secret_seed, TRINE_secret_seed_bytes) != 0)
    return -1;

  if (trine_xof_squeeze(&xof, matrix_seed, sizeof(matrix_seed)) != 0)
  {
    trine_xof_release(&xof);
    return -1;
  }
  trine_xof_release(&xof);

  if (rnd_inv_matrix(matrix_tmp, n, n, matrix_seed, sizeof(matrix_seed)) != 0)
    return -1;

  if (out_inverse != NULL &&
      pmod_mat_inv_vartime(inverse_tmp, matrix_tmp, n) != 0)
    return -1;

  if (out_matrix != NULL)
    memcpy(out_matrix, matrix_tmp, sizeof(matrix_tmp));

  if (out_inverse != NULL)
    memcpy(out_inverse, inverse_tmp, sizeof(inverse_tmp));

  return 0;
}
