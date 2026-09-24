#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/utsname.h>
#include <ctype.h>
#include "report.h"
#include "test_util.h"

#define CPUINFO_PATH    "/proc/cpuinfo"
#define MEMINFO_PATH    "/proc/meminfo"

extern FILE * g_log_file;

static void print_date(void) {
    char *date_str = get_timestamp("%Y-%m-%d");
    BENCH_LOG("Date: %s\n", date_str);
}

static char *trim_whitespace(char *str) {
    while(isspace((unsigned char)(*str)))
        str++;
    return str;
}

static void print_cpu_frequency(void) {
    FILE *cpuinfo = fopen(CPUINFO_PATH, "r");
    char line[256];
    char *cpu_freq_mhz = NULL;
    float cpu_freq_ghz = 0;

    if (cpuinfo == NULL) {
        fprintf(stderr, "Failure to open cpuinfo %s\n", CPUINFO_PATH);
        return;
    }
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (strncmp(line, "cpu MHz", 7) == 0) {
            cpu_freq_mhz = strchr(line, ':') + 1;
            cpu_freq_ghz = (atoi(trim_whitespace(cpu_freq_mhz)) * 1.0) / 1000;
            BENCH_LOG("\tFrequency of CPU:\t%.2f GHz\n", cpu_freq_ghz);
            break;
        }
    }
}

static void print_core_num(void) {
    FILE *cpuinfo = fopen(CPUINFO_PATH, "r");
    char line[256];
    char *core_num = NULL;

    if (cpuinfo == NULL) {
        fprintf(stderr, "Failure to open cpuinfo %s\n", CPUINFO_PATH);
        return;
    }
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (strncmp(line, "cpu cores", 9) == 0) {
            core_num = trim_whitespace(strchr(line, ':') + 1);
            BENCH_LOG("\tTotal core number:\t%s", core_num);
            // BENCH_LOG("\tTotal core number:\t1\n"); 
            break;
        }
    }
}


static void print_meminfo(void) {
    FILE *cpuinfo = fopen(MEMINFO_PATH, "r");
    char line[256];
    char *mem_total;
    size_t mem_size = 0;

    if (cpuinfo == NULL) {
        fprintf(stderr, "Failure to open meminfo %s\n", MEMINFO_PATH);
        return;
    }
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (strncmp(line, "MemTotal", 8) == 0) {
            mem_total = trim_whitespace(strchr(line, ':') + 1);
            mem_size = strtol(mem_total, NULL, 10) >> 20;
            BENCH_LOG("\tTotal memory size:\t%ld GB", mem_size);
            break;
        }
    }
}

static void print_cpuinfo(void) {
#if false
    FILE *cpuinfo = fopen(CPUINFO_PATH, "r");
    char line[256];
    char *cpu_info = NULL;

    if (cpuinfo == NULL) {
        fprintf(stderr, "Failure to open cpuinfo %s\n", CPUINFO_PATH);
        return;
    }
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (strncmp(line, "model name", 10) == 0) {
            cpu_info = strchr(line, ':') + 1;
            for(int i = 0; cpu_info[i] != '\0'; i++) {
                if(cpu_info[i    ] == 'C' &&
                   cpu_info[i + 1] == '8' &&
                   cpu_info[i + 2] == '6') {
                    cpu_info[i] = 'X';
                    break;
                }
            }
            break;
        }
    }
#endif
    BENCH_LOG("Perform the test on CPU Model:\tARM\n");
    print_cpu_frequency();
    print_core_num();
    print_meminfo();
}

static void print_uname(void) {
    struct utsname details = { 0 };

    if (uname(&details) != 0) {
        fprintf(stderr, "Failure to get the uname of test environment\n");
    }

    BENCH_LOG("Operating System platform: %s %s\n", details.sysname, details.machine);
}

void print_banner(void){
    print_date();
    print_uname();
    print_cpuinfo();
    BENCH_LOG("\n");
}