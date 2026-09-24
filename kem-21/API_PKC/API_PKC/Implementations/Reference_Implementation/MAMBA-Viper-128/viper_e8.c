#include "viper_e8.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint16_t modq_i32(int32_t x) { return (uint16_t)(x & VIPER_QMASK); }
static int32_t centered_u16(uint16_t x) {
  int32_t v = (int32_t)(x & VIPER_QMASK);
  if (v >= VIPER_Q / 2) v -= VIPER_Q;
  return v;
}

static unsigned e8_modulus(void) { return 2u * (unsigned)(VIPER_Q / VIPER_E8_ALPHA); }

static unsigned e8_count_cache[9][2][4];
static int e8_count_ready = 0;

static void e8_init_counts(void) {
  if (e8_count_ready) return;
  unsigned mod = e8_modulus();
  for (unsigned parity = 0; parity < 2; parity++) e8_count_cache[0][parity][0] = 1;
  for (unsigned rem = 1; rem <= 8; rem++) {
    for (unsigned parity = 0; parity < 2; parity++) {
      for (unsigned sum = 0; sum < 4; sum++) {
        unsigned count = 0;
        for (unsigned v = parity; v < mod; v += 2) count += e8_count_cache[rem - 1][parity][(sum + v) & 3u];
        e8_count_cache[rem][parity][sum] = count;
      }
    }
  }
  e8_count_ready = 1;
}

static unsigned e8_count_suffix(unsigned remaining, unsigned parity, unsigned sum_mod4) {
  e8_init_counts();
  return e8_count_cache[remaining][parity & 1u][sum_mod4 & 3u];
}

static int e8_valid_residue_vector(const unsigned d[8]) {
  unsigned parity = d[0] & 1u, sum = 0;
  for (unsigned i = 0; i < 8; i++) {
    if ((d[i] & 1u) != parity) return 0;
    sum += d[i];
  }
  return (sum & 3u) == 0;
}

static void e8_unrank(uint8_t out[8], unsigned label) {
  unsigned mod = e8_modulus();
  unsigned labels = 1u << VIPER_E8_BITS_PER_BLOCK;
  unsigned rank = label & (labels - 1u);
  unsigned sum = 0;
  int parity = -1;
  for (unsigned pos = 0; pos < 8; pos++) {
    for (unsigned v = 0; v < mod; v++) {
      if (parity >= 0 && (int)(v & 1u) != parity) continue;
      unsigned next_parity = (parity >= 0) ? (unsigned)parity : (v & 1u);
      unsigned cnt = e8_count_suffix(7 - pos, next_parity, (sum + v) & 3u);
      if (rank >= cnt) {
        rank -= cnt;
      } else {
        out[pos] = (uint8_t)v;
        sum = (sum + v) & 3u;
        parity = (int)next_parity;
        break;
      }
    }
  }
}

static unsigned e8_rank(const unsigned d[8]) {
  unsigned mod = e8_modulus();
  if (!e8_valid_residue_vector(d)) return 0;
  unsigned rank = 0, sum = 0;
  int parity = -1;
  for (unsigned pos = 0; pos < 8; pos++) {
    for (unsigned v = 0; v < d[pos]; v++) {
      if (parity >= 0 && (int)(v & 1u) != parity) continue;
      unsigned next_parity = (parity >= 0) ? (unsigned)parity : (v & 1u);
      rank += e8_count_suffix(7 - pos, next_parity, (sum + v) & 3u);
    }
    if (d[pos] >= mod) return 0;
    if (parity < 0) parity = (int)(d[pos] & 1u);
    if ((int)(d[pos] & 1u) != parity) return 0;
    sum = (sum + d[pos]) & 3u;
  }
  return rank;
}

uint16_t viper_e8_label_to_coeff(unsigned label, unsigned coord) {
  uint8_t d[8];
  e8_unrank(d, label);
  coord &= 7u;
  return modq_i32((int32_t)d[coord] * (int32_t)(VIPER_E8_ALPHA / 2));
}

unsigned viper_e8_coeffs_to_label(const uint16_t coeffs[8]) {
  unsigned d[8];
  for (unsigned i = 0; i < 8; i++) {
    int32_t c = centered_u16(coeffs[i]);
    int32_t half = VIPER_E8_ALPHA / 2;
    int32_t q = c / half;
    int32_t r = c % half;
    if (r < 0) { r += half; q--; }
    if (2 * r >= half) q++;
    d[i] = (unsigned)q & (e8_modulus() - 1u);
  }
  return e8_rank(d);
}

static unsigned e8_label_from_doubled(const int d[8]) {
  unsigned u[8];
  unsigned mod = e8_modulus();
  for (unsigned i = 0; i < 8; i++) u[i] = (unsigned)d[i] & (mod - 1u);
  return e8_rank(u);
}

static int32_t div_floor_i32(int32_t a, int32_t b) {
  int32_t q = a / b, r = a % b;
  if (r != 0 && ((r < 0) != (b < 0))) q--;
  return q;
}

static int32_t round_div_i32(int32_t a, int32_t b) {
  int32_t q = div_floor_i32(a, b);
  int32_t lo = q * b;
  int32_t hi = lo + b;
  return ((int64_t)a - lo <= (int64_t)hi - a) ? q : q + 1;
}

static uint64_t d8_candidate(int out_d[8], const int32_t c[8], int half_shift) {
  int z[8];
  int sum = 0;
  uint64_t dist = 0;
  for (unsigned i = 0; i < 8; i++) {
    int32_t target = c[i] - (half_shift ? (int32_t)(VIPER_E8_ALPHA / 2) : 0);
    z[i] = (int)round_div_i32(target, VIPER_E8_ALPHA);
    sum += z[i];
  }
  if (sum & 1) {
    uint64_t best_penalty = UINT64_MAX;
    unsigned best_i = 0;
    int best_delta = 1;
    for (unsigned i = 0; i < 8; i++) {
      int32_t base = ((2 * z[i]) + half_shift) * (int32_t)(VIPER_E8_ALPHA / 2);
      int32_t plus = base + VIPER_E8_ALPHA;
      int32_t minus = base - VIPER_E8_ALPHA;
      uint64_t cur = (uint64_t)((int64_t)c[i] - base) * ((int64_t)c[i] - base);
      uint64_t ppen = (uint64_t)((int64_t)c[i] - plus) * ((int64_t)c[i] - plus) - cur;
      uint64_t mpen = (uint64_t)((int64_t)c[i] - minus) * ((int64_t)c[i] - minus) - cur;
      if (ppen < best_penalty || (ppen == best_penalty && i < best_i)) { best_penalty = ppen; best_i = i; best_delta = 1; }
      if (mpen < best_penalty || (mpen == best_penalty && i < best_i)) { best_penalty = mpen; best_i = i; best_delta = -1; }
    }
    z[best_i] += best_delta;
  }
  for (unsigned i = 0; i < 8; i++) {
    out_d[i] = 2 * z[i] + half_shift;
    int32_t val = out_d[i] * (int32_t)(VIPER_E8_ALPHA / 2);
    int64_t diff = (int64_t)c[i] - val;
    dist += (uint64_t)(diff * diff);
  }
  return dist;
}

static void nearest_e8_doubled(int out_d[8], const uint16_t coeffs[8]) {
  int32_t c[8];
  int d0[8], d1[8];
  for (unsigned i = 0; i < 8; i++) c[i] = centered_u16(coeffs[i]);
  uint64_t dist0 = d8_candidate(d0, c, 0);
  uint64_t dist1 = d8_candidate(d1, c, 1);
  const int *src = (dist0 <= dist1) ? d0 : d1;
  for (unsigned i = 0; i < 8; i++) out_d[i] = src[i];
}

void viper_e8_encode(vpoly out, const unsigned char m[VIPER_MSGBYTES]) {
  memset(out, 0, sizeof(vpoly));
  const unsigned mask = (1u << VIPER_E8_BITS_PER_BLOCK) - 1u;
  for (unsigned block = 0; block < VIPER_E8_ACTIVE_BLOCKS; block++) {
    unsigned label = 0;
    for (unsigned b = 0; b < VIPER_E8_BITS_PER_BLOCK; b++) {
      unsigned bit = block * VIPER_E8_BITS_PER_BLOCK + b;
      label |= (unsigned)((m[bit >> 3] >> (bit & 7)) & 1u) << b;
    }
    label &= mask;
    for (unsigned j = 0; j < 8; j++) out[8 * block + j] = viper_e8_label_to_coeff(label, j);
  }
}

void viper_e8_decode(unsigned char m[VIPER_MSGBYTES], const vpoly in) {
  memset(m, 0, VIPER_MSGBYTES);
  for (unsigned block = 0; block < VIPER_E8_ACTIVE_BLOCKS; block++) {
    uint16_t coeffs[8];
    int d[8];
    for (unsigned j = 0; j < 8; j++) coeffs[j] = in[8 * block + j];
    nearest_e8_doubled(d, coeffs);
    unsigned label = e8_label_from_doubled(d);
    for (unsigned b = 0; b < VIPER_E8_BITS_PER_BLOCK; b++) {
      unsigned bit = block * VIPER_E8_BITS_PER_BLOCK + b;
      if (bit < VIPER_MSGBITS) m[bit >> 3] |= (unsigned char)(((label >> b) & 1u) << (bit & 7));
    }
  }
}

const char *viper_e8_labeling_name(void) { return "lexicographic E8/(q/alpha)Z^8 doubled-coordinate residues"; }
