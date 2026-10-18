#define _POSIX_C_SOURCE 200809L
#include "nist_sig_api.h"
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GUARD 64
#define MAX_BYTES (512u * 1024u * 1024u)
typedef struct { uint8_t *base, *data; size_t size; } buffer;
typedef struct {
    const nist_sig_api *(*meta)(void);
    int (*seed)(const uint8_t *);
    int (*keypair)(uint8_t *, uint8_t *);
    int (*sign)(uint8_t *, uint64_t *, const uint8_t *, uint64_t, const uint8_t *);
    int (*open)(uint8_t *, uint64_t *, const uint8_t *, uint64_t, const uint8_t *);
} functions;

static void json_string(const char *s) {
    putchar('"');
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') printf("\\%c", c);
        else if (c < 32) printf("\\u%04x", c);
        else putchar(c);
    }
    putchar('"');
}

static int number(const char *s, uint64_t *out) {
    if (!*s) return 0;
    for (const char *p = s; *p; ++p) if (*p < '0' || *p > '9') return 0;
    errno = 0;
    char *end;
    *out = strtoull(s, &end, 10);
    return !errno && !*end;
}
static int nybble(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static int decode(const char *s, uint8_t *out, size_t size) {
    if (strlen(s) != size * 2) return 0;
    for (size_t i = 0; i < size; ++i) {
        int a = nybble((unsigned char)s[2 * i]), b = nybble((unsigned char)s[2 * i + 1]);
        if (a < 0 || b < 0) return 0;
        if (out) out[i] = (uint8_t)((a << 4) | b);
    }
    return 1;
}
static int digest_equal(const uint8_t *data, size_t size, const uint8_t expected[32]) {
    uint8_t digest[EVP_MAX_MD_SIZE];
    unsigned length = 0;
    return EVP_Digest(data, size, digest, &length, EVP_sha256(), NULL) == 1 &&
           length == 32 && !memcmp(digest, expected, 32);
}
static int allocate(buffer *b, uint64_t size) {
    if (size > MAX_BYTES) return 0;
    b->size = (size_t)size;
    b->base = malloc(b->size + 2 * GUARD);
    if (!b->base) return 0;
    memset(b->base, 0xa5, b->size + 2 * GUARD);
    b->data = b->base + GUARD;
    memset(b->data, 0, b->size);
    return 1;
}
static int intact(const buffer *b) {
    for (size_t i = 0; i < GUARD; ++i)
        if (b->base[i] != 0xa5 || b->data[b->size + i] != 0xa5) return 0;
    return 1;
}
static int all_intact(buffer *buffers, size_t count) {
    for (size_t i = 0; i < count; ++i) if (!intact(&buffers[i])) return 0;
    return 1;
}

static int columns(char *line, char **row) {
    char *cursor = line;
    size_t fields = 0;
    for (;;) {
        if (fields == 8) return 0;
        row[fields++] = cursor;
        char *tab = strchr(cursor, '\t');
        if (!tab) break;
        *tab = 0; cursor = tab + 1;
    }
    return fields == 8;
}

/* Scan the whole manifest before running any cryptography. In particular,
 * a truncated manifest cannot turn a full replay into a smaller PASS. */
static const char *preflight(FILE *manifest, const nist_sig_api *meta, uint64_t limit) {
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    uint64_t available = 0, declared = 0;
    int magic_seen = 0, count_seen = 0;
    const char *error = NULL;
    while ((length = getline(&line, &capacity, manifest)) >= 0) {
        while (length && (line[length - 1] == '\n' || line[length - 1] == '\r')) line[--length] = 0;
        if (!length) continue;
        if (line[0] == '#') {
            if (!strcmp(line, "# nist-sig-kat-v1")) magic_seen = 1;
            const char *prefix = "# records = ";
            if (!strncmp(line, prefix, strlen(prefix))) {
                if (count_seen || !number(line + strlen(prefix), &declared)) {
                    error = "invalid_manifest_record_count"; break;
                }
                count_seen = 1;
            }
            prefix = "# public_key_bytes = ";
            if (!strncmp(line, prefix, strlen(prefix))) {
                uint64_t size;
                if (!number(line + strlen(prefix), &size) || size != meta->public_key_bytes) {
                    error = "manifest_public_key_size_mismatch"; break;
                }
            }
            prefix = "# secret_key_bytes = ";
            if (!strncmp(line, prefix, strlen(prefix))) {
                uint64_t size;
                if (!number(line + strlen(prefix), &size) || size != meta->secret_key_bytes) {
                    error = "manifest_secret_key_size_mismatch"; break;
                }
            }
            continue;
        }
        if (!magic_seen) { error = "missing_manifest_version"; break; }
        if (count_seen) { error = "manifest_record_after_footer"; break; }
        char *row[8];
        uint64_t count, mlen, smlen;
        if (!columns(line, row)) { error = "invalid_manifest_columns"; break; }
        if (!number(row[0], &count) || !decode(row[1], NULL, 48) ||
            !number(row[2], &mlen) || mlen > MAX_BYTES ||
            meta->signature_bytes > MAX_BYTES - mlen ||
            !decode(row[3], NULL, (size_t)mlen) ||
            !decode(row[4], NULL, 32) || !decode(row[5], NULL, 32) ||
            !number(row[6], &smlen) || smlen < mlen ||
            smlen > mlen + meta->signature_bytes || !decode(row[7], NULL, 32)) {
            error = "invalid_manifest_record"; break;
        }
        ++available;
    }
    if (!error && ferror(manifest)) error = "manifest_read_error";
    if (!error && !magic_seen) error = "missing_manifest_version";
    if (!error && !count_seen) error = "missing_manifest_record_count";
    if (!error && available != declared) error = "manifest_record_count_mismatch";
    if (!error && !available) error = "empty_manifest";
    if (!error && limit != UINT64_MAX && limit > available) error = "requested_limit_exceeds_manifest";
    if (!error && fseek(manifest, 0, SEEK_SET)) error = "manifest_seek_error";
    free(line);
    return error;
}

static const char *replay(functions *f, const nist_sig_api *meta, char **row) {
    uint64_t count, mlen, expected_smlen;
    uint8_t seed[48], pkhash[32], skhash[32], smhash[32];
    if (!number(row[0], &count) || !decode(row[1], seed, sizeof seed) ||
        !number(row[2], &mlen) || !number(row[6], &expected_smlen) ||
        !decode(row[4], pkhash, 32) || !decode(row[5], skhash, 32) ||
        !decode(row[7], smhash, 32) || mlen > MAX_BYTES ||
        meta->signature_bytes > MAX_BYTES - mlen || expected_smlen < mlen ||
        expected_smlen > mlen + meta->signature_bytes)
        return "invalid_manifest_record";
    (void)count;
    /* open() may write smlen bytes before removing the signature. */
    buffer buffers[5] = {{0}};
    uint64_t sizes[5] = {meta->public_key_bytes, meta->secret_key_bytes,
                        mlen, mlen + meta->signature_bytes,
                        mlen + meta->signature_bytes};
    const char *error = NULL;
    for (size_t i = 0; i < 5; ++i)
        if (!allocate(&buffers[i], sizes[i])) { error = "allocation_failed"; goto done; }
    uint8_t *pk = buffers[0].data, *sk = buffers[1].data;
    uint8_t *message = buffers[2].data, *sm = buffers[3].data, *opened = buffers[4].data;
    uint64_t smlen = 0, opened_len = 0;
    if (!decode(row[3], message, (size_t)mlen)) { error = "invalid_message_hex"; goto done; }
    if (f->seed(seed)) { error = "seed_failed"; goto done; }
    if (f->keypair(pk, sk)) { error = "keypair_failed"; goto done; }
    if (!all_intact(buffers, 5)) { error = "keypair_buffer_overflow"; goto done; }
    if (!digest_equal(pk, buffers[0].size, pkhash)) { error = "public_key_mismatch"; goto done; }
    if (!digest_equal(sk, buffers[1].size, skhash)) { error = "secret_key_mismatch"; goto done; }
    if (f->sign(sm, &smlen, message, mlen, sk)) { error = "sign_failed"; goto done; }
    if (!all_intact(buffers, 5)) { error = "sign_buffer_overflow"; goto done; }
    if (smlen != expected_smlen) { error = "signed_message_length_mismatch"; goto done; }
    if (!digest_equal(sm, (size_t)smlen, smhash)) { error = "signed_message_mismatch"; goto done; }
    if (f->open(opened, &opened_len, sm, smlen, pk)) { error = "valid_signature_rejected"; goto done; }
    if (!all_intact(buffers, 5)) { error = "open_buffer_overflow"; goto done; }
    if (opened_len != mlen || memcmp(opened, message, (size_t)mlen)) {
        error = "recovered_message_mismatch"; goto done;
    }
    /* No assumption about signature-prefix versus signature-suffix layout. */
    if (smlen) {
        sm[0] ^= 1;
        int rc = f->open(opened, &opened_len, sm, smlen, pk);
        sm[0] ^= 1;
        if (!all_intact(buffers, 5)) { error = "negative_open_buffer_overflow"; goto done; }
        if (!rc) { error = "first_byte_mutation_accepted"; goto done; }
        sm[smlen - 1] ^= 1;
        rc = f->open(opened, &opened_len, sm, smlen, pk);
        sm[smlen - 1] ^= 1;
        if (!all_intact(buffers, 5)) { error = "negative_open_buffer_overflow"; goto done; }
        if (!rc) { error = "last_byte_mutation_accepted"; goto done; }
    }
done:
    for (size_t i = 0; i < 5; ++i) {
        if (buffers[i].base) {
            /* Candidate secret keys are deterministic public test material. */
            memset(buffers[i].base, 0, buffers[i].size + 2 * GUARD);
            free(buffers[i].base);
        }
    }
    return error;
}

static void usage(void) {
    fputs("usage: nist-sig info LIBRARY\n"
          "       nist-sig kat LIBRARY MANIFEST [LIMIT]\n", stderr);
}
int main(int argc, char **argv) {
    if (argc < 3 || (strcmp(argv[1], "info") && strcmp(argv[1], "kat"))) {
        usage(); return 2;
    }
    int info = !strcmp(argv[1], "info");
    uint64_t limit = UINT64_MAX;
    if ((info && argc != 3) || (!info && (argc < 4 || argc > 5)) ||
        (argc == 5 && (!number(argv[4], &limit) || !limit))) {
        usage(); return 2;
    }
    /* dlopen only treats paths containing / as relative to cwd. */
    char *path = NULL;
    if (strchr(argv[2], '/')) path = strdup(argv[2]);
    else {
        path = malloc(strlen(argv[2]) + 3);
        if (path) sprintf(path, "./%s", argv[2]);
    }
    if (!path) { fputs("out of memory\n", stderr); return 2; }
    void *library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    free(path);
    if (!library) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }
    functions f = {0};
#define LOAD(member, symbol) do { \
    void *address = dlsym(library, symbol); \
    if (!address) { fprintf(stderr, "missing symbol: %s\n", symbol); dlclose(library); return 2; } \
    _Static_assert(sizeof f.member == sizeof address, "POSIX function pointer size"); \
    memcpy(&f.member, &address, sizeof address); \
} while (0)
    LOAD(meta, "nist_sig_get_api"); LOAD(seed, "nist_sig_seed");
    LOAD(keypair, "nist_sig_keypair"); LOAD(sign, "nist_sig_sign");
    LOAD(open, "nist_sig_open");
#undef LOAD
    const nist_sig_api *meta = f.meta();
    if (!meta || meta->abi_version != NIST_SIG_ABI_VERSION || !meta->scheme ||
        !meta->parameter || !meta->implementation || !meta->public_key_bytes ||
        !meta->secret_key_bytes || !meta->signature_bytes ||
        meta->public_key_bytes > MAX_BYTES || meta->secret_key_bytes > MAX_BYTES ||
        meta->signature_bytes > MAX_BYTES) {
        fputs("invalid library metadata\n", stderr); dlclose(library); return 2;
    }
    printf("{\"scheme\":"); json_string(meta->scheme);
    printf(",\"parameter\":"); json_string(meta->parameter);
    printf(",\"implementation\":"); json_string(meta->implementation);
    printf(",\"abi\":%u,\"public_key_bytes\":%" PRIu64
           ",\"secret_key_bytes\":%" PRIu64 ",\"signature_bytes\":%" PRIu64,
           meta->abi_version, meta->public_key_bytes, meta->secret_key_bytes,
           meta->signature_bytes);
    if (info) { puts("}"); dlclose(library); return 0; }
    FILE *manifest = fopen(argv[3], "r");
    const char *error = manifest ? NULL : "cannot_open_manifest";
    char *line = NULL;
    size_t capacity = 0;
    uint64_t records = 0;
    char failed_count[32] = "";
    int magic_seen = 0;
    if (manifest) error = preflight(manifest, meta, limit);
    if (manifest && !error) {
        ssize_t length;
        while (records < limit && (length = getline(&line, &capacity, manifest)) >= 0) {
            while (length && (line[length - 1] == '\n' || line[length - 1] == '\r')) line[--length] = 0;
            if (!length) continue;
            if (line[0] == '#') {
                if (!strcmp(line, "# nist-sig-kat-v1")) magic_seen = 1;
                continue;
            }
            if (!magic_seen) { error = "missing_manifest_version"; break; }
            char *row[8];
            if (!columns(line, row)) { error = "invalid_manifest_columns"; break; }
            snprintf(failed_count, sizeof failed_count, "%s", row[0]);
            error = replay(&f, meta, row);
            if (error) break;
            ++records;
            /* Keep progress off stdout, which remains one JSON object. */
            fprintf(stderr, "%s %s count=%s PASS\n", meta->scheme, meta->parameter, row[0]);
            failed_count[0] = 0;
        }
        if (ferror(manifest)) error = "manifest_read_error";
    }
    if (manifest) fclose(manifest);
    free(line);
    if (!error && !records) error = "empty_manifest";
    printf(",\"status\":\"%s\",\"records\":%" PRIu64, error ? "FAIL" : "PASS", records);
    if (error) {
        printf(",\"error\":"); json_string(error);
        if (*failed_count) { printf(",\"count\":"); json_string(failed_count); }
    }
    puts("}");
    printf("RESULT\t%s\t%s\t%s\t%" PRIu64 "\t%s\n", meta->scheme,
           meta->parameter, error ? "FAIL" : "PASS", records, error ? error : "ok");
    dlclose(library);
    return error ? 1 : 0;
}
