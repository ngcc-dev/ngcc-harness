#ifndef PARSING_H
#define PARSING_H

#include "ffi_vec.h"

void bag_piglet_key_to_string(unsigned char *key, const ffi_vec *s,
                              const unsigned char *seed, int flag);
void bag_piglet_key_from_string(unsigned char *seed, ffi_vec *s,
                                const unsigned char *key, int flag);
void bag_piglet_cipher_to_string(unsigned char *ciphertext,
                                 const ffi_vec *u,
                                 const ffi_vec *v);
void bag_piglet_cipher_from_string(ffi_vec *u, ffi_vec *v,
                                   const unsigned char *ciphertext);
void unsigned_char_print(const unsigned char *value, int size);

#endif
