#ifndef SEEDED_KEYGEN_H
#define SEEDED_KEYGEN_H

#include <stddef.h>

#include "goppa.h"

#define KEYGEN_SEED_BYTES 64

typedef struct {
    unsigned long attempts_used;
    unsigned long support_rejects;
    unsigned long goppa_poly_rejects;
    unsigned long systematic_form_rejects;
    unsigned long goppa_init_rejects;
    int success;
} scheme_keygen_stats_t;

gfelt_t *seeded_keygen_support(int n, int l, int m, gf_t eta,
                             const unsigned char *bits,
                             size_t bit_offset, size_t bit_len);

poly_t seeded_keygen_goppa_poly(int t, int l, int sigma1, gf_t eta,
                              const unsigned char *bits,
                              size_t bit_offset, size_t bit_len);

goppa_t scheme_keygen_seeded(int n, int l, int m, int t,
                           gf_t eta_out,
                           const unsigned char *seed, size_t seed_len,
                           unsigned char **pk,
                           unsigned char *decode_map,
                           unsigned char *delta_out,
                           unsigned char *fallback_s);

void scheme_reset_last_keygen_stats(void);

const scheme_keygen_stats_t *scheme_last_keygen_stats(void);

#endif
