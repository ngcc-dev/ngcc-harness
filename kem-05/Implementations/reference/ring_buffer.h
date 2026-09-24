 /******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <stddef.h>
#include <stdlib.h>

#include "types.h"

ring_buffer_t rb_alloc(size_t size);
void rb_free(ring_buffer_t rb);
void rb_prepend(ring_buffer_t rb, index_t index, index_t position, int extra);
void rb_append(ring_buffer_t rb, index_t index, index_t position, int extra);
void rb_get_first(ring_buffer_t rb, index_t *index, index_t *position,
                  int *extra);
void rb_get(ring_buffer_t rb, size_t i, index_t *index, index_t *position,
            int *extra);
void rb_remove_first(ring_buffer_t rb);
void rb_remove(ring_buffer_t rb, size_t i);
void rb_put(ring_buffer_t rb, size_t i, index_t index, index_t position,
            int extra);
#endif
