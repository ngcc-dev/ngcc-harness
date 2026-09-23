#ifndef LOONG_XOF_READER_H
#define LOONG_XOF_READER_H

typedef struct {
	const unsigned char *data;
	unsigned long long len_bytes;
	unsigned long long cursor;
} loong_xof_reader;

void loong_xof_reader_init(loong_xof_reader *reader, const unsigned char *data,
                           unsigned long long len_bytes);
unsigned long long loong_xof_reader_remaining(const loong_xof_reader *reader);
int loong_xof_reader_read(unsigned char *out, unsigned long long out_len_bytes,
                          loong_xof_reader *reader);
int loong_xof_reader_read_byte(unsigned char *out, loong_xof_reader *reader);

#endif
