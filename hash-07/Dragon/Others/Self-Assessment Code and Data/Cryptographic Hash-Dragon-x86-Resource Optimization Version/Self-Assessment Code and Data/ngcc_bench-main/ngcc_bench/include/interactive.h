#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include <stddef.h>

#include "cli_types.h"

int run_interactive_setup(cli_options_t *opts,
                          char *lib_buf,
                          size_t lib_buf_len,
                          char *kat_buf,
                          size_t kat_buf_len,
                          char *json_buf,
                          size_t json_buf_len);

#endif
