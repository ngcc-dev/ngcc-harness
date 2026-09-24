#include <stdint.h>
#include "BWcoding.h"

#define BWKEM128_CODEC_BLOCK_COEFFS 8
#define BWKEM128_CODEC_MESSAGE_BITS_PER_BLOCK 4
#define BWKEM128_CODEC_MESSAGE_BLOCKS (BWKEM128_N / BWKEM128_CODEC_BLOCK_COEFFS)
#define BWKEM128_CODEC_LEVEL (KYBER_Q >> 1)
#define BWKEM128_CODEC_QHAT_LEVEL BWKEM128_QHAT_HALF

#if defined(__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
#define BWKEM128_DECODE_IN_ASM 1
#endif

#ifndef BWKEM128_DECODE_IN_ASM

static uint32_t bwkem128_ct_lt_u32(uint32_t a, uint32_t b)
{
  return (uint32_t)(((uint64_t)a - (uint64_t)b) >> 63);
}

static uint32_t bwkem128_ct_sel_u32(uint32_t mask, uint32_t a, uint32_t b)
{
  return (mask & a) | (~mask & b);
}

static uint8_t bwkem128_ct_sel_u8(uint32_t mask, uint8_t a, uint8_t b)
{
  uint8_t byte_mask;

  byte_mask = (uint8_t)mask;
  return (uint8_t)((byte_mask & a) | ((uint8_t)~byte_mask & b));
}

#define BWKEM128_QHAT16 0x10001000u
#define BWKEM128_HALF16 0x08000800u
#define BWKEM128_MASK12 0x0FFF0FFFu

#if defined(__ARM_FEATURE_DSP) && (__ARM_FEATURE_DSP == 1)
static inline uint32_t bwkem128_usub16(uint32_t a, uint32_t b)
{
  uint32_t r;
  __asm__("usub16 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
  return r;
}
static inline uint32_t bwkem128_ssub16(uint32_t a, uint32_t b)
{
  uint32_t r;
  __asm__("ssub16 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
  return r;
}
static inline uint32_t bwkem128_umin16(uint32_t a, uint32_t b)
{
  uint32_t r;
  __asm__("usub16 %0, %1, %2\n\tsel %0, %2, %1"
          : "=&r"(r) : "r"(a), "r"(b) : "cc");
  return r;
}
static inline uint32_t bwkem128_sqbb(uint32_t a)
{
  uint32_t r;
  __asm__("smulbb %0, %1, %1" : "=r"(r) : "r"(a));
  return r;
}
static inline uint32_t bwkem128_sqtt(uint32_t a)
{
  uint32_t r;
  __asm__("smultt %0, %1, %1" : "=r"(r) : "r"(a));
  return r;
}
#else
static inline uint32_t bwkem128_usub16(uint32_t a, uint32_t b)
{
  uint32_t lo = (a & 0xFFFFu) - (b & 0xFFFFu);
  uint32_t hi = (a >> 16) - (b >> 16);
  return (lo & 0xFFFFu) | (hi << 16);
}
static inline uint32_t bwkem128_ssub16(uint32_t a, uint32_t b)
{
  return bwkem128_usub16(a, b);
}
static inline uint32_t bwkem128_umin16(uint32_t a, uint32_t b)
{
  uint16_t al = (uint16_t)a, ah = (uint16_t)(a >> 16);
  uint16_t bl = (uint16_t)b, bh = (uint16_t)(b >> 16);
  uint16_t lo = al < bl ? al : bl;
  uint16_t hi = ah < bh ? ah : bh;
  return ((uint32_t)hi << 16) | lo;
}
static inline uint32_t bwkem128_sqbb(uint32_t a)
{
  int32_t x = (int16_t)a;
  return (uint32_t)(x * x);
}
static inline uint32_t bwkem128_sqtt(uint32_t a)
{
  int32_t x = (int16_t)(a >> 16);
  return (uint32_t)(x * x);
}
#endif
#endif /* !BWKEM128_DECODE_IN_ASM (ct + DSP helpers) */

static uint8_t bwkem128_encode_codeword(uint8_t nibble)
{
  static const uint8_t generator_masks[4] = {0x55u, 0x0fu, 0x3cu, 0xf0u};
  unsigned int i;
  uint8_t codeword;

  codeword = 0;
  nibble &= 0x0fu;
  for(i = 0; i < 4; i++) {
    uint8_t bit;
    uint8_t mask;

    bit = (uint8_t)((nibble >> i) & 1u);
    mask = (uint8_t)(-(int8_t)bit);
    codeword ^= (uint8_t)(mask & generator_masks[i]);
  }

  return codeword;
}

#ifndef BWKEM128_DECODE_IN_ASM
static uint8_t bwkem128_d8_labels_to_nibble(uint8_t labels, uint32_t coset10)
{
  uint8_t nibble;

  nibble = (uint8_t)((((labels ^ (labels << 1)) & 0x3u) |
                      ((labels >> 1) & 0x4u)) << 1);
  return (uint8_t)(nibble | (uint8_t)coset10);
}
#endif /* !BWKEM128_DECODE_IN_ASM (d8_labels_to_nibble) */

static void bwkem128_encode_block(int16_t out[BWKEM128_CODEC_BLOCK_COEFFS],
                                  uint8_t nibble)
{
  unsigned int i;
  uint8_t codeword;

  codeword = bwkem128_encode_codeword(nibble);      /* 4-bit -> 8-bit */
  for(i = 0; i < BWKEM128_CODEC_BLOCK_COEFFS; i++) {
    out[i] = (int16_t)(((codeword >> i) & 1u) * BWKEM128_CODEC_LEVEL);
  }
}

#ifndef BWKEM128_DECODE_IN_ASM
#define BWKEM128_D8_STEP(tot, pl, pa, bd, bp, cl0, cl1, k)                  \
  do {                                                                     \
    uint32_t _t  = bwkem128_ct_lt_u32((cl1), (cl0));                       \
    uint32_t _m  = (uint32_t)-(int32_t)_t;                                 \
    uint32_t _ch = bwkem128_ct_sel_u32(_m, (cl1), (cl0));                  \
    uint32_t _ot = bwkem128_ct_sel_u32(_m, (cl0), (cl1));                  \
    uint32_t _dl = _ot - _ch;                                             \
    uint8_t  _lb = bwkem128_ct_sel_u8(_m, 1u, 0u);                         \
    uint32_t _td, _dm;                                                     \
    (pl)  |= (uint8_t)(_lb << (k));                                        \
    (pa)  ^= _lb;                                                          \
    (tot) += _ch;                                                          \
    _td = bwkem128_ct_lt_u32(_dl, (bd));                                   \
    _dm = (uint32_t)-(int32_t)_td;                                         \
    (bd) = bwkem128_ct_sel_u32(_dm, _dl, (bd));                            \
    (bp) = bwkem128_ct_sel_u32(_dm, (k), (bp));                            \
  } while(0)

#define BWKEM128_D8_FINISH(nib, tot, pl, pa, bd, bp, coset)                 \
  do {                                                                     \
    uint32_t _pm = (uint32_t)-(int32_t)((pa) & 1u);                        \
    (tot) += (bd) & _pm;                                                   \
    (pl)  ^= (uint8_t)(_pm & (1u << (bp)));                                \
    (nib)  = bwkem128_d8_labels_to_nibble((pl), (coset));                  \
  } while(0)

static uint8_t bwkem128_decode_block(const int16_t in[BWKEM128_CODEC_BLOCK_COEFFS])
{
  uint32_t tot0 = 0u, tot1 = 0u;
  uint32_t bd0 = ~0u, bd1 = ~0u;
  uint32_t bp0 = 0u, bp1 = 0u;
  uint8_t pl0 = 0u, pl1 = 0u;
  uint8_t pa0 = 0u, pa1 = 0u;
  uint8_t nib0, nib1;
  unsigned int k;

  for(k = 0; k < BWKEM128_CODEC_BLOCK_COEFFS / 2; k++) {
    uint32_t packed;
    uint32_t dist0;
    uint32_t dist1;
    uint32_t de0, do0, de1, do1;

    packed = (((uint32_t)(uint16_t)in[2u * k + 1u] << 16) |
              (uint16_t)in[2u * k]) & BWKEM128_MASK12;
    dist0 = bwkem128_umin16(packed, bwkem128_usub16(BWKEM128_QHAT16, packed));
    dist1 = bwkem128_ssub16(packed, BWKEM128_HALF16);
    de0 = bwkem128_sqbb(dist0);
    do0 = bwkem128_sqtt(dist0);
    de1 = bwkem128_sqbb(dist1);
    do1 = bwkem128_sqtt(dist1);

    /* coset0: label0->(0,0) label1->(1,1)；coset1: label0->(1,0) label1->(0,1) */
    BWKEM128_D8_STEP(tot0, pl0, pa0, bd0, bp0, de0 + do0, de1 + do1, k);
    BWKEM128_D8_STEP(tot1, pl1, pa1, bd1, bp1, de1 + do0, de0 + do1, k);
  }

  BWKEM128_D8_FINISH(nib0, tot0, pl0, pa0, bd0, bp0, 0u);
  BWKEM128_D8_FINISH(nib1, tot1, pl1, pa1, bd1, bp1, 1u);

  {
    uint32_t take_second = bwkem128_ct_lt_u32(tot1, tot0);
    uint32_t select_mask = (uint32_t)-(int32_t)take_second;
    return bwkem128_ct_sel_u8(select_mask, nib1, nib0);
  }
}

#undef BWKEM128_D8_STEP
#undef BWKEM128_D8_FINISH
#endif /* !BWKEM128_DECODE_IN_ASM (decode_block) */

void codec_encode(int16_t out[BWKEM128_N],
                  const uint8_t msg[BWKEM128_INDCPA_MSGBYTES])
{
  unsigned int i;

  for(i = 0; i < BWKEM128_INDCPA_MSGBYTES; i++) {
    unsigned int block_index;
    uint8_t lo;
    uint8_t hi;

    block_index = 2u * i;
    lo = (uint8_t)(msg[i] & 0x0fu);
    hi = (uint8_t)(msg[i] >> BWKEM128_CODEC_MESSAGE_BITS_PER_BLOCK);
    bwkem128_encode_block(out + block_index * BWKEM128_CODEC_BLOCK_COEFFS, lo);
    bwkem128_encode_block(out + (block_index + 1u) * BWKEM128_CODEC_BLOCK_COEFFS, hi);
  }
}

#ifndef BWKEM128_DECODE_IN_ASM
void codec_decode(uint8_t msg[BWKEM128_INDCPA_MSGBYTES],
                  const int16_t in[BWKEM128_N])
{
  unsigned int i;

  for(i = 0; i < BWKEM128_INDCPA_MSGBYTES; i++) {
    unsigned int block_index;
    uint8_t lo;
    uint8_t hi;

    block_index = 2u * i;
    lo = bwkem128_decode_block(in + block_index * BWKEM128_CODEC_BLOCK_COEFFS);
    hi = bwkem128_decode_block(in + (block_index + 1u) * BWKEM128_CODEC_BLOCK_COEFFS);
    msg[i] = (uint8_t)(lo | (uint8_t)(hi << BWKEM128_CODEC_MESSAGE_BITS_PER_BLOCK));
  }
}
#endif /* !BWKEM128_DECODE_IN_ASM (codec_decode) */
