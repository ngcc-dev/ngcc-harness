#define _POSIX_C_SOURCE 200809L

#include "test_support.h"

#include <stdio.h>

#define CHECK(cond, fmt, ...)                                                     \
    do {                                                                          \
        if (!(cond)) {                                                            \
            fprintf(stderr, "FAIL: " fmt "\n", ##__VA_ARGS__);                    \
            return 1;                                                             \
        }                                                                         \
    } while (0)

static int run_expect(char *const argv[],
                      const char *needle1,
                      const char *needle2,
                      const char *needle3) {
    test_command_result_t result;

    CHECK(test_run_command(argv, &result) == 0, "command execution failed");
    if (result.exit_code != 0 ||
        (needle1 != NULL && !test_output_contains(result.output, needle1)) ||
        (needle2 != NULL && !test_output_contains(result.output, needle2)) ||
        (needle3 != NULL && !test_output_contains(result.output, needle3))) {
        fprintf(stderr, "%s\n", result.output);
        test_free_command_result(&result);
        CHECK(0, "unexpected mock mlkem output");
    }
    test_free_command_result(&result);
    return 0;
}

int main(int argc, char **argv) {
    CHECK(argc == 3, "usage: test_mock_mlkem BENCH MOCK_LIB");

    {
        char *const cmd[] = {argv[1], "--lib", argv[2], "--test", "kem", "--mode", "correctness", NULL};
        CHECK(run_expect(cmd, "[correctness] BEGIN", "[correctness] END status=PASS", NULL) == 0, "aggregate correctness failed");
    }
    {
        char *const cmd[] = {
            argv[1], "--lib", argv[2], "--test", "kem", "--mode", "performance",
            "--iterations", "1000", "--cycles", "off", NULL
        };
        CHECK(run_expect(cmd, "[performance] BEGIN", "[performance] END status=PASS", NULL) == 0,
              "sub-op performance failed");
    }

    printf("mock mlkem regression passed\n");
    return 0;
}
