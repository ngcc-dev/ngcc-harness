/*
 * Batch all-but-one verifiable commitments (BAVC)
 */

#ifndef BAVC_H
#define BAVC_H

#include <assert.h>
#include <stdint.h>

#include "params.h"
#include "utils.h"


typedef struct bavc_t {
  uint8_t* h;
  uint8_t* k;
  uint8_t* com;
  uint8_t* sd;
  uint8_t* sd_prg162;
} bavc_t;

typedef struct bavc_rec_t {
  uint8_t* h;
  uint8_t* s;
  uint8_t* s_prg162;
} bavc_rec_t;

static inline unsigned int bavc_max_node_depth(unsigned int i, unsigned int tau_1,
                                                          unsigned int k) {
  return (i < tau_1) ? k : (k - 1);
}

static inline unsigned int bavc_max_node_index(unsigned int i, unsigned int tau_1,
                                                          unsigned int k) {
  // for scan-build
  assert(k >= 1);
  return 1u << ((i < tau_1) ? k : (k - 1));
}

void bavc_commit(const params_t* params, bavc_t* bavc, const uint8_t* root_key, const uint8_t* iv);

bool bavc_open(const params_t* params, uint8_t* decom_i, const bavc_t* vc, const uint16_t* i_delta);

bool bavc_reconstruct(const params_t* params, bavc_rec_t* bavc_rec, const uint8_t* decom_i,
                      const uint16_t* i_delta, const uint8_t* iv);

void bavc_clear(bavc_t* com);

#endif
