#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#define WCHAIN_NO_MAIN
#include "wchain_c.c"
#undef WCHAIN_NO_MAIN

#include <inttypes.h>

static volatile uint64_t bench_sink;

static double bench_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000000.0 + (double)ts.tv_nsec;
}

static void bench_fill(uint8_t *buf, size_t len, uint64_t seed) {
    for (size_t i = 0; i < len; ++i) {
        buf[i] = (uint8_t)((seed + 0x9e3779b97f4a7c15ULL * (uint64_t)(i + 1)) >> ((i & 7u) * 8u));
    }
}

static void bench_fill_state(uint64_t s[9], uint64_t seed) {
    for (int i = 0; i < 9; ++i) s[i] = seed ^ (0x9e3779b97f4a7c15ULL * (uint64_t)(i + 1));
}

static size_t bench_iters_for_len(size_t len) {
    const size_t target = 128u * 1024u * 1024u;
    size_t iters = target / (len ? len : 1u);
    if (iters < 128u) iters = 128u;
    if (iters > 2000000u) iters = 2000000u;
    return iters;
}

static void ref_v1_compress_loop(uint64_t out[9], const uint64_t v[9], const uint8_t block[144]) {
    uint64_t b[WCHAIN_V1_ROUNDS + 1][9], x[9];
    load_state9(b[0], block);
    load_state9(b[1], block + 72);
    for (int j = 2; j <= WCHAIN_V1_ROUNDS; ++j) mef_v1_xor_u64(b[j], b[j - 1], b[j - 2]);
    memcpy(x, v, sizeof(x));
    for (int j = 0; j < WCHAIN_V1_ROUNDS; ++j) round_v1_inject_u64(x, b[j], j);
    for (int i = 0; i < 9; ++i) out[i] = x[i] ^ b[WCHAIN_V1_ROUNDS][i] ^ v[i];
}

static int selftest_scalar(void) {
    uint8_t block[144];
    uint64_t v[9], got[9], ref[9];
    for (int tc = 0; tc < 128; ++tc) {
        bench_fill(block, sizeof(block), 0x12345678abcdef00ULL ^ (uint64_t)tc);
        bench_fill_state(v, 0xa5a5a5a500000000ULL ^ (uint64_t)tc);
        wchain_v1_compress_c(got, v, block);
        ref_v1_compress_loop(ref, v, block);
        if (memcmp(got, ref, sizeof(got)) != 0) return 0;
    }
    return 1;
}

#if !defined(WCHAIN_DISABLE_SIMD)
static int selftest_avx2_v1(void) {
    if (!cpu_has_avx2()) return 1;
    uint64_t v[4][9], got[4][9], ref[4][9];
    uint8_t blocks[4][144];
    for (int lane = 0; lane < 4; ++lane) {
        bench_fill(blocks[lane], 144, 0x3141592600000000ULL ^ (uint64_t)lane);
        bench_fill_state(v[lane], 0x2718281800000000ULL ^ (uint64_t)lane);
        wchain_v1_compress_c(ref[lane], v[lane], blocks[lane]);
    }
    wchain_v1_compress4_avx2(got, v, blocks);
    return memcmp(got, ref, sizeof(got)) == 0;
}

static int selftest_avx512_v1(void) {
    if (!cpu_has_avx512f()) return 1;
    uint64_t v[8][9], got[8][9], ref[8][9];
    uint8_t blocks[8][144];
    for (int lane = 0; lane < 8; ++lane) {
        bench_fill(blocks[lane], 144, 0x1618033900000000ULL ^ (uint64_t)lane);
        bench_fill_state(v[lane], 0x5772156600000000ULL ^ (uint64_t)lane);
        wchain_v1_compress_c(ref[lane], v[lane], blocks[lane]);
    }
    wchain_v1_compress8_avx512(got, v, blocks);
    return memcmp(got, ref, sizeof(got)) == 0;
}

static uint8_t *make_wchain_v1_blocks4(size_t len, size_t *num_blocks) {
    size_t padded_len = len + 1;
    while (padded_len % 144u != 0) ++padded_len;
    *num_blocks = padded_len / 144u;
    uint8_t *blocks = (uint8_t *)calloc((*num_blocks) * 4u * 144u, 1);
    if (!blocks) return NULL;
    for (int lane = 0; lane < 4; ++lane) {
        for (size_t i = 0; i < len; ++i) {
            blocks[(i / 144u) * 4u * 144u + (size_t)lane * 144u + (i % 144u)] =
                (uint8_t)(i * 131u + 17u + (size_t)lane * 29u);
        }
        blocks[(len / 144u) * 4u * 144u + (size_t)lane * 144u + (len % 144u)] = 0x80;
    }
    return blocks;
}

static uint8_t *make_wchain_v1_blocks8(size_t len, size_t *num_blocks) {
    size_t padded_len = len + 1;
    while (padded_len % 144u != 0) ++padded_len;
    *num_blocks = padded_len / 144u;
    uint8_t *blocks = (uint8_t *)calloc((*num_blocks) * 8u * 144u, 1);
    if (!blocks) return NULL;
    for (int lane = 0; lane < 8; ++lane) {
        for (size_t i = 0; i < len; ++i) {
            blocks[(i / 144u) * 8u * 144u + (size_t)lane * 144u + (i % 144u)] =
                (uint8_t)(i * 131u + 17u + (size_t)lane * 29u);
        }
        blocks[(len / 144u) * 8u * 144u + (size_t)lane * 144u + (len % 144u)] = 0x80;
    }
    return blocks;
}

__attribute__((target("avx2")))
static void wchain_v1_hash4_avx2_blocks(const uint8_t *blocks, size_t num_blocks, uint8_t out[4][64]) {
    uint64_t h_prev2[4][9] = {{0}}, h_prev1[4][9] = {{0}}, c[4][9];
    uint64_t sigma_words[4][18] = {{0}};
    uint8_t sigma[4][144];
    for (size_t blk = 0; blk < num_blocks; ++blk) {
        uint8_t (*block4)[144] = (uint8_t (*)[144])((uint8_t *)blocks + blk * 4u * 144u);
        for (int lane = 0; lane < 4; ++lane) {
            for (int i = 0; i < 18; ++i) sigma_words[lane][i] ^= load64_raw(block4[lane] + 8 * i);
        }
        wchain_v1_compress4_avx2(c, h_prev1, block4);
        for (int lane = 0; lane < 4; ++lane) {
            for (int i = 0; i < 9; ++i) {
                const uint64_t old_h1 = h_prev1[lane][i];
                h_prev1[lane][i] = c[lane][i] ^ h_prev2[lane][i];
                h_prev2[lane][i] = old_h1;
            }
        }
    }
    for (int lane = 0; lane < 4; ++lane) {
        for (int i = 0; i < 18; ++i) store64_raw(sigma[lane] + 8 * i, sigma_words[lane][i]);
    }
    wchain_v1_compress4_avx2(c, h_prev1, sigma);
    for (int lane = 0; lane < 4; ++lane) {
        for (int i = 0; i < 8; ++i) store64_be(out[lane] + 8 * i, c[lane][i]);
    }
}

__attribute__((target("avx512f")))
static void wchain_v1_hash8_avx512_blocks(const uint8_t *blocks, size_t num_blocks, uint8_t out[8][64]) {
    uint64_t h_prev2[8][9] = {{0}}, h_prev1[8][9] = {{0}}, c[8][9];
    uint64_t sigma_words[8][18] = {{0}};
    uint8_t sigma[8][144];
    for (size_t blk = 0; blk < num_blocks; ++blk) {
        uint8_t (*block8)[144] = (uint8_t (*)[144])((uint8_t *)blocks + blk * 8u * 144u);
        for (int lane = 0; lane < 8; ++lane) {
            for (int i = 0; i < 18; ++i) sigma_words[lane][i] ^= load64_raw(block8[lane] + 8 * i);
        }
        wchain_v1_compress8_avx512(c, h_prev1, block8);
        for (int lane = 0; lane < 8; ++lane) {
            for (int i = 0; i < 9; ++i) {
                const uint64_t old_h1 = h_prev1[lane][i];
                h_prev1[lane][i] = c[lane][i] ^ h_prev2[lane][i];
                h_prev2[lane][i] = old_h1;
            }
        }
    }
    for (int lane = 0; lane < 8; ++lane) {
        for (int i = 0; i < 18; ++i) store64_raw(sigma[lane] + 8 * i, sigma_words[lane][i]);
    }
    wchain_v1_compress8_avx512(c, h_prev1, sigma);
    for (int lane = 0; lane < 8; ++lane) {
        for (int i = 0; i < 8; ++i) store64_be(out[lane] + 8 * i, c[lane][i]);
    }
}

static int selftest_avx_hashes(void) {
    const size_t sizes[] = {0, 3, 1024, 4096};
    uint8_t scalar[64];
    for (size_t si = 0; si < sizeof(sizes) / sizeof(sizes[0]); ++si) {
        const size_t len = sizes[si];
        uint8_t *msg = (uint8_t *)malloc(len ? len : 1);
        if (!msg) return 0;
        if (cpu_has_avx2()) {
            size_t nb = 0;
            uint8_t (*out)[64];
            uint8_t *blocks = make_wchain_v1_blocks4(len, &nb);
            if (!blocks) return 0;
            out = (uint8_t (*)[64])calloc(4, 64);
            if (!out) return 0;
            wchain_v1_hash4_avx2_blocks(blocks, nb, out);
            for (int lane = 0; lane < 4; ++lane) {
                for (size_t i = 0; i < len; ++i) msg[i] = (uint8_t)(i * 131u + 17u + (size_t)lane * 29u);
                wchain_v1_hash_c(msg, len, scalar);
                if (memcmp(out[lane], scalar, 64) != 0) return 0;
            }
            free(out);
            free(blocks);
        }
        if (cpu_has_avx512f()) {
            size_t nb = 0;
            uint8_t (*out)[64];
            uint8_t *blocks = make_wchain_v1_blocks8(len, &nb);
            if (!blocks) return 0;
            out = (uint8_t (*)[64])calloc(8, 64);
            if (!out) return 0;
            wchain_v1_hash8_avx512_blocks(blocks, nb, out);
            for (int lane = 0; lane < 8; ++lane) {
                for (size_t i = 0; i < len; ++i) msg[i] = (uint8_t)(i * 131u + 17u + (size_t)lane * 29u);
                wchain_v1_hash_c(msg, len, scalar);
                if (memcmp(out[lane], scalar, 64) != 0) return 0;
            }
            free(out);
            free(blocks);
        }
        free(msg);
    }
    return 1;
}
#endif

static void bench_perm18(size_t iters) {
    uint64_t x[9], base[9];
    bench_fill_state(base, 0x0123456789abcdefULL);
    const double t0 = bench_now_ns();
    for (size_t it = 0; it < iters; ++it) {
        memcpy(x, base, sizeof(x));
        x[0] ^= (uint64_t)it;
        for (int r = 0; r < 20; ++r) round_v1_u64(x, r);
        bench_sink ^= x[it % 9u];
    }
    const double t1 = bench_now_ns();
    printf("PERF,perm18_scalar,%zu,%.2f,%.2f\n", iters, (t1 - t0) / (double)iters, bench_sink ? 1.0 : 0.0);
}

static void bench_compress_scalar(size_t iters) {
    uint8_t block[144];
    uint64_t v[9], out[9];
    bench_fill(block, sizeof(block), 0xabcdef0123456789ULL);
    bench_fill_state(v, 0xfedcba9876543210ULL);
    const double t0 = bench_now_ns();
    for (size_t it = 0; it < iters; ++it) {
        v[0] ^= (uint64_t)it;
        wchain_v1_compress_c(out, v, block);
        bench_sink ^= out[it % 9u];
    }
    const double t1 = bench_now_ns();
    const double ns = (t1 - t0) / (double)iters;
    printf("PERF,compress18_scalar,%zu,%.2f,%.2f\n", iters, ns, 144000.0 / ns);
}

#if !defined(WCHAIN_DISABLE_SIMD)
static void bench_compress_avx2(size_t iters) {
    if (!cpu_has_avx2()) {
        printf("PERF,compress18_avx2x4,0,unsupported,unsupported\n");
        return;
    }
    uint8_t blocks[4][144];
    uint64_t v[4][9], out[4][9];
    for (int lane = 0; lane < 4; ++lane) {
        bench_fill(blocks[lane], 144, 0x100000000ULL + (uint64_t)lane);
        bench_fill_state(v[lane], 0x200000000ULL + (uint64_t)lane);
    }
    const double t0 = bench_now_ns();
    for (size_t it = 0; it < iters; ++it) {
        v[it & 3u][0] ^= (uint64_t)it;
        wchain_v1_compress4_avx2(out, v, blocks);
        bench_sink ^= out[it & 3u][it % 9u];
    }
    const double t1 = bench_now_ns();
    const double ns = (t1 - t0) / (double)iters;
    printf("PERF,compress18_avx2x4,%zu,%.2f,%.2f\n", iters, ns, (4.0 * 144000.0) / ns);
}

static void bench_compress_avx512(size_t iters) {
    if (!cpu_has_avx512f()) {
        printf("PERF,compress18_avx512x8,0,unsupported,unsupported\n");
        return;
    }
    uint8_t blocks[8][144];
    uint64_t v[8][9], out[8][9];
    for (int lane = 0; lane < 8; ++lane) {
        bench_fill(blocks[lane], 144, 0x300000000ULL + (uint64_t)lane);
        bench_fill_state(v[lane], 0x400000000ULL + (uint64_t)lane);
    }
    const double t0 = bench_now_ns();
    for (size_t it = 0; it < iters; ++it) {
        v[it & 7u][0] ^= (uint64_t)it;
        wchain_v1_compress8_avx512(out, v, blocks);
        bench_sink ^= out[it & 7u][it % 9u];
    }
    const double t1 = bench_now_ns();
    const double ns = (t1 - t0) / (double)iters;
    printf("PERF,compress18_avx512x8,%zu,%.2f,%.2f\n", iters, ns, (8.0 * 144000.0) / ns);
}

static void bench_hash4_avx2(size_t len) {
    if (!cpu_has_avx2()) {
        printf("PERF,hash_avx2x4,%zu,0,unsupported,unsupported\n", len);
        return;
    }
    size_t nb = 0;
    uint8_t (*out)[64] = (uint8_t (*)[64])calloc(4, 64);
    uint8_t *blocks = make_wchain_v1_blocks4(len, &nb);
    if (!out || !blocks) exit(2);
    size_t iters = bench_iters_for_len(len);
    if (iters > 20000u) iters = 20000u;
    const double t0 = bench_now_ns();
    for (size_t it = 0; it < iters; ++it) {
        blocks[(it % nb) * 4u * 144u] ^= (uint8_t)it;
        wchain_v1_hash4_avx2_blocks(blocks, nb, out);
        bench_sink ^= out[it & 3u][it & 63u];
    }
    const double t1 = bench_now_ns();
    const double ns = (t1 - t0) / (double)iters;
    printf("PERF,hash_avx2x4,%zu,%zu,%.2f,%.2f\n", len, iters, ns, (4.0 * (double)len * 1000.0) / ns);
    free(blocks);
    free(out);
}

static void bench_hash8_avx512(size_t len) {
    if (!cpu_has_avx512f()) {
        printf("PERF,hash_avx512x8,%zu,0,unsupported,unsupported\n", len);
        return;
    }
    size_t nb = 0;
    uint8_t (*out)[64] = (uint8_t (*)[64])calloc(8, 64);
    uint8_t *blocks = make_wchain_v1_blocks8(len, &nb);
    if (!out || !blocks) exit(2);
    size_t iters = bench_iters_for_len(len);
    if (iters > 20000u) iters = 20000u;
    const double t0 = bench_now_ns();
    for (size_t it = 0; it < iters; ++it) {
        blocks[(it % nb) * 8u * 144u] ^= (uint8_t)it;
        wchain_v1_hash8_avx512_blocks(blocks, nb, out);
        bench_sink ^= out[it & 7u][it & 63u];
    }
    const double t1 = bench_now_ns();
    const double ns = (t1 - t0) / (double)iters;
    printf("PERF,hash_avx512x8,%zu,%zu,%.2f,%.2f\n", len, iters, ns, (8.0 * (double)len * 1000.0) / ns);
    free(blocks);
    free(out);
}
#endif

static void bench_hash_scalar(size_t len) {
    uint8_t *msg = (uint8_t *)malloc(len ? len : 1);
    uint8_t out[64];
    if (!msg) exit(2);
    bench_fill(msg, len, 0x55555555ULL ^ (uint64_t)len);
    size_t iters = bench_iters_for_len(len);
    const double t0 = bench_now_ns();
    for (size_t it = 0; it < iters; ++it) {
        if (len) msg[0] ^= (uint8_t)it;
        wchain_v1_hash_c(msg, len, out);
        bench_sink ^= out[it & 63u];
    }
    const double t1 = bench_now_ns();
    const double ns = (t1 - t0) / (double)iters;
    printf("PERF,hash_scalar,%zu,%zu,%.2f,%.2f\n", len, iters, ns, ((double)len * 1000.0) / ns);
    free(msg);
}

int main(void) {
    const size_t sizes[] = {32, 128, 512, 1024, 4096, 8192, 16384, 65536};
    printf("SELFTEST,scalar,%s\n", selftest_scalar() ? "ok" : "FAILED");
#if !defined(WCHAIN_DISABLE_SIMD)
    printf("SELFTEST,avx2_compress,%s\n", selftest_avx2_v1() ? "ok" : "FAILED");
    printf("SELFTEST,avx512_compress,%s\n", selftest_avx512_v1() ? "ok" : "FAILED");
    printf("SELFTEST,avx_hash,%s\n", selftest_avx_hashes() ? "ok" : "FAILED");
#else
    printf("SELFTEST,simd,disabled\n");
#endif
    printf("PERF_HEADER,target,input_or_iters,iters_or_zero,avg_ns,MBps_or_sink\n");
    bench_perm18(300000);
    bench_compress_scalar(300000);
#if !defined(WCHAIN_DISABLE_SIMD)
    bench_compress_avx2(300000);
    bench_compress_avx512(300000);
#endif
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        bench_hash_scalar(sizes[i]);
#if !defined(WCHAIN_DISABLE_SIMD)
        bench_hash4_avx2(sizes[i]);
        bench_hash8_avx512(sizes[i]);
#endif
    }
    fprintf(stderr, "bench_sink=%" PRIu64 "\n", bench_sink);
    return 0;
}
