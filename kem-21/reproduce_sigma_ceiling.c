/* kem-21-2: MAMBA-Viper encapsulation secret r is derived from a 256-bit sigma.
 *
 * Key generation and encapsulation run in the prebuilt NGCC library
 * (kem-21/lib/libMAMBA-Viper-<L>.so, loaded with dlopen). The "attacker" half
 * is compiled from an unmodified copy of the submitted reference sources and
 * uses only (pk, ct) and a 32-byte sigma candidate; it never sees sk.
 *
 * Per trial:
 *   (a) K recomputed from (pk, ct, sigma[0..31]) equals the library's K;
 *   (b) re-encrypting with omega bytes 32..|sigma|-1 inverted gives the
 *       library's ciphertext byte for byte (those bytes are never read);
 *   (c) control: sigma[0..31] with one bit flipped fails the consistency test;
 *   (d) control: the library's own decapsulation returns the same K.
 * A reduced search then enumerates the low SEARCH_BITS bits of sigma (the rest
 * is given as a hint) and recovers K, showing the test used by a full 2^256
 * enumeration.
 *
 * Usage: harness <lib.so> <search_bits> <trials>
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "drng.h"
#include "fips202.h"
#include "viper.h"
#include "viper_arith.h"

DRNG_ctx drng_algorithm; /* used only by the source-built objects; unused here */

typedef int (*keygen_fn)(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
typedef int (*enc_fn)(unsigned char *, unsigned long long, unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
typedef int (*dec_fn)(unsigned char *, unsigned long long, unsigned char *, unsigned long long, unsigned char *, unsigned long long *);
typedef int (*seed_fn)(const unsigned char *, unsigned long long);
typedef int (*rand_fn)(unsigned char *, unsigned long long);

#define SIGMA_READ 32 /* bytes absorbed by viper_sample_secret (viper.c:425,437) */
#define SIGMA_HELD VIPER_FALLBACK_KEY_BYTES /* sigma bytes the KEM carries in omega */

static const unsigned char *ct_mu(const unsigned char *ct) { return ct + VIPER_PACKED_U_BYTES + VIPER_PACKED_V_BYTES; }

/* Precomputed public data for one (pk, ct). */
typedef struct {
  vpolyvec bhat;
  vpoly vhat;
  unsigned char hpk[32];
} pubctx;

static void pub_init(pubctx *p, const unsigned char *pk, const unsigned char *ct) {
  static vpoly A[VIPER_K][VIPER_K];
  static uint16_t dpk[VIPER_K][VIPER_N], du[VIPER_K][VIPER_N], dv[VIPER_N];
  viper_gen_public(A, dpk, pk);
  viper_gen_dither(du, dv, ct_mu(ct));
  for (size_t i = 0; i < VIPER_K; i++)
    viper_unpack_reconstruct_array(p->bhat[i], pk + 32 + i * VIPER_PACKED_PK_POLYBYTES, dpk[i], VIPER_T_PK);
  viper_unpack_reconstruct_array(p->vhat, ct + VIPER_PACKED_U_BYTES, dv, VIPER_T_V);
  shake128(p->hpk, 32, pk, VIPER_PUBLICKEYBYTES);
}

/* Candidate test: r = GenSecret(sigma), m = Decode(vhat - bhat^T r),
 * (Kbar, sigma') = G(m || H(pk)); accept iff sigma'[0..31] == sigma.
 * On acceptance, confirm with the submitted re-encryption check and derive K. */
static int try_sigma(unsigned char K[VIPER_SSBYTES], const pubctx *p, const unsigned char *pk,
                     const unsigned char *ct, const unsigned char sigma[SIGMA_READ]) {
  vpolyvec r;
  vpoly w, t;
  unsigned char m[VIPER_MSGBYTES], buf[VIPER_MSGBYTES + 32], kr[VIPER_SSBYTES + VIPER_FALLBACK_KEY_BYTES];
  unsigned char hct[32], kdfin[VIPER_SSBYTES + 32];
  viper_sample_secret(r, sigma, VIPER_ETA_R);
  viper_dot(t, p->bhat, r);
  for (size_t i = 0; i < VIPER_N; i++) w[i] = (uint16_t)((p->vhat[i] - t[i]) & VIPER_Q_MASK);
  viper_decode(m, w);
  memcpy(buf, m, VIPER_MSGBYTES);
  memcpy(buf + VIPER_MSGBYTES, p->hpk, 32);
  shake128(kr, sizeof(kr), buf, sizeof(buf));
  if (memcmp(kr + VIPER_SSBYTES, sigma, SIGMA_READ) != 0) return 0;
  if (!viper_reencrypt_check(ct, pk, m, kr + VIPER_SSBYTES)) return 0;
  shake128(hct, 32, ct, VIPER_CIPHERTEXTBYTES);
  memcpy(kdfin, kr, VIPER_SSBYTES);
  memcpy(kdfin + VIPER_SSBYTES, hct, 32);
  shake128(K, VIPER_SSBYTES, kdfin, sizeof(kdfin));
  return 1;
}

int main(int argc, char **argv) {
  if (argc != 4) { fprintf(stderr, "usage: %s lib.so search_bits trials\n", argv[0]); return 2; }
  int search_bits = atoi(argv[2]), trials = atoi(argv[3]);
  if (search_bits < 0 || search_bits > 32 || trials < 1) return 2;
  void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!h) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }
  keygen_fn kg = (keygen_fn)dlsym(h, "kem_keygen");
  enc_fn enc = (enc_fn)dlsym(h, "kem_enc");
  dec_fn dec = (dec_fn)dlsym(h, "kem_dec");
  seed_fn seed = (seed_fn)dlsym(h, "ngcc_seed");
  rand_fn rnd = (rand_fn)dlsym(h, "ngcc_random");
  unsigned long long (*ctlen)(void) = (unsigned long long (*)(void))dlsym(h, "kem_get_ct_len_bytes");
  if (!kg || !enc || !dec || !seed || !rnd || !ctlen) { fprintf(stderr, "missing symbol\n"); return 2; }
  if (ctlen() != VIPER_CIPHERTEXTBYTES) { fprintf(stderr, "library/source level mismatch\n"); return 2; }

  static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], ct[CRYPTO_CIPHERTEXTBYTES], ct2[CRYPTO_CIPHERTEXTBYTES];
  unsigned char ss[CRYPTO_BYTES], ssd[CRYPTO_BYTES], K[VIPER_SSBYTES];
  unsigned long long l1, l2;
  static pubctx P;
  int ok_dec = 0, ok_rec = 0, ok_tail = 0, ok_rej = 0, ok_search = 0, search_trials = trials < 3 ? trials : 3;

  printf("%s: target=%d bits, |K|=%d bits, sigma held=%d bits, sigma read=%d bits, tail bytes=%d\n",
         VIPER_ALGNAME, VIPER_SSBYTES * 8, VIPER_SSBYTES * 8, SIGMA_HELD * 8, SIGMA_READ * 8, SIGMA_HELD - SIGMA_READ);
  for (int tr = 0; tr < trials; tr++) {
    unsigned char s1[48], s2[48], m0[VIPER_MSGBYTES], m[VIPER_MSGBYTES], buf[VIPER_MSGBYTES + 32], kr[VIPER_SSBYTES + VIPER_FALLBACK_KEY_BYTES];
    for (int i = 0; i < 48; i++) { s1[i] = (unsigned char)(i + 7 * tr); s2[i] = (unsigned char)(0xA5 ^ i ^ (13 * tr)); }
    /* victim: library key generation, encapsulation and decapsulation */
    seed(s1, 48); if (kg(pk, &l1, sk, &l2) != 0) return 2;
    seed(s2, 48); if (enc(pk, sizeof(pk), ss, &l1, ct, &l2) != 0) return 2;
    if (dec(sk, sizeof(sk), ct, sizeof(ct), ssd, &l1) != 0) return 2;
    ok_dec += memcmp(ss, ssd, CRYPTO_BYTES) == 0;
    /* The search target: replay the encapsulator's DRBG to learn its sigma.
     * This stands in for the attacker's enumeration of 2^256 candidates. */
    seed(s2, 48); rnd(m0, VIPER_MSGBYTES * 8ULL);
    shake128(m, VIPER_MSGBYTES, m0, VIPER_MSGBYTES);
    shake128(buf + VIPER_MSGBYTES, 32, pk, VIPER_PUBLICKEYBYTES);
    memcpy(buf, m, VIPER_MSGBYTES);
    shake128(kr, sizeof(kr), buf, sizeof(buf));
    const unsigned char *sigma = kr + VIPER_SSBYTES;

    pub_init(&P, pk, ct);
    /* (a) recovery from (pk, ct, sigma[0..31]) only */
    if (try_sigma(K, &P, pk, ct, sigma) && memcmp(K, ss, CRYPTO_BYTES) == 0) ok_rec++;
    /* (b) omega bytes beyond 32 do not affect the ciphertext */
    unsigned char omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES];
    memcpy(omega, sigma, VIPER_FALLBACK_KEY_BYTES);
    memcpy(omega + VIPER_FALLBACK_KEY_BYTES, ct_mu(ct), VIPER_MU_BYTES);
    for (int i = SIGMA_READ; i < VIPER_FALLBACK_KEY_BYTES; i++) omega[i] ^= 0xFF;
    viper_pke_enc(ct2, pk, m, omega);
    if (memcmp(ct, ct2, CRYPTO_CIPHERTEXTBYTES) == 0) ok_tail++;
    /* (c) control: one wrong bit in the 32-byte prefix */
    unsigned char bad[SIGMA_READ];
    memcpy(bad, sigma, SIGMA_READ); bad[tr % SIGMA_READ] ^= (unsigned char)(1u << (tr % 8));
    if (!try_sigma(K, &P, pk, ct, bad)) ok_rej++;
    /* reduced enumeration: low search_bits of sigma unknown, the rest a hint */
    if (tr < search_trials) {
      unsigned char cand[SIGMA_READ];
      uint32_t mask = search_bits == 32 ? 0xffffffffu : ((1u << search_bits) - 1u), hits = 0, found = 0;
      uint32_t low = (uint32_t)sigma[0] | (uint32_t)sigma[1] << 8 | (uint32_t)sigma[2] << 16 | (uint32_t)sigma[3] << 24;
      memcpy(cand, sigma, SIGMA_READ);
      for (uint64_t g = 0; g <= mask; g++) {
        uint32_t x = (low & ~mask) | (uint32_t)g;
        cand[0] = (unsigned char)x; cand[1] = (unsigned char)(x >> 8); cand[2] = (unsigned char)(x >> 16); cand[3] = (unsigned char)(x >> 24);
        if (try_sigma(K, &P, pk, ct, cand)) { hits++; found = memcmp(K, ss, CRYPTO_BYTES) == 0 && x == low; }
      }
      printf("  trial %d: 2^%d search over sigma[0..31]: %u hit(s), recovered K %s\n", tr, search_bits, hits,
             (hits == 1 && found) ? "matches library K" : "MISMATCH");
      ok_search += hits == 1 && found;
    }
  }
  printf("control: library decaps matches encaps: %d/%d\n", ok_dec, trials);
  printf("K recovered from (pk, ct, sigma[0..31]) without sk: %d/%d\n", ok_rec, trials);
  if (SIGMA_HELD == SIGMA_READ)
    printf("omega tail beyond 32 sigma bytes: none at this level\n");
  else
    printf("ciphertext unchanged with omega[32..%d] inverted: %d/%d\n", VIPER_FALLBACK_KEY_BYTES - 1, ok_tail, trials);
  printf("control: one-bit-wrong sigma prefix rejected: %d/%d\n", ok_rej, trials);
  printf("reduced 2^%d enumeration recovered K: %d/%d\n", search_bits, ok_search, search_trials);
  int pass = ok_dec == trials && ok_rec == trials && ok_tail == trials && ok_rej == trials && ok_search == search_trials;
  printf("RESULT %s sigma_bits=%d target_bits=%d %s\n", VIPER_ALGNAME, SIGMA_READ * 8, VIPER_SSBYTES * 8, pass ? "PASS" : "FAIL");
  return pass ? 0 : 1;
}
