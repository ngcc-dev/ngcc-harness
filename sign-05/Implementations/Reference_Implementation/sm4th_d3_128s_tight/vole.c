#include "vole.h"
#include "prg.h"
#include "utils.h"
#include "random_oracle.h"
#include "sig_timing.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VOLE_TWEAK_OFFSET UINT32_C(0x80000000)

typedef struct {
  uint8_t* ui;
  size_t ui_cap;
  uint8_t* qtmp;
  size_t qtmp_cap;
  uint8_t* r_workspace;
  size_t r_workspace_cap;
  uint8_t* sd_precomp;
  size_t sd_precomp_cap;
} vole_scratch_cache_t;

static vole_scratch_cache_t g_vole_scratch = {0};

// Reserves at least `need` bytes in the buffer, and returns a pointer to the buffer. The caller is responsible for freeing the buffer when done.
// Work when running the signature multiple times, this can save some time on memory allocation and deallocation.
static uint8_t* vole_cache_reserve(uint8_t** buf, size_t* cap, size_t need) {
  if (*cap >= need) {
    return *buf;
  }
  uint8_t* p = realloc(*buf, need);
  assert(p);
  *buf = p;
  *cap = need;
  return p;
}

bool decode_all_chall_3(const params_t* params, uint16_t* decoded_chall, const uint8_t* chall) {
  const unsigned int tau = params->tau;
  const unsigned int tau1 = params->tau1;
  const unsigned int k = params->k;
  const size_t chall_bytes = params->lambda_f_bytes;

  for (unsigned int i = 0; i != tau; ++i) {
    unsigned int lo;
    unsigned int len;
    if (i < tau1) {
      lo = i * k;
      len = k;
    } else {
      const unsigned int t = i - tau1;
      lo = (tau1 * k) + (t * (k - 1));
      len = k - 1;
    }

    const size_t byte_off = lo >> 3;
    const unsigned int bit_off = lo & 7u;
    uint32_t word = 0;
    if (byte_off < chall_bytes) {
      word |= (uint32_t)chall[byte_off];
    }
    if (byte_off + 1u < chall_bytes) {
      word |= (uint32_t)chall[byte_off + 1u] << 8;
    }
    if (byte_off + 2u < chall_bytes) {
      word |= (uint32_t)chall[byte_off + 2u] << 16;
    }

    const uint16_t mask = (uint16_t)((1u << len) - 1u);
    decoded_chall[i] = (uint16_t)((word >> bit_off) & mask);
  }
  return true;
}

// Generate v_j and r_j+1,i at the same time
// Acce by xor
static inline void vole_mix_accumulate_pair(const uint8_t* left, const uint8_t* right, uint8_t* acc,
                                            uint8_t* out, size_t len) {
  size_t i = 0;
  for (; i < len; ++i) {
    const uint8_t r = right[i];
    // Step 10 in ConvertToVOLE
    out[i]          = left[i] ^ r;
    // Step 9 in ConvertToVOLE
    acc[i] ^= r;
  }
}

static unsigned int convert_to_vole(const params_t* params, const uint8_t* sd_precomp, bool sd0_bot,
                                    unsigned int i, unsigned int outlen, uint8_t* u, uint8_t* v,
                                    uint8_t* r
                                    ) {
  const unsigned int tau_1         = params->tau1;
  assert(params->lambda == 128u);
  const unsigned int k             = params->k;
  const unsigned int num_instances = bavc_max_node_index(i, tau_1, k);
  const unsigned int depth         = bavc_max_node_depth(i, tau_1, k);

  assert(r);
  assert(outlen != 0u);
  assert(sd_precomp != NULL);

#define R(row, column) (r + (((row) % 2) * num_instances + (column)) * outlen)
#define V(idx) (v + (idx) * outlen)

  if (sd0_bot) {
    memset(R(0, 0), 0, outlen);
  }
  // Step: 1-5
  // PRG copied from sd_precomp instead of computing.
  const unsigned int start_idx = sd0_bot ? 1u : 0u;
  if (depth == 0) {
    if (!sd0_bot) {
      memcpy(R(0, 0), sd_precomp, outlen);
    } else if (start_idx < num_instances) {
      memcpy(R(0, start_idx), sd_precomp + (size_t)start_idx * outlen,
             (size_t)(num_instances - start_idx) * outlen);
    }
  }

  // Step: 6
  memset(v, 0, depth * outlen);
  // Step 7-12
  unsigned int j_start = 0;
  if (depth > 0) {
    const unsigned int depthloop = num_instances >> 1;
    if (sd0_bot) {
      memcpy(V(0), sd_precomp + outlen, outlen);
      memcpy(R(1, 0), sd_precomp + outlen, outlen);
      for (unsigned int idx = 1; idx < depthloop; ++idx) {
        const uint8_t* left  = sd_precomp + (size_t)(2 * idx) * outlen;
        const uint8_t* right = left + outlen;
        vole_mix_accumulate_pair(left, right, V(0), R(1, idx), outlen);
      }
    } else {
      for (unsigned int idx = 0; idx < depthloop; ++idx) {
        const uint8_t* left  = sd_precomp + (size_t)(2 * idx) * outlen;
        const uint8_t* right = left + outlen;
        vole_mix_accumulate_pair(left, right, V(0), R(1, idx), outlen);
      }
    }
    j_start = 1;
  }
  for (unsigned int j = j_start; j < depth; j++) {
    const unsigned int depthloop = num_instances >> (j + 1);
    for (unsigned int idx = 0; idx < depthloop; idx++) {
      vole_mix_accumulate_pair(R(j, 2 * idx), R(j, 2 * idx + 1), V(j), R(j + 1, idx), outlen);
    }
  }

  // Step: 13
  if (!sd0_bot && u != NULL) {
    memcpy(u, R(depth, 0), outlen);
  }

  return depth;
}

void vole_commit(const params_t* params, const uint8_t* rootKey, const uint8_t* iv,
                 unsigned int ellhat, bavc_t* bavc, uint8_t* c, uint8_t* u, uint8_t** v) {
  const unsigned int lambda_f     = params->lambda_f;
  const unsigned int ellhat_bytes = (ellhat + 7) / 8;
  assert(params->lambda == 128u);
  assert(ellhat_bytes == params->ell_hat_bytes);
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int k            = params->k;

  SIG_TSTART(t_bavc_commit);
  bavc_commit(params, bavc, rootKey, iv);
  SIG_TEND("sign.bavc_commit", t_bavc_commit);

  const unsigned int max_num_instances = bavc_max_node_index(0, tau_1, k);
  const uint8_t* sd_precomp_base = bavc->sd_prg288;
  assert(sd_precomp_base != NULL);
  const size_t ui_need = (size_t)tau * ellhat_bytes;
  const size_t r_workspace_need = (size_t)(max_num_instances + (max_num_instances >> 1)) * ellhat_bytes;
  uint8_t* ui =
      vole_cache_reserve(&g_vole_scratch.ui, &g_vole_scratch.ui_cap, ui_need);
  uint8_t* r_workspace = vole_cache_reserve(&g_vole_scratch.r_workspace,
                                            &g_vole_scratch.r_workspace_cap,
                                            r_workspace_need);

  SIG_TSTART(t_vole_convert);
  unsigned int v_idx = 0;
  size_t sd_precomp_off = 0;
  for (unsigned int i = 0; i < tau; ++i) {
    const size_t ni = (size_t)bavc_max_node_index(i, tau_1, k);
    const uint8_t* sd_precomp_i = sd_precomp_base + sd_precomp_off;
    // Step 6
    v_idx += convert_to_vole(params, sd_precomp_i, false, i, ellhat_bytes,
                             ui + i * ellhat_bytes, v[v_idx], r_workspace);
    sd_precomp_off += ni * ellhat_bytes;
  }

  // ensure 0-padding up to lambda_f
  for (; v_idx != lambda_f; ++v_idx) {
    memset(v[v_idx], 0, ellhat_bytes);
  }
  SIG_TEND("sign.vole_convert", t_vole_convert);

  SIG_TSTART(t_vole_correction);
  // Step 9
  memcpy(u, ui, ellhat_bytes);
  for (unsigned int i = 1; i < tau; i++) {
    // Step 11
    xor_u8_array(u, ui + i * ellhat_bytes, c + (i - 1) * ellhat_bytes, ellhat_bytes);
  }
  SIG_TEND("sign.misc_vole_correction", t_vole_correction);

}

bool vole_reconstruct(const params_t* params, uint8_t* com, uint8_t** q, const uint8_t* iv,
                      const uint8_t* chall_3, const uint8_t* decom_i, const uint8_t* c,
                      unsigned int ellhat) {
  const unsigned int lambda_f     = params->lambda_f;
  const unsigned int seed_bytes = params->lambda_prg_bytes;
  const unsigned int ellhat_bytes = (ellhat + 7) / 8;
  const unsigned int tau          = params->tau;
  assert(lambda == 128u);
  assert(ellhat_bytes == params->ell_hat_bytes);
  const unsigned int tau1         = params->tau1;
  const unsigned int L            = params->L;
  const unsigned int k            = params->k;

  uint16_t* i_delta = malloc(sizeof(uint16_t) * tau);
  assert(i_delta);

  if (!decode_all_chall_3(params, i_delta, chall_3)) {
    free(i_delta);
    return false;
  }

  bavc_rec_t bavc_rec;
  bavc_rec.h = com;
  bavc_rec.s = malloc((size_t)L * seed_bytes);
  bavc_rec.s_prg288 = malloc((size_t)L * ellhat_bytes);
  assert(bavc_rec.s);
  assert(bavc_rec.s_prg288);

  SIG_TSTART(t_bavc_reconstruct);
  if (!bavc_reconstruct(params, &bavc_rec, decom_i, i_delta, iv)) {
    free(bavc_rec.s_prg288);
    free(bavc_rec.s);
    free(i_delta);
    SIG_TEND("verify.bavc_reconstruct", t_bavc_reconstruct);
    return false;
  }
  SIG_TEND("verify.bavc_reconstruct", t_bavc_reconstruct);

  const unsigned int max_num_instances = bavc_max_node_index(0, tau1, k);
  const uint8_t* sd_precomp_base = bavc_rec.s_prg288;
  const size_t qtmp_need = (size_t)params->k * ellhat_bytes;
  const size_t r_workspace_need = (size_t)(max_num_instances + (max_num_instances >> 1)) * ellhat_bytes;
  uint8_t* qtmp =
      vole_cache_reserve(&g_vole_scratch.qtmp, &g_vole_scratch.qtmp_cap, qtmp_need);
  uint8_t* r_workspace = vole_cache_reserve(&g_vole_scratch.r_workspace,
                                            &g_vole_scratch.r_workspace_cap,
                                            r_workspace_need);

  SIG_TSTART(t_vole_reconstruct_convert);
  // Step: 1
  unsigned int q_idx = 0;
  size_t sd_precomp_off = 0;
  for (unsigned int i = 0; i < tau; i++) {
    // Step: 2
    const unsigned int Ni = bavc_max_node_index(i, tau1, k);
    const uint8_t* sd_precomp_i = sd_precomp_base + sd_precomp_off;

    // Step: 7..8
    const unsigned int ki = convert_to_vole(params, sd_precomp_i, true, i, ellhat_bytes, NULL, qtmp,
                                            r_workspace);

    // Step 11
    if (i == 0) {
      // Step 8
      memcpy(q[q_idx], qtmp, ellhat_bytes * ki);
      q_idx += ki;
    } else {
      // Step 14
      for (unsigned int d = 0; d < ki; ++d, ++q_idx) {
        masked_xor_u8_array(qtmp + d * ellhat_bytes, c + (i - 1) * ellhat_bytes, q[q_idx],
                            (i_delta[i] >> d) & 1, ellhat_bytes);
      }
    }
    sd_precomp_off += (size_t)Ni * ellhat_bytes;
  }

  // ensure 0-padding up to lambda_f
  for (; q_idx != lambda_f; ++q_idx) {
    memset(q[q_idx], 0, ellhat_bytes);
  }
  SIG_TEND("verify.vole_convert", t_vole_reconstruct_convert);

  free(bavc_rec.s_prg288);
  free(bavc_rec.s);
  free(i_delta);
  return true;
}
