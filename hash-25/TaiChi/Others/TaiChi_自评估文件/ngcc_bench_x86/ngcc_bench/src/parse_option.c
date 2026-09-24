#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "registry.h"
#include "test_util.h"

#define ALG_BUF_LENGTH  256

typedef struct {
    const char *s;
    int32_t id;
} str_id_map;

static str_id_map g_str_id_map[] = {
    {"taichi-512-ref", TAICHI_512_REF},
    {"taichi-768-ref", TAICHI_768_REF},
    {"taichi-1024-ref", TAICHI_1024_REF},

    {"taichi-512-op-per", TAICHI_512_OP_PER},
    {"taichi-768-op-per", TAICHI_768_OP_PER},
    {"taichi-1024-op-per", TAICHI_1024_OP_PER},

    {"taichi-512-op-res", TAICHI_512_OP_RES},
    {"taichi-768-op-res", TAICHI_768_OP_RES},
    {"taichi-1024-op-res", TAICHI_1024_OP_RES},
};

static int algstr_to_id(char *alg, uint8_t *candidate_algs) {
    char alg_buf[ALG_BUF_LENGTH] = { 0 };
    size_t i = 0;
    size_t support_alg_cnt = sizeof(g_str_id_map) / sizeof(g_str_id_map[0]);

    strncpy(alg_buf, alg, ALG_BUF_LENGTH - 1);

    char *token = strtok(alg_buf, ",");
    while(token != NULL) {
        for(i = 0; i < support_alg_cnt; i++) {
            if(strcmp(token, g_str_id_map[i].s) == 0) {
                candidate_algs[g_str_id_map[i].id] = 1;
                break;
            }
        }

        if(i == support_alg_cnt)
            return 1;

        token = strtok(NULL, ",");
    }

    return 0;
}

static void print_usage(void) {
    fprintf(stderr, "Usage: ngcc_bench [options]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a <algorithm>      Specify algorithm to benchmark. Separated by ','\n");
    fprintf(stderr, "  -t <times>          Number of times to run. Must be >= 100\n");
    fprintf(stderr, "  -h                  Show this help message\n");
    fprintf(stderr, "Hash performance test always uses S1-S8 input sizes.\n");
}

static void print_support_alg(void) {
    fprintf(stderr, "Supported algorithms list.\n");
    for(size_t i = 0; i < sizeof(g_str_id_map) / sizeof(g_str_id_map[0]); i++) {
        fprintf(stderr, "\t%s\n", g_str_id_map[i].s);
    }
}

void parse_options(int argc, char **argv, cmd_opt *opts) {
    int c;

    if(argc < 2) {
        print_usage();
        exit(1);
    }

    while((c = getopt(argc, argv, "a:t:h")) != -1) {
        switch(c) {
        case 'a':
            if(algstr_to_id(optarg, opts->candidate_algs)) {
                fprintf(stderr, "Unknown algorithm.\n");
                print_support_alg();
                exit(1);
            }
            break;

        case 't':
            opts->bench_opt.times = (uint32_t)atoi(optarg);
            if(opts->bench_opt.times < MIN_PERF_TIME) {
                fprintf(stderr, "Performance test times must be >= %d.\n", MIN_PERF_TIME);
                exit(1);
            }
            break;

        case 'h':
            print_usage();
            exit(0);

        default:
            print_usage();
            exit(1);
        }
    }
}