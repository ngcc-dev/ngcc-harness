/*
 * QuantaSylva Hash (QSH) -- ISO C99 reference implementation (core).
 *
 * Implements the official interface
 *     int CryptHash(int digest_len_bits, const unsigned char *msg,
 *                   unsigned long long msg_len_bits, unsigned char *digest);
 * for QSH-512 (digest_len_bits == 512), QSH-768 (768) and QSH-1024 (1024).
 *
 * Conventions:
 *   - message bits are MSB-first within each byte;
 *   - w-bit words and the 128-bit length field are little-endian;
 *   - digest is the root chaining value serialized little-endian, truncated.
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "CryptHash_AlgorithmInstance.h"

#define U 64u        /* words per state value (4^d, d=3)            */
#define V 32u        /* words per message block (u/2)               */
#define RHO 16u      /* blocks per chunk                            */

#define FLAG_CHUNK_START 1u
#define FLAG_CHUNK_END   2u
#define FLAG_PARENT      4u
#define FLAG_ROOT        8u

/* ----- per-variant parameters ------------------------------------------- */
typedef struct {
    int      w;        /* word width in bits (32 or 64)             */
    int      rounds;   /* n_r full R-rounds (= d*3w/32)             */
    int      n;        /* digest length in bits                     */
    uint64_t iv0;      /* H^{-1}_0 variant constant                 */
    uint64_t mask;     /* (1<<w)-1                                  */
    int      m;        /* message block size in bits (= h/2)        */
} qsh_params;

static int select_params(int digest_len_bits, qsh_params *P)
{
    switch (digest_len_bits) {
    case 512:  P->w = 32; P->rounds = 9;  P->n = 512;  P->iv0 = 17; break;
    case 768:  P->w = 64; P->rounds = 18; P->n = 768;  P->iv0 = 1;  break;
    case 1024: P->w = 64; P->rounds = 18; P->n = 1024; P->iv0 = 7;  break;
    default: return -1;
    }
    P->mask = (P->w == 64) ? ~(uint64_t)0 : (uint64_t)0xFFFFFFFFu;
    P->m = P->w * 32; /* h/2 = (4^3*w)/2 = 32w */
    return 0;
}

/* ----- word primitives -------------------------------------------------- */
static inline uint64_t rotl(uint64_t x, int r, int w, uint64_t mask)
{
    r %= w;
    x &= mask;
    if (r == 0) return x;
    return ((x << r) | (x >> (w - r))) & mask;
}

/* G : quadruple-word transformation (Section 3.1) */
static void Gfunc(uint64_t q[4], const qsh_params *P)
{
    const int w = P->w; const uint64_t mk = P->mask;
    const int s1 = 1, s2 = w / 2 - 1, s3 = 0, s4 = w / 4;
    const int t1 = w / 2, t2 = w / 2 - 4, t3 = w / 4, t4 = w / 4 - 1;
    uint64_t a = q[0], b = q[1], c = q[2], d = q[3];

    a = (a + rotl(b, s1, w, mk)) & mk; d = rotl(d ^ a, t1, w, mk);
    c = (c + rotl(d, s2, w, mk)) & mk; b = rotl(b ^ c, t2, w, mk);
    a = (a + rotl(b, s3, w, mk)) & mk; d = rotl(d ^ a, t3, w, mk);
    c = (c + rotl(d, s4, w, mk)) & mk; b = rotl(b ^ c, t4, w, mk);

    q[0] = a; q[1] = b; q[2] = c; q[3] = d;
}

/* linear index i = 16*x0 + 4*x2 + x1  (Section 3.3) */
static inline unsigned idx(unsigned x0, unsigned x1, unsigned x2)
{
    return 16u * x0 + 4u * x2 + x1;
}

/* apply G to every 4-tuple along axis 0 (vary x0) or axis 1 (vary x1) */
static void G_along(uint64_t *s, int axis, const qsh_params *P)
{
    uint64_t q[4];
    unsigned a, b, k;
    if (axis == 0) {
        for (a = 0; a < 4; a++)        /* a = x1 */
            for (b = 0; b < 4; b++) {  /* b = x2 */
                for (k = 0; k < 4; k++) q[k] = s[idx(k, a, b)];
                Gfunc(q, P);
                for (k = 0; k < 4; k++) s[idx(k, a, b)] = q[k];
            }
    } else { /* axis == 1, vary x1 */
        for (a = 0; a < 4; a++)        /* a = x0 */
            for (b = 0; b < 4; b++) {  /* b = x2 */
                for (k = 0; k < 4; k++) q[k] = s[idx(a, k, b)];
                Gfunc(q, P);
                for (k = 0; k < 4; k++) s[idx(a, k, b)] = q[k];
            }
    }
}

/* P16 plane shuffle (dir=+1) or inverse (dir=-1).
 * plane 0: rows=x0, cols=x1, per x2 ; plane 1: rows=x1, cols=x2, per x0 */
static void apply_plane(uint64_t *s, int dir, int plane)
{
    uint64_t tmp[U];
    unsigned r, c, f, src;
    memcpy(tmp, s, sizeof(tmp));
    if (plane == 0) {
        for (f = 0; f < 4; f++)            /* f = x2 */
            for (r = 0; r < 4; r++)        /* r = x0 */
                for (c = 0; c < 4; c++) {  /* c = x1 */
                    src = (dir == 1) ? (c + r) % 4 : (c + 4 - r % 4) % 4;
                    s[idx(r, c, f)] = tmp[idx(r, src, f)];
                }
    } else {
        for (f = 0; f < 4; f++)            /* f = x0 */
            for (r = 0; r < 4; r++)        /* r = x1 */
                for (c = 0; c < 4; c++) {  /* c = x2 */
                    src = (dir == 1) ? (c + r) % 4 : (c + 4 - r % 4) % 4;
                    s[idx(f, r, c)] = tmp[idx(f, r, src)];
                }
    }
}

/* round function R_{3,w} (Algorithm 1, d=3) */
static void R3(uint64_t *s, const qsh_params *P)
{
    G_along(s, 0, P);                      /* column layer            */
    apply_plane(s, +1, 0); G_along(s, 0, P); apply_plane(s, -1, 0); /* (x0,x1) diag */
    apply_plane(s, +1, 1); G_along(s, 1, P); apply_plane(s, -1, 1); /* (x1,x2) diag */
}

/* permutation E_{3,w} = ChaCha-Bahru (Algorithm 2) */
static void E3(uint64_t *s, const qsh_params *P)
{
    int r;
    for (r = 0; r < P->rounds; r++) R3(s, P);
    G_along(s, 0, P);                      /* final column-only layer */
}

/* compression F_{3,w} (Algorithm 3): H,M -> H'  (all arrays of words) */
static void F3(const uint64_t *H, const uint64_t *M, uint64_t flag,
               uint64_t *Hout, const qsh_params *P)
{
    uint64_t S[U];
    unsigned j;
    for (j = 0; j < V; j++) {
        S[j]     = H[j];
        S[V + j] = (H[V + j] + M[j]) & P->mask;
    }
    S[0] = (S[0] ^ flag) & P->mask;
    E3(S, P);
    for (j = 0; j < V; j++) {
        Hout[j]     = (S[j] + M[j]) & P->mask;
        Hout[V + j] = S[V + j];
    }
}

/* initial state value H^{(0)} = F(0^m, H^{-1}) */
static void initial_state(uint64_t *H0, const qsh_params *P)
{
    uint64_t Hm1[U], zero[V];
    memset(Hm1, 0, sizeof(Hm1));
    memset(zero, 0, sizeof(zero));
    Hm1[0] = P->iv0 & P->mask;
    F3(Hm1, zero, 0, H0, P);
}

/* ----- padded-message byte access (no full padded buffer) --------------- */
static inline unsigned char high_n_mask(int n) /* keep high n bits */
{
    return (unsigned char)((0xFFu << (8 - n)) & 0xFF);
}

/* return byte i of the padded message */
static unsigned char padded_byte(const unsigned char *msg,
                                 unsigned long long L,        /* bits */
                                 unsigned long long tot_bytes,
                                 unsigned long long i)
{
    unsigned long long full = L >> 3;          /* full message bytes */
    int rem = (int)(L & 7u);                    /* trailing bits      */
    if (i < full) return msg[i];
    if (i == full) {                            /* '1' bit / partial  */
        unsigned char b = 0;
        if (rem) b = (unsigned char)(msg[i] & high_n_mask(rem));
        b |= (unsigned char)(0x80u >> rem);     /* append single 1    */
        return b;
    }
    if (i >= tot_bytes - 16) {                   /* 128-bit length, LE */
        unsigned shift = (unsigned)(i - (tot_bytes - 16)) * 8u;
        if (shift >= 64) return 0;               /* high 64 bits are 0 */
        return (unsigned char)((L >> shift) & 0xFFu);
    }
    return 0;
}

/* build block t into V words (little-endian) */
static void build_block(const unsigned char *msg, unsigned long long L,
                        unsigned long long tot_bytes, unsigned long long t,
                        uint64_t *blk, const qsh_params *P)
{
    unsigned wb = (unsigned)(P->w / 8);
    unsigned long long base = t * (unsigned long long)(P->m / 8);
    unsigned j, b;
    for (j = 0; j < V; j++) {
        uint64_t word = 0;
        for (b = 0; b < wb; b++)
            word |= (uint64_t)padded_byte(msg, L, tot_bytes, base + j * wb + b) << (8 * b);
        blk[j] = word;
    }
}

static unsigned long long largest_pow2_lt(unsigned long long n)
{
    unsigned long long p = 1;
    while (p * 2 < n) p *= 2;
    return p;
}

/* recursive tree over cv array; each cv is V/2 words.
 * writes the resulting (V/2)-word chaining value into out. */
static void tree(const uint64_t *cvs, unsigned long long lo, unsigned long long hi,
                 int is_root, uint64_t *out, const qsh_params *P)
{
    const unsigned half = V / 2;
    if (hi - lo == 1) {
        memcpy(out, cvs + lo * half, half * sizeof(uint64_t));
        return;
    }
    unsigned long long llen = largest_pow2_lt(hi - lo);
    uint64_t left[V / 2], right[V / 2];
    tree(cvs, lo, lo + llen, 0, left, P);
    tree(cvs, lo + llen, hi, 0, right, P);

    uint64_t Mblk[V], H0[U], I1[U];
    memcpy(Mblk, left, half * sizeof(uint64_t));
    memcpy(Mblk + half, right, half * sizeof(uint64_t));
    initial_state(H0, P);
    uint64_t flag = FLAG_PARENT | (is_root ? FLAG_ROOT : 0);
    F3(H0, Mblk, flag, I1, P);
    memcpy(out, I1, half * sizeof(uint64_t));
}

/* ----- public interface ------------------------------------------------- */
int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest)
{
    qsh_params P;
    if (select_params(digest_len_bits, &P)) return -1;

    const unsigned half = V / 2;
    unsigned long long L = msg_len_bits;
    unsigned long long m = (unsigned long long)P.m;
    /* padded length in bits = L + 2m - (L mod m); a multiple of m */
    unsigned long long tot_bits = L + 2ull * m - (L % m);
    unsigned long long tot_bytes = tot_bits / 8ull;
    unsigned long long N = tot_bits / m;                 /* blocks            */
    unsigned long long Pi = (N + RHO - 1) / RHO;         /* chunks            */
    int is_only = (Pi == 1);

    uint64_t *cvs = (uint64_t *)malloc((size_t)Pi * half * sizeof(uint64_t));
    if (!cvs) return -2;

    uint64_t H0[U];
    initial_state(H0, &P);

    unsigned long long t;
    for (t = 0; t < Pi; t++) {
        unsigned long long start = t * RHO;
        unsigned long long Lc = (start + RHO <= N) ? RHO : (N - start);
        uint64_t H[U];
        memcpy(H, H0, sizeof(H));
        unsigned long long i;
        for (i = 1; i <= Lc; i++) {
            uint64_t blk[V], Hn[U], flag = 0;
            build_block(msg, L, tot_bytes, start + (i - 1), blk, &P);
            if (i == 1)             flag |= FLAG_CHUNK_START;
            if (i == Lc)            flag |= FLAG_CHUNK_END;
            if (i == Lc && is_only) flag |= FLAG_ROOT;
            F3(H, blk, flag, Hn, &P);
            memcpy(H, Hn, sizeof(H));
        }
        memcpy(cvs + t * half, H, half * sizeof(uint64_t));
    }

    uint64_t root[V / 2];
    if (Pi == 1) memcpy(root, cvs, half * sizeof(uint64_t));
    else         tree(cvs, 0, Pi, 1, root, &P);
    free(cvs);

    /* serialize root little-endian, truncate to n bits */
    unsigned wb = (unsigned)(P.w / 8);
    int out_bytes = P.n / 8, produced = 0, j, b;
    for (j = 0; j < (int)half && produced < out_bytes; j++)
        for (b = 0; b < (int)wb && produced < out_bytes; b++)
            digest[produced++] = (unsigned char)((root[j] >> (8 * b)) & 0xFF);
    return 0;
}
