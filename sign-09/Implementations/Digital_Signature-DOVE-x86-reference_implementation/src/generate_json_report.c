#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "report.h"

static void json_sig_perf(FILE *fp, const char *name, struct perf_profile *p, int last)
{
    fprintf(fp,
        "      \"%s\": {\n"
        "        \"avg_cycles\": %.2f,\n"
        "        \"min_cycles\": %llu,\n"
        "        \"max_cycles\": %llu,\n"
        "        \"throughput_ops_per_sec\": %.2f,\n"
        "        \"run_times\": %u\n"
        "      }%s\n",
        name, p->avg_cycles,
        (unsigned long long)p->min_cycles,
        (unsigned long long)p->max_cycles,
        p->throughput, p->run_time,
        last ? "" : ",");
}

void save_sig_full_report(const char *report_file_name, bench_report_t *report,
                          const char *timestamp)
{
    FILE *fp = fopen(report_file_name, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: cannot write JSON report %s\n", report_file_name);
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"algorithm\": {\n");
    fprintf(fp, "    \"category\": \"digital-signature\",\n");
    fprintf(fp, "    \"name\": \"%s\",\n", report->alg_name);
    fprintf(fp, "    \"author\": \"%s\",\n", report->author_name);
    fprintf(fp, "    \"security_level\": %d,\n", report->security_level);
    fprintf(fp, "    \"implementation_version\": \"%s\"\n",
#ifdef PKC_OPTIMIZED_IMPL
            "optimized"
#else
            "reference"
#endif
    );
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"run\": {\n");
    fprintf(fp, "    \"timestamp\": \"%s\",\n", timestamp ? timestamp : "");
    fprintf(fp, "    \"command\": \"ngcc_bench\",\n");
    fprintf(fp, "    \"iterations\": %u\n", report->iterations);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"functional\": {\n");
    fprintf(fp, "    \"correctness\": %s,\n", report->sig_func.correct ? "true" : "false");
    fprintf(fp, "    \"forged_signature_rejected\": %s\n", report->sig_func.sig_forgery ? "true" : "false");
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"sizes_bytes\": {\n");
    fprintf(fp, "    \"public_key\": %zu,\n", report->sig_para.pub_len);
    fprintf(fp, "    \"private_key\": %zu,\n", report->sig_para.pri_len);
    fprintf(fp, "    \"signature\": %zu\n", report->sig_para.sig_len);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"performance\": {\n");
    json_sig_perf(fp, "keygen", &report->sig_perf.keygen_perf, 0);
    json_sig_perf(fp, "sign", &report->sig_perf.sign_perf, 0);
    json_sig_perf(fp, "verify", &report->sig_perf.verify_perf, 1);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"memory_bytes\": {\n");
    fprintf(fp, "    \"static_text\": %zu,\n", report->mem_report.text_size);
    fprintf(fp, "    \"static_data\": %zu,\n", report->mem_report.data_size);
    fprintf(fp, "    \"static_bss\": %zu,\n", report->mem_report.bss_size);
    fprintf(fp, "    \"peak_resident\": %ld\n", report->mem_report.peak_rss);
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");
    fclose(fp);
}

void append_sig_csv_summary(const char *output_dir, bench_report_t *report, const char *timestamp)
{
    char csv_path[REPORT_PATH_LEN];
    int existed = 0;
    FILE *probe;
    FILE *fp;

    snprintf(csv_path, sizeof(csv_path), "%s/summary.csv", output_dir);
    probe = fopen(csv_path, "r");
    if (probe) {
        existed = 1;
        fclose(probe);
    }

    fp = fopen(csv_path, "a");
    if (!fp) {
        fprintf(stderr, "ERROR: cannot append CSV %s\n", csv_path);
        return;
    }
    if (!existed) {
        fprintf(fp,
            "timestamp,algorithm,security_level,iterations,"
            "correctness,forgery_rejected,"
            "pk_bytes,sk_bytes,sig_bytes,"
            "keygen_avg_cycles,keygen_ops,"
            "sign_avg_cycles,sign_ops,"
            "verify_avg_cycles,verify_ops,"
            "static_text,static_data,static_bss,peak_resident\n");
    }
    fprintf(fp,
        "%s,%s,%d,%u,%d,%d,%zu,%zu,%zu,"
        "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
        "%zu,%zu,%zu,%ld\n",
        timestamp, report->alg_name, report->security_level, report->iterations,
        report->sig_func.correct, report->sig_func.sig_forgery,
        report->sig_para.pub_len, report->sig_para.pri_len, report->sig_para.sig_len,
        report->sig_perf.keygen_perf.avg_cycles, report->sig_perf.keygen_perf.throughput,
        report->sig_perf.sign_perf.avg_cycles, report->sig_perf.sign_perf.throughput,
        report->sig_perf.verify_perf.avg_cycles, report->sig_perf.verify_perf.throughput,
        report->mem_report.text_size, report->mem_report.data_size,
        report->mem_report.bss_size, report->mem_report.peak_rss);
    fclose(fp);
}
