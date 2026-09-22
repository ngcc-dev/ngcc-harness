/*
 * Guard-page witness for the BRA reference field-multiplier bounds.
 * Compile once per BRA_FIELD (67, 83, or 127).
 */
#define _GNU_SOURCE
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include "drng.h"
#include "rbc_elt.h"

DRNG_ctx drng_algorithm;

#if BRA_FIELD == 67
# define UR_SIZE RBC_67_ELT_UR_SIZE
# define EXPECT_OOB 1
#elif BRA_FIELD == 83
# define UR_SIZE RBC_83_ELT_UR_SIZE
# define EXPECT_OOB 1
#elif BRA_FIELD == 127
# define UR_SIZE RBC_127_ELT_UR_SIZE
# define EXPECT_OOB 0
#else
# error unsupported BRA_FIELD
#endif

static int exercise(int guard)
{
    long page_size = sysconf(_SC_PAGESIZE);
    unsigned char *pages;
    rbc_elt e1 = {0}, e2 = {0};
    uint64_t *out;

    if (page_size <= 0)
        return 2;
    pages = mmap(NULL, (size_t)page_size * 2, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pages == MAP_FAILED)
        return 2;
    if (guard && mprotect(pages + page_size, (size_t)page_size, PROT_NONE) != 0)
        return 2;

    /* The declared output ends exactly at the first page boundary. */
    out = (uint64_t *)(pages + page_size - UR_SIZE * sizeof(uint64_t));
    e1[1] = 1;
    e2[1] = 1;
    rbc_elt_ur_mul(out, e1, e2);
    munmap(pages, (size_t)page_size * 2);
    return 0;
}

int main(void)
{
    struct rlimit no_core = {0, 0};
    int status;
    pid_t child;

    if (exercise(0) != 0) {
        fprintf(stderr, "ERROR: writable-page control failed\n");
        return 2;
    }
    printf("CONTROL GF(2^%d) multiplication returned with a writable next page\n",
           BRA_FIELD);
    fflush(stdout);

    child = fork();
    if (child < 0)
        return 2;
    if (child == 0) {
        (void)setrlimit(RLIMIT_CORE, &no_core);
        _exit(exercise(1));
    }
    if (waitpid(child, &status, 0) != child)
        return 2;

    if (EXPECT_OOB && WIFSIGNALED(status) &&
        (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS)) {
        printf("CONFIRMED kem-06-2: GF(2^%d) touched one limb past rbc_elt_ur\n",
               BRA_FIELD);
        return 0;
    }
    if (!EXPECT_OOB && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("CONTROL GF(2^%d): exact-size guarded output stayed in bounds\n",
               BRA_FIELD);
        return 0;
    }

    fprintf(stderr, "NOT-CONFIRMED GF(2^%d): child status 0x%x\n",
            BRA_FIELD, status);
    return 1;
}
