/* API_PKC pseudoXOF adapter for BIKE's stateful PRF interface. */

#include <limits.h>
#include <stdlib.h>

#include "auxfunc.h"
#include "cleanup.h"
#include "prf_internal.h"
#include "utilities.h"

#define AUX_XOF_BLOCK_BYTES 32ULL
#define AUX_PRF_WORDS                                                      \
  (((2ULL * D) > MAX_RAND_INDICES_T) ? (2ULL * D) : MAX_RAND_INDICES_T)
#define AUX_PRF_INITIAL_BYTES (AUX_PRF_WORDS * sizeof(idx_t))

static ret_t fill_prf_buffer(IN OUT aux_prf_state_t *s, IN size_t new_size)
{
  uint8_t *new_buffer;

  if(new_size == 0 || new_size > s->max_output_bytes ||
     new_size > (ULLONG_MAX / 8ULL)) {
    BIKE_ERROR(E_AUX_PRF_OVER_USED);
  }

  new_buffer = (uint8_t *)malloc(new_size);
  if(new_buffer == NULL) {
    BIKE_ERROR(E_AUX_PRF_INIT_FAIL);
  }

  if(pseudoXOF(((unsigned long long)new_size) * 8ULL, s->seed.raw,
               sizeof(s->seed) * 8ULL, new_buffer) != 0) {
    secure_clean(new_buffer, (uint32_t)new_size);
    free(new_buffer);
    BIKE_ERROR(E_AUX_PRF_INIT_FAIL);
  }

  if(s->buffer != NULL) {
    secure_clean(s->buffer, (uint32_t)s->buffer_size);
    free(s->buffer);
  }

  s->buffer      = new_buffer;
  s->buffer_size = new_size;
  return SUCCESS;
}

ret_t init_prf_state(OUT aux_prf_state_t *s, IN size_t max_num_invocations,
                     IN const seed_t *seed)
{
  size_t initial_size = AUX_PRF_INITIAL_BYTES;

  if(max_num_invocations == 0 || seed == NULL) {
    BIKE_ERROR(E_AUX_PRF_INIT_FAIL);
  }

  bike_memset(s, 0, sizeof(*s));
  s->seed = *seed;

  if(max_num_invocations > (UINT32_MAX / AUX_XOF_BLOCK_BYTES)) {
    s->max_output_bytes = UINT32_MAX;
  } else {
    s->max_output_bytes = max_num_invocations * AUX_XOF_BLOCK_BYTES;
  }

  if(initial_size > s->max_output_bytes) {
    initial_size = s->max_output_bytes;
  }

  return fill_prf_buffer(s, initial_size);
}

ret_t get_prf_output(OUT uint8_t *out, IN OUT aux_prf_state_t *s, IN size_t len)
{
  size_t required_size;
  size_t new_size;

  if(out == NULL || s == NULL || len > (SIZE_MAX - s->curr_pos)) {
    BIKE_ERROR(E_AUX_PRF_OVER_USED);
  }

  required_size = s->curr_pos + len;
  if(required_size > s->max_output_bytes) {
    BIKE_ERROR(E_AUX_PRF_OVER_USED);
  }

  if(required_size > s->buffer_size) {
    new_size = s->buffer_size;
    while(new_size < required_size) {
      if(new_size > (s->max_output_bytes / 2)) {
        new_size = s->max_output_bytes;
      } else {
        new_size *= 2;
      }
    }
    GUARD(fill_prf_buffer(s, new_size));
  }

  bike_memcpy(out, &s->buffer[s->curr_pos], len);
  s->curr_pos += len;
  return SUCCESS;
}

void clean_prf_state(IN OUT aux_prf_state_t *s)
{
  if(s->buffer != NULL) {
    secure_clean(s->buffer, (uint32_t)s->buffer_size);
    free(s->buffer);
  }
  secure_clean((uint8_t *)s, sizeof(*s));
}
