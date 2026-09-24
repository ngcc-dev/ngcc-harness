#include "bench_hash.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bench_core.h"
#include "drng.h"
#include "kat_parser.h"
#include "ngcc_log.h"

typedef struct {
    const ngcc_api_t *api;
    int digest_len_bits;
    unsigned char *msg;
    size_t msg_len;
    unsigned char *digest;
    size_t digest_len;
} hash_perf_ctx_t;

static int hash_run_once(const ngcc_api_t *api,
                         int digest_len_bits,
                         const unsigned char *msg,
                         size_t msg_len,
                         unsigned char *digest,
                         size_t digest_len) {
    if (api->CryptHash(digest_len_bits, msg, (unsigned long long) msg_len * 8ULL, digest) != 0) {
        return -1;
    }

    if (digest_len == 0) {
        return -1;
    }

    return 0;
}

static int hash_run_checked_bits(const ngcc_api_t *api,
                                 int digest_len_bits,
                                 const unsigned char *msg,
                                 unsigned long long msg_len_bits,
                                 unsigned char *digest,
                                 size_t digest_len) {
    if (api == NULL || msg == NULL || digest == NULL || digest_len == 0) {
        return -1;
    }

    memset(digest, 0xA5, digest_len);
    if (api->CryptHash(digest_len_bits, msg, msg_len_bits, digest) != 0) {
        return -1;
    }

    return 0;
}

static int hash_perf_op(void *ctx_ptr) {
    hash_perf_ctx_t *ctx = (hash_perf_ctx_t *) ctx_ptr;
    return hash_run_once(ctx->api,
                         ctx->digest_len_bits,
                         ctx->msg,
                         ctx->msg_len,
                         ctx->digest,
                         ctx->digest_len);
}

int ngcc_hash_correctness(const ngcc_api_t *api, int digest_len_bits, size_t msg_len) {
    unsigned char *msg = NULL;
    unsigned char *digest_a = NULL;
    unsigned char *digest_b = NULL;
    size_t digest_len;
    int rc = -1;

    if (api == NULL || digest_len_bits <= 0 || msg_len == 0 || msg_len > NGCC_MAX_BUFFER_LEN) {
        ngcc_log_error("[hash][correctness] invalid arguments: api_null=%d digest_len_bits=%d msg_len=%zu",
                       api == NULL,
                       digest_len_bits,
                       msg_len);
        return -1;
    }

    digest_len = (size_t) ((digest_len_bits + 7) / 8);
    if (digest_len == 0) {
        ngcc_log_error("[hash][correctness] invalid digest length: digest_len_bits=%d digest_len_bytes=%zu",
                       digest_len_bits,
                       digest_len);
        return -1;
    }

    msg = (unsigned char *) calloc(1, msg_len);
    digest_a = (unsigned char *) calloc(1, digest_len);
    digest_b = (unsigned char *) calloc(1, digest_len);
    if (msg == NULL || digest_a == NULL || digest_b == NULL) {
        ngcc_log_error("[hash][correctness] allocation failed: msg_len=%zu digest_len=%zu",
                       msg_len,
                       digest_len);
        goto out;
    }

    if (ngcc_fill_random(msg, msg_len) != 0) {
        ngcc_log_error("[hash][correctness] failed to fill random message: msg_len=%zu", msg_len);
        goto out;
    }

    memset(digest_a, 0xA5, digest_len);
    if (hash_run_once(api, digest_len_bits, msg, msg_len, digest_a, digest_len) != 0) {
        ngcc_log_error("[hash][correctness] first hash invocation failed: digest_len_bits=%d msg_len=%zu",
                       digest_len_bits,
                       msg_len);
        goto out;
    }

    memset(digest_b, 0x5A, digest_len);
    if (hash_run_once(api, digest_len_bits, msg, msg_len, digest_b, digest_len) != 0) {
        ngcc_log_error("[hash][correctness] second hash invocation failed: digest_len_bits=%d msg_len=%zu",
                       digest_len_bits,
                       msg_len);
        goto out;
    }

    if (memcmp(digest_a, digest_b, digest_len) != 0) {
        ngcc_log_error("[hash][correctness] digest mismatch across repeated runs: digest_len_bits=%d msg_len=%zu digest_len=%zu",
                       digest_len_bits,
                       msg_len,
                       digest_len);
        goto out;
    }

    rc = 0;

out:
    free(msg);
    free(digest_a);
    free(digest_b);
    return rc;
}

/* ── Helper: extract unsigned long long from 8-byte big-endian field ── */

static unsigned long long field_to_u64(const ngcc_kat_field_t *f) {
    unsigned long long v = 0;
    size_t i;

    if (f == NULL || f->data == NULL || f->len != 8) {
        return 0;
    }
    for (i = 0; i < 8; ++i) {
        v = (v << 8) | f->data[i];
    }
    return v;
}

/* ── Verify vectors from a single parsed KAT file ── */

static int verify_kat_vectors(const ngcc_api_t *api,
                              int digest_len_bits,
                              const ngcc_kat_file_t *kat,
                              unsigned long long *io_total,
                              unsigned long long *io_passed,
                              unsigned long long *io_failed) {

    size_t digest_len;
    unsigned char *digest = NULL;
    unsigned char *msg_buf = NULL;
    size_t i;
    int any_fail = 0;

    digest_len = (size_t) ((digest_len_bits + 7) / 8);
    if (digest_len == 0) {
        return -1;
    }

    digest = (unsigned char *) calloc(1, digest_len);
    if (digest == NULL) {
        return -1;
    }

    for (i = 0; i < kat->count; ++i) {
        const ngcc_kat_vector_t *vec = &kat->vectors[i];
        const ngcc_kat_field_t *input = ngcc_kat_get_field(vec, "Msg");
        const ngcc_kat_field_t *output = ngcc_kat_get_field(vec, "Dst");
        const ngcc_kat_field_t *msg_len_f = ngcc_kat_get_field(vec, "Msg_Len");
        const ngcc_kat_field_t *dst_len_f = ngcc_kat_get_field(vec, "Dst_Len");
        const ngcc_kat_field_t *msg_seed_f = ngcc_kat_get_field(vec, "Msg_Seed");
        unsigned long long msg_len_bits_val;
        const unsigned char *msg_data;
        size_t msg_data_bytes;

        /* Skip vectors without expected output (blank template) */
        if (output == NULL || output->data == NULL || output->len == 0) {
            continue;
        }

        /* Validate Dst_Len matches digest_len_bits */
        if (dst_len_f != NULL) {
            int dst_len_val = (int) field_to_u64(dst_len_f);
            if (dst_len_val != digest_len_bits) {
                ngcc_log_error("[hash][kat] Dst_Len mismatch: vector=%zu Dst_Len=%d digest_len_bits=%d",
                               i,
                               dst_len_val,
                               digest_len_bits);
                any_fail = 1;
                continue;
            }
        }

        /* Determine message length in bits */
        if (msg_len_f != NULL) {
            msg_len_bits_val = field_to_u64(msg_len_f);
        } else if (input != NULL) {
            msg_len_bits_val = (unsigned long long) input->len * 8ULL;
        } else {
            continue;
        }

        /* Determine message data */
        msg_data = NULL;
        if (msg_len_bits_val > (NGCC_MAX_BUFFER_LEN * 8ULL)) {
            ngcc_log_error("[hash][kat] Msg_Len exceeds maximum buffer size: vector=%zu Msg_Len=%llu max_bits=%llu",
                           i,
                           msg_len_bits_val,
                           NGCC_MAX_BUFFER_LEN * 8ULL);
            any_fail = 1;
            continue;
        }
        msg_data_bytes = (size_t) ((msg_len_bits_val + 7) / 8);

        if (msg_seed_f != NULL && msg_seed_f->data != NULL && msg_seed_f->len > 0) {
            /* DRNG seed-based message reconstruction (KAT_2_23, KAT_2_33) */
            DRNG_ctx drng;
            msg_buf = (unsigned char *) calloc(1, msg_data_bytes > 0 ? msg_data_bytes : 1);
            if (msg_buf == NULL) {
                ngcc_log_error("[hash][kat] failed to allocate DRNG message: vector=%zu msg_bytes=%zu Msg_Len=%llu bits",
                               i,
                               msg_data_bytes,
                               msg_len_bits_val);
                any_fail = 1;
                continue;
            }
            if (init_random_number(&drng, msg_seed_f->data, msg_seed_f->len) != 0) {
                ngcc_log_error("[hash][kat] failed to initialize DRNG: vector=%zu seed_len=%zu Msg_Len=%llu bits",
                               i,
                               msg_seed_f->len,
                               msg_len_bits_val);
                free(msg_buf);
                msg_buf = NULL;
                any_fail = 1;
                continue;
            }
            if (get_random_number(&drng, msg_buf, msg_len_bits_val) != 0) {
                ngcc_log_error("[hash][kat] failed to generate DRNG message: vector=%zu Msg_Len=%llu bits msg_bytes=%zu",
                               i,
                               msg_len_bits_val,
                               msg_data_bytes);
                free(msg_buf);
                msg_buf = NULL;
                any_fail = 1;
                continue;
            }
            msg_data = msg_buf;
        } else if (msg_seed_f != NULL && (msg_seed_f->data == NULL || msg_seed_f->len == 0)) {
            /* Empty seed: all-zero or all-FF message determined by Msg_Exp (skip for now,
               need to check Msg_Exp pattern).  For KAT_2_23/2_33 pattern:
               - If first Msg_Exp suggest all-0: msg is all zero
               - If second Msg_Exp suggest all-F: msg is all 0xFF
               These are identified by the position in the file (1st/2nd/3rd vector).
               For simplicity, check the Msg_Exp field. */
            const ngcc_kat_field_t *msg_exp = ngcc_kat_get_field(vec, "Msg_Exp");
            msg_buf = (unsigned char *) calloc(1, msg_data_bytes > 0 ? msg_data_bytes : 1);
            if (msg_buf == NULL) {
                ngcc_log_error("[hash][kat] failed to allocate synthetic message: vector=%zu msg_bytes=%zu Msg_Len=%llu bits",
                               i,
                               msg_data_bytes,
                               msg_len_bits_val);
                any_fail = 1;
                continue;
            }
            if (msg_exp != NULL && msg_exp->data != NULL && msg_exp->len >= 1 && msg_exp->data[0] == 0xFF) {
                memset(msg_buf, 0xFF, msg_data_bytes);
            }
            /* else: already all zeros from calloc */
            msg_data = msg_buf;
        } else if (input != NULL && input->data != NULL) {
            if (input->len < msg_data_bytes) {
                ngcc_log_error("[hash][kat] Msg field shorter than Msg_Len: vector=%zu Msg_Len=%llu bits required_bytes=%zu actual_bytes=%zu",
                               i,
                               msg_len_bits_val,
                               msg_data_bytes,
                               input->len);
                (*io_total)++;
                (*io_failed)++;
                any_fail = 1;
                continue;
            }
            msg_data = input->data;
        } else if (input != NULL && msg_len_bits_val == 0) {
            /* Empty message (Msg_Len = 0, Msg = "") */
            msg_data = (const unsigned char *) "";
        } else {
            continue;
        }

        (*io_total)++;
        if (output->len != digest_len) {
            ngcc_log_error("[hash][kat] Dst length mismatch: vector=%zu expected_bytes=%zu actual_bytes=%zu",
                           i,
                           digest_len,
                           output->len);
            (*io_failed)++;
            if (msg_buf != NULL) { free(msg_buf); msg_buf = NULL; }
            continue;
        }
        {
            int hash_rc = hash_run_checked_bits(api,
                                                digest_len_bits,
                                                msg_data,
                                                msg_len_bits_val,
                                                digest,
                                                digest_len);
            if (hash_rc != 0) {
                ngcc_log_error("[hash][kat] CryptHash failed: vector=%zu digest_len_bits=%d Msg_Len=%llu bits msg_bytes=%zu",
                               i,
                               digest_len_bits,
                               msg_len_bits_val,
                               msg_data_bytes);
                (*io_failed)++;
                any_fail = 1;
                if (msg_buf != NULL) { free(msg_buf); msg_buf = NULL; }
                continue;
            }
        }
        if (memcmp(digest, output->data, digest_len) != 0) {
            ngcc_log_error("[hash][kat] digest mismatch: vector=%zu digest_len_bits=%d Msg_Len=%llu bits digest_bytes=%zu",
                           i,
                           digest_len_bits,
                           msg_len_bits_val,
                           digest_len);
            (*io_failed)++;
            any_fail = 1;
            if (msg_buf != NULL) { free(msg_buf); msg_buf = NULL; }
            continue;
        }
        (*io_passed)++;
        if (msg_buf != NULL) { free(msg_buf); msg_buf = NULL; }
    }

    free(digest);
    return any_fail ? -1 : 0;
}

/* ── Check if path is a directory ── */

static int path_is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int path_is_regular_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

/* ── KAT_Loop verification ── */

#define KAT_LOOP_ITERATIONS    1000000

static int verify_kat_loop(const ngcc_api_t *api,
                           int digest_len_bits,
                           const ngcc_kat_file_t *kat,
                           unsigned long long *io_total,
                           unsigned long long *io_passed,
                           unsigned long long *io_failed) {
    const ngcc_kat_vector_t *vec;
    const ngcc_kat_field_t *input;
    const ngcc_kat_field_t *output;
    const ngcc_kat_field_t *msg_len_f;
    const ngcc_kat_field_t *dst_len_f;
    unsigned long long msg_len_bits_val;
    int dst_len_bits_val;
    size_t digest_len;
    size_t msg_bytes;
    unsigned char *msg = NULL;
    unsigned char *digest = NULL;
    unsigned char *buffer = NULL;
    int i;
    int rc = -1;

    if (kat->count < 1) {
        ngcc_log_error("[hash][kat_loop] KAT file has no vectors");
        return -1;
    }
    vec = &kat->vectors[0];
    input = ngcc_kat_get_field(vec, "Msg");
    output = ngcc_kat_get_field(vec, "Dst");
    msg_len_f = ngcc_kat_get_field(vec, "Msg_Len");
    dst_len_f = ngcc_kat_get_field(vec, "Dst_Len");

    if (input == NULL || input->data == NULL) {
        ngcc_log_error("[hash][kat_loop] Msg field missing");
        return -1;
    }
    if (output == NULL || output->data == NULL || output->len == 0) {
        ngcc_log_error("[hash][kat_loop] Dst field missing or empty");
        return -1;
    }
    if (msg_len_f == NULL) {
        ngcc_log_error("[hash][kat_loop] Msg_Len field missing");
        return -1;
    }
    if (dst_len_f == NULL) {
        ngcc_log_error("[hash][kat_loop] Dst_Len field missing");
        return -1;
    }

    msg_len_bits_val = field_to_u64(msg_len_f);
    dst_len_bits_val = (int) field_to_u64(dst_len_f);

    if (dst_len_bits_val != digest_len_bits) {
        ngcc_log_error("[hash][kat_loop] Dst_Len mismatch: Dst_Len=%d digest_len_bits=%d",
                       dst_len_bits_val,
                       digest_len_bits);
        return -1;
    }

    digest_len = (size_t) ((digest_len_bits + 7) / 8);
    msg_bytes = (size_t) ((msg_len_bits_val + 7) / 8);

    if (msg_bytes == 0 || msg_bytes > NGCC_MAX_BUFFER_LEN || digest_len == 0 || digest_len > msg_bytes) {
        ngcc_log_error("[hash][kat_loop] invalid lengths: Msg_Len=%llu bits Dst_Len=%d msg_bytes=%zu digest_bytes=%zu max_bytes=%llu",
                       msg_len_bits_val,
                       dst_len_bits_val,
                       msg_bytes,
                       digest_len,
                       NGCC_MAX_BUFFER_LEN);
        return -1;
    }

    msg = (unsigned char *) calloc(1, msg_bytes);
    digest = (unsigned char *) calloc(1, digest_len);
    buffer = (unsigned char *) calloc(1, digest_len);
    if (msg == NULL || digest == NULL || buffer == NULL) {
        ngcc_log_error("[hash][kat_loop] allocation failed: msg_bytes=%zu digest_bytes=%zu",
                       msg_bytes,
                       digest_len);
        goto out;
    }

    /* Copy initial message from KAT */
    memset(msg, 0, msg_bytes);
    memcpy(msg, input->data, input->len < msg_bytes ? input->len : msg_bytes);

    /* h_0 = CryptHash(m_0) */
    {
        int hash_rc = hash_run_checked_bits(api,
                                            digest_len_bits,
                                            msg,
                                            msg_len_bits_val,
                                            digest,
                                            digest_len);
        if (hash_rc != 0) {
            ngcc_log_error("[hash][kat_loop] initial CryptHash failed: digest_len_bits=%d Msg_Len=%llu bits",
                           digest_len_bits,
                           msg_len_bits_val);
            goto out;
        }
    }

    /* Loop i in [0:1,000,000):
       m_i = m_i <<< digest_len_bits  (circular left shift of message)
       m_(i+1) = m_i XOR (h_i || 000...000)
       h_(i+1) = CryptHash(m_(i+1))  */
    for (i = 0; i < KAT_LOOP_ITERATIONS; i++) {
        /* Circular left shift: move first digest_len bytes to temp,
           shift remainder left, put temp at end */
        memcpy(buffer, msg, digest_len);
        memmove(msg, msg + digest_len, msg_bytes - digest_len);
        memcpy(msg + msg_bytes - digest_len, buffer, digest_len);

        /* XOR digest into first digest_len bytes */
        {
            size_t j;
            for (j = 0; j < digest_len; j++) {
                msg[j] ^= digest[j];
            }
        }
        {
            int hash_rc = hash_run_checked_bits(api,
                                                digest_len_bits,
                                                msg,
                                                msg_len_bits_val,
                                                digest,
                                                digest_len);
            if (hash_rc != 0) {
                ngcc_log_error("[hash][kat_loop] CryptHash failed during loop: iteration=%d/%d digest_len_bits=%d Msg_Len=%llu bits",
                               i + 1,
                               KAT_LOOP_ITERATIONS,
                               digest_len_bits,
                               msg_len_bits_val);
                goto out;
            }
        }
    }

    (*io_total)++;
    if (output->len != digest_len || memcmp(digest, output->data, digest_len) != 0) {
        ngcc_log_error("[hash][kat_loop] digest mismatch: expected_bytes=%zu actual_bytes=%zu",
                       digest_len,
                       output->len);
        (*io_failed)++;
    } else {
        (*io_passed)++;
        rc = 0;
    }

out:
    free(msg);
    free(digest);
    free(buffer);
    return rc;
}

/* ── Classify file by prefix ── */

typedef enum {
    KAT_TYPE_2_12,
    KAT_TYPE_2_23,
    KAT_TYPE_2_33,
    KAT_TYPE_LOOP,
    KAT_TYPE_UNKNOWN
} kat_file_type_t;

static kat_file_type_t classify_kat_file(const char *filename) {
    if (strncmp(filename, "KAT_2_12_", 9) == 0) {
        return KAT_TYPE_2_12;
    }
    if (strncmp(filename, "KAT_2_23_", 9) == 0) {
        return KAT_TYPE_2_23;
    }
    if (strncmp(filename, "KAT_2_33_", 9) == 0) {
        return KAT_TYPE_2_33;
    }
    if (strncmp(filename, "KAT_Loop_", 9) == 0) {
        return KAT_TYPE_LOOP;
    }
    return KAT_TYPE_UNKNOWN;
}

static const char *kat_type_name(kat_file_type_t t) {
    switch (t) {
        case KAT_TYPE_2_12: return "KAT_2_12";
        case KAT_TYPE_2_23: return "KAT_2_23";
        case KAT_TYPE_2_33: return "KAT_2_33";
        case KAT_TYPE_LOOP: return "KAT_Loop";
        default:            return "unknown";
    }
}

static const char *path_basename_hash(const char *path) {
    const char *base;

    if (path == NULL) {
        return "";
    }
    base = strrchr(path, '/');
    return base != NULL ? base + 1 : path;
}

static int verify_hash_kat_one_file(const ngcc_api_t *api,
                                    int digest_len_bits,
                                    const char *file_path,
                                    kat_file_type_t ftype,
                                    unsigned long long *io_total,
                                    unsigned long long *io_passed,
                                    unsigned long long *io_failed) {
    ngcc_kat_file_t kat;
    int rc;

    memset(&kat, 0, sizeof(kat));
    if (ngcc_kat_parse_file(file_path, &kat) != 0) {
        ngcc_log_error("[hash][kat] failed to parse file: %s", file_path);
        return -2;
    }

    if (ftype == KAT_TYPE_LOOP) {
        rc = verify_kat_loop(api, digest_len_bits, &kat, io_total, io_passed, io_failed);
    } else {
        rc = verify_kat_vectors(api, digest_len_bits, &kat, io_total, io_passed, io_failed);
    }

    ngcc_kat_free(&kat);
    return rc;
}

int ngcc_hash_correctness_kat_file(const ngcc_api_t *api,
                                   int digest_len_bits,
                                   const char *kat_path,
                                   unsigned long long *out_total,
                                   unsigned long long *out_passed,
                                   unsigned long long *out_failed) {
    DIR *dir;
    struct dirent *entry;
    unsigned long long total = 0;
    unsigned long long passed_count = 0;
    unsigned long long failed_count = 0;
    int file_count = 0;
    int found_types[4] = {0, 0, 0, 0};  /* KAT_2_12, KAT_2_23, KAT_2_33, KAT_Loop */
    int any_file_failed = 0;
    int rc = -1;

    if (api == NULL || digest_len_bits <= 0 || kat_path == NULL) {
        ngcc_log_error("[hash][kat] invalid arguments: api_null=%d digest_len_bits=%d kat_path_null=%d",
                       api == NULL,
                       digest_len_bits,
                       kat_path == NULL);
        return -1;
    }

    if (path_is_regular_file(kat_path)) {
        kat_file_type_t ftype = classify_kat_file(path_basename_hash(kat_path));

        file_count = 1;
        int vrc = verify_hash_kat_one_file(api,
                                           digest_len_bits,
                                           kat_path,
                                           ftype,
                                           &total,
                                           &passed_count,
                                           &failed_count);

        rc = (vrc == 0 && total > 0 && failed_count == 0) ? 0 : -1;
        if (rc != 0) {
            ngcc_log_error("[hash][kat] verification failed: total=%llu passed=%llu failed=%llu file=%s",
                           total,
                           passed_count,
                           failed_count,
                           kat_path);
        }
        goto done;
    }

    if (!path_is_directory(kat_path)) {
        ngcc_log_error("[hash][kat] --kat path is not a file or directory: %s", kat_path);
        return -1;
    }

    dir = opendir(kat_path);
    if (dir == NULL) {
        ngcc_log_error("[hash][kat] cannot open directory: %s", kat_path);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char file_path[2048];
        int len;
        int vrc;
        kat_file_type_t ftype;
        unsigned long long before_total;
        unsigned long long before_passed;
        unsigned long long before_failed;

        if (entry->d_name[0] == '.') {
            continue;
        }

        len = snprintf(file_path, sizeof(file_path), "%s/%s", kat_path, entry->d_name);
        if (len < 0 || len >= (int) sizeof(file_path)) {
            ngcc_log_error("[hash][kat] KAT file path too long: dir=%s file=%s", kat_path, entry->d_name);
            continue;
        }
        if (path_is_directory(file_path)) {
            continue;
        }

        ftype = classify_kat_file(entry->d_name);
        if (ftype == KAT_TYPE_UNKNOWN) {
            continue;  /* skip files not matching any known KAT prefix */
        }

        before_total = total;
        before_passed = passed_count;
        before_failed = failed_count;

        vrc = verify_hash_kat_one_file(api,
                                       digest_len_bits,
                                       file_path,
                                       ftype,
                                       &total,
                                       &passed_count,
                                       &failed_count);
        if (vrc == -2) {
            closedir(dir);
            rc = -1;
            goto done;
        }

        file_count++;
        if (vrc != 0) {
            any_file_failed = 1;
            ngcc_log_error("[hash][kat] file verification failed: file=%s type=%s rc=%d total_delta=%llu passed_delta=%llu failed_delta=%llu",
                           file_path,
                           kat_type_name(ftype),
                           vrc,
                           total - before_total,
                           passed_count - before_passed,
                           failed_count - before_failed);
            continue;
        }
        if (ftype < 4 && passed_count > before_passed) {
            found_types[ftype] = 1;
        }
    }
    closedir(dir);

    if (file_count == 0) {
        ngcc_log_error("[hash][kat] no KAT files found in directory: %s", kat_path);
        rc = -1;
        goto done;
    }

    /* Verify all required KAT file types were found */
    {
        static const kat_file_type_t required[] = {KAT_TYPE_2_12, KAT_TYPE_2_23, KAT_TYPE_2_33, KAT_TYPE_LOOP};
        size_t ri;
        for (ri = 0; ri < sizeof(required) / sizeof(required[0]); ++ri) {
            if (!found_types[required[ri]]) {
                ngcc_log_error("[hash][kat] missing required KAT type: %s", kat_type_name(required[ri]));
                rc = -1;
                goto done;
            }
        }
    }

    rc = (total > 0 && failed_count == 0 && !any_file_failed) ? 0 : -1;
    if (rc != 0) {
        ngcc_log_error("[hash][kat] verification failed: total=%llu passed=%llu failed=%llu dir=%s",
                       total,
                       passed_count,
                       failed_count,
                       kat_path);
    }

done:
    if (out_total != NULL) {
        *out_total = total;
    }
    if (out_passed != NULL) {
        *out_passed = passed_count;
    }
    if (out_failed != NULL) {
        *out_failed = failed_count;
    }
    return rc;
}

int ngcc_hash_performance(const ngcc_api_t *api,
                          int digest_len_bits,
                          size_t msg_len,
                          const ngcc_perf_config_t *cfg,
                          ngcc_perf_result_t *out_result) {
    hash_perf_ctx_t ctx;
    ngcc_perf_config_t local_cfg;

    if (api == NULL || cfg == NULL || out_result == NULL || digest_len_bits <= 0 || msg_len == 0 ||
        msg_len > NGCC_MAX_BUFFER_LEN) {
        ngcc_log_error("[hash][performance] invalid arguments: api_null=%d cfg_null=%d out_null=%d digest_len_bits=%d msg_len=%zu",
                       api == NULL,
                       cfg == NULL,
                       out_result == NULL,
                       digest_len_bits,
                       msg_len);
        return -1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.api = api;
    ctx.digest_len_bits = digest_len_bits;
    ctx.msg_len = msg_len;
    ctx.digest_len = (size_t) ((digest_len_bits + 7) / 8);
    if (ctx.digest_len == 0) {
        ngcc_log_error("[hash][performance] invalid digest length: digest_len_bits=%d digest_len_bytes=%zu",
                       digest_len_bits,
                       ctx.digest_len);
        return -1;
    }

    ctx.msg = (unsigned char *) calloc(1, ctx.msg_len);
    ctx.digest = (unsigned char *) calloc(1, ctx.digest_len);
    if (ctx.msg == NULL || ctx.digest == NULL) {
        ngcc_log_error("[hash][performance] allocation failed: msg_len=%zu digest_len=%zu",
                       ctx.msg_len,
                       ctx.digest_len);
        free(ctx.msg);
        free(ctx.digest);
        return -1;
    }

    if (ngcc_fill_random(ctx.msg, ctx.msg_len) != 0) {
        ngcc_log_error("[hash][performance] failed to fill random message: msg_len=%zu", ctx.msg_len);
        free(ctx.msg);
        free(ctx.digest);
        return -1;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = (unsigned long long) msg_len;
    if (ngcc_run_performance_op(&local_cfg, hash_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[hash][performance] benchmark failed: digest_len_bits=%d msg_len=%zu iterations=%llu",
                       digest_len_bits,
                       msg_len,
                       cfg->iterations);
        free(ctx.msg);
        free(ctx.digest);
        return -1;
    }

    free(ctx.msg);
    free(ctx.digest);
    return 0;
}
