#include "bench_sig.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bench_core.h"
#include "kat_parser.h"
#include "ngcc_log.h"

typedef struct {
    const ngcc_api_t *api;
    unsigned char *pk;
    unsigned char *sk;
    unsigned char *sn;
    unsigned char *msg;
    unsigned long long pk_cap;
    unsigned long long sk_cap;
    unsigned long long sn_cap;
    unsigned long long pk_len;
    unsigned long long sk_len;
    unsigned long long sn_len;
    unsigned long long msg_len;
} sig_ctx_t;

static int validate_sig_caps(unsigned long long pk_cap,
                             unsigned long long sk_cap,
                             unsigned long long sn_cap) {
    return ngcc_is_valid_len(pk_cap) &&
           ngcc_is_valid_len(sk_cap) &&
           ngcc_is_valid_len(sn_cap);
}

static int sig_keygen_once(const ngcc_api_t *api,
                           unsigned char *pk,
                           unsigned long long pk_cap,
                           unsigned long long *pk_len,
                           unsigned char *sk,
                           unsigned long long sk_cap,
                           unsigned long long *sk_len) {
    if (api->sig_keygen(pk, pk_len, sk, sk_len) != 0) {
        ngcc_log_error("[sig] sig_keygen failed");
        return -1;
    }
    if (*pk_len == 0 || *pk_len > pk_cap || *sk_len == 0 || *sk_len > sk_cap) {
        ngcc_log_error("[sig] sig_keygen returned invalid lengths: pk_len=%llu pk_cap=%llu sk_len=%llu sk_cap=%llu",
                       *pk_len,
                       pk_cap,
                       *sk_len,
                       sk_cap);
        return -1;
    }
    return 0;
}

static int sig_sign_once(const ngcc_api_t *api,
                         unsigned char *sk,
                         unsigned long long sk_len,
                         unsigned char *msg,
                         unsigned long long msg_len,
                         unsigned char *sn,
                         unsigned long long sn_cap,
                         unsigned long long *sn_len) {
    if (api->sig_sign(sk, sk_len, msg, msg_len, sn, sn_len) != 0) {
        ngcc_log_error("[sig] sig_sign failed: sk_len=%llu msg_len=%llu sn_cap=%llu",
                       sk_len,
                       msg_len,
                       sn_cap);
        return -1;
    }
    if (*sn_len == 0 || *sn_len > sn_cap) {
        ngcc_log_error("[sig] sig_sign returned invalid signature length: sn_len=%llu sn_cap=%llu",
                       *sn_len,
                       sn_cap);
        return -1;
    }
    return 0;
}

static int sig_verify_once(const ngcc_api_t *api,
                           unsigned char *pk,
                           unsigned long long pk_len,
                           unsigned char *sn,
                           unsigned long long sn_len,
                           unsigned char *msg,
                           unsigned long long msg_len) {
    if (api->sig_verify(pk, pk_len, sn, sn_len, msg, msg_len) != 0) {
        ngcc_log_error("[sig] sig_verify failed: pk_len=%llu sn_len=%llu msg_len=%llu",
                       pk_len,
                       sn_len,
                       msg_len);
        return -1;
    }
    return 0;
}

static int sig_prepare_sign_case(sig_ctx_t *ctx) {
    if (ngcc_fill_random(ctx->msg, (size_t) ctx->msg_len) != 0) {
        return -1;
    }

    ctx->pk_len = ctx->pk_cap;
    ctx->sk_len = ctx->sk_cap;
    if (sig_keygen_once(ctx->api,
                        ctx->pk,
                        ctx->pk_cap,
                        &ctx->pk_len,
                        ctx->sk,
                        ctx->sk_cap,
                        &ctx->sk_len) != 0) {
        return -1;
    }

    ctx->sn_len = ctx->sn_cap;
    if (sig_sign_once(ctx->api,
                      ctx->sk,
                      ctx->sk_len,
                      ctx->msg,
                      ctx->msg_len,
                      ctx->sn,
                      ctx->sn_cap,
                      &ctx->sn_len) != 0) {
        return -1;
    }

    return 0;
}

static int alloc_sig_ctx(const ngcc_api_t *api, size_t msg_len, sig_ctx_t *out_ctx) {
    if (api == NULL || out_ctx == NULL || msg_len == 0 || msg_len > NGCC_MAX_BUFFER_LEN) {
        ngcc_log_error("[sig] invalid context allocation arguments: api_null=%d out_null=%d msg_len=%zu",
                       api == NULL,
                       out_ctx == NULL,
                       msg_len);
        return -1;
    }

    memset(out_ctx, 0, sizeof(*out_ctx));
    out_ctx->api = api;
    out_ctx->pk_cap = api->sig_get_pk_len_bytes();
    out_ctx->sk_cap = api->sig_get_sk_len_bytes();
    out_ctx->sn_cap = api->sig_get_sn_len_bytes();
    out_ctx->msg_len = (unsigned long long) msg_len;

    if (!validate_sig_caps(out_ctx->pk_cap, out_ctx->sk_cap, out_ctx->sn_cap)) {
        ngcc_log_error("[sig] invalid advertised caps: pk_cap=%llu sk_cap=%llu sn_cap=%llu",
                       out_ctx->pk_cap,
                       out_ctx->sk_cap,
                       out_ctx->sn_cap);
        return -1;
    }

    out_ctx->pk = (unsigned char *) calloc(1, (size_t) out_ctx->pk_cap);
    out_ctx->sk = (unsigned char *) calloc(1, (size_t) out_ctx->sk_cap);
    out_ctx->sn = (unsigned char *) calloc(1, (size_t) out_ctx->sn_cap);
    out_ctx->msg = (unsigned char *) calloc(1, msg_len);
    if (out_ctx->pk == NULL || out_ctx->sk == NULL || out_ctx->sn == NULL || out_ctx->msg == NULL) {
        ngcc_log_error("[sig] allocation failed: pk_cap=%llu sk_cap=%llu sn_cap=%llu msg_len=%zu",
                       out_ctx->pk_cap,
                       out_ctx->sk_cap,
                       out_ctx->sn_cap,
                       msg_len);
        free(out_ctx->pk);
        free(out_ctx->sk);
        free(out_ctx->sn);
        free(out_ctx->msg);
        memset(out_ctx, 0, sizeof(*out_ctx));
        return -1;
    }

    return 0;
}

static void free_sig_ctx(sig_ctx_t *ctx) {
    if (ctx == NULL) {
        return;
    }
    free(ctx->pk);
    free(ctx->sk);
    free(ctx->sn);
    free(ctx->msg);
    memset(ctx, 0, sizeof(*ctx));
}

int ngcc_sig_correctness(const ngcc_api_t *api, size_t msg_len) {
    sig_ctx_t ctx;
    int rc = -1;

    if (alloc_sig_ctx(api, msg_len, &ctx) != 0) {
        return -1;
    }

    if (sig_prepare_sign_case(&ctx) != 0) {
        ngcc_log_error("[sig][correctness] failed to prepare sign/verify case: msg_len=%zu", msg_len);
        goto out;
    }

    if (sig_verify_once(api, ctx.pk, ctx.pk_len, ctx.sn, ctx.sn_len, ctx.msg, ctx.msg_len) != 0) {
        ngcc_log_error("[sig][correctness] verification failed after signing: msg_len=%zu pk_len=%llu sn_len=%llu",
                       msg_len,
                       ctx.pk_len,
                       ctx.sn_len);
        goto out;
    }

    rc = 0;

out:
    free_sig_ctx(&ctx);
    return rc;
}

static unsigned long long sig_field_to_u64(const ngcc_kat_field_t *f) {
    unsigned long long v = 0;
    size_t i;
    if (f == NULL || f->data == NULL || f->len != 8) {
        return 0;
    }
    for (i = 0; i < 8; i++) {
        v = (v << 8) | f->data[i];
    }
    return v;
}

static int sig_check_field_len(const char *field_name, const ngcc_kat_field_t *data_field,
                               const ngcc_kat_field_t *len_field) {
    unsigned long long expected;
    if (len_field == NULL || data_field == NULL) {
        return 0;  /* no len field to validate */
    }
    expected = sig_field_to_u64(len_field);
    if (expected == 0) {
        return 0;  /* empty len value, skip */
    }
    if ((unsigned long long) data_field->len != expected) {
        ngcc_log_error("[sig][kat] %s length mismatch: expected_bytes=%llu actual_bytes=%zu",
                       field_name,
                       expected,
                       data_field->len);
        return -1;
    }
    return 0;
}

static int path_is_directory_sig(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int path_is_regular_file_sig(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

static int verify_sig_kat_vectors(const ngcc_api_t *api,
                                  const ngcc_kat_file_t *kat,
                                  unsigned long long *io_total,
                                  unsigned long long *io_passed,
                                  unsigned long long *io_failed) {
    unsigned long long pk_cap;
    unsigned long long sn_cap;
    size_t i;

    pk_cap = api->sig_get_pk_len_bytes();
    sn_cap = api->sig_get_sn_len_bytes();
    if (!ngcc_is_valid_len(pk_cap) || !ngcc_is_valid_len(sn_cap)) {
        ngcc_log_error("[sig][kat] invalid advertised caps: pk_cap=%llu sn_cap=%llu",
                       pk_cap,
                       sn_cap);
        return -1;
    }

    for (i = 0; i < kat->count; ++i) {
        const ngcc_kat_vector_t *vec = &kat->vectors[i];
        const ngcc_kat_field_t *pk = ngcc_kat_get_field(vec, "PK");
        const ngcc_kat_field_t *msg = ngcc_kat_get_field(vec, "M");
        const ngcc_kat_field_t *sn = ngcc_kat_get_field(vec, "Sn");

        /* Skip vectors with empty output (blank template) */
        if (pk == NULL || pk->data == NULL || pk->len == 0) {
            continue;
        }
        if (sn == NULL || sn->data == NULL || sn->len == 0) {
            continue;
        }
        if (msg == NULL || msg->data == NULL) {
            continue;
        }

        (*io_total)++;

        /* Validate _Len fields match actual data length */
        if (sig_check_field_len("PK", pk, ngcc_kat_get_field(vec, "PK_Len")) != 0 ||
            sig_check_field_len("M", msg, ngcc_kat_get_field(vec, "M_Len")) != 0 ||
            sig_check_field_len("Sn", sn, ngcc_kat_get_field(vec, "Sn_Len")) != 0) {
            (*io_failed)++;
            continue;
        }
        if (pk->len > pk_cap || sn->len > sn_cap || msg->len > NGCC_MAX_BUFFER_LEN) {
            ngcc_log_error("[sig][kat] vector exceeds caps: vector=%zu pk_len=%zu pk_cap=%llu sn_len=%zu sn_cap=%llu msg_len=%zu max_msg=%llu",
                           i,
                           pk->len,
                           pk_cap,
                           sn->len,
                           sn_cap,
                           msg->len,
                           NGCC_MAX_BUFFER_LEN);
            (*io_failed)++;
            continue;
        }
        if (sig_verify_once(api,
                            (unsigned char *) pk->data,
                            (unsigned long long) pk->len,
                            (unsigned char *) sn->data,
                            (unsigned long long) sn->len,
                            (unsigned char *) msg->data,
                            (unsigned long long) msg->len) != 0) {
            (*io_failed)++;
            continue;
        }
        (*io_passed)++;
    }

    return 0;
}

static int verify_sig_kat_one_file(const ngcc_api_t *api,
                                   const char *file_path,
                                   unsigned long long *io_total,
                                   unsigned long long *io_passed,
                                   unsigned long long *io_failed) {
    ngcc_kat_file_t kat;
    int rc;

    memset(&kat, 0, sizeof(kat));
    if (ngcc_kat_parse_file(file_path, &kat) != 0) {
        ngcc_log_error("[sig][kat] failed to parse file: %s", file_path);
        return -2;
    }

    rc = verify_sig_kat_vectors(api, &kat, io_total, io_passed, io_failed);
    ngcc_kat_free(&kat);
    return rc;
}

int ngcc_sig_verify_correctness_kat_file(const ngcc_api_t *api,
                                         const char *kat_path,
                                         unsigned long long *out_total,
                                         unsigned long long *out_passed,
                                         unsigned long long *out_failed) {
    DIR *dir;
    struct dirent *entry;
    unsigned long long total = 0;
    unsigned long long passed = 0;
    unsigned long long failed = 0;
    int file_count = 0;
    int rc = -1;

    if (api == NULL || kat_path == NULL) {
        ngcc_log_error("[sig][kat] invalid arguments: api_null=%d kat_path_null=%d",
                       api == NULL,
                       kat_path == NULL);
        return -1;
    }

    if (path_is_regular_file_sig(kat_path)) {
        int vrc = verify_sig_kat_one_file(api, kat_path, &total, &passed, &failed);

        file_count = 1;
        rc = (vrc == 0 && total > 0 && failed == 0) ? 0 : -1;
        if (rc != 0) {
            ngcc_log_error("[sig][kat] verification failed: total=%llu passed=%llu failed=%llu file=%s",
                           total,
                           passed,
                           failed,
                           kat_path);
        }
        goto done;
    }

    if (!path_is_directory_sig(kat_path)) {
        ngcc_log_error("[sig][kat] --kat path is not a file or directory: %s", kat_path);
        return -1;
    }

    dir = opendir(kat_path);
    if (dir == NULL) {
        ngcc_log_error("[sig][kat] cannot open directory: %s", kat_path);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char file_path[2048];
        int len;
        int vrc;

        if (entry->d_name[0] == '.') {
            continue;
        }
        len = snprintf(file_path, sizeof(file_path), "%s/%s", kat_path, entry->d_name);
        if (len < 0 || len >= (int) sizeof(file_path)) {
            ngcc_log_error("[sig][kat] KAT file path too long: dir=%s file=%s", kat_path, entry->d_name);
            continue;
        }
        if (path_is_directory_sig(file_path)) {
            continue;
        }

        if (strncmp(entry->d_name, "KAT_SIG_", 8) != 0) {
            continue;  /* skip files not matching KAT_SIG_ prefix */
        }

        vrc = verify_sig_kat_one_file(api, file_path, &total, &passed, &failed);
        if (vrc == -2) {
            closedir(dir);
            goto done;
        }

        file_count++;
    }
    closedir(dir);

    if (file_count == 0) {
        ngcc_log_error("[sig][kat] no KAT_SIG_ files found in: %s", kat_path);
        goto done;
    }

    rc = (total > 0 && failed == 0) ? 0 : -1;
    if (rc != 0) {
        ngcc_log_error("[sig][kat] verification failed: total=%llu passed=%llu failed=%llu dir=%s",
                       total,
                       passed,
                       failed,
                       kat_path);
    }

done:
    if (out_total != NULL) {
        *out_total = total;
    }
    if (out_passed != NULL) {
        *out_passed = passed;
    }
    if (out_failed != NULL) {
        *out_failed = failed;
    }
    return rc;
}

static int sig_perf_op(void *ctx_ptr) {
    sig_ctx_t *ctx = (sig_ctx_t *) ctx_ptr;

    if (sig_prepare_sign_case(ctx) != 0) {
        return -1;
    }

    return sig_verify_once(ctx->api, ctx->pk, ctx->pk_len, ctx->sn, ctx->sn_len, ctx->msg, ctx->msg_len);
}

static int sig_keygen_perf_op(void *ctx_ptr) {
    sig_ctx_t *ctx = (sig_ctx_t *) ctx_ptr;

    ctx->pk_len = ctx->pk_cap;
    ctx->sk_len = ctx->sk_cap;
    return sig_keygen_once(ctx->api,
                           ctx->pk,
                           ctx->pk_cap,
                           &ctx->pk_len,
                           ctx->sk,
                           ctx->sk_cap,
                           &ctx->sk_len);
}

int ngcc_sig_keygen_performance(const ngcc_api_t *api,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result) {
    sig_ctx_t ctx;
    ngcc_perf_config_t local_cfg;
    int rc = -1;

    if (api == NULL || cfg == NULL || out_result == NULL) {
        ngcc_log_error("[sig][performance][keygen] invalid arguments: api_null=%d cfg_null=%d out_null=%d",
                       api == NULL,
                       cfg == NULL,
                       out_result == NULL);
        return -1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.api = api;
    ctx.pk_cap = api->sig_get_pk_len_bytes();
    ctx.sk_cap = api->sig_get_sk_len_bytes();

    if (!ngcc_is_valid_len(ctx.pk_cap) || !ngcc_is_valid_len(ctx.sk_cap)) {
        ngcc_log_error("[sig][performance][keygen] invalid advertised caps: pk_cap=%llu sk_cap=%llu",
                       ctx.pk_cap,
                       ctx.sk_cap);
        return -1;
    }

    ctx.pk = (unsigned char *) calloc(1, (size_t) ctx.pk_cap);
    ctx.sk = (unsigned char *) calloc(1, (size_t) ctx.sk_cap);
    if (ctx.pk == NULL || ctx.sk == NULL) {
        ngcc_log_error("[sig][performance][keygen] allocation failed: pk_cap=%llu sk_cap=%llu",
                       ctx.pk_cap,
                       ctx.sk_cap);
        goto out;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = ctx.pk_cap + ctx.sk_cap;
    if (ngcc_run_performance_op(&local_cfg, sig_keygen_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[sig][performance][keygen] benchmark failed: iterations=%llu", cfg->iterations);
        goto out;
    }

    rc = 0;

out:
    free(ctx.pk);
    free(ctx.sk);
    return rc;
}

static int sig_sign_perf_op(void *ctx_ptr) {
    sig_ctx_t *ctx = (sig_ctx_t *) ctx_ptr;

    if (ngcc_fill_random(ctx->msg, (size_t) ctx->msg_len) != 0) {
        return -1;
    }

    ctx->sn_len = ctx->sn_cap;
    return sig_sign_once(ctx->api,
                         ctx->sk,
                         ctx->sk_len,
                         ctx->msg,
                         ctx->msg_len,
                         ctx->sn,
                         ctx->sn_cap,
                         &ctx->sn_len);
}

int ngcc_sig_sign_performance(const ngcc_api_t *api,
                              size_t msg_len,
                              const ngcc_perf_config_t *cfg,
                              ngcc_perf_result_t *out_result) {
    sig_ctx_t ctx;
    ngcc_perf_config_t local_cfg;
    int rc = -1;

    if (cfg == NULL || out_result == NULL) {
        ngcc_log_error("[sig][performance][sign] invalid arguments: cfg_null=%d out_null=%d",
                       cfg == NULL,
                       out_result == NULL);
        return -1;
    }
    if (alloc_sig_ctx(api, msg_len, &ctx) != 0) {
        return -1;
    }

    ctx.pk_len = ctx.pk_cap;
    ctx.sk_len = ctx.sk_cap;
    if (sig_keygen_once(api,
                        ctx.pk,
                        ctx.pk_cap,
                        &ctx.pk_len,
                        ctx.sk,
                        ctx.sk_cap,
                        &ctx.sk_len) != 0) {
        ngcc_log_error("[sig][performance][sign] setup keygen failed: msg_len=%zu", msg_len);
        goto out;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = (unsigned long long) msg_len;
    if (ngcc_run_performance_op(&local_cfg, sig_sign_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[sig][performance][sign] benchmark failed: msg_len=%zu iterations=%llu",
                       msg_len,
                       cfg->iterations);
        goto out;
    }

    rc = 0;

out:
    free_sig_ctx(&ctx);
    return rc;
}

static int sig_verify_perf_op(void *ctx_ptr) {
    sig_ctx_t *ctx = (sig_ctx_t *) ctx_ptr;
    return sig_verify_once(ctx->api, ctx->pk, ctx->pk_len, ctx->sn, ctx->sn_len, ctx->msg, ctx->msg_len);
}

int ngcc_sig_verify_performance(const ngcc_api_t *api,
                                size_t msg_len,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result) {
    sig_ctx_t ctx;
    ngcc_perf_config_t local_cfg;
    int rc = -1;

    if (cfg == NULL || out_result == NULL || alloc_sig_ctx(api, msg_len, &ctx) != 0) {
        ngcc_log_error("[sig][performance][verify] invalid arguments or allocation failed: cfg_null=%d out_null=%d msg_len=%zu",
                       cfg == NULL,
                       out_result == NULL,
                       msg_len);
        return -1;
    }

    if (sig_prepare_sign_case(&ctx) != 0) {
        ngcc_log_error("[sig][performance][verify] setup sign case failed: msg_len=%zu", msg_len);
        goto out;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = (unsigned long long) msg_len;
    if (ngcc_run_performance_op(&local_cfg, sig_verify_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[sig][performance][verify] benchmark failed: msg_len=%zu iterations=%llu",
                       msg_len,
                       cfg->iterations);
        goto out;
    }

    rc = 0;

out:
    free_sig_ctx(&ctx);
    return rc;
}
