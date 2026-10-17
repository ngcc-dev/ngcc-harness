/* sign-20-5 (signing path): QingLuan-512 derives every per-signature round
 * seed from SeedLeaves = XOF(0x03 || master_seed || salt) (src/sign.c:73-82).
 * The XOF output is SM3(K || BE32(i)) with a 128-byte, block-aligned K
 * (api_pkc/hash_adapter.c:168-177), so one 256-bit SM3 chaining value after
 * K's two blocks determines all round seeds.
 *
 * The program signs with the unmodified signer (randombytes is wrapped only to
 * record master_seed and salt for the ground truth), derives that chaining
 * value, discards K and master_seed, and then works from (pk, sig, CV) only:
 *   1. regenerate all 1018 round seeds from CV;
 *   2. check the 842 opened seeds published in the signature;
 *   3. for each unopened round, expand its seed and add the published
 *      v = eta - eta' to recover eta, the secret exponent vector;
 * and compares with the true eta.  A CV with one flipped bit is the control. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "params.h"
#include "api.h"
#include "sign.h"
#include "hash.h"
#include "mpc.h"
#include "rsdp.h"
#include "restr.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

/* ---- record the two randombytes() draws made by crypto_sign_signature ---- */
int __real_randombytes(uint8_t *out, size_t len);
static int recording;
static uint8_t rec_seed[PARAM_SEED_BYTES], rec_salt[PARAM_SALT_BYTES];
int __wrap_randombytes(uint8_t *out, size_t len)
{
  int r = __real_randombytes(out, len);
  if (recording == 1 && len == PARAM_SEED_BYTES) { memcpy(rec_seed, out, len); recording = 2; }
  else if (recording == 2 && len == PARAM_SALT_BYTES) { memcpy(rec_salt, out, len); recording = 3; }
  return r;
}

/* ---- SM3 compression function (GB/T 32905-2016) ---- */
#define ROL(x, n) ((uint32_t)(((x) << ((n) & 31)) | ((x) >> ((32 - ((n) & 31)) & 31))))
static void sm3_cf(uint32_t V[8], const uint8_t B[64])
{
  uint32_t W[68], W1[64], A, Bb, C, D, E, F, G, H;
  for (int i = 0; i < 16; i++)
    W[i] = (uint32_t)B[4 * i] << 24 | (uint32_t)B[4 * i + 1] << 16 | (uint32_t)B[4 * i + 2] << 8 | B[4 * i + 3];
  for (int j = 16; j < 68; j++) {
    uint32_t x = W[j - 16] ^ W[j - 9] ^ ROL(W[j - 3], 15);
    W[j] = (x ^ ROL(x, 15) ^ ROL(x, 23)) ^ ROL(W[j - 13], 7) ^ W[j - 6];
  }
  for (int j = 0; j < 64; j++) W1[j] = W[j] ^ W[j + 4];
  A = V[0]; Bb = V[1]; C = V[2]; D = V[3]; E = V[4]; F = V[5]; G = V[6]; H = V[7];
  for (int j = 0; j < 64; j++) {
    uint32_t T = j < 16 ? 0x79cc4519u : 0x7a879d8au;
    uint32_t SS1 = ROL(ROL(A, 12) + E + ROL(T, j), 7), SS2 = SS1 ^ ROL(A, 12);
    uint32_t FF = j < 16 ? (A ^ Bb ^ C) : ((A & Bb) | (A & C) | (Bb & C));
    uint32_t GG = j < 16 ? (E ^ F ^ G) : ((E & F) | (~E & G));
    uint32_t TT1 = FF + D + SS2 + W1[j], TT2 = GG + H + SS1 + W[j];
    D = C; C = ROL(Bb, 9); Bb = A; A = TT1; H = G; G = ROL(F, 19); F = E;
    E = TT2 ^ ROL(TT2, 9) ^ ROL(TT2, 17);
  }
  V[0] ^= A; V[1] ^= Bb; V[2] ^= C; V[3] ^= D; V[4] ^= E; V[5] ^= F; V[6] ^= G; V[7] ^= H;
}
static const uint32_t SM3_IV[8] = {0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
                                   0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e};

/* output block i of the counter XOF from the post-key chaining value only */
static void block_from_cv(uint8_t out[32], const uint32_t cv[8], uint32_t ctr)
{
  uint8_t tail[64] = {0};
  uint32_t V[8];
  uint64_t bits = (uint64_t)(PARAM_HASH_BYTES + 4) * 8;
  tail[0] = ctr >> 24; tail[1] = ctr >> 16; tail[2] = ctr >> 8; tail[3] = ctr;
  tail[4] = 0x80;
  for (int i = 0; i < 8; i++) tail[56 + i] = (uint8_t)(bits >> (56 - 8 * i));
  memcpy(V, cv, sizeof V);
  sm3_cf(V, tail);
  for (int i = 0; i < 8; i++) {
    out[4 * i] = V[i] >> 24; out[4 * i + 1] = V[i] >> 16; out[4 * i + 2] = V[i] >> 8; out[4 * i + 3] = V[i];
  }
}

static void leaves_from_cv(uint8_t *leaves, const uint32_t cv[8])
{
  size_t total = (size_t)PARAM_TAU * PARAM_SEED_BYTES;
  uint8_t blk[32];
  for (uint32_t c = 0; (size_t)c * 32 < total; c++) {
    block_from_cv(blk, cv, c);
    size_t take = total - (size_t)c * 32 < 32 ? total - (size_t)c * 32 : 32;
    memcpy(leaves + (size_t)c * 32, blk, take);
  }
}

/* attacker: from (sig, cv) count opened-seed matches and recover eta */
static void attack(const uint8_t *sig, const uint32_t cv[8], uint8_t eta_rec[PARAM_N],
                   int *opened_match, int *opened_total, int *unopened)
{
  static uint8_t leaves[(size_t)PARAM_TAU * PARAM_SEED_BYTES];
  uint8_t chall2[PARAM_TAU], v[PARAM_N], eta_p[PARAM_N];
  fq_t u_p[PARAM_N];
  const uint8_t *salt = sig + SIG_OFF_SALT;
  size_t op = SIG_OFF_PATH, ore = SIG_OFF_RESP;
  int have = 0;

  leaves_from_cv(leaves, cv);
  mpc_gen_chall2(chall2, sig + SIG_OFF_DIGEST_CHALL2);
  *opened_match = *opened_total = *unopened = 0;
  memset(eta_rec, 0, PARAM_N);
  for (int i = 0; i < PARAM_TAU; i++) {
    const uint8_t *leaf = leaves + (size_t)i * PARAM_SEED_BYTES;
    if (chall2[i] == 1) {
      (*opened_total)++;
      if (memcmp(leaf, sig + op, PARAM_SEED_BYTES) == 0) (*opened_match)++;
      op += PARAM_SEED_BYTES;
    } else {
      (*unopened)++;
      rsdp_unpack_fz(v, sig + ore + QINGLUAN_Y_BYTES, PARAM_N);
      ore += SIG_RESP0_BYTES;
      if (!have) {
        mpc_expand_round(eta_p, u_p, leaf, salt, (uint16_t)(i + PARAM_C));
        for (int j = 0; j < PARAM_N; j++) eta_rec[j] = restr_exp_add(v[j], eta_p[j]);
        have = 1;
      }
    }
  }
}

int main(void)
{
  static uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], sig[CRYPTO_BYTES];
  uint8_t seed[48], msg[] = "QingLuan-512 signing-path chaining-value witness";
  uint8_t seed_e[PARAM_KEYSEED_BYTES], seed_pk[PARAM_KEYSEED_BYTES];
  uint8_t eta_true[PARAM_N], eta_rec[PARAM_N];
  uint32_t cv[8];
  size_t siglen;
  int om, ot, un;

  restr_init();
  for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(0x29 * i + 7);
  init_random_number(&drng_algorithm, seed, 48);
  if (crypto_sign_keypair(pk, sk) != 0) return 2;
  recording = 1;
  if (crypto_sign_signature(sig, &siglen, msg, sizeof msg, sk) != 0) return 2;
  recording = 0;
  printf("signature: %zu bytes, verify = %d, tau = %d, opened = %d\n", siglen,
         crypto_sign_verify(sig, siglen, msg, sizeof msg, pk), PARAM_TAU, PARAM_W);
  if (memcmp(sig + SIG_OFF_SALT, rec_salt, PARAM_SALT_BYTES) != 0) {
    printf("recorded salt does not match the signature\n");
    return 2;
  }

  { /* ground truth: the 256-bit chaining value after the 128-byte XOF key */
    xof_ctx_t xof;
    uint8_t d = DOMAIN_SEEDLEAVES;
    xof_init(&xof);
    xof_absorb(&xof, &d, 1);
    xof_absorb(&xof, rec_seed, PARAM_SEED_BYTES);
    xof_absorb(&xof, rec_salt, PARAM_SALT_BYTES);
    xof_finalize(&xof);
    printf("XOF key: %d bytes = %d SM3 blocks\n", (int)sizeof xof.key, (int)sizeof xof.key / 64);
    memcpy(cv, SM3_IV, sizeof cv);
    sm3_cf(cv, xof.key);
    sm3_cf(cv, xof.key + 64);
    memset(&xof, 0, sizeof xof);
    memset(rec_seed, 0, sizeof rec_seed);
  }

  rsdp_keypair_seeds(seed_e, seed_pk, sk);
  rsdp_gen_secret_exp(eta_true, seed_e);

  attack(sig, cv, eta_rec, &om, &ot, &un);
  printf("from CV: opened seeds matched %d/%d; eta from unopened round: %s (%d unopened rounds)\n",
         om, ot, memcmp(eta_rec, eta_true, PARAM_N) == 0 ? "equal to the secret" : "different", un);

  cv[0] ^= 1;
  attack(sig, cv, eta_rec, &om, &ot, &un);
  printf("control (one CV bit flipped): opened seeds matched %d/%d; eta %s\n",
         om, ot, memcmp(eta_rec, eta_true, PARAM_N) == 0 ? "equal (unexpected)" : "different");
  return 0;
}
