#include "loong_xof_reader.h"

#include "loong_status.h"

#include <string.h>

void loong_xof_reader_init(loong_xof_reader *reader, const unsigned char *data,
                           unsigned long long len_bytes)
{
	if (reader == 0) {
		return;
	}
	reader->data = data;
	reader->len_bytes = len_bytes;
	reader->cursor = 0;
}

unsigned long long loong_xof_reader_remaining(const loong_xof_reader *reader)
{
	if (reader == 0 || reader->cursor > reader->len_bytes) {
		return 0;
	}
	return reader->len_bytes - reader->cursor;
}

int loong_xof_reader_read(unsigned char *out, unsigned long long out_len_bytes,
                          loong_xof_reader *reader)
{
	if (reader == 0 || (out_len_bytes != 0 && out == 0)) {
		return LOONG_ERR_NULL;
	}
	if (out_len_bytes > loong_xof_reader_remaining(reader)) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (out_len_bytes != 0) {
		if (reader->data == 0) {
			return LOONG_ERR_NULL;
		}
		memcpy(out, reader->data + reader->cursor, (size_t)out_len_bytes);
	}
	reader->cursor += out_len_bytes;
	return LOONG_SUCCESS;
}

int loong_xof_reader_read_byte(unsigned char *out, loong_xof_reader *reader)
{
	return loong_xof_reader_read(out, 1, reader);
}
