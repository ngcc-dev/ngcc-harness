#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "auxfunc.h"
#include "symmetric.h"

#define SHAKE_META_WORDS 2
#define SHAKE_BUF_WORDS (28 - SHAKE_META_WORDS)
#define SHAKE_BUF_BYTES (SHAKE_BUF_WORDS * sizeof(uint64_t))

static size_t shake_len(const shake_t *instance) { return (size_t)instance->state[0]; }
static size_t shake_off(const shake_t *instance) { return (size_t)instance->state[1]; }
static void shake_set_len(shake_t *instance, size_t v) { instance->state[0] = (uint64_t)v; }
static void shake_set_off(shake_t *instance, size_t v) { instance->state[1] = (uint64_t)v; }
static uint8_t *shake_buf(shake_t *instance) { return (uint8_t *)&instance->state[SHAKE_META_WORDS]; }
static const uint8_t *shake_cbuf(const shake_t *instance) { return (const uint8_t *)&instance->state[SHAKE_META_WORDS]; }
static void shake_reset(shake_t *instance) { memset(instance, 0, sizeof(*instance)); }

static void shake_do_squeeze(uint8_t *out, size_t outlen, shake_t *instance) {
    size_t len = shake_len(instance);
    size_t off = shake_off(instance);
    size_t need = off + outlen;
    uint8_t *tmp = (uint8_t *)malloc(need ? need : 1);
    if (tmp == NULL) {
        memset(out, 0, outlen);
        return;
    }
    if (pseudoXOF((unsigned long long)need * 8ULL, shake_cbuf(instance), (unsigned long long)len * 8ULL, tmp) != 0) {
        memset(out, 0, outlen);
        free(tmp);
        return;
    }
    memcpy(out, tmp + off, outlen);
    shake_set_off(instance, need);
    free(tmp);
}

void shake128_init(shake_t *instance) { shake_reset(instance); }
void shake256_init(shake_t *instance) { shake_reset(instance); }
void shake_absorb(shake_t *instance, const uint8_t *in, size_t inlen) {
    size_t len = shake_len(instance);
    if (len + inlen > SHAKE_BUF_BYTES) {
        inlen = (len < SHAKE_BUF_BYTES) ? (SHAKE_BUF_BYTES - len) : 0;
    }
    if (inlen > 0) {
        memcpy(shake_buf(instance) + len, in, inlen);
        shake_set_len(instance, len + inlen);
    }
}
void shake_finalize(shake_t *instance) { (void)instance; }
void shake_squeeze(uint8_t *out, size_t outlen, shake_t *instance) { shake_do_squeeze(out, outlen, instance); }
void shake_squeeze_keep(uint8_t *out, size_t outlen, shake_t *instance) { shake_do_squeeze(out, outlen, instance); }
void shake_release(shake_t *instance) { (void)instance; }
void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) {
    if (pseudoXOF((unsigned long long)outlen * 8ULL, in, (unsigned long long)inlen * 8ULL, out) != 0) {
        memset(out, 0, outlen);
    }
}
void origami_pk_expand(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen) {
    if (pseudoXOF((unsigned long long)outlen * 8ULL, in, (unsigned long long)inlen * 8ULL, out) != 0) {
        memset(out, 0, outlen);
    }
}
