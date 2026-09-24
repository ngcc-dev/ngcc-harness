#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "json_report.h"
#include "ngcc_log.h"

#define L(lang, zh, en) ((lang) == LANG_EN ? (en) : (zh))

#ifndef NGCC_VERSION
#define NGCC_VERSION "unknown"
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

/* ── JSON writer helpers ──────────────────────────────────────── */

typedef struct {
    FILE *fp;
    int indent;
    int needs_comma;
} json_writer_t;

static void jw_init(json_writer_t *w, FILE *fp) {
    w->fp = fp;
    w->indent = 0;
    w->needs_comma = 0;
}

static void jw_indent(json_writer_t *w) {
    int i;
    for (i = 0; i < w->indent; ++i) {
        fputs("  ", w->fp);
    }
}

static void jw_comma(json_writer_t *w) {
    if (w->needs_comma) {
        fputs(",\n", w->fp);
    }
    w->needs_comma = 1;
}

static void jw_write_escaped(FILE *fp, const char *s) {
    const unsigned char *p = (const unsigned char *) s;
    fputc('"', fp);
    if (p != NULL) {
        while (*p != '\0') {
            switch (*p) {
                case '\\': fputs("\\\\", fp); break;
                case '"':  fputs("\\\"", fp); break;
                case '\n': fputs("\\n", fp);  break;
                case '\r': fputs("\\r", fp);  break;
                case '\t': fputs("\\t", fp);  break;
                default:
                    if (*p < 0x20U) {
                        fprintf(fp, "\\u%04x", (unsigned int) *p);
                    } else {
                        fputc((int) *p, fp);
                    }
                    break;
            }
            ++p;
        }
    }
    fputc('"', fp);
}

static void jw_begin_object(json_writer_t *w, const char *key) {
    jw_comma(w);
    jw_indent(w);
    if (key != NULL) {
        jw_write_escaped(w->fp, key);
        fputs(": {\n", w->fp);
    } else {
        fputs("{\n", w->fp);
    }
    w->indent++;
    w->needs_comma = 0;
}

static void jw_end_object(json_writer_t *w) {
    w->indent--;
    fputc('\n', w->fp);
    jw_indent(w);
    fputc('}', w->fp);
    w->needs_comma = 1;
}

static void jw_key_str(json_writer_t *w, const char *key, const char *val) {
    jw_comma(w);
    jw_indent(w);
    jw_write_escaped(w->fp, key);
    fputs(": ", w->fp);
    jw_write_escaped(w->fp, val);
}

static void jw_key_null(json_writer_t *w, const char *key);

static void jw_key_optional_str(json_writer_t *w, const char *key, const char *val) {
    if (val != NULL) {
        jw_key_str(w, key, val);
    } else {
        jw_key_null(w, key);
    }
}

static void jw_key_null(json_writer_t *w, const char *key) {
    jw_comma(w);
    jw_indent(w);
    jw_write_escaped(w->fp, key);
    fputs(": null", w->fp);
}

static void jw_key_double(json_writer_t *w, const char *key, double val) {
    jw_comma(w);
    jw_indent(w);
    jw_write_escaped(w->fp, key);
    fprintf(w->fp, ": %.6f", val);
}

static void jw_key_llu(json_writer_t *w, const char *key, unsigned long long val) {
    jw_comma(w);
    jw_indent(w);
    jw_write_escaped(w->fp, key);
    fprintf(w->fp, ": %llu", val);
}

static void jw_key_int(json_writer_t *w, const char *key, int val) {
    jw_comma(w);
    jw_indent(w);
    jw_write_escaped(w->fp, key);
    fprintf(w->fp, ": %d", val);
}

static void jw_key_bool(json_writer_t *w, const char *key, int val) {
    jw_comma(w);
    jw_indent(w);
    jw_write_escaped(w->fp, key);
    fprintf(w->fp, ": %s", val ? "true" : "false");
}

/* ── Status helper ────────────────────────────────────────────── */

static const char *status_to_text(run_status_t status) {
    switch (status) {
        case STATUS_PASS:    return "PASS";
        case STATUS_FAIL:    return "FAIL";
        case STATUS_STOPPED: return "STOPPED";
        case STATUS_SKIPPED:
        default:             return "SKIPPED";
    }
}

static const char *stability_status_to_text(const test_report_t *test) {
    if (test == NULL) {
        return "SKIPPED";
    }
    if (test->stability_status != STATUS_PASS) {
        return status_to_text(test->stability_status);
    }
    if (test->stability.status[0] != '\0') {
        return test->stability.status;
    }
    return "PASS";
}

static void write_environment_metadata(json_writer_t *w, int lang) {
    char hostname[256];
    char cwd[PATH_MAX];
    struct utsname uts;
    const char *hostname_value = NULL;
    const char *cwd_value = NULL;
    const char *sysname_value = NULL;
    const char *release_value = NULL;
    const char *version_value = NULL;
    const char *machine_value = NULL;

    memset(hostname, 0, sizeof(hostname));
    memset(cwd, 0, sizeof(cwd));
    memset(&uts, 0, sizeof(uts));

    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[sizeof(hostname) - 1U] = '\0';
        hostname_value = hostname;
    }
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        cwd_value = cwd;
    }
    if (uname(&uts) == 0) {
        sysname_value = uts.sysname;
        release_value = uts.release;
        version_value = uts.version;
        machine_value = uts.machine;
    }

    jw_begin_object(w, L(lang, "运行环境", "environment"));
    jw_key_optional_str(w, L(lang, "主机名", "hostname"), hostname_value);
    jw_key_optional_str(w, L(lang, "工作目录", "cwd"), cwd_value);
    jw_key_optional_str(w, L(lang, "操作系统", "sysname"), sysname_value);
    jw_key_optional_str(w, L(lang, "内核版本", "release"), release_value);
    jw_key_optional_str(w, L(lang, "系统版本", "version"), version_value);
    jw_key_optional_str(w, L(lang, "处理器架构", "machine"), machine_value);
    jw_end_object(w);
}

static FILE *open_json_report_file(const char *out_path) {
    int fd;
    struct stat st;
    FILE *fp;

    fd = open(out_path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0) {
        return NULL;
    }

    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        return NULL;
    }

    fp = fdopen(fd, "w");
    if (fp == NULL) {
        close(fd);
        return NULL;
    }
    return fp;
}

/* ── Public API ────────────────────────────────────────────────── */

int write_json_report(const cli_options_t *opts,
                      const run_report_t *report,
                      int overall_failed,
                      int lang,
                      const char *out_path) {
    FILE *fp;
    time_t now;
    struct tm tm_now;
    char timestamp[64];
    size_t i;
    json_writer_t w;

    if (opts == NULL || report == NULL || out_path == NULL) {
        return 0;
    }

    fp = open_json_report_file(out_path);
    if (fp == NULL) {
        ngcc_log_error("[report] failed to open json report: %s", out_path);
        return -1;
    }

    now = time(NULL);
    if (localtime_r(&now, &tm_now) == NULL) {
        memset(&tm_now, 0, sizeof(tm_now));
    }
    if (strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S%z", &tm_now) == 0) {
        strcpy(timestamp, "unknown");
    }

    jw_init(&w, fp);
    jw_begin_object(&w, NULL);

    jw_key_int(&w, L(lang, "报告版本", "schema_version"), 4);
    jw_key_str(&w, L(lang, "时间戳", "timestamp"), timestamp);
    jw_key_str(&w, L(lang, "算法库路径", "library"), opts->lib_path);

    /* 报告信息 */
    jw_begin_object(&w, L(lang, "报告信息", "report_metadata"));
    jw_key_str(&w, L(lang, "生成工具", "generator"), "ngcc_bench");
    jw_key_str(&w, L(lang, "工具版本", "generator_version"), NGCC_VERSION);
    jw_key_str(&w, L(lang, "输出路径", "json_out_path"), opts->json_out_path);
    jw_end_object(&w);

    write_environment_metadata(&w, lang);

    /* options */
    {
        char tests_str[64] = "";
        char modes_str[128] = "";
        if (opts->test_mask & TEST_MASK_HASH) { strcat(tests_str, "hash,"); }
        if (opts->test_mask & TEST_MASK_SIG)  { strcat(tests_str, "sig,"); }
        if (opts->test_mask & TEST_MASK_KEM)  { strcat(tests_str, "kem,"); }
        if (opts->test_mask & TEST_MASK_KEX)  { strcat(tests_str, "kex,"); }
        if (tests_str[0] != '\0') { tests_str[strlen(tests_str) - 1] = '\0'; }

        if (opts->mode_mask & MODE_MASK_CORRECTNESS) { strcat(modes_str, "correctness,"); }
        if (opts->mode_mask & MODE_MASK_PERFORMANCE) { strcat(modes_str, "performance,"); }
        if (opts->mode_mask & MODE_MASK_MEMORY)      { strcat(modes_str, "memory,"); }
        if (opts->mode_mask & MODE_MASK_STABILITY)   { strcat(modes_str, "stability,"); }
        if (modes_str[0] != '\0') { modes_str[strlen(modes_str) - 1] = '\0'; }

        jw_begin_object(&w, L(lang, "测试配置", "options"));
        jw_key_str(&w, L(lang, "测试算法", "tests"), tests_str);
        jw_key_str(&w, L(lang, "测试模式", "modes"), modes_str);
    }
    jw_key_double(&w, L(lang, "稳定性测试时长(小时)", "duration_hours"), opts->duration_hours);
    jw_key_llu(&w, L(lang, "稳定性最大用例数(次)", "stability_max_cases"), opts->stability_max_cases);
    jw_key_double(&w, L(lang, "稳定性采样间隔(毫秒)", "stability_sample_ms"), opts->stability_sample_ms);

    jw_begin_object(&w, L(lang, "稳定性判定阈值", "stability_thresholds"));
    jw_key_double(&w, L(lang, "吞吐量变异系数(%)", "throughput_cv_percent"), opts->stability_thresholds.stable_throughput_cv_percent);
    jw_key_double(&w, L(lang, "CPU周期变异系数(%)", "cycles_cv_percent"), opts->stability_thresholds.stable_cycles_cv_percent);
    jw_key_double(&w, L(lang, "耗时变异系数(%)", "time_cv_percent"), opts->stability_thresholds.stable_time_cv_percent);
    jw_key_double(&w, L(lang, "物理内存增长阈值(%)", "stable_rss_growth_percent"), opts->stability_thresholds.stable_rss_growth_percent);
    jw_key_llu(&w, L(lang, "物理内存增长绝对阈值(字节)", "stable_rss_growth_abs_bytes"), (unsigned long long) opts->stability_thresholds.stable_rss_growth_abs_bytes);
    jw_key_double(&w, L(lang, "错误率(%)", "error_rate_percent"), opts->stability_thresholds.stable_error_rate_percent);
    jw_end_object(&w);

    if (opts->kat_path != NULL) {
        jw_key_str(&w, L(lang, "KAT向量路径", "kat_path"), opts->kat_path);
    } else {
        jw_key_null(&w, L(lang, "KAT向量路径", "kat_path"));
    }
    jw_end_object(&w); /* 测试配置 */

    jw_begin_object(&w, L(lang, "测试结果", "test_results"));
    for (i = 0; i < sizeof(report->tests) / sizeof(report->tests[0]); ++i) {
        const test_report_t *test = &report->tests[i];

        jw_begin_object(&w, test->name);
        jw_key_bool(&w, L(lang, "已选择", "selected"), test->selected);
        jw_key_str(&w, L(lang, "正确性", "correctness"), status_to_text(test->correctness_status));
        jw_key_str(&w, L(lang, "性能", "performance"), status_to_text(test->performance_status));
        jw_key_str(&w, L(lang, "稳定性", "stability"), stability_status_to_text(test));

        /* kat */
        if (test->kat_used) {
            jw_begin_object(&w, L(lang, "KAT验证", "kat"));
            jw_key_llu(&w, L(lang, "总数(条)", "total"), test->kat_total);
            jw_key_llu(&w, L(lang, "通过(条)", "passed"), test->kat_passed);
            jw_key_llu(&w, L(lang, "失败(条)", "failed"), test->kat_failed);
            jw_end_object(&w);
        } else {
            jw_key_null(&w, L(lang, "KAT验证", "kat"));
        }

        /* performance_metrics — one object per msg_len */
        if (test->performance_status == STATUS_PASS && test->performance_count > 0) {
            int pi;
            jw_begin_object(&w, L(lang, "性能指标", "performance_metrics"));
            for (pi = 0; pi < test->performance_count; ++pi) {
                const ngcc_perf_result_t *p = &test->performance[pi];
                const char *label = (lang == LANG_EN) ? test->performance_labels_en[pi] : test->performance_labels[pi];
                if (label == NULL) { label = "unknown"; }
                jw_begin_object(&w, label);
                jw_key_llu(&w, L(lang, "测试次数(次)", "iterations"), p->iterations);
                jw_key_llu(&w, L(lang, "预热次数(次)", "warmup_iterations"), p->warmup_iterations);
                jw_key_double(&w, L(lang, "总耗时(毫秒)", "elapsed_ms"), p->elapsed_ms);
                if (test->is_hash) {
                    jw_key_double(&w, L(lang, "每次操作数据量(字节)", "bytes_per_op"), p->bytes_per_op);
                    if (p->cycles_available) {
                        jw_key_double(&w, L(lang, "CPU周期均值(周期/次)", "cycles_per_op_mean"), p->cycles_per_op);
                        jw_key_double(&w, L(lang, "CPU周期标准差(周期)", "cycles_stddev"), p->cycles_stddev);
                        jw_key_double(&w, L(lang, "CPU周期最小值(周期)", "cycles_min"), p->cycles_min);
                        jw_key_double(&w, L(lang, "CPU周期最大值(周期)", "cycles_max"), p->cycles_max);
                        jw_key_double(&w, L(lang, "CPU周期中位数(周期)", "cycles_median"), p->cycles_median);
                        jw_key_double(&w, L(lang, "CPU周期每字节(周期/字节)", "cycles_per_byte"), p->cycles_per_byte);
                        jw_key_double(&w, L(lang, "CPU周期变异系数(%)", "cycles_cv_percent"), p->cycles_cv_percent);
                    }
                    jw_key_double(&w, L(lang, "吞吐量(MB/s)", "throughput_mb_per_sec"), p->mb_per_sec);
                } else {
                    if (p->cycles_available) {
                        jw_key_double(&w, L(lang, "CPU周期均值(周期/次)", "cycles_per_op_mean"), p->cycles_per_op);
                        jw_key_double(&w, L(lang, "CPU周期标准差(周期)", "cycles_stddev"), p->cycles_stddev);
                        jw_key_double(&w, L(lang, "CPU周期最小值(周期)", "cycles_min"), p->cycles_min);
                        jw_key_double(&w, L(lang, "CPU周期最大值(周期)", "cycles_max"), p->cycles_max);
                        jw_key_double(&w, L(lang, "CPU周期中位数(周期)", "cycles_median"), p->cycles_median);
                        jw_key_double(&w, L(lang, "CPU周期变异系数(%)", "cycles_cv_percent"), p->cycles_cv_percent);
                    }
                    jw_key_double(&w, L(lang, "操作吞吐量(次/秒)", "ops_per_sec"), p->ops_per_sec);
                }
                jw_key_double(&w, L(lang, "单次耗时均值(毫秒)", "time_ms_mean"), p->time_ms_mean);
                jw_key_double(&w, L(lang, "单次耗时中位数(毫秒)", "time_ms_median"), p->time_ms_median);
                jw_key_double(&w, L(lang, "单次耗时标准差(毫秒)", "time_ms_stddev"), p->time_ms_stddev);
                jw_key_double(&w, L(lang, "单次耗时变异系数(%)", "time_ms_cv_percent"), p->time_ms_cv_percent);
                jw_end_object(&w);
            }
            jw_end_object(&w);
        } else {
            jw_key_null(&w, L(lang, "性能指标", "performance_metrics"));
        }

        /* stability_metrics */
        if (test->stability_status != STATUS_SKIPPED) {
            jw_begin_object(&w, L(lang, "稳定性指标", "stability_metrics"));
            jw_key_llu(&w, L(lang, "执行用例数(次)", "cases_run"), test->stability.cases_run);
            jw_key_llu(&w, L(lang, "采样次数(次)", "sample_count"), test->stability.sample_count);
            jw_key_double(&w, L(lang, "总耗时(秒)", "elapsed_seconds"), test->stability.elapsed_seconds);
            jw_key_bool(&w, L(lang, "是否中断", "interrupted"), test->stability.interrupted);
            jw_key_bool(&w, L(lang, "是否失败", "failed_flag"), test->stability.failed);
            jw_key_str(&w, L(lang, "状态", "status"), test->stability.status);
            if (test->is_hash) {
                jw_key_double(&w, L(lang, "吞吐量均值(MB/s)", "throughput_mean_mb_per_sec"), test->stability.throughput_mean_mb);
                jw_key_double(&w, L(lang, "吞吐量标准差(MB/s)", "throughput_stddev_mb_per_sec"), test->stability.throughput_stddev_mb);
                jw_key_double(&w, L(lang, "吞吐量变异系数(%)", "throughput_cv_percent_bytes"), test->stability.throughput_cv_percent_bytes);
                jw_key_double(&w, L(lang, "吞吐量最小值(MB/s)", "throughput_min_mb_per_sec"), test->stability.throughput_min_mb);
                jw_key_double(&w, L(lang, "吞吐量最大值(MB/s)", "throughput_max_mb_per_sec"), test->stability.throughput_max_mb);
            } else {
                jw_key_double(&w, L(lang, "操作吞吐量均值(次/秒)", "throughput_mean_ops"), test->stability.throughput_mean_ops);
                jw_key_double(&w, L(lang, "操作吞吐量标准差(次/秒)", "throughput_stddev_ops"), test->stability.throughput_stddev_ops);
                jw_key_double(&w, L(lang, "操作吞吐量变异系数(%)", "throughput_cv_percent_ops"), test->stability.throughput_cv_percent);
                jw_key_double(&w, L(lang, "操作吞吐量最小值(次/秒)", "throughput_min_ops"), test->stability.throughput_min_ops);
                jw_key_double(&w, L(lang, "操作吞吐量最大值(次/秒)", "throughput_max_ops"), test->stability.throughput_max_ops);
            }
            if (test->stability.cycles_available) {
                jw_key_double(&w, L(lang, "CPU周期均值(周期/次)", "cycles_per_op_mean"), test->stability.cycles_mean);
                jw_key_double(&w, L(lang, "CPU周期标准差(周期)", "cycles_stddev"), test->stability.cycles_stddev);
                jw_key_double(&w, L(lang, "CPU周期变异系数(%)", "cycles_cv_percent"), test->stability.cycles_cv_percent);
                jw_key_double(&w, L(lang, "CPU周期最小值(周期)", "cycles_min"), test->stability.cycles_min);
                jw_key_double(&w, L(lang, "CPU周期最大值(周期)", "cycles_max"), test->stability.cycles_max);
            }
            jw_key_double(&w, L(lang, "单次耗时均值(毫秒)", "time_ms_mean"), test->stability.time_mean_ms);
            jw_key_double(&w, L(lang, "单次耗时标准差(毫秒)", "time_ms_stddev"), test->stability.time_stddev_ms);
            jw_key_double(&w, L(lang, "单次耗时变异系数(%)", "time_ms_cv_percent"), test->stability.time_cv_percent);
            jw_key_double(&w, L(lang, "单次耗时最小值(毫秒)", "time_min_ms"), test->stability.time_min_ms);
            jw_key_double(&w, L(lang, "单次耗时最大值(毫秒)", "time_max_ms"), test->stability.time_max_ms);
            jw_key_llu(&w, L(lang, "物理内存增长(字节)", "rss_growth_abs_bytes"), (unsigned long long) test->stability.rss_growth_abs_bytes);
            jw_key_double(&w, L(lang, "物理内存增长(%)", "rss_growth_percent"), test->stability.rss_growth_percent);
            jw_key_llu(&w, L(lang, "总执行次数(次)", "total_executions"), test->stability.total_executions);
            jw_key_llu(&w, L(lang, "错误次数(次)", "error_count"), test->stability.error_count);
            jw_key_double(&w, L(lang, "错误率(%)", "error_rate_percent"), test->stability.error_rate_percent);
            jw_key_bool(&w, L(lang, "性能稳定", "performance_stable"), test->stability.performance_stable);
            jw_key_bool(&w, L(lang, "内存稳定", "memory_stable"), test->stability.memory_stable);
            jw_key_bool(&w, L(lang, "正确性稳定", "correctness_stable"), test->stability.correctness_stable);
            jw_key_bool(&w, L(lang, "整体稳定", "is_stable"), test->stability.is_stable);
            jw_key_str(&w, L(lang, "失败原因", "failure_reasons"), test->stability.failure_reasons);
            jw_end_object(&w);
        } else {
            jw_key_null(&w, L(lang, "稳定性指标", "stability_metrics"));
        }

        if (test->memory_status != STATUS_SKIPPED) {
            jw_begin_object(&w, L(lang, "内存指标", "memory_metrics"));
            jw_key_str(&w, L(lang, "状态", "status"), status_to_text(test->memory_status));
            jw_key_llu(&w, L(lang, "静态内存占用(字节)", "static_memory_bytes"), (unsigned long long) test->static_memory_bytes);
            jw_key_llu(&w, L(lang, "峰值内存占用(字节)", "peak_memory_bytes"), (unsigned long long) test->peak_memory_bytes);
            jw_end_object(&w);
        } else {
            jw_key_null(&w, L(lang, "内存指标", "memory_metrics"));
        }

        jw_end_object(&w); /* test */
    }
    jw_end_object(&w); /* 测试结果 */

    /* 总体结果 */
    jw_begin_object(&w, L(lang, "总体结果", "overall"));
    jw_key_str(&w, L(lang, "状态", "status"), overall_failed ? "FAIL" : "PASS");
    jw_end_object(&w);

    jw_end_object(&w); /* root */
    fputc('\n', fp);

    fclose(fp);
    return 0;
}

int write_json_reports(const cli_options_t *opts,
                       const run_report_t *report,
                       int overall_failed) {
    char path_zh[4096];
    char path_en[4096];
    int path_zh_len;
    int path_en_len;
    int rc;

    if (opts == NULL || report == NULL || opts->json_out_path == NULL) {
        return 0;
    }

    path_zh_len = snprintf(path_zh, sizeof(path_zh), "%s.zh", opts->json_out_path);
    path_en_len = snprintf(path_en, sizeof(path_en), "%s.en", opts->json_out_path);
    if (path_zh_len < 0 || path_zh_len >= (int) sizeof(path_zh) ||
        path_en_len < 0 || path_en_len >= (int) sizeof(path_en)) {
        ngcc_log_error("[report] json report path is too long: %s", opts->json_out_path);
        printf("[report] END status=FAIL path=%s\n", opts->json_out_path);
        fflush(stdout);
        return -1;
    }

    rc = write_json_report(opts, report, overall_failed, LANG_ZH, path_zh);
    if (rc != 0) {
        printf("[report] END status=FAIL path=%s\n", path_zh);
        fflush(stdout);
        return rc;
    }
    rc = write_json_report(opts, report, overall_failed, LANG_EN, path_en);
    if (rc != 0) {
        printf("[report] END status=FAIL path=%s\n", path_en);
        fflush(stdout);
        return rc;
    }
    printf("[report] END status=PASS path_zh=%s path_en=%s\n", path_zh, path_en);
    fflush(stdout);
    return rc;
}
