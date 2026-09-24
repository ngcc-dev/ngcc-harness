/********************************************************************************************
* MAMBA-Frost-CC: unstructured LWQ-Z key encapsulation mechanism.
*
* Abstract: header for internal functions.
*
*********************************************************************************************/

#ifndef _FROST_MACRIFY_H_
#define _FROST_MACRIFY_H_

#include <stddef.h>
#include <stdint.h>
#include "config.h"


void frost_pack(unsigned char *out, const size_t outlen, const uint16_t *in, const size_t inlen, const unsigned char lsb);
void frost_unpack(uint16_t *out, const size_t outlen, const unsigned char *in, const size_t inlen, const unsigned char lsb);
void frost_sample_n(uint16_t *s, const size_t n);
int8_t ct_verify(const uint16_t *a, const uint16_t *b, size_t len);
void ct_select(uint8_t *r, const uint8_t *a, const uint8_t *b, size_t len, int8_t selector);
void clear_bytes(uint8_t *mem, size_t n);

int frost_mul_add_as_plus_e(uint16_t *b, const uint16_t *s, const uint16_t *e, const uint8_t *seed_A);
int frost_mul_add_sa_plus_e(uint16_t *b, const uint16_t *s, uint16_t *e, const uint8_t *seed_A);
void frost_mul_add_sb_plus_e(uint16_t *out, const uint16_t *b, const uint16_t *s, const uint16_t *e);
void frost_mul_bs(uint16_t *out, const uint16_t *b, const uint16_t *s);

void frost_add(uint16_t *out, const uint16_t *a, const uint16_t *b);
void frost_sub(uint16_t *out, const uint16_t *a, const uint16_t *b);
void frost_key_encode(uint16_t *out, const uint16_t *in);
void frost_key_decode(uint16_t *out, const uint16_t *in);
const char *frost_message_codec_name(void);
#ifdef FROST_CODEC_TRACE
void frost_codec_trace_reset(void);
unsigned long frost_codec_trace_e8_encode_calls(void);
unsigned long frost_codec_trace_e8_decode_calls(void);
#endif

#endif
