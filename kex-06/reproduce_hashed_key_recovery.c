/*
 * MAMBA-NIKE static-key recovery through the DEPLOYED (hashed) responder API.
 *
 * Victim: unmodified reference kex_init_b / kex_derive_ss_b (KEX_AlgorithmInstance.c,
 * nike.c) built WITHOUT -DSTATISTICAL_TEST, i.e. the responder outputs only
 *   K_B = SHAKE256("MAMBA-NIKE-KDF20" || nu || pk_B || M1).
 *
 * Oracle models (the attacker never reads skb; it only submits M1):
 *   MODEL_REVEAL (default): the attacker learns K_B of a session it initiated
 *     (CK/eCK SessionKeyReveal on a non-test session, or any offline-checkable use
 *     of K_B such as a key-confirmation MAC).
 *   MODEL_EQ (-e): the attacker submits (M1, K_guess) and learns only [K_B == K_guess]
 *     (pure key-confirmation / key-mismatch bit).
 *
 * Method: uhat = c*X^0 + small residual (dither), so v = s*uhat has v_j = c*s_j + noise_j.
 * Reconciliation group g sees (s_g, s_{g+k}, s_{g+2k}, s_{g+3k}); the attacker's 2-bit
 * helper pattern per group turns each nu-bit into a threshold predicate of that 4-tuple.
 * Non-target groups get helpers that make their bit predictable from current knowledge;
 * J target groups per query get informative helpers.  Bit vectors are tested through the
 * KDF (2^J offline hashes per query in MODEL_REVEAL, one guess in MODEL_EQ).  A Bayesian
 * posterior per group over (2eta+1)^4 tuples is updated with Gaussian-linearised noise.
 * Recovery is verified exactly against pk_B, then used to compute an honest session key.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include "params.h"
#include "poly.h"
#include "fips202.h"
#include "error_correction.h"
#include "KEX_AlgorithmInstance.h"

#define N PARAM_N
#define Q PARAM_Q
#define ETA PARAM_K
#define G (N/4)
#define B (2*ETA+1)
#define NT (B*B*B*B)
#define DU PARAM_DELTA_U

/* ---------------- victim side (only touched through the API) ---------------- */
static unsigned char V_skb[NIKE_SKBYTES], V_pkb[NIKE_SENDABYTES];
static unsigned char A_ska[NIKE_SKBYTES], A_pka[NIKE_SENDABYTES];
static long long n_oracle = 0;
static int oracle_reveal(const unsigned char *m1, unsigned char *ss)
{
  unsigned long long l = 0;
  n_oracle++;
  return kex_derive_ss_b(V_skb, NIKE_SKBYTES, A_pka, NIKE_SENDABYTES,
                         (unsigned char *)m1, NIKE_SENDBBYTES, NULL, 0, ss, &l);
}

/* ---------------- attacker replicas of public functions ---------------- */
static uint64_t rng_s[2];
static uint64_t xrand(void)
{ uint64_t s1 = rng_s[0]; const uint64_t s0 = rng_s[1]; rng_s[0] = s0; s1 ^= s1 << 23;
  rng_s[1] = s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26); return rng_s[1] + s0; }

static void dither(int *r, const unsigned char *seed, unsigned char domain, unsigned delta)
{
  unsigned char ext[NIKE_SEEDBYTES + 1], buf[SHAKE128_RATE];
  uint64_t st[25]; unsigned pos = SHAKE128_RATE; int i;
  ext[0] = domain; memcpy(ext + 1, seed, NIKE_SEEDBYTES);
  shake128_absorb(st, ext, sizeof ext);
  for (i = 0; i < N; i++) {
    if (pos > SHAKE128_RATE - 2) { shake128_squeezeblocks(buf, 1, st); pos = 0; }
    r[i] = (buf[pos] | ((unsigned)buf[pos + 1] << 8)) & (delta - 1); pos += 2;
  }
}
static void pack_bits(unsigned char *out, const int *p, unsigned bits, int nbytes)
{
  unsigned acc = 0, ab = 0, op = 0, mask = (1u << bits) - 1; int i;
  memset(out, 0, nbytes);
  for (i = 0; i < N; i++) {
    acc |= ((unsigned)p[i] & mask) << ab; ab += bits;
    while (ab >= 8) { out[op++] = acc & 0xff; acc >>= 8; ab -= 8; }
  }
  if (ab) out[op] = acc & 0xff;
}
static void unpack_bits(int *p, const unsigned char *in, unsigned bits)
{
  unsigned acc = 0, ab = 0, ip = 0, mask = (1u << bits) - 1; int i;
  for (i = 0; i < N; i++) {
    while (ab < bits) { acc |= ((unsigned)in[ip++]) << ab; ab += 8; }
    p[i] = acc & mask; acc >>= bits; ab -= bits;
  }
}
static void kdf(unsigned char *out, const unsigned char *nu, const unsigned char *pk,
                const unsigned char *m1)
{
  static unsigned char in[NIKE_KDF_INPUTBYTES];
  static const char dom[16] = {'M','A','M','B','A','-','N','I','K','E','-','K','D','F','2','0'};
  memcpy(in, dom, 16); memcpy(in + 16, nu, NIKE_KEYBYTES);
  memcpy(in + 16 + NIKE_KEYBYTES, pk, NIKE_SENDABYTES);
  memcpy(in + 16 + NIKE_KEYBYTES + NIKE_SENDABYTES, m1, NIKE_SENDBBYTES);
  shake256(out, NIKE_SSBYTES, in, NIKE_KDF_INPUTBYTES);
}
/* exact replicas of error_correction.c g() for margin computation */
static int32_t gfun(int32_t x, int *slope)
{
  int32_t t = x >> (LOG2Q + 2), c = t & 1; t = (t >> 1) + c; t *= 8 * Q;
  *slope = (x - t) >= 0 ? 1 : -1; return x - t >= 0 ? x - t : t - x;
}

/* ---------------- attacker state ---------------- */
static double *P[G];              /* posterior over tuples, per group */
static double prior1[B];
static int tup[NT][4];
static int known[G];
static double Es[N], Vs[N];       /* marginal mean/var per coefficient */
static long long n_hash = 0;
static int debug = 0;            /* DIAGNOSTIC ONLY: reads the victim secret to check calibration; never used in decisions */
static int dbg_s[N];
static int dbg_tuple(int g) { int k, t = 0, m = 1; for (k = 0; k < 4; k++) { t += (dbg_s[g + k * G] + ETA) * m; m *= B; } return t; }

static void marginals(int g)
{
  int k, t; double m[4] = {0}, m2[4] = {0};
  for (t = 0; t < NT; t++) { double p = P[g][t]; if (p < 1e-300) continue;
    for (k = 0; k < 4; k++) { m[k] += p * tup[t][k]; m2[k] += p * tup[t][k] * tup[t][k]; } }
  for (k = 0; k < 4; k++) { Es[g + k * G] = m[k]; Vs[g + k * G] = m2[k] - m[k] * m[k]; if (Vs[g+k*G] < 0) Vs[g+k*G] = 0; }
}
static double maxp(int g, int *arg)
{ double b = -1; int t, a = 0; for (t = 0; t < NT; t++) if (P[g][t] > b) { b = P[g][t]; a = t; } if (arg) *arg = a; return b; }

/* Per-query design: c = uhat_0.  tmp0[a][o] = 16q + 8*(c*a mod q) - q*o (exact, noise-free). */
typedef struct { int32_t tmp0[B][10]; } gtab_t;
static void build_gtab(gtab_t *T, int c)
{
  int a, o;
  for (a = -ETA; a <= ETA; a++) for (o = 0; o < 10; o++) {
    int32_t v = ((c * a) % Q + Q) % Q;
    T->tmp0[a + ETA][o] = 16 * Q + 8 * v - Q * o;
  }
}
static inline void pat_offsets(int pat, int *o)
{ int c0 = pat & 3, c1 = (pat >> 2) & 3, c2 = (pat >> 4) & 3, c3 = (pat >> 6) & 3;
  o[0] = 2 * c0 + c3; o[1] = 2 * c1 + c3; o[2] = 2 * c2 + c3; o[3] = c3; }

static double Phi(double x) { return 0.5 * erfc(-x / M_SQRT2); }
static double H2(double p) { if (p <= 1e-15 || p >= 1 - 1e-15) return 0; return -p * log2(p) - (1 - p) * log2(1 - p); }
#define ZT 16.0
#define ZS 2000
static double phitab[2 * 16 * ZS + 2], h2tab[2 * 16 * ZS + 2];
static void init_tabs(void) { int i; for (i = 0; i <= 2 * 16 * ZS; i++) { double z = -ZT + (double)i / ZS; phitab[i] = Phi(z); h2tab[i] = H2(phitab[i]); } }
static inline int zidx(double z) { if (z <= -ZT) return 0; if (z >= ZT) return 2 * 16 * ZS; return (int)((z + ZT) * ZS + 0.5); }
#define SD2_FLOOR 16.0

/* folded-normal moments of |m + N(0,v)| */
static inline void foldmom(double m, double v, double *mean, double *var)
{
  double s = sqrt(v), am = fabs(m);
  if (am > 8.5 * s) { *mean = am; *var = v; return; }
  double fm = s * 0.7978845608028654 * exp(-m * m / (2 * v)) + m * (1 - 2 * Phi(-m / s));
  *mean = fm; *var = m * m + v - fm * fm; if (*var < 1e-9) *var = 1e-9;
}
/* g(tmp0 + 8n) with n ~ N(mu, sd2): mean/var, exact kinks of g = dist(., 8qZ) handled by folding */
static inline void gmom(int32_t tmp0, double mu, double sd2, double *mean, double *var)
{
  double x = tmp0 + 8.0 * mu, v = 64.0 * sd2;
  double r = fmod(x, 8.0 * Q); if (r < 0) r += 8.0 * Q; if (r >= 4.0 * Q) r -= 8.0 * Q;   /* r in [-4q,4q) */
  if (fabs(r) < 2.0 * Q) foldmom(r, v, mean, var);
  else { double c = r > 0 ? 4.0 * Q : -4.0 * Q, fm, fv; foldmom(r - c, v, &fm, &fv); *mean = 4.0 * Q - fm; *var = fv; }
}
/* per-group moment tables for one pattern */
static inline void prep(const gtab_t *T, const int *o, const double *mu, const double *sd2, double FM[4][B], double FV[4][B])
{
  int k, a;
  for (k = 0; k < 4; k++) for (a = 0; a < B; a++) gmom(T->tmp0[a][o[k]], mu[k], sd2[k] + SD2_FLOOR, &FM[k][a], &FV[k][a]);
}
static inline double zfrom(const double FM[4][B], const double FV[4][B], int t)
{
  int a0 = tup[t][0] + ETA, a1 = tup[t][1] + ETA, a2 = tup[t][2] + ETA, a3 = tup[t][3] + ETA;
  double m = 8.0 * Q - FM[0][a0] - FM[1][a1] - FM[2][a2] - FM[3][a3];
  double v = FV[0][a0] + FV[1][a1] + FV[2][a2] + FV[3][a3];
  return m / sqrt(v);
}
static inline double lik1(const gtab_t *T, int t, const int *o, const double *mu, const double *sd2)
{
  double FM[4][B], FV[4][B]; prep(T, o, mu, sd2, FM, FV); return Phi(zfrom(FM, FV, t));
}
static inline double zval(const gtab_t *T, int t, const int *o, const double *mu, double isd)
{
  (void)isd; double FM[4][B], FV[4][B]; double sd2[4] = {0,0,0,0}; prep(T, o, mu, sd2, FM, FV); return zfrom(FM, FV, t);
}
/* evaluate pattern on group: returns P(bit=1) and mutual information */
static void eval_pat(const gtab_t *T, int g, int pat, const double *mu, const double *sd2, double *p1, double *info)
{
  int o[4], t; double s1 = 0, hc = 0, FM[4][B], FV[4][B];
  pat_offsets(pat, o); prep(T, o, mu, sd2, FM, FV);
  for (t = 0; t < NT; t++) { double p = P[g][t]; if (p < 1e-14) continue;
    int zi = zidx(zfrom(FM, FV, t)); s1 += p * phitab[zi]; if (info) hc += p * h2tab[zi]; }
  *p1 = s1; if (info) *info = H2(s1) - hc;
}

static int robust_pats[] = { 0x00, 0x2a, 0xff, 0xd5, 0x55, 0xaa };

int main(int argc, char **argv)
{
  int J = 12, model_eq = 0, verbose = 0, i, g, k, t;
  unsigned long long seed = 1;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-e")) model_eq = 1;
    else if (!strcmp(argv[i], "-J")) J = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-s")) seed = strtoull(argv[++i], 0, 10);
    else if (!strcmp(argv[i], "-v")) verbose = 1; else if (!strcmp(argv[i], "-vv")) verbose = 2;
    else if (!strcmp(argv[i], "-d")) debug = 1;
  }
  if (model_eq) J = 1;
  setvbuf(stdout, NULL, _IONBF, 0);
  init_tabs();
  rng_s[0] = seed * 0x9E3779B97F4A7C15ULL + 1; rng_s[1] = seed ^ 0xD1B54A32D192ED03ULL; for (i = 0; i < 20; i++) xrand();

  /* victim key pair (fresh OS randomness inside the unmodified API) and an attacker pair */
  { unsigned long long a, b, c; unsigned char stb[1], sta[NIKE_SSBYTES];
    if (kex_init_b(V_pkb, &a, V_skb, &b, stb, &c) != 0) { puts("init_b failed"); return 1; }
    if (kex_init_a(A_pka, &a, A_ska, &b, sta, &c) != 0) { puts("init_a failed"); return 1; } }
  clock_t t0 = clock();
  if (debug) { for (i = 0; i < N; i++) { int v = V_skb[2*i] | (V_skb[2*i+1] << 8); dbg_s[i] = v > Q/2 ? v - Q : v; } }

  /* tuples and prior */
  { double tot = 0; for (k = 0; k <= 2 * ETA; k++) { double c = 1; int j; for (j = 0; j < k; j++) c = c * (2 * ETA - j) / (j + 1); prior1[k] = c; tot += c; }
    for (k = 0; k < B; k++) prior1[k] /= tot; }
  for (t = 0; t < NT; t++) { int x = t; for (k = 0; k < 4; k++) { tup[t][k] = x % B - ETA; x /= B; } }
  for (g = 0; g < G; g++) { P[g] = malloc(sizeof(double) * NT);
    for (t = 0; t < NT; t++) P[g][t] = prior1[tup[t][0] + ETA] * prior1[tup[t][1] + ETA] * prior1[tup[t][2] + ETA] * prior1[tup[t][3] + ETA];
    marginals(g); known[g] = 0; }

  /* c menu: scale c' = 8c/q */
  double cmenu_f[] = {0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.2, 1.5, 1.8, 2.0, 2.5, 3.0};
  int NC = sizeof cmenu_f / sizeof cmenu_f[0];

  static int du[N], dvv[N], ulab[N], uhat[N];
  static double mu_n[N], sd2_n[N];
  static int bestpat[G], predbit[G];

  static unsigned char m1[NIKE_SENDBBYTES], ss[NIKE_SSBYTES], kk[NIKE_SSBYTES], nu[NIKE_KEYBYTES];
  int *tg = malloc(sizeof(int) * G), *tpat = malloc(sizeof(int) * G);
  long long nq = 0, ndiscard = 0;
  int nknown = 0, verified = 0, rounds = 0;
  double eps_total = model_eq ? 1e-5 : 1e-4;
  static double gent[G];

  while (!verified) {
    while (nknown < G) {
      unsigned char mu[NIKE_SEEDBYTES];
      for (i = 0; i < NIKE_SEEDBYTES; i++) mu[i] = xrand() & 0xff;
      dither(du, mu, 0xB0, PARAM_DELTA_U);
      /* residual labels for i>=1: target 0 */
      for (i = 1; i < N; i++) { int T = 0; ulab[i] = ((T + du[i] + DU / 2) >> PARAM_H_U) & (PARAM_P_U - 1);
        int uh = ((ulab[i] << PARAM_H_U) - du[i]) & (Q - 1); uhat[i] = uh >= Q / 2 ? uh - Q : uh; }
      /* noise: n_k = sum_{i>=1} uhat_i * sgn * s_{k-i} */
      for (k = 0; k < N; k++) { double m = 0, v = 0; int ii;
        for (ii = 1; ii < N; ii++) { int j = k - ii; double sg = 1; if (j < 0) { j += N; sg = -1; }
          double u = uhat[ii]; m += sg * u * Es[j]; v += u * u * Vs[j]; }
        mu_n[k] = m; sd2_n[k] = v; }
      for (g = 0; g < G; g++) { double h = 0; for (t = 0; t < NT; t++) { double p = P[g][t]; if (p > 1e-300) h -= p * log2(p); } gent[g] = known[g] ? -1 : h; }

      double bestinfo = -1; int bestc = -1, bestJ = 0;

      static int cache_pat[64][G]; static int cache_init = 0;
      if (!cache_init) { for (i = 0; i < 64; i++) for (g = 0; g < G; g++) cache_pat[i][g] = 0; cache_init = 1; }
      int tg_c[4096], tpat_c[4096];
      /* candidate targets: highest-entropy unknown groups */
      int order_n = 0; static int order[G];
      for (g = 0; g < G; g++) if (!known[g]) order[order_n++] = g;
      for (i = 0; i < order_n && i < J + 8; i++) { int b = i; for (k = i + 1; k < order_n; k++) if (gent[order[k]] > gent[order[b]]) b = k; int tmp = order[i]; order[i] = order[b]; order[b] = tmp; }
      int ncand = order_n < J ? order_n : J;
      static char istarget[G]; memset(istarget, 0, sizeof istarget);
      for (i = 0; i < ncand; i++) istarget[order[i]] = 1;
      /* feasibility of each c for the non-targets */
      int feas[64], nfeas = 0;
      static int fpat[64][G], fbit[64][G];
      for (int ci = 0; ci < NC; ci++) {
        int c = (int)lround(cmenu_f[ci] * Q / 8.0);
        int u0 = ((c + du[0] + DU / 2) >> PARAM_H_U) & (PARAM_P_U - 1);
        int c_act = ((u0 << PARAM_H_U) - du[0]) & (Q - 1); if (c_act >= Q / 2) c_act -= Q;
        gtab_t T; build_gtab(&T, c_act);
        double errsum = 0; int ok = 1;
        for (g = 0; g < G && ok; g++) {
          if (istarget[g]) continue;
          double mu4[4], sd4[4]; for (k = 0; k < 4; k++) { mu4[k] = mu_n[g + k * G]; sd4[k] = sd2_n[g + k * G]; }
          double p1; eval_pat(&T, g, cache_pat[ci][g], mu4, sd4, &p1, 0);
          double be = p1 < 0.5 ? p1 : 1 - p1; int bp = cache_pat[ci][g], bb = p1 >= 0.5;
          if (be > eps_total / (4.0 * G)) {
            int cnt = 0; for (t = 0; t < NT; t++) if (P[g][t] > 1e-14) cnt++;
            if (cnt <= 64) { for (int pat = 0; pat < 256; pat++) { eval_pat(&T, g, pat, mu4, sd4, &p1, 0); double e = p1 < 0.5 ? p1 : 1 - p1; if (e < be) { be = e; bp = pat; bb = p1 >= 0.5; } } }
            else { for (int r = 0; r < (int)(sizeof robust_pats / sizeof(int)); r++) { eval_pat(&T, g, robust_pats[r], mu4, sd4, &p1, 0); double e = p1 < 0.5 ? p1 : 1 - p1; if (e < be) { be = e; bp = robust_pats[r]; bb = p1 >= 0.5; } } }
            cache_pat[ci][g] = bp;
          }
          fpat[ci][g] = bp; fbit[ci][g] = bb; errsum += be; if (errsum > eps_total) ok = 0;
        }
        if (ok) feas[nfeas++] = ci; else if (verbose>1) { printf("c%d infeasible at g=%d errsum=%g\n", ci, g, errsum); }
      }
      /* evaluate target information on a subset of feasible c */
      int sub[8], nsub = 0;
      if (nfeas > 0) { sub[nsub++] = feas[nfeas - 1]; if (nfeas > 1) sub[nsub++] = feas[nfeas - 2];
        for (i = 1; i <= 4 && nfeas > 2; i++) { int idx = (int)((long)(nfeas - 2) * i / 5); int dup = 0; for (k = 0; k < nsub; k++) if (sub[k] == feas[idx]) dup = 1; if (!dup) sub[nsub++] = feas[idx]; } }
      for (int si = 0; si < nsub; si++) {
        int ci = sub[si];
        int c = (int)lround(cmenu_f[ci] * Q / 8.0);
        int u0 = ((c + du[0] + DU / 2) >> PARAM_H_U) & (PARAM_P_U - 1);
        int c_act = ((u0 << PARAM_H_U) - du[0]) & (Q - 1); if (c_act >= Q / 2) c_act -= Q;
        gtab_t T; build_gtab(&T, c_act);
        int nt = 0; double info_sum = 0;
        for (i = 0; i < ncand; i++) {
          int gg = order[i]; double mu4[4], sd4[4]; for (k = 0; k < 4; k++) { mu4[k] = mu_n[gg + k * G]; sd4[k] = sd2_n[gg + k * G]; }
          double bi = -1, bp1 = 0.5; int bp = 0;
          for (int pat = 0; pat < 256; pat++) { double p1, inf; eval_pat(&T, gg, pat, mu4, sd4, &p1, &inf); if (inf > bi) { bi = inf; bp = pat; bp1 = p1; } }
          if (bi < 0.02) { /* uninformative: use as a predicted group instead */ tg_c[nt] = -1; continue; }
          (void)bp1; tg_c[nt] = gg; tpat_c[nt] = bp; nt++; info_sum += bi;
        }
        if (nt == 0) continue;
        if (info_sum > bestinfo) { bestinfo = info_sum; bestc = ci; bestJ = nt;
          for (g = 0; g < G; g++) { bestpat[g] = fpat[ci][g]; predbit[g] = fbit[ci][g]; }
          for (i = 0; i < nt; i++) { tg[i] = tg_c[i]; tpat[i] = tpat_c[i]; } }
      }
      /* candidate groups that were not used as targets need a predicted pattern */
      if (bestc >= 0) {
        int ci = bestc; int c = (int)lround(cmenu_f[ci] * Q / 8.0);
        int u0 = ((c + du[0] + DU / 2) >> PARAM_H_U) & (PARAM_P_U - 1);
        int c_act = ((u0 << PARAM_H_U) - du[0]) & (Q - 1); if (c_act >= Q / 2) c_act -= Q;
        gtab_t T; build_gtab(&T, c_act); double extra = 0;
        for (i = 0; i < ncand; i++) { int gg = order[i], used = 0; for (k = 0; k < bestJ; k++) if (tg[k] == gg) used = 1; if (used) continue;
          double mu4[4], sd4[4], p1, be = 2; int bp = 0, bb = 0; for (k = 0; k < 4; k++) { mu4[k] = mu_n[gg + k * G]; sd4[k] = sd2_n[gg + k * G]; }
          for (int pat = 0; pat < 256; pat++) { eval_pat(&T, gg, pat, mu4, sd4, &p1, 0); double e = p1 < 0.5 ? p1 : 1 - p1; if (e < be) { be = e; bp = pat; bb = p1 >= 0.5; } }
          bestpat[gg] = bp; predbit[gg] = bb; extra += be; }
        (void)extra;
      }
      if (bestc < 0 && nsub < nfeas && !(model_eq && 0)) {
        /* fallback: all feasible c, low info threshold */
        for (int si = 0; si < nfeas; si++) {
          int ci = feas[si];
          int c = (int)lround(cmenu_f[ci] * Q / 8.0);
          int u0 = ((c + du[0] + DU / 2) >> PARAM_H_U) & (PARAM_P_U - 1);
          int c_act = ((u0 << PARAM_H_U) - du[0]) & (Q - 1); if (c_act >= Q / 2) c_act -= Q;
          gtab_t T; build_gtab(&T, c_act);
          int nt = 0; double info_sum = 0;
          for (i = 0; i < ncand; i++) {
            int gg = order[i]; double mu4[4], sd4[4]; for (k = 0; k < 4; k++) { mu4[k] = mu_n[gg + k * G]; sd4[k] = sd2_n[gg + k * G]; }
            double bi = -1; int bp = 0;
            for (int pat = 0; pat < 256; pat++) { double p1, inf; eval_pat(&T, gg, pat, mu4, sd4, &p1, &inf); if (inf > bi) { bi = inf; bp = pat; } }
            if (bi < 1e-4) continue;
            tg_c[nt] = gg; tpat_c[nt] = bp; nt++; info_sum += bi;
          }
          if (nt == 0) continue;
          if (info_sum > bestinfo) { bestinfo = info_sum; bestc = ci; bestJ = nt;
            for (g = 0; g < G; g++) { bestpat[g] = fpat[ci][g]; predbit[g] = fbit[ci][g]; }
            for (i = 0; i < nt; i++) { tg[i] = tg_c[i]; tpat[i] = tpat_c[i]; } }
        }
      }
      if (bestc < 0) {
        if (verbose) printf("no informative feasible design (nknown=%d nfeas=%d); stuck candidates:\n", nknown, nfeas);
        for (i = 0; i < ncand && i < 3 && verbose > 1; i++) { int gg = order[i]; printf(" g=%d H=%.3f:", gg, gent[gg]);
          for (t = 0; t < NT; t++) if (P[gg][t] > 1e-3) printf(" (%d,%d,%d,%d):%.3f", tup[t][0], tup[t][1], tup[t][2], tup[t][3], P[gg][t]); printf("\n"); }
        /* accept MAP for stuck candidates; pk verification will catch errors */
        for (i = 0; i < ncand; i++) { known[order[i]] = 1; nknown++; }
        continue;
      }
      /* build M1 */
      int c = (int)lround(cmenu_f[bestc] * Q / 8.0);
      ulab[0] = ((c + du[0] + DU / 2) >> PARAM_H_U) & (PARAM_P_U - 1);
      int c_act = ((ulab[0] << PARAM_H_U) - du[0]) & (Q - 1); if (c_act >= Q / 2) c_act -= Q;
      gtab_t T; build_gtab(&T, c_act);
      for (i = 0; i < bestJ; i++) bestpat[tg[i]] = tpat[i];
      memcpy(m1, mu, NIKE_SEEDBYTES);
      pack_bits(m1 + NIKE_SEEDBYTES, ulab, PARAM_T_U, NIKE_UPOLYBYTES);
      for (g = 0; g < G; g++) { int p = bestpat[g]; m1[NIKE_SEEDBYTES + NIKE_UPOLYBYTES + g] = (unsigned char)((p & 3) | (((p >> 2) & 3) << 2) | (((p >> 4) & 3) << 4) | (((p >> 6) & 3) << 6)); }
      /* oracle */
      int obs = -1; unsigned long combo = 0;
      memset(nu, 0, sizeof nu);
      for (g = 0; g < G; g++) nu[g >> 3] |= predbit[g] << (g & 7);
      if (!model_eq) {
        oracle_reveal(m1, ss);
        for (combo = 0; combo < (1UL << bestJ); combo++) {
          for (i = 0; i < bestJ; i++) { int gg = tg[i]; nu[gg >> 3] &= ~(1 << (gg & 7)); nu[gg >> 3] |= ((combo >> i) & 1) << (gg & 7); }
          kdf(kk, nu, V_pkb, m1); n_hash++;
          if (!memcmp(kk, ss, NIKE_SSBYTES)) { obs = 1; break; }
        }
      } else {
        /* single guess: target bit = its more likely value */
        int gg = tg[0]; double mu4[4], sd4[4], p1; for (k = 0; k < 4; k++) { mu4[k] = mu_n[gg + k * G]; sd4[k] = sd2_n[gg + k * G]; }
        eval_pat(&T, gg, tpat[0], mu4, sd4, &p1, 0);
        int guess = p1 >= 0.5; nu[gg >> 3] &= ~(1 << (gg & 7)); nu[gg >> 3] |= guess << (gg & 7);
        kdf(kk, nu, V_pkb, m1); n_hash++;
        oracle_reveal(m1, ss);
        combo = memcmp(kk, ss, NIKE_SSBYTES) ? !guess : guess; obs = 1;
      }
      nq++;
      if (debug) { /* compare predicted non-target bits to truth via exact rec on the true secret */
        poly sp, bp, v, cp; unsigned char tnu[NIKE_KEYBYTES]; static int dd[N];
        for (i = 0; i < N; i++) sp.coeffs[i] = (uint16_t)((dbg_s[i] + Q) & (Q - 1));
        dither(dd, mu, 0xB0, PARAM_DELTA_U);
        for (i = 0; i < N; i++) bp.coeffs[i] = (uint16_t)(((ulab[i] << PARAM_H_U) - dd[i]) & (Q - 1));
        for (g = 0; g < G; g++) { int p = bestpat[g]; cp.coeffs[4*g] = p & 3; cp.coeffs[4*g+1] = (p>>2)&3; cp.coeffs[4*g+2] = (p>>4)&3; cp.coeffs[4*g+3] = (p>>6)&3; }
        poly_pointwise(&v, &sp, &bp); rec(tnu, &v, &cp);
        for (g = 0; g < G; g++) { int istg = 0; for (i = 0; i < bestJ; i++) if (tg[i] == g) istg = 1; if (istg) continue;
          int tb = (tnu[g >> 3] >> (g & 7)) & 1; if (tb != predbit[g]) { double mu4[4], sd4[4], p1; for (k = 0; k < 4; k++) { mu4[k] = mu_n[g + k * G]; sd4[k] = sd2_n[g + k * G]; }
            gtab_t T2; build_gtab(&T2, c_act); eval_pat(&T2, g, bestpat[g], mu4, sd4, &p1, 0);
            int o[4]; pat_offsets(bestpat[g], o); double isd = 1.0 / sqrt(64.0 * (sd4[0]+sd4[1]+sd4[2]+sd4[3]));
            double z = zval(&T2, dbg_tuple(g), o, mu4, isd);
            /* actual margin */
            int32_t M = 8 * Q; for (k = 0; k < 4; k++) { int sl; int32_t tmp = 16*Q + 8*(int32_t)v.coeffs[g + k*G] - Q*o[k]; M -= gfun(tmp, &sl); }
            double nreal[4]; for (k = 0; k < 4; k++) { int32_t sig = ((c_act * dbg_s[g+k*G]) % Q + Q) % Q; int32_t d = (int32_t)v.coeffs[g+k*G] - sig; d = ((d % Q) + Q) % Q; if (d > Q/2) d -= Q; nreal[k] = d; }
            printf("DBG mispredict q=%lld g=%d known=%d P1=%.3g ztrue=%.2f Mreal=%d noise=(%.0f,%.0f,%.0f,%.0f) mu=(%.0f,%.0f,%.0f,%.0f) sd=%.0f c=%d pat=%02x\n", nq, g, known[g], p1, z, M, nreal[0],nreal[1],nreal[2],nreal[3], mu4[0],mu4[1],mu4[2],mu4[3], sqrt(sd4[0]), c_act, bestpat[g]); } } }
      if (obs < 0) { ndiscard++; continue; }
      for (i = 0; i < bestJ; i++) {
        int gg = tg[i], bit = model_eq ? (int)combo : (int)((combo >> i) & 1), o[4];
        double mu4[4], sd4[4]; for (k = 0; k < 4; k++) { mu4[k] = mu_n[gg + k * G]; sd4[k] = sd2_n[gg + k * G]; }
        pat_offsets(tpat[i], o);
        double tot = 0, FM[4][B], FV[4][B]; prep(&T, o, mu4, sd4, FM, FV);
        for (t = 0; t < NT; t++) { if (P[gg][t] < 1e-300) { P[gg][t] = 0; continue; }
          double l = Phi(zfrom(FM, FV, t)); P[gg][t] *= bit ? l : 1 - l; if (P[gg][t] < 1e-300) P[gg][t] = 0; tot += P[gg][t]; }
        for (t = 0; t < NT; t++) { double pr = prior1[tup[t][0] + ETA] * prior1[tup[t][1] + ETA] * prior1[tup[t][2] + ETA] * prior1[tup[t][3] + ETA];
          P[gg][t] = (1 - 1e-12) * P[gg][t] / tot + 1e-12 * pr; }
        marginals(gg);
        if (maxp(gg, 0) > 1 - 1e-6) { known[gg] = 1; nknown++;
          if (debug) { int am; maxp(gg, &am); if (am != dbg_tuple(gg)) printf("DBG wrong-known g=%d map=(%d,%d,%d,%d) true=(%d,%d,%d,%d) Ptrue=%.3g q=%lld\n", gg, tup[am][0],tup[am][1],tup[am][2],tup[am][3], dbg_s[gg],dbg_s[gg+G],dbg_s[gg+2*G],dbg_s[gg+3*G], P[gg][dbg_tuple(gg)], nq); } }
      }
      if (verbose && nq % 10 == 0) printf("q=%lld known=%d c'=%.2f J=%d info=%.2f disc=%lld nfeas=%d t=%.1f\n", nq, nknown, cmenu_f[bestc], bestJ, bestinfo, ndiscard, nfeas, (double)(clock() - t0) / CLOCKS_PER_SEC);
    }
    /* assemble MAP secret and verify against pk_B */
    static int shat[N]; int arg;
    for (g = 0; g < G; g++) { maxp(g, &arg); for (k = 0; k < 4; k++) shat[g + k * G] = tup[arg][k]; }
    {
      poly a, sp, r; static int dpk[N], blab[N]; unsigned char rho[NIKE_SEEDBYTES]; int ok = 1;
      unpack_bits(blab, V_pkb, PARAM_T_PK); memcpy(rho, V_pkb + NIKE_PKPOLYBYTES, NIKE_SEEDBYTES);
      poly_uniform(&a, rho); dither(dpk, rho, 0xA0, PARAM_DELTA_PK);
      for (i = 0; i < N; i++) sp.coeffs[i] = (uint16_t)((shat[i] + Q) & (Q - 1));
      poly_pointwise(&r, &sp, &a);
      for (i = 0; i < N; i++) { unsigned lab = (((unsigned)r.coeffs[i] + dpk[i] + (1u << (PARAM_H_PK - 1))) >> PARAM_H_PK) & (PARAM_P_PK - 1); if ((int)lab != blab[i]) { ok = 0; break; } }
      rounds++;
      if (ok) verified = 1;
      else {
        /* reopen the least confident groups */
        int reopened = 0;
        for (int it = 0; it < 8; it++) { double lo = 2; int lg = -1; for (g = 0; g < G; g++) if (known[g]) { double m = maxp(g, 0); if (m < lo) { lo = m; lg = g; } }
          if (lg < 0) break; known[lg] = 0; nknown--; reopened++;
          for (t = 0; t < NT; t++) { double pr = prior1[tup[t][0] + ETA] * prior1[tup[t][1] + ETA] * prior1[tup[t][2] + ETA] * prior1[tup[t][3] + ETA]; P[lg][t] = 0.5 * P[lg][t] + 0.5 * pr; }
          marginals(lg); }
        /* mark it so the threshold is stricter next time */
        printf("verify failed (round %d), reopened %d groups\n", rounds, reopened);
        if (rounds > 50) { printf("FAIL\n"); return 3; }
      }
    }
    if (verified) {
      /* use recovered secret: compute an honest initiator's session key passively */
      unsigned char sta[NIKE_SSBYTES], hm1[NIKE_SENDBBYTES], ssa[NIKE_SSBYTES], mine[NIKE_SSBYTES]; unsigned long long l1, l2, l3;
      kex_generate_pass1_msg_a(A_ska, NIKE_SKBYTES, V_pkb, NIKE_SENDABYTES, sta, &l1, hm1, &l2);
      kex_derive_ss_a(A_ska, NIKE_SKBYTES, V_pkb, NIKE_SENDABYTES, NULL, 0, sta, NIKE_SSBYTES, ssa, &l3);
      { poly v, bp, cpoly, sp; static int dd[N], lab[N]; unsigned char hmu[NIKE_SEEDBYTES], hnu[NIKE_KEYBYTES];
        memcpy(hmu, hm1, NIKE_SEEDBYTES); unpack_bits(lab, hm1 + NIKE_SEEDBYTES, PARAM_T_U); dither(dd, hmu, 0xB0, PARAM_DELTA_U);
        for (i = 0; i < N; i++) bp.coeffs[i] = (uint16_t)(((lab[i] << PARAM_H_U) - dd[i]) & (Q - 1));
        for (i = 0; i < G; i++) { unsigned char b8 = hm1[NIKE_SEEDBYTES + NIKE_UPOLYBYTES + i]; cpoly.coeffs[4*i] = b8 & 3; cpoly.coeffs[4*i+1] = (b8 >> 2) & 3; cpoly.coeffs[4*i+2] = (b8 >> 4) & 3; cpoly.coeffs[4*i+3] = b8 >> 6; }
        for (i = 0; i < N; i++) sp.coeffs[i] = (uint16_t)((shat[i] + Q) & (Q - 1));
        poly_pointwise(&v, &sp, &bp); rec(hnu, &v, &cpoly); kdf(mine, hnu, V_pkb, hm1); }
      int sess_ok = !memcmp(mine, ssa, NIKE_SSBYTES);
      /* negative control: perturb one recovered coefficient; the exact pk check must then fail */
      int ctrl_rejected;
      { poly a, sp, r; static int dpk[N], blab[N]; unsigned char rho[NIKE_SEEDBYTES]; int ok = 1;
        unpack_bits(blab, V_pkb, PARAM_T_PK); memcpy(rho, V_pkb + NIKE_PKPOLYBYTES, NIKE_SEEDBYTES);
        poly_uniform(&a, rho); dither(dpk, rho, 0xA0, PARAM_DELTA_PK);
        for (i = 0; i < N; i++) sp.coeffs[i] = (uint16_t)((shat[i] + (i == 7 ? 1 : 0) + Q) & (Q - 1));
        poly_pointwise(&r, &sp, &a);
        for (i = 0; i < N; i++) { unsigned lab = (((unsigned)r.coeffs[i] + dpk[i] + (1u << (PARAM_H_PK - 1))) >> PARAM_H_PK) & (PARAM_P_PK - 1); if ((int)lab != blab[i]) { ok = 0; break; } }
        ctrl_rejected = !ok; }
      /* control: a one-coefficient-wrong secret must fail the pk check */
      printf("RESULT MAMBA-NIKE-%d model=%s J=%d seed=%llu oracle_queries=%lld discarded=%lld offline_kdf=%lld verify_rounds=%d pk_verified=1 honest_session_key_match=%d perturbed_control_rejected=%d cpu=%.1fs\n",
             NIKE_LEVEL, model_eq ? "equality-bit" : "session-key-reveal", J, seed, n_oracle, ndiscard, n_hash, rounds, sess_ok, ctrl_rejected, (double)(clock() - t0) / CLOCKS_PER_SEC);
      return (sess_ok && ctrl_rejected) ? 0 : 4;
    }
  }
  return 0;
}
