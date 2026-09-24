#include "loader.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TEST_MASK_* constants live in cli_types.h */
#include "cli_types.h"
#include "ngcc_log.h"

static int load_symbol(void *handle, const char *name, void **fn_ptr) {
    dlerror();
    *fn_ptr = dlsym(handle, name);
    if (dlerror() != NULL || *fn_ptr == NULL) {
        ngcc_log_error("[loader] missing symbol: %s", name);
        return -1;
    }
    return 0;
}

/* ── Per-group loaders ──────────────────────────────────────────── */

static int load_hash_symbols(void *handle, ngcc_api_t *api) {
    if (load_symbol(handle, "CryptHash", (void **) &api->CryptHash) != 0) {
        return -1;
    }
    return 0;
}

static int load_sig_symbols(void *handle, ngcc_api_t *api) {
    if (load_symbol(handle, "sig_get_pk_len_bytes", (void **) &api->sig_get_pk_len_bytes) != 0 ||
        load_symbol(handle, "sig_get_sk_len_bytes", (void **) &api->sig_get_sk_len_bytes) != 0 ||
        load_symbol(handle, "sig_get_sn_len_bytes", (void **) &api->sig_get_sn_len_bytes) != 0 ||
        load_symbol(handle, "sig_keygen", (void **) &api->sig_keygen) != 0 ||
        load_symbol(handle, "sig_sign", (void **) &api->sig_sign) != 0 ||
        load_symbol(handle, "sig_verify", (void **) &api->sig_verify) != 0) {
        return -1;
    }
    return 0;
}

static int load_kem_symbols(void *handle, ngcc_api_t *api) {
    if (load_symbol(handle, "kem_get_pk_len_bytes", (void **) &api->kem_get_pk_len_bytes) != 0 ||
        load_symbol(handle, "kem_get_sk_len_bytes", (void **) &api->kem_get_sk_len_bytes) != 0 ||
        load_symbol(handle, "kem_get_ss_len_bytes", (void **) &api->kem_get_ss_len_bytes) != 0 ||
        load_symbol(handle, "kem_get_ct_len_bytes", (void **) &api->kem_get_ct_len_bytes) != 0 ||
        load_symbol(handle, "kem_keygen", (void **) &api->kem_keygen) != 0 ||
        load_symbol(handle, "kem_enc", (void **) &api->kem_enc) != 0 ||
        load_symbol(handle, "kem_dec", (void **) &api->kem_dec) != 0) {
        return -1;
    }
    return 0;
}

static int load_kex_symbols(void *handle, ngcc_api_t *api) {
    unsigned long long passes;
    unsigned long long i;

    if (load_symbol(handle, "kex_get_passes_num", (void **) &api->kex_get_passes_num) != 0 ||
        load_symbol(handle, "kex_get_pk_len_bytes", (void **) &api->kex_get_pk_len_bytes) != 0 ||
        load_symbol(handle, "kex_get_sk_len_bytes", (void **) &api->kex_get_sk_len_bytes) != 0 ||
        load_symbol(handle, "kex_get_sta_len_bytes", (void **) &api->kex_get_sta_len_bytes) != 0 ||
        load_symbol(handle, "kex_get_stb_len_bytes", (void **) &api->kex_get_stb_len_bytes) != 0 ||
        load_symbol(handle, "kex_get_ss_len_bytes", (void **) &api->kex_get_ss_len_bytes) != 0 ||
        load_symbol(handle, "kex_get_total_msg_len_bytes", (void **) &api->kex_get_total_msg_len_bytes) != 0 ||
        load_symbol(handle, "kex_init_a", (void **) &api->kex_init_a) != 0 ||
        load_symbol(handle, "kex_init_b", (void **) &api->kex_init_b) != 0 ||
        load_symbol(handle, "kex_derive_ss_a", (void **) &api->kex_derive_ss_a) != 0 ||
        load_symbol(handle, "kex_derive_ss_b", (void **) &api->kex_derive_ss_b) != 0) {
        return -1;
    }

    passes = api->kex_get_passes_num();
    if (passes < NGCC_KEX_MIN_PASSES || passes > NGCC_KEX_MAX_PASSES) {
        ngcc_log_error("[loader] kex_get_passes_num returned invalid value: %llu "
                       "(expected %llu..%llu)",
                       passes,
                       NGCC_KEX_MIN_PASSES,
                       NGCC_KEX_MAX_PASSES);
        return -1;
    }

    api->kex_passes_num = passes;

    /* Load pass1 (always A-side, 8 params) */
    if (load_symbol(handle, "kex_generate_pass1_msg_a", (void **) &api->kex_pass1_fn) != 0) {
        return -1;
    }

    /* Load pass2..N (10 params each) */
    if (passes > 1) {
        api->kex_pass_fns = (kex_pass_fn_t *) calloc((size_t) (passes - 1), sizeof(kex_pass_fn_t));
        if (api->kex_pass_fns == NULL) {
            ngcc_log_error("[loader] failed to allocate KEX pass function table for %llu passes", passes);
            return -1;
        }
        for (i = 2; i <= passes; ++i) {
            char sym_name[64];
            const char *side = (i % 2 == 1) ? "a" : "b";  /* pass 2,4,6..=b; 3,5,7..=a */
            snprintf(sym_name, sizeof(sym_name), "kex_generate_pass%llu_msg_%s", i, side);
            if (load_symbol(handle, sym_name, (void **) &api->kex_pass_fns[i - 2]) != 0) {
                free(api->kex_pass_fns);
                api->kex_pass_fns = NULL;
                return -1;
            }
        }
    }

    return 0;
}

/* ── Public API ─────────────────────────────────────────────────── */

int ngcc_load_library(const char *lib_path, unsigned int test_mask,
                      ngcc_library_t *out_lib) {
    void *handle;
    ngcc_api_t *api;

    if (lib_path == NULL || out_lib == NULL) {
        return -1;
    }

    memset(out_lib, 0, sizeof(*out_lib));

    handle = dlopen(lib_path, RTLD_NOW);
    if (handle == NULL) {
        ngcc_log_error("[loader] dlopen failed for %s: %s", lib_path, dlerror());
        return -1;
    }

    out_lib->handle = handle;
    api = &out_lib->api;

    if ((test_mask & TEST_MASK_HASH) && load_hash_symbols(handle, api) != 0) {
        ngcc_unload_library(out_lib);
        return -1;
    }
    if ((test_mask & TEST_MASK_SIG) &&
        load_sig_symbols(handle, api) != 0) {
        ngcc_unload_library(out_lib);
        return -1;
    }
    if ((test_mask & TEST_MASK_KEM) &&
        load_kem_symbols(handle, api) != 0) {
        ngcc_unload_library(out_lib);
        return -1;
    }
    if ((test_mask & TEST_MASK_KEX) && load_kex_symbols(handle, api) != 0) {
        ngcc_unload_library(out_lib);
        return -1;
    }

    return 0;
}

void ngcc_unload_library(ngcc_library_t *lib) {
    if (lib == NULL) {
        return;
    }

    free(lib->api.kex_pass_fns);

    if (lib->handle != NULL) {
        dlclose(lib->handle);
    }

    memset(lib, 0, sizeof(*lib));
}
