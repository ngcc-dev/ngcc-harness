// bench.cpp - x86 self-evaluation benchmark for GALAS optimized impl.
#include "api.hpp"
#include "parameters.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/resource.h>
#include <x86intrin.h>

using namespace faest;

extern "C" void randombytes(unsigned char* x, unsigned long long xlen)
{
    static uint64_t state = 0x6a09e667f3bcc909ULL;
    for (unsigned long long i = 0; i < xlen; ++i)
    {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        x[i] = static_cast<unsigned char>(state >> 56);
    }
}

static constexpr unsigned long long MESSAGE_LEN = 64;

static void fill_message(unsigned char* msg)
{
    for (unsigned long long i = 0; i < MESSAGE_LEN; ++i)
        msg[i] = static_cast<unsigned char>(0x40u + ((17u * i + 29u) & 0x3fu));
}

static uint64_t read_cycles()
{
    return __rdtsc();
}

static double elapsed_seconds(std::chrono::steady_clock::time_point start,
                              std::chrono::steady_clock::time_point end)
{
    return std::chrono::duration<double>(end - start).count();
}

static long peak_rss_kb()
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0)
        return -1;
    return ru.ru_maxrss;
}

template <typename P>
static bool bench_instance(const char* name, int reps)
{
    using Scheme = faest_scheme<P>;
    constexpr size_t PK_SIZE = Scheme::CRYPTO_PUBLICKEYBYTES;
    constexpr size_t SK_SIZE = Scheme::CRYPTO_SECRETKEYBYTES;
    constexpr size_t SIG_SIZE = Scheme::CRYPTO_BYTES;
    constexpr size_t SM_SIZE = MESSAGE_LEN + SIG_SIZE;

    std::array<unsigned char, MESSAGE_LEN> msg{};
    std::array<unsigned char, PK_SIZE> pk{};
    std::array<unsigned char, SK_SIZE> sk{};
    std::array<unsigned char, SM_SIZE> sm{};
    std::array<unsigned char, SM_SIZE> opened{};
    unsigned long long smlen = 0;
    unsigned long long opened_len = 0;

    double t_keygen = 0;
    double t_sign = 0;
    double t_verify = 0;
    double c_keygen = 0;
    double c_sign = 0;
    double c_verify = 0;

    fill_message(msg.data());

    printf("instance=%s\n", name);
    fflush(stdout);

    for (int i = 0; i < reps; ++i)
    {
        auto t0 = std::chrono::steady_clock::now();
        uint64_t c0 = read_cycles();
        if (Scheme::crypto_sign_keypair(pk.data(), sk.data()) != 0)
        {
            printf("keygen failed\n");
            return false;
        }
        uint64_t c1 = read_cycles();
        auto t1 = std::chrono::steady_clock::now();
        t_keygen += elapsed_seconds(t0, t1);
        c_keygen += static_cast<double>(c1 - c0);

        t0 = std::chrono::steady_clock::now();
        c0 = read_cycles();
        if (Scheme::crypto_sign(
                sm.data(), &smlen, msg.data(), MESSAGE_LEN, sk.data()) != 0)
        {
            printf("sign failed\n");
            return false;
        }
        c1 = read_cycles();
        t1 = std::chrono::steady_clock::now();
        t_sign += elapsed_seconds(t0, t1);
        c_sign += static_cast<double>(c1 - c0);

        t0 = std::chrono::steady_clock::now();
        c0 = read_cycles();
        if (Scheme::crypto_sign_open(
                opened.data(), &opened_len, sm.data(), smlen, pk.data()) != 0 ||
            opened_len != MESSAGE_LEN ||
            memcmp(opened.data(), msg.data(), MESSAGE_LEN) != 0)
        {
            printf("verify failed\n");
            return false;
        }
        c1 = read_cycles();
        t1 = std::chrono::steady_clock::now();
        t_verify += elapsed_seconds(t0, t1);
        c_verify += static_cast<double>(c1 - c0);
    }

    printf("reps=%d msg_len=64 pk=%zu sk=%zu sig=%zu peak_rss_kb=%ld\n",
           reps, PK_SIZE, SK_SIZE, SIG_SIZE, peak_rss_kb());
    printf("operation,avg_ms,avg_cycles,ops_per_second\n");
    printf("keygen,%.6f,%.0f,%.6f\n",
           1000.0 * t_keygen / reps, c_keygen / reps,
           t_keygen > 0 ? static_cast<double>(reps) / t_keygen : 0.0);
    printf("sign,%.6f,%.0f,%.6f\n",
           1000.0 * t_sign / reps, c_sign / reps,
           t_sign > 0 ? static_cast<double>(reps) / t_sign : 0.0);
    printf("verify,%.6f,%.0f,%.6f\n\n",
           1000.0 * t_verify / reps, c_verify / reps,
           t_verify > 0 ? static_cast<double>(reps) / t_verify : 0.0);
    fflush(stdout);
    return true;
}

static bool should_run(int argc, char** argv, const char* name)
{
    if (argc <= 1 || strcmp(argv[1], "all") == 0)
        return true;
    return strcmp(argv[1], name) == 0;
}

template <typename P>
static bool maybe_bench(int argc, char** argv, const char* name, int reps)
{
    if (!should_run(argc, argv, name))
        return true;
    return bench_instance<P>(name, reps);
}

int main(int argc, char** argv)
{
    int reps = 100;
    if (argc >= 3)
        reps = std::max(1, std::atoi(argv[2]));

    bool ok = true;
    ok &= maybe_bench<galas::galas_ngcc_160s>(argc, argv, "Galas-160S", reps);
    ok &= maybe_bench<galas::galas_ngcc_160f>(argc, argv, "Galas-160F", reps);
    ok &= maybe_bench<galas::galas_ngcc_256s>(argc, argv, "Galas-256S", reps);
    ok &= maybe_bench<galas::galas_ngcc_256f>(argc, argv, "Galas-256F", reps);
    ok &= maybe_bench<galas::galas_ngcc_384s>(argc, argv, "Galas-384S", reps);
    ok &= maybe_bench<galas::galas_ngcc_384f>(argc, argv, "Galas-384F", reps);
    ok &= maybe_bench<galas::galas_ngcc_512s>(argc, argv, "Galas-512S", reps);
    ok &= maybe_bench<galas::galas_ngcc_512f>(argc, argv, "Galas-512F", reps);

    return ok ? 0 : 1;
}
