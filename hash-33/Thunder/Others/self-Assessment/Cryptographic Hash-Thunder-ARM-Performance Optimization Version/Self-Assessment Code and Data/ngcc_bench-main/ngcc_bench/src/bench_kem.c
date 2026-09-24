#include "bench_kem.h"

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
    unsigned char *ct;
    unsigned char *ss_a;
    unsigned char *ss_b;
    unsigned long long pk_cap;
    unsigned long long sk_cap;
    unsigned long long ct_cap;
    unsigned long long ss_cap;
} kem_perf_ctx_t;

static int kem_is_valid_output_len(unsigned long long len, unsigned long long cap) {
    return len != 0 && len <= cap;
}

static int kem_keygen_checked(const ngcc_api_t *api,
                              unsigned char *pk,
                              unsigned long long pk_cap,
                              unsigned char *sk,
                              unsigned long long sk_cap,
                              unsigned long long *out_pk_len,
                              unsigned long long *out_sk_len) {
    unsigned long long pk_len = pk_cap;
    unsigned long long sk_len = sk_cap;

    if (api->kem_keygen(pk, &pk_len, sk, &sk_len) != 0) {
        ngcc_log_error("[kem] kem_keygen failed");
        return -1;
    }
    if (!kem_is_valid_output_len(pk_len, pk_cap) ||
        !kem_is_valid_output_len(sk_len, sk_cap)) {
        ngcc_log_error("[kem] kem_keygen returned invalid lengths: pk_len=%llu pk_cap=%llu sk_len=%llu sk_cap=%llu",
                       pk_len,
                       pk_cap,
                       sk_len,
                       sk_cap);
        return -1;
    }

    if (out_pk_len != NULL) {
        *out_pk_len = pk_len;
    }
    if (out_sk_len != NULL) {
        *out_sk_len = sk_len;
    }
    return 0;
}

static int kem_enc_checked(const ngcc_api_t *api,
                           unsigned char *pk,
                           unsigned long long pk_len,
                           unsigned char *ss,
                           unsigned long long ss_cap,
                           unsigned long long *out_ss_len,
                           unsigned char *ct,
                           unsigned long long ct_cap,
                           unsigned long long *out_ct_len) {
    unsigned long long ss_len = ss_cap;
    unsigned long long ct_len = ct_cap;

    if (api->kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) {
        ngcc_log_error("[kem] kem_enc failed: pk_len=%llu ss_cap=%llu ct_cap=%llu",
                       pk_len,
                       ss_cap,
                       ct_cap);
        return -1;
    }
    if (!kem_is_valid_output_len(ss_len, ss_cap) ||
        !kem_is_valid_output_len(ct_len, ct_cap)) {
        ngcc_log_error("[kem] kem_enc returned invalid lengths: ss_len=%llu ss_cap=%llu ct_len=%llu ct_cap=%llu",
                       ss_len,
                       ss_cap,
                       ct_len,
                       ct_cap);
        return -1;
    }

    if (out_ss_len != NULL) {
        *out_ss_len = ss_len;
    }
    if (out_ct_len != NULL) {
        *out_ct_len = ct_len;
    }
    return 0;
}

static int kem_dec_checked(const ngcc_api_t *api,
                           unsigned char *sk,
                           unsigned long long sk_len,
                           unsigned char *ct,
                           unsigned long long ct_len,
                           unsigned char *ss,
                           unsigned long long ss_cap,
                           unsigned long long *out_ss_len) {
    unsigned long long ss_len = ss_cap;

    if (api->kem_dec(sk, sk_len, ct, ct_len, ss, &ss_len) != 0) {
        ngcc_log_error("[kem] kem_dec failed: sk_len=%llu ct_len=%llu ss_cap=%llu",
                       sk_len,
                       ct_len,
                       ss_cap);
        return -1;
    }
    if (!kem_is_valid_output_len(ss_len, ss_cap)) {
        ngcc_log_error("[kem] kem_dec returned invalid shared-secret length: ss_len=%llu ss_cap=%llu",
                       ss_len,
                       ss_cap);
        return -1;
    }

    if (out_ss_len != NULL) {
        *out_ss_len = ss_len;
    }
    return 0;
}

static int kem_dec_matches_expected(const ngcc_api_t *api,
                                    const unsigned char *sk,
                                    size_t sk_len,
                                    const unsigned char *ct,
                                    size_t ct_len,
                                    unsigned char *ss_out,
                                    unsigned long long ss_cap,
                                    const ngcc_kat_field_t *expected_ss,
                                    size_t vector,
                                    unsigned char sentinel) {
    unsigned long long ss_out_len = ss_cap;

    memset(ss_out, sentinel, (size_t) ss_cap);
    if (api->kem_dec((unsigned char *) sk,
                     (unsigned long long) sk_len,
                     (unsigned char *) ct,
                     (unsigned long long) ct_len,
                     ss_out,
                     &ss_out_len) != 0) {
        ngcc_log_error("[kem][kat] kem_dec failed: vector=%zu sk_len=%zu ct_len=%zu ss_cap=%llu sentinel=0x%02x",
                       vector,
                       sk_len,
                       ct_len,
                       ss_cap,
                       (unsigned int) sentinel);
        return -1;
    }

    if (ss_out_len != (unsigned long long) expected_ss->len ||
        memcmp(ss_out, expected_ss->data, expected_ss->len) != 0) {
        ngcc_log_error("[kem][kat] shared-secret mismatch: vector=%zu expected_len=%zu actual_len=%llu sentinel=0x%02x",
                       vector,
                       expected_ss->len,
                       ss_out_len,
                       (unsigned int) sentinel);
        return -1;
    }

    return 0;
}

static int kem_run_once(const ngcc_api_t *api,
                        unsigned char *pk,
                        unsigned long long pk_cap,
                        unsigned char *sk,
                        unsigned long long sk_cap,
                        unsigned char *ct,
                        unsigned long long ct_cap,
                        unsigned char *ss_a,
                        unsigned char *ss_b,
                        unsigned long long ss_cap) {
    unsigned long long pk_len;
    unsigned long long sk_len;
    unsigned long long ct_len;
    unsigned long long ss_a_len;
    unsigned long long ss_b_len;

    memset(pk, 0xA5, (size_t) pk_cap);
    memset(sk, 0x5A, (size_t) sk_cap);
    if (kem_keygen_checked(api, pk, pk_cap, sk, sk_cap, &pk_len, &sk_len) != 0) {
        return -1;
    }

    memset(ss_a, 0xA5, (size_t) ss_cap);
    memset(ct, 0x3C, (size_t) ct_cap);
    if (kem_enc_checked(api, pk, pk_len, ss_a, ss_cap, &ss_a_len, ct, ct_cap, &ct_len) != 0) {
        return -1;
    }

    memset(ss_b, 0x5A, (size_t) ss_cap);
    if (kem_dec_checked(api, sk, sk_len, ct, ct_len, ss_b, ss_cap, &ss_b_len) != 0 ||
        ss_a_len != ss_b_len) {
        ngcc_log_error("[kem] decapsulation failed or shared-secret length mismatch: ss_a_len=%llu ss_b_len=%llu",
                       ss_a_len,
                       ss_b_len);
        return -1;
    }

    if (memcmp(ss_a, ss_b, (size_t) ss_a_len) != 0) {
        ngcc_log_error("[kem] shared-secret mismatch: ss_len=%llu", ss_a_len);
        return -1;
    }

    return 0;
}

static int kem_perf_op(void *ctx_ptr) {
    kem_perf_ctx_t *ctx = (kem_perf_ctx_t *) ctx_ptr;
    return kem_run_once(ctx->api,
                        ctx->pk,
                        ctx->pk_cap,
                        ctx->sk,
                        ctx->sk_cap,
                        ctx->ct,
                        ctx->ct_cap,
                        ctx->ss_a,
                        ctx->ss_b,
                        ctx->ss_cap);
}

int ngcc_kem_correctness(const ngcc_api_t *api) {
    unsigned long long pk_cap;
    unsigned long long sk_cap;
    unsigned long long ct_cap;
    unsigned long long ss_cap;
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss_a = NULL;
    unsigned char *ss_b = NULL;
    int rc = -1;

    if (api == NULL) {
        ngcc_log_error("[kem][correctness] invalid arguments: api_null=1");
        return -1;
    }

    pk_cap = api->kem_get_pk_len_bytes();
    sk_cap = api->kem_get_sk_len_bytes();
    ct_cap = api->kem_get_ct_len_bytes();
    ss_cap = api->kem_get_ss_len_bytes();
    if (!ngcc_is_valid_len(pk_cap) || !ngcc_is_valid_len(sk_cap) || !ngcc_is_valid_len(ct_cap) || !ngcc_is_valid_len(ss_cap)) {
        ngcc_log_error("[kem][correctness] invalid advertised caps: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       ct_cap,
                       ss_cap);
        return -1;
    }

    pk = (unsigned char *) calloc(1, (size_t) pk_cap);
    sk = (unsigned char *) calloc(1, (size_t) sk_cap);
    ct = (unsigned char *) calloc(1, (size_t) ct_cap);
    ss_a = (unsigned char *) calloc(1, (size_t) ss_cap);
    ss_b = (unsigned char *) calloc(1, (size_t) ss_cap);
    if (pk == NULL || sk == NULL || ct == NULL || ss_a == NULL || ss_b == NULL) {
        ngcc_log_error("[kem][correctness] allocation failed: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       ct_cap,
                       ss_cap);
        goto out;
    }

    memset(pk, 0xA5, (size_t) pk_cap);
    memset(sk, 0x5A, (size_t) sk_cap);
    memset(ct, 0x3C, (size_t) ct_cap);
    memset(ss_a, 0xA5, (size_t) ss_cap);
    memset(ss_b, 0x5A, (size_t) ss_cap);

    if (kem_run_once(api, pk, pk_cap, sk, sk_cap, ct, ct_cap, ss_a, ss_b, ss_cap) != 0) {
        ngcc_log_error("[kem][correctness] KEM run failed");
        goto out;
    }

    rc = 0;

out:
    free(pk);
    free(sk);
    free(ct);
    free(ss_a);
    free(ss_b);
    return rc;
}

static unsigned long long kem_field_to_u64(const ngcc_kat_field_t *f) {
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

static int kem_check_field_len(const char *field_name, const ngcc_kat_field_t *data_field,
                               const ngcc_kat_field_t *len_field) {
    unsigned long long expected;
    if (len_field == NULL || data_field == NULL) {
        return 0;
    }
    expected = kem_field_to_u64(len_field);
    if (expected == 0) {
        return 0;
    }
    if ((unsigned long long) data_field->len != expected) {
        ngcc_log_error("[kem][kat] %s length mismatch: expected_bytes=%llu actual_bytes=%zu",
                       field_name,
                       expected,
                       data_field->len);
        return -1;
    }
    return 0;
}

static int path_is_directory_kem(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int path_is_regular_file_kem(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

static int verify_kem_kat_vectors(const ngcc_api_t *api,
                                  const ngcc_kat_file_t *kat,
                                  unsigned long long *io_total,
                                  unsigned long long *io_passed,
                                  unsigned long long *io_failed) {
    unsigned long long sk_cap;
    unsigned long long ct_cap;
    unsigned long long ss_cap;
    unsigned char *ss_out = NULL;
    size_t i;

    sk_cap = api->kem_get_sk_len_bytes();
    ct_cap = api->kem_get_ct_len_bytes();
    ss_cap = api->kem_get_ss_len_bytes();
    if (!ngcc_is_valid_len(sk_cap) || !ngcc_is_valid_len(ct_cap) || !ngcc_is_valid_len(ss_cap)) {
        ngcc_log_error("[kem][kat] invalid advertised caps: sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       sk_cap,
                       ct_cap,
                       ss_cap);
        return -1;
    }

    ss_out = (unsigned char *) calloc(1, (size_t) ss_cap);
    if (ss_out == NULL) {
        ngcc_log_error("[kem][kat] allocation failed: ss_cap=%llu", ss_cap);
        return -1;
    }

    for (i = 0; i < kat->count; ++i) {
        const ngcc_kat_vector_t *vec = &kat->vectors[i];
        const ngcc_kat_field_t *sk = ngcc_kat_get_field(vec, "SK");
        const ngcc_kat_field_t *ct = ngcc_kat_get_field(vec, "CT");
        const ngcc_kat_field_t *ss = ngcc_kat_get_field(vec, "SS");

        /* Skip vectors with empty output (blank template) */
        if (sk == NULL || sk->data == NULL || sk->len == 0) {
            continue;
        }
        if (ct == NULL || ct->data == NULL || ct->len == 0) {
            continue;
        }
        if (ss == NULL || ss->data == NULL || ss->len == 0) {
            continue;
        }

        (*io_total)++;

        /* Validate _Len fields match actual data length */
        if (kem_check_field_len("SK", sk, ngcc_kat_get_field(vec, "SK_Len")) != 0 ||
            kem_check_field_len("CT", ct, ngcc_kat_get_field(vec, "CT_Len")) != 0 ||
            kem_check_field_len("SS", ss, ngcc_kat_get_field(vec, "SS_Len")) != 0) {
            (*io_failed)++;
            continue;
        }
        if (sk->len > sk_cap || ct->len > ct_cap || ss->len > ss_cap) {
            ngcc_log_error("[kem][kat] vector exceeds caps: vector=%zu sk_len=%zu sk_cap=%llu ct_len=%zu ct_cap=%llu ss_len=%zu ss_cap=%llu",
                           i,
                           sk->len,
                           sk_cap,
                           ct->len,
                           ct_cap,
                           ss->len,
                           ss_cap);
            (*io_failed)++;
            continue;
        }

        if (kem_dec_matches_expected(api,
                                     sk->data,
                                     sk->len,
                                     ct->data,
                                     ct->len,
                                     ss_out,
                                     ss_cap,
                                     ss,
                                     i,
                                     0xA5) != 0) {
            (*io_failed)++;
            continue;
        }

        (*io_passed)++;
    }

    free(ss_out);
    return 0;
}

static int verify_kem_kat_one_file(const ngcc_api_t *api,
                                   const char *file_path,
                                   unsigned long long *io_total,
                                   unsigned long long *io_passed,
                                   unsigned long long *io_failed) {
    ngcc_kat_file_t kat;
    int rc;

    memset(&kat, 0, sizeof(kat));
    if (ngcc_kat_parse_file(file_path, &kat) != 0) {
        ngcc_log_error("[kem][kat] failed to parse file: %s", file_path);
        return -2;
    }

    rc = verify_kem_kat_vectors(api, &kat, io_total, io_passed, io_failed);
    ngcc_kat_free(&kat);
    return rc;
}

int ngcc_kem_correctness_kat_file(const ngcc_api_t *api,
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
        ngcc_log_error("[kem][kat] invalid arguments: api_null=%d kat_path_null=%d",
                       api == NULL,
                       kat_path == NULL);
        return -1;
    }

    if (path_is_regular_file_kem(kat_path)) {
        int vrc = verify_kem_kat_one_file(api, kat_path, &total, &passed, &failed);

        file_count = 1;
        rc = (vrc == 0 && total > 0 && failed == 0) ? 0 : -1;
        if (rc != 0) {
            ngcc_log_error("[kem][kat] verification failed: total=%llu passed=%llu failed=%llu file=%s",
                           total,
                           passed,
                           failed,
                           kat_path);
        }
        goto done;
    }

    if (!path_is_directory_kem(kat_path)) {
        ngcc_log_error("[kem][kat] --kat path is not a file or directory: %s", kat_path);
        return -1;
    }

    dir = opendir(kat_path);
    if (dir == NULL) {
        ngcc_log_error("[kem][kat] cannot open directory: %s", kat_path);
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
            ngcc_log_error("[kem][kat] KAT file path too long: dir=%s file=%s", kat_path, entry->d_name);
            continue;
        }
        if (path_is_directory_kem(file_path)) {
            continue;
        }

        if (strncmp(entry->d_name, "KAT_KEM_", 8) != 0) {
            continue;  /* skip files not matching KAT_KEM_ prefix */
        }

        vrc = verify_kem_kat_one_file(api, file_path, &total, &passed, &failed);
        if (vrc == -2) {
            closedir(dir);
            goto done;
        }

        file_count++;
    }
    closedir(dir);

    if (file_count == 0) {
        ngcc_log_error("[kem][kat] no KAT_KEM_ files found in: %s", kat_path);
        goto done;
    }

    rc = (total > 0 && failed == 0) ? 0 : -1;
    if (rc != 0) {
        ngcc_log_error("[kem][kat] verification failed: total=%llu passed=%llu failed=%llu dir=%s",
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

int ngcc_kem_performance(const ngcc_api_t *api,
                         const ngcc_perf_config_t *cfg,
                         ngcc_perf_result_t *out_result) {
    kem_perf_ctx_t ctx;
    ngcc_perf_config_t local_cfg;
    int rc = -1;

    if (api == NULL || cfg == NULL || out_result == NULL) {
        ngcc_log_error("[kem][performance] invalid arguments: api_null=%d cfg_null=%d out_null=%d",
                       api == NULL,
                       cfg == NULL,
                       out_result == NULL);
        return -1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.api = api;
    ctx.pk_cap = api->kem_get_pk_len_bytes();
    ctx.sk_cap = api->kem_get_sk_len_bytes();
    ctx.ct_cap = api->kem_get_ct_len_bytes();
    ctx.ss_cap = api->kem_get_ss_len_bytes();

    if (!ngcc_is_valid_len(ctx.pk_cap) || !ngcc_is_valid_len(ctx.sk_cap) ||
        !ngcc_is_valid_len(ctx.ct_cap) || !ngcc_is_valid_len(ctx.ss_cap)) {
        ngcc_log_error("[kem][performance] invalid advertised caps: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       ctx.pk_cap,
                       ctx.sk_cap,
                       ctx.ct_cap,
                       ctx.ss_cap);
        return -1;
    }

    ctx.pk = (unsigned char *) calloc(1, (size_t) ctx.pk_cap);
    ctx.sk = (unsigned char *) calloc(1, (size_t) ctx.sk_cap);
    ctx.ct = (unsigned char *) calloc(1, (size_t) ctx.ct_cap);
    ctx.ss_a = (unsigned char *) calloc(1, (size_t) ctx.ss_cap);
    ctx.ss_b = (unsigned char *) calloc(1, (size_t) ctx.ss_cap);
    if (ctx.pk == NULL || ctx.sk == NULL || ctx.ct == NULL || ctx.ss_a == NULL || ctx.ss_b == NULL) {
        ngcc_log_error("[kem][performance] allocation failed: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       ctx.pk_cap,
                       ctx.sk_cap,
                       ctx.ct_cap,
                       ctx.ss_cap);
        goto cleanup;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = ctx.ct_cap;
    if (ngcc_run_performance_op(&local_cfg, kem_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[kem][performance] aggregate benchmark failed: iterations=%llu", cfg->iterations);
        goto cleanup;
    }

    rc = 0;

cleanup:
    free(ctx.pk);
    free(ctx.sk);
    free(ctx.ct);
    free(ctx.ss_a);
    free(ctx.ss_b);
    return rc;
}

/* ================================================================
 *  Separate KEM Keygen / Encap / Decap performance functions
 * ================================================================ */

/* ── Keygen-only performance ─────────────────────────────────────── */

static int kem_keygen_perf_op(void *ctx_ptr) {
    kem_perf_ctx_t *ctx = (kem_perf_ctx_t *) ctx_ptr;
    return kem_keygen_checked(ctx->api,
                              ctx->pk,
                              ctx->pk_cap,
                              ctx->sk,
                              ctx->sk_cap,
                              NULL,
                              NULL);
}

int ngcc_kem_keygen_performance(const ngcc_api_t *api,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result) {
    kem_perf_ctx_t ctx;
    ngcc_perf_config_t local_cfg;
    int rc = -1;

    if (api == NULL || cfg == NULL || out_result == NULL) {
        ngcc_log_error("[kem][performance][keygen] invalid arguments: api_null=%d cfg_null=%d out_null=%d",
                       api == NULL,
                       cfg == NULL,
                       out_result == NULL);
        return -1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.api = api;
    ctx.pk_cap = api->kem_get_pk_len_bytes();
    ctx.sk_cap = api->kem_get_sk_len_bytes();

    if (!ngcc_is_valid_len(ctx.pk_cap) || !ngcc_is_valid_len(ctx.sk_cap)) {
        ngcc_log_error("[kem][performance][keygen] invalid advertised caps: pk_cap=%llu sk_cap=%llu",
                       ctx.pk_cap,
                       ctx.sk_cap);
        return -1;
    }

    ctx.pk = (unsigned char *) calloc(1, (size_t) ctx.pk_cap);
    ctx.sk = (unsigned char *) calloc(1, (size_t) ctx.sk_cap);
    if (ctx.pk == NULL || ctx.sk == NULL) {
        ngcc_log_error("[kem][performance][keygen] allocation failed: pk_cap=%llu sk_cap=%llu",
                       ctx.pk_cap,
                       ctx.sk_cap);
        goto cleanup_keygen;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = ctx.pk_cap;
    if (ngcc_run_performance_op(&local_cfg, kem_keygen_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[kem][performance][keygen] benchmark failed: iterations=%llu", cfg->iterations);
        goto cleanup_keygen;
    }

    rc = 0;

cleanup_keygen:
    free(ctx.pk);
    free(ctx.sk);
    return rc;
}

/* ── Encap-only performance ──────────────────────────────────────── */

typedef struct {
    const ngcc_api_t *api;
    unsigned char *pk;
    unsigned char *ct;
    unsigned char *ss;
    unsigned long long pk_len;
    unsigned long long ct_cap;
    unsigned long long ss_cap;
} kem_encap_perf_ctx_t;

static int kem_encap_perf_op(void *ctx_ptr) {
    kem_encap_perf_ctx_t *ctx = (kem_encap_perf_ctx_t *) ctx_ptr;
    return kem_enc_checked(ctx->api,
                           ctx->pk,
                           ctx->pk_len,
                           ctx->ss,
                           ctx->ss_cap,
                           NULL,
                           ctx->ct,
                           ctx->ct_cap,
                           NULL);
}

int ngcc_kem_encap_performance(const ngcc_api_t *api,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result) {
    kem_encap_perf_ctx_t ctx;
    ngcc_perf_config_t local_cfg;
    unsigned char *sk = NULL;
    unsigned long long pk_cap, sk_cap;
    int rc = -1;

    if (api == NULL || cfg == NULL || out_result == NULL) {
        ngcc_log_error("[kem][performance][encap] invalid arguments: api_null=%d cfg_null=%d out_null=%d",
                       api == NULL,
                       cfg == NULL,
                       out_result == NULL);
        return -1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.api = api;
    pk_cap = api->kem_get_pk_len_bytes();
    sk_cap = api->kem_get_sk_len_bytes();
    ctx.ct_cap = api->kem_get_ct_len_bytes();
    ctx.ss_cap = api->kem_get_ss_len_bytes();

    if (!ngcc_is_valid_len(pk_cap) || !ngcc_is_valid_len(sk_cap) ||
        !ngcc_is_valid_len(ctx.ct_cap) || !ngcc_is_valid_len(ctx.ss_cap)) {
        ngcc_log_error("[kem][performance][encap] invalid advertised caps: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       ctx.ct_cap,
                       ctx.ss_cap);
        return -1;
    }

    ctx.pk = (unsigned char *) calloc(1, (size_t) pk_cap);
    sk = (unsigned char *) calloc(1, (size_t) sk_cap);
    ctx.ct = (unsigned char *) calloc(1, (size_t) ctx.ct_cap);
    ctx.ss = (unsigned char *) calloc(1, (size_t) ctx.ss_cap);
    if (ctx.pk == NULL || sk == NULL || ctx.ct == NULL || ctx.ss == NULL) {
        ngcc_log_error("[kem][performance][encap] allocation failed: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       ctx.ct_cap,
                       ctx.ss_cap);
        goto cleanup_encap;
    }

    /* keygen once as setup */
    {
        unsigned long long pk_len;
        if (kem_keygen_checked(api, ctx.pk, pk_cap, sk, sk_cap, &pk_len, NULL) != 0) {
            ngcc_log_error("[kem][performance][encap] setup keygen failed");
            goto cleanup_encap;
        }
        ctx.pk_len = pk_len;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = ctx.ct_cap;
    if (ngcc_run_performance_op(&local_cfg, kem_encap_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[kem][performance][encap] benchmark failed: pk_len=%llu iterations=%llu",
                       ctx.pk_len,
                       cfg->iterations);
        goto cleanup_encap;
    }

    rc = 0;

cleanup_encap:
    free(ctx.pk);
    free(sk);
    free(ctx.ct);
    free(ctx.ss);
    return rc;
}

/* ── Decap-only performance ──────────────────────────────────────── */

typedef struct {
    const ngcc_api_t *api;
    unsigned char *sk;
    unsigned char *ct;
    unsigned char *ss;
    unsigned long long sk_len;
    unsigned long long ct_len;
    unsigned long long ss_cap;
} kem_decap_perf_ctx_t;

static int kem_decap_perf_op(void *ctx_ptr) {
    kem_decap_perf_ctx_t *ctx = (kem_decap_perf_ctx_t *) ctx_ptr;
    return kem_dec_checked(ctx->api,
                           ctx->sk,
                           ctx->sk_len,
                           ctx->ct,
                           ctx->ct_len,
                           ctx->ss,
                           ctx->ss_cap,
                           NULL);
}

int ngcc_kem_decap_performance(const ngcc_api_t *api,
                                const ngcc_perf_config_t *cfg,
                                ngcc_perf_result_t *out_result) {
    kem_decap_perf_ctx_t ctx;
    ngcc_perf_config_t local_cfg;
    unsigned char *pk = NULL;
    unsigned char *ss_enc = NULL;
    unsigned long long pk_cap, ss_cap;
    int rc = -1;

    if (api == NULL || cfg == NULL || out_result == NULL) {
        ngcc_log_error("[kem][performance][decap] invalid arguments: api_null=%d cfg_null=%d out_null=%d",
                       api == NULL,
                       cfg == NULL,
                       out_result == NULL);
        return -1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.api = api;
    pk_cap = api->kem_get_pk_len_bytes();
    ctx.ss_cap = api->kem_get_ss_len_bytes();
    ss_cap = ctx.ss_cap;

    unsigned long long sk_cap = api->kem_get_sk_len_bytes();
    unsigned long long ct_cap = api->kem_get_ct_len_bytes();

    if (!ngcc_is_valid_len(pk_cap) || !ngcc_is_valid_len(sk_cap) ||
        !ngcc_is_valid_len(ct_cap) || !ngcc_is_valid_len(ss_cap)) {
        ngcc_log_error("[kem][performance][decap] invalid advertised caps: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       ct_cap,
                       ss_cap);
        return -1;
    }

    pk = (unsigned char *) calloc(1, (size_t) pk_cap);
    ctx.sk = (unsigned char *) calloc(1, (size_t) sk_cap);
    ctx.ct = (unsigned char *) calloc(1, (size_t) ct_cap);
    ctx.ss = (unsigned char *) calloc(1, (size_t) ss_cap);
    ss_enc = (unsigned char *) calloc(1, (size_t) ss_cap);
    if (pk == NULL || ctx.sk == NULL || ctx.ct == NULL || ctx.ss == NULL || ss_enc == NULL) {
        ngcc_log_error("[kem][performance][decap] allocation failed: pk_cap=%llu sk_cap=%llu ct_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       ct_cap,
                       ss_cap);
        goto cleanup_decap;
    }

    /* keygen + encap once as setup */
    {
        unsigned long long pk_len;
        unsigned long long sk_len_out;
        unsigned long long ct_len;
        if (kem_keygen_checked(api, pk, pk_cap, ctx.sk, sk_cap, &pk_len, &sk_len_out) != 0) {
            ngcc_log_error("[kem][performance][decap] setup keygen failed");
            goto cleanup_decap;
        }
        ctx.sk_len = sk_len_out;
        if (kem_enc_checked(api, pk, pk_len, ss_enc, ss_cap, NULL, ctx.ct, ct_cap, &ct_len) != 0) {
            ngcc_log_error("[kem][performance][decap] setup encap failed: pk_len=%llu", pk_len);
            goto cleanup_decap;
        }
        ctx.ct_len = ct_len;
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = ss_cap;
    if (ngcc_run_performance_op(&local_cfg, kem_decap_perf_op, &ctx, out_result) != 0) {
        ngcc_log_error("[kem][performance][decap] benchmark failed: sk_len=%llu ct_len=%llu iterations=%llu",
                       ctx.sk_len,
                       ctx.ct_len,
                       cfg->iterations);
        goto cleanup_decap;
    }

    rc = 0;

cleanup_decap:
    free(pk);
    free(ctx.sk);
    free(ctx.ct);
    free(ctx.ss);
    free(ss_enc);
    return rc;
}
