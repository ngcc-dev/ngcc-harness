#ifndef VIPER_H
#define VIPER_H

/* MAMBA-Viper implementation and implementation support layer where applicable. */

#include <stddef.h>
#include <stdint.h>
#include "viper_params.h"

typedef uint16_t vpoly[VIPER_N];
typedef vpoly vpolyvec[VIPER_K];

void viper_pack_bits(unsigned char *out, const uint16_t *in, size_t n, unsigned bits);
void viper_unpack_bits(uint16_t *out, const unsigned char *in, size_t n, unsigned bits);
uint16_t viper_quantize(uint16_t x, uint16_t d, unsigned t);
uint16_t viper_reconstruct(uint16_t b, uint16_t d, unsigned t);

void viper_quantize_pack_t10_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither);
void viper_quantize_pack_t9_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither);
void viper_quantize_pack_t4_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither);
void viper_quantize_pack_t3_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither);
void viper_quantize_pack_array(unsigned char *out, const uint16_t *poly, const uint16_t *dither, unsigned bits);
void viper_unpack_reconstruct_t10_array(uint16_t *out, const unsigned char *in, const uint16_t *dither);
void viper_unpack_reconstruct_t9_array(uint16_t *out, const unsigned char *in, const uint16_t *dither);
void viper_unpack_reconstruct_t4_array(uint16_t *out, const unsigned char *in, const uint16_t *dither);
void viper_unpack_reconstruct_t3_array(uint16_t *out, const unsigned char *in, const uint16_t *dither);
void viper_unpack_reconstruct_array(uint16_t *out, const unsigned char *in, const uint16_t *dither, unsigned bits);
void viper_gen_dither(uint16_t du[VIPER_K][VIPER_N], uint16_t dv[VIPER_N], const unsigned char mu[VIPER_MU_BYTES]);
void viper_gen_public_shake(unsigned char *buf, const unsigned char rho[32]);
void viper_gen_public_parse_A(vpoly A[VIPER_K][VIPER_K], const unsigned char *buf);
void viper_gen_public_parse_dpk(uint16_t dpk[VIPER_K][VIPER_N], const unsigned char *buf);
void viper_gen_public(vpoly A[VIPER_K][VIPER_K], uint16_t dpk[VIPER_K][VIPER_N], const unsigned char rho[32]);
void viper_genpublic_matvec_fused_experiment(vpolyvec out, uint16_t dpk[VIPER_K][VIPER_N], const unsigned char rho[32], const vpolyvec s, int transpose);
void viper_sample_secret(vpolyvec s, const unsigned char seed[32], unsigned eta);
void viper_poly_mul_schoolbook_oracle(vpoly c, const vpoly a, const vpoly b);
void viper_encode(vpoly out, const unsigned char m[VIPER_MSGBYTES]);
void viper_decode(unsigned char m[VIPER_MSGBYTES], const vpoly in);
void viper_pke_keypair(unsigned char *pk, unsigned char *skpke, const unsigned char rho[32], const unsigned char sseed[32]);
void viper_pke_enc(unsigned char *ct, const unsigned char *pk, const unsigned char m[VIPER_MSGBYTES], const unsigned char omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES]);
void viper_pke_dec(unsigned char m[VIPER_MSGBYTES], const unsigned char *skpke, const unsigned char *ct);
int viper_reencrypt_check(const unsigned char *ct, const unsigned char *pk, const unsigned char m[VIPER_MSGBYTES], const unsigned char sigma[VIPER_FALLBACK_KEY_BYTES]);

#endif
