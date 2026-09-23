#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "link_common.h"
#include "link_sig.h"

enum short_result {
    SHORT_SETUP = -1,
    SHORT_REJECTED = 0,
    SHORT_FAULT = 1,
    SHORT_ACCEPTED = 2
};

static void fail(const char *message)
{
    fprintf(stderr, "short-key-read: %s\n", message);
    exit(2);
}

static void *load_symbol(void *library, const char *name)
{
    void *value = dlsym(library, name);
    if (value == NULL)
        fail(dlerror());
    return value;
}

static void short_fault(int signal_number)
{
    _exit(128 + signal_number);
}

static int install_fault_handlers(void)
{
    struct sigaction action = {0};
    action.sa_handler = short_fault;
    sigemptyset(&action.sa_mask);
    return sigaction(SIGSEGV, &action, NULL) == 0 &&
           sigaction(SIGBUS, &action, NULL) == 0;
}

static enum short_result run_short_sign(ngcc_sig_sign_fn sign,
                                        unsigned char *signature,
                                        unsigned long long signature_len,
                                        unsigned char *message,
                                        unsigned long long message_len)
{
    pid_t child = fork();
    if (child < 0)
        return SHORT_SETUP;
    if (child == 0) {
        if (!install_fault_handlers())
            _exit(125);
        unsigned char *short_key = NULL;
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size <= 0)
            _exit(125);
        unsigned char *mapping = mmap(NULL, (size_t)page_size * 2,
                                      PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapping == MAP_FAILED ||
            mprotect(mapping + page_size, (size_t)page_size, PROT_NONE) != 0)
            _exit(125);
        short_key = mapping + page_size - 1;
        *short_key = 0;
        unsigned long long output_len = signature_len;
        int result = sign(short_key, 0, message, message_len, signature,
                          &output_len);
        munmap(mapping, (size_t)page_size * 2);
        _exit(result == 0 ? 0 : 1);
    }

    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR)
            return SHORT_SETUP;
    }
    if (WIFSIGNALED(status) &&
        (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS))
        return SHORT_FAULT;
    if (WIFEXITED(status) &&
        (WEXITSTATUS(status) == 128 + SIGSEGV ||
         WEXITSTATUS(status) == 128 + SIGBUS))
        return SHORT_FAULT;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return SHORT_ACCEPTED;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
        return SHORT_REJECTED;
    return SHORT_SETUP;
}

static enum short_result run_short_verify(ngcc_sig_verify_fn verify,
                                          unsigned char *signature,
                                          unsigned long long signature_len,
                                          unsigned char *message,
                                          unsigned long long message_len)
{
    pid_t child = fork();
    if (child < 0)
        return SHORT_SETUP;
    if (child == 0) {
        if (!install_fault_handlers())
            _exit(125);
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size <= 0)
            _exit(125);
        unsigned char *mapping = mmap(NULL, (size_t)page_size * 2,
                                      PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapping == MAP_FAILED ||
            mprotect(mapping + page_size, (size_t)page_size, PROT_NONE) != 0)
            _exit(125);
        unsigned char *short_key = mapping + page_size - 1;
        *short_key = 0;
        int result = verify(short_key, 0, signature, signature_len, message,
                            message_len);
        munmap(mapping, (size_t)page_size * 2);
        _exit(result == 0 ? 0 : 1);
    }

    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR)
            return SHORT_SETUP;
    }
    if (WIFSIGNALED(status) &&
        (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS))
        return SHORT_FAULT;
    if (WIFEXITED(status) &&
        (WEXITSTATUS(status) == 128 + SIGSEGV ||
         WEXITSTATUS(status) == 128 + SIGBUS))
        return SHORT_FAULT;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return SHORT_ACCEPTED;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
        return SHORT_REJECTED;
    return SHORT_SETUP;
}

static const char *short_result_name(enum short_result result)
{
    switch (result) {
    case SHORT_REJECTED:
        return "rejected";
    case SHORT_FAULT:
        return "guard-fault";
    case SHORT_ACCEPTED:
        return "accepted";
    default:
        return "setup-failed";
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
        fail("usage: reproduce LIBRARY");

    void *library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (library == NULL)
        fail(dlerror());

    const ngcc_meta_t *(*get_meta)(void) =
        (const ngcc_meta_t *(*)(void))load_symbol(library, "ngcc_meta");
    int (*seed)(const unsigned char *, unsigned long long) =
        (int (*)(const unsigned char *, unsigned long long))
        load_symbol(library, "ngcc_seed");
    ngcc_sig_keygen_fn keygen =
        (ngcc_sig_keygen_fn)load_symbol(library, "sig_keygen");
    ngcc_sig_sign_fn sign =
        (ngcc_sig_sign_fn)load_symbol(library, "sig_sign");
    ngcc_sig_verify_fn verify =
        (ngcc_sig_verify_fn)load_symbol(library, "sig_verify");
    const ngcc_meta_sig_t *meta = (const ngcc_meta_sig_t *)get_meta();
    if (meta == NULL || meta->h.magic != NGCC_META_MAGIC ||
        meta->h.type != NGCC_TYPE_SIG || meta->pk_len == 0 ||
        meta->sk_len == 0 || meta->sn_len == 0)
        fail("invalid signature metadata");

    unsigned char seed_bytes[64];
    for (size_t i = 0; i < sizeof seed_bytes; i++)
        seed_bytes[i] = (unsigned char)(0x23 + 29 * i);
    unsigned char message[] = "short-key-read full-size control";
    unsigned char *pk = calloc((size_t)meta->pk_len, 1);
    unsigned char *sk = calloc((size_t)meta->sk_len, 1);
    unsigned char *signature = calloc((size_t)meta->sn_len, 1);
    if (pk == NULL || sk == NULL || signature == NULL)
        fail("allocation failed");

    unsigned long long pk_len = meta->pk_len;
    unsigned long long sk_len = meta->sk_len;
    unsigned long long signature_len = meta->sn_len;
    if (seed(seed_bytes, sizeof seed_bytes) != 0 ||
        keygen(pk, &pk_len, sk, &sk_len) != 0 ||
        pk_len != meta->pk_len || sk_len != meta->sk_len ||
        sign(sk, sk_len, message, sizeof message - 1, signature,
             &signature_len) != 0 || signature_len > meta->sn_len ||
        verify(pk, pk_len, signature, signature_len, message,
               sizeof message - 1) != 0)
        fail("full-size key round trip failed");
    if (getenv("A1_FORCE_CONTROL_FAILURE") != NULL)
        fail("forced control failure");

    enum short_result short_sign = run_short_sign(
        sign, signature, signature_len, message, sizeof message - 1);
    enum short_result short_verify = run_short_verify(
        verify, signature, signature_len, message, sizeof message - 1);
    printf("short-key-read full=accepted short_sign=%s short_verify=%s\n",
           short_result_name(short_sign), short_result_name(short_verify));

    free(pk);
    free(sk);
    free(signature);
    dlclose(library);
    if (short_sign == SHORT_FAULT && short_verify == SHORT_FAULT) {
        printf("CONFIRMED: truncated key buffers are unpacked\n");
        return 0;
    }
    fprintf(stderr, "NOT-CONFIRMED: short keys were not unpacked\n");
    return 1;
}
