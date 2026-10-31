/*
 * BAG-Piglet (kem-04) chosen-ciphertext key recovery via the rejection-key
 * plaintext-checking oracle (extension of kem-04-2).
 *
 * The attacker uses ONLY: the public key, and a decapsulation oracle that
 * returns the 32-byte session key for any submitted ciphertext
 * (ccakem_decaps).  The secret key is touched only in the clearly marked
 * VALIDATION section, after the attack has finished, to check the result.
 *
 * Oracle: Decaps(c) returns K(xi, Enc(pk, m', G(pk, m', salt))) for invalid c,
 * a deterministic function of the decrypted m' (salt fixed).  A reference
 * ciphertext decrypting to m'=0 gives R0; Decaps(c)==R0 iff Dec(c)==0.
 *
 * Decryption: combined = v + x*u, prefix = first N_ coords, suffix = rest.
 * The decoder takes W = span of the first EPS independent suffix coords,
 * applies the subspace polynomial of W to the prefix and runs a Gabidulin
 * decoder of radius TG = PARAM_E_Gabidulin.  With u = (u1, 0), prefix =
 * z + D-vectors where z = x*u1 + base (z = x for u1=1, base=0; z = y1 for
 * u1 = h1, base = s1).  Decoding to 0 succeeds iff
 *   dim(span(z) + D + W) - EPS <= TG,
 * which (generically) is: dim(Supp(z) cap (W' + D)) >= 1 for
 * d = TG + A - s + 1 D-vectors, A = #z-coords that fall in the suffix.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/random.h>

#include "auxfunc.h"
#include "augabidulin.h"
#include "bag_piglet.h"
#include "ccakem.h"
#include "ffi_elt.h"
#include "ffi_vec.h"
#include "ffi_field.h"
#include "parameters.h"

#define M PARAM_M
#define N PARAM_N
#define N1 PARAM_N_1
#define NP PARAM_N_
#define EPS PARAM_epsilon
#define TG PARAM_E_Gabidulin
#define SX WEIGHT_X_VALUES
#define SY WEIGHT_Y1_VALUES
#define SEEDB SEEDEXPANDER_SEED_BYTES
#define NW 3

typedef struct { uint64_t w[NW]; } bv;

static uint64_t rng_s[2];
static uint64_t rnd64(void) {
  uint64_t s1 = rng_s[0]; const uint64_t s0 = rng_s[1];
  rng_s[0] = s0; s1 ^= s1 << 23;
  rng_s[1] = s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26);
  return rng_s[1] + s0;
}
static bv bv_rand(void) {
  bv r; memset(&r, 0, sizeof r);
  for(int i = 0; i < NW; i++) r.w[i] = rnd64();
  int top = M % 64; int tw = M / 64;
  for(int i = tw + (top ? 1 : 0); i < NW; i++) r.w[i] = 0;
  if(top) r.w[tw] &= (1ULL << top) - 1;
  return r;
}
static int bv_iszero(const bv *a) { for(int i = 0; i < NW; i++) if(a->w[i]) return 0; return 1; }
static void bv_xor(bv *a, const bv *b) { for(int i = 0; i < NW; i++) a->w[i] ^= b->w[i]; }
static int bv_get(const bv *a, int j) { return (a->w[j / 64] >> (j % 64)) & 1; }
static void bv_set(bv *a, int j, int b) { if(b) a->w[j/64] |= 1ULL << (j%64); else a->w[j/64] &= ~(1ULL << (j%64)); }
static int bv_eq(const bv *a, const bv *b) { return memcmp(a, b, sizeof *a) == 0; }

/* echelon subspace: basis vectors with distinct pivots (highest set bit) */
typedef struct { bv b[160]; int piv[160]; int dim; } sp;
static int bv_top(const bv *a) { for(int j = M - 1; j >= 0; j--) if(bv_get(a, j)) return j; return -1; }
static void sp_reduce(const sp *S, bv *v) {
  for(int i = 0; i < S->dim; i++) if(bv_get(v, S->piv[i])) bv_xor(v, &S->b[i]);
}
/* keep basis sorted so reduction in order works: insert maintaining full reduction */
static int sp_add(sp *S, bv v) {
  sp_reduce(S, &v);
  if(bv_iszero(&v)) return 0;
  int p = bv_top(&v);
  for(int i = 0; i < S->dim; i++) if(bv_get(&S->b[i], p)) bv_xor(&S->b[i], &v);
  S->b[S->dim] = v; S->piv[S->dim] = p; S->dim++;
  return 1;
}
static int sp_contains(const sp *S, bv v) { sp_reduce(S, &v); return bv_iszero(&v); }

/* compact element packing, element idx at bit idx*M */
static void put_elt(unsigned char *buf, int idx, const bv *e) {
  for(int j = 0; j < M; j++) {
    unsigned pos = (unsigned) idx * M + j;
    buf[pos / 8] = (unsigned char) ((buf[pos / 8] & ~(1u << (pos % 8))) | ((unsigned) bv_get(e, j) << (pos % 8)));
  }
}
static bv get_elt(const unsigned char *buf, int idx) {
  bv e; memset(&e, 0, sizeof e);
  for(int j = 0; j < M; j++) { unsigned pos = (unsigned) idx * M + j; bv_set(&e, j, (buf[pos/8] >> (pos%8)) & 1); }
  return e;
}
static void ffivec_to_bv(bv *out, const ffi_vec *v, int n) {
  unsigned char *buf = calloc((size_t) n * M / 8 + 8, 1);
  ffi_vec_to_string_compact(buf, v, (unsigned) n);
  for(int i = 0; i < n; i++) out[i] = get_elt(buf, i);
  free(buf);
}
static void bv_to_ffivec(ffi_vec *v, const bv *in, int n) {
  unsigned char *buf = calloc((size_t) n * M / 8 + 8, 1);
  for(int i = 0; i < n; i++) put_elt(buf, i, &in[i]);
  ffi_vec_from_string_compact(v, (unsigned) n, buf);
  free(buf);
}

/* ---------------- oracle ---------------- */
static uint8_t SK[CCAKEM_SK_SIZE];          /* held by the oracle only */
static long long nqueries = 0;
static void oracle(uint8_t key[CCAKEM_KEY_SIZE], const uint8_t ct[CCAKEM_CT_SIZE]) {
  nqueries++;
  ccakem_decaps(key, SK, ct);
}

/* ---------------- public data ---------------- */
static uint8_t PK[CCAKEM_PK_SIZE];
static bv H1[N], H2[N], S1[N], S2[N];
static uint8_t R0[CCAKEM_KEY_SIZE];
static bv ONE;
static bv CW1[N], CW2[N];   /* augmented-Gabidulin codeword of m0 (prefix part) */

static void parse_pk(void) {
  unsigned char *output = malloc(SEEDEXPANDER_MAX_LENGTH);
  pseudoXOF(8ULL * SEEDEXPANDER_MAX_LENGTH, PK, 8ULL * SEEDB, output);
  ffi_vec g = {0}, h[2] = {{0}};
  ffi_vec_set_length(&g, N * N1);
  int g_blocks = ffi_vec_set_g_using_shake(&g, N * N1, NP, output);
  size_t g_end = (size_t) g_blocks * (NP * M / 8 + 1);
  for(int i = 0; i < 2; i++) {
    ffi_vec_set_length(&h[i], N);
    ffi_vec_set_random_using_shake(&h[i], N, output + g_end + (size_t) i * N * FFI_ELT_BYTES);
  }
  ffivec_to_bv(H1, &h[0], N); ffivec_to_bv(H2, &h[1], N);
  for(int i = 0; i < N; i++) { S1[i] = get_elt(PK + SEEDB, i); S2[i] = get_elt(PK + SEEDB, N + i); }
  free(output);
  {
    augabidulin_code code = {0};
    if(augabidulin_code_init(&code, &g, PARAM_K, N * N1, NP) != 0) abort();
    ffi_vec mm = {0}, cw = {0}; ffi_vec_set_length(&mm, PARAM_K);
    for(int i = 0; i < PARAM_K; i++) { ffi_elt r; ffi_elt_set_random2(&r); ffi_vec_set_coeff(&mm, &r, i); }
    augabidulin_code_encode(&cw, &code, &mm);
    bv tmpc[N * N1]; ffivec_to_bv(tmpc, &cw, N * N1);
    for(int i = 0; i < N; i++) { CW1[i] = tmpc[i]; CW2[i] = tmpc[N + i]; }
    augabidulin_code_clear(&code);
  }
  ffi_vec one = {0}; ffi_vec_set_length(&one, 1); ffi_elt o = ffi_elt_get_one(); ffi_vec_set_coeff(&one, &o, 0);
  bv tmp[1]; ffivec_to_bv(tmp, &one, 1); ONE = tmp[0];
}

/* ring product a*b mod P (both length N) via library */
static void ring_mul(bv *out, const bv *a, const bv *b) {
  ffi_vec A = {0}, B = {0}, O = {0};
  bv_to_ffivec(&A, a, N); bv_to_ffivec(&B, b, N);
  ffi_vec_mul(&O, &A, &B, N);
  ffivec_to_bv(out, &O, N);
  ffi_vec_clear(&A); ffi_vec_clear(&B); ffi_vec_clear(&O);
}

/* build ciphertext: v block1 (N), v block2 (N), u1 (N), u2 = 0; salt = 0 */
static void build_ct(uint8_t ct[CCAKEM_CT_SIZE], const bv *v1, const bv *v2, const bv *u1) {
  memset(ct, 0, CCAKEM_CT_SIZE);
  for(int i = 0; i < N; i++) put_elt(ct, i, &v1[i]);
  for(int i = 0; i < N; i++) put_elt(ct, N + i, &v2[i]);
  for(int i = 0; i < N; i++) put_elt(ct, 2 * N + i, &u1[i]);
  /* u2 = 0 and salt = 0 already */
}

/* geometry: which block-1 coords are prefix; where W goes in block 2 */
#define PREF1 (NP < N ? NP : N)                 /* prefix coords of block 1 */
#define AEXTRA (N - PREF1)                       /* block-1 coords in suffix */
#define WOFF (NP > N ? NP - N : 0)               /* first suffix coord in block 2 */

static int mode_y = 0;            /* 0: target x (u1=1), 1: target y1 (u1=h1, base s1) */
static int tg_eff = TG;
static int s_dim;                 /* dimension of target support */

/* one test: W' basis (EPS-AEXTRA vectors) + fillers, D basis (d vectors) */
static int test(const bv *Wp, int nw, const bv *Fill, int nfill, const bv *D, int d) {
  bv v1[N], v2[N], u1[N];
  memset(v1, 0, sizeof v1); memset(v2, 0, sizeof v2); memset(u1, 0, sizeof u1);
  if(mode_y) { for(int i = 0; i < N; i++) { v1[i] = S1[i]; u1[i] = H1[i]; } }
  else u1[0] = ONE;
  for(int i = 0; i < N; i++) bv_xor(&v1[i], &CW1[i]);
  for(int i = 0; i < WOFF; i++) bv_xor(&v2[i], &CW2[i]);
  for(int j = 0; j < d; j++) bv_xor(&v1[j], &D[j]);
  int pos = WOFF;
  for(int j = 0; j < nw; j++) v2[pos++] = Wp[j];
  for(int j = 0; j < nfill && pos < N; j++) v2[pos++] = Fill[j];
  uint8_t ct[CCAKEM_CT_SIZE], key[CCAKEM_KEY_SIZE];
  build_ct(ct, v1, v2, u1);
  oracle(key, ct);
  return memcmp(key, R0, CCAKEM_KEY_SIZE) == 0;
}

static void make_reference(void) {
  bv v1[N], v2[N], u1[N];
  memset(v1, 0, sizeof v1); memset(v2, 0, sizeof v2); memset(u1, 0, sizeof u1);
  for(int i = 0; i < N; i++) v1[i] = CW1[i];
  for(int i = 0; i < WOFF; i++) v2[i] = CW2[i];
  sp tmp; tmp.dim = 0; int pos = WOFF;
  while(tmp.dim < EPS + 2 && pos < N) { bv r = bv_rand(); if(sp_add(&tmp, r)) v2[pos++] = r; }
  uint8_t ct[CCAKEM_CT_SIZE];
  build_ct(ct, v1, v2, u1);
  oracle(R0, ct);
}

/* random subspace of span(U) of dim b, as list */
static void rand_sub_of(bv *out, int b, const sp *U, sp *acc) {
  int got = 0;
  while(got < b) {
    bv r; memset(&r, 0, sizeof r);
    for(int i = 0; i < U->dim; i++) if(rnd64() & 1) bv_xor(&r, &U->b[i]);
    if(sp_add(acc, r)) out[got++] = r;
  }
}

/* recover the support (dim s) of the current target; returns it in OUT */
static void recover_support(sp *OUT) {
  const int d = tg_eff + AEXTRA - s_dim + 1;
  const int nw = EPS - AEXTRA;
  const int w0 = nw + d - 1;
  const int kappa = s_dim + 2;
  bv Fill[8]; for(int i = 0; i < 8; i++) Fill[i] = bv_rand();
  printf("  d=%d, |W'|=%d, |W0|=%d, quotient dim=%d, s=%d\n", d, nw, w0, M - w0, s_dim);
  /* ---------- round 1: random probing in F^M / W0 ---------- */
  sp U; U.dim = 0;
  for(;;) {
    sp W0s; W0s.dim = 0; bv W0[200]; int k = 0;
    while(k < w0) { bv r = bv_rand(); if(sp_add(&W0s, r)) W0[k++] = r; }
    sp hits = W0s; int nh = 0; long long tries = 0; int allhit = 0, nprobe = 0;
    while(hits.dim < w0 + s_dim) {
      bv v = bv_rand(); if(sp_contains(&W0s, v)) continue;
      bv D[64]; for(int j = 0; j < d - 1; j++) D[j] = W0[nw + j]; D[d - 1] = v;
      int r = test(W0, nw, Fill, 8, D, d); tries++; nprobe++;
      if(r) { nh++; sp_add(&hits, v); }
      if(nprobe >= 24 && nh * 10 >= nprobe * 9) { allhit = 1; break; }
      if(nprobe > (64LL << (M - w0 - s_dim)) + 2000) { allhit = 1; printf("  round1: stalled (hits span %d), resampling\n", hits.dim - w0); break; }
    }
    if(allhit) { printf("  round1: W0 meets target support, resampling\n"); continue; }
    printf("  round1: %lld probes, %d hits, U dim %d\n", tries, nh, hits.dim);
    U = hits; break;
  }
  /* ---------- later rounds: enumerate U / B ---------- */
  int round = 2;
  while(U.dim > s_dim) {
    int b = U.dim - kappa; if(b < 0) b = 0;
    sp acc; acc.dim = 0; bv B[200]; rand_sub_of(B, b, &U, &acc);
    /* complement of B inside U */
    bv C[64]; int nc = 0; sp accU = acc;
    for(int i = 0; i < U.dim && nc < U.dim - b; i++) { if(sp_add(&accU, U.b[i])) C[nc++] = U.b[i]; }
    /* W0 = B + extra random vectors avoiding U */
    bv W0[200]; int k = 0; sp W0s; W0s.dim = 0; sp UW = U;
    for(int i = 0; i < b; i++) { W0[k++] = B[i]; sp_add(&W0s, B[i]); }
    while(k < w0) { bv r = bv_rand(); sp t2 = UW; if(sp_add(&t2, r)) { UW = t2; sp_add(&W0s, r); W0[k++] = r; } }
    /* shuffle W0 so that B is spread between W' and D' parts: keep as is (B first) */
    sp newU = acc; /* B */
    int nh = 0; long long cnt = 0;
    for(uint64_t c = 1; c < (1ULL << nc); c++) {
      bv v; memset(&v, 0, sizeof v);
      for(int j = 0; j < nc; j++) if((c >> j) & 1) bv_xor(&v, &C[j]);
      bv D[64]; for(int j = 0; j < d - 1; j++) D[j] = W0[nw + j]; D[d - 1] = v;
      if(test(W0, nw, Fill, 8, D, d)) { nh++; sp_add(&newU, v); }
      cnt++;
    }
    if(nh == (int) cnt) { printf("  round %d: B meets support, retry\n", round); continue; }
    if(nh != (1 << s_dim) - 1) { printf("  round %d: anomalous hit count %d, retry\n", round, nh); continue; }
    printf("  round %d: %lld probes, U dim %d -> %d\n", round, cnt, U.dim, newU.dim);
    U = newU; round++;
  }
  *OUT = U;
}

/* ---------- F2 linear system for s1 = x*h1 + y1 with known supports ---------- */
static int solve_x(const sp *Sx, const sp *Ty, bv *xout) {
  const int nvx = Sx->dim * N, nvy = Ty->dim * N, nv = nvx + nvy;
  const int neq = N * M;
  const int words = (nv + 1 + 63) / 64;
  uint64_t *A = calloc((size_t) neq * words, 8);
  /* column for x-var (a,i): sigma_a X^i * h1 */
  bv *col = malloc(sizeof(bv) * N);
  for(int a = 0; a < Sx->dim; a++) for(int i = 0; i < N; i++) {
    bv e[N]; memset(e, 0, sizeof e); e[i] = Sx->b[a];
    ring_mul(col, e, H1);
    int c = a * N + i;
    for(int r = 0; r < N; r++) for(int j = 0; j < M; j++) if(bv_get(&col[r], j)) A[(size_t)(r * M + j) * words + c / 64] |= 1ULL << (c % 64);
  }
  for(int b = 0; b < Ty->dim; b++) for(int i = 0; i < N; i++) {
    int c = nvx + b * N + i;
    for(int j = 0; j < M; j++) if(bv_get(&Ty->b[b], j)) A[(size_t)(i * M + j) * words + c / 64] |= 1ULL << (c % 64);
  }
  for(int r = 0; r < N; r++) for(int j = 0; j < M; j++) if(bv_get(&S1[r], j)) A[(size_t)(r * M + j) * words + nv / 64] |= 1ULL << (nv % 64);
  /* gaussian elimination */
  int *pivrow = malloc(sizeof(int) * nv); int row = 0;
  for(int c = 0; c < nv; c++) {
    pivrow[c] = -1; int p = -1;
    for(int r = row; r < neq; r++) if((A[(size_t) r * words + c / 64] >> (c % 64)) & 1) { p = r; break; }
    if(p < 0) continue;
    if(p != row) for(int w = 0; w < words; w++) { uint64_t t = A[(size_t) p * words + w]; A[(size_t) p * words + w] = A[(size_t) row * words + w]; A[(size_t) row * words + w] = t; }
    for(int r = 0; r < neq; r++) if(r != row && ((A[(size_t) r * words + c / 64] >> (c % 64)) & 1))
      for(int w = 0; w < words; w++) A[(size_t) r * words + w] ^= A[(size_t) row * words + w];
    pivrow[c] = row; row++;
  }
  int rank = row, consistent = 1;
  for(int r = rank; r < neq; r++) if((A[(size_t) r * words + nv / 64] >> (nv % 64)) & 1) consistent = 0;
  int fullrank = (rank == nv);
  memset(xout, 0, sizeof(bv) * N);
  for(int a = 0; a < Sx->dim; a++) for(int i = 0; i < N; i++) {
    int c = a * N + i; int r = pivrow[c];
    if(r >= 0 && ((A[(size_t) r * words + nv / 64] >> (nv % 64)) & 1)) bv_xor(&xout[i], &Sx->b[a]);
  }
  printf("  linear system: %d eqs, %d unknowns, rank %d, consistent=%d\n", neq, nv, rank, consistent);
  free(A); free(col); free(pivrow);
  return consistent && fullrank;
}

static int rank_of(const bv *v, int n) { sp t; t.dim = 0; for(int i = 0; i < n; i++) sp_add(&t, v[i]); return t.dim; }

/* KEM key derivation replicated from public spec (ccakem.c) */
static void xof_dom(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen, uint8_t dom) {
  uint8_t *b = malloc(inlen + 1); memcpy(b, in, inlen); b[inlen] = dom;
  pseudoXOF(8ULL * outlen, b, 8ULL * (inlen + 1), out); free(b);
}

int main(int argc, char **argv) {
  uint64_t seed = argc > 1 ? strtoull(argv[1], 0, 0) : 1;
  if(argc > 2) tg_eff = atoi(argv[2]);
  setvbuf(stdout, NULL, _IOLBF, 0);
  rng_s[0] = seed * 0x9E3779B97F4A7C15ULL + 1; rng_s[1] = seed ^ 0xD1B54A32D192ED03ULL;
  for(int i = 0; i < 20; i++) rnd64();
  ffi_field_init();
  time_t t0 = time(NULL);
  /* ===== honest key generation (oracle side) ===== */
  uint8_t kseed[CCAKEM_SK_SIZE];
  for(int i = 0; i < CCAKEM_SK_SIZE; i++) kseed[i] = (uint8_t) rnd64();
  ccakem_keygen(PK, SK, kseed);
  printf("BAG-Piglet-%d: m=%d n=%d n'=%d eps=%d t_gab=%d s_x=%d s_y1=%d\n", PARAM_SECURITY, M, N, NP, EPS, TG, SX, SY);
  /* ===== attack (pk + decapsulation oracle only) ===== */
  parse_pk();
  make_reference();
  /* control: honest ciphertext decaps != R0 */
  sp Sx, Ty;
  mode_y = 0; s_dim = SX;
  printf("phase 1: support of x\n"); recover_support(&Sx);
  long long q1 = nqueries;
  mode_y = 1; s_dim = SY;
  printf("phase 2: support of y1\n"); recover_support(&Ty);
  long long q2 = nqueries - q1;
  printf("phase 3: solve s1 = x*h1 + y1\n");
  bv xr[N];
  int ok = solve_x(&Sx, &Ty, xr);
  bv chk[N]; ring_mul(chk, xr, H2); for(int i = 0; i < N; i++) bv_xor(&chk[i], &S2[i]);
  printf("  rank(x)=%d, rank(s2 + x*h2)=%d (expect %d)\n", rank_of(xr, N), rank_of(chk, N), WEIGHT_Y2_VALUES);
  /* ===== phase 4: decrypt an honest challenge with recovered x ===== */
  uint8_t Kstar[CCAKEM_KEY_SIZE], ctstar[CCAKEM_CT_SIZE];
  ccakem_encaps(Kstar, ctstar, PK, NULL);
  uint8_t pkesk[CPAPKE_SK_SIZE]; memcpy(pkesk, PK, SEEDB);
  { ffi_vec X = {0}; bv_to_ffivec(&X, xr, N); ffi_vec_to_string_compact(pkesk + SEEDB, &X, N); ffi_vec_clear(&X); }
  uint8_t msg[CPAPKE_PT_SIZE]; bag_piglet_pke_decrypt(msg, ctstar, pkesk);
  uint8_t id[PARAMS_ID_SIZE]; xof_dom(id, sizeof id, PK, CCAKEM_PK_SIZE, 0x02);
  uint8_t in[PARAMS_ID_SIZE + CPAPKE_PT_SIZE + PARAMS_SALT_SIZE];
  memcpy(in, id, sizeof id); memcpy(in + sizeof id, msg, CPAPKE_PT_SIZE); memcpy(in + sizeof id + CPAPKE_PT_SIZE, ctstar + CPAPKE_CT_SIZE, PARAMS_SALT_SIZE);
  uint8_t kr[PARAMS_PREKEY_SIZE + PARAMS_RAND_SIZE]; xof_dom(kr, sizeof kr, in, sizeof in, 0x03);
  uint8_t kin[PARAMS_PREKEY_SIZE + CPAPKE_CT_SIZE]; memcpy(kin, kr, PARAMS_PREKEY_SIZE); memcpy(kin + PARAMS_PREKEY_SIZE, ctstar, CPAPKE_CT_SIZE);
  uint8_t Katt[CCAKEM_KEY_SIZE]; xof_dom(Katt, sizeof Katt, kin, sizeof kin, 0x04);
  int win = memcmp(Katt, Kstar, CCAKEM_KEY_SIZE) == 0;
  printf("RESULT: queries phase1=%lld phase2=%lld total=%lld; linear-system ok=%d; challenge key recovered=%s; wall=%lds\n",
         q1, q2, nqueries, ok, win ? "YES" : "NO", (long)(time(NULL) - t0));
  /* ===== VALIDATION ONLY (uses SK): compare with the true x ===== */
  {
    uint8_t exp[SEEDB + PARAMS_PREKEY_SIZE]; xof_dom(exp, sizeof exp, SK, CCAKEM_SK_SIZE, 0x01);
    uint8_t pk2[CPAPKE_PK_SIZE], psk[CPAPKE_SK_SIZE]; bag_piglet_pke_keygen(pk2, psk, exp);
    bv xt[N]; for(int i = 0; i < N; i++) xt[i] = get_elt(psk + SEEDB, i);
    int same = 1; for(int i = 0; i < N; i++) if(!bv_eq(&xt[i], &xr[i])) same = 0;
    sp St; St.dim = 0; for(int i = 0; i < N; i++) sp_add(&St, xt[i]);
    int supp_ok = (St.dim == Sx.dim); for(int i = 0; i < St.dim; i++) if(!sp_contains(&Sx, St.b[i])) supp_ok = 0;
    printf("VALIDATION: recovered support == true supp(x): %s; recovered x == true x: %s\n", supp_ok ? "yes" : "no", same ? "yes" : "no");
  }
  return win ? 0 : 1;
}
