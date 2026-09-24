#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "registry.h"
#include "crypthash_adapter.h"
#include "bench_local.h"
#include "test_util.h"

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#define FUNC_TEST_OUTPUT_LEN    64

#define TAICHI_512_DIGEST_LEN   64
#define TAICHI_768_DIGEST_LEN   96
#define TAICHI_1024_DIGEST_LEN  128

extern FILE * g_log_file;

static const char *hash_perf_level_name[HASH_PERF_LEVELS] = {
    "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8"
};

static const size_t hash_perf_input_size[HASH_PERF_LEVELS] = {
    32, 128, 512, 1024, 4096, 8192, 16384, 65536
};

static int run_hash_speed_bench(const ALGORITHM *hash_alg, struct hash_perf_profile *hash_perf, const bench_opt *opt) {
    const HASH_METHOD *hash_method = hash_alg->method;
    size_t digest_len = hash_method->get_digest_len();

    BENCH_LOG("\t|--------------------------------------------------------------------------------|\n");
    BENCH_LOG("\t|   %-8s %12s %10s %18s %16s   |\n",
            "Level", "input size", "run times", "average cycles", "MB/s");

    for(size_t i = 0; i < HASH_PERF_LEVELS; i++) {
        uint8_t *input = NULL;
        uint8_t *md = NULL;
        size_t input_len = hash_perf_input_size[i];

        input = malloc(input_len);
        md = malloc(digest_len);

        if(input == NULL || md == NULL) {
            free(input);
            free(md);
            return CRYPTO_FAILED;
        }

        memset(input, 0, input_len);

        hash_perf->items[i].level = hash_perf_level_name[i];
        hash_perf->items[i].input_size = input_len;
        hash_perf->items[i].hash_perf.max_cycles = 0;
        hash_perf->items[i].hash_perf.min_cycles = UINT64_MAX;
        hash_perf->items[i].hash_perf.run_time = opt->times;

        BENCH_HASH_SPEED(
            hash_method->do_hash((int)(digest_len << 3), input, input_len << 3, md),
            opt->times,
            input_len,
            hash_perf->items[i].hash_perf.avg_cycles,
            hash_perf->items[i].hash_perf.max_cycles,
            hash_perf->items[i].hash_perf.min_cycles,
            hash_perf->items[i].hash_perf.throughput
        );

        BENCH_LOG("\t|   %-8s %12zu %10u %18.2f %16.2f   |\n",
                hash_perf_level_name[i],
                input_len,
                opt->times,
                hash_perf->items[i].hash_perf.avg_cycles,
                hash_perf->items[i].hash_perf.throughput);

        free(input);
        free(md);
    }

    BENCH_LOG("\t|--------------------------------------------------------------------------------|\n\n");
    return CRYPTO_SUCCESS;
}

static int run_hash_func_bench(const ALGORITHM *hash_alg, struct hash_func_profile *hash_func) {
    uint8_t *md = NULL;
    size_t digest_len = 0;
    char func_output[FUNC_TEST_OUTPUT_LEN];
    const HASH_METHOD *hash_method = hash_alg->method;
    const unsigned char *expected1 = NULL;
    const unsigned char *expected2 = NULL;
    int ret = CRYPTO_SUCCESS;

    const unsigned char input1[] = {
        0x61, 0x62, 0x63
    };

    const unsigned char input2[] = {
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64
    };

    /*
     * TODO: Replace all-zero placeholders with official TaiChi test vectors.
     * input1 = "abc"
     * input2 = "abcd" repeated 16 times
     */
    const unsigned char taichi_512_expected1[TAICHI_512_DIGEST_LEN] = {
        0x83,0xbd,0x7a,0x10,0x6e,0x53,0x83,0xd4,
        0x90,0x0d,0xae,0x48,0x68,0xea,0x57,0x31,
        0xbb,0x2d,0x6c,0xae,0x0f,0xdc,0x0d,0xf2,
        0x5e,0x12,0xe8,0x4a,0xa0,0x65,0x25,0x65,
        0xc1,0x93,0x87,0x2c,0x44,0x84,0x5a,0x7a,
        0x92,0xb1,0x56,0x17,0x7b,0x5b,0x50,0x0c,
        0x61,0x6a,0x43,0xec,0x9a,0xcd,0x25,0xc4,
        0x3f,0x97,0xe7,0x57,0xee,0x03,0xe0,0x10
    };

    const unsigned char taichi_512_expected2[TAICHI_512_DIGEST_LEN] = {
        0x2e,0xed,0x9d,0xc6,0x87,0xbc,0x31,0xe4,
        0x30,0x3a,0x80,0x41,0x82,0xa1,0x7e,0x64,
        0xaf,0x59,0x13,0x98,0x13,0xc9,0x48,0xd7,
        0x8a,0x1c,0x3a,0x8a,0xae,0x83,0xb5,0x26,
        0xd1,0x7c,0x68,0x38,0x6e,0x8f,0x75,0x87,
        0x4f,0x35,0xe5,0x4c,0xb8,0xd0,0x76,0xeb,
        0x52,0x69,0xdf,0xce,0x92,0x38,0x39,0xc3,
        0xff,0x66,0x87,0x46,0x2f,0x9e,0x22,0xcd
    };

    const unsigned char taichi_768_expected1[TAICHI_768_DIGEST_LEN] = {
        0x91,0x97,0x5f,0x7f,0xf6,0x11,0x35,0x25,
        0xcc,0x95,0xd2,0xc2,0x57,0x3c,0xef,0x16,
        0x0a,0x41,0x18,0xfb,0xe0,0x32,0x67,0x6e,
        0x45,0xb2,0x8a,0x6b,0xbb,0xab,0x40,0x8b,
        0x6a,0x64,0xb4,0x2b,0xd5,0xa5,0x49,0x8b,
        0xdc,0xe2,0x9c,0xd5,0xc2,0xca,0x91,0xe1,
        0x3f,0x40,0x4c,0x20,0x2a,0x75,0x37,0x1e,
        0x38,0xea,0xe7,0x29,0xa9,0x95,0xcd,0x51,
        0x28,0x31,0x97,0x95,0xbb,0x81,0x7f,0x18,
        0x2f,0xda,0x16,0xcc,0xf5,0xdb,0x25,0x01,
        0x81,0x84,0x33,0x2a,0x76,0x09,0x08,0x98,
        0x07,0x2e,0xa5,0xd7,0x8f,0x06,0xc9,0x40
    };

    const unsigned char taichi_768_expected2[TAICHI_768_DIGEST_LEN] = {
        0xa8,0xcf,0x1a,0xe2,0xf2,0x80,0xa2,0x4f,
        0x52,0xba,0x91,0xdb,0xe0,0xed,0x31,0x52,
        0xd4,0x79,0xa9,0xbe,0xc8,0xc2,0xdd,0x8d,
        0xf7,0x5c,0xf8,0xa4,0xe4,0x1c,0xe3,0x60,
        0x7b,0x4a,0xeb,0x48,0x90,0x0c,0xfa,0x07,
        0xb6,0x59,0xbf,0x12,0x8a,0x25,0xff,0xc7,
        0x2e,0x02,0x7b,0xff,0xd3,0xaf,0xa0,0xba,
        0x4b,0xc8,0xc2,0x00,0xc8,0x5f,0x37,0x42,
        0x87,0xef,0x8d,0xbe,0xd8,0xe0,0xc5,0xdf,
        0x43,0x2b,0x2a,0x36,0x36,0x81,0xde,0xbd,
        0x0a,0x1f,0xec,0x56,0x4b,0x90,0x8b,0x86,
        0x39,0xb2,0x45,0x98,0x06,0x8a,0x6d,0x46
    };

    const unsigned char taichi_1024_expected1[TAICHI_1024_DIGEST_LEN] = {
        0x39,0x1a,0x11,0x1b,0xe9,0x21,0x3d,0xed,
        0x59,0xd0,0xc5,0x7f,0x67,0x55,0x70,0x85,
        0xd6,0x92,0x87,0xcb,0x03,0x77,0xe5,0x9e,
        0x16,0xd3,0x75,0x5c,0x13,0xf5,0x68,0x8f,
        0xc2,0x63,0x7d,0xe5,0x95,0x49,0xdd,0x85,
        0x0f,0xc4,0x0f,0xd4,0xec,0xa6,0x1a,0x40,
        0x14,0xd8,0x0e,0x26,0x35,0xff,0x4d,0xaa,
        0x01,0x09,0x31,0x66,0xcf,0x19,0x6c,0xaa,
        0xcf,0x76,0x26,0x5a,0x67,0xdf,0xc3,0xf2,
        0xb6,0xc2,0x1e,0xc2,0x1a,0x2e,0x4e,0x6c,
        0x52,0xa8,0xb9,0x58,0xba,0xba,0x1c,0xa9,
        0x48,0x55,0x98,0xc3,0xd1,0x99,0xb9,0x45,
        0x5e,0x91,0x40,0x3e,0xb7,0x1a,0x0e,0x70,
        0xb7,0xe8,0x6e,0x6a,0x5f,0x9a,0xdc,0x87,
        0xc8,0x02,0xbc,0x6e,0xe0,0x30,0xdb,0x54,
        0xee,0x1e,0xcc,0xab,0xf5,0x31,0x0f,0x9b
    };

    const unsigned char taichi_1024_expected2[TAICHI_1024_DIGEST_LEN] = {
        0x0f,0xc3,0x4b,0x69,0xb4,0x0d,0xc7,0xc9,
        0x03,0xde,0x58,0xd7,0xd4,0xa4,0x19,0xc0,
        0x21,0xd7,0x03,0x5d,0x2f,0xef,0xa9,0x2b,
        0xac,0x35,0xb3,0x27,0x89,0xc4,0x87,0x01,
        0x8b,0x0a,0x07,0xc4,0x0c,0x57,0x1a,0x2d,
        0xd7,0xc7,0x2b,0x0f,0xc8,0xa0,0x2c,0x82,
        0x93,0x25,0xe1,0x17,0x90,0x72,0xed,0x77,
        0x54,0xd1,0xe8,0x99,0x06,0x33,0x60,0xc2,
        0xde,0x11,0x01,0x11,0x00,0x0d,0x16,0x40,
        0xe4,0x57,0xd1,0x10,0x83,0xfe,0x7f,0x86,
        0x2f,0x5f,0xbf,0xaa,0x73,0xb3,0xb1,0x73,
        0xc0,0x09,0x20,0xcd,0x35,0x48,0xb8,0x83,
        0xb7,0x2f,0x21,0x95,0xce,0x40,0x7a,0xd7,
        0x50,0x1a,0x04,0x9c,0x17,0xa3,0xc3,0x05,
        0x6e,0x3e,0xfe,0x07,0x3a,0x63,0xe0,0xfe,
        0x64,0xd9,0xd5,0x39,0x1e,0x60,0xc3,0x64
    };

    switch(hash_alg->alg_id) {
    case TAICHI_512_REF:
    case TAICHI_512_OP_PER:
    case TAICHI_512_OP_RES:
        expected1 = taichi_512_expected1;
        expected2 = taichi_512_expected2;
        break;
    case TAICHI_768_REF:
    case TAICHI_768_OP_PER:
    case TAICHI_768_OP_RES:
        expected1 = taichi_768_expected1;
        expected2 = taichi_768_expected2;
        break;
    case TAICHI_1024_REF:
    case TAICHI_1024_OP_PER:
    case TAICHI_1024_OP_RES:
        expected1 = taichi_1024_expected1;
        expected2 = taichi_1024_expected2;
        break;
    default:
        fprintf(stderr, "No test vector for %s.\n", hash_alg->alg_name);
        return CRYPTO_FAILED;
    }

    BENCH_LOG("Algorithm name:\t%s\n", hash_alg->alg_name);

    BENCH_LOG("\t|---------------------------------------------------|\n");
    digest_len = hash_method->get_digest_len();
    md = malloc(digest_len);
    if(md == NULL) {
        return CRYPTO_FAILED;
    }

    if(hash_method->do_hash((int)(digest_len << 3), input1, sizeof(input1) << 3, md)) {
        fprintf(stderr, "The short input hash of %s failed.\n", hash_alg->alg_name);
        hash_func->short_input_correct = 0;
        ret = CRYPTO_FAILED;
    } 
    else if(memcmp(md, expected1, digest_len)) {
        fprintf(stderr, "The result hash of %s is out of expect.\n", hash_alg->alg_name);
        hash_func->short_input_correct = 0;
        ret = CRYPTO_FAILED;
    } 
    else {
        snprintf(func_output, FUNC_TEST_OUTPUT_LEN, "The short input functional test         PASS");
        BENCH_LOG("\t|   %-48s|\n", func_output);
        hash_func->short_input_correct = 1;
    }

    if(hash_method->do_hash((int)(digest_len << 3), input2, sizeof(input2) << 3, md)) {
        fprintf(stderr, "The long input hash of %s failed.\n", hash_alg->alg_name);
        hash_func->long_input_correct = 0;
        ret = CRYPTO_FAILED;
    } 
    else if(memcmp(md, expected2, digest_len)) {
        fprintf(stderr, "The result hash of %s is out of expect.\n", hash_alg->alg_name);
        hash_func->long_input_correct = 0;
        ret = CRYPTO_FAILED;
    } 
    else {
        snprintf(func_output, FUNC_TEST_OUTPUT_LEN, "The long input functional test          PASS");
        BENCH_LOG("\t|   %-48s|\n", func_output);
        hash_func->long_input_correct = 1;
    }

    BENCH_LOG("\t|---------------------------------------------------|\n");
    if(ret == CRYPTO_SUCCESS) {
        BENCH_LOG("Functional test status: ALL PASS.\n\n");
    } else {
        BENCH_LOG("Functional test status: FAILED.\n\n");
    }

    free(md);
    return ret;
}

static int run_hash_mem_profile(const ALGORITHM *hash_alg, struct mem_profile *mem_prof) {
    static_mem_parse(hash_alg->lib_name, mem_prof);
    measure_memory(hash_alg->lib_name, mem_prof);

    mem_prof->static_total = mem_prof->text_size + mem_prof->data_size + mem_prof->bss_size;

    mem_prof->peak_total = mem_prof->text_size + mem_prof->data_size + mem_prof->bss_size +
                           mem_prof->heap_peak + mem_prof->stack_peak;

    BENCH_LOG("\t|---------------------------------------------------------------|\n");
    BENCH_LOG("\t|   %-18s %18s %8s   |\n", "Item", "Value", "Unit");
    BENCH_LOG("\t|---------------------------------------------------------------|\n");
    BENCH_LOG("\t|   %-18s %18zu %8s   |\n", "TEXT size", mem_prof->text_size, "bytes");
    BENCH_LOG("\t|   %-18s %18zu %8s   |\n", "DATA size", mem_prof->data_size, "bytes");
    BENCH_LOG("\t|   %-18s %18zu %8s   |\n", "BSS size", mem_prof->bss_size, "bytes");
    BENCH_LOG("\t|   %-18s %18zu %8s   |\n", "STATIC total", mem_prof->static_total, "bytes");
    BENCH_LOG("\t|   %-18s %18zu %8s   |\n", "HEAP peak", mem_prof->heap_peak, "bytes");
    BENCH_LOG("\t|   %-18s %18zu %8s   |\n", "STACK peak", mem_prof->stack_peak, "bytes");
    BENCH_LOG("\t|   %-18s %18zu %8s   |\n", "TOTAL peak", mem_prof->peak_total, "bytes");
    BENCH_LOG("\t|---------------------------------------------------------------|\n");
    return CRYPTO_SUCCESS;
}

int run_hash_bench(const ALGORITHM *hash_alg, const bench_opt *opt) {
    bench_report_t hash_report = { 0 };
    char *date_str = get_timestamp("%Y%m%d_%H%M%S");
    char report_path[REPORT_PATH_LEN] = { 0 };

    BENCH_LOG("\n==============================================================================================\n");
    BENCH_LOG("Running bench for %s (by %s)...\n\n", hash_alg->alg_name, hash_alg->author_name);

    hash_report.alg_name = hash_alg->alg_name;
    hash_report.author_name = hash_alg->author_name;

    const HASH_METHOD *hash_method = hash_alg->method;
    hash_report.alg_parameter.hash_para.block_len = hash_method->get_block_len();
    hash_report.alg_parameter.hash_para.digest_len = hash_method->get_digest_len();

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Functional Test result\n");
    BENCH_LOG("==============================================================================================\n");
    run_hash_func_bench(hash_alg, &hash_report.func_report.hash_func);

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Performance Test result\n");
    BENCH_LOG("==============================================================================================\n");
    run_hash_speed_bench(hash_alg, &hash_report.perf_report.hash_perf, opt);

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Memory profiling result\n");
    BENCH_LOG("==============================================================================================\n");
    run_hash_mem_profile(hash_alg, &hash_report.mem_report);

    BENCH_LOG("\n==============================================================================================\n");
    snprintf(report_path, REPORT_PATH_LEN, "../reports/%s_%s.json", hash_alg->alg_name, date_str);
    save_hash_full_report(report_path, &hash_report);
    BENCH_LOG("Test report has been saved to %s\n", report_path);
    BENCH_LOG("==============================================================================================\n");

    return CRYPTO_SUCCESS;
}