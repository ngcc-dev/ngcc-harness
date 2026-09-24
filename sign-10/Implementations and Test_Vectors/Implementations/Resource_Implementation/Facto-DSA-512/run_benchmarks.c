#include <stdio.h>
#include <stdlib.h>

static int run_command(const char *cmd, const char *what)
{
    int rc;

    rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "ERROR: %s failed with status %d\n", what, rc);
        return -1;
    }
    return 0;
}

int main(void)
{
    const char *prepare =
    "cwd=$(pwd -P); "
    "current=${cwd##*/}; "
    "parent=${cwd%/*}; "
    "parent_name=${parent##*/}; "
    "tag=$parent_name-$current; "
    "printf '%s/%s\\n' \"$parent_name\" \"$current\"; "
    "printf '%s\\n' \"$tag\" > .bench_tag_tmp";

    const char *cleanup =
        "rm -f .bench_tag_tmp";

    if (run_command(prepare, "prepare benchmark tag") != 0) {
        return EXIT_FAILURE;
    }

    if (run_command(cleanup, "cleanup") != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
