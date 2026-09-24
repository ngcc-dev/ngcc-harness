#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auxfunc.h"
#include "vdoo_config.h"

void hash_msg(unsigned char *output, unsigned long long output_len,
              const unsigned char *input, unsigned long long input_len);


int byte_fdump(FILE * fp, const char * extra_msg , const unsigned char *v, unsigned n_byte);

unsigned byte_fget( FILE * fp, unsigned char *v , unsigned n_byte );

int byte_from_file( unsigned char *v , unsigned n_byte , const char * f_name );

int byte_from_binfile( unsigned char *v , unsigned n_byte , const char * f_name );

int byte_read_file( unsigned char ** msg , unsigned long long * len , const char * f_name );

#endif /* !UTILS_H */