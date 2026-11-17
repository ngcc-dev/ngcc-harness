#!/bin/sh
# hash-28 (XRH-1): two statements of specification section 3.1 about the XRH-p permutation do not hold:
#   3.1.3  "degree of n-round XRH-p is at most 2^n and that of the inverse is at most 3^n"  (it is 4 and 9 per round)
#   3.1.2  "no 5-round impossible differential ... with only one active cell" (every such pair is impossible)
# Reduced-round / analysis statements only; no attack on the hash function follows.
# Run from the repository root:  sh hash-28/reproduce_spec_claims.sh
set -e
cd "$(dirname "$0")/.." || exit 2
for t in cc gcc; do
    command -v "$t" >/dev/null 2>&1 || { echo "SKIP hash-28-2: $t unavailable"; exit 77; }
done
ROOT=$(pwd)
REF="$ROOT/hash-28/XRH-1/Implementations/Reference_Implementation/XRH-1-512"
T=$(mktemp -d)
trap 'rm -r "$T"' EXIT
cat > "$T/claims.c" <<'CEOF'
/* XRH-p: checks of two statements of specification section 3.1 against the shipped reference permutation.
 *  (A) 3.1.3 "the degree of n-round XRH-p is at most 2^n and that of the inverse is at most 3^n":
 *      one round = Super S-box S' = S^4 o M o S^4 followed by linear maps; deg S' = 4, deg S'^-1 = 9.
 *  (B) 3.1.2 "no 5-round impossible differential distinguisher exists ... when both the input and
 *      output differences contain only one active cell": every such pair is impossible over 5 rounds.
 * The round model below is validated against the candidate's own XRH1280() before use. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
void XRH1280(int R0, int R1, uint64_t *buf);
extern const uint64_t ROUND_CONSTANTS[24];
static const uint8_t SB[32] = {4,14,10,5,9,24,20,0,18,28,30,21,2,23,29,13,22,27,15,7,26,12,16,3,8,1,19,31,25,11,17,6};
static const uint8_t MM[4][4] = {{3,2,1,3},{3,1,4,4},{1,3,6,4},{2,2,3,1}};   /* [b3..b0] = M [a3..a0] */
static uint8_t SI[32], MI[4][4];
static uint8_t gmul(uint8_t a, uint8_t b) { unsigned r = 0, i; for (i = 0; i < 5; i++) if (b >> i & 1) r ^= (unsigned)a << i;
    for (i = 8; i >= 5; i--) if (r >> i & 1) r ^= 0x25u << (i - 5); return (uint8_t)r; }
static uint8_t ginv(uint8_t a) { uint8_t b; for (b = 1; b < 32; b++) if (gmul(a, b) == 1) return b; return 0; }
static void matinv(void) {
    uint8_t A[4][8]; int i, j, c, r;
    for (i = 0; i < 4; i++) for (j = 0; j < 8; j++) A[i][j] = j < 4 ? MM[i][j] : (j - 4 == i);
    for (c = 0; c < 4; c++) { uint8_t iv, t;
        for (r = c; !A[r][c]; r++) ;
        for (j = 0; j < 8; j++) { t = A[c][j]; A[c][j] = A[r][j]; A[r][j] = t; }
        iv = ginv(A[c][c]); for (j = 0; j < 8; j++) A[c][j] = gmul(iv, A[c][j]);
        for (r = 0; r < 4; r++) if (r != c && A[r][c]) { uint8_t f = A[r][c]; for (j = 0; j < 8; j++) A[r][j] ^= gmul(f, A[c][j]); } }
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) MI[i][j] = A[i][j + 4];
}
static void mds(const uint8_t M[4][4], uint8_t a[4]) {   /* a[i] = a_i */
    uint8_t b[4]; int r, c;
    for (r = 0; r < 4; r++) { b[3 - r] = 0; for (c = 0; c < 4; c++) b[3 - r] ^= gmul(M[r][c], a[3 - c]); }
    memcpy(a, b, 4);
}
typedef struct { uint8_t c[64][4]; } st_t;
static int tgt(int kind, int i, int j) {
    if (kind == 0) return 4 * (j / 4) + ((j % 4) + i) % 4;
    if (kind == 1) return 16 * (j / 16) + ((j % 16) + 4 * i) % 16;
    return (j + 16 * i) % 64;
}
static void sr(st_t *s, int kind, int inv) { st_t o; int i, j;
    for (j = 0; j < 64; j++) for (i = 0; i < 4; i++) { int t = tgt(kind, i, j); if (!inv) o.c[t][i] = s->c[j][i]; else o.c[j][i] = s->c[t][i]; }
    *s = o; }
static void rnd(st_t *s, int r) { int i, j;
    for (j = 0; j < 64; j++) { for (i = 0; i < 4; i++) s->c[j][i] = SB[s->c[j][i]]; mds(MM, s->c[j]); for (i = 0; i < 4; i++) s->c[j][i] = SB[s->c[j][i]]; }
    sr(s, r % 3, 0); for (j = 0; j < 64; j++) mds(MM, s->c[j]); sr(s, r % 3, 1);
    for (j = 0; j < 64; j++) s->c[j][0] ^= (uint8_t)(ROUND_CONSTANTS[r] >> j & 1); }
static void rnd_inv(st_t *s, int r) { int i, j;
    for (j = 0; j < 64; j++) s->c[j][0] ^= (uint8_t)(ROUND_CONSTANTS[r] >> j & 1);
    sr(s, r % 3, 0); for (j = 0; j < 64; j++) mds(MI, s->c[j]); sr(s, r % 3, 1);
    for (j = 0; j < 64; j++) { for (i = 0; i < 4; i++) s->c[j][i] = SI[s->c[j][i]]; mds(MI, s->c[j]); for (i = 0; i < 4; i++) s->c[j][i] = SI[s->c[j][i]]; } }
static void to_lanes(const st_t *s, uint64_t x[20]) { int i, j, k; memset(x, 0, 160);
    for (j = 0; j < 64; j++) for (i = 0; i < 4; i++) for (k = 0; k < 5; k++) x[5 * i + k] |= (uint64_t)(s->c[j][i] >> k & 1) << j; }
static uint64_t rs = 0x243F6A8885A308D3ULL;
static uint32_t rng(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 20); }
static void rand_st(st_t *s) { int i, j; for (j = 0; j < 64; j++) for (i = 0; i < 4; i++) s->c[j][i] = rng() & 31; }
static uint64_t active(const st_t *a, const st_t *b) { uint64_t m = 0; int j; for (j = 0; j < 64; j++) if (memcmp(a->c[j], b->c[j], 4)) m |= 1ULL << j; return m; }

static int degree(const uint32_t *T) {       /* max algebraic degree of the 20 coordinates of a 20-bit map */
    static uint8_t f[1 << 20]; int bit, i, d = 0; uint32_t x;
    for (bit = 0; bit < 20; bit++) {
        for (x = 0; x < 1u << 20; x++) f[x] = T[x] >> bit & 1;
        for (i = 0; i < 20; i++) for (x = 0; x < 1u << 20; x++) if (x >> i & 1) f[x] ^= f[x ^ (1u << i)];
        for (x = 0; x < 1u << 20; x++) if (f[x] && __builtin_popcount(x) > d) d = __builtin_popcount(x);
    }
    return d;
}
/* the 4 columns coupled with column w by the Super-MixColumn of the given kind */
static void line(int kind, int w, int L[4]) { int d; for (d = 0; d < 4; d++) L[d] = kind == 0 ? 4 * (w / 4) + d : kind == 1 ? 16 * (w / 16) + w % 4 + 4 * d : w % 16 + 16 * d; }
/* column-level certain propagation through one Super-MixColumn (or its inverse): a line with exactly one
 * non-zero column makes all four columns non-zero (branch number 5, one cell of each column per MDS);
 * returns 0 if some line has two or more non-zero columns (then nothing is certain). */
static int prop(int kind, uint64_t in, uint64_t *out) { int w, d, L[4]; uint64_t o = 0;
    for (w = 0; w < 64; w++) { int n = 0; line(kind, w, L); for (d = 0; d < 4; d++) n += (int)(in >> L[d] & 1);
        if (n > 1) return 0; if (n == 1) o |= 1ULL << w; }
    *out = o; return 1; }

int main(void) {
    static uint32_t SP[1 << 20], SPI[1 << 20]; uint32_t x; int i, j, t, s, fail = 0, df, db;
    for (i = 0; i < 32; i++) SI[SB[i]] = (uint8_t)i;
    matinv();
    /* validate the model against the shipped XRH1280 */
    for (t = 0; t < 20; t++) { st_t a, b; uint64_t la[20], lb[20]; int r0 = t % 18, r1 = r0 + 1 + (int)(rng() % (18 - r0));
        rand_st(&a); b = a; to_lanes(&a, la); for (i = r0; i < r1; i++) rnd(&b, i); XRH1280(r0, r1, la); to_lanes(&b, lb);
        if (memcmp(la, lb, 160)) fail = 1;
        for (i = r1 - 1; i >= r0; i--) rnd_inv(&b, i); if (memcmp(&a, &b, sizeof a)) fail = 1; }
    printf("  round model == shipped XRH1280, inverse consistent: %s\n", fail ? "NO" : "yes");
    if (fail) return 1;
    /* (A) degrees */
    for (x = 0; x < 1u << 20; x++) { uint8_t a[4]; for (i = 0; i < 4; i++) a[i] = SB[x >> (5 * i) & 31]; mds(MM, a);
        SP[x] = 0; for (i = 0; i < 4; i++) SP[x] |= (uint32_t)SB[a[i]] << (5 * i); }
    for (x = 0; x < 1u << 20; x++) SPI[SP[x]] = x;
    df = degree(SP); db = degree(SPI);
    printf("  (A) algebraic degree of one round: %d (spec bound 2^1 = 2); of one inverse round: %d (spec bound 3^1 = 3)\n", df, db);
    /* (B) impossible differentials: exact column-level propagation for every alignment and cell pair */
    { long pairs = 0, imp = 0;
      for (s = 0; s < 3; s++) for (j = 0; j < 64; j++) { uint64_t f1, f2, b1, b2, b3; int jo, io;
        if (!prop(s % 3, 1ULL << j, &f1) || !prop((s + 1) % 3, f1, &f2)) return 1;      /* after rounds s, s+1 */
        for (jo = 0; jo < 64; jo++) for (io = 0; io < 4; io++) {
            /* output cell (io,jo) after round s+4: its MDS in M'^-1 sits at shifted column tgt(); the four
               source columns are the line of jo; exactly these four columns are non-zero before M' */
            int L[4], d; uint64_t m = 0; line((s + 4) % 3, jo, L); for (d = 0; d < 4; d++) m |= 1ULL << L[d];
            b1 = m;                                                    /* input of round s+4 */
            if (!prop((s + 3) % 3, b1, &b2) || !prop((s + 2) % 3, b2, &b3)) return 1; /* inputs of rounds s+3, s+2 */
            pairs += 4;                                                /* 4 input nibbles of column j */
            if (f2 != b3) imp += 4;                                    /* certain non-zero sets differ */
        } }
      printf("  (B) 5-round single-cell -> single-cell pairs that are impossible: %ld of %ld (all three round alignments)\n", imp, pairs);
      if (imp != pairs) fail = 1; }
    /* Monte-Carlo confirmation of the two probability-1 truncated differentials on the real permutation */
    { int okf = 1, okb = 1;
      for (t = 0; t < 2000; t++) { st_t a, b; int col = (int)(rng() % 64), nib = (int)(rng() & 3), s0 = (int)(rng() % 14); uint8_t dv = (uint8_t)(1 + rng() % 31);
        rand_st(&a); b = a; b.c[col][nib] ^= dv; rnd(&a, s0); rnd(&a, s0 + 1); rnd(&b, s0); rnd(&b, s0 + 1);
        if (__builtin_popcountll(active(&a, &b)) != 16) okf = 0;
        rand_st(&a); b = a; b.c[col][nib] ^= dv; for (i = s0 + 4; i >= s0 + 2; i--) { rnd_inv(&a, i); rnd_inv(&b, i); }
        if (active(&a, &b) != ~0ULL) okb = 0; }
      printf("      2000 random pairs: forward 2 rounds -> exactly 16 active columns: %s; backward 3 rounds -> all 64 columns active: %s\n", okf ? "always" : "NO", okb ? "always" : "NO");
      if (!okf || !okb) fail = 1; }
    return (fail || df != 4 || db != 9) ? 1 : 0;
}
CEOF
cc -O2 -std=gnu99 -I"$REF" "$T/claims.c" "$REF/CryptHash_AlgorithmInstance.c" -o "$T/claims"
if "$T/claims"; then
    echo "ATTACK hash-28-2 (spec 3.1.2/3.1.3 statements on XRH-p contradicted: 5-round impossible differentials, round degree 4/9) CONFIRMED"
else
    echo "ATTACK hash-28-2 NOT reproduced"; exit 1
fi
