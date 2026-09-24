#include <stdint.h>
#include "BWcoding.h"

#define BWKEM128_CODEC_BLOCK_COEFFS 8
#define BWKEM128_CODEC_MESSAGE_BITS_PER_BLOCK 4
#define BWKEM128_CODEC_MESSAGE_BLOCKS (BWKEM128_N / BWKEM128_CODEC_BLOCK_COEFFS)
#define BWKEM128_CODEC_LEVEL (KYBER_Q >> 1)
#define BWKEM128_CODEC_QHAT_LEVEL BWKEM128_QHAT_HALF

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

static uint32_t bwkem128_abs_i32(int32_t x)
{
  uint32_t mask;

  mask = (uint32_t)(x >> 31);
  return (uint32_t)((x ^ (int32_t)mask) - (int32_t)mask);
}

static uint32_t bwkem128_abs_qhat(uint32_t x)
{
  uint32_t wrap;
  uint32_t take_wrap;
  uint32_t mask;

  wrap = BWKEM128_QHAT - x;
  take_wrap = bwkem128_ct_lt_u32(wrap, x);
  mask = (uint32_t)-(int32_t)take_wrap;
  return bwkem128_ct_sel_u32(mask, wrap, x);
}

static void bwkem128_coord_costs(uint32_t coord_costs[BWKEM128_CODEC_BLOCK_COEFFS][2],
                                 const int16_t in[BWKEM128_CODEC_BLOCK_COEFFS])
{
  unsigned int i;

  for(i = 0; i < BWKEM128_CODEC_BLOCK_COEFFS; i++) {
    uint32_t value;
    uint32_t dist0;
    uint32_t dist1;

    value = ((uint32_t)(uint16_t)in[i]) & (BWKEM128_QHAT - 1u);
    dist0 = bwkem128_abs_qhat(value);
    dist1 = bwkem128_abs_i32((int32_t)value - (int32_t)BWKEM128_CODEC_QHAT_LEVEL);
    coord_costs[i][0] = dist0 * dist0;
    coord_costs[i][1] = dist1 * dist1;
  }
}

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

static void bwkem128_decode_d8(uint32_t *cost,
                               uint8_t *labels,
                               uint32_t coord_costs[BWKEM128_CODEC_BLOCK_COEFFS][2],
                               uint32_t coset10)
{
  unsigned int pair_index;
  uint8_t pair_labels;
  uint8_t parity;
  uint32_t total_cost;
  uint32_t best_delta;
  uint32_t best_pair;

  pair_labels = 0;
  parity = 0;
  total_cost = 0;
  best_delta = ~0u;
  best_pair = 0;

  for(pair_index = 0; pair_index < BWKEM128_CODEC_BLOCK_COEFFS / 2; pair_index++) {
    unsigned int even_index;
    unsigned int odd_index;
    uint32_t cost_label0;
    uint32_t cost_label1;
    uint32_t take_label1;
    uint32_t select_mask;
    uint32_t chosen_cost;
    uint32_t other_cost;
    uint32_t delta;
    uint32_t take_delta;
    uint32_t delta_mask;
    uint8_t chosen_label;

    even_index = 2u * pair_index;
    odd_index = even_index + 1u;

    if(coset10 == 0u) {
      cost_label0 = coord_costs[even_index][0] + coord_costs[odd_index][0];
      cost_label1 = coord_costs[even_index][1] + coord_costs[odd_index][1];
    } else {
      cost_label0 = coord_costs[even_index][1] + coord_costs[odd_index][0];
      cost_label1 = coord_costs[even_index][0] + coord_costs[odd_index][1];
    }

    take_label1 = bwkem128_ct_lt_u32(cost_label1, cost_label0);
    select_mask = (uint32_t)-(int32_t)take_label1;
    chosen_cost = bwkem128_ct_sel_u32(select_mask, cost_label1, cost_label0);
    other_cost = bwkem128_ct_sel_u32(select_mask, cost_label0, cost_label1);
    delta = other_cost - chosen_cost;
    chosen_label = bwkem128_ct_sel_u8(select_mask, 1u, 0u);

    pair_labels |= (uint8_t)(chosen_label << pair_index);
    parity ^= chosen_label;
    total_cost += chosen_cost;

    take_delta = bwkem128_ct_lt_u32(delta, best_delta);
    delta_mask = (uint32_t)-(int32_t)take_delta;
    best_delta = bwkem128_ct_sel_u32(delta_mask, delta, best_delta);
    best_pair = bwkem128_ct_sel_u32(delta_mask, pair_index, best_pair);
  }

  {
    uint32_t parity_mask;

    parity_mask = (uint32_t)-(int32_t)(parity & 1u);
    total_cost += best_delta & parity_mask;
    pair_labels ^= (uint8_t)(parity_mask & (1u << best_pair));
  }

  *cost = total_cost;
  *labels = pair_labels;
}

static uint8_t bwkem128_d8_labels_to_nibble(uint8_t labels, uint32_t coset10)
{
  uint8_t nibble;

  nibble = (uint8_t)((((labels ^ (labels << 1)) & 0x3u) |
                      ((labels >> 1) & 0x4u)) << 1);
  return (uint8_t)(nibble | (uint8_t)coset10);
}

static void bwkem128_encode_block(int16_t out[BWKEM128_CODEC_BLOCK_COEFFS],
                                  uint8_t nibble)
{
  unsigned int i;
  uint8_t codeword;

  codeword = bwkem128_encode_codeword(nibble);
  for(i = 0; i < BWKEM128_CODEC_BLOCK_COEFFS; i++) {
    out[i] = (int16_t)(((codeword >> i) & 1u) * BWKEM128_CODEC_LEVEL);
  }
}

static uint8_t bwkem128_decode_block(const int16_t in[BWKEM128_CODEC_BLOCK_COEFFS])
{
  uint32_t coord_costs[BWKEM128_CODEC_BLOCK_COEFFS][2];
  uint32_t d8_cost[2];
  uint8_t d8_labels[2];
  uint8_t nibble_candidates[2];
  uint32_t take_second;
  uint32_t select_mask;
  uint8_t best_nibble;

  bwkem128_coord_costs(coord_costs, in);
  bwkem128_decode_d8(&d8_cost[0], &d8_labels[0], coord_costs, 0u);
  bwkem128_decode_d8(&d8_cost[1], &d8_labels[1], coord_costs, 1u);
  nibble_candidates[0] = bwkem128_d8_labels_to_nibble(d8_labels[0], 0u);
  nibble_candidates[1] = bwkem128_d8_labels_to_nibble(d8_labels[1], 1u);

  take_second = bwkem128_ct_lt_u32(d8_cost[1], d8_cost[0]);
  select_mask = (uint32_t)-(int32_t)take_second;
  best_nibble = bwkem128_ct_sel_u8(select_mask,
                                   nibble_candidates[1],
                                   nibble_candidates[0]);
  return best_nibble;
}

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
