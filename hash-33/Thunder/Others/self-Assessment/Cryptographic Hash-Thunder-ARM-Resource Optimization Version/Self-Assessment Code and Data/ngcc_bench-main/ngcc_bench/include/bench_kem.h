#ifndef BENCH_KEM_H
#define BENCH_KEM_H

#include "bench_core.h"
#include "ngcc_api.h"

int ngcc_kem_correctness(const ngcc_api_t *api);
int ngcc_kem_correctness_kat_file(const ngcc_api_t *api,
                                  const char *kat_path,
                                  unsigned long long *out_total,
                                  unsigned long long *out_passed,
                                  unsigned long long *out_failed);
int ngcc_kem_performance(const ngcc_api_t *api,
                         const ngcc_perf_config_t *cfg,
                         ngcc_perf_result_t *out_result);
int ngcc_kem_keygen_performance(const ngcc_api_t *api,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result);
int ngcc_kem_encap_performance(const ngcc_api_t *api,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result);
int ngcc_kem_decap_performance(const ngcc_api_t *api,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result);

#endif
