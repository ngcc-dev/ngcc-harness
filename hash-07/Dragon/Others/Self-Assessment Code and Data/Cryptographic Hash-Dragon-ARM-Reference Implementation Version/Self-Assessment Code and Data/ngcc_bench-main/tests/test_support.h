#ifndef NGCC_TEST_SUPPORT_H
#define NGCC_TEST_SUPPORT_H

#include <stddef.h>

typedef struct {
    char *output;
    size_t output_len;
    int exit_code;
} test_command_result_t;

int test_run_command(char *const argv[], test_command_result_t *out_result);
void test_free_command_result(test_command_result_t *result);

int test_read_file(const char *path, char **out_data);
int test_write_file(const char *path, const char *content);
int test_make_temp_dir(char *tmpl);
int test_remove_tree(const char *path);
int test_mkdir_p(const char *path);

int test_output_contains(const char *haystack, const char *needle);
int test_file_contains(const char *path, const char *needle);

#endif
