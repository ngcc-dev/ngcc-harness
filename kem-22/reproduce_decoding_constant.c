/*
 * Mithril-256: an honest KEM encapsulation that the specified decapsulation
 * gets wrong.  Self-contained reproducer for the NGCC Round 1 submission
 * (Mithril.zip, SHA-256 9fed68e7923c6bc9ede072183f7d3983058f88deec5534e779ed2200ee4f7f9f).
 *
 * Build against the submitted reference implementation:
 *   D=kem-22/Implementations/Reference_Implementation/Mithril-256
 *   gcc -O2 -I$D -I$D/utils -I$D/arith -DRRLWR_SECURITY_LEVEL=256 \
 *       $D/arith/poly.c $D/arith/ring.c $D/arith/packing.c $D/arith/uniform.c \
 *       $D/pke.c $D/kem.c $D/utils/auxfunc.c $D/utils/drng.c kem-22/reproduce_decoding_constant.c -o repro
 *
 * The record is a key pair (seedA, s, z) and a message m.  Everything is
 * computed with the submitters' own functions:
 *   pk = seedA || Pack(round(p/q * A * s))       (as in pke_keygen, with s given)
 *   sk = Pack(s) || pk || F(pk) || z             (as in kem_keygen)
 *   (K, seed_s') = G(F(pk), m),  ct = pke_encrypt(pk, m; seed_s')   (as in kem_enc)
 *   K' = kem_dec(sk, ct)
 * and the failing coefficient is then decomposed into the LWR noise D and the
 * floor remainder r3 of the c_m compression.
 */
#include <stdio.h>
#include <string.h>
#include "drng.h"
#include "kem.h"
#include "pke.h"
#include "ring.h"
#include "packing.h"
#include "uniform.h"
#include "hash_domain.h"

DRNG_ctx drng_algorithm;   /* referenced by kem.c; unused here */

static const char *SEED_A =
  "9532679be33d4b4ef0570829fe0493243e178bed17b01e628e3649dbfad3bd76"
  "d55cb16fac38b6256a571eac62c3f8a2d48b3bb8d4d3544b089207ed381cfef2";
static const char *S_PACKED =
  "288b85b718f1e0f7c185504efc5d36ec698dd39ec73fcae9af83f4b5ddf72ada"
  "bf9cb85e4e6051a453b82afef9f1a108277b3a1fbc21ed4a72c22d0084237197"
  "5f09777017724c7ed8f502f3c3948086ef0bf1620d701fa2b8a34f71cf27a485"
  "be3a6eb0430d58a5551f489aa75d01e8aa313c8a63b9ea7539e01e1a4b424106"
  "976b3cec57cb18deaba972a5c0224962f74d33814503d92c68b65112a2d3739d"
  "bdac3fd79aedc468c3c7b63f16850b5ab3b2b21af2e3101525edbb13af9f154a"
  "58d35050528f68290dfce9c880f39f0882005db13356ff5a1f610d618858ff3c"
  "0308a51689bf319b448d727801f3195d62663e9cc812dc633023c184537b0eaf"
  "8d9d9e8704361c8300c7d78711831e0b5a11619a3120786e133ee2be651b6b5f";
static const char *Z = "b719590b534783436ae4a28e187ec7ec43a86c005bb8149d3bfa18873072701d";
static const char *M = "b380620c33988e2a25e8a47912bf3c0290d6accc139e044f0ea2f30c2dbf0bd2";

#define P  RRLWR_PKE_P                                   /* 2^11 */
#define PT (1 << (RRLWR_PKE_LOGP - RRLWR_PKE_LOGT))      /* p/t  = 256 */
#define H1 (1 << (RRLWR_PKE_LOGQ - RRLWR_PKE_LOGP - 1))  /* q/2p = 2 */

static void unhex(unsigned char *out, const char *h, size_t n)
{
  for (size_t i = 0; i < n; i++) sscanf(h + 2 * i, "%2hhx", &out[i]);
}
static void hex(const char *tag, const unsigned char *x, size_t n)
{
  printf("%s", tag);
  for (size_t i = 0; i < n; i++) printf("%02x", x[i]);
  printf("\n");
}
static int centred(int x, int mod) { x &= mod - 1; return x >= mod / 2 ? x - mod : x; }

int main(void)
{
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN], m[RRLWR_PKE_MESSAGE_LEN], mdec[RRLWR_PKE_MESSAGE_LEN];
  unsigned char pk[RRLWR_KEM_PK_LEN], sk[RRLWR_KEM_SK_LEN], ct[RRLWR_KEM_CT_LEN];
  unsigned char hpk[RRLWR_KEM_HPK_LEN], Ks[RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN], K2[RRLWR_KEM_SS_LEN];
  unsigned char *seedSp = Ks + RRLWR_KEM_SS_LEN;
  unsigned long long len;
  ring_element s, sp, b;
  ring_element_Awin a, bw, bpw;
  poly accE[RRLWR_PKE_ELL], accD[RRLWR_PKE_ELL];

  unhex(seedA, SEED_A, sizeof seedA);
  unhex(m, M, sizeof m);
  unhex(sk, S_PACKED, RRLWR_PKE_SK_LEN);

  /* key pair: pk = seedA || Pack(round(p/q A s)), exactly as pke_keygen computes it */
  ring_unpack(&s, sk, RRLWR_PKE_LOG_ETA + 1);
  ring_uniform_Awin(&a, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN);
  ring_mul_Awin_round_p(b.x, &a, &s, RRLWR_K);
  memcpy(pk, seedA, RRLWR_PKE_SEED_A_LEN);
  ring_pack(pk + RRLWR_PKE_SEED_A_LEN, &b, RRLWR_PKE_LOGP);
  memcpy(sk + RRLWR_PKE_SK_LEN, pk, RRLWR_PKE_PK_LEN);
  RRLWR_KEM_HASH_F(sk + RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN, pk);
  unhex(sk + RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN, Z, RRLWR_KEM_SEED_Z_LEN);

  /* honest encapsulation of m, exactly as kem_enc does after drawing m */
  RRLWR_KEM_HASH_F(hpk, pk);
  RRLWR_KEM_HASH_G(Ks, RRLWR_KEM_SS_LEN + RRLWR_SEED_S_LEN, hpk, m);
  pke_encrypt(ct, pk, m, seedSp);

  /* decapsulation with the submitted code */
  kem_dec(sk, RRLWR_KEM_SK_LEN, ct, RRLWR_KEM_CT_LEN, K2, &len);
  pke_decrypt(mdec, ct, sk);

  printf("Mithril-256, honest encapsulation of m under pk (submitted reference code)\n");
  hex("  encapsulator K  = ", Ks, RRLWR_KEM_SS_LEN);
  hex("  decapsulator K' = ", K2, RRLWR_KEM_SS_LEN);
  printf("  shared secrets agree: %s\n", memcmp(Ks, K2, RRLWR_KEM_SS_LEN) ? "NO" : "yes");

  /* explain each wrong bit: v - (p/t) c_m = D + r3 - q/2p - (p/2) m, Alg. 1 then adds p/2t - q/2p */
  ring_unpack_Awin(&bw, pk + RRLWR_PKE_SEED_A_LEN, RRLWR_PKE_LOGP);
  ring_uniform(&sp, RRLWR_PKE_LOG_ETA + 1, seedSp, RRLWR_SEED_S_LEN);
  ring_mul_Awin(accE, &bw, &sp, RRLWR_PKE_ELL);                 /* b * s'  (encryptor) */
  ring_unpack_Awin(&bpw, ct + RRLWR_PKE_ELL * RRLWR_PKE_PACKED_POLYT_LEN, RRLWR_PKE_LOGP);
  ring_mul_Awin(accD, &bpw, &s, RRLWR_PKE_ELL);                 /* b' * s  (decryptor) */
  int wrong_bits = 0;
  for (int bit = 0; bit < 8 * RRLWR_PKE_MESSAGE_LEN; bit++) {
    if (!(((m[bit >> 3] ^ mdec[bit >> 3]) >> (bit & 7)) & 1)) continue;
    wrong_bits++;
    int out = bit / RRLWR_N, j = bit % RRLWR_N;
    int A = accE[out].coeffs[j] & (P - 1), V = accD[out].coeffs[j] & (P - 1);
    int D = centred(V - A, P), r3 = (A + H1) & (PT - 1);
    printf("  message bit %d is decrypted wrongly (radical coefficient x^%d, y^%d)\n",
           bit, RRLWR_K - RRLWR_PKE_ELL + out, j);
    printf("    LWR noise D = %d, c_m floor remainder r3 = %d\n", D, r3);
    printf("    Alg. 1 constant +(p/2t - q/2p): decoder offset D + r3 + %d = %d >= p/4 = %d  -> wrong bit\n",
           PT / 2 - 2 * H1, D + r3 + PT / 2 - 2 * H1, P / 4);
    printf("    constant  (q/2p - p/2t):        decoder offset D + r3 - %d = %d <  p/4        -> correct bit\n",
           PT / 2, D + r3 - PT / 2);
  }
  if (wrong_bits != 1 || !memcmp(Ks, K2, RRLWR_KEM_SS_LEN))
    return 1;
  puts("CONFIRMED: honest Mithril-256 encapsulation decapsulates to another secret");
  return 0;
}
