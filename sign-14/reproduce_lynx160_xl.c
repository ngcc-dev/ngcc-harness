/* sign-14-6 witness, part 2: plain XL over GF(2) (multilinear, field
 * equations implicit), written by an analysis agent and reviewed for this
 * repository. If the system is solved it also prints
 *   SOLUTION <bit 0><bit 1>... of the n unknowns.
 * Usage: xl <system.bin> <D> [maxrows [mv [meq]]]
 * system.bin: int32 n, int32 m, int32 nb, then m equations of nb bytes (bit t = coefficient of
 * monomial t in the order [1][z_0..z_{n-1}][z_i z_j, i<j, colex: index 1+n+j(j-1)/2+i]),
 * then n bytes: expected solution (0/1), used only to check the result.
 * Rows: every equation, plus (for multiplier degree 1..D-2) equations 0..meq-1 times every
 * monomial in variables 0..mv-1 (defaults: all equations, all variables).
 * Columns: only monomials that occur (plus all linear ones and 1), degree descending.
 * Echelon form M4RI-style (64-column stripes, 8-bit tables); reports rank and linear relations. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <immintrin.h>

static uint64_t binom[300][8];
static int n, m, D, mv, meq;
static uint64_t start[8];
static uint64_t nrows_max;
static int (*terms)[2];
static size_t* tstart;

static uint64_t col_of(const int* s, int d) {
  uint64_t r = 0;
  for (int t = 0; t < d; t++) r += binom[s[t]][t + 1];
  return start[d] + r;
}

static double now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

static int next_comb(int* s, int e, int nn) {
  for (int t = 0; t < e; t++) {
    if (t + 1 < e ? s[t] + 1 < s[t + 1] : s[t] + 1 < nn) {
      s[t]++;
      for (int u = 0; u < t; u++) s[u] = u;
      return 1;
    }
  }
  return 0;
}

/* mode 0: mark used columns in bitmap; mode 1: toggle compacted bits in M */
static uint64_t enumerate(int mode, uint64_t* used, const uint32_t* cmap, uint64_t* M, uint64_t W) {
  uint64_t row = 0;
  for (int dm = 0; dm <= D - 2 && row < nrows_max; dm++) {
    int mu[8];
    for (int u = 0; u < dm; u++) mu[u] = u;
    if (dm > mv) break;
    int more = 1;
    while (more && row < nrows_max) {
      int me = dm == 0 ? m : meq;
      for (int e = 0; e < me && row < nrows_max; e++, row++) {
        for (size_t p = tstart[e]; p < tstart[e + 1]; p++) {
          int s[8], d = 0;
          int a = terms[p][0], b = terms[p][1];
          int extra[2], ne = 0;
          if (a >= 0) extra[ne++] = a;
          if (b >= 0) extra[ne++] = b;
          int x = 0, y = 0;
          while (x < dm || y < ne) {
            int v;
            if (y >= ne || (x < dm && mu[x] < extra[y])) v = mu[x++];
            else if (x >= dm || extra[y] < mu[x]) v = extra[y++];
            else { v = mu[x]; x++; y++; }
            s[d++] = v;
          }
          uint64_t c = col_of(s, d);
          if (mode == 0) used[c >> 6] |= UINT64_C(1) << (c & 63);
          else { uint64_t cm = cmap[c]; M[row * W + (cm >> 6)] ^= UINT64_C(1) << (cm & 63); }
        }
      }
      more = dm > 0 ? next_comb(mu, dm, mv) : 0;
    }
  }
  return row;
}

int main(int argc, char** argv) {
  if (argc < 3) { fprintf(stderr, "usage: xl sys.bin D [maxrows [mv [meq]]]\n"); return 1; }
  FILE* f = fopen(argv[1], "rb");
  if (!f) { perror("open"); return 1; }
  D = atoi(argv[2]);
  int32_t hdr[3];
  if (fread(hdr, 4, 3, f) != 3) return 1;
  n = hdr[0]; m = hdr[1]; int nb = hdr[2];
  uint8_t* eqb = malloc((size_t)m * nb);
  if (fread(eqb, 1, (size_t)m * nb, f) != (size_t)m * nb) return 1;
  uint8_t* sol = malloc(n);
  if (fread(sol, 1, n, f) != (size_t)n) return 1;
  fclose(f);
  for (int a = 0; a < 300; a++) {
    binom[a][0] = 1;
    for (int b = 1; b < 8; b++) binom[a][b] = a == 0 ? 0 : binom[a - 1][b - 1] + binom[a - 1][b];
  }
  mv = n; meq = m;
  if (argc > 4) mv = atoi(argv[4]);
  if (argc > 5) meq = atoi(argv[5]);
  uint64_t off = 0;
  for (int d = D; d >= 0; d--) { start[d] = off; off += binom[n][d]; }
  uint64_t fullcols = off;
  nrows_max = m;
  for (int d = 1; d <= D - 2; d++) nrows_max += (uint64_t)meq * binom[mv][d];
  if (argc > 3) { uint64_t mr = strtoull(argv[3], 0, 10); if (mr && mr < nrows_max) nrows_max = mr; }
  int T2 = 1 + n + n * (n - 1) / 2;
  terms = malloc(sizeof(int[2]) * (size_t)m * T2);
  tstart = malloc(sizeof(size_t) * (m + 1));
  int* qi = malloc(sizeof(int) * T2);
  int* qj = malloc(sizeof(int) * T2);
  {
    int t = 0;
    qi[t] = -1; qj[t] = -1; t++;
    for (int i = 0; i < n; i++) { qi[t] = i; qj[t] = -1; t++; }
    for (int j = 1; j < n; j++)
      for (int i = 0; i < j; i++) { qi[t] = i; qj[t] = j; t++; }
  }
  size_t tpos = 0;
  for (int e = 0; e < m; e++) {
    tstart[e] = tpos;
    for (int t = 0; t < T2; t++)
      if ((eqb[(size_t)e * nb + t / 8] >> (t % 8)) & 1) { terms[tpos][0] = qi[t]; terms[tpos][1] = qj[t]; tpos++; }
  }
  tstart[m] = tpos;
  double t0 = now();
  uint64_t* used = calloc((fullcols + 63) / 64, 8);
  enumerate(0, used, NULL, NULL, 0);
  for (uint64_t c = start[1]; c < fullcols; c++) used[c >> 6] |= UINT64_C(1) << (c & 63);
  uint32_t* cmap = malloc(sizeof(uint32_t) * fullcols);
  uint64_t ncols = 0;
  for (uint64_t c = 0; c < fullcols; c++) { cmap[c] = (uint32_t)ncols; if ((used[c >> 6] >> (c & 63)) & 1) ncols++; }
  uint64_t lin0 = cmap[start[1]], cst = cmap[start[0]];
  uint64_t W = (ncols + 63) / 64;
  fprintf(stderr, "n=%d m=%d D=%d mv=%d meq=%d rows=%llu cols=%llu (of %llu) words=%llu mem=%.1f MB\n", n, m, D,
          mv, meq, (unsigned long long)nrows_max, (unsigned long long)ncols, (unsigned long long)fullcols,
          (unsigned long long)W, nrows_max * W * 8.0 / 1e6);
  uint64_t* M = aligned_alloc(64, ((nrows_max * W * 8 + 63) / 64) * 64);
  if (!M) { fprintf(stderr, "alloc failed\n"); return 1; }
  memset(M, 0, nrows_max * W * 8);
  uint64_t nrows = enumerate(1, NULL, cmap, M, W);
  double t1 = now();
  uint64_t r = 0;
  int64_t* pivcol = malloc(sizeof(int64_t) * (ncols + 1));
  uint64_t npiv = 0;
  uint64_t* tab = aligned_alloc(64, sizeof(uint64_t) * 8 * 256 * W);
  uint64_t* tmp = malloc(W * 8);
  uint64_t wops = 0; /* 64-bit word XORs performed by the elimination */
  for (uint64_t s = 0; s < W && r < nrows; s++) {
    int k = 0;
    uint64_t pr[64];
    int pb[64];
    uint64_t pmask = 0;
    for (uint64_t i = r; i < nrows && k < 64; i++) {
      uint64_t* Ri = M + i * W;
      uint64_t w = Ri[s];
      if (!w) continue;
      if (w & pmask) {
        for (int j = 0; j < k; j++)
          if ((Ri[s] >> pb[j]) & 1) {
            uint64_t* P = M + pr[j] * W;
            for (uint64_t q = s; q < W; q++) Ri[q] ^= P[q];
            wops += W - s;
          }
        w = Ri[s];
      }
      if (!w) continue;
      int b = __builtin_ctzll(w);
      for (int j = 0; j < k; j++) {
        uint64_t* P = M + pr[j] * W;
        if ((P[s] >> b) & 1) {
          for (uint64_t q = s; q < W; q++) P[q] ^= Ri[q];
          wops += W - s;
        }
      }
      pr[k] = i; pb[k] = b; pmask |= UINT64_C(1) << b; k++;
    }
    if (k == 0) continue;
    for (int j = 0; j < k; j++) {
      uint64_t dst = r + j;
      if (pr[j] != dst) {
        memcpy(tmp, M + dst * W, W * 8);
        memcpy(M + dst * W, M + pr[j] * W, W * 8);
        memcpy(M + pr[j] * W, tmp, W * 8);
        for (int jj = j + 1; jj < k; jj++) if (pr[jj] == dst) pr[jj] = pr[j];
        pr[j] = dst;
      }
      pivcol[npiv++] = (int64_t)(64 * s + pb[j]);
    }
    int ng = (k + 7) / 8;
    uint64_t gmask[8];
    uint64_t WW = W - s;
    for (int g = 0; g < ng; g++) {
      gmask[g] = 0;
      int idx[8];
      for (int j = 8 * g; j < k && j < 8 * g + 8; j++) gmask[g] |= UINT64_C(1) << pb[j];
      int cnt = 0;
      for (int b = 0; b < 64; b++)
        if ((gmask[g] >> b) & 1) {
          for (int j = 8 * g; j < k && j < 8 * g + 8; j++) if (pb[j] == b) idx[cnt] = (int)pr[j];
          cnt++;
        }
      uint64_t* T = tab + (size_t)g * 256 * W;
      memset(T, 0, WW * 8);
      for (int v = 1; v < (1 << cnt); v++) {
        int low = __builtin_ctz(v);
        uint64_t* dst = T + (size_t)v * W;
        uint64_t* src = T + (size_t)(v & (v - 1)) * W;
        uint64_t* P = M + (uint64_t)idx[low] * W + s;
        for (uint64_t q = 0; q < WW; q++) dst[q] = src[q] ^ P[q];
      }
      wops += (uint64_t)((1 << cnt) - 1) * WW;
    }
    uint64_t wloc = 0;
    #pragma omp parallel for schedule(dynamic, 64) reduction(+:wloc)
    for (uint64_t i = r + k; i < nrows; i++) {
      uint64_t* Ri = M + i * W + s;
      uint64_t w = Ri[0];
      if (!w) continue;
      for (int g = 0; g < ng; g++) {
        uint64_t v = _pext_u64(w, gmask[g]);
        if (!v) continue;
        const uint64_t* T = tab + (size_t)g * 256 * W + (size_t)v * W;
        for (uint64_t q = 0; q < WW; q++) Ri[q] ^= T[q];
        wloc += WW;
      }
    }
    wops += wloc;
    r += k;
  }
  double t2 = now();
  uint64_t rank = r;
  int incons = 0, L = 0;
  uint8_t (*lr)[512] = malloc(sizeof(uint8_t[512]) * (rank + 1));
  for (uint64_t i = 0; i < rank; i++) {
    if ((uint64_t)pivcol[i] < lin0) continue;
    uint64_t* Ri = M + i * W;
    memset(lr[L], 0, 512);
    for (int v = 0; v < n; v++) {
      uint64_t c = lin0 + v;
      lr[L][v] = (Ri[c >> 6] >> (c & 63)) & 1;
    }
    lr[L][n] = (Ri[cst >> 6] >> (cst & 63)) & 1;
    L++;
  }
  int lrank = 0, pv[512];
  for (int c = 0; c <= n && lrank < L; c++) {
    int p = -1;
    for (int i = lrank; i < L; i++) if (lr[i][c]) { p = i; break; }
    if (p < 0) continue;
    if (p != lrank) { uint8_t t[512]; memcpy(t, lr[p], 512); memcpy(lr[p], lr[lrank], 512); memcpy(lr[lrank], t, 512); }
    for (int i = 0; i < L; i++) if (i != lrank && lr[i][c]) for (int q = 0; q <= n; q++) lr[i][q] ^= lr[lrank][q];
    pv[lrank] = c;
    lrank++;
  }
  for (int i = 0; i < lrank; i++) if (pv[i] == n) incons = 1;
  int nlin = incons ? lrank - 1 : lrank;
  int sat = 1;
  for (int i = 0; i < lrank; i++) {
    int acc = lr[i][n];
    for (int v = 0; v < n; v++) acc ^= lr[i][v] & sol[v];
    if (acc) sat = 0;
  }
  int solved = (!incons && nlin == n);
  if (solved) {
    char* bits = malloc(n + 1);
    for (int v = 0; v < n; v++) bits[v] = '?';
    bits[n] = 0;
    for (int i = 0; i < lrank; i++) if (pv[i] < n) bits[pv[i]] = lr[i][n] ? '1' : '0';
    printf("SOLUTION %s\n", bits);
    free(bits);
  }
  printf("RESULT n=%d m=%d D=%d mv=%d meq=%d rows=%llu cols=%llu rank=%llu deficit=%lld linear_relations=%d "
         "inconsistent=%d solved=%d expected_satisfies=%d build_s=%.2f elim_s=%.2f log2_wordops=%.2f\n",
         n, m, D, mv, meq, (unsigned long long)nrows, (unsigned long long)ncols, (unsigned long long)rank,
         (long long)ncols - 1 - (long long)rank, nlin, incons, solved, sat, t1 - t0, t2 - t1, wops ? log2((double)wops) : 0.0);
  return 0;
}
