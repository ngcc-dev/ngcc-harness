#include "bench_kex.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bench_core.h"
#include "kat_parser.h"
#include "ngcc_log.h"

static int kex_has_supported_pass_count(const ngcc_api_t *api) {
    return api != NULL &&
           api->kex_passes_num >= NGCC_KEX_MIN_PASSES &&
           api->kex_passes_num <= NGCC_KEX_MAX_PASSES;
}

static int kex_is_valid_output_len(unsigned long long len, unsigned long long cap) {
    return len != 0 && len <= cap;
}

typedef int (*kex_derive_fn_t)(unsigned char *sk,
                               unsigned long long sk_len,
                               unsigned char *pk,
                               unsigned long long pk_len,
                               unsigned char *msg,
                               unsigned long long msg_len,
                               unsigned char *state,
                               unsigned long long state_len,
                               unsigned char *ss,
                               unsigned long long *ss_len);

static int kex_derive_matches_expected(kex_derive_fn_t derive_fn,
                                       const char *side,
                                       size_t vector,
                                       unsigned char *sk,
                                       size_t sk_len,
                                       unsigned char *pk,
                                       size_t pk_len,
                                       unsigned char *msg,
                                       size_t msg_len,
                                       unsigned char *state,
                                       size_t state_len,
                                       unsigned char *ss_out,
                                       unsigned long long ss_cap,
                                       const ngcc_kat_field_t *expected_ss,
                                       unsigned char sentinel) {
    unsigned long long ss_out_len = ss_cap;

    memset(ss_out, sentinel, (size_t) ss_cap);
    if (derive_fn(sk,
                  (unsigned long long) sk_len,
                  pk,
                  (unsigned long long) pk_len,
                  msg,
                  (unsigned long long) msg_len,
                  state,
                  (unsigned long long) state_len,
                  ss_out,
                  &ss_out_len) != 0) {
        ngcc_log_error("[kex][kat] %s derive failed: vector=%zu sk_len=%zu pk_len=%zu msg_len=%zu state_len=%zu ss_cap=%llu sentinel=0x%02x",
                       side,
                       vector,
                       sk_len,
                       pk_len,
                       msg_len,
                       state_len,
                       ss_cap,
                       (unsigned int) sentinel);
        return -1;
    }

    if (ss_out_len != (unsigned long long) expected_ss->len ||
        memcmp(ss_out, expected_ss->data, expected_ss->len) != 0) {
        ngcc_log_error("[kex][kat] %s shared-secret mismatch: vector=%zu expected_len=%zu actual_len=%llu sentinel=0x%02x",
                       side,
                       vector,
                       expected_ss->len,
                       ss_out_len,
                       (unsigned int) sentinel);
        return -1;
    }

    return 0;
}

/* Dynamic multi-pass KEX execution.
 * Pass 1 (A-side, 8 params): ska, pkb, sta, m_out
 * Pass 2+ (10 params): sk, pk, m_in, st, m_out
 *   - Odd passes: A-side (ska, pkb, sta)
 *   - Even passes: B-side (skb, pka, stb)
 * derive_ss_a takes last B-msg, derive_ss_b takes last A-msg.
 */
static int kex_run_once(const ngcc_api_t *api,
                        unsigned char *pka,
                        unsigned char *ska,
                        unsigned char *sta,
                        unsigned char *pkb,
                        unsigned char *skb,
                        unsigned char *stb,
                        unsigned char *msg_buf0,
                        unsigned char *msg_buf1,
                        unsigned char *ssa,
                        unsigned char *ssb,
                        unsigned long long pk_cap,
                        unsigned long long sk_cap,
                        unsigned long long sta_cap,
                        unsigned long long stb_cap,
                        unsigned long long msg_cap,
                        unsigned long long ss_cap) {
    unsigned long long pka_len = pk_cap;
    unsigned long long ska_len = sk_cap;
    unsigned long long sta_len = sta_cap;
    unsigned long long pkb_len = pk_cap;
    unsigned long long skb_len = sk_cap;
    unsigned long long stb_len = stb_cap;
    unsigned long long ssa_len = ss_cap;
    unsigned long long ssb_len = ss_cap;

    unsigned char *m_cur;
    unsigned char *m_prev;
    unsigned long long m_cur_len = 0;
    unsigned long long m_prev_len = 0;
    unsigned char *ma = NULL;    /* last message from A */
    unsigned char *mb = NULL;    /* last message from B */
    unsigned long long ma_len = 0;
    unsigned long long mb_len = 0;
    unsigned long long passes = api->kex_passes_num;
    unsigned long long p;
    int rc;

    if (!kex_has_supported_pass_count(api)) {
        ngcc_log_error("[kex] unsupported pass count: passes=%llu min=%llu max=%llu",
                       api != NULL ? api->kex_passes_num : 0ULL,
                       NGCC_KEX_MIN_PASSES,
                       NGCC_KEX_MAX_PASSES);
        return -1;
    }

    memset(pka, 0xA5, (size_t) pk_cap);
    memset(ska, 0xA6, (size_t) sk_cap);
    memset(sta, 0xA7, (size_t) sta_cap);
    memset(pkb, 0x5A, (size_t) pk_cap);
    memset(skb, 0x5B, (size_t) sk_cap);
    memset(stb, 0x5C, (size_t) stb_cap);

    if (api->kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len) != 0) {
        ngcc_log_error("[kex] kex_init_a failed");
        return -1;
    }
    if (api->kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len) != 0) {
        ngcc_log_error("[kex] kex_init_b failed");
        return -1;
    }

    if (!kex_is_valid_output_len(pka_len, pk_cap) ||
        !kex_is_valid_output_len(pkb_len, pk_cap) ||
        !kex_is_valid_output_len(ska_len, sk_cap) ||
        !kex_is_valid_output_len(skb_len, sk_cap) ||
        !kex_is_valid_output_len(sta_len, sta_cap) ||
        !kex_is_valid_output_len(stb_len, stb_cap)) {
        ngcc_log_error("[kex] init returned invalid lengths: pka_len=%llu pkb_len=%llu pk_cap=%llu ska_len=%llu skb_len=%llu sk_cap=%llu sta_len=%llu sta_cap=%llu stb_len=%llu stb_cap=%llu",
                       pka_len,
                       pkb_len,
                       pk_cap,
                       ska_len,
                       skb_len,
                       sk_cap,
                       sta_len,
                       sta_cap,
                       stb_len,
                       stb_cap);
        return -1;
    }

    /* Pass 1: A-side, 8 params (ska, pkb, sta, m_out) */
    m_cur = msg_buf0;
    memset(m_cur, 0xA8, (size_t) msg_cap);
    m_cur_len = msg_cap;
    rc = api->kex_pass1_fn(ska, ska_len, pkb, pkb_len, sta, &sta_len, m_cur, &m_cur_len);
    if (rc < 0) {
        ngcc_log_error("[kex] pass1 failed: rc=%d", rc);
        return -1;
    }
    if (!kex_is_valid_output_len(m_cur_len, msg_cap) ||
        !kex_is_valid_output_len(sta_len, sta_cap)) {
        ngcc_log_error("[kex] pass1 returned invalid lengths: msg_len=%llu msg_cap=%llu sta_len=%llu sta_cap=%llu",
                       m_cur_len,
                       msg_cap,
                       sta_len,
                       sta_cap);
        return -1;
    }
    ma = m_cur;
    ma_len = m_cur_len;

    /* Pass 2..N: alternate A/B sides */
    for (p = 2; p <= passes && rc == 0; ++p) {
        kex_pass_fn_t fn = api->kex_pass_fns[p - 2];
        m_prev = m_cur;
        m_prev_len = m_cur_len;
        m_cur = (m_prev == msg_buf0) ? msg_buf1 : msg_buf0;
        memset(m_cur, (p % 2 == 0) ? 0x5D : 0xA9, (size_t) msg_cap);
        m_cur_len = msg_cap;

        if (p % 2 == 0) {
            /* Even pass: B-side (skb, pka, m_prev, stb, m_out) */
            rc = fn(skb, skb_len, pka, pka_len, m_prev, m_prev_len,
                    stb, &stb_len, m_cur, &m_cur_len);
            if (rc < 0) {
                ngcc_log_error("[kex] pass%llu failed on B side: rc=%d prev_msg_len=%llu", p, rc, m_prev_len);
                return -1;
            }
            if (!kex_is_valid_output_len(m_cur_len, msg_cap) ||
                !kex_is_valid_output_len(stb_len, stb_cap)) {
                ngcc_log_error("[kex] pass%llu returned invalid B-side lengths: msg_len=%llu msg_cap=%llu stb_len=%llu stb_cap=%llu",
                               p,
                               m_cur_len,
                               msg_cap,
                               stb_len,
                               stb_cap);
                return -1;
            }
            mb = m_cur;
            mb_len = m_cur_len;
        } else {
            /* Odd pass: A-side (ska, pkb, m_prev, sta, m_out) */
            rc = fn(ska, ska_len, pkb, pkb_len, m_prev, m_prev_len,
                    sta, &sta_len, m_cur, &m_cur_len);
            if (rc < 0) {
                ngcc_log_error("[kex] pass%llu failed on A side: rc=%d prev_msg_len=%llu", p, rc, m_prev_len);
                return -1;
            }
            if (!kex_is_valid_output_len(m_cur_len, msg_cap) ||
                !kex_is_valid_output_len(sta_len, sta_cap)) {
                ngcc_log_error("[kex] pass%llu returned invalid A-side lengths: msg_len=%llu msg_cap=%llu sta_len=%llu sta_cap=%llu",
                               p,
                               m_cur_len,
                               msg_cap,
                               sta_len,
                               sta_cap);
                return -1;
            }
            ma = m_cur;
            ma_len = m_cur_len;
        }
    }

    if (ma == NULL || mb == NULL ||
        !kex_is_valid_output_len(ma_len, msg_cap) ||
        !kex_is_valid_output_len(mb_len, msg_cap)) {
        ngcc_log_error("[kex] missing final messages or invalid lengths: ma_null=%d mb_null=%d ma_len=%llu mb_len=%llu msg_cap=%llu passes=%llu rc=%d",
                       ma == NULL,
                       mb == NULL,
                       ma_len,
                       mb_len,
                       msg_cap,
                       passes,
                       rc);
        return -1;
    }

    memset(ssa, 0xA5, (size_t) ss_cap);
    if (api->kex_derive_ss_a(ska, ska_len, pkb, pkb_len, mb, mb_len, sta, sta_len, ssa, &ssa_len) != 0) {
        ngcc_log_error("[kex] kex_derive_ss_a failed: ska_len=%llu pkb_len=%llu mb_len=%llu sta_len=%llu ss_cap=%llu",
                       ska_len,
                       pkb_len,
                       mb_len,
                       sta_len,
                       ss_cap);
        return -1;
    }
    memset(ssb, 0x5A, (size_t) ss_cap);
    if (api->kex_derive_ss_b(skb, skb_len, pka, pka_len, ma, ma_len, stb, stb_len, ssb, &ssb_len) != 0) {
        ngcc_log_error("[kex] kex_derive_ss_b failed: skb_len=%llu pka_len=%llu ma_len=%llu stb_len=%llu ss_cap=%llu",
                       skb_len,
                       pka_len,
                       ma_len,
                       stb_len,
                       ss_cap);
        return -1;
    }

    if (!kex_is_valid_output_len(ssa_len, ss_cap) ||
        !kex_is_valid_output_len(ssb_len, ss_cap) ||
        ssa_len != ssb_len) {
        ngcc_log_error("[kex] derive returned invalid shared-secret lengths: ssa_len=%llu ssb_len=%llu ss_cap=%llu",
                       ssa_len,
                       ssb_len,
                       ss_cap);
        return -1;
    }

    if (memcmp(ssa, ssb, (size_t) ssa_len) != 0) {
        ngcc_log_error("[kex] shared-secret mismatch: ss_len=%llu", ssa_len);
        return -1;
    }

    return 0;
}


int ngcc_kex_correctness(const ngcc_api_t *api) {
    unsigned long long pk_cap;
    unsigned long long sk_cap;
    unsigned long long sta_cap;
    unsigned long long stb_cap;
    unsigned long long msg_cap;
    unsigned long long ss_cap;

    unsigned char *pka = NULL;
    unsigned char *ska = NULL;
    unsigned char *sta = NULL;
    unsigned char *pkb = NULL;
    unsigned char *skb = NULL;
    unsigned char *stb = NULL;
    unsigned char *msg_buf[2] = {NULL, NULL};
    unsigned char *ssa = NULL;
    unsigned char *ssb = NULL;
    int rc = -1;

    if (api == NULL) {
        ngcc_log_error("[kex][correctness] invalid arguments: api_null=1");
        return -1;
    }
    if (!kex_has_supported_pass_count(api)) {
        ngcc_log_error("[kex][correctness] unsupported pass count: passes=%llu min=%llu max=%llu",
                       api->kex_passes_num,
                       NGCC_KEX_MIN_PASSES,
                       NGCC_KEX_MAX_PASSES);
        return -1;
    }

    pk_cap = api->kex_get_pk_len_bytes();
    sk_cap = api->kex_get_sk_len_bytes();
    sta_cap = api->kex_get_sta_len_bytes();
    stb_cap = api->kex_get_stb_len_bytes();
    msg_cap = api->kex_get_total_msg_len_bytes();
    ss_cap = api->kex_get_ss_len_bytes();

    if (!ngcc_is_valid_len(pk_cap) || !ngcc_is_valid_len(sk_cap) || !ngcc_is_valid_len(sta_cap) || !ngcc_is_valid_len(stb_cap) ||
        !ngcc_is_valid_len(msg_cap) || !ngcc_is_valid_len(ss_cap)) {
        ngcc_log_error("[kex][correctness] invalid advertised caps: pk_cap=%llu sk_cap=%llu sta_cap=%llu stb_cap=%llu msg_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       sta_cap,
                       stb_cap,
                       msg_cap,
                       ss_cap);
        return -1;
    }

    pka = (unsigned char *) calloc(1, (size_t) pk_cap);
    ska = (unsigned char *) calloc(1, (size_t) sk_cap);
    sta = (unsigned char *) calloc(1, (size_t) sta_cap);
    pkb = (unsigned char *) calloc(1, (size_t) pk_cap);
    skb = (unsigned char *) calloc(1, (size_t) sk_cap);
    stb = (unsigned char *) calloc(1, (size_t) stb_cap);
    msg_buf[0] = (unsigned char *) calloc(1, (size_t) msg_cap);
    msg_buf[1] = (unsigned char *) calloc(1, (size_t) msg_cap);
    ssa = (unsigned char *) calloc(1, (size_t) ss_cap);
    ssb = (unsigned char *) calloc(1, (size_t) ss_cap);

    if (pka == NULL || ska == NULL || sta == NULL || pkb == NULL || skb == NULL || stb == NULL ||
        msg_buf[0] == NULL || msg_buf[1] == NULL || ssa == NULL || ssb == NULL) {
        ngcc_log_error("[kex][correctness] allocation failed: pk_cap=%llu sk_cap=%llu sta_cap=%llu stb_cap=%llu msg_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       sta_cap,
                       stb_cap,
                       msg_cap,
                       ss_cap);
        goto out;
    }

    if (kex_run_once(api,
                     pka,
                     ska,
                     sta,
                     pkb,
                     skb,
                     stb,
                     msg_buf[0],
                     msg_buf[1],
                     ssa,
                     ssb,
                     pk_cap,
                     sk_cap,
                     sta_cap,
                     stb_cap,
                     msg_cap,
                     ss_cap) != 0) {
        ngcc_log_error("[kex][correctness] KEX run failed: passes=%llu", api->kex_passes_num);
        goto out;
    }

    rc = 0;

out:
    free(pka);
    free(ska);
    free(sta);
    free(pkb);
    free(skb);
    free(stb);
    free(msg_buf[0]);
    free(msg_buf[1]);
    free(ssa);
    free(ssb);
    return rc;
}

static unsigned long long kex_field_to_u64(const ngcc_kat_field_t *f) {
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

static int kex_check_field_len(const char *field_name, const ngcc_kat_field_t *data_field,
                               const ngcc_kat_field_t *len_field) {
    unsigned long long expected;
    if (len_field == NULL || data_field == NULL) {
        return 0;
    }
    expected = kex_field_to_u64(len_field);
    if (expected == 0) {
        return 0;
    }
    if ((unsigned long long) data_field->len != expected) {
        ngcc_log_error("[kex][kat] %s length mismatch: expected_bytes=%llu actual_bytes=%zu",
                       field_name,
                       expected,
                       data_field->len);
        return -1;
    }
    return 0;
}

static int path_is_directory_kex(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int path_is_regular_file_kex(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}



static int kex_field_populated(const ngcc_kat_field_t *f) {
    return f != NULL && f->data != NULL && f->len > 0;
}

static int verify_kex_kat_vectors(const ngcc_api_t *api,
                                  const ngcc_kat_file_t *kat,
                                  unsigned long long *io_total,
                                  unsigned long long *io_passed,
                                  unsigned long long *io_failed) {
    unsigned long long pk_cap;
    unsigned long long sk_cap;
    unsigned long long sta_cap;
    unsigned long long stb_cap;
    unsigned long long msg_cap;
    unsigned long long ss_cap;
    unsigned char *ss_out = NULL;
    size_t i;

    pk_cap = api->kex_get_pk_len_bytes();
    sk_cap = api->kex_get_sk_len_bytes();
    sta_cap = api->kex_get_sta_len_bytes();
    stb_cap = api->kex_get_stb_len_bytes();
    msg_cap = api->kex_get_total_msg_len_bytes();
    ss_cap = api->kex_get_ss_len_bytes();
    if (!ngcc_is_valid_len(pk_cap) || !ngcc_is_valid_len(sk_cap) || !ngcc_is_valid_len(sta_cap) ||
        !ngcc_is_valid_len(stb_cap) || !ngcc_is_valid_len(msg_cap) || !ngcc_is_valid_len(ss_cap)) {
        ngcc_log_error("[kex][kat] invalid advertised caps: pk_cap=%llu sk_cap=%llu sta_cap=%llu stb_cap=%llu msg_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       sta_cap,
                       stb_cap,
                       msg_cap,
                       ss_cap);
        return -1;
    }

    ss_out = (unsigned char *) calloc(1, (size_t) ss_cap);
    if (ss_out == NULL) {
        ngcc_log_error("[kex][kat] allocation failed: ss_cap=%llu", ss_cap);
        return -1;
    }

    for (i = 0; i < kat->count; ++i) {
        const ngcc_kat_vector_t *vec = &kat->vectors[i];

        const ngcc_kat_field_t *ska = ngcc_kat_get_field(vec, "SKa");
        const ngcc_kat_field_t *pkb = ngcc_kat_get_field(vec, "PKb");
        const ngcc_kat_field_t *pka = ngcc_kat_get_field(vec, "PKa");
        const ngcc_kat_field_t *skb = ngcc_kat_get_field(vec, "SKb");
        const ngcc_kat_field_t *ss = ngcc_kat_get_field(vec, "SS");

        /* ma = last message from A (odd pass), mb = last message from B (even pass),
         * sta_field = last state of A, stb_field = last state of B.
         * Initialized to Init_Sta / Init_Stb as defaults for protocols that
         * derive SS directly from the initial state. */
        const ngcc_kat_field_t *ma_field = NULL;
        const ngcc_kat_field_t *mb_field = NULL;
        const ngcc_kat_field_t *sta_field = ngcc_kat_get_field(vec, "Init_Sta");
        const ngcc_kat_field_t *stb_field = ngcc_kat_get_field(vec, "Init_Stb");

        int has_a = 0;
        int has_b = 0;
        int case_failed = 0;
        int len_failed = 0;
        int pass;

        /* Skip vectors with empty SS (blank template) */
        if (ss == NULL || ss->data == NULL || ss->len == 0) {
            continue;
        }

        /* Validate Pass_Num consistency if present */
        {
            const ngcc_kat_field_t *pass_num_field = ngcc_kat_get_field(vec, "Pass_Num");
            if (pass_num_field != NULL && pass_num_field->data != NULL && pass_num_field->len > 0) {
                unsigned long long kat_passes = kex_field_to_u64(pass_num_field);
                if (kat_passes != 0 && kat_passes != api->kex_passes_num) {
                    ngcc_log_error("[kex][kat] Pass_Num mismatch: vector=%zu kat_passes=%llu library_passes=%llu",
                                   i,
                                   kat_passes,
                                   api->kex_passes_num);
                    (*io_total)++;
                    (*io_failed)++;
                    continue;
                }
            }
        }

        /* Scan M1..M{N} and corresponding Pass{N}_Sta / Pass{N}_Stb to
         * find the last message and state for each party.
         *   Odd passes (1,3,5...) → message from A, state update for Sta
         *   Even passes (2,4,6...) → message from B, state update for Stb
         * Also validate _Len consistency for each populated field. */
        for (pass = 1; pass <= (int) api->kex_passes_num; ++pass) {
            char m_name[16];
            char m_len_name[24];
            char st_name[24];
            char st_len_name[32];
            const ngcc_kat_field_t *msg;
            const ngcc_kat_field_t *state;

            snprintf(m_name, sizeof(m_name), "M%d", pass);
            msg = ngcc_kat_get_field(vec, m_name);
            if (!kex_field_populated(msg)) {
                break;  /* no more passes */
            }

            /* validate M{n}_Len */
            snprintf(m_len_name, sizeof(m_len_name), "M%d_Len", pass);
            if (kex_check_field_len(m_name, msg, ngcc_kat_get_field(vec, m_len_name)) != 0) {
                len_failed = 1;
                break;
            }

            if (pass % 2 == 1) {
                /* Odd pass: message from A */
                ma_field = msg;
                snprintf(st_name, sizeof(st_name), "Pass%d_Sta", pass);
                state = ngcc_kat_get_field(vec, st_name);
                if (kex_field_populated(state)) {
                    sta_field = state;
                    snprintf(st_len_name, sizeof(st_len_name), "Pass%d_Sta_Len", pass);
                    if (kex_check_field_len(st_name, state, ngcc_kat_get_field(vec, st_len_name)) != 0) {
                        len_failed = 1;
                        break;
                    }
                }
            } else {
                /* Even pass: message from B */
                mb_field = msg;
                snprintf(st_name, sizeof(st_name), "Pass%d_Stb", pass);
                state = ngcc_kat_get_field(vec, st_name);
                if (kex_field_populated(state)) {
                    stb_field = state;
                    snprintf(st_len_name, sizeof(st_len_name), "Pass%d_Stb_Len", pass);
                    if (kex_check_field_len(st_name, state, ngcc_kat_get_field(vec, st_len_name)) != 0) {
                        len_failed = 1;
                        break;
                    }
                }
            }
        }

        (*io_total)++;

        if (len_failed) {
            (*io_failed)++;
            continue;
        }

        /* Validate Init_Sta_Len / Init_Stb_Len if present */
        if (kex_field_populated(ngcc_kat_get_field(vec, "Init_Sta"))) {
            if (kex_check_field_len("Init_Sta",
                                    ngcc_kat_get_field(vec, "Init_Sta"),
                                    ngcc_kat_get_field(vec, "Init_Sta_Len")) != 0) {
                (*io_failed)++;
                continue;
            }
        }
        if (kex_field_populated(ngcc_kat_get_field(vec, "Init_Stb"))) {
            if (kex_check_field_len("Init_Stb",
                                    ngcc_kat_get_field(vec, "Init_Stb"),
                                    ngcc_kat_get_field(vec, "Init_Stb_Len")) != 0) {
                (*io_failed)++;
                continue;
            }
        }

        /* Validate key and SS length fields */
        if (kex_check_field_len("SKa", ska, ngcc_kat_get_field(vec, "SKa_Len")) != 0 ||
            kex_check_field_len("PKb", pkb, ngcc_kat_get_field(vec, "PKb_Len")) != 0 ||
            kex_check_field_len("SKb", skb, ngcc_kat_get_field(vec, "SKb_Len")) != 0 ||
            kex_check_field_len("PKa", pka, ngcc_kat_get_field(vec, "PKa_Len")) != 0 ||
            kex_check_field_len("SS", ss, ngcc_kat_get_field(vec, "SS_Len")) != 0) {
            (*io_failed)++;
            continue;
        }

        /* Check if we can verify side A: needs ska, pkb, mb (last B msg), sta */
        has_a = (kex_field_populated(ska) && kex_field_populated(pkb) &&
                 kex_field_populated(mb_field) && kex_field_populated(sta_field));
        /* Check if we can verify side B: needs skb, pka, ma (last A msg), stb */
        has_b = (kex_field_populated(skb) && kex_field_populated(pka) &&
                 kex_field_populated(ma_field) && kex_field_populated(stb_field));

        if (!has_a && !has_b) {
            (*io_total)--; /* undo: not a testable vector */
            continue;
        }

        if (has_a) {
            if (ska->len > sk_cap || pkb->len > pk_cap ||
                mb_field->len > msg_cap || sta_field->len > sta_cap ||
                ss->len > ss_cap) {
                ngcc_log_error("[kex][kat] A-side vector exceeds caps: vector=%zu ska_len=%zu sk_cap=%llu pkb_len=%zu pk_cap=%llu mb_len=%zu msg_cap=%llu sta_len=%zu sta_cap=%llu ss_len=%zu ss_cap=%llu",
                               i,
                               ska->len,
                               sk_cap,
                               pkb->len,
                               pk_cap,
                               mb_field->len,
                               msg_cap,
                               sta_field->len,
                               sta_cap,
                               ss->len,
                               ss_cap);
                case_failed = 1;
            } else if (kex_derive_matches_expected(api->kex_derive_ss_a,
                                                   "A-side",
                                                   i,
                                                   (unsigned char *) ska->data,
                                                   ska->len,
                                                   (unsigned char *) pkb->data,
                                                   pkb->len,
                                                   (unsigned char *) mb_field->data,
                                                   mb_field->len,
                                                   (unsigned char *) sta_field->data,
                                                   sta_field->len,
                                                   ss_out,
                                                   ss_cap,
                                                   ss,
                                                   0xA5) != 0) {
                case_failed = 1;
            }
        }

        if (!case_failed && has_b) {
            if (skb->len > sk_cap || pka->len > pk_cap ||
                ma_field->len > msg_cap || stb_field->len > stb_cap ||
                ss->len > ss_cap) {
                ngcc_log_error("[kex][kat] B-side vector exceeds caps: vector=%zu skb_len=%zu sk_cap=%llu pka_len=%zu pk_cap=%llu ma_len=%zu msg_cap=%llu stb_len=%zu stb_cap=%llu ss_len=%zu ss_cap=%llu",
                               i,
                               skb->len,
                               sk_cap,
                               pka->len,
                               pk_cap,
                               ma_field->len,
                               msg_cap,
                               stb_field->len,
                               stb_cap,
                               ss->len,
                               ss_cap);
                case_failed = 1;
            } else if (kex_derive_matches_expected(api->kex_derive_ss_b,
                                                   "B-side",
                                                   i,
                                                   (unsigned char *) skb->data,
                                                   skb->len,
                                                   (unsigned char *) pka->data,
                                                   pka->len,
                                                   (unsigned char *) ma_field->data,
                                                   ma_field->len,
                                                   (unsigned char *) stb_field->data,
                                                   stb_field->len,
                                                   ss_out,
                                                   ss_cap,
                                                   ss,
                                                   0xA5) != 0) {
                case_failed = 1;
            }
        }

        if (case_failed) {
            (*io_failed)++;
        } else {
            (*io_passed)++;
        }
    }

    free(ss_out);
    return 0;
}

static int verify_kex_kat_one_file(const ngcc_api_t *api,
                                   const char *file_path,
                                   unsigned long long *io_total,
                                   unsigned long long *io_passed,
                                   unsigned long long *io_failed) {
    ngcc_kat_file_t kat;
    int rc;

    memset(&kat, 0, sizeof(kat));
    if (ngcc_kat_parse_file(file_path, &kat) != 0) {
        ngcc_log_error("[kex][kat] failed to parse file: %s", file_path);
        return -2;
    }

    rc = verify_kex_kat_vectors(api, &kat, io_total, io_passed, io_failed);
    ngcc_kat_free(&kat);
    return rc;
}

int ngcc_kex_correctness_kat_file(const ngcc_api_t *api,
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
        ngcc_log_error("[kex][kat] invalid arguments: api_null=%d kat_path_null=%d",
                       api == NULL,
                       kat_path == NULL);
        return -1;
    }
    if (!kex_has_supported_pass_count(api)) {
        ngcc_log_error("[kex][kat] unsupported pass count: passes=%llu min=%llu max=%llu",
                       api->kex_passes_num,
                       NGCC_KEX_MIN_PASSES,
                       NGCC_KEX_MAX_PASSES);
        return -1;
    }

    if (path_is_regular_file_kex(kat_path)) {
        int vrc = verify_kex_kat_one_file(api, kat_path, &total, &passed, &failed);

        file_count = 1;
        rc = (vrc == 0 && total > 0 && failed == 0) ? 0 : -1;
        if (rc != 0) {
            ngcc_log_error("[kex][kat] verification failed: total=%llu passed=%llu failed=%llu file=%s",
                           total,
                           passed,
                           failed,
                           kat_path);
        }
        goto done;
    }

    if (!path_is_directory_kex(kat_path)) {
        ngcc_log_error("[kex][kat] --kat path is not a file or directory: %s", kat_path);
        return -1;
    }

    dir = opendir(kat_path);
    if (dir == NULL) {
        ngcc_log_error("[kex][kat] cannot open directory: %s", kat_path);
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
            ngcc_log_error("[kex][kat] KAT file path too long: dir=%s file=%s", kat_path, entry->d_name);
            continue;
        }
        if (path_is_directory_kex(file_path)) {
            continue;
        }

        if (strncmp(entry->d_name, "KAT_KEX_", 8) != 0) {
            continue;  /* skip files not matching KAT_KEX_ prefix */
        }

        vrc = verify_kex_kat_one_file(api, file_path, &total, &passed, &failed);
        if (vrc == -2) {
            closedir(dir);
            goto done;
        }

        file_count++;
    }
    closedir(dir);

    if (file_count == 0) {
        ngcc_log_error("[kex][kat] no KAT_KEX_ files found in: %s", kat_path);
        goto done;
    }

    rc = (total > 0 && failed == 0) ? 0 : -1;
    if (rc != 0) {
        ngcc_log_error("[kex][kat] verification failed: total=%llu passed=%llu failed=%llu dir=%s",
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

/* ── KEX derive_ss performance ─────────────────────────────────── */

typedef struct {
    const ngcc_api_t *api;
    /* Pre-computed protocol state (from running the full protocol once as setup) */
    unsigned char *ska;  unsigned long long ska_len;
    unsigned char *pkb;  unsigned long long pkb_len;
    unsigned char *skb;  unsigned long long skb_len;
    unsigned char *pka;  unsigned long long pka_len;
    unsigned char *sta;  unsigned long long sta_len;
    unsigned char *stb;  unsigned long long stb_len;
    unsigned char *ma;   unsigned long long ma_len;   /* last A-msg */
    unsigned char *mb;   unsigned long long mb_len;   /* last B-msg */
    unsigned char *ss;   unsigned long long ss_cap;   /* output buffer */
} kex_derive_ctx_t;

static int kex_derive_ss_a_op(void *ctx_ptr) {
    kex_derive_ctx_t *c = (kex_derive_ctx_t *) ctx_ptr;
    unsigned long long ss_len = c->ss_cap;
    if (c->api->kex_derive_ss_a(c->ska, c->ska_len, c->pkb, c->pkb_len,
                                c->mb, c->mb_len, c->sta, c->sta_len,
                                c->ss, &ss_len) != 0) {
        ngcc_log_error("[kex][performance][derive_a] kex_derive_ss_a failed: ska_len=%llu pkb_len=%llu mb_len=%llu sta_len=%llu ss_cap=%llu",
                       c->ska_len,
                       c->pkb_len,
                       c->mb_len,
                       c->sta_len,
                       c->ss_cap);
        return -1;
    }
    if (!kex_is_valid_output_len(ss_len, c->ss_cap)) {
        ngcc_log_error("[kex][performance][derive_a] invalid shared-secret length: ss_len=%llu ss_cap=%llu",
                       ss_len,
                       c->ss_cap);
        return -1;
    }
    return 0;
}

static int kex_derive_ss_b_op(void *ctx_ptr) {
    kex_derive_ctx_t *c = (kex_derive_ctx_t *) ctx_ptr;
    unsigned long long ss_len = c->ss_cap;
    if (c->api->kex_derive_ss_b(c->skb, c->skb_len, c->pka, c->pka_len,
                                c->ma, c->ma_len, c->stb, c->stb_len,
                                c->ss, &ss_len) != 0) {
        ngcc_log_error("[kex][performance][derive_b] kex_derive_ss_b failed: skb_len=%llu pka_len=%llu ma_len=%llu stb_len=%llu ss_cap=%llu",
                       c->skb_len,
                       c->pka_len,
                       c->ma_len,
                       c->stb_len,
                       c->ss_cap);
        return -1;
    }
    if (!kex_is_valid_output_len(ss_len, c->ss_cap)) {
        ngcc_log_error("[kex][performance][derive_b] invalid shared-secret length: ss_len=%llu ss_cap=%llu",
                       ss_len,
                       c->ss_cap);
        return -1;
    }
    return 0;
}

/* Run the full KEX protocol once to obtain intermediate state for derive_ss,
 * then benchmark derive_ss_a and derive_ss_b separately. */
int ngcc_kex_derive_ss_performance(const ngcc_api_t *api,
                                   const ngcc_perf_config_t *cfg,
                                   ngcc_perf_result_t *out_a,
                                   ngcc_perf_result_t *out_b) {
    unsigned long long pk_cap, sk_cap, sta_cap, stb_cap, msg_cap, ss_cap;
    unsigned char *pka = NULL, *ska = NULL, *sta = NULL;
    unsigned char *pkb = NULL, *skb = NULL, *stb = NULL;
    unsigned char *msg_buf[2] = {NULL, NULL};
    unsigned char *ssa = NULL, *ssb = NULL;
    unsigned long long pka_len, ska_len, sta_len, pkb_len, skb_len, stb_len;
    unsigned char *ma = NULL, *mb = NULL;
    unsigned long long ma_len = 0, mb_len = 0;
    unsigned long long passes;
    ngcc_perf_config_t local_cfg;
    int rc = -1;

    if (api == NULL || cfg == NULL || out_a == NULL || out_b == NULL) {
        ngcc_log_error("[kex][performance] invalid arguments: api_null=%d cfg_null=%d out_a_null=%d out_b_null=%d",
                       api == NULL,
                       cfg == NULL,
                       out_a == NULL,
                       out_b == NULL);
        return -1;
    }
    if (!kex_has_supported_pass_count(api)) {
        ngcc_log_error("[kex][performance] unsupported pass count: passes=%llu min=%llu max=%llu",
                       api->kex_passes_num,
                       NGCC_KEX_MIN_PASSES,
                       NGCC_KEX_MAX_PASSES);
        return -1;
    }

    pk_cap = api->kex_get_pk_len_bytes();
    sk_cap = api->kex_get_sk_len_bytes();
    sta_cap = api->kex_get_sta_len_bytes();
    stb_cap = api->kex_get_stb_len_bytes();
    msg_cap = api->kex_get_total_msg_len_bytes();
    ss_cap = api->kex_get_ss_len_bytes();
    passes = api->kex_passes_num;

    if (!ngcc_is_valid_len(pk_cap) || !ngcc_is_valid_len(sk_cap) || !ngcc_is_valid_len(sta_cap) ||
        !ngcc_is_valid_len(stb_cap) || !ngcc_is_valid_len(msg_cap) || !ngcc_is_valid_len(ss_cap)) {
        ngcc_log_error("[kex][performance] invalid advertised caps: pk_cap=%llu sk_cap=%llu sta_cap=%llu stb_cap=%llu msg_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       sta_cap,
                       stb_cap,
                       msg_cap,
                       ss_cap);
        return -1;
    }

    pka = (unsigned char *) calloc(1, (size_t) pk_cap);
    ska = (unsigned char *) calloc(1, (size_t) sk_cap);
    sta = (unsigned char *) calloc(1, (size_t) sta_cap);
    pkb = (unsigned char *) calloc(1, (size_t) pk_cap);
    skb = (unsigned char *) calloc(1, (size_t) sk_cap);
    stb = (unsigned char *) calloc(1, (size_t) stb_cap);
    msg_buf[0] = (unsigned char *) calloc(1, (size_t) msg_cap);
    msg_buf[1] = (unsigned char *) calloc(1, (size_t) msg_cap);
    ssa = (unsigned char *) calloc(1, (size_t) ss_cap);
    ssb = (unsigned char *) calloc(1, (size_t) ss_cap);
    if (pka == NULL || ska == NULL || sta == NULL || pkb == NULL || skb == NULL ||
        stb == NULL || msg_buf[0] == NULL || msg_buf[1] == NULL || ssa == NULL || ssb == NULL) {
        ngcc_log_error("[kex][performance] allocation failed: pk_cap=%llu sk_cap=%llu sta_cap=%llu stb_cap=%llu msg_cap=%llu ss_cap=%llu",
                       pk_cap,
                       sk_cap,
                       sta_cap,
                       stb_cap,
                       msg_cap,
                       ss_cap);
        goto cleanup;
    }

    /* ── Setup: run the full protocol once to get intermediate state ── */
    {
        unsigned long long m_cur_len, m_prev_len;
        unsigned char *m_cur, *m_prev;
        unsigned long long p;
        int pass_rc;

        pka_len = pk_cap; ska_len = sk_cap; sta_len = sta_cap;
        pkb_len = pk_cap; skb_len = sk_cap; stb_len = stb_cap;

        if (api->kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len) != 0) {
            ngcc_log_error("[kex][performance] setup kex_init_a failed");
            goto cleanup;
        }
        if (api->kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len) != 0) {
            ngcc_log_error("[kex][performance] setup kex_init_b failed");
            goto cleanup;
        }
        if (!kex_is_valid_output_len(pka_len, pk_cap) ||
            !kex_is_valid_output_len(pkb_len, pk_cap) ||
            !kex_is_valid_output_len(ska_len, sk_cap) ||
            !kex_is_valid_output_len(skb_len, sk_cap) ||
            !kex_is_valid_output_len(sta_len, sta_cap) ||
            !kex_is_valid_output_len(stb_len, stb_cap)) {
            ngcc_log_error("[kex][performance] setup init returned invalid lengths: pka_len=%llu pkb_len=%llu pk_cap=%llu ska_len=%llu skb_len=%llu sk_cap=%llu sta_len=%llu sta_cap=%llu stb_len=%llu stb_cap=%llu",
                           pka_len,
                           pkb_len,
                           pk_cap,
                           ska_len,
                           skb_len,
                           sk_cap,
                           sta_len,
                           sta_cap,
                           stb_len,
                           stb_cap);
            goto cleanup;
        }

        m_cur = msg_buf[0];
        m_cur_len = msg_cap;
        pass_rc = api->kex_pass1_fn(ska, ska_len, pkb, pkb_len, sta, &sta_len, m_cur, &m_cur_len);
        if (pass_rc < 0) {
            ngcc_log_error("[kex][performance] setup pass1 failed: rc=%d", pass_rc);
            goto cleanup;
        }
        if (!kex_is_valid_output_len(m_cur_len, msg_cap) ||
            !kex_is_valid_output_len(sta_len, sta_cap)) {
            ngcc_log_error("[kex][performance] setup pass1 returned invalid lengths: msg_len=%llu msg_cap=%llu sta_len=%llu sta_cap=%llu",
                           m_cur_len,
                           msg_cap,
                           sta_len,
                           sta_cap);
            goto cleanup;
        }
        ma = m_cur; ma_len = m_cur_len;

        for (p = 2; p <= passes && pass_rc == 0; ++p) {
            kex_pass_fn_t fn = api->kex_pass_fns[p - 2];
            m_prev = m_cur; m_prev_len = m_cur_len;
            m_cur = (m_prev == msg_buf[0]) ? msg_buf[1] : msg_buf[0];
            m_cur_len = msg_cap;
            if (p % 2 == 0) {
                pass_rc = fn(skb, skb_len, pka, pka_len, m_prev, m_prev_len,
                             stb, &stb_len, m_cur, &m_cur_len);
                if (pass_rc < 0) {
                    ngcc_log_error("[kex][performance] setup pass%llu failed on B side: rc=%d prev_msg_len=%llu",
                                   p,
                                   pass_rc,
                                   m_prev_len);
                    goto cleanup;
                }
                if (!kex_is_valid_output_len(m_cur_len, msg_cap) ||
                    !kex_is_valid_output_len(stb_len, stb_cap)) {
                    ngcc_log_error("[kex][performance] setup pass%llu returned invalid B-side lengths: msg_len=%llu msg_cap=%llu stb_len=%llu stb_cap=%llu",
                                   p,
                                   m_cur_len,
                                   msg_cap,
                                   stb_len,
                                   stb_cap);
                    goto cleanup;
                }
                mb = m_cur; mb_len = m_cur_len;
            } else {
                pass_rc = fn(ska, ska_len, pkb, pkb_len, m_prev, m_prev_len,
                             sta, &sta_len, m_cur, &m_cur_len);
                if (pass_rc < 0) {
                    ngcc_log_error("[kex][performance] setup pass%llu failed on A side: rc=%d prev_msg_len=%llu",
                                   p,
                                   pass_rc,
                                   m_prev_len);
                    goto cleanup;
                }
                if (!kex_is_valid_output_len(m_cur_len, msg_cap) ||
                    !kex_is_valid_output_len(sta_len, sta_cap)) {
                    ngcc_log_error("[kex][performance] setup pass%llu returned invalid A-side lengths: msg_len=%llu msg_cap=%llu sta_len=%llu sta_cap=%llu",
                                   p,
                                   m_cur_len,
                                   msg_cap,
                                   sta_len,
                                   sta_cap);
                    goto cleanup;
                }
                ma = m_cur; ma_len = m_cur_len;
            }
        }
        if (ma == NULL || mb == NULL ||
            !kex_is_valid_output_len(ma_len, msg_cap) ||
            !kex_is_valid_output_len(mb_len, msg_cap)) {
            ngcc_log_error("[kex][performance] setup missing final messages or invalid lengths: ma_null=%d mb_null=%d ma_len=%llu mb_len=%llu msg_cap=%llu passes=%llu",
                           ma == NULL,
                           mb == NULL,
                           ma_len,
                           mb_len,
                           msg_cap,
                           passes);
            goto cleanup;
        }
    }

    local_cfg = *cfg;
    local_cfg.bytes_per_op = ss_cap;

    /* ── Benchmark derive_ss_a ── */
    {
        kex_derive_ctx_t ctx_a;
        memset(&ctx_a, 0, sizeof(ctx_a));
        ctx_a.api = api;
        ctx_a.ska = ska;  ctx_a.ska_len = ska_len;
        ctx_a.pkb = pkb;  ctx_a.pkb_len = pkb_len;
        ctx_a.sta = sta;  ctx_a.sta_len = sta_len;
        ctx_a.mb  = mb;   ctx_a.mb_len  = mb_len;
        ctx_a.ss  = ssa;  ctx_a.ss_cap  = ss_cap;
        if (ngcc_run_performance_op(&local_cfg, kex_derive_ss_a_op, &ctx_a, out_a) != 0) {
            ngcc_log_error("[kex][performance][derive_a] benchmark failed: iterations=%llu mb_len=%llu ss_cap=%llu",
                           cfg->iterations,
                           mb_len,
                           ss_cap);
            goto cleanup;
        }
    }

    /* ── Benchmark derive_ss_b ── */
    {
        kex_derive_ctx_t ctx_b;
        memset(&ctx_b, 0, sizeof(ctx_b));
        ctx_b.api = api;
        ctx_b.skb = skb;  ctx_b.skb_len = skb_len;
        ctx_b.pka = pka;  ctx_b.pka_len = pka_len;
        ctx_b.stb = stb;  ctx_b.stb_len = stb_len;
        ctx_b.ma  = ma;   ctx_b.ma_len  = ma_len;
        ctx_b.ss  = ssb;  ctx_b.ss_cap  = ss_cap;
        if (ngcc_run_performance_op(&local_cfg, kex_derive_ss_b_op, &ctx_b, out_b) != 0) {
            ngcc_log_error("[kex][performance][derive_b] benchmark failed: iterations=%llu ma_len=%llu ss_cap=%llu",
                           cfg->iterations,
                           ma_len,
                           ss_cap);
            goto cleanup;
        }
    }

    rc = 0;

cleanup:
    free(pka); free(ska); free(sta);
    free(pkb); free(skb); free(stb);
    free(msg_buf[0]); free(msg_buf[1]);
    free(ssa); free(ssb);
    return rc;
}
