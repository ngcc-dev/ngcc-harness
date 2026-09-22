#ifndef YUANYANG_512_CODEC_H
#define YUANYANG_512_CODEC_H

#include <stdint.h>

#include "yuanyang_inner.h"

void yuanyang_encode_uniform(
	const uint16_t *tab, unsigned int n, unsigned int b, unsigned int k,
	uint8_t *output);

void yuanyang_decode_uniform(
	uint16_t *tab, unsigned int n, unsigned int b, unsigned int k,
	const uint8_t *input);

#endif
