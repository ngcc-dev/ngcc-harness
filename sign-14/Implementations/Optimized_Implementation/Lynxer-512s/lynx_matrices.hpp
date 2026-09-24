#ifndef LYNX_MATRICES_HPP
#define LYNX_MATRICES_HPP

// Matrix handling for Lynx OWF
// Ported from lynx-sig/lynx_matrices.h

#include "parameters.hpp"
#include "hash.hpp"
#include "lynx_constants.hpp"
#include "lynx_l3_consts.hpp"
#include "hash.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace sig
{
namespace lynx
{

// Maximum dimensions for matrix storage
constexpr std::size_t LYNX_MAX_LAMBDA = 512;
constexpr std::size_t LYNX_MAX_WORDS = (LYNX_MAX_LAMBDA + 63) / 64;

// Helper: compute parity of 64-bit word
inline int parity_u64(uint64_t x)
{
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return static_cast<int>(x & 1);
}

// GF(2) matrix-vector multiplication
// M is stored row-major: M[i][k] = k-th 64-bit word of row i
template <std::size_t N, std::size_t W>
inline void lynx_L_words(const uint64_t M[][W], const std::array<uint64_t, W>& x, std::array<uint64_t, W>& res)
{
    res.fill(0);
    for (unsigned int i = 0; i < N; i++)
    {
        uint64_t dot = 0;
        for (unsigned int j = 0; j < W; ++j)
        {
            dot ^= (M[i][j] & x[j]);
        }
        if (parity_u64(dot))
        {
            res[i / 64] ^= UINT64_C(1) << (i % 64);
        }
    }
}

template <secpar S>
inline poly_secpar<S> lynx_L(const uint64_t M[][(secpar_to_bits(S) + 63) / 64], const poly_secpar<S>& x)
{
    constexpr std::size_t N = secpar_to_bits(S);
    constexpr std::size_t W = (N + 63) / 64;
    std::array<uint64_t, W> in_words{};
    std::array<uint8_t, N / 8> out_bytes{};
    x.store(in_words.data());

    for (std::size_t i = 0; i < N; ++i)
    {
        uint64_t dot = 0;
        for (std::size_t w = 0; w < W; ++w)
            dot ^= M[i][w] & in_words[w];
        if (__builtin_parityll(dot))
            out_bytes[i / 8] ^= static_cast<uint8_t>(1u << (i % 8));
    }

    return poly_secpar<S>::load(out_bytes.data());
}

// Matrix structure for Lynx OWF
template <secpar S>
struct lynx_matrices;

template <>
struct lynx_matrices<secpar::s128>
{
    static constexpr std::size_t N = 128;
    static constexpr std::size_t words = 2;
    static constexpr std::size_t storage_words = LYNX_MAX_WORDS;
    std::array<std::array<uint64_t, storage_words>, N> L0{}, L1{}, L2{}, L3{};
    std::array<uint8_t, 16> C0{}, C1{};

    void clear()
    {
        L0.fill({});
        L1.fill({});
        L2.fill({});
        L3.fill({});
        C0.fill({});
        C1.fill({});
    }
};

template <>
struct lynx_matrices<secpar::s160>
{
    static constexpr std::size_t N = 160;
    static constexpr std::size_t words = 3;
    static constexpr std::size_t storage_words = LYNX_MAX_WORDS;
    std::array<std::array<uint64_t, storage_words>, N> L0{}, L1{}, L2{}, L3{};
    std::array<uint8_t, 20> C0{}, C1{};

    void clear()
    {
        L0.fill({});
        L1.fill({});
        L2.fill({});
        L3.fill({});
        C0.fill({});
        C1.fill({});
    }
};

template <>
struct lynx_matrices<secpar::s192>
{
    static constexpr std::size_t N = 192;
    static constexpr std::size_t words = 3;
    static constexpr std::size_t storage_words = LYNX_MAX_WORDS;
    std::array<std::array<uint64_t, storage_words>, N> L0{}, L1{}, L2{}, L3{};
    std::array<uint8_t, 24> C0{}, C1{};

    void clear()
    {
        L0.fill({});
        L1.fill({});
        L2.fill({});
        L3.fill({});
        C0.fill({});
        C1.fill({});
    }
};

template <>
struct lynx_matrices<secpar::s256>
{
    static constexpr std::size_t N = 256;
    static constexpr std::size_t words = 4;
    static constexpr std::size_t storage_words = LYNX_MAX_WORDS;
    std::array<std::array<uint64_t, storage_words>, N> L0{}, L1{}, L2{}, L3{};
    std::array<uint8_t, 32> C0{}, C1{}, C2{};

    void clear()
    {
        L0.fill({});
        L1.fill({});
        L2.fill({});
        L3.fill({});
        C0.fill({});
        C1.fill({});
        C2.fill({});
    }
};

template <>
struct lynx_matrices<secpar::s384>
{
    static constexpr std::size_t N = 384;
    static constexpr std::size_t words = 6;
    static constexpr std::size_t storage_words = LYNX_MAX_WORDS;
    std::array<std::array<uint64_t, storage_words>, N> L0{}, L1{}, L2{}, L3{};
    std::array<uint8_t, 48> C0{}, C1{}, C2{};

    void clear()
    {
        L0.fill({});
        L1.fill({});
        L2.fill({});
        L3.fill({});
        C0.fill({});
        C1.fill({});
        C2.fill({});
    }
};

template <>
struct lynx_matrices<secpar::s512>
{
    static constexpr std::size_t N = 512;
    static constexpr std::size_t words = 8;
    static constexpr std::size_t storage_words = LYNX_MAX_WORDS;
    std::array<std::array<uint64_t, storage_words>, N> L0{}, L1{}, L2{}, L3{};
    std::array<uint8_t, 64> C0{}, C1{}, C2{};

    void clear()
    {
        L0.fill({});
        L1.fill({});
        L2.fill({});
        L3.fill({});
        C0.fill({});
        C1.fill({});
        C2.fill({});
    }
};

// Helper functions for LU-sampling bit masks
inline uint64_t lower_mask(unsigned int bit)
{
    if (bit == 0)
        return 0;
    return (UINT64_C(1) << bit) - 1;
}

inline uint64_t upper_mask(unsigned int bit)
{
    if (bit == 63)
        return 0;
    return ~((UINT64_C(1) << (bit + 1)) - 1);
}

// Squeeze a little-endian uint64_t from the selected XOF context
inline uint64_t squeeze_u64(hash_state& ctx)
{
    uint8_t tmp[8] = {};
    ctx.finalize(tmp, sizeof(tmp));
    return static_cast<uint64_t>(tmp[0]) | (static_cast<uint64_t>(tmp[1]) << 8) |
           (static_cast<uint64_t>(tmp[2]) << 16) | (static_cast<uint64_t>(tmp[3]) << 24) |
           (static_cast<uint64_t>(tmp[4]) << 32) | (static_cast<uint64_t>(tmp[5]) << 40) |
           (static_cast<uint64_t>(tmp[6]) << 48) | (static_cast<uint64_t>(tmp[7]) << 56);
}

// Sample a lower triangular matrix with 1s on diagonal
template <std::size_t N, std::size_t W, std::size_t SW>
void sample_lower_triangular(hash_state& ctx, std::array<std::array<uint64_t, SW>, N>& L)
{
    for (unsigned int r = 0; r < N; ++r)
    {
        L[r].fill(0);
        const unsigned int diag_word = r / 64;
        const unsigned int diag_bit = r % 64;
        for (unsigned int w = 0; w < diag_word; ++w)
            L[r][w] = squeeze_u64(ctx);
        L[r][diag_word] = (squeeze_u64(ctx) & lower_mask(diag_bit)) | (UINT64_C(1) << diag_bit);
    }
}

// Sample an upper triangular matrix with 1s on diagonal
template <std::size_t N, std::size_t W, std::size_t SW>
void sample_upper_triangular(hash_state& ctx, std::array<std::array<uint64_t, SW>, N>& U)
{
    for (unsigned int r = 0; r < N; ++r)
    {
        U[r].fill(0);
        const unsigned int diag_word = r / 64;
        const unsigned int diag_bit = r % 64;
        U[r][diag_word] = (squeeze_u64(ctx) & upper_mask(diag_bit)) | (UINT64_C(1) << diag_bit);
        for (unsigned int w = diag_word + 1; w < W; ++w)
            U[r][w] = squeeze_u64(ctx);
    }
}

// GF(2) matrix multiplication: C = A * B
template <std::size_t N, std::size_t W, std::size_t SW>
void matrix_mul_gf2(const std::array<std::array<uint64_t, SW>, N>& A,
                    const std::array<std::array<uint64_t, SW>, N>& B,
                    std::array<std::array<uint64_t, SW>, N>& C)
{
    for (unsigned int r = 0; r < N; ++r)
        C[r].fill(0);

    for (unsigned int r = 0; r < N; ++r)
    {
        for (unsigned int w = 0; w < W; ++w)
        {
            uint64_t x = A[r][w];
            while (x)
            {
                const unsigned int bit = static_cast<unsigned int>(__builtin_ctzll(x));
                const unsigned int c = w * 64 + bit;
                if (c < N)
                {
                    for (unsigned int j = 0; j < W; ++j)
                        C[r][j] ^= B[c][j];
                }
                x &= (x - 1);
            }
        }
    }
}

// Sample a random invertible matrix M = upper * lower
template <std::size_t N, std::size_t W, std::size_t SW>
void sample_matrix(hash_state& ctx, std::array<std::array<uint64_t, SW>, N>& M)
{
    std::array<std::array<uint64_t, SW>, N> lower_mat, upper_mat;
    sample_lower_triangular<N, W, SW>(ctx, lower_mat);
    sample_upper_triangular<N, W, SW>(ctx, upper_mat);
    matrix_mul_gf2<N, W, SW>(upper_mat, lower_mat, M);
}

// Initialize matrices from seed using LU-sampling
// Matches the reference lynx-sig/lynx_matrices.c lynx_init()
template <secpar S>
void lynx_init(lynx_matrices<S>& mats, const uint8_t* seed, std::size_t seed_len)
{
    constexpr unsigned int lambda = static_cast<unsigned int>(secpar_to_bits(S));
    constexpr unsigned int bytes = lambda / 8;
    constexpr unsigned int W = lynx_matrices<S>::words;
    constexpr unsigned int SW = lynx_matrices<S>::storage_words;

    // Initialize selected XOF with domain separator
    hash_state ctx;
    ctx.init(S);

    // Domain separation: "LYNX-SAMPLEMATVEC-v1" || lambda_le || seed_len_le || seed
    const uint8_t domain[] = "LYNX-SAMPLEMATVEC-v1";
    ctx.update(domain, sizeof(domain) - 1);

    const uint8_t lambda_le[2] = {static_cast<uint8_t>(lambda & 0xff),
                                  static_cast<uint8_t>((lambda >> 8) & 0xff)};
    ctx.update(lambda_le, sizeof(lambda_le));

    const uint8_t seed_len_le[2] = {static_cast<uint8_t>(seed_len & 0xff),
                                    static_cast<uint8_t>((seed_len >> 8) & 0xff)};
    ctx.update(seed_len_le, sizeof(seed_len_le));

    ctx.update(seed, seed_len);

    // Sample 3 matrices via LU decomposition (L3 is a fixed constant)
    sample_matrix<lambda, W, SW>(ctx, mats.L0);
    sample_matrix<lambda, W, SW>(ctx, mats.L1);
    sample_matrix<lambda, W, SW>(ctx, mats.L2);
    const auto l3 = lynx_l3_matrix<S>();
    mats.L3.fill({});
    uint64_t* l3_flat = mats.L3[0].data();
    for (unsigned int r = 0; r < lambda; ++r)
        for (unsigned int w = 0; w < W; ++w)
            l3_flat[r * W + w] = l3[r][w];

    // Squeeze constants
    ctx.finalize(mats.C0.data(), bytes);
    ctx.finalize(mats.C1.data(), bytes);
    if constexpr (S == secpar::s256 || S == secpar::s384 || S == secpar::s512)
    {
        ctx.finalize(mats.C2.data(), bytes);
    }
}

} // namespace lynx
} // namespace sig

#endif
