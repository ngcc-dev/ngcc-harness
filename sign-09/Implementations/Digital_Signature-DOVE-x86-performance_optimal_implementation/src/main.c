#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "sig_adapter.h"
#include "test_util.h"

#define PDF_FILE_LEN    64
#define PDF_CMD_LEN     256

FILE *g_log_file = NULL;

static void ensure_output_dirs(const char *output_dir)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\" logs", output_dir);
    int mk_rc = system(cmd);
    (void)mk_rc;
    if (mk_rc != 0)
        fprintf(stderr, "WARN: could not create output directories.\n");
}

void register_all_alg(void)
{
    register_sig_algorithm();
}

void prepare_log_file(int argc, char *argv[])
{
    ensure_output_dirs("reports");
    g_log_file = fopen(TEST_LOG_FILE, "a");
    if (!g_log_file)
        return;
    fprintf(g_log_file, "Executing test command: ");
    for (int i = 0; i < argc; i++)
        fprintf(g_log_file, "%s ", argv[i]);
    fprintf(g_log_file, "\n");
}

void convert_log_to_pdf(void)
{
    if (system("command -v enscript >/dev/null 2>&1") != 0)
        return;

    char pdf_file_name[PDF_FILE_LEN];
    char *date = get_timestamp("%Y%m%d%H%M%S");
    char pdf_cmd_line[PDF_CMD_LEN];

    snprintf(pdf_file_name, PDF_FILE_LEN, "logs/ngcc_bench_%s.pdf", date);
    snprintf(pdf_cmd_line, PDF_CMD_LEN,
             "enscript -p - %s 2>/dev/null | ps2pdf - %s 2>/dev/null",
             TEST_LOG_FILE, pdf_file_name);
    int pdf_rc = system(pdf_cmd_line);
    (void)pdf_rc;
}

int main(int argc, char *argv[])
{
    cmd_opt cmd_opt = {0};

    parse_options(argc, argv, &cmd_opt);
    prepare_log_file(argc, argv);
    print_banner();
    register_all_alg();

    for (uint32_t alg_id = 0; alg_id < MAX_ALG; alg_id++) {
        if (!cmd_opt.candidate_algs[alg_id])
            continue;
        const ALGORITHM *test_alg = get_algorithm_by_id(alg_id);
        if (test_alg == NULL) {
            fprintf(stderr, "Unregistered algorithm id %u.\n", alg_id);
            continue;
        }
        if (test_alg->type == ALG_SIG)
            run_sig_bench(test_alg, &cmd_opt.bench_opt);
    }

    convert_log_to_pdf();
    if (g_log_file)
        fclose(g_log_file);
    return 0;
}
