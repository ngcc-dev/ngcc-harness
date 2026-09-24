#ifndef SQISIGN_XOF_H
#define SQISIGN_XOF_H

#include <stddef.h>

int sqisign_xof(unsigned char *out,
                size_t out_len_bytes,
                const unsigned char *in,
                size_t in_len_bytes);

#endif
