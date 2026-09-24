#ifndef FROST_U32_CORE_H
#define FROST_U32_CORE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t *arow;
    uint8_t *arow_bytes;
    size_t row_count;
} frost_u32_workspace_t;

typedef struct {
    uint64_t expand_row_calls;
    uint64_t shake_init_calls;
    uint64_t shake_squeeze_calls;
    uint64_t bytes_squeezed_for_a;
    uint64_t a_rows_per_shake_batch;
    uint64_t shake4x_used;
    uint64_t coeff_parse_mode;
    uint64_t mac_ops;
    uint64_t matrix_products;
} frost_u32_fast_stats_t;

int frost_mul_add_as_plus_e_u32(uint32_t *out, const int32_t *s, const uint32_t *e, const uint8_t *seed_A, frost_u32_workspace_t *ws);
int frost_mul_add_sa_plus_e_u32(uint32_t *out, const int32_t *s, const uint32_t *e, const uint8_t *seed_A, frost_u32_workspace_t *ws);
void frost_mul_bs_u32(uint32_t *out, const uint32_t *b, const int32_t *s);
void frost_mul_add_sb_plus_e_u32(uint32_t *out, const uint32_t *b, const int32_t *s, const uint32_t *e);

void frost_pack_u32(unsigned char *out, size_t outlen, const uint32_t *in, size_t inlen, unsigned bits);
void frost_unpack_u32(uint32_t *out, size_t outlen, const unsigned char *in, size_t inlen, unsigned bits);
void frost_key_encode_u32(uint32_t *out, const uint8_t *in);
void frost_key_decode_u32(uint8_t *out, const uint32_t *in);
const char *frost_message_codec_name(void);
#ifdef FROST_CODEC_TRACE
void frost_codec_trace_reset(void);
unsigned long frost_codec_trace_e8_encode_calls(void);
unsigned long frost_codec_trace_e8_decode_calls(void);
#endif

void frost_sample_n_u32(int32_t *s, const uint16_t *rnd, size_t n, unsigned eta);
void frost_u32_fast_stats_reset(void);
frost_u32_fast_stats_t frost_u32_fast_stats_get(void);

#endif
