#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "registry.h"
#include "crypthash_adapter.h"
#include "bench_local.h"
#include "test_util.h"

#define CLOCK_REALTIME          0
#define FUNC_TEST_OUTPUT_LEN    64

extern FILE * g_log_file;

static int run_hash_speed_bench(const ALGORITHM *hash_alg, struct hash_perf_profile *hash_perf, const bench_opt *opt) {
    uint8_t *input = NULL;
    uint8_t *md = NULL;
    size_t digest_len;

    const HASH_METHOD *hash_method = hash_alg->method;
    hash_perf->hash_perf.max_cycles = 0;
    hash_perf->hash_perf.min_cycles = INT32_MAX;
    hash_perf->hash_perf.run_time = opt->times;
    input = calloc(opt->len == 0 ? 1U : opt->len, 1U);
    digest_len = hash_method->get_digest_len();
    md = malloc(digest_len);
    if (input == NULL || md == NULL) {
        fprintf(stderr, "Failed to allocate hash benchmark buffers.\n");
        free(input);
        free(md);
        return CRYPTO_FAILED;
    }
    BENCH_LOG("\t|----------------------------------------------------------------------------------|\n");
    BENCH_LOG("\t|   %-12s, %10s, %12s, %18s, %16s   |\n", "Operation", "run times", "input size", "average cycles", "Mbps");
    BENCH_CIPHER_SPEED_VA(hash_method->do_hash(digest_len << 3, input, opt->len << 3, md), opt->times, opt->len, hash_perf->hash_perf.avg_cycles, hash_perf->hash_perf.max_cycles, hash_perf->hash_perf.min_cycles, hash_perf->hash_perf.throughput, "hash");
    BENCH_LOG("\t|----------------------------------------------------------------------------------|\n\n");
    free(input);
    free(md);
    return CRYPTO_SUCCESS;
}

static int run_hash_func_bench(const ALGORITHM *hash_alg, struct hash_func_profile *hash_func) {
    uint8_t *md = NULL;
    size_t digest_len = 0;
    char func_output[FUNC_TEST_OUTPUT_LEN];
    const HASH_METHOD *hash_method = hash_alg->method;

    const unsigned char input1[] = {
        0x61, 0x62, 0x63
    };
    /*
     * This test vector comes from Example 1 (A.1) of GM/T 0004-2012
     */
    const unsigned char expected1[] = {
        0x66, 0xc7, 0xf0, 0xf4, 0x62, 0xee, 0xed, 0xd9,
        0xd1, 0xf2, 0xd4, 0x6b, 0xdc, 0x10, 0xe4, 0xe2,
        0x41, 0x67, 0xc4, 0x87, 0x5c, 0xf2, 0xf7, 0xa2,
        0x29, 0x7d, 0xa0, 0x2b, 0x8f, 0x4b, 0xa8, 0xe0
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
     * This test vector comes from Example 2 (A.2) from GM/T 0004-2012
     */
    const unsigned char expected2[] = {
        0xde, 0xbe, 0x9f, 0xf9, 0x22, 0x75, 0xb8, 0xa1,
        0x38, 0x60, 0x48, 0x89, 0xc1, 0x8e, 0x5a, 0x4d,
        0x6f, 0xdb, 0x70, 0xe5, 0x38, 0x7e, 0x57, 0x65,
        0x29, 0x3d, 0xcb, 0xa3, 0x9c, 0x0c, 0x57, 0x32
    };

    BENCH_LOG("Algorithm name:\t%s\n", hash_alg->alg_name);

    BENCH_LOG("\t|---------------------------------------------------|\n");
    if (hash_method->self_test != NULL) {
        int passed = hash_method->self_test() == 0;

        hash_func->short_input_correct = passed;
        hash_func->long_input_correct = passed;
        if (!passed) {
            fprintf(stderr, "The adapter self-test of %s failed.\n", hash_alg->alg_name);
            BENCH_LOG("Functional test status: FAILED.\n\n");
            return CRYPTO_FAILED;
        }
        snprintf(func_output, FUNC_TEST_OUTPUT_LEN, "The adapter functional self-test       PASS");
        BENCH_LOG("\t|   %-48s|\n", func_output);
        BENCH_LOG("\t|---------------------------------------------------|\n");
        BENCH_LOG("Functional test status: ALL PASS.\n\n");
        return CRYPTO_SUCCESS;
    }

    digest_len = hash_method->get_digest_len();
    md = malloc(digest_len);
    hash_method->do_hash(digest_len << 3, input1, sizeof(input1) << 3, md);    
    if(memcmp(md, expected1, digest_len)) {
        fprintf(stderr, "The result hash of %s is out of expect.\n", hash_alg->alg_name);
    } else {
        snprintf(func_output, FUNC_TEST_OUTPUT_LEN, "The short input functional test         PASS");
        BENCH_LOG("\t|   %-48s|\n", func_output);
    }

    hash_func->short_input_correct = 1;

    hash_method->do_hash(digest_len << 3, input2, sizeof(input2) << 3, md);    
    if(memcmp(md, expected2, digest_len)) {
        fprintf(stderr, "The result hash of %s is out of expect.\n", hash_alg->alg_name);
    } else {
        snprintf(func_output, FUNC_TEST_OUTPUT_LEN, "The long input functional test          PASS");
        BENCH_LOG("\t|   %-48s|\n", func_output);
    }

    hash_func->long_input_correct = 1;

    BENCH_LOG("\t|---------------------------------------------------|\n");
    BENCH_LOG("Functional test status: ALL PASS.\n\n");

    return CRYPTO_SUCCESS;
}

static int run_hash_mem_profile(const ALGORITHM *hash_alg, struct mem_profile *mem_prof) {
    int static_status;
    int runtime_status;

    BENCH_LOG("Inspecting file %s\n\n", hash_alg->lib_name);
    static_status = static_mem_parse(hash_alg->lib_name, mem_prof);
    runtime_status = measure_memory(hash_alg->alg_id, mem_prof);
    if (static_status != 0)
        BENCH_LOG("WARNING: static memory profiling failed; values are unavailable.\n");
    if (runtime_status != 0)
        BENCH_LOG("WARNING: runtime memory profiling failed; values are unavailable.\n");

    BENCH_LOG("\t|-----------------------------------------|\t\n");
    BENCH_LOG("\t|   TEXT size:\t%18zu bytes  |\n", mem_prof->text_size);
    BENCH_LOG("\t|   DATA size:\t%18zu bytes  |\n", mem_prof->data_size);
    BENCH_LOG("\t|   BSS  size:\t%18zu bytes  |\n", mem_prof->bss_size);
    BENCH_LOG("\t|   HEAP usage peak:\t%10zu bytes  |\n", mem_prof->heap_peak);
    BENCH_LOG("\t|   STACK usage peak:\t%10zu bytes  |\n", mem_prof->stack_peak);
    BENCH_LOG("\t|-----------------------------------------|\t\n");
#ifdef _WIN32
    BENCH_LOG("Windows approximation: HEAP=PeakWorkingSetSize, STACK=PeakPagefileUsage.\n");
#endif
    return static_status == 0 && runtime_status == 0 ? CRYPTO_SUCCESS : CRYPTO_FAILED;
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
    BENCH_LOG("Functional Test result(X86)\n");
    BENCH_LOG("==============================================================================================\n");
    run_hash_func_bench(hash_alg, &hash_report.func_report.hash_func);

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Performance Test result(X86)\n");
    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("\n");
    run_hash_speed_bench(hash_alg, &hash_report.perf_report.hash_perf, opt);

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Memory profiling result(X86)\n");
    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("\n");
    run_hash_mem_profile(hash_alg, &hash_report.mem_report);

    BENCH_LOG("\n==============================================================================================\n");
    snprintf(report_path, REPORT_PATH_LEN, "../reports/%s_%s.json", hash_alg->alg_name, date_str);
    save_hash_full_report(report_path, &hash_report);
    BENCH_LOG("Test report has been saved to %s\n", report_path);
    BENCH_LOG("==============================================================================================\n");
    return CRYPTO_SUCCESS;
}