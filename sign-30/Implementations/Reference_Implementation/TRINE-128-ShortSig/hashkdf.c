#include "hashkdf.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void trine_clear(void *ptr, size_t len)
{
  volatile uint8_t *bytes = (volatile uint8_t *)ptr;

  if (ptr == NULL)
    return;

  for (size_t i = 0; i < len; i++)
    bytes[i] = 0;
}

#ifdef USE_ICCS
static int size_to_bits_ull(
    unsigned long long *out_bits,
    size_t byte_len)
{
  if (out_bits == NULL)
    return -1;

  if (byte_len > (size_t)(ULLONG_MAX / 8u))
    return -1;

  *out_bits = (unsigned long long)byte_len * 8u;
  return 0;
}

static int grow_buffer(
    uint8_t **buffer,
    size_t *capacity,
    size_t required)
{
  size_t new_capacity;
  uint8_t *new_buffer;

  if (buffer == NULL || capacity == NULL)
    return -1;

  if (required <= *capacity)
    return 0;

  new_capacity = *capacity == 0u ? 64u : *capacity;
  while (new_capacity < required)
  {
    if (new_capacity > SIZE_MAX / 2u)
    {
      new_capacity = required;
      break;
    }
    new_capacity *= 2u;
  }

  new_buffer = (uint8_t *)realloc(*buffer, new_capacity);
  if (new_buffer == NULL)
    return -1;

  if (new_capacity > *capacity)
    memset(new_buffer + *capacity, 0, new_capacity - *capacity);

  *buffer = new_buffer;
  *capacity = new_capacity;
  return 0;
}

static int append_input(
    uint8_t **buffer,
    size_t *len,
    size_t *capacity,
    const uint8_t *input,
    size_t input_len)
{
  if (input == NULL && input_len != 0u)
    return -1;

  if (len == NULL || input_len > SIZE_MAX - *len)
    return -1;

  if (grow_buffer(buffer, capacity, *len + input_len) != 0)
    return -1;

  if (input_len != 0u)
    memcpy(*buffer + *len, input, input_len);
  *len += input_len;
  return 0;
}
#endif

int trine_xof_init(trine_xof_state *state)
{
  if (state == NULL)
    return -1;

  memset(state, 0, sizeof(*state));
#ifdef USE_SHA3
  shake256_init(&state->shake);
#endif
  return 0;
}

int trine_xof_absorb(
    trine_xof_state *state,
    const uint8_t *input,
    size_t input_len)
{
  if (state == NULL || (input == NULL && input_len != 0u))
    return -1;

  if (state->finalized)
    return -1;

#ifdef USE_SHA3
  shake256_absorb(&state->shake, input, input_len);
  return 0;
#else
  if (state->failed)
    return -1;

  if (append_input(
          &state->input,
          &state->input_len,
          &state->input_capacity,
          input,
          input_len) != 0)
  {
    state->failed = 1;
    return -1;
  }

  return 0;
#endif
}

int trine_xof_finalize(trine_xof_state *state)
{
  if (state == NULL)
    return -1;

  if (state->finalized)
    return -1;

#ifdef USE_SHA3
  shake256_finalize(&state->shake);
#else
  if (state->failed)
    return -1;
#endif

  state->finalized = 1;
  return 0;
}

int trine_xof_squeeze(
    trine_xof_state *state,
    uint8_t *output,
    size_t output_len)
{
  if (state == NULL || (output == NULL && output_len != 0u))
    return -1;

  if (!state->finalized)
    return -1;

  if (output_len == 0u)
    return 0;

#ifdef USE_SHA3
  shake256_squeeze(output, output_len, &state->shake);
  return 0;
#else
  size_t required;
  unsigned long long output_bits;
  unsigned long long input_bits;

  if (state->failed)
    return -1;

  if (output_len > SIZE_MAX - state->output_offset)
  {
    state->failed = 1;
    return -1;
  }
  required = state->output_offset + output_len;

  if (required > state->output_capacity)
  {
    if (grow_buffer(
            &state->output_cache,
            &state->output_capacity,
            required) != 0 ||
        size_to_bits_ull(&output_bits, state->output_capacity) != 0 ||
        size_to_bits_ull(&input_bits, state->input_len) != 0 ||
        pseudoXOF(
            output_bits,
            state->input,
            input_bits,
            state->output_cache) != 0)
    {
      state->failed = 1;
      return -1;
    }
  }

  memcpy(output, state->output_cache + state->output_offset, output_len);
  state->output_offset += output_len;
  return 0;
#endif
}

void trine_xof_release(trine_xof_state *state)
{
  if (state == NULL)
    return;

#ifdef USE_SHA3
  trine_clear(&state->shake, sizeof(state->shake));
#else
  trine_clear(state->input, state->input_capacity);
  trine_clear(state->output_cache, state->output_capacity);
  free(state->input);
  free(state->output_cache);
#endif
  memset(state, 0, sizeof(*state));
}

int trine_xof_once(
    uint8_t *output,
    size_t output_len,
    const uint8_t *input,
    size_t input_len)
{
  if ((output == NULL && output_len != 0u) ||
      (input == NULL && input_len != 0u))
    return -1;

#ifdef USE_SHA3
  shake256(output, output_len, input, input_len);
  return 0;
#else
  unsigned long long output_bits;
  unsigned long long input_bits;

  if (size_to_bits_ull(&output_bits, output_len) != 0 ||
      size_to_bits_ull(&input_bits, input_len) != 0)
    return -1;

  return pseudoXOF(output_bits, input, input_bits, output) == 0 ? 0 : -1;
#endif
}

int trine_hash_init(trine_hash_state *state)
{
  if (state == NULL)
    return -1;

  memset(state, 0, sizeof(*state));
#ifdef USE_SHA3
  shake256_init(&state->shake);
#endif
  return 0;
}

int trine_hash_absorb(
    trine_hash_state *state,
    const uint8_t *input,
    size_t input_len)
{
  if (state == NULL || (input == NULL && input_len != 0u))
    return -1;

  if (state->finalized)
    return -1;

#ifdef USE_SHA3
  shake256_absorb(&state->shake, input, input_len);
  return 0;
#else
  if (state->failed)
    return -1;

  if (append_input(
          &state->input,
          &state->input_len,
          &state->input_capacity,
          input,
          input_len) != 0)
  {
    state->failed = 1;
    return -1;
  }

  return 0;
#endif
}

int trine_hash_finalize(
    trine_hash_state *state,
    uint8_t *output,
    size_t output_len)
{
  if (state == NULL || (output == NULL && output_len != 0u))
    return -1;

  if (state->finalized)
    return -1;

#ifdef USE_SHA3
  shake256_finalize(&state->shake);
  shake256_squeeze(output, output_len, &state->shake);
  state->finalized = 1;
  return 0;
#else
  unsigned long long input_bits;
  int ret;

  if (state->failed)
    return -1;

  if (size_to_bits_ull(&input_bits, state->input_len) != 0)
  {
    state->failed = 1;
    return -1;
  }

  if (output_len == 32u)
    ret = sm3hash(256, state->input, input_bits, output);
  else if (output_len == 64u)
    ret = pseudohash(512, state->input, input_bits, output);
  else if (output_len == 128u)
    ret = pseudohash(1024, state->input, input_bits, output);
  else
    ret = -1;

  if (ret != 0)
  {
    state->failed = 1;
    return -1;
  }

  state->finalized = 1;
  return 0;
#endif
}

void trine_hash_release(trine_hash_state *state)
{
  if (state == NULL)
    return;

#ifdef USE_SHA3
  trine_clear(&state->shake, sizeof(state->shake));
#else
  trine_clear(state->input, state->input_capacity);
  free(state->input);
#endif
  memset(state, 0, sizeof(*state));
}
