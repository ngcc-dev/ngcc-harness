#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include <cstdint>
#include <type_traits>
#include <utility>

namespace sig
{

// Helper for dependent static_assert(false) in if-constexpr branches.
// GCC 12 in C++20 mode evaluates static_assert(false) even in non-taken branches.
template<auto...> inline constexpr bool dependent_false = false;

// Enum representing the security parameter
enum class secpar : std::size_t
{
    s128 = 128,
    s160 = 160,
    s192 = 192,
    s256 = 256,
    s384 = 384,
    s512 = 512,
};

// Convert the security parameter to the corresponding number of bits
constexpr std::size_t secpar_to_bits(secpar s)
{
    return static_cast<std::underlying_type_t<secpar>>(s);
}
// Convert the security parameter to the corresponding number of bytes
constexpr std::size_t secpar_to_bytes(secpar s) { return secpar_to_bits(s) / 8; }

namespace
{
constexpr unsigned int owf_algo_ecb = 0;
constexpr unsigned int owf_algo_lynx = 1;
constexpr unsigned int owf_algo_em = 2;
constexpr unsigned int owf_algo_shift = 8;
constexpr unsigned int owf_flag_zero_sboxes = 0b0001;
constexpr unsigned int owf_flag_norm_proof = 0b0010;
constexpr unsigned int owf_flag_shrunk_keyspace = 0b0100;
constexpr unsigned int owf_flag_ctr_input = 0b1000;
} // namespace

// Enum of the supported one-way functions
enum class owf : unsigned int
{
    aes_ecb = owf_algo_ecb << owf_algo_shift,
    aes_em = owf_algo_em << owf_algo_shift,
    lynx = owf_algo_lynx << owf_algo_shift,
    aes_ecb_with_zero_sboxes = aes_ecb | owf_flag_zero_sboxes,
    aes_em_with_zero_sboxes = aes_em | owf_flag_zero_sboxes,
    lynx_with_zero_sboxes = lynx | owf_flag_zero_sboxes,
    aes_ecb_with_zero_sboxes_and_norm_proof = aes_ecb_with_zero_sboxes | owf_flag_norm_proof,
    aes_em_with_zero_sboxes_and_norm_proof = aes_em_with_zero_sboxes | owf_flag_norm_proof,
    lynx_with_zero_sboxes_and_norm_proof = lynx_with_zero_sboxes | owf_flag_norm_proof,
    v1 = aes_ecb,
    v1_em = aes_em,
    v2 = aes_ecb | owf_flag_zero_sboxes | owf_flag_norm_proof | owf_flag_shrunk_keyspace |
         owf_flag_ctr_input,
    v2_em = aes_em | owf_flag_zero_sboxes | owf_flag_norm_proof | owf_flag_shrunk_keyspace,
};

constexpr bool is_owf_with_aes_ecb(owf o)
{
    return (static_cast<std::underlying_type_t<owf>>(o) >> owf_algo_shift) == owf_algo_ecb;
}

constexpr bool is_owf_with_aes_em(owf o)
{
    return (static_cast<std::underlying_type_t<owf>>(o) >> owf_algo_shift) == owf_algo_em;
}

constexpr bool is_owf_with_lynx(owf o)
{
    return (static_cast<std::underlying_type_t<owf>>(o) >> owf_algo_shift) == owf_algo_lynx;
}

constexpr bool is_owf_with_zero_sboxes(owf o)
{
    return static_cast<std::underlying_type_t<owf>>(o) & owf_flag_zero_sboxes;
}

constexpr bool is_owf_with_norm_proof(owf o) { return static_cast<std::underlying_type_t<owf>>(o) & owf_flag_norm_proof; }

constexpr bool is_owf_with_shrunk_keyspace(owf o)
{
    return static_cast<std::underlying_type_t<owf>>(o) & owf_flag_shrunk_keyspace;
}

constexpr bool is_owf_with_ctr_input(owf o) { return static_cast<std::underlying_type_t<owf>>(o) & owf_flag_ctr_input; }

// Enum of the supported PRGs
enum class prg
{
    aes_ctr,
    ref_tccr,
    ref_shacal2,
    rijndael_fixed_key_ctr,
    shake_wide,
    tccr_hash,  // TCCR-based tree expansion (ref-compatible)
};

// Enum of the supported leaf hashes
enum class leaf_hash
{
    aes_ctr,
    aes_ctr_stat_bind,
    rijndael_fixed_key_ctr,
    rijndael_fixed_key_ctr_stat_bind,
    tccr_prg,
    shacal2_prg,
    shake,
};

// Defined in prgs.hpp
template <secpar S> struct aes_ctr_prg;
template <secpar S> struct ref_tccr_prg;
template <secpar S> struct ref_shacal2_prg;
template <secpar S> struct rijndael_fixed_key_ctr_prg;
template <secpar S> struct shake_wide_prg;

// Defined in vector_com.hpp
template <typename PRG> struct prg_leaf_hash;
template <typename PRG, uint32_t MAX_TWEAKS> struct stat_binding_leaf_hash;
template <secpar S> struct shake_leaf_hash;

// Template to obtain the PRG type corresponding to a prg enum value
template <secpar S, prg PRG> struct prg_type;
template <secpar S, prg PRG> using prg_type_t = prg_type<S, PRG>::type;
template <secpar S> struct prg_type<S, prg::aes_ctr>
{
    using type = aes_ctr_prg<S>;
};
template <secpar S> struct prg_type<S, prg::ref_tccr>
{
    using type = ref_tccr_prg<S>;
};
template <secpar S> struct prg_type<S, prg::ref_shacal2>
{
    using type = ref_shacal2_prg<S>;
};
template <secpar S> struct prg_type<S, prg::rijndael_fixed_key_ctr>
{
    using type = rijndael_fixed_key_ctr_prg<S>;
};
template <secpar S> struct prg_type<S, prg::shake_wide>
{
    using type = shake_wide_prg<S>;
};

// Forward declaration for TCCR hash PRG (defined in prgs.hpp)
template <secpar S> struct tccr_hash_prg;
template <secpar S> struct prg_type<S, prg::tccr_hash>
{
    using type = tccr_hash_prg<S>;
};

// Template to obtain the leaf hash type corresponding to a leaf_hash enum value
template <secpar S, uint32_t MAX_TWEAKS, leaf_hash LH> struct leaf_hash_type;
template <secpar S, uint32_t MAX_TWEAKS, leaf_hash LH>
using leaf_hash_type_t = leaf_hash_type<S, MAX_TWEAKS, LH>::type;
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::aes_ctr>
{
    using type = prg_leaf_hash<aes_ctr_prg<S>>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::aes_ctr_stat_bind>
{
    using type = stat_binding_leaf_hash<aes_ctr_prg<S>, MAX_TWEAKS>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::rijndael_fixed_key_ctr>
{
    using type = prg_leaf_hash<rijndael_fixed_key_ctr_prg<S>>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::rijndael_fixed_key_ctr_stat_bind>
{
    using type = stat_binding_leaf_hash<rijndael_fixed_key_ctr_prg<S>, MAX_TWEAKS>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::tccr_prg>
{
    static_assert(S == secpar::s384, "TCCR leaf PRG is defined for Lynx CSP 384");
    using type = prg_leaf_hash<ref_tccr_prg<S>>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::shacal2_prg>
{
    static_assert(S == secpar::s512, "SHACAL-2 leaf PRG is defined for Lynx CSP 512");
    using type = prg_leaf_hash<ref_shacal2_prg<S>>;
};
template <secpar S, uint32_t MAX_TWEAKS>
struct leaf_hash_type<S, MAX_TWEAKS, leaf_hash::shake>
{
    using type = shake_leaf_hash<S>;
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
using bavc_type_t =
    bavc_type<S, TAU, DELTA_BITS, TREE_PRG, LEAF_HASH, VOLE_WIDTH_SHIFT, BAVC, BAVC_PARAM>::type;
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

// Defined in constants.hpp
template <secpar S, owf P> struct OWF_CONSTANTS;
template <typename P> struct CONSTANTS;

// Template describing a particular VOLEitH-style signature instance.
//
// The parameters are
// - the security parameter
// - tau = number of bits per witness bit
//       = number of GGM trees
// - the one-way function to prove
// - the PRG used for the VOLEs
// - the PRG used for inner nodes in the GGM trees
// - the PRG used for leaf nodes of the GGM trees
// - number of zero bits in Delta
template <secpar S, std::size_t TAU, owf OWF, prg VOLE_PRG, prg TREE_PRG = prg::aes_ctr,
          leaf_hash LEAF_HASH = leaf_hash::shake, std::size_t ZERO_BITS_IN_DELTA = 0,
          std::pair<bavc, std::size_t> BAVC = {bavc::ggm_forest, 0}>
struct parameter_set
{
    // Values of the template parameters as constants
    constexpr static secpar secpar_v = S;
    constexpr static std::size_t tau_v = TAU;
    constexpr static owf owf_v = OWF;
    constexpr static prg vole_prg_v = VOLE_PRG;
    constexpr static prg tree_prg_v = TREE_PRG;
    constexpr static leaf_hash leaf_hash_v = LEAF_HASH;
    constexpr static std::size_t zero_bits_in_delta_v = ZERO_BITS_IN_DELTA;

    // Shorthands for the security parameter in bits and bytes
    constexpr static std::size_t secpar_bits = secpar_to_bits(S);
    constexpr static std::size_t secpar_bytes = secpar_to_bytes(S);

    // Size of Delta
    constexpr static std::size_t delta_bits_v = secpar_bits - zero_bits_in_delta_v;

    // Access to the implementation constants that depend on the parameters
    using CONSTS = CONSTANTS<
        parameter_set<S, TAU, OWF, VOLE_PRG, TREE_PRG, LEAF_HASH, ZERO_BITS_IN_DELTA, BAVC>>;
    // Access to the one-way function constants that depend on the parameters
    using OWF_CONSTS = OWF_CONSTANTS<S, OWF>;

    // The types of the selected PRGs
    using leaf_hash_t = leaf_hash_type_t<S, TAU, LEAF_HASH>;
    using tree_prg_t = prg_type_t<S, TREE_PRG>;
    using vole_prg_t = prg_type_t<S, VOLE_PRG>;

    // The batched all-but-one vector commitment
    using bavc_t = bavc_type_t<S, TAU, delta_bits_v, TREE_PRG, LEAF_HASH, CONSTS::VOLE_WIDTH_SHIFT,
                               BAVC.first, BAVC.second>;

    constexpr static bool use_grinding =
        (zero_bits_in_delta_v > 0) || !bavc_t::OPEN_ALWAYS_SUCCEEDS;
    constexpr static std::size_t grinding_counter_size = []
    {
        if (use_grinding)
            return 4;
        else
            return 0;
    }();
};

// The Lynx NGCC instances (matching lynx-ref parameter sets)
// All use owf::lynx, bavc::one_tree, degree-2 proofs
// Witness bits = 3 * lambda (k || v1 || v2)
namespace lynx
{

// --- 160-bit ---
using lynx_160_s = parameter_set<secpar::s160, 14, owf::lynx,
    prg::aes_ctr, prg::tccr_hash, leaf_hash::aes_ctr, 6, {bavc::one_tree, 129}>;   // TAU=14, POW=6, T_open=129
using lynx_160_f = parameter_set<secpar::s160, 21, owf::lynx,
    prg::aes_ctr, prg::tccr_hash, leaf_hash::aes_ctr, 8, {bavc::one_tree, 139}>;   // TAU=21, POW=8, T_open=139

// --- 256-bit ---
using lynx_256_s = parameter_set<secpar::s256, 22, owf::lynx,
    prg::aes_ctr, prg::tccr_hash, leaf_hash::aes_ctr, 12, {bavc::one_tree, 224}>;   // TAU=22, POW=12, T_open=224
using lynx_256_f = parameter_set<secpar::s256, 35, owf::lynx,
    prg::aes_ctr, prg::tccr_hash, leaf_hash::aes_ctr, 8, {bavc::one_tree, 223}>;    // TAU=35, POW=8, T_open=223

// --- 384-bit ---
using lynx_384_s = parameter_set<secpar::s384, 34, owf::lynx,
    prg::ref_tccr, prg::tccr_hash, leaf_hash::tccr_prg, 10, {bavc::one_tree, 332}>;
using lynx_384_f = parameter_set<secpar::s384, 53, owf::lynx,
    prg::ref_tccr, prg::tccr_hash, leaf_hash::tccr_prg, 9, {bavc::one_tree, 336}>;

// --- 512-bit ---
using lynx_512_s = parameter_set<secpar::s512, 46, owf::lynx,
    prg::ref_shacal2, prg::tccr_hash, leaf_hash::shacal2_prg, 6, {bavc::one_tree, 439}>;
using lynx_512_f = parameter_set<secpar::s512, 72, owf::lynx,
    prg::ref_shacal2, prg::tccr_hash, leaf_hash::shacal2_prg, 8, {bavc::one_tree, 447}>;

} // namespace lynx

// Lynx NGCC instances (8 parameter sets matching lynx-ref)
// 160-bit sets disabled pending poly<160> implementation in polynomials_impl.hpp
#define ALL_LYNX_INSTANCES                                                                         \
    lynx::lynx_160_s, lynx::lynx_160_f,                                                           \
        lynx::lynx_256_s, lynx::lynx_256_f, lynx::lynx_384_s, lynx::lynx_384_f,                   \
        lynx::lynx_512_s, lynx::lynx_512_f

} // namespace sig

#endif
