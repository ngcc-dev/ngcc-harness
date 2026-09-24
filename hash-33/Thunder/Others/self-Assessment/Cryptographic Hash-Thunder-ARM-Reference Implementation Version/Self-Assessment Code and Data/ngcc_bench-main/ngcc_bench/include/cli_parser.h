#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "cli_types.h"

void print_version(void);
void print_usage(const char *prog);
void init_default_options(cli_options_t *opts);

int parse_unsigned_ll(const char *s, unsigned long long *out);
int parse_int_value(const char *s, int *out);
int parse_double_value(const char *s, double *out);
int parse_test_mask(const char *s, unsigned int *out_mask);
int parse_mode_mask(const char *s, unsigned int *out_mask);

int parse_cli_options(int argc, char **argv, cli_options_t *opts);
int validate_options(const cli_options_t *opts);

#endif
