#include "vole.h"
#include "prg.h"
#include "utils.h"
#include "random_oracle.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef SIG_TIMING
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

static uint64_t vole_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static void vole_print_timing(const char* label, uint64_t dt_ns) {
  printf("[vole] %s: %.3f ms\n", label, (double)dt_ns / 1000000.0);
}

typedef struct {
  uint64_t prg_fixed_tweak_independent_batch_ns;
  uint64_t prg_scalar_ns;
  uint64_t mix_ns;
  uint64_t copy_ns;
} vole_convert_stats_t;
#endif

static const uint32_t TWEAK_OFFSET = UINT32_C(0x80000000); // 2^31

typedef struct {
  uint8_t* ui;
  size_t ui_cap;
  uint8_t* qtmp;
  size_t qtmp_cap;
  uint8_t* r_workspace;
  size_t r_workspace_cap;
} vole_scratch_cache_t;

static vole_scratch_cache_t g_vole_scratch = {0};

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

static inline void vole_mix_accumulate_pair(const uint8_t* left, const uint8_t* right, uint8_t* acc,
                                            uint8_t* out, size_t len) {
  size_t i = 0;
#if defined(__x86_64__) && defined(__AVX2__)
  for (; i + 32 <= len; i += 32) {
    const __m256i vl = _mm256_loadu_si256((const __m256i*)(left + i));
    const __m256i vr = _mm256_loadu_si256((const __m256i*)(right + i));
    const __m256i va = _mm256_loadu_si256((const __m256i*)(acc + i));
    _mm256_storeu_si256((__m256i*)(out + i), _mm256_xor_si256(vl, vr));
    _mm256_storeu_si256((__m256i*)(acc + i), _mm256_xor_si256(va, vr));
  }
#endif
#if defined(__x86_64__)
  for (; i + 16 <= len; i += 16) {
    const __m128i vl = _mm_loadu_si128((const __m128i*)(left + i));
    const __m128i vr = _mm_loadu_si128((const __m128i*)(right + i));
    const __m128i va = _mm_loadu_si128((const __m128i*)(acc + i));
    _mm_storeu_si128((__m128i*)(out + i), _mm_xor_si128(vl, vr));
    _mm_storeu_si128((__m128i*)(acc + i), _mm_xor_si128(va, vr));
  }
#endif
  for (; i < len; ++i) {
    const uint8_t r = right[i];
    out[i]          = left[i] ^ r;
    acc[i] ^= r;
  }
}

static inline void vole_mix_accumulate_pair_64(const uint8_t* left, const uint8_t* right,
                                                uint8_t* acc, uint8_t* out) {
#if defined(__x86_64__) && defined(__AVX2__)
  const __m256i l0 = _mm256_loadu_si256((const __m256i*)(left + 0));
  const __m256i l1 = _mm256_loadu_si256((const __m256i*)(left + 32));
  const __m256i r0 = _mm256_loadu_si256((const __m256i*)(right + 0));
  const __m256i r1 = _mm256_loadu_si256((const __m256i*)(right + 32));
  const __m256i a0 = _mm256_loadu_si256((const __m256i*)(acc + 0));
  const __m256i a1 = _mm256_loadu_si256((const __m256i*)(acc + 32));

  _mm256_storeu_si256((__m256i*)(out + 0), _mm256_xor_si256(l0, r0));
  _mm256_storeu_si256((__m256i*)(out + 32), _mm256_xor_si256(l1, r1));
  _mm256_storeu_si256((__m256i*)(acc + 0), _mm256_xor_si256(a0, r0));
  _mm256_storeu_si256((__m256i*)(acc + 32), _mm256_xor_si256(a1, r1));
  return;
#elif defined(__x86_64__)
  const __m128i l0 = _mm_loadu_si128((const __m128i*)(left + 0));
  const __m128i l1 = _mm_loadu_si128((const __m128i*)(left + 16));
  const __m128i l2 = _mm_loadu_si128((const __m128i*)(left + 32));
  const __m128i l3 = _mm_loadu_si128((const __m128i*)(left + 48));
  const __m128i r0 = _mm_loadu_si128((const __m128i*)(right + 0));
  const __m128i r1 = _mm_loadu_si128((const __m128i*)(right + 16));
  const __m128i r2 = _mm_loadu_si128((const __m128i*)(right + 32));
  const __m128i r3 = _mm_loadu_si128((const __m128i*)(right + 48));
  const __m128i a0 = _mm_loadu_si128((const __m128i*)(acc + 0));
  const __m128i a1 = _mm_loadu_si128((const __m128i*)(acc + 16));
  const __m128i a2 = _mm_loadu_si128((const __m128i*)(acc + 32));
  const __m128i a3 = _mm_loadu_si128((const __m128i*)(acc + 48));

  _mm_storeu_si128((__m128i*)(out + 0), _mm_xor_si128(l0, r0));
  _mm_storeu_si128((__m128i*)(out + 16), _mm_xor_si128(l1, r1));
  _mm_storeu_si128((__m128i*)(out + 32), _mm_xor_si128(l2, r2));
  _mm_storeu_si128((__m128i*)(out + 48), _mm_xor_si128(l3, r3));
  _mm_storeu_si128((__m128i*)(acc + 0), _mm_xor_si128(a0, r0));
  _mm_storeu_si128((__m128i*)(acc + 16), _mm_xor_si128(a1, r1));
  _mm_storeu_si128((__m128i*)(acc + 32), _mm_xor_si128(a2, r2));
  _mm_storeu_si128((__m128i*)(acc + 48), _mm_xor_si128(a3, r3));
  return;
#endif

  for (size_t w = 0; w < 8; ++w) {
    uint64_t l;
    uint64_t r;
    uint64_t a;
    memcpy(&l, left + 8u * w, sizeof(uint64_t));
    memcpy(&r, right + 8u * w, sizeof(uint64_t));
    memcpy(&a, acc + 8u * w, sizeof(uint64_t));
    const uint64_t o = l ^ r;
    a ^= r;
    memcpy(out + 8u * w, &o, sizeof(uint64_t));
    memcpy(acc + 8u * w, &a, sizeof(uint64_t));
  }
}


static unsigned int convert_to_vole(const params_t* params, const uint8_t* iv,
                                    const uint8_t* sd, const uint8_t* sd_precomp, bool sd0_bot,
                                    unsigned int i, unsigned int outlen, uint8_t* u, uint8_t* v,
                                    uint8_t* r
#ifdef SIG_TIMING
                                    , vole_convert_stats_t* stats
#endif
                                    ) {
  const unsigned int lambda        = params->lambda;
  const unsigned int tau_1         = params->tau1;
  const unsigned int k             = params->k;
  const unsigned int num_instances = bavc_max_node_index(i, tau_1, k);
  const unsigned int lambda_bytes  = lambda / 8;
  const unsigned int depth         = bavc_max_node_depth(i, tau_1, k);

  assert(r);

#define R(row, column) (r + (((row) % 2) * num_instances + (column)) * outlen)
#define V(idx) (v + (idx) * outlen)

  uint32_t tweak = i ^ TWEAK_OFFSET;
  if (sd0_bot) {
    memset(R(0, 0), 0, outlen);
  }
  // Step: 2
  if (lambda == 256u || lambda == 512u) {
    const unsigned int start_idx = sd0_bot ? 1u : 0u;
    if (start_idx < num_instances) {
#ifdef SIG_TIMING
      const uint64_t t_prg = vole_now_ns();
#endif
      prg_fixed_tweak_independent_batch(sd + (size_t)start_idx * lambda_bytes, lambda_bytes, iv,
                                        tweak, R(0, start_idx), outlen, lambda, outlen,
                                        num_instances - start_idx);
#ifdef SIG_TIMING
      if (stats) {
        stats->prg_fixed_tweak_independent_batch_ns += vole_now_ns() - t_prg;
      }
#endif
    }
  } else {
#ifdef SIG_TIMING
    const uint64_t t_prg = vole_now_ns();
#endif
    if (!sd0_bot) {
      prg(sd, iv, tweak, R(0, 0), lambda, outlen);
    }

    // Step: 3..4
    for (unsigned int j = 1; j < num_instances; ++j) {
      prg(sd + lambda_bytes * j, iv, tweak, R(0, j), lambda, outlen);
    }
#ifdef SIG_TIMING
    if (stats) {
      stats->prg_scalar_ns += vole_now_ns() - t_prg;
    }
#endif
  }

#ifdef SIG_TIMING
  uint64_t t_stage = vole_now_ns();
#endif

  // Step: 5..9
  (void)sd_precomp;
  for (unsigned int j = 0; j < depth; j++) {
    const unsigned int depthloop = num_instances >> (j + 1);
    uint8_t* v_row = V(j);
    memset(v_row, 0, outlen);
    uint8_t* in_row = r + (size_t)(j & 1u) * num_instances * outlen;
    uint8_t* out_row = r + (size_t)((j + 1u) & 1u) * num_instances * outlen;
    for (unsigned int idx = 0; idx < depthloop; idx++) {
      if (outlen == 64u) {
        vole_mix_accumulate_pair_64(in_row, in_row + 64u, v_row, out_row);
      } else {
        vole_mix_accumulate_pair(in_row, in_row + outlen, v_row, out_row, outlen);
      }
      in_row += 2u * outlen;
      out_row += outlen;
    }
  }

#ifdef SIG_TIMING
  if (stats) {
    stats->mix_ns += vole_now_ns() - t_stage;
  }
  t_stage = vole_now_ns();
#endif

  // Step: 10
  if (!sd0_bot && u != NULL) {
    memcpy(u, R(depth, 0), outlen);
  }

#ifdef SIG_TIMING
  if (stats) {
    stats->copy_ns += vole_now_ns() - t_stage;
  }
#endif

  return depth;
}

void vole_commit(const params_t* params, const uint8_t* rootKey, const uint8_t* iv,
                 unsigned int ellhat, bavc_t* bavc, uint8_t* c, uint8_t* u, uint8_t** v) {
  const unsigned int lambda       = params->lambda;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int ellhat_bytes = (ellhat + 7) / 8;
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int k            = params->k;

#ifdef SIG_TIMING
  const uint64_t t_total = vole_now_ns();
#endif

#ifdef SIG_TIMING
  const uint64_t t_bavc = vole_now_ns();
#endif
  bavc_commit(params, bavc, rootKey, iv);
#ifdef SIG_TIMING
  const uint64_t dt_bavc = vole_now_ns() - t_bavc;
#endif

  const unsigned int max_num_instances = bavc_max_node_index(0, tau_1, k);
  const unsigned int scratch_instances = max_num_instances + (max_num_instances >> 1);
  const size_t ui_need = (size_t)tau * ellhat_bytes;
  const size_t r_workspace_need = (size_t)scratch_instances * ellhat_bytes;
  uint8_t* ui =
      vole_cache_reserve(&g_vole_scratch.ui, &g_vole_scratch.ui_cap, ui_need);
  uint8_t* r_workspace = vole_cache_reserve(&g_vole_scratch.r_workspace,
                                            &g_vole_scratch.r_workspace_cap,
                                            r_workspace_need);

#ifdef SIG_TIMING
  vole_convert_stats_t convert_stats = {0};
  uint64_t convert_total_ns          = 0;
#endif

  unsigned int v_idx = 0;
  uint8_t* sd_i      = bavc->sd;
  for (unsigned int i = 0; i < tau; ++i) {
#ifdef SIG_TIMING
    const uint64_t t_convert = vole_now_ns();
#endif
    // Step 6
    v_idx += convert_to_vole(params, iv, sd_i, NULL, false, i, ellhat_bytes,
                   ui + i * ellhat_bytes, v[v_idx], r_workspace
#ifdef SIG_TIMING
                   , &convert_stats
#endif
                   );
#ifdef SIG_TIMING
    convert_total_ns += vole_now_ns() - t_convert;
#endif
    const size_t ni = (size_t)bavc_max_node_index(i, tau_1, k);
    sd_i += lambda_bytes * ni;
  }

  // ensure 0-padding up to lambda
  for (; v_idx != lambda; ++v_idx) {
    memset(v[v_idx], 0, ellhat_bytes);
  }

#ifdef SIG_TIMING
  const uint64_t t_post = vole_now_ns();
#endif

  // Step 9
  memcpy(u, ui, ellhat_bytes);
  for (unsigned int i = 1; i < tau; i++) {
    // Step 11
    xor_u8_array(u, ui + i * ellhat_bytes, c + (i - 1) * ellhat_bytes, ellhat_bytes);
  }

#ifdef SIG_TIMING
  const uint64_t dt_post  = vole_now_ns() - t_post;
  const uint64_t dt_total = vole_now_ns() - t_total;
  const uint64_t convert_prg_total_ns = convert_stats.prg_fixed_tweak_independent_batch_ns +
                                        convert_stats.prg_scalar_ns;
  const uint64_t convert_other_ns =
      (convert_total_ns > convert_prg_total_ns) ? (convert_total_ns - convert_prg_total_ns) : 0;
  vole_print_timing("detail.sign.vole_commit.bavc_commit", dt_bavc);
  vole_print_timing("detail.sign.vole_commit.convert.total", convert_total_ns);
  vole_print_timing("detail.sign.vole_commit.convert.prg_fixed_tweak_independent_batch",
                    convert_stats.prg_fixed_tweak_independent_batch_ns);
  vole_print_timing("detail.sign.vole_commit.convert.prg_scalar", convert_stats.prg_scalar_ns);
  vole_print_timing("detail.sign.vole_commit.convert.other", convert_other_ns);
  vole_print_timing("detail.sign.vole_commit.convert.get_u&v_via_divide_and_conqure", convert_stats.mix_ns);
  vole_print_timing("detail.sign.vole_commit.convert.copy", convert_stats.copy_ns);
  vole_print_timing("detail.sign.vole_commit.post", dt_post);
  vole_print_timing("detail.sign.vole_commit.total", dt_total);
#endif
}

bool vole_reconstruct(const params_t* params, uint8_t* com, uint8_t** q, const uint8_t* iv,
                      const uint8_t* chall_3, const uint8_t* decom_i, const uint8_t* c,
                      unsigned int ellhat) {
  const unsigned int lambda       = params->lambda;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int ellhat_bytes = (ellhat + 7) / 8;
  const unsigned int tau          = params->tau;
  const unsigned int tau1         = params->tau1;
  const unsigned int L            = params->L;
  const unsigned int k            = params->k;

#ifdef SIG_TIMING
  const uint64_t t_total = vole_now_ns();
#endif

  uint16_t* i_delta = malloc(sizeof(uint16_t) * tau);
  assert(i_delta);

#ifdef SIG_TIMING
  const uint64_t t_decode = vole_now_ns();
#endif
  if (!decode_all_chall_3(params, i_delta, chall_3)) {
    free(i_delta);
    return false;
  }
#ifdef SIG_TIMING
  const uint64_t dt_decode = vole_now_ns() - t_decode;
#endif

  bavc_rec_t bavc_rec;
  bavc_rec.h = com;
  bavc_rec.s = malloc((size_t)L * lambda_bytes);
  assert(bavc_rec.s);

#ifdef SIG_TIMING
  const uint64_t t_bavc_rec = vole_now_ns();
#endif
  if (!bavc_reconstruct(params, &bavc_rec, decom_i, i_delta, iv)) {
    free(bavc_rec.s);
    free(i_delta);
    return false;
  }
#ifdef SIG_TIMING
  const uint64_t dt_bavc_rec = vole_now_ns() - t_bavc_rec;
#endif

  const unsigned int max_num_instances = bavc_max_node_index(0, tau1, k);
  const unsigned int scratch_instances = max_num_instances + (max_num_instances >> 1);
  const size_t qtmp_need = (size_t)params->k * ellhat_bytes;
  const size_t r_workspace_need = (size_t)scratch_instances * ellhat_bytes;
  uint8_t* qtmp =
      vole_cache_reserve(&g_vole_scratch.qtmp, &g_vole_scratch.qtmp_cap, qtmp_need);
  uint8_t* r_workspace = vole_cache_reserve(&g_vole_scratch.r_workspace,
                                            &g_vole_scratch.r_workspace_cap,
                                            r_workspace_need);

#ifdef SIG_TIMING
  vole_convert_stats_t convert_stats = {0};
  uint64_t convert_total_ns          = 0;
  uint64_t q_update_ns               = 0;
#endif

  // Step: 1
  unsigned int q_idx = 0;
  uint8_t* sd_i      = bavc_rec.s;
  for (unsigned int i = 0; i < tau; i++) {
      // Step: 2
      const unsigned int Ni = bavc_max_node_index(i, tau1, k);

      // Step: 7..8
#ifdef SIG_TIMING
      const uint64_t t_convert = vole_now_ns();
#endif
      const unsigned int ki = convert_to_vole(params, iv, sd_i, NULL, true, i, ellhat_bytes, NULL, qtmp,
                          r_workspace
    #ifdef SIG_TIMING
                          , &convert_stats
    #endif
                          );
#ifdef SIG_TIMING
      convert_total_ns += vole_now_ns() - t_convert;
#endif

      // Step 11
#ifdef SIG_TIMING
      const uint64_t t_q_update = vole_now_ns();
#endif
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
#ifdef SIG_TIMING
      q_update_ns += vole_now_ns() - t_q_update;
#endif
      sd_i += (size_t)lambda_bytes * Ni;
  }

  // ensure 0-padding up to lambda
  for (; q_idx != lambda; ++q_idx) {
    memset(q[q_idx], 0, ellhat_bytes);
  }

#ifdef SIG_TIMING
  const uint64_t dt_total = vole_now_ns() - t_total;
  const uint64_t convert_prg_total_ns = convert_stats.prg_fixed_tweak_independent_batch_ns +
                                        convert_stats.prg_scalar_ns;
  const uint64_t convert_other_ns =
      (convert_total_ns > convert_prg_total_ns) ? (convert_total_ns - convert_prg_total_ns) : 0;
  vole_print_timing("verify.vole_reconstruct.decode_chall3", dt_decode);
  vole_print_timing("verify.vole_reconstruct.bavc_reconstruct", dt_bavc_rec);
  vole_print_timing("verify.vole_reconstruct.convert.total", convert_total_ns);
  vole_print_timing("verify.vole_reconstruct.convert.prg_fixed_tweak_independent_batch",
                    convert_stats.prg_fixed_tweak_independent_batch_ns);
  vole_print_timing("verify.vole_reconstruct.convert.prg_scalar", convert_stats.prg_scalar_ns);
  vole_print_timing("verify.vole_reconstruct.convert.other", convert_other_ns);
  vole_print_timing("verify.vole_reconstruct.convert.get_u&v_via_divide_and_conqure", convert_stats.mix_ns);
  vole_print_timing("verify.vole_reconstruct.convert.copy", convert_stats.copy_ns);
  vole_print_timing("verify.vole_reconstruct.q_update", q_update_ns);
  vole_print_timing("verify.vole_reconstruct.total", dt_total);
#endif

  free(bavc_rec.s);
  free(i_delta);
  return true;
}
