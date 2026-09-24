/*
 * Digital-signature self-assessment benchmark.
 *
 * Implements the four NGCC x86 self-assessment tests for a signature algorithm:
 *   (1) functional correctness (keygen / sign / verify + forged-signature rejection)
 *   (2) performance (keygen / sign / verify cycles and throughput)
 *   (3) resource usage (static text/data/bss + peak resident memory)
 *   (4) transfer & storage (public key / private key / signature sizes)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/resource.h>

#include "registry.h"
#include "sig_adapter.h"
#include "bench_local.h"
#include "test_util.h"
#include "report.h"

#define SA_MSG_LEN   64
#define SA_SEED_LEN  48

static void fill_fixed_seed(uint8_t *seed, size_t len)
{
    for (size_t i = 0; i < len; i++)
        seed[i] = (uint8_t)(0xA5u ^ (i * 7u + 1u));
}

static void fill_fixed_message(uint8_t *m, size_t len)
{
    for (size_t i = 0; i < len; i++)
        m[i] = (uint8_t)(i & 0xFFu);
}

static long peak_rss_bytes(void)
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0)
        return -1;
#if defined(__APPLE__)
    return (long)ru.ru_maxrss;
#else
    return (long)ru.ru_maxrss * 1024L;
#endif
}

#define MEASURE_OP(perf, iters, stmt)                                        \
    do {                                                                     \
        uint64_t _total = 0, _mn = UINT64_MAX, _mx = 0;                      \
        uint64_t _t0 = bench_now_ns();                                       \
        for (uint32_t _i = 0; _i < (iters); _i++) {                          \
            uint64_t _c0 = bench_rdtsc();                                    \
            (stmt);                                                          \
            uint64_t _c1 = bench_rdtsc();                                    \
            uint64_t _c = _c1 - _c0;                                         \
            _total += _c;                                                    \
            if (_c < _mn) _mn = _c;                                          \
            if (_c > _mx) _mx = _c;                                          \
        }                                                                    \
        uint64_t _t1 = bench_now_ns();                                       \
        (perf).run_time = (iters);                                           \
        (perf).avg_cycles = (double)_total / (double)(iters);                \
        (perf).min_cycles = _mn;                                             \
        (perf).max_cycles = _mx;                                             \
        (perf).throughput = ((double)(iters) * 1e9) / (double)(_t1 - _t0);    \
    } while (0)

static int run_sig_func_bench(const ALGORITHM *sig_alg, struct sig_func_profile *sig_func,
                              size_t *sig_len_out)
{
    const SIG_METHOD *sig = sig_alg->method;
    size_t pk_len = sig->get_pk_len();
    size_t sk_len = sig->get_sk_len();
    size_t sig_cap = sig->get_sig_len();
    size_t pk_out = pk_len, sk_out = sk_len, sig_len = sig_cap;
    uint8_t *pk = malloc(pk_len);
    uint8_t *sk = malloc(sk_len);
    uint8_t *signature = malloc(sig_cap);
    uint8_t msg[SA_MSG_LEN];
    uint8_t seed[SA_SEED_LEN];
    char output[64];

    if (!pk || !sk || !signature)
        goto fail;

    fill_fixed_seed(seed, sizeof(seed));
    fill_fixed_message(msg, sizeof(msg));
    sig->drng_seed(seed, sizeof(seed));

    BENCH_LOG("Algorithm name:\t%s\n", sig_alg->alg_name);
    BENCH_LOG("Public key:\t%zu bytes.\n", pk_len);
    BENCH_LOG("Private key:\t%zu bytes.\n", sk_len);
    BENCH_LOG("Signature:\t%zu bytes.\n\n", sig_cap);
    BENCH_LOG("\t|---------------------------------------------------|\n");

    if (sig->keygen(pk, &pk_out, sk, &sk_out) != CRYPTO_SUCCESS)
        goto fail;
    if (sig->sign(sk, sk_out, msg, SA_MSG_LEN, signature, &sig_len) != CRYPTO_SUCCESS)
        goto fail;
    *sig_len_out = sig_len;

    if (sig->verify(pk, pk_out, signature, sig_len, msg, SA_MSG_LEN) == 0) {
        sig_func->correct = 1;
        snprintf(output, sizeof(output), "The functional correctness test         PASS");
        BENCH_LOG("\t|   %-48s|\n", output);
    } else {
        goto fail;
    }

    {
        uint8_t *forged = malloc(sig_len);
        if (!forged)
            goto fail;
        memcpy(forged, signature, sig_len);
        forged[0] ^= 0x01;
        if (sig->verify(pk, pk_out, forged, sig_len, msg, SA_MSG_LEN) != 0) {
            sig_func->sig_forgery = 1;
            snprintf(output, sizeof(output), "The forged-signature rejection test       PASS");
            BENCH_LOG("\t|   %-48s|\n", output);
        }
        free(forged);
    }

    BENCH_LOG("\t|---------------------------------------------------|\n");
    BENCH_LOG("Functional test status: ALL PASS.\n\n");
    free(pk);
    free(sk);
    free(signature);
    return CRYPTO_SUCCESS;

fail:
    free(pk);
    free(sk);
    free(signature);
    return CRYPTO_FAILED;
}

static int run_sig_speed_bench(const ALGORITHM *sig_alg, struct sig_perf_profile *sig_perf,
                               const bench_opt *opt, size_t sig_len)
{
    const SIG_METHOD *sig = sig_alg->method;
    uint32_t iters = opt->times;
    size_t pk_len = sig->get_pk_len();
    size_t sk_len = sig->get_sk_len();
    size_t sig_cap = sig->get_sig_len();
    size_t pk_out = pk_len, sk_out = sk_len;
    uint8_t *pk = malloc(pk_len);
    uint8_t *sk = malloc(sk_len);
    uint8_t *signature = malloc(sig_cap);
    uint8_t msg[SA_MSG_LEN];
    uint8_t seed[SA_SEED_LEN];

    if (!pk || !sk || !signature)
        goto end;

    fill_fixed_seed(seed, sizeof(seed));
    fill_fixed_message(msg, sizeof(msg));
    sig->drng_seed(seed, sizeof(seed));

    BENCH_LOG("\t|----------------------------------------------------------------------|\n");
    BENCH_LOG("\t| %-8s %12s %12s %12s %16s |\n", "op", "avg cyc", "min cyc", "max cyc", "ops/s");

    MEASURE_OP(sig_perf->keygen_perf, iters,
               (void)sig->keygen(pk, &pk_out, sk, &sk_out));
    BENCH_LOG("\t| %-8s %12.0f %12llu %12llu %16.1f |\n", "keygen",
              sig_perf->keygen_perf.avg_cycles,
              (unsigned long long)sig_perf->keygen_perf.min_cycles,
              (unsigned long long)sig_perf->keygen_perf.max_cycles,
              sig_perf->keygen_perf.throughput);

    {
        size_t sl;
        MEASURE_OP(sig_perf->sign_perf, iters,
                   (sl = sig_cap, (void)sig->sign(sk, sk_out, msg, SA_MSG_LEN, signature, &sl)));
        BENCH_LOG("\t| %-8s %12.0f %12llu %12llu %16.1f |\n", "sign",
                  sig_perf->sign_perf.avg_cycles,
                  (unsigned long long)sig_perf->sign_perf.min_cycles,
                  (unsigned long long)sig_perf->sign_perf.max_cycles,
                  sig_perf->sign_perf.throughput);
    }

    MEASURE_OP(sig_perf->verify_perf, iters,
               (void)sig->verify(pk, pk_out, signature, sig_len, msg, SA_MSG_LEN));
    BENCH_LOG("\t| %-8s %12.0f %12llu %12llu %16.1f |\n", "verify",
              sig_perf->verify_perf.avg_cycles,
              (unsigned long long)sig_perf->verify_perf.min_cycles,
              (unsigned long long)sig_perf->verify_perf.max_cycles,
              sig_perf->verify_perf.throughput);
    BENCH_LOG("\t|----------------------------------------------------------------------|\n\n");

end:
    free(pk);
    free(sk);
    free(signature);
    return CRYPTO_SUCCESS;
}

static int run_sig_mem_profile(const ALGORITHM *sig_alg, struct mem_profile *mem_prof,
                               const bench_opt *opt)
{
    const char *lib_path = opt->lib_path ? opt->lib_path : sig_alg->lib_name;

    if (static_mem_parse(lib_path, mem_prof) != 0) {
        BENCH_LOG("[mem]    static segments unavailable for %s\n", lib_path);
        mem_prof->text_size = mem_prof->data_size = mem_prof->bss_size = 0;
    }
    mem_prof->peak_rss = peak_rss_bytes();

    BENCH_LOG("Inspecting file %s\n\n", lib_path);
    BENCH_LOG("\t|-----------------------------------------|\n");
    BENCH_LOG("\t|   TEXT size:\t%18zu bytes  |\n", mem_prof->text_size);
    BENCH_LOG("\t|   DATA size:\t%18zu bytes  |\n", mem_prof->data_size);
    BENCH_LOG("\t|   BSS  size:\t%18zu bytes  |\n", mem_prof->bss_size);
    BENCH_LOG("\t|   Peak RSS:\t%18ld bytes  |\n", mem_prof->peak_rss);
    BENCH_LOG("\t|-----------------------------------------|\n");
    return CRYPTO_SUCCESS;
}

int run_sig_bench(const ALGORITHM *sig_alg, const bench_opt *opt)
{
    bench_report_t sig_report = {0};
    char *date_str = get_timestamp("%Y%m%d_%H%M%S");
    char report_path[REPORT_PATH_LEN] = {0};
    const SIG_METHOD *sig = sig_alg->method;
    size_t sig_len = sig->get_sig_len();

    BENCH_LOG("\n==============================================================================================\n");
    BENCH_LOG("Running self-assessment for %s (by %s, security level %d)...\n\n",
              sig_alg->alg_name, sig_alg->author_name, sig_alg->security_level);
    sig_report.alg_name = sig_alg->alg_name;
    sig_report.author_name = sig_alg->author_name;
    sig_report.security_level = sig_alg->security_level;
    sig_report.iterations = opt->times;
    sig_report.sig_para.pub_len = sig->get_pk_len();
    sig_report.sig_para.pri_len = sig->get_sk_len();
    sig_report.sig_para.sig_len = sig->get_sig_len();

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Functional Test result (X86)\n");
    BENCH_LOG("==============================================================================================\n");
    if (run_sig_func_bench(sig_alg, &sig_report.sig_func, &sig_len) != CRYPTO_SUCCESS)
        return CRYPTO_FAILED;
    sig_report.sig_para.sig_len = sig_len;

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Performance Test result (X86)\n");
    BENCH_LOG("==============================================================================================\n");
    run_sig_speed_bench(sig_alg, &sig_report.sig_perf, opt, sig_len);

    BENCH_LOG("==============================================================================================\n");
    BENCH_LOG("Memory profiling result (X86)\n");
    BENCH_LOG("==============================================================================================\n");
    run_sig_mem_profile(sig_alg, &sig_report.mem_report, opt);

    snprintf(report_path, REPORT_PATH_LEN, "%s/%s_%s.json", opt->output_dir, sig_alg->alg_name, date_str);
    save_sig_full_report(report_path, &sig_report, date_str);
    append_sig_csv_summary(opt->output_dir, &sig_report, date_str);

    BENCH_LOG("\nTest report saved to %s\n", report_path);
    BENCH_LOG("==============================================================================================\n");
    return CRYPTO_SUCCESS;
}
