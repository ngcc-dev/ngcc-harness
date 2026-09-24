#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "registry.h"

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s -a <algorithm[,algorithm...]> [-t times] [-l bytes]\n", prog);
}

int main(int argc, char **argv)
{
    const char *alg_list = NULL;
    unsigned int times = 1000;
    size_t len_bytes = 128;
    int i;
    int status = 0;

    register_iphe_algorithms();
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            alg_list = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            times = (unsigned int)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            len_bytes = (size_t)strtoull(argv[++i], NULL, 10);
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (alg_list == NULL) {
        usage(argv[0]);
        return 1;
    }

    char *list = (char *)malloc(strlen(alg_list) + 1U);
    if (list == NULL)
        return 1;
    strcpy(list, alg_list);

    char *token = strtok(list, ",");
    while (token != NULL) {
        const algorithm_t *alg = get_algorithm_by_id(token);
        if (alg == NULL) {
            fprintf(stderr, "Unknown algorithm id: %s\n", token);
            status = 1;
        } else if (run_hash_bench(alg, times, len_bytes) != 0) {
            status = 1;
        }
        token = strtok(NULL, ",");
    }

    free(list);
    return status;
}
