#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "registry.h"
#include "test_util.h"

#define ALG_BUF_LENGTH  256

#ifndef ALG_ID
#define ALG_ID 0
#endif
#ifndef ALG_CLI_HYPHEN
#define ALG_CLI_HYPHEN "unknown-0"
#endif
#ifndef ALG_CLI_UNDER
#define ALG_CLI_UNDER "unknown_0"
#endif

typedef struct {
    const char *s;
    int32_t id;
} str_id_map;

static str_id_map g_str_id_map[] = {
    {ALG_CLI_HYPHEN, ALG_ID},
    {ALG_CLI_UNDER, ALG_ID},
};

static int algstr_to_id(char *alg, uint8_t *candidate_algs)
{
    char alg_buf[ALG_BUF_LENGTH] = {0};
    size_t i = 0;
    size_t support_alg_cnt = sizeof(g_str_id_map) / sizeof(g_str_id_map[0]);

    strncpy(alg_buf, alg, ALG_BUF_LENGTH - 1);
    char *token = strtok(alg_buf, ",");
    while (token != NULL) {
        for (i = 0; i < support_alg_cnt; i++) {
            if (strcmp(token, g_str_id_map[i].s) == 0) {
                candidate_algs[g_str_id_map[i].id] = 1;
                break;
            }
        }
        if (i == support_alg_cnt)
            return 1;
        token = strtok(NULL, ",");
    }
    return 0;
}

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a <algorithm>      Algorithm id (%s or %s)\n",
            ALG_CLI_HYPHEN, ALG_CLI_UNDER);
    fprintf(stderr, "  -t <times>          Performance iterations (default 1000, minimum 100)\n");
    fprintf(stderr, "  -l <lib_archive>    Static library for static memory measurement\n");
    fprintf(stderr, "  -o <output_dir>     Report output directory (default reports)\n");
    fprintf(stderr, "  -h                  Show this help message\n");
}

static void print_support_alg(void)
{
    fprintf(stderr, "Supported algorithms:\n");
    for (size_t i = 0; i < sizeof(g_str_id_map) / sizeof(g_str_id_map[0]); i++)
        fprintf(stderr, "\t%s\n", g_str_id_map[i].s);
}

void parse_options(int argc, char **argv, cmd_opt *opts)
{
    int c;

    opts->bench_opt.times = DEFAULT_PERF_TIME;
    opts->bench_opt.lib_path = NULL;
    opts->bench_opt.output_dir = DEFAULT_OUTPUT_DIR;

    while ((c = getopt(argc, argv, "a:t:l:o:h")) != -1) {
        switch (c) {
        case 'a':
            if (algstr_to_id(optarg, opts->candidate_algs)) {
                fprintf(stderr, "Unknown algorithm.\n");
                print_support_alg();
                exit(1);
            }
            break;
        case 't': {
            long v = strtol(optarg, NULL, 10);
            opts->bench_opt.times = (v < 100) ? 100 : (uint32_t)v;
            break;
        }
        case 'l':
            opts->bench_opt.lib_path = optarg;
            break;
        case 'o':
            opts->bench_opt.output_dir = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        default:
            print_usage(argv[0]);
            exit(1);
        }
    }

    if (!opts->candidate_algs[ALG_ID])
        opts->candidate_algs[ALG_ID] = 1;
}
