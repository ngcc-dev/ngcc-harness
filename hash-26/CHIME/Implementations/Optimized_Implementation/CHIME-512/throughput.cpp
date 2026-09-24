/*
 * This file benchmarks the optimized CHIME-512 implementation and compares it with SHA-512 implementations.
 * It measures throughput for several message sizes and prints the results in a table.
 *
 * 本文件用于对 CHIME-512 优化实现进行基准测试，并与 SHA-512 实现进行比较。
 * 它会对多种消息长度测量吞吐率，并以表格形式输出结果。
 */
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include "chime512.hpp"
#include "sha512.h"
#include "sha_openssl.h"

/*
 * Represents a hash function used by the benchmark harness.
 * 表示基准测试驱动中使用的哈希函数。
 */
using HashFn = int (*)(const uint8_t* msg, size_t len_bits, uint8_t* digest);

/*
 * Stores the benchmark result for one algorithm and one message size.
 * 存储一个算法在某个消息长度下的基准测试结果。
 */
struct BenchResult {
    std::string algo_name;
    std::string label;
    size_t bytes = 0;
    size_t digest_bytes = 0;
    int runs = 0;
    int outer_repeats = 0;
    double median_seconds = 0.0;
    double min_seconds = 0.0;
    double max_seconds = 0.0;
    double median_mbps = 0.0;
    double min_mbps = 0.0;
    double max_mbps = 0.0;
};

/*
 * Describes one benchmark case.
 * 描述一个基准测试用例。
 */
struct BenchCase {
    const char* label;
    size_t bytes;
    int runs;
    int outer_repeats;
};

/*
 * Describes one benchmark algorithm entry.
 * 描述一个基准测试算法条目。
 */
struct BenchAlgorithm {
    const char* name;
    HashFn fn;
    size_t digest_bytes;
};

/*
 * Generates a deterministic pseudo-random benchmark message.
 * Inputs:
 *     num_bytes: message size in bytes
 * Outputs:
 *     return value: byte vector containing the generated message
 *
 * 生成确定性的伪随机基准测试消息。
 * 输入：
 *     num_bytes：消息字节数
 * 输出：
 *     返回值：包含生成消息的字节向量
 */
static std::vector<uint8_t> make_bench_message(size_t num_bytes) {
    std::vector<uint8_t> msg(num_bytes);
    for (size_t i = 0; i < num_bytes; ++i) {
        msg[i] = static_cast<uint8_t>((i * 131u + 17u) & 0xFFu);
    }
    return msg;
}

/*
 * Computes the median value of a non-empty vector.
 * Inputs:
 *     v: vector of sample values
 * Outputs:
 *     return value: median of the input samples
 *
 * 计算非空向量的中位数。
 * 输入：
 *     v：样本值向量
 * 输出：
 *     返回值：输入样本的中位数
 */
static double median_of(std::vector<double> v) {
    if (v.empty()) {
        throw std::invalid_argument("median_of: empty vector");
    }
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    if (n & 1u) return v[n / 2];
    return 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

/*
 * Benchmarks one algorithm on one message size and returns the statistics.
 * Inputs:
 *     algo: algorithm entry to benchmark
 *     bc: benchmark case definition
 * Outputs:
 *     return value: benchmark result for this algorithm and size
 *
 * 对某个消息长度下的单个算法进行基准测试并返回统计结果。
 * 输入：
 *     algo：待测试的算法条目
 *     bc：基准测试用例定义
 * 输出：
 *     返回值：该算法在此消息长度下的测试结果
 */
static BenchResult benchmark_one_size(const BenchAlgorithm& algo,
                                      const BenchCase& bc) {
    if (algo.fn == nullptr) {
        throw std::invalid_argument("benchmark_one_size: algo.fn must not be null");
    }
    if (bc.runs <= 0 || bc.outer_repeats <= 0) {
        throw std::invalid_argument("benchmark_one_size: invalid runs/repeats");
    }

    std::vector<uint8_t> msg = make_bench_message(bc.bytes);
    std::vector<uint8_t> digest(algo.digest_bytes, 0);

    for (int i = 0; i < 3; ++i) {
        algo.fn(msg.data(), bc.bytes * 8, digest.data());
    }

    volatile uint8_t sink = 0;

    std::vector<double> avg_seconds_list;
    std::vector<double> mbps_list;
    avg_seconds_list.reserve((size_t)bc.outer_repeats);
    mbps_list.reserve((size_t)bc.outer_repeats);

    for (int rep = 0; rep < bc.outer_repeats; ++rep) {
        auto t0 = std::chrono::steady_clock::now();

        for (int i = 0; i < bc.runs; ++i) {
            algo.fn(msg.data(), bc.bytes * 8, digest.data());
            sink ^= digest[0];
        }

        auto t1 = std::chrono::steady_clock::now();
        const double total_seconds =
            std::chrono::duration<double>(t1 - t0).count();
        const double avg_seconds = total_seconds / static_cast<double>(bc.runs);
        const double mbps =
            (static_cast<double>(bc.bytes) * 8.0) / avg_seconds / 1.0e6;

        avg_seconds_list.push_back(avg_seconds);
        mbps_list.push_back(mbps);
    }

    if (sink == 0xFFu) {
        std::fprintf(stderr, "sink=%u\n", static_cast<unsigned>(sink));
    }

    BenchResult r;
    r.algo_name = algo.name;
    r.label = bc.label;
    r.bytes = bc.bytes;
    r.digest_bytes = algo.digest_bytes;
    r.runs = bc.runs;
    r.outer_repeats = bc.outer_repeats;

    r.median_seconds = median_of(avg_seconds_list);
    r.min_seconds = *std::min_element(avg_seconds_list.begin(), avg_seconds_list.end());
    r.max_seconds = *std::max_element(avg_seconds_list.begin(), avg_seconds_list.end());

    r.median_mbps = median_of(mbps_list);
    r.min_mbps = *std::min_element(mbps_list.begin(), mbps_list.end());
    r.max_mbps = *std::max_element(mbps_list.begin(), mbps_list.end());

    return r;
}

/*
 * Benchmarks one algorithm across all configured cases.
 * Inputs:
 *     algo: algorithm entry to benchmark
 *     cases: list of benchmark cases
 * Outputs:
 *     return value: list of benchmark results
 *
 * 对一个算法在全部配置用例上进行基准测试。
 * 输入：
 *     algo：待测试的算法条目
 *     cases：基准测试用例列表
 * 输出：
 *     返回值：基准测试结果列表
 */
static std::vector<BenchResult>
benchmark_algorithm(const BenchAlgorithm& algo,
                    const std::vector<BenchCase>& cases) {
    std::vector<BenchResult> results;
    results.reserve(cases.size());

    for (const auto& bc : cases) {
        results.push_back(benchmark_one_size(algo, bc));
    }
    return results;
}

/*
 * Prints benchmark results in a table format.
 * Inputs:
 *     results: benchmark results to print
 *
 * 以表格形式打印基准测试结果。
 * 输入：
 *     results：待打印的基准测试结果
 */
static void print_bench_results(const std::vector<BenchResult>& results) {
    if (results.empty()) return;

    std::printf("============================================================\n");
    std::printf("%s (digest = %zu bytes)\n",
                results[0].algo_name.c_str(),
                results[0].digest_bytes);
    std::printf("============================================================\n");

    std::printf("%-8s %-14s %-8s %-8s %-14s %-14s %-14s\n",
                "Size", "Bytes", "Runs", "Outer",
                "Median Mbps", "Min Mbps", "Max Mbps");

    for (const auto& r : results) {
        std::printf("%-8s %-14zu %-8d %-8d %-14.3f %-14.3f %-14.3f\n",
                    r.label.c_str(),
                    r.bytes,
                    r.runs,
                    r.outer_repeats,
                    r.median_mbps,
                    r.min_mbps,
                    r.max_mbps);
    }

    std::printf("\n");
}

/*
 * Wraps the OpenSSL SHA-512 implementation to match the benchmark function signature.
 * Inputs:
 *     msg: message buffer
 *     len_bits: message length in bits, must be byte-aligned
 *     digest: output digest buffer
 * Outputs:
 *     return value: 0 on success, throws on invalid input
 *
 * 将 OpenSSL SHA-512 实现包装为与基准测试一致的函数签名。
 * 输入：
 *     msg：消息缓冲区
 *     len_bits：消息位长度，必须按字节对齐
 *     digest：输出摘要缓冲区
 * 输出：
 *     返回值：成功返回 0，输入非法时抛出异常
 */
static int sha512_openssl_wrapper(const uint8_t* msg, size_t len_bits, uint8_t* digest) {
    if ((len_bits & 7u) != 0u) {
        throw std::invalid_argument("sha512_openssl_wrapper: len_bits must be a multiple of 8");
    }

    const size_t len_bytes = len_bits >> 3;
    sha512_openssl(
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(msg)),
        len_bytes,
        reinterpret_cast<unsigned char*>(digest)
    );
    return 0;
}

/*
 * Wraps the manual SHA-512 implementation to match the benchmark function signature.
 * Inputs:
 *     msg: message buffer
 *     len_bits: message length in bits, must be byte-aligned
 *     digest: output digest buffer
 * Outputs:
 *     return value: 0 on success, throws on invalid input
 *
 * 将手写 SHA-512 实现包装为与基准测试一致的函数签名。
 * 输入：
 *     msg：消息缓冲区
 *     len_bits：消息位长度，必须按字节对齐
 *     digest：输出摘要缓冲区
 * 输出：
 *     返回值：成功返回 0，输入非法时抛出异常
 */
static int sha512_manual_wrapper(const uint8_t* msg, size_t len_bits, uint8_t* digest) {
    if ((len_bits & 7u) != 0u) {
        throw std::invalid_argument("sha512_manual_wrapper: len_bits must be a multiple of 8");
    }

    const size_t len_bytes = len_bits >> 3;
    sha_512(
        const_cast<uint8_t*>(msg),
        len_bytes,
        reinterpret_cast<unsigned char*>(digest)
    );
    return 0;
}

/*
 * Runs the benchmark suite for CHIME-512 and SHA-512 implementations.
 * 运行 CHIME-512 与 SHA-512 实现的基准测试套件。
 */
int main() {
    constexpr size_t KB = 1024ULL;
    constexpr size_t MB = 1024ULL * 1024ULL;
    constexpr size_t GB = 1024ULL * 1024ULL * 1024ULL;

    std::vector<BenchCase> cases = {
        {"128B",   128ULL,      2000000, 7},
        {"1KB",    1ULL * KB,   1000000, 7},
        {"16KB",   16ULL * KB,   200000, 7},
        {"100MB", 100ULL * MB,       10, 5},
        {"1GB",    1ULL * GB,         3, 5},
    };

    std::vector<BenchAlgorithm> algos = {
        {"sponge_512_bit_padding",   chime_512_bit_padding,   64},
        {"sponge_512_byte_padding",  chime_512_byte_padding,  64},
        {"sha512_manual",            sha512_manual_wrapper,         64},
        {"sha512_openssl",           sha512_openssl_wrapper,        64},
    };

    for (const auto& algo : algos) {
        auto results = benchmark_algorithm(algo, cases);
        print_bench_results(results);
    }

    return 0;
}