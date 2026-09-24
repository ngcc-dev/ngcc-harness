#include <munit.h>
#include <stdio.h>
#include "munit_utils.h"
#include "parameters.h"

extern MunitTest kem_tests[];
extern MunitTest pke_tests[];

static MunitSuite nested_suites[] = {MUNIT_LEAF_ONCE("kem", kem_tests), MUNIT_LEAF_ONCE("pke", pke_tests),
                                     MUNIT_SUITE_END};

static MunitSuite main_suite = MUNIT_TOP_SUITE("api", nested_suites);

int main(int argc, char *const argv[]) {
    printf("----\n");
    printf("  %s\n", CRYPTO_ALGNAME);
    printf("  N: %d   \n", PARAM_N);
    printf("  Sec: %d bits\n", PARAM_SECURITY);
    printf("----\n\n");

    return munit_suite_main(&main_suite, NULL, argc, argv);
}
