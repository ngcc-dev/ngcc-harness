#define _POSIX_C_SOURCE 200809L

#include "test_support.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#ifndef NGCC_DEFAULT_MOCK_LIB
#define NGCC_DEFAULT_MOCK_LIB ""
#endif

static void usage(void) {
    fprintf(stderr,
            "Usage:\n"
            "  ngcc_stability_profile --benchmark /path/to/ngcc_bench\n"
            "                         [--lib /path/to/lib.so]\n"
            "                         [--profile quick|soak|nightly]\n"
            "                         [--output-dir DIR]\n"
            "                         [--allow-unstable]\n");
}

static const char *read_profile_duration(const char *profile) {
    if (strcmp(profile, "quick") == 0) {
        return "0.002";
    }
    if (strcmp(profile, "soak") == 0) {
        return "0.05";
    }
    if (strcmp(profile, "nightly") == 0) {
        return "0.25";
    }
    return NULL;
}

static const char *read_profile_cases(const char *profile) {
    if (strcmp(profile, "quick") == 0) {
        return "200000";
    }
    if (strcmp(profile, "soak") == 0) {
        return "2000000";
    }
    if (strcmp(profile, "nightly") == 0) {
        return "10000000";
    }
    return NULL;
}

static const char *read_profile_sample_ms(const char *profile) {
    if (strcmp(profile, "quick") == 0) {
        return "2.0";
    }
    if (strcmp(profile, "soak") == 0 || strcmp(profile, "nightly") == 0) {
        return "5.0";
    }
    return NULL;
}

int main(int argc, char **argv) {
    const char *benchmark = NULL;
    const char *lib_path = NULL;
    const char *profile = "quick";
    const char *output_dir = "reports/stability";
    int allow_unstable = 0;
    const char *duration_hours;
    const char *max_cases;
    const char *sample_ms;
    char timestamp[32];
    char report_json[PATH_MAX];
    char meta_txt[PATH_MAX];
    char log_txt[PATH_MAX];
    time_t now;
    struct tm tm_now;
    FILE *meta;
    struct utsname uts;
    char hostname[256];
    test_command_result_t result;

    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--benchmark") == 0 && i + 1 < argc) {
            benchmark = argv[++i];
        } else if (strcmp(argv[i], "--lib") == 0 && i + 1 < argc) {
            lib_path = argv[++i];
        } else if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
            profile = argv[++i];
        } else if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (strcmp(argv[i], "--allow-unstable") == 0) {
            allow_unstable = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else {
            usage();
            return 2;
        }
    }

    if (benchmark == NULL) {
        usage();
        return 2;
    }
    if (access(benchmark, X_OK) != 0) {
        fprintf(stderr, "benchmark binary not executable: %s\n", benchmark);
        return 2;
    }

    duration_hours = read_profile_duration(profile);
    max_cases = read_profile_cases(profile);
    sample_ms = read_profile_sample_ms(profile);
    if (duration_hours == NULL || max_cases == NULL || sample_ms == NULL) {
        fprintf(stderr, "invalid --profile: %s\n", profile);
        return 2;
    }

    if (lib_path == NULL || lib_path[0] == '\0') {
        lib_path = NGCC_DEFAULT_MOCK_LIB;
        allow_unstable = 1;
    }
    if (lib_path[0] == '\0' || access(lib_path, R_OK) != 0) {
        fprintf(stderr, "lib not found: %s\n", lib_path);
        return 2;
    }
    if (test_mkdir_p(output_dir) != 0) {
        fprintf(stderr, "failed to create output dir: %s\n", output_dir);
        return 1;
    }

    now = time(NULL);
    localtime_r(&now, &tm_now);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm_now);

    if (snprintf(report_json, sizeof(report_json), "%s/stability_%s_%s.json", output_dir, profile, timestamp) >= (int) sizeof(report_json) ||
        snprintf(meta_txt, sizeof(meta_txt), "%s/stability_%s_%s.meta.txt", output_dir, profile, timestamp) >= (int) sizeof(meta_txt) ||
        snprintf(log_txt, sizeof(log_txt), "%s/stability_%s_%s.log", output_dir, profile, timestamp) >= (int) sizeof(log_txt)) {
        fprintf(stderr, "output path too long\n");
        return 1;
    }

    meta = fopen(meta_txt, "wb");
    if (meta == NULL) {
        fprintf(stderr, "failed to open meta file: %s\n", strerror(errno));
        return 1;
    }
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S%z", &tm_now);
    fprintf(meta, "timestamp=%s\nprofile=%s\nbenchmark=%s\nlib=%s\nduration_hours=%s\nmax_cases=%s\nsample_ms=%s\n",
            timestamp, profile, benchmark, lib_path, duration_hours, max_cases, sample_ms);
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        fprintf(meta, "hostname=%s\n", hostname);
    }
    if (uname(&uts) == 0) {
        fprintf(meta, "kernel=%s %s %s %s\n", uts.sysname, uts.release, uts.version, uts.machine);
    }
    fclose(meta);

    {
        char *const cmd[] = {
            (char *) benchmark,
            "--lib", (char *) lib_path,
            "--test", "all",
            "--mode", "stability",
            "--digest-len-bits", "8",
            "--duration-hours", (char *) duration_hours,
            "--stability-max-cases", (char *) max_cases,
            "--stability-sample-ms", (char *) sample_ms,
            "--json-out", report_json,
            NULL
        };
        if (test_run_command(cmd, &result) != 0) {
            fprintf(stderr, "failed to execute benchmark\n");
            return 1;
        }
    }

    if (test_write_file(log_txt, result.output) != 0) {
        fprintf(stderr, "failed to write log file\n");
        test_free_command_result(&result);
        return 1;
    }

    printf("%s", result.output);
    printf("report_json=%s\n", report_json);
    printf("log_file=%s\n", log_txt);

    if (result.exit_code != 0 && !allow_unstable) {
        test_free_command_result(&result);
        return result.exit_code;
    }
    if (result.exit_code != 0) {
        printf("stability run returned rc=%d but allowed by --allow-unstable\n", result.exit_code);
    }
    printf("stability profile completed: %s\n", report_json);
    test_free_command_result(&result);
    return 0;
}
