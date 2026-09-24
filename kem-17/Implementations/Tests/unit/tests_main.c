#include <munit.h>
#include <stdio.h>

#include "munit_utils.h"
#include "parameters.h"

extern MunitTest gf_tests[];
extern MunitTest rs_tests[];
extern MunitTest vector_tests[];
#ifdef HEP_QC_HAVE_NTL
extern MunitTest gf2x_ntl_tests[];
#endif

static MunitSuite nested_suites[] = {MUNIT_LEAF_ONCE("galois field", gf_tests),
                                     MUNIT_LEAF_ONCE("reed solomon", rs_tests), MUNIT_LEAF_ONCE("vector", vector_tests),
#ifdef HEP_QC_HAVE_NTL
                                     MUNIT_LEAF_ONCE("gf2x", gf2x_ntl_tests),
#endif
                                     MUNIT_SUITE_END};

static MunitSuite main_suite = MUNIT_TOP_SUITE("unit", nested_suites);

int main(int argc, char *const argv[]) {
    printf("----\n");
    printf("  %s\n", CRYPTO_ALGNAME);
    printf("  N: %d   \n", PARAM_N);
    printf("  Sec: %d bits\n", PARAM_SECURITY);
    printf("----\n\n");

    return munit_suite_main(&main_suite, NULL, argc, argv);
}
