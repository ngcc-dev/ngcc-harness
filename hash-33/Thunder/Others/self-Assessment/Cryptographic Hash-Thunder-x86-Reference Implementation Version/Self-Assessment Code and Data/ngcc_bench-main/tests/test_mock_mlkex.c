#define _POSIX_C_SOURCE 200809L

#include "test_support.h"

#include <limits.h>
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
        CHECK(0, "unexpected mock mlkex output");
    }
    test_free_command_result(&result);
    return 0;
}

int main(int argc, char **argv) {
    char tmp_dir[] = "/tmp/ngcc_mock_mlkex.XXXXXX";
    char kat_dir[PATH_MAX];
    char kat_path[PATH_MAX];
    /*
     * 3-pass mock KEX test vector:
     *   kex_init_a  → pka=01, ska=02, sta=03
     *   kex_init_b  → pkb=01, skb=02, stb=03
     *   pass1_msg_a → sta=03(?), m1=11
     *   pass2_msg_b → stb=03(?), m2=22
     *   pass3_msg_a → sta=03(?), m3=33
     *   derive_ss_a(ska=02, pkb=01, mb=22, sta=03) → ssa=09
     *   derive_ss_b(skb=02, pka=01, ma=33, stb=03) → ssb=09
     */
    const char *kat_content =
        "Count = 0\n"
        "PKa = 01\n"
        "SKa = 02\n"
        "Init_Sta = 03\n"
        "PKb = 01\n"
        "SKb = 02\n"
        "Init_Stb = 03\n"
        "Pass1_Sta = 03\n"
        "M1 = 11\n"
        "Pass2_Stb = 03\n"
        "M2 = 22\n"
        "Pass3_Sta = 03\n"
        "M3 = 33\n"
        "SS = 09\n";

    CHECK(argc == 3, "usage: test_mock_mlkex BENCH MOCK_LIB");
    CHECK(test_make_temp_dir(tmp_dir) == 0, "failed to create temp dir");
    CHECK(snprintf(kat_dir, sizeof(kat_dir), "%s/kex_kat", tmp_dir) < (int) sizeof(kat_dir),
          "kat dir path too long");
    CHECK(test_mkdir_p(kat_dir) == 0, "failed to create kat dir");
    CHECK(snprintf(kat_path, sizeof(kat_path), "%s/KAT_KEX_mock.txt", kat_dir) < (int) sizeof(kat_path),
          "kat path too long");
    CHECK(test_write_file(kat_path, kat_content) == 0, "failed to write kat file");

    {
        char *const cmd[] = {argv[1], "--lib", argv[2], "--test", "kex", "--mode", "correctness", NULL};
        CHECK(run_expect(cmd, "[correctness] BEGIN", "[correctness] END status=PASS", NULL) == 0, "correctness failed");
    }
    {
        char *const cmd[] = {
            argv[1], "--lib", argv[2], "--test", "kex", "--mode", "performance",
            "--iterations", "1000", "--cycles", "off", NULL
        };
        CHECK(run_expect(cmd, "[performance] BEGIN", "[performance] END status=PASS", NULL) == 0,
              "performance failed");
    }
    {
        char *const cmd[] = {argv[1], "--lib", argv[2], "--test", "kex", "--mode", "correctness", "--kat", kat_dir, NULL};
        CHECK(run_expect(cmd, "[correctness] BEGIN", "[correctness] END status=PASS", NULL) == 0,
              "kat correctness failed");
    }

    CHECK(test_remove_tree(tmp_dir) == 0, "failed to remove temp dir");
    printf("mock mlkex regression passed\n");
    return 0;
}
