#ifndef SYDO_PARAMETERS_HPP
#define SYDO_PARAMETERS_HPP

#include "avx2/constants_impl.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace sydo
{

enum class secpar : std::size_t
{
    s128 = 128,
    s160 = 160,
    s192 = 192,
    s256 = 256,
    s512 = 512,
};

constexpr std::size_t secpar_to_bits(secpar s) { return std::to_underlying(s); }
constexpr std::size_t secpar_to_bytes(secpar s) { return secpar_to_bits(s) / 8; }

enum class owf : unsigned int
{
    rsd = 0,
};

enum class elementary_encoding : unsigned int
{
    hamming_sphere = 0,
    hamming_ball = 1,
};

enum class quicksilver_term : unsigned int
{
    leading = 0,
    constant = 1,
};

enum class prg
{
    aes_ctr,
    rijndael_ctr,
    blake2b_512,
    blake2s_512_bc,
};

enum class leaf_hash
{
    aes_ctr,
    aes_ctr_stat_bind,
    crh,
    rijndael_ctr,
    rijndael_ctr_stat_bind,
    blake2b_512,
    blake2s_512_bc,
};

template <std::size_t bits> struct block;
template <std::size_t bits> struct poly;

constexpr std::size_t ceil_log2(std::size_t x)
{
    std::size_t out = 0;
    std::size_t y = x - 1;
    while (y)
    {
        ++out;
        y >>= 1;
    }
    return out;
}

template <secpar S> using block_secpar = block<secpar_to_bits(S)>;
template <secpar S> using block_2secpar = block<2 * secpar_to_bits(S)>;
template <secpar S> using poly_secpar = poly<secpar_to_bits(S)>;
template <secpar S> using poly_2secpar = poly<2 * secpar_to_bits(S)>;

template <secpar S> struct aes_ctr_prg;
struct aes192_ctr_trunc160_prg;
template <secpar S> struct rijndael_ctr_prg;
struct blake2b_512_prg;
struct blake2s_512_bc_prg;
template <typename PRG> struct prg_leaf_hash;
template <typename PRG, uint32_t MAX_TWEAKS> struct stat_binding_leaf_hash;
template <secpar S> struct crh_leaf_hash;

template <secpar S, prg PRG> struct prg_type;
template <secpar S, prg PRG> using prg_type_t = typename prg_type<S, PRG>::type;
template <secpar S> struct prg_type<S, prg::aes_ctr>
{
    using type = aes_ctr_prg<S>;
};
template <secpar S> struct prg_type<S, prg::rijndael_ctr>
{
    using type = rijndael_ctr_prg<S>;
};
template <> struct prg_type<secpar::s512, prg::blake2b_512>
{
    using type = blake2b_512_prg;
};
template <> struct prg_type<secpar::s512, prg::blake2s_512_bc>
{
    using type = blake2s_512_bc_prg;
};

template <secpar S, uint32_t MAX_TWEAKS, leaf_hash LH> struct leaf_hash_type;
template <secpar S, uint32_t MAX_TWEAKS, leaf_hash LH>
using leaf_hash_type_t = typename leaf_hash_type<S, MAX_TWEAKS, LH>::type;
template <secpar S, uint32_t MAX_TWEAKS> struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::aes_ctr>
{
    using type = prg_leaf_hash<aes_ctr_prg<S>>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::aes_ctr_stat_bind>
{
    using type = stat_binding_leaf_hash<aes_ctr_prg<S>, MAX_TWEAKS>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::rijndael_ctr>
{
    using type = prg_leaf_hash<rijndael_ctr_prg<S>>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::rijndael_ctr_stat_bind>
{
    using type = stat_binding_leaf_hash<rijndael_ctr_prg<S>, MAX_TWEAKS>;
};
template <secpar S, uint32_t MAX_TWEAKS> struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::crh>
{
    using type = crh_leaf_hash<S>;
};
template <uint32_t MAX_TWEAKS>
struct leaf_hash_type<secpar::s512, MAX_TWEAKS, leaf_hash::blake2b_512>
{
    using type = prg_leaf_hash<blake2b_512_prg>;
};
template <uint32_t MAX_TWEAKS>
struct leaf_hash_type<secpar::s512, MAX_TWEAKS, leaf_hash::blake2s_512_bc>
{
    using type = prg_leaf_hash<blake2s_512_bc_prg>;
};

enum class bavc
{
    ggm_forest,
    one_tree,
};

template <secpar S, std::size_t TAU, std::size_t DELTA_BITS, prg TREE_PRG, leaf_hash LEAF_HASH,
          std::size_t VOLE_WIDTH_SHIFT>
struct ggm_forest_bavc;

template <secpar S, std::size_t TAU, std::size_t DELTA_BITS, prg TREE_PRG, leaf_hash LEAF_HASH,
          std::size_t VOLE_WIDTH_SHIFT, std::size_t OPENING_SEEDS_THRESHOLD>
struct one_tree_bavc;

template <secpar S, std::size_t TAU, std::size_t DELTA_BITS, prg TREE_PRG, leaf_hash LEAF_HASH,
          std::size_t VOLE_WIDTH_SHIFT, bavc BAVC, std::size_t BAVC_PARAM>
struct bavc_type;
template <secpar S, std::size_t TAU, std::size_t DELTA_BITS, prg TREE_PRG, leaf_hash LEAF_HASH,
          std::size_t VOLE_WIDTH_SHIFT, bavc BAVC, std::size_t BAVC_PARAM>
using bavc_type_t = typename bavc_type<S, TAU, DELTA_BITS, TREE_PRG, LEAF_HASH, VOLE_WIDTH_SHIFT,
                                       BAVC, BAVC_PARAM>::type;
template <secpar S, std::size_t TAU, std::size_t DELTA_BITS, prg TREE_PRG, leaf_hash LEAF_HASH,
          std::size_t VOLE_WIDTH_SHIFT>
struct bavc_type<S, TAU, DELTA_BITS, TREE_PRG, LEAF_HASH, VOLE_WIDTH_SHIFT, bavc::ggm_forest, 0>
{
    using type = ggm_forest_bavc<S, TAU, DELTA_BITS, TREE_PRG, LEAF_HASH, VOLE_WIDTH_SHIFT>;
};
template <secpar S, std::size_t TAU, std::size_t DELTA_BITS, prg TREE_PRG, leaf_hash LEAF_HASH,
          std::size_t VOLE_WIDTH_SHIFT, std::size_t BAVC_PARAM>
struct bavc_type<S, TAU, DELTA_BITS, TREE_PRG, LEAF_HASH, VOLE_WIDTH_SHIFT, bavc::one_tree,
                 BAVC_PARAM>
{
    using type =
        one_tree_bavc<S, TAU, DELTA_BITS, TREE_PRG, LEAF_HASH, VOLE_WIDTH_SHIFT, BAVC_PARAM>;
};

template <secpar S, std::size_t WITNESS_BITS_IN, std::size_t QS_DEGREE_IN,
          std::size_t OWF_NUM_CONSTRAINTS_IN, std::size_t OWF_MUL_C_PRE_IN,
          std::size_t OWF_MUL_C_IP_IN, std::size_t OWF_MUL_C_TOTAL_IN>
struct SYDO_OWF_CONSTANTS;
template <typename P> struct SYDO_CONSTANTS;

template <secpar S> struct security_parameters;

template <> struct security_parameters<secpar::s128>
{
    constexpr static elementary_encoding elementary_encoding_v = elementary_encoding::hamming_ball;
    constexpr static std::size_t rsd_n = 13113;
    constexpr static std::size_t rsd_w = 47;
    constexpr static std::size_t rsd_codim = 384;
    constexpr static std::size_t num_var_blocks = 2;
    constexpr static std::array<std::size_t, num_var_blocks> block_sizes = {2, 8};
    constexpr static std::array<std::size_t, num_var_blocks> block_wts = {1, 3};
    constexpr static std::size_t deg = 4;
};

template <> struct security_parameters<secpar::s160>
{
    constexpr static elementary_encoding elementary_encoding_v = elementary_encoding::hamming_ball;
    constexpr static std::size_t rsd_n = 16461;
    constexpr static std::size_t rsd_w = 59;
    constexpr static std::size_t rsd_codim = 480;
    constexpr static std::size_t num_var_blocks = 2;
    constexpr static std::array<std::size_t, num_var_blocks> block_sizes = {2, 8};
    constexpr static std::array<std::size_t, num_var_blocks> block_wts = {1, 3};
    constexpr static std::size_t deg = 4;
};

template <> struct security_parameters<secpar::s192>
{
    constexpr static elementary_encoding elementary_encoding_v = elementary_encoding::hamming_ball;
    constexpr static std::size_t rsd_n = 23298;
    constexpr static std::size_t rsd_w = 66;
    constexpr static std::size_t rsd_codim = 576;
    constexpr static std::size_t num_var_blocks = 2;
    constexpr static std::array<std::size_t, num_var_blocks> block_sizes = {3, 8};
    constexpr static std::array<std::size_t, num_var_blocks> block_wts = {1, 3};
    constexpr static std::size_t deg = 4;
};

template <> struct security_parameters<secpar::s256>
{
    constexpr static elementary_encoding elementary_encoding_v = elementary_encoding::hamming_ball;
    constexpr static std::size_t rsd_n = 26226;
    constexpr static std::size_t rsd_w = 94;
    constexpr static std::size_t rsd_codim = 768;
    constexpr static std::size_t num_var_blocks = 2;
    constexpr static std::array<std::size_t, num_var_blocks> block_sizes = {2, 8};
    constexpr static std::array<std::size_t, num_var_blocks> block_wts = {1, 3};
    constexpr static std::size_t deg = 4;
};

template <> struct security_parameters<secpar::s512>
{
    constexpr static elementary_encoding elementary_encoding_v = elementary_encoding::hamming_ball;
    constexpr static std::size_t rsd_n = 49941;
    constexpr static std::size_t rsd_w = 179;
    constexpr static std::size_t rsd_codim = 1456;
    constexpr static std::size_t num_var_blocks = 2;
    constexpr static std::array<std::size_t, num_var_blocks> block_sizes = {2, 8};
    constexpr static std::array<std::size_t, num_var_blocks> block_wts = {1, 3};
    constexpr static std::size_t deg = 4;
};

template <secpar S, std::size_t TAU, owf OWF = owf::rsd, prg VOLE_PRG = prg::aes_ctr,
          prg TREE_PRG = prg::aes_ctr, leaf_hash LEAF_HASH = leaf_hash::crh,
          std::size_t ZERO_BITS_IN_DELTA = 0,
          std::pair<bavc, std::size_t> BAVC = std::pair<bavc, std::size_t>{bavc::ggm_forest, 0},
          quicksilver_term QS_TERM = quicksilver_term::leading>
struct sydo_parameter_set
{
    using security_params = security_parameters<S>;

    constexpr static secpar secpar_v = S;
    constexpr static std::size_t tau_v = TAU;
    constexpr static owf owf_v = OWF;
    constexpr static elementary_encoding elementary_encoding_v =
        security_params::elementary_encoding_v;
    constexpr static quicksilver_term quicksilver_term_v = QS_TERM;
    constexpr static prg prg_v = S == secpar::s512 ? VOLE_PRG : prg::aes_ctr;
    constexpr static prg h_prg_v = prg_v;
    constexpr static prg vole_prg_v = VOLE_PRG;
    constexpr static prg tree_prg_v = TREE_PRG;
    constexpr static leaf_hash leaf_hash_v = LEAF_HASH;
    constexpr static std::size_t zero_bits_in_delta_v = ZERO_BITS_IN_DELTA;
    constexpr static bavc bavc_kind_v = BAVC.first;
    constexpr static std::size_t bavc_param_v = BAVC.second;

    static_assert(S != secpar::s512 ||
                      ((VOLE_PRG == prg::blake2b_512 && TREE_PRG == prg::blake2b_512 &&
                        LEAF_HASH == leaf_hash::blake2b_512) ||
                       (VOLE_PRG == prg::blake2s_512_bc && TREE_PRG == prg::blake2s_512_bc &&
                        LEAF_HASH == leaf_hash::blake2s_512_bc)),
                  "s512 must use a fixed 512-bit PRG for VOLE/tree PRG and leaf hash.");
    static_assert(S == secpar::s512 ||
                      (VOLE_PRG != prg::blake2b_512 && TREE_PRG != prg::blake2b_512 &&
                       VOLE_PRG != prg::blake2s_512_bc && TREE_PRG != prg::blake2s_512_bc),
                  "512-bit PRG modes are only allowed for s512.");

    constexpr static std::size_t secpar_bits = secpar_to_bits(S);
    constexpr static std::size_t secpar_bytes = secpar_to_bytes(S);
    constexpr static std::size_t delta_bits_v =
        secpar_bits - zero_bits_in_delta_v + ceil_log2(security_params::deg);
    constexpr static std::size_t unused_delta_bits_v = secpar_bits - delta_bits_v;

    constexpr static std::size_t rsd_n = security_params::rsd_n;
    constexpr static std::size_t rsd_w = security_params::rsd_w;
    constexpr static std::size_t rsd_codim = security_params::rsd_codim;
    constexpr static std::size_t num_var_blocks = security_params::num_var_blocks;
    constexpr static std::array<std::size_t, num_var_blocks> block_sizes =
        security_params::block_sizes;
    constexpr static std::array<std::size_t, num_var_blocks> block_wts = security_params::block_wts;
    constexpr static std::size_t deg = security_params::deg;

    constexpr static std::size_t input_reduced_bits = []
    {
        std::size_t total = 0;
        for (std::size_t b = 0; b < num_var_blocks; ++b)
        {
            if constexpr (elementary_encoding_v == elementary_encoding::hamming_sphere)
                total += block_sizes[b] - 1;
            else
                total += block_sizes[b];
        }
        return total;
    }();

    constexpr static std::size_t witness_rows = rsd_w;
    constexpr static std::size_t witness_raw_bits = rsd_w * input_reduced_bits;
    constexpr static std::size_t witness_bits = ((witness_raw_bits + 7) / 8) * 8;
    constexpr static std::size_t witness_bytes = witness_bits / 8;

    constexpr static std::size_t witness_extended_subvector_bits = []
    {
        std::size_t total = 0;
        for (std::size_t b = 0; b < num_var_blocks; ++b)
            total += block_sizes[b];
        return total;
    }();
    constexpr static std::size_t witness_extended_bits = rsd_w * witness_extended_subvector_bits;
    constexpr static std::size_t witness_extended_bytes = (witness_extended_bits + 7) / 8;

    constexpr static std::size_t prg_seed_bytes = secpar_bytes;
    constexpr static std::size_t y_vector_len =
        (security_params::rsd_codim + secpar_bits - 1) / secpar_bits;
    constexpr static std::size_t qs_extra_challenges = y_vector_len;
    constexpr static std::size_t packed_public_key_bytes =
        security_params::rsd_codim / 8 + prg_seed_bytes;
    constexpr static std::size_t unpacked_public_key_bytes =
        y_vector_len * secpar_bytes + prg_seed_bytes;
    constexpr static std::size_t public_key_bytes = packed_public_key_bytes;

    constexpr static std::size_t secret_seed_bytes = secpar_bytes;
    constexpr static std::size_t unpacked_secret_key_bytes =
        unpacked_public_key_bytes + secret_seed_bytes + witness_bytes;
    constexpr static std::size_t packed_secret_key_bytes =
        packed_public_key_bytes + secret_seed_bytes + witness_bytes;

    constexpr static std::size_t ncr(std::size_t n, std::size_t r)
    {
        if (r > n)
            return 0;
        if (r == 0 || r == n)
            return 1;
        const std::size_t k = (r < (n - r)) ? r : (n - r);
        std::size_t result = 1;
        for (std::size_t i = 1; i <= k; ++i)
            result = (result * (n - k + i)) / i;
        return result;
    }

    constexpr static std::size_t elementary_vector_len = rsd_n / rsd_w;

    constexpr static std::size_t sum_block_choose = []
    {
        std::size_t total = 0;
        for (std::size_t i = 0; i < num_var_blocks; ++i)
            total += ncr(block_sizes[i], block_wts[i] + 1);
        return total;
    }();

    constexpr static std::size_t exact_weight_constraints_per_subvector =
        (elementary_encoding_v == elementary_encoding::hamming_sphere) ? num_var_blocks : 0;
    constexpr static std::size_t membership_constraints_per_subvector =
        sum_block_choose + exact_weight_constraints_per_subvector;

    constexpr static std::size_t owf_mul_block_M(std::size_t block_idx)
    {
        if constexpr (elementary_encoding_v == elementary_encoding::hamming_sphere)
        {
            return ncr(block_sizes[block_idx], block_wts[block_idx]);
        }
        else
        {
            std::size_t total = 0;
            for (std::size_t t = 0; t <= block_wts[block_idx]; ++t)
                total += ncr(block_sizes[block_idx], t);
            return total;
        }
    }

    constexpr static std::size_t elementary_domain_size = []
    {
        std::size_t total = 1;
        for (std::size_t b = 0; b < num_var_blocks; ++b)
            total *= owf_mul_block_M(b);
        return total;
    }();

    static_assert(rsd_n % rsd_w == 0, "rsd_n must be divisible by rsd_w.");
    static_assert(elementary_vector_len <= elementary_domain_size,
                  "Actual elementary-vector segment length must fit in the block domain.");

    constexpr static std::size_t owf_mul_block_L(std::size_t block_idx)
    {
        std::size_t total = 0;
        for (std::size_t t = 1; t <= block_wts[block_idx]; ++t)
            total += ncr(block_sizes[block_idx], t);
        return total;
    }

    constexpr static std::size_t owf_mul_c_pre = []
    {
        std::size_t total = 0;
        for (std::size_t b = 0; b < num_var_blocks; ++b)
            for (std::size_t t = 2; t <= block_wts[b] + 1; ++t)
                total += ncr(block_sizes[b], t);
        return total;
    }();

    constexpr static std::size_t
    owf_mul_c_ip_for_order(const std::array<std::size_t, num_var_blocks>& order)
    {
        std::size_t total = 0;
        std::size_t prefix_w_sum = 0;
        for (std::size_t m = 0; m < num_var_blocks; ++m)
        {
            std::size_t suffix_M_prod = 1;
            for (std::size_t r = m + 1; r < num_var_blocks; ++r)
                suffix_M_prod *= owf_mul_block_M(order[r]);

            total += suffix_M_prod * (1 + prefix_w_sum) * owf_mul_block_L(order[m]);
            prefix_w_sum += block_wts[order[m]];
        }
        return total;
    }

    constexpr static std::size_t
    owf_mul_c_ip_min_impl(std::array<std::size_t, num_var_blocks>& order,
                          std::array<bool, num_var_blocks>& used, std::size_t depth,
                          std::size_t best_so_far,
                          std::array<std::size_t, num_var_blocks>& best_order)
    {
        if (depth == num_var_blocks)
        {
            const std::size_t c = owf_mul_c_ip_for_order(order);
            if (c < best_so_far)
            {
                best_order = order;
                return c;
            }
            return (c < best_so_far) ? c : best_so_far;
        }

        for (std::size_t b = 0; b < num_var_blocks; ++b)
        {
            if (used[b])
                continue;
            used[b] = true;
            order[depth] = b;
            best_so_far = owf_mul_c_ip_min_impl(order, used, depth + 1, best_so_far, best_order);
            used[b] = false;
        }
        return best_so_far;
    }

    constexpr static std::array<std::size_t, num_var_blocks> owf_mul_best_order = []
    {
        std::array<std::size_t, num_var_blocks> order{};
        std::array<bool, num_var_blocks> used{};
        std::array<std::size_t, num_var_blocks> best_order{};
        (void)owf_mul_c_ip_min_impl(order, used, 0, std::numeric_limits<std::size_t>::max(),
                                    best_order);
        return best_order;
    }();

    constexpr static std::array<std::size_t, num_var_blocks> FOLD_ORDER = owf_mul_best_order;
    constexpr static std::size_t owf_mul_c_ip = owf_mul_c_ip_for_order(FOLD_ORDER);
    constexpr static std::size_t owf_mul_c_total = owf_mul_c_pre + owf_mul_c_ip;
    constexpr static std::size_t owf_num_constraints =
        y_vector_len + rsd_w * membership_constraints_per_subvector;

    using OWF_CONSTS = SYDO_OWF_CONSTANTS<S, witness_bits, deg, owf_num_constraints,
                                            owf_mul_c_pre, owf_mul_c_ip, owf_mul_c_total>;
    using CONSTS = SYDO_CONSTANTS<sydo_parameter_set<S, TAU, OWF, VOLE_PRG, TREE_PRG, LEAF_HASH,
                                                         ZERO_BITS_IN_DELTA, BAVC, QS_TERM>>;

    constexpr static std::size_t PRG_VOLE_BLOCK_SIZE_SHIFT = 0;
    constexpr static std::size_t PRG_VOLE_BLOCKS_SHIFT =
        VOLE_BLOCK_SHIFT - PRG_VOLE_BLOCK_SIZE_SHIFT;
    constexpr static std::size_t VOLE_WIDTH_SHIFT =
        AES_PREFERRED_WIDTH_SHIFT - PRG_VOLE_BLOCKS_SHIFT;

    using bavc_t = bavc_type_t<S, tau_v, delta_bits_v, TREE_PRG, LEAF_HASH, VOLE_WIDTH_SHIFT,
                               BAVC.first, BAVC.second>;

    constexpr static bool use_grinding =
        (unused_delta_bits_v > 0) || (BAVC.first != bavc::ggm_forest);
    constexpr static std::size_t grinding_counter_size = use_grinding ? 4 : 0;

    constexpr static std::size_t salt_bytes = CONSTS::SALT_BYTES;
    constexpr static std::size_t digest_bytes = CONSTS::DIGEST_BYTES;
    constexpr static std::size_t commitment_bytes = CONSTS::COMMITMENT_BYTES;
    constexpr static std::size_t response_bytes = CONSTS::RESPONSE_BYTES;

    using prg_t =
        std::conditional_t<S == secpar::s160, aes192_ctr_trunc160_prg, prg_type_t<S, prg_v>>;
    using h_prg_t = prg_t;
    using leaf_hash_t = leaf_hash_type_t<S, tau_v, LEAF_HASH>;
    using tree_prg_t = prg_type_t<S, TREE_PRG>;
    using vole_prg_t = prg_type_t<S, VOLE_PRG>;
    using seed_block_t = block_secpar<S>;
    using digest_block_t = block_2secpar<S>;
    using field_t = poly_secpar<S>;
    using wide_field_t = poly_2secpar<S>;
};

template <secpar S, std::size_t TAU, prg VOLE_PRG, prg TREE_PRG, leaf_hash LEAF_HASH,
          std::size_t ZERO_BITS_IN_DELTA, std::size_t BAVC_PARAM,
          quicksilver_term QS_TERM = quicksilver_term::leading>
using sydo_rsd_parameter_set =
    sydo_parameter_set<S, TAU, owf::rsd, VOLE_PRG, TREE_PRG, LEAF_HASH, ZERO_BITS_IN_DELTA,
                       std::pair<bavc, std::size_t>{bavc::one_tree, BAVC_PARAM}, QS_TERM>;

template <typename P, quicksilver_term QS_TERM>
using sydo_with_quicksilver_term =
    sydo_parameter_set<P::secpar_v, P::tau_v, P::owf_v, P::vole_prg_v, P::tree_prg_v,
                       P::leaf_hash_v, P::zero_bits_in_delta_v,
                       std::pair<bavc, std::size_t>{P::bavc_kind_v, P::bavc_param_v}, QS_TERM>;

using sydo_128_s = sydo_rsd_parameter_set<secpar::s128, 11, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 9, 102>;
using sydo_128_f = sydo_rsd_parameter_set<secpar::s128, 16, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 2, 110>;
using sydo_160_s = sydo_rsd_parameter_set<secpar::s160, 14, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 8, 132>;
using sydo_160_f = sydo_rsd_parameter_set<secpar::s160, 20, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 2, 138>;
using sydo_192_s = sydo_rsd_parameter_set<secpar::s192, 17, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 7, 162>;
using sydo_192_f = sydo_rsd_parameter_set<secpar::s192, 24, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 2, 163>;
using sydo_256_s = sydo_rsd_parameter_set<secpar::s256, 23, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 5, 225>;
using sydo_256_f = sydo_rsd_parameter_set<secpar::s256, 32, prg::rijndael_ctr,
                                          prg::rijndael_ctr, leaf_hash::rijndael_ctr, 2, 236>;
using sydo_512_s = sydo_rsd_parameter_set<secpar::s512, 46, prg::blake2s_512_bc,
                                          prg::blake2s_512_bc, leaf_hash::blake2s_512_bc, 8, 445>;
using sydo_512_f = sydo_rsd_parameter_set<secpar::s512, 64, prg::blake2s_512_bc,
                                          prg::blake2s_512_bc, leaf_hash::blake2s_512_bc, 2, 446>;

#define ALL_SYDO_INSTANCES                                                                       \
    sydo_128_s, sydo_128_f, sydo_160_s, sydo_160_f, sydo_192_s, sydo_192_f, sydo_256_s,           \
        sydo_256_f, sydo_512_s, sydo_512_f

} // namespace sydo

#endif
