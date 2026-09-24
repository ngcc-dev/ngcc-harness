#include "macros.h"
#include "compat.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/* The fallback implementation tries to be as generic as possible. While all callers in this code
 * base satisfy all requirements, we still check them. Thereby, the fallback implementation may be
 * of use for others as well. */
void* aligned_alloc(size_t alignment, size_t size) {
  /* check alignment (power of 2) and size (multiple of alignment) */
  if (alignment & (alignment - 1) || size & (alignment - 1)) {
    errno = EINVAL;
    return NULL;
  }

  if (!size) {
    return NULL;
  }
  const size_t offset = alignment - 1 + sizeof(uint8_t);
  uint8_t* buffer     = malloc(size + offset);
  if (!buffer) {
    return NULL;
  }

  uint8_t* ptr   = (uint8_t*)(((uintptr_t)(buffer) + offset) & ~(alignment - 1));
  ptrdiff_t diff = ptr - buffer;
  if (diff > UINT8_MAX) {
    /* this should never happen in our code, but just to be safe */
    free(buffer);
    errno = EINVAL;
    return NULL;
  }
  ptr[-1] = diff;
  return ptr;
}

void aligned_free(void* ptr) {
  if (ptr) {
    uint8_t* u8ptr = ptr;
    free(u8ptr - u8ptr[-1]);
  }
}

int timingsafe_bcmp(const void* a, const void* b, size_t len) {
  const unsigned char* p1 = a;
  const unsigned char* p2 = b;

  unsigned int res = 0;
  for (; len; --len, ++p1, ++p2) {
    res |= *p1 ^ *p2;
  }
  return res;
}

/* `explicit_bzero` is provided as a `static inline` fallback via
 * `fallbacks.h` when the standalone bundle build is used. To avoid
 * duplicate definitions we no longer provide a separate definition here.
 */
