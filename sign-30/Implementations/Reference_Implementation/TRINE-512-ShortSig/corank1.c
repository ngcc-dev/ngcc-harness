#include "corank1.h"

#include <string.h>

#include "matrixelim.h"
#include "triform.h"
#include "util.h"

static int vector_is_nonzero(
    const Fq *v,
    int n)
{
  for (int i = 0; i < n; i++)
    if (v[i] != 0)
      return 1;

  return 0;
}

int corank1_cal_vartime(
    Fq *out_u,
    const Fq *M,
    trine_xof_state *xof,
    int n)
{
  if (out_u == NULL ||
      M == NULL ||
      xof == NULL)
    return -1;

  if (n < 1 || n > TRINE_n)
    return -1;

  Fq candidate[n];
  Fq phi_u[n * n];

  for (;;)
  {
    for (int i = 0; i < n; i++)
      if (rnd_GF(&candidate[i], xof) != 0)
        return -1;

    if (!vector_is_nonzero(candidate, n))
      continue;

    triform_phi_u(phi_u, M, candidate, n);

    if (pmod_mat_rank_vartime(phi_u, n) == n - 1)
    {
      memcpy(out_u, candidate, (size_t)n * sizeof(*out_u));
      return 0;
    }
  }
}
