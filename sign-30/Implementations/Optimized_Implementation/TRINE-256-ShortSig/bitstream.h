#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint8_t *data;
  size_t buf_len;
  uint32_t byte_pos;
  uint32_t bit_pos;
} bitstream_t;

int bs_init(bitstream_t *bs, uint8_t *buf, size_t buf_len);

int bs_write(bitstream_t *bs, uint32_t data, uint32_t data_len);

uint32_t bs_read(bitstream_t *bs, uint32_t data_len);

int bs_finalize(bitstream_t *bs);

#endif

