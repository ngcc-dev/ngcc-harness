#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "report.h"

#define THROUGHPUT_LEN  32

static cJSON *create_memory_node(struct mem_profile *mem_report) {
    cJSON *mem = cJSON_CreateObject();

    cJSON *static_seg = cJSON_AddObjectToObject(mem, "Static segments");
    cJSON_AddNumberToObject(static_seg, "text_size", mem_report->text_size);
    cJSON_AddNumberToObject(static_seg, "data_size", mem_report->data_size);
    cJSON_AddNumberToObject(static_seg, "bss_size", mem_report->bss_size);

    cJSON *dynamic = cJSON_AddObjectToObject(mem, "Dynamic usage");
    cJSON_AddNumberToObject(dynamic, "heap_peak", mem_report->heap_peak);
    cJSON_AddNumberToObject(dynamic, "stack_peak", mem_report->stack_peak);

    cJSON_AddNumberToObject(mem, "Peak total bytes", mem_report->peak_total);
    return mem;
}

static cJSON *create_hash_perf_node(struct hash_perf_item *item) {
    char throughput_result[THROUGHPUT_LEN] = { 0 };
    cJSON *perf = cJSON_CreateObject();

    cJSON_AddStringToObject(perf, "level", item->level);
    cJSON_AddNumberToObject(perf, "input_size_bytes", item->input_size);

    cJSON *cycle = cJSON_AddObjectToObject(perf, "Cycle measurement");
    cJSON_AddNumberToObject(cycle, "avg_cycles", item->hash_perf.avg_cycles);
    cJSON_AddNumberToObject(cycle, "min_cycles", item->hash_perf.min_cycles);
    cJSON_AddNumberToObject(cycle, "max_cycles", item->hash_perf.max_cycles);

    snprintf(throughput_result, THROUGHPUT_LEN, "%.2f MB/s", item->hash_perf.throughput);

    cJSON *throughput = cJSON_AddObjectToObject(perf, "Throughput");
    cJSON_AddStringToObject(throughput, "throughput", throughput_result);
    cJSON_AddNumberToObject(throughput, "run_times", item->hash_perf.run_time);

    return perf;
}

static void write_json_to_file(cJSON *root, char *report_file_name) {
    char *out = cJSON_Print(root);
    FILE *fp = fopen(report_file_name, "w+");

    if(fp) {
        fputs(out, fp);
        fclose(fp);
    }

    free(out);
}

void save_hash_full_report(char *report_file_name, bench_report_t *report) {
    cJSON *root = cJSON_CreateObject();

    cJSON *algorithms = cJSON_AddObjectToObject(root, "Algorithms overview");
    cJSON_AddStringToObject(algorithms, "Algorithm_name", report->alg_name);
    cJSON_AddStringToObject(algorithms, "Developed by", report->author_name);

    struct hash_parameter hash_para = report->alg_parameter.hash_para;
    cJSON_AddNumberToObject(algorithms, "Block size", hash_para.block_len);
    cJSON_AddNumberToObject(algorithms, "Digest Length", hash_para.digest_len);

    cJSON *func_report = cJSON_AddObjectToObject(root, "Functional test report");
    struct hash_func_profile hash_func = report->func_report.hash_func;
    cJSON_AddBoolToObject(func_report, "long input", hash_func.long_input_correct);
    cJSON_AddBoolToObject(func_report, "short input", hash_func.short_input_correct);

    cJSON *perf = cJSON_AddArrayToObject(root, "Performance evaluation");
    for(size_t i = 0; i < HASH_PERF_LEVELS; i++) {
        cJSON_AddItemToArray(perf, create_hash_perf_node(&report->perf_report.hash_perf.items[i]));
    }

    cJSON_AddItemToObject(root, "Memory evaluation", create_memory_node(&report->mem_report));

    write_json_to_file(root, report_file_name);
    cJSON_Delete(root);
}