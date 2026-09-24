#ifndef SYDO_PROFILE_HPP
#define SYDO_PROFILE_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace sydo::profile
{

constexpr size_t STAGE_COUNT = 88;

enum stage : size_t
{
    SIGN_MU = 0,
    SIGN_SEED_IV = 1,
    SIGN_VEC_COMMIT = 2,
    SIGN_HASH_LEAVES = 3,
    SIGN_VOLE_SENDER = 4,
    SIGN_CHAL1 = 5,
    SIGN_VOLE_CHECK_CHAL2 = 6,
    SIGN_QS_OWF_PROVE = 7,
    SIGN_GRIND_OPEN = 8,

    VERIFY_MU_IV = 16,
    VERIFY_VEC_VERIFY = 17,
    VERIFY_HASH_LEAVES = 18,
    VERIFY_VOLE_RECEIVER = 19,
    VERIFY_CHAL1 = 20,
    VERIFY_VOLE_CHECK_CHAL2 = 21,
    VERIFY_QS_OWF_VERIFY = 22,
    VERIFY_DELTA_CHECK = 23,

    BAVC_COMMIT_ROOTS = 32,
    BAVC_COMMIT_TREE = 33,
    BAVC_COMMIT_LEAVES = 34,
    BAVC_VERIFY_PARSE_OPENING = 35,
    BAVC_VERIFY_ROOTS = 36,
    BAVC_VERIFY_TREE = 37,
    BAVC_VERIFY_LEAVES = 38,
    CRH_LEAF_HASH_XOF = 39,

    OWF_FOLD_H_ROWS = 40,
    OWF_FOLD_PUBLIC_Y = 41,
    OWF_LOAD_BLOCK_CACHES = 42,
    OWF_EMIT_BLOCK_CONSTRAINTS = 43,
    OWF_FOLDED_IP_STAGE0 = 44,
    OWF_FOLDED_IP_REST = 45,
    SIGN_QS_PREP = 46,
    SIGN_QS_PROVE = 47,
    VERIFY_QS_PREP = 48,
    VERIFY_QS_VERIFY = 49,

    OWF_BALL_STAGE0_ZERO = 50,
    OWF_BALL_STAGE0_SINGLE = 51,
    OWF_BALL_STAGE0_PAIR = 52,
    OWF_BALL_STAGE0_TRIPLE = 53,
    OWF_BALL_STAGE0_PAIR_REDUCE = 54,
    OWF_BALL_STAGE0_SINGLE_REDUCE = 55,
    OWF_BALL_STAGE0_TRIPLE_COEFF = 56,
    OWF_BALL_STAGE0_TRIPLE_MUL = 57,
    OWF_BALL_STAGE0_TRIPLE_PAIR_MIX = 58,
    OWF_BALL_STAGE0_TRIPLE_SINGLE_MIX = 59,
    OWF_BALL_STAGE0_TRIPLE_OUT3 = 60,
    OWF_BALL_STAGE0_COEFFS = 61,

    SIGN_QS_ALLOC = 62,
    SIGN_QS_STATE_INIT = 63,
    SIGN_OWF_CONSTRAINTS_TOTAL = 64,
    SIGN_QS_FREE = 65,
    OWF_REST_STATE_COPY = 66,
    OWF_FINAL_FOLDED_CONSTRAINT = 67,
    OWF_PADDING_CONSTRAINTS = 68,
    OWF_SUBVECTOR_TOTAL = 69,
    OWF_PADDED_H_SUBVECTOR = 70,
    OWF_FOLDED_CONSTRAINT_ACCUMULATE = 71,
    OWF_EMIT_BLOCK0 = 72,
    OWF_EMIT_BLOCK1 = 73,
    OWF_EMIT_BLOCK2 = 74,
    OWF_EMIT_BLOCK3 = 75,
    OWF_LOAD_BLOCK0 = 76,
    OWF_LOAD_BLOCK1 = 77,
    OWF_LOAD_BLOCK2 = 78,
    OWF_LOAD_BLOCK3 = 79,
    OWF_EMIT_PROVER_COEFF_BUILD = 80,
    OWF_EMIT_PROVER_ADD_CONSTRAINT = 81,
};

#if !defined(SYDO_ENABLE_PROFILE)

inline void set_enabled(bool) {}
inline void set_all_stages_enabled(bool) {}
inline void set_stage_enabled(size_t, bool) {}
inline bool is_enabled() { return false; }
inline void reset() {}
inline uint64_t now_ns() { return 0; }
inline void add_time(size_t, uint64_t) {}
inline std::array<uint64_t, STAGE_COUNT> snapshot() { return {}; }

class scope
{
  public:
    explicit scope(size_t) {}
};

#else

void set_enabled(bool enabled);
void set_all_stages_enabled(bool enabled);
void set_stage_enabled(size_t stage, bool enabled);
bool is_enabled();
void reset();
uint64_t now_ns();
void add_time(size_t stage, uint64_t ns);
std::array<uint64_t, STAGE_COUNT> snapshot();

class scope
{
  public:
    explicit scope(size_t stage);
    ~scope();

  private:
    size_t stage_;
    uint64_t start_;
    bool active_;
};

#endif

} // namespace sydo::profile

#endif
