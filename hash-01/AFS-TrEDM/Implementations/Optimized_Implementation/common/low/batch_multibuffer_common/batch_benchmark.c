/*
 * Batch throughput benchmark for optional CryptHash_Batch4/Batch8.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#include "CryptHash_AlgorithmInstance.h"
#include "CryptHash_Batch.h"

#if defined(__x86_64__) || defined(__i386__)
/* Function rdtsc_read: reads a processor cycle counter or monotonic fallback value for benchmarking. */
static uint64_t rdtsc_read(void)
{
# if defined(__x86_64__)
    unsigned int lo, hi;
    __asm__ __volatile__ ("lfence\n\t"
                          "rdtsc\n\t"
                          : "=a"(lo), "=d"(hi)
                          :
                          : "memory");
    return ((uint64_t)hi << 32) | (uint64_t)lo;
# else
    unsigned long long v;
    __asm__ __volatile__ ("lfence\n\t"
                          "rdtsc\n\t"
                          : "=A"(v)
                          :
                          : "memory");
    return (uint64_t)v;
# endif
}
# define HAVE_TSC 1
#else
# define HAVE_TSC 0
#endif

/* Function wall_seconds: returns a wall-clock timestamp used by benchmark drivers. */
static double wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

/* Function fill_message: fills a deterministic test or benchmark message buffer. */
static void fill_message(unsigned char *msg, size_t nbytes, unsigned lane)
{
    uint64_t x = UINT64_C(0x6A09E667F3BCC909) ^ ((uint64_t)lane << 40);
    size_t i;
    for (i = 0U; i < nbytes; i++) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        msg[i] = (unsigned char)(x >> 56);
    }
}

/* Function select_batches: chooses a batch benchmark repetition count for the requested message size. */
static unsigned long select_batches(unsigned long long msg_bits, double target_seconds)
{
    if (msg_bits <= 512ULL) return target_seconds < 0.05 ? 2000UL : 20000UL;
    if (msg_bits <= 8192ULL) return target_seconds < 0.05 ? 400UL : 5000UL;
    if (msg_bits <= 65536ULL) return target_seconds < 0.05 ? 80UL : 1000UL;
    if (msg_bits <= 1048576ULL) return target_seconds < 0.05 ? 8UL : 100UL;
    return target_seconds < 0.05 ? 2UL : 16UL;
}

/* Function print_hex32: prints the leading bytes of a digest in hexadecimal. */
static void print_hex32(const unsigned char *p, size_t n)
{
    size_t i;
    size_t limit = n < 32U ? n : 32U;
    for (i = 0U; i < limit; i++) {
        printf("%02X", p[i]);
    }
}

static const char *row_backend_name(AFS_TREDM_BatchBackendStats stats)
{
    if (stats.batch16_avx512_hot_messages != 0U) {
        if (stats.scalar_fallback_messages == 0U &&
            stats.batch16_scalar_fallback_messages == 0U) {
            return "avx512_batch16_true";
        }
        return "mixed_avx512_batch16_true_scalar_fallback";
    }
    return "scalar_dispatch_fallback";
}

/* Function run_width: runs one fixed-width batch benchmark case. */
static int run_width(unsigned width, unsigned long long msg_bits, double target_seconds)
{
    const size_t digest_bytes = DIGEST_BIT_LENGTH / 8U;
    const size_t msg_bytes = (size_t)(msg_bits / 8ULL);
    const unsigned long long lens[16] = {
        msg_bits, msg_bits, msg_bits, msg_bits,
        msg_bits, msg_bits, msg_bits, msg_bits,
        msg_bits, msg_bits, msg_bits, msg_bits,
        msg_bits, msg_bits, msg_bits, msg_bits
    };
    const unsigned char *msgs[16];
    unsigned char *msg_storage[16];
    unsigned char *digests[16];
    unsigned long batches = select_batches(msg_bits, target_seconds);
    unsigned long i;
    unsigned lane;
    int rc = 0;
    double t0, t1, elapsed;
    uint64_t c0 = 0, c1 = 0;
    unsigned char checksum = 0;

    for (lane = 0U; lane < 16U; lane++) {
        msg_storage[lane] = (unsigned char *)malloc(msg_bytes == 0U ? 1U : msg_bytes);
        digests[lane] = (unsigned char *)calloc(digest_bytes, 1U);
        if (msg_storage[lane] == NULL || digests[lane] == NULL) {
            fprintf(stderr, "allocation failed\n");
            return 1;
        }
        fill_message(msg_storage[lane], msg_bytes, lane);
        msgs[lane] = msg_storage[lane];
    }

    if (width == 4U) {
        rc = CryptHash_Batch4(DIGEST_BIT_LENGTH, msgs, lens, digests);
    } else if (width == 8U) {
        rc = CryptHash_Batch8(DIGEST_BIT_LENGTH, msgs, lens, digests);
    } else {
        rc = CryptHash_Batch16(DIGEST_BIT_LENGTH, msgs, lens, digests);
    }
    if (rc != 0) {
        fprintf(stderr, "warm-up Batch%u failed, rc=%d\n", width, rc);
        return 1;
    }

    do {
        CryptHash_Batch_ResetStats();
        t0 = wall_seconds();
#if HAVE_TSC
        c0 = rdtsc_read();
#endif
        for (i = 0UL; i < batches; i++) {
            if (width == 4U) {
                rc = CryptHash_Batch4(DIGEST_BIT_LENGTH, msgs, lens, digests);
            } else if (width == 8U) {
                rc = CryptHash_Batch8(DIGEST_BIT_LENGTH, msgs, lens, digests);
            } else {
                rc = CryptHash_Batch16(DIGEST_BIT_LENGTH, msgs, lens, digests);
            }
            if (rc != 0) {
                fprintf(stderr, "Batch%u failed, rc=%d\n", width, rc);
                return 1;
            }
            checksum ^= digests[i % width][i % digest_bytes];
        }
#if HAVE_TSC
        c1 = rdtsc_read();
#endif
        t1 = wall_seconds();
        elapsed = t1 - t0;
        if (elapsed < target_seconds && batches < 1000000UL) {
            batches *= 2UL;
        } else {
            break;
        }
    } while (1);

    {
        const double hashes = (double)batches * (double)width;
        const double total_bytes = (double)msg_bytes * hashes;
        const AFS_TREDM_BatchBackendStats stats = CryptHash_Batch_GetStats();
        double cycles_per_batch = 0.0;
        double cycles_per_hash = 0.0;
        double cpb_norm = 0.0;
        double total_mib_s = 0.0;

#if HAVE_TSC
        cycles_per_batch = (double)(c1 - c0) / (double)batches;
        cycles_per_hash = (double)(c1 - c0) / hashes;
        if (msg_bytes != 0U) {
            cpb_norm = (double)(c1 - c0) / total_bytes;
        }
#endif
        if (elapsed > 0.0 && msg_bytes != 0U) {
            total_mib_s = total_bytes / (1024.0 * 1024.0) / elapsed;
        }

        printf("batch%u,%s,%llu,%zu,%lu,%.6f,", width,
               row_backend_name(stats), msg_bits, msg_bytes, batches, elapsed);
#if HAVE_TSC
        printf("%.2f,%.2f,", cycles_per_batch, cycles_per_hash);
        if (msg_bytes != 0U) {
            printf("%.2f,", cpb_norm);
        } else {
            printf("NA,");
        }
#else
        printf("NA,NA,NA,");
#endif
        if (msg_bytes != 0U) {
            printf("%.3f,", total_mib_s);
        } else {
            printf("NA,");
        }
        printf("%llu,%llu,%llu,%llu,0x%02X,",
               (unsigned long long)stats.batch16_avx512_hot_blocks,
               (unsigned long long)stats.batch16_avx512_hot_messages,
               (unsigned long long)stats.scalar_fallback_messages,
               (unsigned long long)stats.scalar_fallback_blocks,
               checksum);
        print_hex32(digests[0], digest_bytes);
        printf("\n");
    }

    for (lane = 0U; lane < 16U; lane++) {
        free(digests[lane]);
        free(msg_storage[lane]);
    }
    return 0;
}

#if defined(AFS_TREDM_BENCH_BATCHMANY_ONLY)
/* Function run_batchmany: runs one arbitrary-count batch benchmark case. */
static int run_batchmany(unsigned count, unsigned long long msg_bits, double target_seconds)
{
    const size_t digest_bytes = DIGEST_BIT_LENGTH / 8U;
    const size_t msg_bytes = (size_t)(msg_bits / 8ULL);
    const unsigned char **msgs = NULL;
    unsigned char **msg_storage = NULL;
    unsigned char **digests = NULL;
    unsigned long long *lens = NULL;
    unsigned long batches = select_batches(msg_bits, target_seconds);
    unsigned long i;
    unsigned lane;
    int rc = 0;
    double t0, t1, elapsed;
    uint64_t c0 = 0, c1 = 0;
    unsigned char checksum = 0;

    msgs = (const unsigned char **)calloc(count, sizeof(*msgs));
    msg_storage = (unsigned char **)calloc(count, sizeof(*msg_storage));
    digests = (unsigned char **)calloc(count, sizeof(*digests));
    lens = (unsigned long long *)calloc(count, sizeof(*lens));
    if (msgs == NULL || msg_storage == NULL || digests == NULL || lens == NULL) {
        fprintf(stderr, "allocation failed\n");
        rc = 1;
        goto cleanup;
    }

    for (lane = 0U; lane < count; lane++) {
        msg_storage[lane] = (unsigned char *)malloc(msg_bytes == 0U ? 1U : msg_bytes);
        digests[lane] = (unsigned char *)calloc(digest_bytes, 1U);
        if (msg_storage[lane] == NULL || digests[lane] == NULL) {
            fprintf(stderr, "allocation failed\n");
            rc = 1;
            goto cleanup;
        }
        fill_message(msg_storage[lane], msg_bytes, lane + 100U);
        msgs[lane] = msg_storage[lane];
        lens[lane] = msg_bits;
    }

    rc = CryptHash_BatchMany(DIGEST_BIT_LENGTH, count, msgs, lens, digests);
    if (rc != 0) {
        fprintf(stderr, "warm-up BatchMany failed, rc=%d\n", rc);
        rc = 1;
        goto cleanup;
    }

    do {
        CryptHash_Batch_ResetStats();
        t0 = wall_seconds();
#if HAVE_TSC
        c0 = rdtsc_read();
#endif
        for (i = 0UL; i < batches; i++) {
            rc = CryptHash_BatchMany(DIGEST_BIT_LENGTH, count, msgs, lens, digests);
            if (rc != 0) {
                fprintf(stderr, "BatchMany failed, rc=%d\n", rc);
                rc = 1;
                goto cleanup;
            }
            checksum ^= digests[i % count][i % digest_bytes];
        }
#if HAVE_TSC
        c1 = rdtsc_read();
#endif
        t1 = wall_seconds();
        elapsed = t1 - t0;
        if (elapsed < target_seconds && batches < 1000000UL) {
            batches *= 2UL;
        } else {
            break;
        }
    } while (1);

    {
        const double hashes = (double)batches * (double)count;
        const double total_bytes = (double)msg_bytes * hashes;
        const AFS_TREDM_BatchBackendStats stats = CryptHash_Batch_GetStats();
        double cycles_per_group = 0.0;
        double cycles_per_hash = 0.0;
        double cpb_norm = 0.0;
        double total_mib_s = 0.0;

#if HAVE_TSC
        cycles_per_group = (double)(c1 - c0) / (double)batches;
        cycles_per_hash = (double)(c1 - c0) / hashes;
        if (msg_bytes != 0U) {
            cpb_norm = (double)(c1 - c0) / total_bytes;
        }
#endif
        if (elapsed > 0.0 && msg_bytes != 0U) {
            total_mib_s = total_bytes / (1024.0 * 1024.0) / elapsed;
        }

        printf("batchmany%u,%s,%llu,%zu,%lu,%.6f,", count,
               row_backend_name(stats), msg_bits, msg_bytes, batches, elapsed);
#if HAVE_TSC
        printf("%.2f,%.2f,", cycles_per_group, cycles_per_hash);
        if (msg_bytes != 0U) {
            printf("%.2f,", cpb_norm);
        } else {
            printf("NA,");
        }
#else
        printf("NA,NA,NA,");
#endif
        if (msg_bytes != 0U) {
            printf("%.3f,", total_mib_s);
        } else {
            printf("NA,");
        }
        printf("%llu,%llu,%llu,%llu,0x%02X,",
               (unsigned long long)stats.batch16_avx512_hot_blocks,
               (unsigned long long)stats.batch16_avx512_hot_messages,
               (unsigned long long)stats.scalar_fallback_messages,
               (unsigned long long)stats.scalar_fallback_blocks,
               checksum);
        print_hex32(digests[0], digest_bytes);
        printf("\n");
    }

cleanup:
    if (digests != NULL) {
        for (lane = 0U; lane < count; lane++) {
            free(digests[lane]);
        }
    }
    if (msg_storage != NULL) {
        for (lane = 0U; lane < count; lane++) {
            free(msg_storage[lane]);
        }
    }
    free(lens);
    free(digests);
    free(msg_storage);
    free(msgs);
    return rc;
}
#endif

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(int argc, char **argv)
{
    double target_seconds = 0.08;
    unsigned only_width = 0U;
    unsigned long long only_bits = 0ULL;
    static const unsigned long long msg_bits[] = {
        512ULL,
        1024ULL,
        8192ULL,
        524288ULL,
        8388608ULL,
        67108864ULL
    };
#if defined(AFS_TREDM_BENCH_BATCHMANY_ONLY)
    static const unsigned long long batchmany_msg_bits[] = {
        512ULL,
        1024ULL,
        8192ULL,
        524288ULL,
        8388608ULL
    };
#endif
    int ai;
    size_t i;

    for (ai = 1; ai < argc; ai++) {
        if (strcmp(argv[ai], "--quick") == 0) {
            target_seconds = 0.02;
        } else if (strcmp(argv[ai], "--full") == 0) {
            target_seconds = 0.20;
        } else if (strcmp(argv[ai], "--batch4-only") == 0) {
            only_width = 4U;
        } else if (strcmp(argv[ai], "--batch8-only") == 0) {
            only_width = 8U;
        } else if (strcmp(argv[ai], "--batch16-only") == 0) {
            only_width = 16U;
        } else if (strncmp(argv[ai], "--bits=", 7) == 0) {
            char *endp = NULL;
            only_bits = strtoull(argv[ai] + 7, &endp, 10);
            if (endp == argv[ai] + 7 || *endp != '\0' || (only_bits & 7ULL) != 0ULL) {
                fprintf(stderr, "invalid --bits value: %s\n", argv[ai] + 7);
                return 2;
            }
        } else if (strncmp(argv[ai], "--target-seconds=", 17) == 0) {
            char *endp = NULL;
            target_seconds = strtod(argv[ai] + 17, &endp);
            if (endp == argv[ai] + 17 || *endp != '\0' ||
                target_seconds <= 0.0 || target_seconds > 60.0) {
                fprintf(stderr, "invalid --target-seconds value: %s\n", argv[ai] + 17);
                return 2;
            }
        } else {
            fprintf(stderr, "Usage: %s [--quick|--full] [--target-seconds=S] [--bits=N] [--batch4-only|--batch8-only|--batch16-only]\n", argv[0]);
            return 2;
        }
    }

#if defined(AFS_TREDM_BENCH_BATCH16_AVX512_ONLY)
    if (only_width != 0U && only_width != 16U) {
        fprintf(stderr, "this dedicated benchmark target only supports Batch16 AVX512\n");
        return 2;
    }
    only_width = 16U;
#endif

#if !(defined(AFS_TREDM_USE_AVX512_BATCH) && defined(__AVX512F__)) && \
    !defined(AFS_TREDM_USE_AVX2_BATCH16_DUAL)
    if (only_width == 16U) {
        fprintf(stderr, "Batch16 requires an AVX512 or AVX2-dual batch build\n");
        return 2;
    }
#endif

    printf("# algorithm_instance=%s\n", ALGORITHM_INSTANCE);
#if defined(AFS_TREDM_BENCH_BATCHMANY_ONLY)
#if defined(AFS_TREDM_S6_BATCH_SCALAR_FALLBACK)
    printf("# implementation=batchmany-s6-scalar-cryptHash-fallback\n");
#else
    printf("# implementation=batchmany-equal-length-dispatch\n");
#endif
    printf("# backend_default=%s\n", CryptHash_Batch_BackendName());
    printf("# normalized_per_message_byte=yes\n");
    printf("# additional_implementation=yes\n");
    printf("# columns=mode,backend,msg_bits,msg_bytes,batches,elapsed_seconds,cycles_per_group,cycles_per_hash_normalized,cycles_per_byte_normalized,total_MiB_per_second,hot_blocks,hot_messages,fallback_messages,fallback_blocks,checksum,digest_prefix\n");
    if (only_bits != 0ULL) {
        return run_batchmany(37U, only_bits, target_seconds);
    }
    for (i = 0U; i < sizeof(batchmany_msg_bits) / sizeof(batchmany_msg_bits[0]); i++) {
        if (run_batchmany(37U, batchmany_msg_bits[i], target_seconds) != 0) {
            return 1;
        }
    }
    return 0;
#endif
#if defined(AFS_TREDM_BENCH_BATCH16_AVX512_ONLY) && \
    defined(AFS_TREDM_USE_AVX512_BATCH) && defined(__AVX512F__) && \
    (DIGEST_BIT_LENGTH == 512 || DIGEST_BIT_LENGTH == 768 || DIGEST_BIT_LENGTH == 1024)
    if (CryptHash_Batch16_IsTrueAVX512Enabled()) {
        printf("# implementation=batch16-avx512-true-s6-same-len-byte-aligned\n");
    } else {
        printf("# implementation=batch16-avx512-build-scalar-fallback\n");
    }
#elif defined(AFS_TREDM_S6_BATCH_SCALAR_FALLBACK)
    printf("# implementation=batch-s6-scalar-cryptHash-fallback\n");
#elif defined(AFS_TREDM_USE_AVX512_BATCH) && defined(__AVX512F__)
# if defined(AFS_TREDM_BENCH_BATCH16_AVX512_ONLY)
#  if defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE150_VPRORD_ROUND_BLOCK5)
    printf("# implementation=batch16-avx512-x16-split32-stage150-vprord-fused-cvec-round-asm-block5\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE131_CVEC_ROUND_BLOCK5)
    printf("# implementation=batch16-avx512-x16-split32-stage131-fused-cvec-round-asm-block5\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE41_ASM_BLOCK5_LINEAR) && \
      defined(AFS_TREDM_USE_AVX512_BATCH16_CVEC_RC) && \
      defined(AFS_TREDM_USE_AVX512_BATCH16_UNROLL_ROUND) && \
      defined(AFS_TREDM_STAGE130_NATIVE_TARGET)
    printf("# implementation=batch16-avx512-x16-split32-cvec-rc-unroll-round-linear-stage130-asm-block5-native\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE41_ASM_BLOCK5_LINEAR) && \
      defined(AFS_TREDM_USE_AVX512_BATCH16_CVEC_RC) && \
      defined(AFS_TREDM_USE_AVX512_BATCH16_UNROLL_ROUND)
    printf("# implementation=batch16-avx512-x16-split32-cvec-rc-unroll-round-linear-stage130-asm-block5\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE41_ASM_BLOCK5_LINEAR) && \
        defined(AFS_TREDM_USE_AVX512_BATCH16_CVEC_RC)
    printf("# implementation=batch16-avx512-x16-split32-cvec-rc-linear-stage130-asm-block5\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_UNROLL_ROUND)
    printf("# implementation=batch16-avx512-x16-split32-unroll-round\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_SPLIT_SYMBOLS)
    printf("# implementation=batch16-avx512-x16-split32-split-symbols\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_CHUNK8_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-chunk8\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_ANDXOR_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-andxor\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_NOINLINE_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-noinline\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE41_ASM_SHIFT_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-stage41-asm-shift\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE41_ASM_SRC_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-stage41-asm-src\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE41_ASM_BLOCK5_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-stage41-asm-block5\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE128_ASM_BLOCK4_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-stage128-asm-block4\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE44_ASM_BLOCK6_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-stage44-asm-block6\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_STAGE44_ASM_BLOCK5_REORDERED_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-stage44-asm-block5-reordered\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_CHUNK5_LINEAR)
    printf("# implementation=batch16-avx512-x16-split32-linear-chunk5\n");
#  elif defined(AFS_TREDM_USE_AVX512_BATCH16_NATIVE_ROT)
    printf("# implementation=batch16-avx512-x16-split32-native-rot\n");
#  else
    printf("# implementation=batch16-avx512-x16-split32\n");
#  endif
#  if defined(AFS_TREDM_BATCH16_FAST_512BIT_FINAL) && (DIGEST_BIT_LENGTH == 512)
    printf("# short_message_fastpath=batch16-512bit-final-direct\n");
#  elif defined(AFS_TREDM_BATCH16_FAST_512BIT_MESSAGE_FINAL) && (DIGEST_BIT_LENGTH == 768)
    printf("# short_message_fastpath=batch16-512bit-message-final-direct\n");
#  elif defined(AFS_TREDM_BATCH_DIRECT_FINAL_ONE_BLOCK)
    printf("# short_message_fastpath=batch-direct-final-one-block\n");
#  else
    printf("# short_message_fastpath=generic-frame-final\n");
#  endif
# elif defined(AFS_TREDM_USE_AVX512_BATCH16_V3_NOCVEC_RC)
    printf("# implementation=batch-avx512-v3-nocvec-rc-x8-soa64-x16-split32\n");
# elif defined(AFS_TREDM_USE_AVX512_BATCH16_CVEC_RC)
    printf("# implementation=batch-avx512-cvec-rc-x8-soa64-x16-split32\n");
# else
    printf("# implementation=batch-avx512-x8-soa64-x16-split32\n");
# endif
#elif defined(AFS_TREDM_USE_AVX2_BATCH16_TRUE_INTERLEAVE)
    printf("# implementation=batch-avx2-batch16-true-interleave-x4-soa64-x8-split32\n");
#elif defined(AFS_TREDM_USE_AVX2_BATCH8_R22_MASKTABLE_LINEAR)
    printf("# implementation=batch-avx2-r22-masktable-linear-x4-soa64-x8-split32\n");
#elif defined(AFS_TREDM_USE_AVX2_BATCH8_R28_CHUNK10_ROUND)
    printf("# implementation=batch-avx2-r28-chunk10-round-x4-soa64-x8-split32\n");
#elif defined(AFS_TREDM_USE_AVX2_BATCH8_R21_LOWLIVE_ROUND)
    printf("# implementation=batch-avx2-r21-lowlive-round-x4-soa64-x8-split32\n");
#elif defined(AFS_TREDM_USE_AVX2_BATCH8_R20_FUSED_ROUND)
    printf("# implementation=batch-avx2-r20-fused-round-x4-soa64-x8-split32\n");
#elif defined(AFS_TREDM_USE_AVX2_BATCH8_R13_ROUND_SCHED)
    printf("# implementation=batch-avx2-r13-round-sched-x4-soa64-x8-split32\n");
#elif defined(AFS_TREDM_USE_AVX2_BATCH8_ROW_LINEAR)
    printf("# implementation=batch-avx2-row-linear-x4-soa64-x8-split32\n");
#elif defined(AFS_TREDM_USE_AVX2_BATCH8_REG_LINEAR)
    printf("# implementation=batch-avx2-reg-linear-x4-soa64-x8-split32\n");
#else
    printf("# implementation=batch-avx2-x4-soa64-x8-split32\n");
#endif
    printf("# backend_default=%s\n", CryptHash_Batch_BackendName());
    printf("# normalized_per_message_byte=yes\n");
    printf("# additional_implementation=yes\n");
    printf("# columns=mode,backend,msg_bits,msg_bytes,batches,elapsed_seconds,cycles_per_batch,cycles_per_hash_normalized,cycles_per_byte_normalized,total_MiB_per_second,hot_blocks,hot_messages,fallback_messages,fallback_blocks,checksum,digest_prefix\n");

    if (only_bits != 0ULL) {
        if ((only_width == 0U || only_width == 4U) &&
            run_width(4U, only_bits, target_seconds) != 0) {
            return 1;
        }
        if ((only_width == 0U || only_width == 8U) &&
            run_width(8U, only_bits, target_seconds) != 0) {
            return 1;
        }
#if defined(AFS_TREDM_USE_AVX512_BATCH) && defined(__AVX512F__)
        if ((only_width == 0U || only_width == 16U) &&
            run_width(16U, only_bits, target_seconds) != 0) {
            return 1;
        }
#elif defined(AFS_TREDM_USE_AVX2_BATCH16_DUAL)
        if ((only_width == 0U || only_width == 16U) &&
            run_width(16U, only_bits, target_seconds) != 0) {
            return 1;
        }
#endif
        return 0;
    }

    for (i = 0U; i < sizeof(msg_bits) / sizeof(msg_bits[0]); i++) {
        if ((only_width == 0U || only_width == 4U) &&
            run_width(4U, msg_bits[i], target_seconds) != 0) {
            return 1;
        }
        if ((only_width == 0U || only_width == 8U) &&
            run_width(8U, msg_bits[i], target_seconds) != 0) {
            return 1;
        }
#if defined(AFS_TREDM_USE_AVX512_BATCH) && defined(__AVX512F__)
        if ((only_width == 0U || only_width == 16U) &&
            run_width(16U, msg_bits[i], target_seconds) != 0) {
            return 1;
        }
#elif defined(AFS_TREDM_USE_AVX2_BATCH16_DUAL)
        if ((only_width == 0U || only_width == 16U) &&
            run_width(16U, msg_bits[i], target_seconds) != 0) {
            return 1;
        }
#endif
    }
    return 0;
}
