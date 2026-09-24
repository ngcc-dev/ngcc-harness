#include <stdint.h>
#include <string.h>

#include "log.h"

#include "util.h"
#include "trine.h"
#include "matrixmod.h"
#include "matrixelim.h"


int XOF(uint8_t **buf, size_t *length, const uint8_t *seed, size_t seed_len, int num)
{
  trine_xof_state xof;

  if (buf == NULL || length == NULL || (seed == NULL && seed_len != 0u) || num < 0)
    return -1;

  if (trine_xof_init(&xof) != 0 ||
      trine_xof_absorb(&xof, seed, seed_len) != 0 ||
      trine_xof_finalize(&xof) != 0)
  {
    trine_xof_release(&xof);
    return -1;
  }

  for (int i = 0; i < num; i++)
  {
    if (trine_xof_squeeze(&xof, buf[i], length[i]) != 0)
    {
      trine_xof_release(&xof);
      return -1;
    }
  }

  trine_xof_release(&xof);
  return 0;
}

int rnd_GF(Fq *out, trine_xof_state *xof)
{
  Fq val = TRINE_q;

  if (out == NULL || xof == NULL)
    return -1;

  while (val >= TRINE_q)
  {
    uint8_t data[sizeof(Fq)];
    
    if (trine_xof_squeeze(xof, data, sizeof(Fq)) != 0)
      return -1;

    val = 0;
    for (int i = 0; i < sizeof(Fq); i++)
      val |= data[i] << (i*8);

    val = val & ((1 << TRINE_q_bits) - 1);
  }

  *out = val;
  return 0;
}

int rnd_sys_mat(Fq *M, int M_r, int M_c, const uint8_t *seed, size_t seed_len)
{
  trine_xof_state xof;

  if (M == NULL || (seed == NULL && seed_len != 0u) || M_r < 0 || M_c < M_r)
    return -1;

  if (trine_xof_init(&xof) != 0 ||
      trine_xof_absorb(&xof, seed, seed_len) != 0 ||
      trine_xof_finalize(&xof) != 0)
  {
    trine_xof_release(&xof);
    return -1;
  }

  for (int r = 0; r < M_r; r++)
    for (int c = M_r; c < M_c; c++)
    {
      Fq sample;
      if (rnd_GF(&sample, &xof) != 0)
      {
        trine_xof_release(&xof);
        return -1;
      }
      pmod_mat_set_entry(M, M_r, M_c, r, c, sample);
    }

  for (int r = 0; r < M_r; r++)
    for (int c = 0; c < M_r; c++)
      if (r == c)
        pmod_mat_set_entry(M, M_r, M_c, r, c, 1);
      else
        pmod_mat_set_entry(M, M_r, M_c, r, c, 0);

  trine_xof_release(&xof);
  return 0;
}

int rnd_inv_matrix(Fq *M, int M_r, int M_c, uint8_t *seed, size_t seed_len)
{
  if (M == NULL || (seed == NULL && seed_len != 0u) || M_r != M_c || M_r < 1)
    return -1;

  trine_xof_state xof;
  if (trine_xof_init(&xof) != 0 ||
      trine_xof_absorb(&xof, seed, seed_len) != 0 ||
      trine_xof_finalize(&xof) != 0)
  {
    trine_xof_release(&xof);
    return -1;
  }

  while (1)
  {
    for (int r = 0; r < M_r; r++)
      for (int c = 0; c < M_c; c++)
      {
        Fq sample;
        if (rnd_GF(&sample, &xof) != 0)
        {
          trine_xof_release(&xof);
          return -1;
        }
        pmod_mat_set_entry(M, M_r, M_c, r, c, sample);
      }

    if (pmod_mat_is_invertible_vartime(M, M_r))
    {
      trine_xof_release(&xof);
      return 0;
    }
  }
}

static unsigned bit_length_u32(uint32_t value)
{
  unsigned bits = 0;

  do
  {
    bits++;
    value >>= 1;
  }
  while (value != 0);

  return bits;
}

static int sample_masked_le(
    uint32_t *out,
    trine_xof_state *xof,
    unsigned bit_count)
{
  if (out == NULL || xof == NULL || bit_count == 0 || bit_count > 32)
    return -1;

  const size_t byte_count = TRINE_CEIL_DIV((size_t)bit_count, 8u);
  uint8_t buf[4] = {0};
  uint32_t value = 0;

  if (trine_xof_squeeze(xof, buf, byte_count) != 0)
    return -1;

  for (size_t i = 0; i < byte_count; i++)
    value |= (uint32_t)buf[i] << (8u * i);

  if (bit_count < 32)
    value &= ((uint32_t)1u << bit_count) - 1u;

  *out = value;
  return 0;
}

int trine_parse_hash(
    const uint8_t *digest,
    size_t digest_len,
    trine_challenge_t *out,
    size_t out_len)
{
  if (digest == NULL || out == NULL)
    return -1;

  if (digest_len != TRINE_digest_bytes || out_len != TRINE_r)
    return -1;

  trine_challenge_t parsed[TRINE_r];
  uint16_t raw[TRINE_r] = {0};
  trine_xof_state xof;
  const unsigned position_bits = bit_length_u32((uint32_t)(TRINE_r - 1));
  const unsigned value_bits = bit_length_u32((uint32_t)TRINE_X);

  if (trine_xof_init(&xof) != 0 ||
      trine_xof_absorb(&xof, digest, digest_len) != 0 ||
      trine_xof_finalize(&xof) != 0)
  {
    trine_xof_release(&xof);
    return -1;
  }

  for (int selected = 0; selected < TRINE_K; selected++)
  {
    uint32_t position = 0;
    uint32_t value = 0;

    do
    {
      if (sample_masked_le(&position, &xof, position_bits) != 0)
      {
        trine_xof_release(&xof);
        return -1;
      }
    }
    while (position >= (uint32_t)TRINE_r || raw[position] != 0);

    do
    {
      if (sample_masked_le(&value, &xof, value_bits) != 0)
      {
        trine_xof_release(&xof);
        return -1;
      }
    }
    while (value == 0 || value > (uint32_t)TRINE_X);

    raw[position] = (uint16_t)value;
  }

  for (int i = 0; i < TRINE_r; i++)
    parsed[i] = (trine_challenge_t)(TRINE_X - raw[i]);

  memcpy(out, parsed, sizeof(parsed));
  trine_xof_release(&xof);
  return 0;
}

int solve(Fq *A, Fq *B_inv, Fq *G0prime, Fq Amm)
{
  Fq P0prime0[TRINE_n*TRINE_n];
  Fq P0prime1[TRINE_n*TRINE_n];

  for (int i = 0; i < TRINE_n*TRINE_n; i++)
  {
    P0prime0[i] = G0prime[i];
    P0prime1[i] = G0prime[i + TRINE_n * TRINE_n];
  }

  Fq N[TRINE_n * TRINE_n];

  for (int i = 0; i < TRINE_n; i++)
    for (int j = 0; j < TRINE_n; j++)
      N[j*TRINE_n + i] = (TRINE_q - P0prime0[i*TRINE_n + j]) % TRINE_q;

  //LOG_MAT(N, TRINE_n, TRINE_n);


  Fq M[TRINE_n*(TRINE_n + TRINE_n + 2)] = {0};

  for (int i = 0; i < TRINE_n; i++)
    for (int j = 0; j < TRINE_n; j++)
      M[j*(TRINE_n + TRINE_n + 2) + i] = (TRINE_q - P0prime1[i*TRINE_n + j]) % TRINE_q;

  for (int i = 0; i < TRINE_n; i++)
    for (int j = 0; j < TRINE_n; j++)
      M[j*(TRINE_n + TRINE_n + 2) + i + TRINE_n] = P0prime0[i*TRINE_n + j];

  for (int j = 0; j < TRINE_n; j++)
    M[j*(TRINE_n + TRINE_n + 2) + TRINE_n + TRINE_n] = ((uint32_t)P0prime0[(TRINE_n-1)*TRINE_n + j] * (TRINE_q - (uint32_t)Amm)) % TRINE_q;

  for (int j = 0; j < TRINE_n; j++)
    M[j*(TRINE_n + TRINE_n + 2) + TRINE_n + TRINE_n + 1] = ((uint32_t)P0prime1[(TRINE_n-1)*TRINE_n + j] * (uint32_t)Amm) % TRINE_q;


  //LOG_MAT(M, TRINE_n, TRINE_n + TRINE_n + 2);

  if (pmod_mat_syst_ct(M, TRINE_n-1, TRINE_n + TRINE_n + 2) < 0)
    return -1;

  //LOG_MAT_FMT(M, TRINE_n, TRINE_n + TRINE_n + 2, "M part");

  // eliminate last row
  for (int r = 0; r < TRINE_n-1; r++)
  {
    uint64_t factor = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, r);

    // ignore last column
    for (int c = TRINE_n-1; c < TRINE_n + TRINE_n + 1; c++)
    {
      uint64_t tmp0 = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, c);
      uint64_t tmp1 = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, r, c);

      int64_t val = (tmp1 * factor) % TRINE_q;

      val = tmp0 - val;

      val += TRINE_q * (val < 0);

      pmod_mat_set_entry(M, TRINE_n, TRINE_n + TRINE_n + 2,  TRINE_n-1, c, val);
    }

    pmod_mat_set_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, r, 0);
  }

  // normalize last row
  {
    uint64_t val = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, TRINE_n-1);

    if (val == 0)
      return -1;

    val = GF_inv(val);

    // ignore last column
    for (int c = TRINE_n; c < TRINE_n + TRINE_n + 1; c++)
    {
      uint64_t tmp = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, c);

      tmp = (tmp * val) % TRINE_q;

      pmod_mat_set_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, c, tmp);
    }
  }

  pmod_mat_set_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, TRINE_n-1, 1);

  M[TRINE_n*(TRINE_n + TRINE_n + 2)-1] = 0;

  //LOG_MAT_FMT(M, TRINE_n, TRINE_n + TRINE_n + 2, "M red");

  // back substitute
  for (int r = 0; r < TRINE_n-1; r++)
  {
    uint64_t factor = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, r, TRINE_n-1);

    // ignore last column
    for (int c = TRINE_n; c < TRINE_n + TRINE_n + 1; c++)
    {
        uint64_t tmp0 = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, TRINE_n-1, c);
        uint64_t tmp1 = pmod_mat_entry(M, TRINE_n, TRINE_n + TRINE_n + 2, r, c);

        int64_t val = (tmp0 * factor) % TRINE_q;

        val = tmp1 - val;

        val += TRINE_q * (val < 0);

        pmod_mat_set_entry(M, TRINE_n, TRINE_n + TRINE_n + 2,  r, c, val);
    }

    pmod_mat_set_entry(M, M_r, TRINE_n + TRINE_n + 2, r, TRINE_n-1, 0);
  }


  //LOG_MAT_FMT(M, TRINE_n, TRINE_n + TRINE_n + 2, "M done");


  Fq sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n] = {0};

  sol[TRINE_n*TRINE_n + TRINE_n * TRINE_n - 1] = Amm;

  for (int i = 0; i < TRINE_n - 1; i++)
    sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - TRINE_n + i] = M[(i+1) * (TRINE_n + TRINE_n + 2) - 1];

  for (int i = 0; i < TRINE_n; i++)
    sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - 2*TRINE_n + i] = M[(i+1) * (TRINE_n + TRINE_n + 2) - 2];

  for (int i = 0; i < TRINE_n; i++)
    sol[TRINE_n*TRINE_n - TRINE_n + i] = ((uint32_t)P0prime0[(TRINE_n-1)*TRINE_n + i] * (uint32_t)Amm) % TRINE_q;

  //LOG_VEC_FMT(sol, TRINE_n*TRINE_n + TRINE_n*TRINE_n, "initial sol");


  // incomplete blocks:

  for (int i = 0; i < TRINE_n; i++)
    for (int j = 0; j < TRINE_n-1; j++)
    {
      uint32_t tmp = (uint32_t)sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - 2*TRINE_n + i] + TRINE_q -
        ((uint32_t)M[i * (TRINE_n + TRINE_n + 2) + TRINE_n + TRINE_n-2 - j] * (uint32_t)sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - 2 - j]) % TRINE_q;
      sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - 2*TRINE_n + i] = tmp % TRINE_q;
    }

  for (int i = 0; i < TRINE_n; i++)
    for (int j = 0; j < TRINE_n-1; j++)
    {
      uint32_t tmp = (uint32_t)sol[TRINE_n*TRINE_n - TRINE_n + i] + TRINE_q - 
        ((uint32_t)N[i * (TRINE_n) + TRINE_n-2 - j] * (uint32_t)sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - 2 - j]) % TRINE_q;
      sol[TRINE_n*TRINE_n - TRINE_n + i] = tmp % TRINE_q;
    }

  //LOG_VEC_FMT(sol, TRINE_n*TRINE_n + TRINE_n*TRINE_n, "incomplete blocks");


  // complete blocks:

  for (int block = 3; block <= TRINE_n; block++)
    for (int i = 0; i < TRINE_n; i++)
      for (int j = 0; j < TRINE_n; j++)
      {
        uint32_t tmp = sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - block*TRINE_n + i] + TRINE_q -
          ((uint32_t)M[i * (TRINE_n + TRINE_n + 2) + TRINE_n + TRINE_n-1 - j] * (uint32_t)sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - 1 - (block-2)*TRINE_n - j]) % TRINE_q;
        sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - block*TRINE_n + i] = tmp % TRINE_q;
      }

  for (int block = 2; block <= TRINE_n; block++)
    for (int i = 0; i < TRINE_n; i++)
      for (int j = 0; j < TRINE_n; j++)
      {
        uint32_t tmp = sol[TRINE_n*TRINE_n - block*TRINE_n + i] + TRINE_q - 
          ((uint32_t)N[i * (TRINE_n) + TRINE_n-1 - j] * (uint32_t)sol[TRINE_n*TRINE_n + TRINE_n*TRINE_n - 1 - (block-1)*TRINE_n - j]) % TRINE_q;
        sol[TRINE_n*TRINE_n - block*TRINE_n + i] = tmp % TRINE_q;
      }

  //LOG_VEC_FMT(sol, TRINE_n*TRINE_n + TRINE_n*TRINE_n, "complete blocks");


  for (int i = 0; i < TRINE_n * TRINE_n; i++)
    A[i] = sol[i + TRINE_n * TRINE_n];

  for (int i = 0; i < TRINE_n * TRINE_n; i++)
    B_inv[i] = sol[i];

  //LOG_MAT(A, TRINE_n, TRINE_n);
  //LOG_MAT(B_inv, TRINE_n, TRINE_n);

  return 0;
}


void G_mat_init(Fq *G, Fq *Gsub[TRINE_n])
{
  for (int i = 0; i < TRINE_n; i++)
    Gsub[i] = G + i*TRINE_n*TRINE_n;
}

void pi(Fq *Gout, Fq *A, Fq *B, Fq *G)
{
  Fq *G0sub[TRINE_n];
  G_mat_init(G, G0sub);

  Fq *Gsub[TRINE_n];
  G_mat_init(Gout, Gsub);

  for (int i = 0; i < TRINE_n; i++)
  {
    pmod_mat_mul(Gsub[i], A, G0sub[i], TRINE_n);
    pmod_mat_mul(Gsub[i], Gsub[i], B, TRINE_n);
  }
}

#if TRINE_n != TRINE_n || TRINE_n != TRINE_n
#error "MEDS2endGen phi() assumes TRINE_n == TRINE_n == TRINE_n"
#endif

void phi(Fq *Gout, Fq *A, Fq *B, Fq *C, Fq *G)
{
  Fq tmp[TRINE_n * TRINE_n * TRINE_n];

  pi(tmp, A, B, G);

  pmod_mat_mul_rect(Gout,
      TRINE_n, TRINE_n * TRINE_n,
      C, TRINE_n, TRINE_n,
      tmp, TRINE_n, TRINE_n * TRINE_n);
}
