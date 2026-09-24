#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "crypthash_adapter.h"
#include "test_util.h"

#define PDF_FILE_LEN    64
#define PDF_CMD_LEN     128

FILE *g_log_file = NULL;

void register_all_alg(void) {
    register_taichi_512_ref();
    register_taichi_768_ref();
    register_taichi_1024_ref();

    register_taichi_512_op_per();
    register_taichi_768_op_per();
    register_taichi_1024_op_per();

    register_taichi_512_op_res();
    register_taichi_768_op_res();
    register_taichi_1024_op_res();
}

void prepare_log_file(int argc, char *argv[]) {
    g_log_file = fopen(TEST_LOG_FILE, "w+");
    fprintf(g_log_file, "Executing test command: ");
    for(int i = 0; i < argc; i++) {
        fprintf(g_log_file, "%s ", argv[i]);
    }
    fprintf(g_log_file, "\n");
}

void convert_log_to_pdf() {
    char pdf_file_name[PDF_FILE_LEN];
    char *date = get_timestamp("%Y%m%d%H%M%S");
    char pdf_cmd_line[PDF_CMD_LEN];

    snprintf(pdf_file_name, PDF_FILE_LEN, "../logs/ngcc_bench_%s.pdf", date);
    snprintf(pdf_cmd_line, PDF_CMD_LEN, "enscript -p - %s | ps2pdf - %s", TEST_LOG_FILE, pdf_file_name);
    if(system(pdf_cmd_line)) {
        fprintf(stderr, "Failed to generate PDF log.\n");
    }
}

int main(int argc, char *argv[]) {
    cmd_opt cmd_opt = { 0 };

    cmd_opt.bench_opt.times = DEFAULT_PERF_TIME;
    parse_options(argc, argv, &cmd_opt);
    prepare_log_file(argc, argv);
    print_banner();
    register_all_alg();
    for(uint32_t alg_id = 0; alg_id < MAX_ALG; alg_id++) {
        if(cmd_opt.candidate_algs[alg_id]) {
            const ALGORITHM *test_alg = get_algorithm_by_id(alg_id);
            if(test_alg == NULL) {
                fprintf(stderr, "Unregistered algorithm id %u.\n", alg_id);
                continue;
            }

            fprintf(stderr, "Selected algorithm id %u: %s\n", alg_id, test_alg->alg_name);

            if(test_alg->type == ALG_HASH) {
                run_hash_bench(test_alg, &cmd_opt.bench_opt);
            }
        }
    }

    /* convert_log_to_pdf(); */
    return 0;
}
