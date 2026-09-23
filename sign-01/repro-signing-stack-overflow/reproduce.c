#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "link_common.h"
#include "link_sig.h"

enum child_result {
    CHILD_ASAN = 0,
    CHILD_ACCEPTED = 1,
    CHILD_REJECTED = 2,
    CHILD_FAILED = 3
};

static void fail(const char *message)
{
    fprintf(stderr, "signing-stack-overflow: %s\n", message);
    exit(2);
}

static void *symbol(void *library, const char *name)
{
    void *value = dlsym(library, name);
    if (value == NULL)
        fail(dlerror());
    return value;
}

static int contains(const char *text, const char *needle)
{
    return strstr(text, needle) != NULL;
}

static enum child_result asan_sign(ngcc_sig_sign_fn sign,
                                   unsigned char *secret_key,
                                   unsigned long long secret_key_length,
                                   unsigned char *message,
                                   unsigned long long message_length,
                                   unsigned char *signature,
                                   unsigned long long signature_capacity)
{
    int pipe_fds[2];
    if (pipe(pipe_fds) != 0)
        return CHILD_FAILED;
    pid_t child = fork();
    if (child < 0) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return CHILD_FAILED;
    }
    if (child == 0) {
        close(pipe_fds[0]);
        if (dup2(pipe_fds[1], STDERR_FILENO) < 0)
            _exit(126);
        close(pipe_fds[1]);
        unsigned long long signature_length = signature_capacity;
        int result = sign(secret_key, secret_key_length, message,
                          message_length, signature, &signature_length);
        _exit(result == 0 ? 0 : 10);
    }

    close(pipe_fds[1]);
    size_t length = 0;
    size_t capacity = 4096;
    char *diagnostic = calloc(capacity, 1);
    int read_failed = diagnostic == NULL;
    int timed_out = 0;
    while (!read_failed && !timed_out) {
        struct pollfd descriptor = {
            .fd = pipe_fds[0],
            .events = POLLIN,
            .revents = 0
        };
        int ready = poll(&descriptor, 1, 10000);
        if (ready < 0) {
            if (errno == EINTR)
                continue;
            read_failed = 1;
            break;
        }
        if (ready == 0) {
            timed_out = 1;
            break;
        }
        char chunk[4096];
        ssize_t received = read(pipe_fds[0], chunk, sizeof chunk);
        if (received == 0)
            break;
        if (received < 0) {
            if (errno == EINTR)
                continue;
            read_failed = 1;
            break;
        }
        size_t received_size = (size_t)received;
        if (received_size > SIZE_MAX - length - 1) {
            read_failed = 1;
            break;
        }
        size_t required = length + received_size + 1;
        if (required > capacity) {
            size_t new_capacity = capacity;
            while (new_capacity < required) {
                if (new_capacity > SIZE_MAX / 2) {
                    read_failed = 1;
                    break;
                }
                new_capacity *= 2;
            }
            if (read_failed)
                break;
            char *grown = realloc(diagnostic, new_capacity);
            if (grown == NULL) {
                read_failed = 1;
                break;
            }
            diagnostic = grown;
            capacity = new_capacity;
        }
        memcpy(diagnostic + length, chunk, received_size);
        length += received_size;
        diagnostic[length] = '\0';
    }
    if (timed_out)
        kill(child, SIGKILL);
    close(pipe_fds[0]);

    int status = 0;
    int child_exited = waitpid(child, &status, 0) >= 0;
    int matched = !read_failed && !timed_out && child_exited &&
                  WIFEXITED(status) && WEXITSTATUS(status) == 1 &&
                  diagnostic != NULL &&
                  contains(diagnostic, "stack-buffer-overflow") &&
                  contains(diagnostic, "polyvecl_uniform_gamma1");
    enum child_result result = CHILD_FAILED;
    if (matched)
        result = CHILD_ASAN;
    else if (!read_failed && !timed_out && child_exited && WIFEXITED(status) &&
             WEXITSTATUS(status) == 0)
        result = CHILD_ACCEPTED;
    else if (!read_failed && !timed_out && child_exited && WIFEXITED(status) &&
             WEXITSTATUS(status) == 10)
        result = CHILD_REJECTED;
    free(diagnostic);
    return result;
}

static const char *result_name(enum child_result result)
{
    switch (result) {
    case CHILD_ASAN:
        return "asan-diagnostic";
    case CHILD_ACCEPTED:
        return "accepted";
    case CHILD_REJECTED:
        return "rejected";
    default:
        return "child-failure";
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
        fail("usage: reproduce CONTROL_LIBRARY ASAN_LIBRARY");
    void *control_library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    void *asan_library = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (control_library == NULL || asan_library == NULL)
        fail(dlerror());

    const ngcc_meta_sig_t *meta = (const ngcc_meta_sig_t *)
        ((const ngcc_meta_t *(*)(void))symbol(control_library, "ngcc_meta"))();
    int (*seed)(const unsigned char *, unsigned long long) =
        (int (*)(const unsigned char *, unsigned long long))
        symbol(control_library, "ngcc_seed");
    ngcc_sig_keygen_fn keygen =
        (ngcc_sig_keygen_fn)symbol(control_library, "sig_keygen");
    ngcc_sig_sign_fn control_sign =
        (ngcc_sig_sign_fn)symbol(control_library, "sig_sign");
    ngcc_sig_verify_fn control_verify =
        (ngcc_sig_verify_fn)symbol(control_library, "sig_verify");
    ngcc_sig_sign_fn asan_sign_fn =
        (ngcc_sig_sign_fn)symbol(asan_library, "sig_sign");
    if (meta == NULL || meta->h.magic != NGCC_META_MAGIC ||
        meta->h.type != NGCC_TYPE_SIG || strcmp(meta->h.id, "sign-01") != 0 ||
        meta->pk_len == 0 || meta->sk_len == 0 || meta->sn_len == 0)
        fail("library metadata is not sign-01");

    unsigned char *public_key = calloc(meta->pk_len, 1);
    unsigned char *secret_key = calloc(meta->sk_len, 1);
    unsigned char *signature = calloc(meta->sn_len, 1);
    if (public_key == NULL || secret_key == NULL || signature == NULL)
        fail("allocation failed");
    unsigned char seed_bytes[48];
    for (size_t i = 0; i < sizeof seed_bytes; i++)
        seed_bytes[i] = (unsigned char)(0x63u + i);
    static unsigned char message[] = "signing-stack-overflow valid signing control";
    unsigned long long public_key_length = meta->pk_len;
    unsigned long long secret_key_length = meta->sk_len;
    unsigned long long signature_length = meta->sn_len;
    if (seed(seed_bytes, sizeof seed_bytes) != 0 ||
        keygen(public_key, &public_key_length, secret_key,
               &secret_key_length) != 0 ||
        control_sign(secret_key, secret_key_length, message, sizeof message - 1,
                     signature, &signature_length) != 0 ||
        control_verify(public_key, public_key_length, signature,
                       signature_length, message, sizeof message - 1) != 0)
        fail("full-size signing control failed");

    enum child_result result = asan_sign(
        asan_sign_fn, secret_key, secret_key_length, message,
        sizeof message - 1, signature, meta->sn_len);
    printf("signing-stack-overflow full-round-trip=passed asan-sign=%s\n",
           result_name(result));
    free(public_key);
    free(secret_key);
    free(signature);
    dlclose(asan_library);
    dlclose(control_library);
    return result == CHILD_ASAN ? 0 : 1;
}
