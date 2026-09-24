#include "vistrutith_constraints.h"

#include "../utils.h"

#include <assert.h>
#include <string.h>

/*
 * vi-SBox affine coefficients zeta_0..zeta_8 in GF(2^512).
 *
 * Derivation:
 * 1) Write AES affine map as a linearized polynomial over GF(2^8):
 *      A(z) = chi_8 + sum_{k=0}^7 chi_k * z^(2^k),
 *    where (chi_0..chi_7) = (0x05,0x09,0xf9,0x25,0xf4,0x01,0xb5,0x8f)
 *    and chi_8 = 0x63.
 * 2) Embed each byte coefficient into GF(2^512) with the vi-compatible map:
 *      zeta_k = bf512_byte_combine_bits(chi_k), k=0..8.
 * 3) Materialize zeta_k as BF512 constants below.
 */
static const bf512_t vistrutith_sbox_affine_coeffs[9] = {
    BF512C(UINT64_C(0xe76ef44d9176df23), UINT64_C(0x7d1fb2306449111b),
           UINT64_C(0xd345568e76e43bfd), UINT64_C(0x0058b1372714488a),
           UINT64_C(0x5abe43a7a38b5ce7), UINT64_C(0xa8fac14641908164),
           UINT64_C(0x8b453f58ecaa6860), UINT64_C(0x90b4b2bd7d5eb34b)),
    BF512C(UINT64_C(0x5de7a890b1db560a), UINT64_C(0xb6da7d8cfc4655b2),
           UINT64_C(0x450a93e2819878ff), UINT64_C(0xe313e7568f4207d9),
           UINT64_C(0xd4b359f402b8ca64), UINT64_C(0xa2e2a68fd71ad70b),
           UINT64_C(0x649911d470a0dd8e), UINT64_C(0x115c71b3ad136af6)),
    BF512C(UINT64_C(0xbcabdb6584b5854e), UINT64_C(0x5967ca5dec26babc),
           UINT64_C(0x0e50c47de8c87c57), UINT64_C(0x4e26bc7de74c1d0e),
           UINT64_C(0xfed826b695125e50), UINT64_C(0x81d9aa6bdbd748ab),
           UINT64_C(0xf8e0b245e4c545fa), UINT64_C(0x565ca04a7319b58f)),
    BF512C(UINT64_C(0xc991e1217957debc), UINT64_C(0x1726552ffa75ac69),
           UINT64_C(0x3612649e5f30209f), UINT64_C(0x6642df4c031c72ea),
           UINT64_C(0xbd965f294078bc28), UINT64_C(0xaa008f18c23ff9e6),
           UINT64_C(0xe4866f93050c33fa), UINT64_C(0x4032097db0918a00)),
    BF512C(UINT64_C(0x062287b8a4180c66), UINT64_C(0x92a205e17429fe15),
           UINT64_C(0x981f01111fb43f55), UINT64_C(0xad6dea1c4f1a525d),
           UINT64_C(0x70d53ce53421c8d3), UINT64_C(0x8bc1cda24d5d1ec4),
           UINT64_C(0x173c9cc978cff014), UINT64_C(0xd7b46344a3546c32)),
    BF512C(UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)),
    BF512C(UINT64_C(0x47fb6ee6e8cedb57), UINT64_C(0xa4efa5560d520c45),
           UINT64_C(0xefaf798b2c4436fa), UINT64_C(0x38b23b8198c60cf6),
           UINT64_C(0xd199a9ff4e1d824f), UINT64_C(0x4dc8e08228276d87),
           UINT64_C(0xd95dac6af1501687), UINT64_C(0xa7874c46172889da)),
    BF512C(UINT64_C(0x75105a3735def006), UINT64_C(0xd2100f36d840d5fe),
           UINT64_C(0xf431dd18ee976463), UINT64_C(0x6592877c892da3e9),
           UINT64_C(0xb0be31d7d2b59897), UINT64_C(0xf0870f386632695a),
           UINT64_C(0x56d4848c8372baa4), UINT64_C(0xe026ec9d2b31cf89)),
    BF512C(UINT64_C(0xbe2c6ce95f077de6), UINT64_C(0x9ae5cd9950c3ada4),
           UINT64_C(0x6c593ae33089f4cf), UINT64_C(0xa3a367ec1d46b5bb),
           UINT64_C(0xde9199146c3f0b3c), UINT64_C(0x8442377402d3dcde),
           UINT64_C(0x9a4222496ffecbc2), UINT64_C(0x43820c3695af677a))};

/*
 * Squared coefficients used when sq=true:
 *   zeta_k_sq = zeta_k^2 (Frobenius square in GF(2^512)).
 *
 * They realize A(z)^2 = zeta_8^2 + sum_{k=0}^7 zeta_k^2 * z^(2^(k+1)),
 * i.e., same linearized affine map on conjugates with one-index shift.
 */
static const bf512_t vistrutith_sbox_affine_coeffs_sq[9] = {
    BF512C(UINT64_C(0x90f919f67f1ad78c), UINT64_C(0x6c8dbac200ecd3fd),
           UINT64_C(0x457d75084032d706), UINT64_C(0x884fedda543343d6),
           UINT64_C(0xca49cdd28813911c), UINT64_C(0x5de65361fea67c4b),
           UINT64_C(0xbf332bd8e4e35cfc), UINT64_C(0x654cf25cb0d9ae37)),
    BF512C(UINT64_C(0x41d9e95e4cd6d731), UINT64_C(0x364da0b7797bf250),
           UINT64_C(0x77b0789a33f009af), UINT64_C(0x95dfd19dd7dc5eab),
           UINT64_C(0xa14c951a7a3c4a9c), UINT64_C(0xc6092d20657a7343),
           UINT64_C(0xce6130a3899fe693), UINT64_C(0x70332f02b47ce5e8)),
    BF512C(UINT64_C(0x366464966a86746b), UINT64_C(0xbb8e3801d3cdf39d),
           UINT64_C(0x2dfb26e75c49ddff), UINT64_C(0x503e693dc98699fa),
           UINT64_C(0xc24b5327567bfd88), UINT64_C(0xe84b954ca596567b),
           UINT64_C(0xb0a57d79e36d1eab), UINT64_C(0x73832a4991420892)),
    BF512C(UINT64_C(0x4556b919fb40880a), UINT64_C(0xfb3c32d67fa4d876),
           UINT64_C(0x41d5faeead6a86c9), UINT64_C(0x98c1045d0fb1684e),
           UINT64_C(0x02205e15b0efadcc), UINT64_C(0x930d57d68ef921e5),
           UINT64_C(0xf14d653c18d0541b), UINT64_C(0x4411a5901927ab29)),
    BF512C(UINT64_C(0xe744943e594a74d7), UINT64_C(0xe14e2274aa5ad230),
           UINT64_C(0x1f362b752f8b0356), UINT64_C(0x4dae557a4a698487),
           UINT64_C(0xa94e0befa4542608), UINT64_C(0x73a4eb0d3e4a5973),
           UINT64_C(0xc1f766028e11a4c4), UINT64_C(0x66fcf71795e7434d)),
    BF512C(UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)),
    BF512C(UINT64_C(0x5bc52f2815c35a6d), UINT64_C(0x2478786d886faba7),
           UINT64_C(0xdd1592f39e2c47aa), UINT64_C(0x4e7e0d4ac0585584),
           UINT64_C(0xa4666511369902b7), UINT64_C(0x29236b2d9a47c9cf),
           UINT64_C(0x73a58d1d086f2d9a), UINT64_C(0xc6e812f70e4706c4)),
    BF512C(UINT64_C(0x34c9b36979082736), UINT64_C(0xe45daf81a13b27ae),
           UINT64_C(0x8381a582dd676dcc), UINT64_C(0xf04d56e15ef1fd42),
           UINT64_C(0x11f2a4cda889d20b), UINT64_C(0x368e221803481a19),
           UINT64_C(0x98b5b42f0aed5c37), UINT64_C(0x9015c39f9f4d2a61)),
    BF512C(UINT64_C(0x8e40efb459a5ae1e), UINT64_C(0x2f98603d39346307),
           UINT64_C(0x15ce60ee2a1b2ece), UINT64_C(0x13060080f6a7b211),
           UINT64_C(0x9fffbe9e09ba4488), UINT64_C(0x3c9645d195c24c76),
           UINT64_C(0x77699aa396e7e9d9), UINT64_C(0x11fd00914f00f3dc))};

/*
 * MixColumns linearized coefficients over GF(2^512).
 * v1, v2, v3 correspond to multiplication by {01}, {02}, {03} in the
 * vi-compatible embedding; _sq are their Frobenius squares for sq=true.
 */
static const bf512_t vistrutith_mixcolumns_coeffs[3] = {
    BF512C(UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)),
    BF512C(UINT64_C(0xd10a90dbfbf0ab48), UINT64_C(0xc6918a31b784e286),
           UINT64_C(0xfebe70692aade602), UINT64_C(0x5066d80aee92d170),
           UINT64_C(0x98f51080f5f0a16f), UINT64_C(0x40b1540ae406d71f),
           UINT64_C(0x3be042210fc776cb), UINT64_C(0xe33798f4ec1cbbd9)),
    BF512C(UINT64_C(0xd10a90dbfbf0ab49), UINT64_C(0xc6918a31b784e286),
           UINT64_C(0xfebe70692aade602), UINT64_C(0x5066d80aee92d170),
           UINT64_C(0x98f51080f5f0a16f), UINT64_C(0x40b1540ae406d71f),
           UINT64_C(0x3be042210fc776cb), UINT64_C(0xe33798f4ec1cbbd9))};

static const bf512_t vistrutith_mixcolumns_coeffs_sq[3] = {
    BF512C(UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000),
           UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000)),
    BF512C(UINT64_C(0xe76ef44d9176df22), UINT64_C(0x7d1fb2306449111b),
           UINT64_C(0xd345568e76e43bfd), UINT64_C(0x0058b1372714488a),
           UINT64_C(0x5abe43a7a38b5ce7), UINT64_C(0xa8fac14641908164),
           UINT64_C(0x8b453f58ecaa6860), UINT64_C(0x90b4b2bd7d5eb34b)),
    BF512C(UINT64_C(0xe76ef44d9176df23), UINT64_C(0x7d1fb2306449111b),
           UINT64_C(0xd345568e76e43bfd), UINT64_C(0x0058b1372714488a),
           UINT64_C(0x5abe43a7a38b5ce7), UINT64_C(0xa8fac14641908164),
           UINT64_C(0x8b453f58ecaa6860), UINT64_C(0x90b4b2bd7d5eb34b))};

/*
 * InvNormToConjugates basis constants.
 *
 * Let beta8 be the generator used by bf512_byte_combine(), and define
 *   beta4 = beta8^6 + beta8^4  (the chosen GF(2^4) basis element).
 *
 * vistrutith_beta4_squares[j] = beta4^(2^j), for j=0..4.
 * j=4 repeats j=0 because beta4^(2^4) = beta4 inside GF(2^4).
 */
static const bf512_t vistrutith_beta4_squares[5] = {
    BF512C(UINT64_C(0xd120f0a833cc00bd), UINT64_C(0x5ac01a75799721ad),
           UINT64_C(0x32cd0d9273c2dea9), UINT64_C(0x1d903c4783ef1d7d),
           UINT64_C(0x6b0558c8f22fdb80), UINT64_C(0x9bef7e419bdc0f08),
           UINT64_C(0x71521b7b6d7cba6f), UINT64_C(0x157fdd5e04a54bdf)),
    BF512C(UINT64_C(0xa0959aab79b80474), UINT64_C(0xd9f01766691b1d5e),
           UINT64_C(0x3cea2f055aa00d07), UINT64_C(0x38ea8ab6bfd2447c),
           UINT64_C(0x8b27ea58ed96dea8), UINT64_C(0xe53221c469b7ece3),
           UINT64_C(0x521893321dfa7ee7), UINT64_C(0x3733fefb6a763a91)),
    BF512C(UINT64_C(0xcb3c36de6ad98de1), UINT64_C(0x48f5c2af8883785a),
           UINT64_C(0x9868e7fbde1e90ac), UINT64_C(0xc631e090946b1652),
           UINT64_C(0x6e2fa8c3be8a93ab), UINT64_C(0x74c5384c64e1b584),
           UINT64_C(0xcc96a6c5ec8c7166), UINT64_C(0xa3a4e0abbe9ea8f3)),
    BF512C(UINT64_C(0xba895cdd20ad8929), UINT64_C(0xcbc5cfbc980f44a9),
           UINT64_C(0x964fc56cf77c4302), UINT64_C(0xe34b5661a8564f53),
           UINT64_C(0x8e0d1a53a1339683), UINT64_C(0x0a1867c9968a566f),
           UINT64_C(0xefdc2e8c9c0ab5ee), UINT64_C(0x81e8c30ed04dd9bd)),
    BF512C(UINT64_C(0xd120f0a833cc00bd), UINT64_C(0x5ac01a75799721ad),
           UINT64_C(0x32cd0d9273c2dea9), UINT64_C(0x1d903c4783ef1d7d),
           UINT64_C(0x6b0558c8f22fdb80), UINT64_C(0x9bef7e419bdc0f08),
           UINT64_C(0x71521b7b6d7cba6f), UINT64_C(0x157fdd5e04a54bdf))};

/*
 * vistrutith_beta4_cubes[j] = beta4^(3*2^j), for j=0..3.
 *
 * Together with vistrutith_beta4_squares, these constants implement
 * y_j = x0 + x1*beta4^(2^j) + x2*beta4^(2^(j+1)) + x3*beta4^(3*2^j).
 */
static const bf512_t vistrutith_beta4_cubes[4] = {
    BF512C(UINT64_C(0xba895cdd20ad8929), UINT64_C(0xcbc5cfbc980f44a9),
           UINT64_C(0x964fc56cf77c4302), UINT64_C(0xe34b5661a8564f53),
           UINT64_C(0x8e0d1a53a1339683), UINT64_C(0x0a1867c9968a566f),
           UINT64_C(0xefdc2e8c9c0ab5ee), UINT64_C(0x81e8c30ed04dd9bd)),
    BF512C(UINT64_C(0xd120f0a833cc00bd), UINT64_C(0x5ac01a75799721ad),
           UINT64_C(0x32cd0d9273c2dea9), UINT64_C(0x1d903c4783ef1d7d),
           UINT64_C(0x6b0558c8f22fdb80), UINT64_C(0x9bef7e419bdc0f08),
           UINT64_C(0x71521b7b6d7cba6f), UINT64_C(0x157fdd5e04a54bdf)),
    BF512C(UINT64_C(0xa0959aab79b80474), UINT64_C(0xd9f01766691b1d5e),
           UINT64_C(0x3cea2f055aa00d07), UINT64_C(0x38ea8ab6bfd2447c),
           UINT64_C(0x8b27ea58ed96dea8), UINT64_C(0xe53221c469b7ece3),
           UINT64_C(0x521893321dfa7ee7), UINT64_C(0x3733fefb6a763a91)),
    BF512C(UINT64_C(0xcb3c36de6ad98de1), UINT64_C(0x48f5c2af8883785a),
           UINT64_C(0x9868e7fbde1e90ac), UINT64_C(0xc631e090946b1652),
           UINT64_C(0x6e2fa8c3be8a93ab), UINT64_C(0x74c5384c64e1b584),
           UINT64_C(0xcc96a6c5ec8c7166), UINT64_C(0xa3a4e0abbe9ea8f3))};

void vistrutith_shuffle_key_prover(uint8_t fixed_key_bits[VISTRUTAH_KEY_SIZE_512],
                                   bf512_t fixed_key_tag[VISTRUTAH_KEY_SIZE_512 * 8u]) {
  bf512_t temp[(VISTRUTAH_KEY_SIZE_512 / 2u) * 8u];

  vistrutah_shuffle_key(fixed_key_bits);

  memcpy(temp, fixed_key_tag + (VISTRUTAH_KEY_SIZE_512 / 2u) * 8u, sizeof(temp));
  for (unsigned int i = 0; i < (VISTRUTAH_KEY_SIZE_512 / 2u); ++i) {
    memcpy(fixed_key_tag + (VISTRUTAH_KEY_SIZE_512 / 2u + i) * 8u,
           temp + (unsigned int)VISTRUTAH_KEXP_SHUFFLE[i] * 8u, 8u * sizeof(bf512_t));
  }
}

void vistrutith_shuffle_key_verifier(bf512_t fixed_key_key[VISTRUTAH_KEY_SIZE_512 * 8u]) {
  bf512_t temp[(VISTRUTAH_KEY_SIZE_512 / 2u) * 8u];

  memcpy(temp, fixed_key_key + (VISTRUTAH_KEY_SIZE_512 / 2u) * 8u, sizeof(temp));
  for (unsigned int i = 0; i < (VISTRUTAH_KEY_SIZE_512 / 2u); ++i) {
    memcpy(fixed_key_key + (VISTRUTAH_KEY_SIZE_512 / 2u + i) * 8u,
           temp + (unsigned int)VISTRUTAH_KEXP_SHUFFLE[i] * 8u, 8u * sizeof(bf512_t));
  }
}

void vistrutith_rotate_bytes_prover(uint8_t* fixed_key_bits, bf512_t* fixed_key_tag,
                                    int shift, int len) {
  bf512_t temp[VISTRUTAH_512_BLOCK_SIZE * 8u];

  vistrutah_rotate_bytes(fixed_key_bits, shift, len);

  assert(fixed_key_tag);
  assert(len > 0);
  assert(shift >= 0);
  assert(len <= (int)VISTRUTAH_512_BLOCK_SIZE);

  memcpy(temp, fixed_key_tag, (size_t)len * 8u * sizeof(bf512_t));
  for (int i = 0; i < len; ++i) {
    memcpy(fixed_key_tag + (size_t)i * 8u, temp + (size_t)((i + shift) % len) * 8u,
           8u * sizeof(bf512_t));
  }
}

void vistrutith_rotate_bytes_verifier(bf512_t* fixed_key_key, int shift, int len) {
  bf512_t temp[VISTRUTAH_512_BLOCK_SIZE * 8u];

  assert(fixed_key_key);
  assert(len > 0);
  assert(shift >= 0);
  assert(len <= (int)VISTRUTAH_512_BLOCK_SIZE);

  memcpy(temp, fixed_key_key, (size_t)len * 8u * sizeof(bf512_t));
  for (int i = 0; i < len; ++i) {
    memcpy(fixed_key_key + (size_t)i * 8u, temp + (size_t)((i + shift) % len) * 8u,
           8u * sizeof(bf512_t));
  }
}

void vistrutith_state_to_bytes_prover(bf512_t out_val[16], bf512_t out_tag[16],
                                      const uint8_t state_bits[16],
                                      const bf512_t state_tag[128]) {
  for (unsigned int i = 0; i < 16u; ++i) {
    out_val[i] = bf512_byte_combine_bits(state_bits[i]);
    out_tag[i] = bf512_byte_combine(state_tag + i * 8u);
  }
}

void vistrutith_state_to_bytes_verifier(bf512_t out_key[16],
                                        const bf512_t state_key[128]) {
  for (unsigned int i = 0; i < 16u; ++i) {
    out_key[i] = bf512_byte_combine(state_key + i * 8u);
  }
}

void vistrutith_state_to_conjugates_prover(bf512_t out_val[128], bf512_t out_tag[128],
                                           const uint8_t state_bits[16],
                                           const bf512_t state_tag[128]) {
  assert(out_val);
  assert(out_tag);
  assert(state_bits);
  assert(state_tag);

  for (unsigned int i = 0; i < 16u; ++i) {
    uint8_t x = state_bits[i];
    for (unsigned int j = 0; j != 7u; ++j) {
      out_val[i * 8u + j] = bf512_byte_combine_bits(x);
      x = bits_sq(x);
    }
    out_val[i * 8u + 7u] = bf512_byte_combine_bits(x);
  }

  for (unsigned int i = 0; i < 16u; ++i) {
    bf512_t x[8];
    memcpy(x, state_tag + i * 8u, sizeof(x));
    for (unsigned int j = 0; j != 7u; ++j) {
      out_tag[i * 8u + j] = bf512_byte_combine(x);
      bf512_sq_bit_inplace(x);
    }
    out_tag[i * 8u + 7u] = bf512_byte_combine(x);
  }
}

void vistrutith_state_to_conjugates_verifier(bf512_t out_key[128],
                                             const bf512_t state_key[128]) {
  assert(out_key);
  assert(state_key);

  for (unsigned int i = 0; i < 16u; ++i) {
    bf512_t x[8];
    memcpy(x, state_key + i * 8u, sizeof(x));
    for (unsigned int j = 0; j != 7u; ++j) {
      out_key[i * 8u + j] = bf512_byte_combine(x);
      bf512_sq_bit_inplace(x);
    }
    out_key[i * 8u + 7u] = bf512_byte_combine(x);
  }
}

void vistrutith_invnorm_to_conjugates_prover(bf512_t out_val[4], bf512_t out_tag[4],
                                             uint8_t x_bits, const bf512_t x_tag[4]) {
  for (unsigned int i = 0; i != 4u; ++i) {
    out_val[i] = bf512_add(
        bf512_add(bf512_from_bit(get_bit(x_bits, 0)),
                  bf512_mul_bit(vistrutith_beta4_squares[i], get_bit(x_bits, 1))),
        bf512_add(bf512_mul_bit(vistrutith_beta4_squares[i + 1u], get_bit(x_bits, 2)),
                  bf512_mul_bit(vistrutith_beta4_cubes[i], get_bit(x_bits, 3))));

    out_tag[i] = bf512_add(
        bf512_add(x_tag[0], bf512_mul(vistrutith_beta4_squares[i], x_tag[1])),
        bf512_add(bf512_mul(vistrutith_beta4_squares[i + 1u], x_tag[2]),
                  bf512_mul(vistrutith_beta4_cubes[i], x_tag[3])));
  }
}

void vistrutith_invnorm_to_conjugates_verifier(bf512_t out_key[4],
                                               const bf512_t x_key[4]) {
  for (unsigned int i = 0; i != 4u; ++i) {
    out_key[i] = bf512_add(
        bf512_add(x_key[0], bf512_mul(vistrutith_beta4_squares[i], x_key[1])),
        bf512_add(bf512_mul(vistrutith_beta4_squares[i + 1u], x_key[2]),
                  bf512_mul(vistrutith_beta4_cubes[i], x_key[3])));
  }
}

void vistrutith_sbox_affine_prover(bf512_t* out_val, bf512_t* out_tag,
                                   const bf512_t* in_val, const bf512_t* in_tag,
                                   size_t nbytes, bool sq) {
  const bf512_t* C = sq ? vistrutith_sbox_affine_coeffs_sq : vistrutith_sbox_affine_coeffs;
  const unsigned int t = sq ? 1u : 0u;

  for (size_t i = 0; i < nbytes; ++i) {
    bf512_t acc_val = C[8];
    bf512_t acc_tag = C[8];

    for (unsigned int k = 0; k < 8u; ++k) {
      const size_t idx = i * 8u + ((k + t) & 7u);
      acc_val = bf512_add(acc_val, bf512_mul(C[k], in_val[idx]));
      acc_tag = bf512_add(acc_tag, bf512_mul(C[k], in_tag[idx]));
    }
    out_val[i] = acc_val;
    out_tag[i] = acc_tag;
  }
}

void vistrutith_sbox_affine_verifier(bf512_t* out_key, const bf512_t* in_key,
                                     size_t nbytes, bool sq) {
  const bf512_t* C = sq ? vistrutith_sbox_affine_coeffs_sq : vistrutith_sbox_affine_coeffs;
  const unsigned int t = sq ? 1u : 0u;

  for (size_t i = 0; i < nbytes; ++i) {
    bf512_t acc_key = C[8];

    for (unsigned int k = 0; k < 8u; ++k) {
      const size_t idx = i * 8u + ((k + t) & 7u);
      acc_key = bf512_add(acc_key, bf512_mul(C[k], in_key[idx]));
    }
    out_key[i] = acc_key;
  }
}

void vistrutith_shiftrows_prover(bf512_t* out_val, bf512_t* out_tag,
                                 const bf512_t* in_val, const bf512_t* in_tag,
                                 size_t nbytes) {
  if ((nbytes & 15u) != 0u) {
    return;
  }

  for (size_t base = 0; base < nbytes; base += 16u) {
    bf512_t out_val_blk[16];
    bf512_t out_tag_blk[16];

    for (unsigned int r = 0; r < 4u; ++r) {
      for (unsigned int c = 0; c < 4u; ++c) {
        const unsigned int dst = 4u * c + r;
        const unsigned int src = 4u * ((c + r) & 3u) + r;
        out_val_blk[dst] = in_val[base + src];
        out_tag_blk[dst] = in_tag[base + src];
      }
    }

    memcpy(out_val + base, out_val_blk, sizeof(out_val_blk));
    memcpy(out_tag + base, out_tag_blk, sizeof(out_tag_blk));
  }
}

void vistrutith_shiftrows_verifier(bf512_t* out_key, const bf512_t* in_key, size_t nbytes) {
  if ((nbytes & 15u) != 0u) {
    return;
  }

  for (size_t base = 0; base < nbytes; base += 16u) {
    bf512_t out_key_blk[16];

    for (unsigned int r = 0; r < 4u; ++r) {
      for (unsigned int c = 0; c < 4u; ++c) {
        const unsigned int dst = 4u * c + r;
        const unsigned int src = 4u * ((c + r) & 3u) + r;
        out_key_blk[dst] = in_key[base + src];
      }
    }

    memcpy(out_key + base, out_key_blk, sizeof(out_key_blk));
  }
}

void vistrutith_mixcolumns_prover(bf512_t* out_val, bf512_t* out_tag,
                                  const bf512_t* in_val, const bf512_t* in_tag,
                                  size_t nbytes, bool sq) {
  if ((nbytes & 15u) != 0u) {
    return;
  }

  const bf512_t* coeffs = sq ? vistrutith_mixcolumns_coeffs_sq : vistrutith_mixcolumns_coeffs;
  const bf512_t v1 = coeffs[0];
  const bf512_t v2 = coeffs[1];
  const bf512_t v3 = coeffs[2];

  for (size_t base = 0; base < nbytes; base += 16u) {
    bf512_t out_val_blk[16];
    bf512_t out_tag_blk[16];

    for (unsigned int c = 0; c < 4u; ++c) {
      const unsigned int i0 = 4u * c;
      const unsigned int i1 = i0 + 1u;
      const unsigned int i2 = i0 + 2u;
      const unsigned int i3 = i0 + 3u;

      const bf512_t a0v = in_val[base + i0];
      const bf512_t a1v = in_val[base + i1];
      const bf512_t a2v = in_val[base + i2];
      const bf512_t a3v = in_val[base + i3];
      const bf512_t a0t = in_tag[base + i0];
      const bf512_t a1t = in_tag[base + i1];
      const bf512_t a2t = in_tag[base + i2];
      const bf512_t a3t = in_tag[base + i3];

      out_val_blk[i0] = bf512_add(bf512_add(bf512_mul(a0v, v2), bf512_mul(a1v, v3)),
                                  bf512_add(bf512_mul(a2v, v1), bf512_mul(a3v, v1)));
      out_val_blk[i1] = bf512_add(bf512_add(bf512_mul(a0v, v1), bf512_mul(a1v, v2)),
                                  bf512_add(bf512_mul(a2v, v3), bf512_mul(a3v, v1)));
      out_val_blk[i2] = bf512_add(bf512_add(bf512_mul(a0v, v1), bf512_mul(a1v, v1)),
                                  bf512_add(bf512_mul(a2v, v2), bf512_mul(a3v, v3)));
      out_val_blk[i3] = bf512_add(bf512_add(bf512_mul(a0v, v3), bf512_mul(a1v, v1)),
                                  bf512_add(bf512_mul(a2v, v1), bf512_mul(a3v, v2)));

      out_tag_blk[i0] = bf512_add(bf512_add(bf512_mul(a0t, v2), bf512_mul(a1t, v3)),
                                  bf512_add(bf512_mul(a2t, v1), bf512_mul(a3t, v1)));
      out_tag_blk[i1] = bf512_add(bf512_add(bf512_mul(a0t, v1), bf512_mul(a1t, v2)),
                                  bf512_add(bf512_mul(a2t, v3), bf512_mul(a3t, v1)));
      out_tag_blk[i2] = bf512_add(bf512_add(bf512_mul(a0t, v1), bf512_mul(a1t, v1)),
                                  bf512_add(bf512_mul(a2t, v2), bf512_mul(a3t, v3)));
      out_tag_blk[i3] = bf512_add(bf512_add(bf512_mul(a0t, v3), bf512_mul(a1t, v1)),
                                  bf512_add(bf512_mul(a2t, v1), bf512_mul(a3t, v2)));
    }

    memcpy(out_val + base, out_val_blk, sizeof(out_val_blk));
    memcpy(out_tag + base, out_tag_blk, sizeof(out_tag_blk));
  }
}

void vistrutith_mixcolumns_verifier(bf512_t* out_key, const bf512_t* in_key,
                                    size_t nbytes, bool sq) {
  if ((nbytes & 15u) != 0u) {
    return;
  }

  const bf512_t* coeffs = sq ? vistrutith_mixcolumns_coeffs_sq : vistrutith_mixcolumns_coeffs;
  const bf512_t v1 = coeffs[0];
  const bf512_t v2 = coeffs[1];
  const bf512_t v3 = coeffs[2];

  for (size_t base = 0; base < nbytes; base += 16u) {
    bf512_t out_key_blk[16];

    for (unsigned int c = 0; c < 4u; ++c) {
      const unsigned int i0 = 4u * c;
      const unsigned int i1 = i0 + 1u;
      const unsigned int i2 = i0 + 2u;
      const unsigned int i3 = i0 + 3u;

      const bf512_t a0 = in_key[base + i0];
      const bf512_t a1 = in_key[base + i1];
      const bf512_t a2 = in_key[base + i2];
      const bf512_t a3 = in_key[base + i3];

      out_key_blk[i0] = bf512_add(bf512_add(bf512_mul(a0, v2), bf512_mul(a1, v3)),
                                  bf512_add(bf512_mul(a2, v1), bf512_mul(a3, v1)));
      out_key_blk[i1] = bf512_add(bf512_add(bf512_mul(a0, v1), bf512_mul(a1, v2)),
                                  bf512_add(bf512_mul(a2, v3), bf512_mul(a3, v1)));
      out_key_blk[i2] = bf512_add(bf512_add(bf512_mul(a0, v1), bf512_mul(a1, v1)),
                                  bf512_add(bf512_mul(a2, v2), bf512_mul(a3, v3)));
      out_key_blk[i3] = bf512_add(bf512_add(bf512_mul(a0, v3), bf512_mul(a1, v1)),
                                  bf512_add(bf512_mul(a2, v1), bf512_mul(a3, v2)));
    }

    memcpy(out_key + base, out_key_blk, sizeof(out_key_blk));
  }
}

static void vistrutith_gamma_all_u8(uint8_t x, uint8_t* g09, uint8_t* g0b, uint8_t* g0d,
                                    uint8_t* g0e) {
  const uint8_t x0 = get_bit(x, 0);
  const uint8_t x1 = get_bit(x, 1);
  const uint8_t x2 = get_bit(x, 2);
  const uint8_t x3 = get_bit(x, 3);
  const uint8_t x4 = get_bit(x, 4);
  const uint8_t x5 = get_bit(x, 5);
  const uint8_t x6 = get_bit(x, 6);
  const uint8_t x7 = get_bit(x, 7);

  *g09 = (uint8_t)(set_bit(x0 ^ x5, 0) ^ set_bit(x1 ^ x5 ^ x6, 1) ^ set_bit(x2 ^ x6 ^ x7, 2) ^
                   set_bit(x0 ^ x3 ^ x5 ^ x7, 3) ^ set_bit(x1 ^ x4 ^ x5 ^ x6, 4) ^
                   set_bit(x2 ^ x5 ^ x6 ^ x7, 5) ^ set_bit(x3 ^ x6 ^ x7, 6) ^
                   set_bit(x4 ^ x7, 7));
  *g0b = (uint8_t)(set_bit(x0 ^ x5 ^ x7, 0) ^ set_bit(x0 ^ x1 ^ x5 ^ x6 ^ x7, 1) ^
                   set_bit(x1 ^ x2 ^ x6 ^ x7, 2) ^ set_bit(x0 ^ x2 ^ x3 ^ x5, 3) ^
                   set_bit(x1 ^ x3 ^ x4 ^ x5 ^ x6 ^ x7, 4) ^
                   set_bit(x2 ^ x4 ^ x5 ^ x6 ^ x7, 5) ^ set_bit(x3 ^ x5 ^ x6 ^ x7, 6) ^
                   set_bit(x4 ^ x6 ^ x7, 7));
  *g0d = (uint8_t)(set_bit(x0 ^ x5 ^ x6, 0) ^ set_bit(x1 ^ x5 ^ x7, 1) ^
                   set_bit(x0 ^ x2 ^ x6, 2) ^ set_bit(x0 ^ x1 ^ x3 ^ x5 ^ x6 ^ x7, 3) ^
                   set_bit(x1 ^ x2 ^ x4 ^ x5 ^ x7, 4) ^ set_bit(x2 ^ x3 ^ x5 ^ x6, 5) ^
                   set_bit(x3 ^ x4 ^ x6 ^ x7, 6) ^ set_bit(x4 ^ x5 ^ x7, 7));
  *g0e = (uint8_t)(set_bit(x5 ^ x6 ^ x7, 0) ^ set_bit(x0 ^ x5, 1) ^ set_bit(x0 ^ x1 ^ x6, 2) ^
                   set_bit(x0 ^ x1 ^ x2 ^ x5 ^ x6, 3) ^ set_bit(x1 ^ x2 ^ x3 ^ x5, 4) ^
                   set_bit(x2 ^ x3 ^ x4 ^ x6, 5) ^ set_bit(x3 ^ x4 ^ x5 ^ x7, 6) ^
                   set_bit(x4 ^ x5 ^ x6, 7));
}

static void vistrutith_gamma_all_bits(const bf512_t x_bits[8], bf512_t g09_bits[8],
                                      bf512_t g0b_bits[8], bf512_t g0d_bits[8],
                                      bf512_t g0e_bits[8]) {
  g09_bits[0] = bf512_add(x_bits[0], x_bits[5]);
  g09_bits[1] = bf512_add(bf512_add(x_bits[1], x_bits[5]), x_bits[6]);
  g09_bits[2] = bf512_add(bf512_add(x_bits[2], x_bits[6]), x_bits[7]);
  g09_bits[3] = bf512_add(bf512_add(x_bits[0], x_bits[3]), bf512_add(x_bits[5], x_bits[7]));
  g09_bits[4] = bf512_add(bf512_add(x_bits[1], x_bits[4]), bf512_add(x_bits[5], x_bits[6]));
  g09_bits[5] = bf512_add(bf512_add(x_bits[2], x_bits[5]), bf512_add(x_bits[6], x_bits[7]));
  g09_bits[6] = bf512_add(bf512_add(x_bits[3], x_bits[6]), x_bits[7]);
  g09_bits[7] = bf512_add(x_bits[4], x_bits[7]);

  g0b_bits[0] = bf512_add(bf512_add(x_bits[0], x_bits[5]), x_bits[7]);
  g0b_bits[1] = bf512_add(bf512_add(x_bits[0], x_bits[1]),
                          bf512_add(bf512_add(x_bits[5], x_bits[6]), x_bits[7]));
  g0b_bits[2] = bf512_add(bf512_add(x_bits[1], x_bits[2]), bf512_add(x_bits[6], x_bits[7]));
  g0b_bits[3] = bf512_add(bf512_add(x_bits[0], x_bits[2]), bf512_add(x_bits[3], x_bits[5]));
  g0b_bits[4] = bf512_add(
      bf512_add(x_bits[1], x_bits[3]),
      bf512_add(bf512_add(x_bits[4], x_bits[5]), bf512_add(x_bits[6], x_bits[7])));
  g0b_bits[5] = bf512_add(bf512_add(x_bits[2], x_bits[4]),
                          bf512_add(bf512_add(x_bits[5], x_bits[6]), x_bits[7]));
  g0b_bits[6] = bf512_add(bf512_add(x_bits[3], x_bits[5]), bf512_add(x_bits[6], x_bits[7]));
  g0b_bits[7] = bf512_add(bf512_add(x_bits[4], x_bits[6]), x_bits[7]);

  g0d_bits[0] = bf512_add(bf512_add(x_bits[0], x_bits[5]), x_bits[6]);
  g0d_bits[1] = bf512_add(bf512_add(x_bits[1], x_bits[5]), x_bits[7]);
  g0d_bits[2] = bf512_add(bf512_add(x_bits[0], x_bits[2]), x_bits[6]);
  g0d_bits[3] = bf512_add(bf512_add(x_bits[0], x_bits[1]),
                          bf512_add(bf512_add(x_bits[3], x_bits[5]),
                                    bf512_add(x_bits[6], x_bits[7])));
  g0d_bits[4] = bf512_add(bf512_add(x_bits[1], x_bits[2]),
                          bf512_add(bf512_add(x_bits[4], x_bits[5]), x_bits[7]));
  g0d_bits[5] = bf512_add(bf512_add(x_bits[2], x_bits[3]), bf512_add(x_bits[5], x_bits[6]));
  g0d_bits[6] = bf512_add(bf512_add(x_bits[3], x_bits[4]), bf512_add(x_bits[6], x_bits[7]));
  g0d_bits[7] = bf512_add(bf512_add(x_bits[4], x_bits[5]), x_bits[7]);

  g0e_bits[0] = bf512_add(bf512_add(x_bits[5], x_bits[6]), x_bits[7]);
  g0e_bits[1] = bf512_add(x_bits[0], x_bits[5]);
  g0e_bits[2] = bf512_add(bf512_add(x_bits[0], x_bits[1]), x_bits[6]);
  g0e_bits[3] = bf512_add(bf512_add(x_bits[0], x_bits[1]),
                          bf512_add(x_bits[2], bf512_add(x_bits[5], x_bits[6])));
  g0e_bits[4] = bf512_add(bf512_add(x_bits[1], x_bits[2]), bf512_add(x_bits[3], x_bits[5]));
  g0e_bits[5] = bf512_add(bf512_add(x_bits[2], x_bits[3]), bf512_add(x_bits[4], x_bits[6]));
  g0e_bits[6] = bf512_add(bf512_add(x_bits[3], x_bits[4]), bf512_add(x_bits[5], x_bits[7]));
  g0e_bits[7] = bf512_add(bf512_add(x_bits[4], x_bits[5]), x_bits[6]);
}

void vistrutith_bit_inv_mixcolumns_prover(uint8_t out_bits[16], bf512_t out_tag[128],
                                          const uint8_t in_bits[16],
                                          const bf512_t in_tag[128]) {
  for (unsigned int c = 0; c < 4u; ++c) {
    const uint8_t* a = in_bits + 4u * c;
    const bf512_t* a_tag = in_tag + 32u * c;
    uint8_t g09[4], g0b[4], g0d[4], g0e[4];
    bf512_t g09_tag[32], g0b_tag[32], g0d_tag[32], g0e_tag[32];

    for (unsigned int r = 0; r < 4u; ++r) {
      vistrutith_gamma_all_u8(a[r], &g09[r], &g0b[r], &g0d[r], &g0e[r]);
      vistrutith_gamma_all_bits(a_tag + r * 8u, g09_tag + r * 8u, g0b_tag + r * 8u,
                                g0d_tag + r * 8u, g0e_tag + r * 8u);
    }

    out_bits[4u * c + 0u] = (uint8_t)(g0e[0] ^ g0b[1] ^ g0d[2] ^ g09[3]);
    out_bits[4u * c + 1u] = (uint8_t)(g09[0] ^ g0e[1] ^ g0b[2] ^ g0d[3]);
    out_bits[4u * c + 2u] = (uint8_t)(g0d[0] ^ g09[1] ^ g0e[2] ^ g0b[3]);
    out_bits[4u * c + 3u] = (uint8_t)(g0b[0] ^ g0d[1] ^ g09[2] ^ g0e[3]);

    for (unsigned int bit = 0; bit < 8u; ++bit) {
      out_tag[8u * (4u * c + 0u) + bit] =
          bf512_add(bf512_add(g0e_tag[0u * 8u + bit], g0b_tag[1u * 8u + bit]),
                    bf512_add(g0d_tag[2u * 8u + bit], g09_tag[3u * 8u + bit]));
      out_tag[8u * (4u * c + 1u) + bit] =
          bf512_add(bf512_add(g09_tag[0u * 8u + bit], g0e_tag[1u * 8u + bit]),
                    bf512_add(g0b_tag[2u * 8u + bit], g0d_tag[3u * 8u + bit]));
      out_tag[8u * (4u * c + 2u) + bit] =
          bf512_add(bf512_add(g0d_tag[0u * 8u + bit], g09_tag[1u * 8u + bit]),
                    bf512_add(g0e_tag[2u * 8u + bit], g0b_tag[3u * 8u + bit]));
      out_tag[8u * (4u * c + 3u) + bit] =
          bf512_add(bf512_add(g0b_tag[0u * 8u + bit], g0d_tag[1u * 8u + bit]),
                    bf512_add(g09_tag[2u * 8u + bit], g0e_tag[3u * 8u + bit]));
    }
  }
}

void vistrutith_bit_inv_mixcolumns_verifier(bf512_t out_key[128], const bf512_t in_key[128]) {
  for (unsigned int c = 0; c < 4u; ++c) {
    const bf512_t* a_key = in_key + 32u * c;
    bf512_t g09_key[32], g0b_key[32], g0d_key[32], g0e_key[32];

    for (unsigned int r = 0; r < 4u; ++r) {
      vistrutith_gamma_all_bits(a_key + r * 8u, g09_key + r * 8u, g0b_key + r * 8u,
                                g0d_key + r * 8u, g0e_key + r * 8u);
    }

    for (unsigned int bit = 0; bit < 8u; ++bit) {
      out_key[8u * (4u * c + 0u) + bit] =
          bf512_add(bf512_add(g0e_key[0u * 8u + bit], g0b_key[1u * 8u + bit]),
                    bf512_add(g0d_key[2u * 8u + bit], g09_key[3u * 8u + bit]));
      out_key[8u * (4u * c + 1u) + bit] =
          bf512_add(bf512_add(g09_key[0u * 8u + bit], g0e_key[1u * 8u + bit]),
                    bf512_add(g0b_key[2u * 8u + bit], g0d_key[3u * 8u + bit]));
      out_key[8u * (4u * c + 2u) + bit] =
          bf512_add(bf512_add(g0d_key[0u * 8u + bit], g09_key[1u * 8u + bit]),
                    bf512_add(g0e_key[2u * 8u + bit], g0b_key[3u * 8u + bit]));
      out_key[8u * (4u * c + 3u) + bit] =
          bf512_add(bf512_add(g0b_key[0u * 8u + bit], g0d_key[1u * 8u + bit]),
                    bf512_add(g09_key[2u * 8u + bit], g0e_key[3u * 8u + bit]));
    }
  }
}

void vistrutith_bit_inv_shiftrows_prover(uint8_t out_bits[16], bf512_t out_tag[128],
                                         const uint8_t in_bits[16],
                                         const bf512_t in_tag[128]) {
  for (unsigned int r = 0; r < 4u; ++r) {
    for (unsigned int c = 0; c < 4u; ++c) {
      const unsigned int dst = 4u * c + r;
      const unsigned int src = 4u * ((c + 4u - r) & 3u) + r;
      out_bits[dst] = in_bits[src];
      memcpy(out_tag + dst * 8u, in_tag + src * 8u, 8u * sizeof(*out_tag));
    }
  }
}

void vistrutith_bit_inv_shiftrows_verifier(bf512_t out_key[128], const bf512_t in_key[128]) {
  for (unsigned int r = 0; r < 4u; ++r) {
    for (unsigned int c = 0; c < 4u; ++c) {
      const unsigned int dst = 4u * c + r;
      const unsigned int src = 4u * ((c + 4u - r) & 3u) + r;
      memcpy(out_key + dst * 8u, in_key + src * 8u, 8u * sizeof(*out_key));
    }
  }
}

void vistrutith_bit_inv_sbox_affine_prover(uint8_t out_bits[16], bf512_t out_tag[128],
                                           const uint8_t in_bits[16],
                                           const bf512_t in_tag[128]) {
  const uint8_t c = UINT8_C(0x05); /* c_j = 1 for j in {0,2} */

  for (unsigned int i = 0; i < 16u; ++i) {
    uint8_t y = 0;

    for (unsigned int j = 0; j < 8u; ++j) {
      const unsigned int j1 = (j + 7u) & 7u;
      const unsigned int j3 = (j + 5u) & 7u;
      const unsigned int j6 = (j + 2u) & 7u;
      const uint8_t y_bit = (uint8_t)(get_bit(in_bits[i], j1) ^ get_bit(in_bits[i], j3) ^
                                       get_bit(in_bits[i], j6) ^ get_bit(c, j));
      const bf512_t t =
          bf512_add(bf512_add(in_tag[i * 8u + j1], in_tag[i * 8u + j3]),
                    bf512_add(in_tag[i * 8u + j6], bf512_from_bit(get_bit(c, j))));
      y |= (uint8_t)set_bit(y_bit, j);
      out_tag[i * 8u + j] = t;
    }
    out_bits[i] = y;
  }
}

void vistrutith_bit_inv_sbox_affine_verifier(bf512_t out_key[128], const bf512_t in_key[128]) {
  const uint8_t c = UINT8_C(0x05); /* c_j = 1 for j in {0,2} */

  for (unsigned int i = 0; i < 16u; ++i) {
    for (unsigned int j = 0; j < 8u; ++j) {
      const unsigned int j1 = (j + 7u) & 7u;
      const unsigned int j3 = (j + 5u) & 7u;
      const unsigned int j6 = (j + 2u) & 7u;
      out_key[i * 8u + j] =
          bf512_add(bf512_add(in_key[i * 8u + j1], in_key[i * 8u + j3]),
                    bf512_add(in_key[i * 8u + j6], bf512_from_bit(get_bit(c, j))));
    }
  }
}

void vistrutith_inv_mix_prover(
    uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t in_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]) {
  static const uint8_t pi_mix[16] = {0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15};

  for (unsigned int j = 0; j < 16u; ++j) {
    const unsigned int u = pi_mix[j];
    for (unsigned int b = 0; b < 4u; ++b) {
      const unsigned int src = 4u * j + b;
      const unsigned int dst = 16u * b + u;
      out_bits[dst] = in_bits[src];
      memcpy(out_tag + dst * 8u, in_tag + src * 8u, 8u * sizeof(*out_tag));
    }
  }
}

void vistrutith_inv_mix_verifier(
    bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t in_key[VISTRUTAH_512_BLOCK_SIZE * 8u]) {
  static const uint8_t pi_mix[16] = {0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15};

  for (unsigned int j = 0; j < 16u; ++j) {
    const unsigned int u = pi_mix[j];
    for (unsigned int b = 0; b < 4u; ++b) {
      const unsigned int src = 4u * j + b;
      const unsigned int dst = 16u * b + u;
      memcpy(out_key + dst * 8u, in_key + src * 8u, 8u * sizeof(*out_key));
    }
  }
}

static uint8_t vistrutith_unpack_norm_nibble(
    const uint8_t norm_packed[VISTRUTAH_512_BLOCK_SIZE / 2u], unsigned int byte_idx) {
  const uint8_t v = norm_packed[byte_idx >> 1u];
  if ((byte_idx & 1u) != 0u) {
    return (uint8_t)((v >> 4) & 0x0Fu);
  }
  return (uint8_t)(v & 0x0Fu);
}

void vistrutith_forward_by_norm_prover(
    bf512_t out_a_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_a_tag[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t out_a_sq_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_a_sq_tag[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t z_norm_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t z_norm_tag[VISTRUTAH_512_BLOCK_SIZE],
    const uint8_t in_state_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_state_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t norm_packed[VISTRUTAH_512_BLOCK_SIZE / 2u],
    const bf512_t norm_tag[VISTRUTAH_512_BLOCK_SIZE * 4u],
    const uint8_t fixed_key_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t fixed_key_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]) {

  for (unsigned int b = 0; b < 4u; ++b) {
    const unsigned int byte_base = 16u * b;
    const unsigned int bit_base = byte_base * 8u;

    bf512_t c_val[128];
    bf512_t c_tag[128];
    bf512_t p_val[128];
    bf512_t p_tag[128];

    bf512_t a_aff_val[16];
    bf512_t a_aff_tag[16];
    bf512_t a_aff_sq_val[16];
    bf512_t a_aff_sq_tag[16];
    bf512_t a_sr_val[16];
    bf512_t a_sr_tag[16];
    bf512_t a_sr_sq_val[16];
    bf512_t a_sr_sq_tag[16];
    bf512_t a_mc_val[16];
    bf512_t a_mc_tag[16];
    bf512_t a_mc_sq_val[16];
    bf512_t a_mc_sq_tag[16];

    bf512_t fk0_val[16];
    bf512_t fk0_tag[16];
    bf512_t fk1_val[16];
    bf512_t fk1_tag[16];

    // :4
    vistrutith_state_to_conjugates_prover(c_val, c_tag, in_state_bits + byte_base,
                                          in_state_tag + bit_base);

    // :5
    for (unsigned int u = 0; u < 16u; ++u) {
      // :6
      const unsigned int t = byte_base + u;

      // :7
      const uint8_t nibble = vistrutith_unpack_norm_nibble(norm_packed, t);
      const bf512_t* nibble_tag = norm_tag + 4u * t;
      bf512_t h_val[4];
      bf512_t h_tag[4];
      vistrutith_invnorm_to_conjugates_prover(h_val, h_tag, nibble, nibble_tag);

      // :8-9
      z_norm_val[t] =
          bf512_add(bf512_mul(bf512_mul(h_val[0], c_val[u * 8u + 4u]), c_val[u * 8u + 1u]),
                    c_val[u * 8u + 0u]);
      z_norm_tag[t] =
          bf512_add(bf512_mul(bf512_mul(h_tag[0], c_tag[u * 8u + 4u]), c_tag[u * 8u + 1u]),
                    c_tag[u * 8u + 0u]);

       // :10-12
      for (unsigned int j = 0; j < 8u; ++j) {
        p_val[u * 8u + j] = bf512_mul(c_val[u * 8u + ((j + 4u) & 7u)], h_val[j & 3u]);
        p_tag[u * 8u + j] = bf512_mul(c_tag[u * 8u + ((j + 4u) & 7u)], h_tag[j & 3u]);
      }
    }

    // :15
    vistrutith_sbox_affine_prover(a_aff_val, a_aff_tag, p_val, p_tag, 16u, false);
    vistrutith_sbox_affine_prover(a_aff_sq_val, a_aff_sq_tag, p_val, p_tag, 16u, true);

    // :16
    vistrutith_shiftrows_prover(a_sr_val, a_sr_tag, a_aff_val, a_aff_tag, 16u);
    vistrutith_shiftrows_prover(a_sr_sq_val, a_sr_sq_tag, a_aff_sq_val, a_aff_sq_tag, 16u);

    // :17
    vistrutith_mixcolumns_prover(a_mc_val, a_mc_tag, a_sr_val, a_sr_tag, 16u, false);
    vistrutith_mixcolumns_prover(a_mc_sq_val, a_mc_sq_tag, a_sr_sq_val, a_sr_sq_tag, 16u, true);

    // :19
    vistrutith_state_to_bytes_prover(fk0_val, fk0_tag, fixed_key_bits + byte_base,
                                     fixed_key_tag + bit_base);
    // :20
    // Vistrutith.StateToBytes(SquareBits(FK1))                       
    for (unsigned int u = 0; u < 16u; ++u) {
      bf512_t key_sq_bits_tag[8];
      fk1_val[u] = bf512_byte_combine_bits_sq(fixed_key_bits[byte_base + u]);
      bf512_sq_bit(key_sq_bits_tag, fixed_key_tag + bit_base + u * 8u);
      fk1_tag[u] = bf512_byte_combine(key_sq_bits_tag);
    }

    // :21-22
    for (unsigned int u = 0; u < 16u; ++u) {
      const unsigned int t = byte_base + u;
      out_a_val[t] = bf512_add(a_mc_val[u], fk0_val[u]);
      out_a_tag[t] = bf512_add(a_mc_tag[u], fk0_tag[u]);
      out_a_sq_val[t] = bf512_add(a_mc_sq_val[u], fk1_val[u]);
      out_a_sq_tag[t] = bf512_add(a_mc_sq_tag[u], fk1_tag[u]);
    }
  }
}

void vistrutith_forward_by_norm_verifier(
    bf512_t out_a_key[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_a_sq_key[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t z_norm_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_state_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t norm_key[VISTRUTAH_512_BLOCK_SIZE * 4u],
    const bf512_t fixed_key_key[VISTRUTAH_512_BLOCK_SIZE * 8u]) {
  for (unsigned int b = 0; b < 4u; ++b) {
    const unsigned int byte_base = 16u * b;
    const unsigned int bit_base = byte_base * 8u;

    bf512_t c_key[128];
    bf512_t p_key[128];

    bf512_t a_aff_key[16];
    bf512_t a_aff_sq_key[16];
    bf512_t a_sr_key[16];
    bf512_t a_sr_sq_key[16];
    bf512_t a_mc_key[16];
    bf512_t a_mc_sq_key[16];

    bf512_t fk0_key[16];
    bf512_t fk1_key[16];

    vistrutith_state_to_conjugates_verifier(c_key, in_state_key + bit_base);

    for (unsigned int u = 0; u < 16u; ++u) {
      const unsigned int t = byte_base + u;
      const bf512_t* nibble_key = norm_key + 4u * t;
      bf512_t h_key[4];

      vistrutith_invnorm_to_conjugates_verifier(h_key, nibble_key);

      z_norm_key[t] =
          bf512_add(bf512_mul(bf512_mul(h_key[0], c_key[u * 8u + 4u]), c_key[u * 8u + 1u]),
                    c_key[u * 8u + 0u]);

      for (unsigned int j = 0; j < 8u; ++j) {
        p_key[u * 8u + j] = bf512_mul(c_key[u * 8u + ((j + 4u) & 7u)], h_key[j & 3u]);
      }
    }

    vistrutith_sbox_affine_verifier(a_aff_key, p_key, 16u, false);
    vistrutith_sbox_affine_verifier(a_aff_sq_key, p_key, 16u, true);

    vistrutith_shiftrows_verifier(a_sr_key, a_aff_key, 16u);
    vistrutith_shiftrows_verifier(a_sr_sq_key, a_aff_sq_key, 16u);

    vistrutith_mixcolumns_verifier(a_mc_key, a_sr_key, 16u, false);
    vistrutith_mixcolumns_verifier(a_mc_sq_key, a_sr_sq_key, 16u, true);

    vistrutith_state_to_bytes_verifier(fk0_key, fixed_key_key + bit_base);
    for (unsigned int u = 0; u < 16u; ++u) {
      bf512_t key_sq_bits[8];
      bf512_sq_bit(key_sq_bits, fixed_key_key + bit_base + u * 8u);
      fk1_key[u] = bf512_byte_combine(key_sq_bits);
    }

    for (unsigned int u = 0; u < 16u; ++u) {
      const unsigned int t = byte_base + u;
      out_a_key[t] = bf512_add(a_mc_key[u], fk0_key[u]);
      out_a_sq_key[t] = bf512_add(a_mc_sq_key[u], fk1_key[u]);
    }
  }
}

void vistrutith_backward_zerornd_by_bits_prover(
    uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t m_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t m_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t rk_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t rk_tag[VISTRUTAH_512_BLOCK_SIZE * 8u], const uint8_t rc_bits[16]) {
  uint8_t u_bits[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t u_tag[VISTRUTAH_512_BLOCK_SIZE * 8u];
  uint8_t v_bits[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t v_tag[VISTRUTAH_512_BLOCK_SIZE * 8u];

  for (unsigned int i = 0; i < VISTRUTAH_512_BLOCK_SIZE; ++i) {
    u_bits[i] = (uint8_t)(m_bits[i] ^ rk_bits[i]);
  }
  for (unsigned int i = 0; i < VISTRUTAH_512_BLOCK_SIZE * 8u; ++i) {
    u_tag[i] = bf512_add(m_tag[i], rk_tag[i]);
  }

  /* Branch 0 additionally xors RC_{i-1}. */
  for (unsigned int i = 0; i < 16u; ++i) {
    u_bits[i] ^= rc_bits[i];
  }
  for (unsigned int bit_i = 0; bit_i < 16u * 8u; ++bit_i) {
    u_tag[bit_i] = bf512_add(u_tag[bit_i], bf512_from_bit(ptr_get_bit(rc_bits, bit_i)));
  }

  vistrutith_inv_mix_prover(v_bits, v_tag, u_bits, u_tag);

  for (unsigned int b = 0; b < 4u; ++b) {
    const unsigned int byte_base = 16u * b;
    const unsigned int bit_base = byte_base * 8u;
    uint8_t m1_bits[16];
    bf512_t m1_tag[128];
    uint8_t m2_bits[16];
    bf512_t m2_tag[128];
    uint8_t y_bits[16];
    bf512_t y_tag[128];

    vistrutith_bit_inv_mixcolumns_prover(m1_bits, m1_tag, v_bits + byte_base, v_tag + bit_base);
    vistrutith_bit_inv_shiftrows_prover(m2_bits, m2_tag, m1_bits, m1_tag);
    vistrutith_bit_inv_sbox_affine_prover(y_bits, y_tag, m2_bits, m2_tag);

    memcpy(out_bits + byte_base, y_bits, sizeof(y_bits));
    memcpy(out_tag + bit_base, y_tag, sizeof(y_tag));
  }
}

void vistrutith_backward_zerornd_by_bits_verifier(
    bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t m_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t rk_key[VISTRUTAH_512_BLOCK_SIZE * 8u], const uint8_t rc_bits[16]) {
  bf512_t u_key[VISTRUTAH_512_BLOCK_SIZE * 8u];
  bf512_t v_key[VISTRUTAH_512_BLOCK_SIZE * 8u];

  for (unsigned int i = 0; i < VISTRUTAH_512_BLOCK_SIZE * 8u; ++i) {
    u_key[i] = bf512_add(m_key[i], rk_key[i]);
  }

  /* Branch 0 additionally xors RC_{i-1}. */
  for (unsigned int bit_i = 0; bit_i < 16u * 8u; ++bit_i) {
    u_key[bit_i] = bf512_add(u_key[bit_i], bf512_from_bit(ptr_get_bit(rc_bits, bit_i)));
  }

  vistrutith_inv_mix_verifier(v_key, u_key);

  for (unsigned int b = 0; b < 4u; ++b) {
    const unsigned int bit_base = 16u * 8u * b;
    bf512_t m1_key[128];
    bf512_t m2_key[128];
    bf512_t y_key[128];

    vistrutith_bit_inv_mixcolumns_verifier(m1_key, v_key + bit_base);
    vistrutith_bit_inv_shiftrows_verifier(m2_key, m1_key);
    vistrutith_bit_inv_sbox_affine_verifier(y_key, m2_key);

    memcpy(out_key + bit_base, y_key, sizeof(y_key));
  }
}

void vistrutith_backward_finalrnd_by_bits_prover(
    uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE], bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t ct_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t ct_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t rk_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t rk_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]) {
  for (unsigned int b = 0; b < 4u; ++b) {
    const unsigned int byte_base = 16u * b;
    const unsigned int bit_base = byte_base * 8u;
    uint8_t u_bits[16];
    bf512_t u_tag[128];
    uint8_t u_sr_bits[16];
    bf512_t u_sr_tag[128];
    uint8_t y_bits[16];
    bf512_t y_tag[128];

    for (unsigned int i = 0; i < 16u; ++i) {
      u_bits[i] = (uint8_t)(ct_bits[byte_base + i] ^ rk_bits[byte_base + i]);
    }
    for (unsigned int i = 0; i < 16u * 8u; ++i) {
      u_tag[i] = bf512_add(ct_tag[bit_base + i], rk_tag[bit_base + i]);
    }

    vistrutith_bit_inv_shiftrows_prover(u_sr_bits, u_sr_tag, u_bits, u_tag);
    vistrutith_bit_inv_sbox_affine_prover(y_bits, y_tag, u_sr_bits, u_sr_tag);

    memcpy(out_bits + byte_base, y_bits, sizeof(y_bits));
    memcpy(out_tag + bit_base, y_tag, sizeof(y_tag));
  }
}

void vistrutith_backward_finalrnd_by_bits_verifier(
    bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t ct_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t rk_key[VISTRUTAH_512_BLOCK_SIZE * 8u]) {
  for (unsigned int b = 0; b < 4u; ++b) {
    const unsigned int bit_base = 16u * 8u * b;
    bf512_t u_key[128];
    bf512_t u_sr_key[128];
    bf512_t y_key[128];

    for (unsigned int i = 0; i < 16u * 8u; ++i) {
      u_key[i] = bf512_add(ct_key[bit_base + i], rk_key[bit_base + i]);
    }

    vistrutith_bit_inv_shiftrows_verifier(u_sr_key, u_key);
    vistrutith_bit_inv_sbox_affine_verifier(y_key, u_sr_key);
    memcpy(out_key + bit_base, y_key, sizeof(y_key));
  }
}

void vistrutith_append_cstrnts_by_bits_prover(
    bf512_t z0_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t z0_tag[VISTRUTAH_512_BLOCK_SIZE],
    bf512_t z1_val[VISTRUTAH_512_BLOCK_SIZE], bf512_t z1_tag[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_val[VISTRUTAH_512_BLOCK_SIZE], const bf512_t x_tag[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_sq_val[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_sq_tag[VISTRUTAH_512_BLOCK_SIZE],
    const uint8_t y_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t y_tag[VISTRUTAH_512_BLOCK_SIZE * 8u]) {
  for (unsigned int t = 0; t < VISTRUTAH_512_BLOCK_SIZE; ++t) {
    const bf512_t y_hat_val = bf512_byte_combine_bits(y_bits[t]);
    const bf512_t y_hat_tag = bf512_byte_combine(y_tag + t * 8u);
    const bf512_t y_hat_sq_val = bf512_byte_combine_bits_sq(y_bits[t]);
    const bf512_t y_hat_sq_tag = bf512_byte_combine_sq(y_tag + t * 8u);

    z0_val[t] = bf512_add(bf512_mul(x_sq_val[t], y_hat_val), x_val[t]);
    z0_tag[t] = bf512_add(bf512_mul(x_sq_tag[t], y_hat_tag), x_tag[t]);

    z1_val[t] = bf512_add(bf512_mul(x_val[t], y_hat_sq_val), y_hat_val);
    z1_tag[t] = bf512_add(bf512_mul(x_tag[t], y_hat_sq_tag), y_hat_tag);
  }
}

void vistrutith_append_cstrnts_by_bits_verifier(
    bf512_t z0_key[VISTRUTAH_512_BLOCK_SIZE], bf512_t z1_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t x_sq_key[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t y_key[VISTRUTAH_512_BLOCK_SIZE * 8u]) {
  for (unsigned int t = 0; t < VISTRUTAH_512_BLOCK_SIZE; ++t) {
    const bf512_t y_hat_key = bf512_byte_combine(y_key + t * 8u);
    const bf512_t y_hat_sq_key = bf512_byte_combine_sq(y_key + t * 8u);

    z0_key[t] = bf512_add(bf512_mul(x_sq_key[t], y_hat_key), x_key[t]);
    z1_key[t] = bf512_add(bf512_mul(x_key[t], y_hat_sq_key), y_hat_key);
  }
}

void vistrutith_enc_constraints_prover(
    bf512_t z_norm_val[VISTRUTITH_ENC_CSTRNTS_NORM_LEN],
    bf512_t z_norm_tag[VISTRUTITH_ENC_CSTRNTS_NORM_LEN],
    bf512_t z_io0_val[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io0_tag[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io1_val[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io1_tag[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    const uint8_t in_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t in_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t out_bits[VISTRUTAH_512_BLOCK_SIZE],
    const bf512_t out_tag[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const uint8_t w[VISTRUTAH_WITNESS_TOTAL_BYTES],
    const bf512_t w_tag[VISTRUTAH_WITNESS_TOTAL_BYTES * 8u]) {

  const unsigned int steps = VISTRUTITH_ENC_CSTRNTS_STEPS;
  unsigned int norm_off = 0u;
  unsigned int io_off = 0u;

  uint8_t fixed_key_bits[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t fixed_key_tag[VISTRUTAH_512_BLOCK_SIZE * 8u];
  uint8_t round_keys_bits[(VISTRUTITH_ENC_CSTRNTS_STEPS + 1u) * VISTRUTAH_512_BLOCK_SIZE];
  bf512_t round_keys_tag[(VISTRUTITH_ENC_CSTRNTS_STEPS + 1u) * VISTRUTAH_512_BLOCK_SIZE * 8u];

  uint8_t p_bits[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t p_tag[VISTRUTAH_512_BLOCK_SIZE * 8u];
  bf512_t a_val[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t a_tag[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t a_sq_val[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t a_sq_tag[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t z_norm_round_val[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t z_norm_round_tag[VISTRUTAH_512_BLOCK_SIZE];

  memcpy(fixed_key_bits, w, VISTRUTAH_WITNESS_KEY_BYTES);
  memcpy(fixed_key_tag, w_tag, VISTRUTAH_WITNESS_KEY_BYTES * 8u * sizeof(*w_tag));

  // :8
  vistrutith_shuffle_key_prover(fixed_key_bits, fixed_key_tag);

  {
    uint8_t* rk0_bits = round_keys_bits;
    bf512_t* rk0_tag = round_keys_tag;
    memcpy(rk0_bits + 0u, fixed_key_bits + 16u, 16u);
    memcpy(rk0_bits + 16u, fixed_key_bits + 0u, 16u);
    memcpy(rk0_bits + 32u, fixed_key_bits + 48u, 16u);
    memcpy(rk0_bits + 48u, fixed_key_bits + 32u, 16u);
    memcpy(rk0_tag + 0u * 128u, fixed_key_tag + 1u * 128u, 128u * sizeof(*rk0_tag));
    memcpy(rk0_tag + 1u * 128u, fixed_key_tag + 0u * 128u, 128u * sizeof(*rk0_tag));
    memcpy(rk0_tag + 2u * 128u, fixed_key_tag + 3u * 128u, 128u * sizeof(*rk0_tag));
    memcpy(rk0_tag + 3u * 128u, fixed_key_tag + 2u * 128u, 128u * sizeof(*rk0_tag));
  }

  // :9-14
  for (unsigned int j = 1u; j <= steps; ++j) {
    uint8_t* rk_bits = round_keys_bits + j * VISTRUTAH_512_BLOCK_SIZE;
    bf512_t* rk_tag = round_keys_tag + j * VISTRUTAH_512_BLOCK_SIZE * 8u;
    const uint8_t* prev_bits = round_keys_bits + (j - 1u) * VISTRUTAH_512_BLOCK_SIZE;
    const bf512_t* prev_tag = round_keys_tag + (j - 1u) * VISTRUTAH_512_BLOCK_SIZE * 8u;

    memcpy(rk_bits, prev_bits, VISTRUTAH_512_BLOCK_SIZE);
    memcpy(rk_tag, prev_tag, VISTRUTAH_512_BLOCK_SIZE * 8u * sizeof(*rk_tag));
    vistrutith_rotate_bytes_prover(rk_bits + 0u, rk_tag + 0u, 5, 16);
    vistrutith_rotate_bytes_prover(rk_bits + 16u, rk_tag + 16u * 8u, 10, 16);
    vistrutith_rotate_bytes_prover(rk_bits + 32u, rk_tag + 32u * 8u, 5, 16);
    vistrutith_rotate_bytes_prover(rk_bits + 48u, rk_tag + 48u * 8u, 10, 16);
  }

  // :17-19
  for (unsigned int i = 0; i < VISTRUTAH_512_BLOCK_SIZE; ++i) {
    p_bits[i] = (uint8_t)(in_bits[i] ^ round_keys_bits[i]);
  }
  for (unsigned int i = 0; i < VISTRUTAH_512_BLOCK_SIZE * 8u; ++i) {
    p_tag[i] = bf512_add(in_tag[i], round_keys_tag[i]);
  }

  // :21
  vistrutith_forward_by_norm_prover(a_val, a_tag, a_sq_val, a_sq_tag, z_norm_round_val,
                                    z_norm_round_tag, p_bits, p_tag, w + VISTRUTAH_WITNESS_KEY_BYTES,
                                    w_tag + VISTRUTAH_WITNESS_KEY_BYTES * 8u, fixed_key_bits,
                                    fixed_key_tag);

  // :22
  memcpy(z_norm_val + norm_off, z_norm_round_val, sizeof(z_norm_round_val));
  memcpy(z_norm_tag + norm_off, z_norm_round_tag, sizeof(z_norm_round_tag));
  // norm_off means norm_offset, Ni offset
  norm_off += VISTRUTAH_512_BLOCK_SIZE;

  // :24
  for (unsigned int i = 1u; i < steps; ++i) {
    // Mi offset
    const size_t mi_off = (size_t)VISTRUTAH_WITNESS_KEY_BYTES +
                          (size_t)VISTRUTAH_WITNESS_PACKED_ROUND_BYTES +
                          (size_t)(i - 1u) *
                              ((size_t)VISTRUTAH_TRACE_ROUND_BYTES +
                               (size_t)VISTRUTAH_WITNESS_PACKED_ROUND_BYTES);
    const uint8_t* m_bits = w + mi_off;
    const bf512_t* m_tag = w_tag + mi_off * 8u;
    const uint8_t* n_packed = m_bits + VISTRUTAH_TRACE_ROUND_BYTES;
    const bf512_t* n_tag = m_tag + VISTRUTAH_TRACE_ROUND_BYTES * 8u;
    const uint8_t* rc = ROUND_CONSTANTS + 16u * (i - 1u);
    const uint8_t* round_key_bits = round_keys_bits + i * VISTRUTAH_512_BLOCK_SIZE;
    const bf512_t* round_key_tag = round_keys_tag + i * VISTRUTAH_512_BLOCK_SIZE * 8u;

    uint8_t y_bits[VISTRUTAH_512_BLOCK_SIZE];
    bf512_t y_tag[VISTRUTAH_512_BLOCK_SIZE * 8u];
    bf512_t z0_val[VISTRUTAH_512_BLOCK_SIZE], z0_tag[VISTRUTAH_512_BLOCK_SIZE];
    bf512_t z1_val[VISTRUTAH_512_BLOCK_SIZE], z1_tag[VISTRUTAH_512_BLOCK_SIZE];

    // :25
    vistrutith_backward_zerornd_by_bits_prover(y_bits, y_tag, m_bits, m_tag, round_key_bits,
                                               round_key_tag, rc);

    // :26
    vistrutith_append_cstrnts_by_bits_prover(z0_val, z0_tag, z1_val, z1_tag, a_val, a_tag, a_sq_val,
                                             a_sq_tag, y_bits, y_tag);
    memcpy(z_io0_val + io_off, z0_val, sizeof(z0_val));
    memcpy(z_io0_tag + io_off, z0_tag, sizeof(z0_tag));
    memcpy(z_io1_val + io_off, z1_val, sizeof(z1_val));
    memcpy(z_io1_tag + io_off, z1_tag, sizeof(z1_tag));
    io_off += VISTRUTAH_512_BLOCK_SIZE;

    // :27
    vistrutith_forward_by_norm_prover(a_val, a_tag, a_sq_val, a_sq_tag, z_norm_round_val,
                                      z_norm_round_tag, m_bits, m_tag, n_packed, n_tag,
                                      fixed_key_bits, fixed_key_tag);
    memcpy(z_norm_val + norm_off, z_norm_round_val, sizeof(z_norm_round_val));
    memcpy(z_norm_tag + norm_off, z_norm_round_tag, sizeof(z_norm_round_tag));
    norm_off += VISTRUTAH_512_BLOCK_SIZE;
  }

  {
    const uint8_t* round_key_bits = round_keys_bits + steps * VISTRUTAH_512_BLOCK_SIZE;
    const bf512_t* round_key_tag = round_keys_tag + steps * VISTRUTAH_512_BLOCK_SIZE * 8u;
    uint8_t y_bits[VISTRUTAH_512_BLOCK_SIZE];
    bf512_t y_tag[VISTRUTAH_512_BLOCK_SIZE * 8u];
    bf512_t z0_val[VISTRUTAH_512_BLOCK_SIZE], z0_tag[VISTRUTAH_512_BLOCK_SIZE];
    bf512_t z1_val[VISTRUTAH_512_BLOCK_SIZE], z1_tag[VISTRUTAH_512_BLOCK_SIZE];

    // :31
    vistrutith_backward_finalrnd_by_bits_prover(y_bits, y_tag, out_bits, out_tag, round_key_bits,
                                                round_key_tag);
    // :32                                            
    vistrutith_append_cstrnts_by_bits_prover(z0_val, z0_tag, z1_val, z1_tag, a_val, a_tag, a_sq_val,
                                             a_sq_tag, y_bits, y_tag);
    memcpy(z_io0_val + io_off, z0_val, sizeof(z0_val));
    memcpy(z_io0_tag + io_off, z0_tag, sizeof(z0_tag));
    memcpy(z_io1_val + io_off, z1_val, sizeof(z1_val));
    memcpy(z_io1_tag + io_off, z1_tag, sizeof(z1_tag));
    io_off += VISTRUTAH_512_BLOCK_SIZE;
  }

  assert(norm_off == VISTRUTITH_ENC_CSTRNTS_NORM_LEN);
  assert(io_off == VISTRUTITH_ENC_CSTRNTS_IO_LEN);
}

void vistrutith_enc_constraints_verifier(
    bf512_t z_norm_key[VISTRUTITH_ENC_CSTRNTS_NORM_LEN],
    bf512_t z_io0_key[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    bf512_t z_io1_key[VISTRUTITH_ENC_CSTRNTS_IO_LEN],
    const bf512_t in_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t out_key[VISTRUTAH_512_BLOCK_SIZE * 8u],
    const bf512_t w_key[VISTRUTAH_WITNESS_TOTAL_BYTES * 8u]) {
  const unsigned int steps = VISTRUTITH_ENC_CSTRNTS_STEPS;
  unsigned int norm_off = 0u;
  unsigned int io_off = 0u;

  bf512_t fixed_key_key[VISTRUTAH_512_BLOCK_SIZE * 8u];
  bf512_t round_keys_key[(VISTRUTITH_ENC_CSTRNTS_STEPS + 1u) * VISTRUTAH_512_BLOCK_SIZE * 8u];

  bf512_t p_key[VISTRUTAH_512_BLOCK_SIZE * 8u];
  bf512_t a_key[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t a_sq_key[VISTRUTAH_512_BLOCK_SIZE];
  bf512_t z_norm_round_key[VISTRUTAH_512_BLOCK_SIZE];

  memcpy(fixed_key_key, w_key, VISTRUTAH_WITNESS_KEY_BYTES * 8u * sizeof(*w_key));
  vistrutith_shuffle_key_verifier(fixed_key_key);

  {
    bf512_t* rk0_key = round_keys_key;
    memcpy(rk0_key + 0u * 128u, fixed_key_key + 1u * 128u, 128u * sizeof(*rk0_key));
    memcpy(rk0_key + 1u * 128u, fixed_key_key + 0u * 128u, 128u * sizeof(*rk0_key));
    memcpy(rk0_key + 2u * 128u, fixed_key_key + 3u * 128u, 128u * sizeof(*rk0_key));
    memcpy(rk0_key + 3u * 128u, fixed_key_key + 2u * 128u, 128u * sizeof(*rk0_key));
  }

  for (unsigned int j = 1u; j <= steps; ++j) {
    bf512_t* rk_key = round_keys_key + j * VISTRUTAH_512_BLOCK_SIZE * 8u;
    const bf512_t* prev_key = round_keys_key + (j - 1u) * VISTRUTAH_512_BLOCK_SIZE * 8u;
    memcpy(rk_key, prev_key, VISTRUTAH_512_BLOCK_SIZE * 8u * sizeof(*rk_key));
    vistrutith_rotate_bytes_verifier(rk_key + 0u, 5, 16);
    vistrutith_rotate_bytes_verifier(rk_key + 16u * 8u, 10, 16);
    vistrutith_rotate_bytes_verifier(rk_key + 32u * 8u, 5, 16);
    vistrutith_rotate_bytes_verifier(rk_key + 48u * 8u, 10, 16);
  }

  for (unsigned int i = 0; i < VISTRUTAH_512_BLOCK_SIZE * 8u; ++i) {
    p_key[i] = bf512_add(in_key[i], round_keys_key[i]);
  }

  vistrutith_forward_by_norm_verifier(
      a_key, a_sq_key, z_norm_round_key, p_key, w_key + VISTRUTAH_WITNESS_KEY_BYTES * 8u,
      fixed_key_key);
  memcpy(z_norm_key + norm_off, z_norm_round_key, sizeof(z_norm_round_key));
  norm_off += VISTRUTAH_512_BLOCK_SIZE;

  for (unsigned int i = 1u; i < steps; ++i) {
    const size_t mi_off = (size_t)VISTRUTAH_WITNESS_KEY_BYTES +
                          (size_t)VISTRUTAH_WITNESS_PACKED_ROUND_BYTES +
                          (size_t)(i - 1u) *
                              ((size_t)VISTRUTAH_TRACE_ROUND_BYTES +
                               (size_t)VISTRUTAH_WITNESS_PACKED_ROUND_BYTES);
    const bf512_t* m_key = w_key + mi_off * 8u;
    const bf512_t* n_key = m_key + VISTRUTAH_TRACE_ROUND_BYTES * 8u;
    const uint8_t* rc = ROUND_CONSTANTS + 16u * (i - 1u);
    const bf512_t* round_key_key = round_keys_key + i * VISTRUTAH_512_BLOCK_SIZE * 8u;

    bf512_t y_key[VISTRUTAH_512_BLOCK_SIZE * 8u];
    bf512_t z0_key[VISTRUTAH_512_BLOCK_SIZE], z1_key[VISTRUTAH_512_BLOCK_SIZE];

    vistrutith_backward_zerornd_by_bits_verifier(y_key, m_key, round_key_key, rc);
    vistrutith_append_cstrnts_by_bits_verifier(z0_key, z1_key, a_key, a_sq_key, y_key);
    memcpy(z_io0_key + io_off, z0_key, sizeof(z0_key));
    memcpy(z_io1_key + io_off, z1_key, sizeof(z1_key));
    io_off += VISTRUTAH_512_BLOCK_SIZE;

    vistrutith_forward_by_norm_verifier(a_key, a_sq_key, z_norm_round_key, m_key, n_key,
                                        fixed_key_key);
    memcpy(z_norm_key + norm_off, z_norm_round_key, sizeof(z_norm_round_key));
    norm_off += VISTRUTAH_512_BLOCK_SIZE;
  }

  {
    const bf512_t* round_key_key = round_keys_key + steps * VISTRUTAH_512_BLOCK_SIZE * 8u;
    bf512_t y_key[VISTRUTAH_512_BLOCK_SIZE * 8u];
    bf512_t z0_key[VISTRUTAH_512_BLOCK_SIZE], z1_key[VISTRUTAH_512_BLOCK_SIZE];

    vistrutith_backward_finalrnd_by_bits_verifier(y_key, out_key, round_key_key);
    vistrutith_append_cstrnts_by_bits_verifier(z0_key, z1_key, a_key, a_sq_key, y_key);
    memcpy(z_io0_key + io_off, z0_key, sizeof(z0_key));
    memcpy(z_io1_key + io_off, z1_key, sizeof(z1_key));
    io_off += VISTRUTAH_512_BLOCK_SIZE;
  }

  assert(norm_off == VISTRUTITH_ENC_CSTRNTS_NORM_LEN);
  assert(io_off == VISTRUTITH_ENC_CSTRNTS_IO_LEN);
}
