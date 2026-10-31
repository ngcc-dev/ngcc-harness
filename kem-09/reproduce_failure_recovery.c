/* Cheetah256 decryption-failure key recovery: exact failure-conditioned sampling + EM recovery.
 *
 * Model (code-faithful, see KEM_Cheetah.c indcpa_encrypt/decrypt):
 *   Y_i = sum_l (e'_l * r_l)[i] - (s_l * f_l)[i] + e2[i] + ev[i],  i < 256 (Truncate to lambda)
 *   e'_l = e_l + e_pk,l = Decompress(Compress(A s + e)) - A s   (computed exactly from a real key)
 *   f_l  = e1_l + e_u,l,  e_u = 4 - (x mod 8) in {-3..4},  ev = 256 - (x mod 512) in {-255..256}
 *   decryption bit i flips iff |2Y_i + m_i| > 3840  (centered mod q=7681, then parity).
 * The attacker knows r, f (= e1 + e_u), e2, ev, m for its own honest ciphertexts, and learns only
 * "decapsulation output != K" (a failure), not the failing index or sign.
 * The true s and e' (sv, ep) are read only to sample failures (the simulated victim) and in
 * diagnostics printed against the truth; the attacker side uses the failure data, A and the
 * decompressed public key bpub, including in its public-key check.
 *
 * Failing ciphertexts are drawn from the exact conditional law  P(r,f,e2,ev | coefficient i0 flips)
 * by exponential tilting of the independent variables and rejection with prob exp(-theta*(Y-thr)).
 * (Independence of e_u/ev from the rest is the usual uniform-rounding model; the other 255
 *  coefficients are left untilted, i.e. the rare multi-coefficient failures are ignored.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define N 640
#ifndef K
#define K 2
#endif
#define Q 7681
#ifndef ETA
#define ETA 4
#endif
#ifndef LAM
#define LAM 256
#endif
#define EM ((1u<<ETA)-1)

static uint64_t rs[4];
static inline uint64_t rotl(const uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
static uint64_t rnd(void) { const uint64_t r = rotl(rs[1] * 5, 7) * 9; const uint64_t t = rs[1] << 17;
  rs[2] ^= rs[0]; rs[3] ^= rs[1]; rs[1] ^= rs[2]; rs[0] ^= rs[3]; rs[2] ^= t; rs[3] = rotl(rs[3], 45); return r; }
static double urand(void) { return (rnd() >> 11) * 0x1.0p-53; }
static int cbd(void) { uint64_t x = rnd(); int a = __builtin_popcountll(x & EM), b = __builtin_popcountll((x >> ETA) & EM); return a - b; }

/* negacyclic product, only first nout coefficients */
static void nmul(const int *a, const int *b, long *out, int nout) {
  for (int i = 0; i < nout; i++) out[i] = 0;
  for (int j = 0; j < N; j++) { if (!a[j]) continue;
    for (int t = 0; t < N; t++) { int i = j + t; long v = (long)a[j] * b[t];
      if (i < N) { if (i < nout) out[i] += v; } else { i -= N; if (i < nout) out[i] -= v; } } }
}
static int modq(long x) { x %= Q; if (x < 0) x += Q; return (int)x; }
static int cmodq(long x) { int y = modq(x); return y > Q / 2 ? y - Q : y; }

/* base distributions */
#define RV 0 /* r: CBD4 on [-4,4] */
#define FV 1 /* f = CBD4 + U{-3..4}: [-7,8] */
#define E2 2 /* CBD4 */
#define EV 3 /* U{-255..256} */
static double pr_cbd[2*ETA+1], pr_f[2*ETA+8];
static int lo_t[4] = {-ETA, -ETA-3, -ETA, -255}, sz_t[4] = {2*ETA+1, 2*ETA+8, 2*ETA+1, 512};
static double th2 = 0, wR = 0, wF = 0, tN = 0, BX = 1.0;
static double ntilt(int t, int x) { return t == RV ? th2 * wR * x * x : (t == FV ? th2 * wF * x * x : 0); }
static double basep(int t, int idx) { if (t == RV || t == E2) return pr_cbd[idx]; if (t == FV) return pr_f[idx]; return 1.0 / 512; }

/* tilted CDF tables for coefficient c in [-CM, CM] */
#define CM 20
static double *cdf[4][2 * CM + 1];
static double theta;
static void build_tables(double th) {
  theta = th;
  for (int t = 0; t < 4; t++) for (int c = -CM; c <= CM; c++) {
    double *p = cdf[t][c + CM]; if (!p) p = cdf[t][c + CM] = malloc(sizeof(double) * sz_t[t]);
    double mx = -1e300; for (int i = 0; i < sz_t[t]; i++) { double l = log(basep(t, i)) + th * c * (lo_t[t] + i) + ntilt(t, lo_t[t] + i); if (l > mx) mx = l; }
    double s = 0; for (int i = 0; i < sz_t[t]; i++) { s += exp(log(basep(t, i)) + th * c * (lo_t[t] + i) + ntilt(t, lo_t[t] + i) - mx); p[i] = s; }
    for (int i = 0; i < sz_t[t]; i++) p[i] /= s; }
}
static int tsample(int t, int c) { double *p = cdf[t][c + CM]; double u = urand(); int lo = 0, hi = sz_t[t] - 1;
  while (lo < hi) { int m = (lo + hi) / 2; if (p[m] < u) lo = m + 1; else hi = m; } return lo_t[t] + lo; }
static double tmean(int t, int c, double th) { double s = 0, z = 0; for (int i = 0; i < sz_t[t]; i++) { double w = basep(t, i) * exp(th * c * (lo_t[t] + i) + ntilt(t, lo_t[t] + i)); s += w * (lo_t[t] + i); z += w; } return s / z; }

/* secret */
static int sv[K][N], ep[K][N];  /* s and e' = e + e_pk */

typedef struct { int8_t r[K][N], f[K][N]; int16_t known[LAM]; uint8_t m[LAM]; int i0, sg; } fail_t;

/* coefficient of r_l[j] and f_l[j] in sg*Y_{i0} */
static void coefs(int i0, int sg, int ca[K][N], int cb[K][N]) {
  for (int l = 0; l < K; l++) for (int j = 0; j < N; j++) { int p = i0 - j, s = 1; if (p < 0) { p += N; s = -1; }
    ca[l][j] = sg * s * ep[l][p]; cb[l][j] = -sg * s * sv[l][p]; }
}
static long tries_total = 0;
static void sample_failure(fail_t *F) {
  static int ca[K][N], cb[K][N];
  int i0 = rnd() % LAM, sg = (rnd() & 1) ? 1 : -1;
  coefs(i0, sg, ca, cb);
  for (int i = 0; i < LAM; i++) F->m[i] = rnd() & 1;
  int thr = (sg > 0) ? 1921 - F->m[i0] : 1921;   /* sg*Y >= thr  <=> bit i0 flips */
  for (;;) {
    tries_total++;
    long y = 0; double nrm = 0;
    for (int l = 0; l < K; l++) for (int j = 0; j < N; j++) {
      int a = tsample(RV, ca[l][j]); int b = tsample(FV, cb[l][j]);
      F->r[l][j] = a; F->f[l][j] = b; y += (long)ca[l][j] * a + (long)cb[l][j] * b; nrm += wR * a * a + wF * b * b; }
    int e2 = tsample(E2, sg), ev = tsample(EV, sg); y += sg * e2 + sg * ev;
    if (y < thr || nrm < tN) continue;
    if (urand() >= exp(-theta * (y - thr) - th2 * (nrm - tN))) continue;
    for (int i = 0; i < LAM; i++) F->known[i] = (i == i0) ? (e2 + ev) : (cbd() + (int)(rnd() % 512) - 255);
    F->i0 = i0; F->sg = sg; return;
  }
}

static double *gconf; static int cmpconf(const void *x, const void *y) { double a = gconf[*(const int *)x], b = gconf[*(const int *)y]; return (a > b) - (a < b); }
/* Gaussian tail */
static double logQ(double x) { if (x < 5) return log(0.5 * erfc(x / sqrt(2))); return -x * x / 2 - log(x) - 0.5 * log(2 * M_PI) + log(1 - 1 / (x * x) + 3 / (x * x * x * x)); }

int main(int argc, char **argv) {
  int nf = argc > 1 ? atoi(argv[1]) : 3000; uint64_t seed = argc > 2 ? atoll(argv[2]) : 1; int iters = argc > 3 ? atoi(argv[3]) : 12; double dampbase = argc > 4 ? atof(argv[4]) : 0.3;
  rs[0] = seed * 0x9E3779B97F4A7C15ull + 1; rs[1] = 0xD1B54A32D192ED03ull ^ seed; rs[2] = 0x8CB92BA72F3D8DD7ull; rs[3] = 0x1234567ull + seed;
  for (int i = 0; i < 20; i++) rnd();
  for (int k = -ETA; k <= ETA; k++) { int c = 0; for (int x = 0; x < (1 << (2*ETA)); x++) if (__builtin_popcount(x & EM) - __builtin_popcount(x >> ETA) == k) c++; pr_cbd[k + ETA] = (double)c / (1 << (2*ETA)); }
  for (int i = 0; i < 2*ETA+8; i++) pr_f[i] = 0; for (int k = -ETA; k <= ETA; k++) for (int u = -3; u <= 4; u++) pr_f[k + u + ETA + 3] += pr_cbd[k + ETA] / 8;

  /* real key with real compression */
  static int A[K][K][N], e[K][N], bpub[K][N]; static long tmp[N];  /* bpub = Decompress(pk), public */
  for (int i = 0; i < K; i++) for (int j = 0; j < K; j++) for (int t = 0; t < N; t++) A[i][j][t] = rnd() % Q;
  for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) { sv[l][t] = cbd(); e[l][t] = cbd(); }
  int epkmin = 99, epkmax = -99;
  for (int i = 0; i < K; i++) { long as[N] = {0};
    for (int j = 0; j < K; j++) { nmul(A[i][j], sv[j], tmp, N); for (int t = 0; t < N; t++) as[t] += tmp[t]; }
    for (int t = 0; t < N; t++) { int b = modq(as[t] + e[i][t]); int c = b >> 3; int d = (c << 3) + 4; if (d >= Q - 1) d = Q - 1;
      bpub[i][t] = d; ep[i][t] = cmodq(d - as[t]); int epk = ep[i][t] - e[i][t]; if (epk < epkmin) epkmin = epk; if (epk > epkmax) epkmax = epk; } }
  double sigCS2 = 0; for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) sigCS2 += 2.0 * ep[l][t] * ep[l][t] + (2.0 + 5.25) * sv[l][t] * sv[l][t];
  printf("key: e_pk range [%d,%d], sigma_CS=%.1f (C-part), CM check\n", epkmin, epkmax, sqrt(sigCS2));
  for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) if (abs(ep[l][t]) > CM) { printf("CM too small\n"); return 1; }

  wR = ETA / 2.0 + 5.25; wF = ETA / 2.0; BX = argc > 5 ? atof(argv[5]) : 1.0;
  { double mean = 0; for (int i = 0; i < sz_t[RV]; i++) mean += K * N * wR * basep(RV, i) * (lo_t[RV] + i) * (lo_t[RV] + i);
    for (int i = 0; i < sz_t[FV]; i++) mean += K * N * wF * basep(FV, i) * (lo_t[FV] + i) * (lo_t[FV] + i);
    tN = (BX > 1.0) ? BX * mean : 0.0;   /* no norm conditioning when BX <= 1 */
    if (BX > 1.0) { double lo = 0, hi = 0.01;
      for (int it = 0; it < 60; it++) { th2 = (lo + hi) / 2; double m = 0;
        for (int t = RV; t <= FV; t++) { double z = 0, s2 = 0; for (int i = 0; i < sz_t[t]; i++) { int x = lo_t[t] + i; double w = basep(t, i) * exp(ntilt(t, x)); z += w; s2 += w * (t == RV ? wR : wF) * x * x; } m += K * N * s2 / z; }
        if (m < tN) lo = th2; else hi = th2; }
      th2 = (lo + hi) / 2; }
    printf("norm boost: select sigma_C^2 >= %.3f x mean (%.0f), th2=%.6f\n", BX, mean, th2); }
  /* theta: tilted mean of sg*Y = ~thr */
  { int ca[K][N], cb[K][N]; coefs(0, 1, ca, cb); double lo = 0, hi = 0.2;
    for (int it = 0; it < 50; it++) { double th = (lo + hi) / 2, m = tmean(E2, 1, th) + tmean(EV, 1, th);
      for (int l = 0; l < K; l++) for (int j = 0; j < N; j++) m += ca[l][j] * tmean(RV, ca[l][j], th) + cb[l][j] * tmean(FV, cb[l][j], th);
      if (m < 1921 + 20) lo = th; else hi = th; }
    build_tables((lo + hi) / 2); printf("theta=%.5f\n", theta); }

  fail_t *F = malloc(sizeof(fail_t) * nf);
  for (int k = 0; k < nf; k++) sample_failure(&F[k]);
  printf("sampled %d failures (%ld proposals)\n", nf, tries_total);
  { int okp=0, okm=0, np_=0, nm=0; static long y1[LAM], y2[LAM]; static int rr[N], ff[N];
    for (int k = 0; k < nf; k++) { double Y = F[k].known[F[k].i0];
      for (int l = 0; l < K; l++) { for (int t = 0; t < N; t++) { rr[t] = F[k].r[l][t]; ff[t] = F[k].f[l][t]; }
        nmul(ep[l], rr, y1, LAM); nmul(sv[l], ff, y2, LAM); Y += y1[F[k].i0] - y2[F[k].i0]; }
      int thr = (F[k].sg > 0) ? 1921 - F[k].m[F[k].i0] : 1921;
      if (F[k].sg > 0) { np_++; if (Y >= thr) okp++; } else { nm++; if (-Y >= thr) okm++; } }
    printf("oracle check: sg=+1 %d/%d satisfy, sg=-1 %d/%d satisfy\n", okp, np_, okm, nm); }
  { double mk = 0; for (int k = 0; k < nf; k++) mk += F[k].sg * F[k].known[F[k].i0]; printf("mean sg*(e2+ev) at failing index: %.1f\n", mk / nf); }

  /* ---- attacker side: EM over unknown failing index and sign, 2-fold cross-fitting ---- */
  double var_r = ETA / 2.0, var_f = ETA / 2.0 + 5.25;
  sigCS2 = BX * K * N * (var_r * (ETA / 2.0 + 5.25) + var_f * ETA / 2.0);   /* attacker model E||S||^2_Sigma */
  printf("attacker model sigma_CS=%.1f\n", sqrt(sigCS2));
  static double Sh[2][2][K][N], G[2][2][K][N];  /* [fold][0=e',1=s] */
  static int shi[2][K][N];
  double lam[2] = {0, 0}, tau2[2] = {sigCS2, sigCS2}, afold[2] = {0, 0};
  memset(Sh, 0, sizeof Sh);
  for (int it = 0; it <= iters; it++) {
    double damp = dampbase + (1.0 - dampbase) * (1.0 - pow(0.7, it));
    memset(G, 0, sizeof G);
    double ptrue = 0, sumpi2[2] = {0, 0}, tbar[2] = {0, 0}; int posp = 0, podd = 0, hard_ok = 0, cntf[2] = {0, 0}, pos_ok = 0, sign_ok = 0; static int offh[512], offs[2]; memset(offh,0,sizeof offh); memset(offs,0,sizeof offs);
    const double SC = 64.0;
    for (int fo = 0; fo < 2; fo++) {
      int src = 1 - fo;  /* estimate used for posteriors of fold fo */
      for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) { shi[0][l][t] = (int)lrint(SC * Sh[src][0][l][t]); shi[1][l][t] = (int)lrint(SC * Sh[src][1][l][t]); }
      for (int k = fo; k < nf; k += 2) {
        static long y1[LAM], y2[LAM]; static int rr[N], ff[N]; double yh[LAM];
        for (int i = 0; i < LAM; i++) yh[i] = 0;
        if (it > 0) for (int l = 0; l < K; l++) {
          for (int t = 0; t < N; t++) { rr[t] = F[k].r[l][t]; ff[t] = 2 * F[k].f[l][t] - 1; }
          nmul(shi[0][l], rr, y1, LAM); nmul(shi[1][l], ff, y2, LAM);
          for (int i = 0; i < LAM; i++) yh[i] += (y1[i] - 0.5 * y2[i]) / SC; }
        double lw[2 * LAM], mx = -1e300;
        for (int i = 0; i < LAM; i++) for (int s2 = 0; s2 < 2; s2++) { int sg = s2 ? -1 : 1; int thr = (sg > 0) ? 1921 - F[k].m[i] : 1921;
          double mu = sg * (lam[src] * yh[i] + F[k].known[i]); lw[2 * i + s2] = logQ((thr - mu) / sqrt(tau2[src])); if (lw[2 * i + s2] > mx) mx = lw[2 * i + s2]; }
        double z = 0; for (int i = 0; i < 2 * LAM; i++) { lw[i] = exp(lw[i] - mx); z += lw[i]; }
        int best = 0; for (int i = 0; i < 2 * LAM; i++) { lw[i] /= z; sumpi2[fo] += lw[i] * lw[i]; if (lw[i] > lw[best]) best = i; }
        int truth = 2 * F[k].i0 + (F[k].sg < 0);           /* diagnostics only */
        ptrue += lw[truth]; if (best == truth) hard_ok++; if (best/2 == F[k].i0) { pos_ok++; if (F[k].sg>0) posp++; if (k&1) podd++; } offh[(best/2 - F[k].i0 + 512)%512]++; offs[(best&1)!=(F[k].sg<0)]++; if ((best&1) == (F[k].sg<0)) sign_ok++;
        cntf[fo]++;
        for (int i = 0; i < LAM; i++) { double W = lw[2 * i] - lw[2 * i + 1]; if (fabs(W) < 1e-7) continue;
          tbar[fo] += lw[2 * i] * (1921 - F[k].known[i]) + lw[2 * i + 1] * (1921 + F[k].known[i]);
          for (int l = 0; l < K; l++) for (int p = 0; p < N; p++) { int j = i - p, s = 1; if (j < 0) { j += N; s = -1; }
            G[fo][0][l][p] += W * s * F[k].r[l][j]; G[fo][1][l][p] -= W * s * (F[k].f[l][j] - 0.5); } }
      }
    }
    for (int fo = 0; fo < 2; fo++) {
      double tb = tbar[fo] / cntf[fo], sc = sigCS2 / (cntf[fo] * tb), nn = 0;
      for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) { Sh[fo][0][l][t] = G[fo][0][l][t] / var_r * sc; Sh[fo][1][l][t] = G[fo][1][l][t] / var_f * sc;
        nn += var_r * Sh[fo][0][l][t] * Sh[fo][0][l][t] + var_f * Sh[fo][1][l][t] * Sh[fo][1][l][t]; }
      double noise = 2.0 * K * N * sc * sc * sumpi2[fo];
      double a2 = (nn - noise) / sigCS2; double a = a2 > 1e-4 ? sqrt(a2) : 0.01;
      afold[fo] = a; { double ae = a * damp; lam[fo] = ae * sigCS2 / nn; tau2[fo] = sigCS2 - ae * ae * sigCS2 * sigCS2 / nn; if (tau2[fo] < 100) tau2[fo] = 100; }
    }
    /* combined estimate and attacker-side rounding; truth only for diagnostics */
    double dot = 0, nrm = 0, tn = 0, ac = 0.5 * (afold[0] + afold[1]); int serr = 0, eerr = 0;
    static int s_rec[K][N];
    for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) { double v = 0.5 * (Sh[0][1][l][t] + Sh[1][1][l][t]);
      dot += v * sv[l][t]; nrm += v * v; tn += (double)sv[l][t] * sv[l][t];
      int rr = (int)lrint(v / ac); if (rr > ETA) rr = ETA; if (rr < -ETA) rr = -ETA; s_rec[l][t] = rr; if (rr != sv[l][t]) serr++; }
    /* attacker's public check: b' - A s_rec must lie in [-ETA-3, ETA+4] everywhere (b' = bpub, public) */
    int pubok = 1;
    for (int i = 0; i < K && pubok; i++) { long as[N] = {0};
      for (int j = 0; j < K; j++) { nmul(A[i][j], s_rec[j], tmp, N); for (int t = 0; t < N; t++) as[t] += tmp[t]; }
      for (int t = 0; t < N; t++) {
        int resid = cmodq(bpub[i][t] - as[t]); if (resid < -ETA-3 || resid > ETA+4) { pubok = 0; break; } } }
    { printf("  top offsets:"); for (int r=0;r<5;r++){int bi=0; for(int d=0;d<512;d++) if(offh[d]>offh[bi]) bi=d; printf(" %d:%d", bi>255?bi-512:bi, offh[bi]); offh[bi]=-1;} printf(" signflip=%d\n", offs[1]); } printf("  correct with sg=+1: %d, in odd fold: %d\n", posp, podd); printf("  pos_ok=%.3f sign_ok=%.3f\n", (double)pos_ok/nf, (double)sign_ok/nf); printf("iter %2d: E[post(true)]=%.3f hard-correct=%.3f a=(%.3f,%.3f) lam=(%.2f,%.2f) tau=(%.0f,%.0f) | true cos(s)=%.4f s-errors=%d/%d pubkey-check=%s\n",
           it, ptrue / nf, (double)hard_ok / nf, afold[0], afold[1], lam[0], lam[1], sqrt(tau2[0]), sqrt(tau2[1]), dot / sqrt(nrm * tn), serr, K * N, pubok ? "PASS" : "fail");
    fflush(stdout);
    if (pubok) { printf("RECOVERED s (public-key check passes; s-errors vs truth = %d) with %d failures\n", serr, nf); break; }
    if (it == iters) {
      /* attacker cleanup: try re-rounding the L least-confident coordinates (up to 4 at a time), accept iff
         R = b' - A s_rec has all coefficients in [-7,8] (b' = decompressed public key, public). */
      enum { L = 40 }; int idx[L]; double conf[K * N]; int alt[K * N];
      for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) { double v = 0.5 * (Sh[0][1][l][t] + Sh[1][1][l][t]) / ac; double fr = v - floor(v);
        conf[l * N + t] = fabs(fr - 0.5); alt[l * N + t] = (int)floor(v) + (fr < 0.5 ? 1 : 0); }
      { static int ord[K * N]; for (int x = 0; x < K * N; x++) ord[x] = x; gconf = conf; qsort(ord, K * N, sizeof(int), cmpconf); for (int j = 0; j < L; j++) idx[j] = ord[j];
        int wr = 0; for (int j = 0; j < L; j++) if (s_rec[idx[j] / N][idx[j] % N] != sv[idx[j] / N][idx[j] % N]) wr++; printf("(diagnostic) wrong coords among the %d least confident: %d of %d\n", L, wr, serr); }
      /* residual R = b' - A s_rec, and column effects of changing s_rec[l][p] by d: R_i -= d * (A_il * X^p) */
      static long R[K][N]; static int colv[L][K][N];
      for (int i = 0; i < K; i++) { long as[N] = {0};
        for (int j = 0; j < K; j++) { nmul(A[i][j], s_rec[j], tmp, N); for (int t = 0; t < N; t++) as[t] += tmp[t]; }
        for (int t = 0; t < N; t++) R[i][t] = cmodq(bpub[i][t] - as[t]); }
      for (int j = 0; j < L; j++) { int l = idx[j] / N, p = idx[j] % N, d = alt[idx[j]] - s_rec[l][p];
        for (int i = 0; i < K; i++) for (int t = 0; t < N; t++) { int src = t - p, sg2 = 1; if (src < 0) { src += N; sg2 = -1; } colv[j][i][t] = modq((long)d * sg2 * A[i][l][src]); } }
      long tried = 0; int found = 0, sel[4];
      for (int w = 1; w <= 4 && !found; w++) {
        int c[4]; for (int x = 0; x < w; x++) c[x] = x;
        for (;;) { tried++; int ok = 1;
          for (int i = 0; i < K && ok; i++) for (int t = 0; t < N; t++) { long v = R[i][t]; for (int x = 0; x < w; x++) v -= colv[c[x]][i][t]; int r2 = cmodq(v); if (r2 < -ETA-3 || r2 > ETA+4) { ok = 0; break; } }
          if (ok) { found = w; for (int x = 0; x < w; x++) sel[x] = c[x]; break; }
          int x = w - 1; while (x >= 0 && c[x] == L - w + x) x--; if (x < 0) break; c[x]++; for (int y = x + 1; y < w; y++) c[y] = c[y - 1] + 1; }
      }
      if (found) { for (int x = 0; x < found; x++) { int l = idx[sel[x]] / N, p = idx[sel[x]] % N; s_rec[l][p] = alt[idx[sel[x]]]; }
        int e2c = 0; for (int l = 0; l < K; l++) for (int t = 0; t < N; t++) if (s_rec[l][t] != sv[l][t]) e2c++;
        printf("cleanup: public-key check passes after re-rounding %d of the %d least-confident coords (%ld candidates); s-errors vs truth = %d\n", found, L, tried, e2c);
        if (e2c == 0) printf("RECOVERED s exactly with %d failures\n", nf); }
      else printf("cleanup failed (%ld candidates)\n", tried);
    }
  }
  return 0;
}
