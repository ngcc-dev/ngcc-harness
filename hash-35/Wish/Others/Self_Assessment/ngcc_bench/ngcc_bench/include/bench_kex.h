#ifndef BENCH_KEX_H
#define BENCH_KEX_H

#include "bench_core.h"
#include "ngcc_api.h"

int ngcc_kex_correctness(const ngcc_api_t *api);
int ngcc_kex_correctness_kat_file(const ngcc_api_t *api,
                                  const char *kat_path,
                                  unsigned long long *out_total,
                                  unsigned long long *out_passed,
                                  unsigned long long *out_failed);
int ngcc_kex_derive_ss_performance(const ngcc_api_t *api,
                                   const ngcc_perf_config_t *cfg,
                                   ngcc_perf_result_t *out_a,
                                   ngcc_perf_result_t *out_b);

#endif
