#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "bench_local.h"

#define LINE_SIZE 256
#define MASSIF_FILE_SIZE 128

static int parse_massif(const char *massif_file, struct mem_profile *mem_prof) {
    FILE *f = NULL;
    char line[LINE_SIZE];
    size_t heap_peak = 0;
    size_t stack_peak = 0;

    f = fopen(massif_file, "r");
    if(f == NULL) {
        fprintf(stderr, "Failed to open massif output file %s: %s\n", massif_file, strerror(errno));
        return -1;
    }

    while(fgets(line, LINE_SIZE, f)) {
        size_t v;

        if(sscanf(line, "mem_heap_B=%zu", &v) == 1 && v > heap_peak) {
            heap_peak = v;
        }

        if(sscanf(line, "mem_stacks_B=%zu", &v) == 1 && v > stack_peak) {
            stack_peak = v;
        }
    }

    fclose(f);
    mem_prof->heap_peak = heap_peak;
    mem_prof->stack_peak = stack_peak;
    return 0;
}

int measure_memory(const char *worker_path, struct mem_profile *mem_prof) {
    pid_t pid;
    int status = 0;
    char massif_file[MASSIF_FILE_SIZE] = { 0 };
    char massif_arg[MASSIF_FILE_SIZE + 32] = { 0 };

    snprintf(massif_file, MASSIF_FILE_SIZE, "massif.%d.out", getpid());
    snprintf(massif_arg, sizeof(massif_arg), "--massif-out-file=%s", massif_file);

    remove(massif_file);

    pid = fork();
    if(pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return -1;
    }

    if(pid == 0) {
        char *const argv[] = {
            "/usr/bin/valgrind",
            "--tool=massif",
            "--stacks=yes",
            massif_arg,
            "--quiet",
            (char *)worker_path,
            NULL
        };

        char *const envp[] = {
            "PATH=/usr/bin:/bin",
            "LANG=C",
            "LC_ALL=C",
            NULL
        };

        execve("/usr/bin/valgrind", argv, envp);

        fprintf(stderr, "Failed to execute /usr/bin/valgrind: %s\n", strerror(errno));
        _exit(127);
    }

    if(waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "waitpid failed: %s\n", strerror(errno));
        return -1;
    }

    if(WIFSIGNALED(status)) {
        fprintf(stderr, "valgrind worker killed by signal %d: %s\n",
                WTERMSIG(status), worker_path);
        return -1;
    }

    if(!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "valgrind worker failed: %s, exit code=%d\n",
                worker_path, WEXITSTATUS(status));
        return -1;
    }

    return parse_massif(massif_file, mem_prof);
}