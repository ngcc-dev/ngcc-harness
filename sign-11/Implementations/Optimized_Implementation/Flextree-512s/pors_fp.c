#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pors_fp.h"
#include "address.h"
#include "auxfunc.h"
#include "hash.h"
#include "hashx8.h"
#include "thash.h"
#include "thashx8.h"
#include "utils.h"
#include "pors_precomputed.h"

typedef struct {
    uint32_t level;
    uint32_t index;
} pors_node_pos;

typedef struct {
    uint32_t pos;
    unsigned char value[SPX_N];
} pors_node_value;

#define PORS_AUTH_NODE_CAPACITY ((size_t)SPX_PORS_FP_K * (size_t)PORS_TREE_HEIGHT_CONST)

static int pors_append_unique_u32(uint32_t *out, size_t *out_len, uint32_t value)
{
    if (*out_len == 0 || out[*out_len - 1] != value) {
        out[*out_len] = value;
        (*out_len)++;
    }
    return 0;
}

static int pors_merge_sorted_u32(uint32_t *dst, size_t *dst_len,
                                 const uint32_t *left, size_t left_len,
                                 const uint32_t *right, size_t right_len)
{
    size_t i = 0;
    size_t j = 0;
    size_t out_len = 0;

    while (i < left_len || j < right_len) {
        uint32_t value;

        if (j >= right_len || (i < left_len && left[i] < right[j])) {
            value = left[i++];
        } else if (i >= left_len || right[j] < left[i]) {
            value = right[j++];
        } else {
            value = left[i];
            i++;
            j++;
        }

        if (out_len == 0 || dst[out_len - 1] != value) {
            dst[out_len++] = value;
        }
    }

    *dst_len = out_len;
    return 0;
}

#if PORS_BINOM_U64_MAX_R >= 2u
static uint64_t pors_binom_u64(uint32_t n, uint32_t r)
{
    uint64_t c = 1u;
    uint32_t j;

    if (r > n) {
        return 0u;
    }
    if (r > n - r) {
        r = n - r;
    }

    for (j = 1u; j <= r; j++) {
        const uint32_t num = n - r + j;
        const pors_u128 v = (pors_u128)c * num;
        c = (uint64_t)(v / j);
    }

    return c;
}

static uint32_t pors_binom_lower_bound_u64(pors_u256 *out,
                                           uint32_t lo,
                                           uint32_t hi,
                                           uint32_t q,
                                           uint64_t target)
{
    uint32_t n;
    uint64_t cn;

    while (lo < hi) {
        const uint32_t mid = lo + ((hi - lo) >> 1);
        const uint64_t cmid = pors_binom_u64(mid, q);

        if (cmid >= target) {
            hi = mid;
        } else {
            lo = mid + 1u;
        }
    }

    n = lo;
    cn = pors_binom_u64(n, q);
    pors_u256_set_u64(out, cn);
    return n;
}
#endif

static uint32_t pors_binom_lower_bound(pors_u256 *out,
                                       uint32_t lo,
                                       uint32_t hi,
                                       uint32_t q,
                                       const pors_u256 *target)
{
#if PORS_BINOM_STRIDE == 1u
    while (lo < hi) {
        const uint32_t mid = lo + ((hi - lo) >> 1);
        pors_u256 cmid;

        pors_precomp_binom_get(&cmid, mid, q);

        if (pors_u256_cmp(&cmid, target) >= 0) {
            hi = mid;
        } else {
            lo = mid + 1u;
        }
    }

    pors_precomp_binom_get(out, lo, q);
    return lo;
#else
    pors_precomp_binom_table table;
    uint32_t start;
    uint32_t end;
    const uint32_t first_block =
        (lo + PORS_BINOM_STRIDE - 1u) / PORS_BINOM_STRIDE;
    const uint32_t last_block = hi / PORS_BINOM_STRIDE;
    const uint32_t table_first_block =
        (q + PORS_BINOM_STRIDE - 1u) / PORS_BINOM_STRIDE;
    const uint32_t target_len = pors_u256_significant_limbs(target);

    if (pors_precomp_binom_table_get(&table, q) != 0) {
        pors_u256_zero(out);
        return lo;
    }

    if (first_block <= last_block) {
        uint32_t block_lo = first_block;
        uint32_t block_hi = last_block + 1u;

        while (block_lo < block_hi) {
            const uint32_t mid_block = block_lo + ((block_hi - block_lo) >> 1);
            const uint32_t row = mid_block - table_first_block;

            if (pors_precomp_binom_table_cmp_sig(&table, row, target,
                                                 target_len) >= 0) {
                block_hi = mid_block;
            } else {
                block_lo = mid_block + 1u;
            }
        }

        if (block_lo <= last_block) {
            end = block_lo * PORS_BINOM_STRIDE;
            if (block_lo == 0u) {
                start = lo;
            } else {
                start = (block_lo - 1u) * PORS_BINOM_STRIDE + 1u;
            }
        } else {
            const uint32_t last_base = last_block * PORS_BINOM_STRIDE;
            start = last_base + 1u;
            end = hi;
        }

        if (start < lo) {
            start = lo;
        }
        if (end > hi) {
            end = hi;
        }
    } else {
        start = lo;
        end = hi;
    }

    pors_precomp_binom_get(out, start, q);

    while (start < end && pors_u256_cmp(out, target) < 0) {
        pors_precomp_step_binom_up(out, start, q);
        start++;
    }

    return start;
#endif
}

static void pors_hpors_reduce_bytes(pors_u256 *out,
                                    const unsigned char hpors[PORS_HPORS_INPUT_BYTES])
{
    size_t i;

    pors_u256_zero(out);
    for (i = 0; i < PORS_HPORS_INPUT_BYTES; i++) {
        const unsigned int byte = hpors[i];

        if (byte != 0u) {
            pors_u256_add_mod_assign(out, &pors_hpors_byte_mod_table[i][byte],
                                      &pors_total_combination);
        }
    }
}

//value = H_PORS(R||ctr||m) %mod
static int pors_hpors_mod(pors_u256 *value, const unsigned char *R,
                          const unsigned char *m, uint32_t ctr)
{
    unsigned char input[SPX_N + COUNTER_SIZE + SPX_PORS_FP_MSG_BYTES];
    unsigned char hpors[PORS_HPORS_INPUT_BYTES];
    unsigned char ctr_bytes[COUNTER_SIZE];

    ull_to_bytes(ctr_bytes, COUNTER_SIZE, ctr);
    memcpy(input, R, SPX_N);
    memcpy(input + SPX_N, ctr_bytes, COUNTER_SIZE);
    memcpy(input + SPX_N + COUNTER_SIZE, m, SPX_PORS_FP_MSG_BYTES);

    pseudoXOF((unsigned long long)sizeof(hpors) * 8u,
              input, (unsigned long long)sizeof(input) * 8u, hpors);
    pors_hpors_reduce_bytes(value, hpors);

    return 0;
}

//Algorithm 3 mapping rho
static int pors_decode_combination(uint32_t *indices, const pors_u256 *value)
{
    pors_u256 r = *value;
    uint32_t p = 0;
    uint32_t i;

    if (SPX_PORS_FP_T < SPX_PORS_FP_K ||
        pors_u256_cmp(&r, &pors_total_combination) >= 0) {
        return -1;
    }

    for (i = 1; i <= SPX_PORS_FP_K; i++) {
        const uint32_t q = SPX_PORS_FP_K - i + 1u;
        const uint32_t lo = q;
        const uint32_t hi = SPX_PORS_FP_T - p;

        uint32_t n;
        uint32_t x;
        pors_u256 base;
        pors_u256 cn;
        pors_u256 target;

        pors_precomp_binom_get(&base, SPX_PORS_FP_T - p, q);
        if (pors_u256_cmp(&base, &r) <= 0) {
            return -1;
        }

        pors_u256_sub(&target, &base, &r);

        if (q == 1u) {
            if (pors_u256_to_u32_checked(&n, &target) != 0) {
                return -1;
            }
            if (n < lo || n > hi) {
                return -1;
            }
            pors_u256_set_u32(&cn, n);
#if PORS_BINOM_U64_MAX_R >= 2u
        } else if (q <= PORS_BINOM_U64_MAX_R) {
            uint64_t target64;

            if (pors_u256_to_u64_checked(&target64, &target) != 0) {
                return -1;
            }
            n = pors_binom_lower_bound_u64(&cn, lo, hi, q, target64);
#endif
        } else {
            n = pors_binom_lower_bound(&cn, lo, hi, q, &target);
        }

        x = SPX_PORS_FP_T - n;
        if (x < p || x > SPX_PORS_FP_T - q) {
            return -1;
        }

        pors_u256_sub(&r, &cn, &target);

        indices[i - 1u] = x;
        p = x + 1u;
    }

    return pors_u256_is_zero(&r) ? 0 : -1;
}

//mid Algorithm
int pors_message_to_indices(uint32_t *indices, const unsigned char *R,
                            const unsigned char *m, uint32_t ctr)
{
    pors_u256 value;

    if (pors_hpors_mod(&value, R, m, ctr) != 0) {
        return -1;
    }

    return pors_decode_combination(indices, &value);
}

//Algorithm 15
//Octopus算法可以提前终止，优化效果需要看具体分布
static int pors_octopus_positions(pors_node_pos *auth_nodes, size_t *auth_len,
                                  const uint32_t *indices)
{
    uint32_t current[SPX_PORS_FP_K];//I
    uint32_t upper[SPX_PORS_FP_K];//P
    uint32_t parents[SPX_PORS_FP_K];
    uint32_t merged[SPX_PORS_FP_K];
    size_t current_len = 0;
    size_t upper_len = 0;
    size_t total_auth = 0;
    uint32_t level;
    unsigned int i;

    //no value, just index in layer
    for (i = 0; i < SPX_PORS_FP_K; i++) {
        if (indices[i] < 2u * PORS_SHORT_LEAF_START_CONST) {
            current[current_len++] = indices[i];
        } else {
            upper[upper_len++] = PORS_SHORT_LEAF_START_CONST +
                                 (indices[i] - 2u * PORS_SHORT_LEAF_START_CONST);
        }
    }

    for (level = PORS_TREE_HEIGHT_CONST; level > 0; level--) {
        size_t parent_len = 0;
        size_t j = 0;
        size_t auth_this_level = 0;

        while (j < current_len) {
            const uint32_t node = current[j];  //node \in I
            const uint32_t sibling = node ^ 1u;

            pors_append_unique_u32(parents, &parent_len, node >> 1);//append and remove the consecutive repeated values
            if (j + 1 < current_len && current[j + 1] == sibling) {
                j += 2;
                continue;
            }

            auth_nodes[total_auth + auth_this_level].level = level;
            auth_nodes[total_auth + auth_this_level].index = sibling;
            auth_this_level++;
            j++;
        }

        total_auth += auth_this_level;
        pors_merge_sorted_u32(merged, &current_len,
                              parents, parent_len, upper, upper_len);
        memcpy(current, merged, current_len * sizeof(uint32_t));
        upper_len = 0;
    }

    *auth_len = total_auth;
    return 0;
}

//return layer level,height = h-level
static size_t pors_level_count(uint32_t height)
{
    if (height == 0) {
        return (size_t)PORS_DEEP_LEAF_COUNT_CONST;
    }
    if (height == 1) {
        return (size_t)PORS_BASE_WIDTH_CONST;
    }
    return (size_t)1u << (PORS_TREE_HEIGHT_CONST - height);
}

static size_t pors_total_node_count(void)
{
    uint32_t height;
    size_t total = 0;

    for (height = 0; height <= PORS_TREE_HEIGHT_CONST; height++) {
        total += pors_level_count(height);
    }

    return total;
}

static void pors_level_offsets(size_t offsets[PORS_TREE_HEIGHT_CONST + 1u])
{
    uint32_t height;
    size_t off = 0;

    for (height = 0; height <= PORS_TREE_HEIGHT_CONST; height++) {
        offsets[height] = off;
        off += pors_level_count(height) * (size_t)SPX_N;
    }
}

static void pors_copy_selected_secret(unsigned char *secret_values,
                                      const uint32_t *indices,
                                      size_t *selected_pos,
                                      uint32_t leaf_idx,
                                      const unsigned char *sk)
{
    if (*selected_pos < SPX_PORS_FP_K && indices[*selected_pos] == leaf_idx) {
        memcpy(secret_values + (*selected_pos * (size_t)SPX_N), sk, SPX_N);
        (*selected_pos)++;
    }
}

static void pors_addrx8_from_indices(uint32_t out[8 * 8],
                                     const uint32_t in[8],
                                     const uint32_t indices[8])
{
    for (unsigned int j = 0; j < 8; j++) {
        memcpy(out + j * 8, in, 8 * sizeof(uint32_t));
        set_tree_index(out + j * 8, indices[j]);
    }
}

static void pors_hash_parent_batch(pors_node_value *parents,
                                   const unsigned char *inputs,
                                   const uint32_t parent_pos[8],
                                   unsigned int lanes,
                                   const spx_ctx *ctx,
                                   const uint32_t node_addr[8])
{
    if (lanes == 8u) {
        uint32_t addrx8[8 * 8];

        pors_addrx8_from_indices(addrx8, node_addr, parent_pos);
        thashx8(parents[0].value, parents[1].value,
                parents[2].value, parents[3].value,
                parents[4].value, parents[5].value,
                parents[6].value, parents[7].value,
                inputs + 0 * 2u * SPX_N,
                inputs + 1 * 2u * SPX_N,
                inputs + 2 * 2u * SPX_N,
                inputs + 3 * 2u * SPX_N,
                inputs + 4 * 2u * SPX_N,
                inputs + 5 * 2u * SPX_N,
                inputs + 6 * 2u * SPX_N,
                inputs + 7 * 2u * SPX_N,
                2, ctx, addrx8);
    } else {
        for (unsigned int j = 0; j < lanes; j++) {
            uint32_t addr[8];

            memcpy(addr, node_addr, 8 * sizeof(uint32_t));
            set_tree_index(addr, parent_pos[j]);
            thash(parents[j].value, inputs + j * 2u * SPX_N, 2, ctx, addr);
        }
    }
}

static void pors_build_leaf8(unsigned char *dst,
                             unsigned char *secret_values,
                             const uint32_t *indices,
                             size_t *selected_pos,
                             uint32_t leaf_idx,
                             const spx_ctx *ctx,
                             const uint32_t sk_addr_base[8],
                             const uint32_t leaf_addr_base[8])
{
    prf_addrx8_tree_index(dst + 0 * SPX_N,
                          dst + 1 * SPX_N,
                          dst + 2 * SPX_N,
                          dst + 3 * SPX_N,
                          dst + 4 * SPX_N,
                          dst + 5 * SPX_N,
                          dst + 6 * SPX_N,
                          dst + 7 * SPX_N,
                          ctx, sk_addr_base, leaf_idx);

    for (unsigned int j = 0; j < 8; j++) {
        pors_copy_selected_secret(secret_values, indices, selected_pos,
                                  leaf_idx + j, dst + j * SPX_N);
    }

    thashx8_tree_index(dst + 0 * SPX_N,
                       dst + 1 * SPX_N,
                       dst + 2 * SPX_N,
                       dst + 3 * SPX_N,
                       dst + 4 * SPX_N,
                       dst + 5 * SPX_N,
                       dst + 6 * SPX_N,
                       dst + 7 * SPX_N,
                       dst + 0 * SPX_N,
                       dst + 1 * SPX_N,
                       dst + 2 * SPX_N,
                       dst + 3 * SPX_N,
                       dst + 4 * SPX_N,
                       dst + 5 * SPX_N,
                       dst + 6 * SPX_N,
                       dst + 7 * SPX_N,
                       1, ctx, leaf_addr_base, leaf_idx);
}

static void pors_build_parent8(unsigned char *dst,
                               const unsigned char *child,
                               uint32_t parent_idx,
                               uint32_t height,
                               const spx_ctx *ctx,
                               const uint32_t node_addr_base[8])
{
    (void)height;

    thashx8_tree_index(dst + 0 * SPX_N,
                       dst + 1 * SPX_N,
                       dst + 2 * SPX_N,
                       dst + 3 * SPX_N,
                       dst + 4 * SPX_N,
                       dst + 5 * SPX_N,
                       dst + 6 * SPX_N,
                       dst + 7 * SPX_N,
                       child + 0 * 2 * SPX_N,
                       child + 1 * 2 * SPX_N,
                       child + 2 * 2 * SPX_N,
                       child + 3 * 2 * SPX_N,
                       child + 4 * 2 * SPX_N,
                       child + 5 * 2 * SPX_N,
                       child + 6 * 2 * SPX_N,
                       child + 7 * 2 * SPX_N,
                       2, ctx, node_addr_base, parent_idx);
}

static void pors_build_leaf8_tail(unsigned char *dst,
                                  unsigned char *secret_values,
                                  const uint32_t *indices,
                                  size_t *selected_pos,
                                  uint32_t leaf_idx,
                                  size_t lane_count,
                                  const spx_ctx *ctx,
                                  const uint32_t sk_addr_base[8],
                                  const uint32_t leaf_addr_base[8])
{
    for (size_t j = 0; j < lane_count; j++) {
        uint32_t sk_addr[8];
        uint32_t leaf_addr[8];
        unsigned char sk[SPX_N];

        memcpy(sk_addr, sk_addr_base, sizeof(sk_addr));
        set_tree_index(sk_addr, leaf_idx + (uint32_t)j);
        prf_addr(sk, ctx, sk_addr);

        pors_copy_selected_secret(secret_values, indices, selected_pos,
                                  leaf_idx + (uint32_t)j,
                                  sk);

        memcpy(leaf_addr, leaf_addr_base, sizeof(leaf_addr));
        set_tree_index(leaf_addr, leaf_idx + (uint32_t)j);
        thash(dst + j * SPX_N, sk, 1, ctx, leaf_addr);
    }
}

static void pors_build_parent8_tail(unsigned char *dst,
                                    const unsigned char *child,
                                    uint32_t parent_idx,
                                    uint32_t height,
                                    size_t lane_count,
                                    const spx_ctx *ctx,
                                    const uint32_t node_addr_base[8])
{
    (void)height;

    for (size_t j = 0; j < lane_count; j++) {
        uint32_t node_addr[8];

        memcpy(node_addr, node_addr_base, sizeof(node_addr));
        set_tree_index(node_addr, parent_idx + (uint32_t)j);
        thash(dst + j * SPX_N, child + j * 2 * SPX_N, 2, ctx, node_addr);
    }
}

static int pors_build_tree(unsigned char *root,
                                unsigned char *auth_values,
                                unsigned char *secret_values,
                                const uint32_t *indices,
                                const pors_node_pos *auth_nodes,
                                size_t auth_len,
                                const spx_ctx *ctx,
                                const uint32_t fors_addr[8])
{
    size_t offsets[PORS_TREE_HEIGHT_CONST + 1u];
    unsigned char *tree;
    uint32_t sk_addr[8] = {0};
    uint32_t leaf_addr[8] = {0};
    uint32_t node_addr[8] = {0};
    size_t selected_pos = 0;
    size_t total_nodes;
    uint32_t height;
    size_t i;

    pors_level_offsets(offsets);
    total_nodes = pors_total_node_count();
    tree = (unsigned char *)malloc(total_nodes * (size_t)SPX_N);
    if (tree == NULL) {
        return -1;
    }

    copy_keypair_addr(sk_addr, fors_addr);
    set_type(sk_addr, SPX_ADDR_TYPE_FORSPRF);
    set_tree_height(sk_addr, 0);

    copy_keypair_addr(leaf_addr, fors_addr);
    set_type(leaf_addr, SPX_ADDR_TYPE_FORSTREE);
    set_tree_height(leaf_addr, 0);

    copy_keypair_addr(node_addr, fors_addr);
    set_type(node_addr, SPX_ADDR_TYPE_FORSTREE);

    //layer h
    i = 0;
    for (; i + 7u < PORS_DEEP_LEAF_COUNT_CONST; i += 8u) {
        pors_build_leaf8(tree + offsets[0] + i * (size_t)SPX_N,
                         secret_values, indices, &selected_pos, (uint32_t)i,
                         ctx, sk_addr, leaf_addr);
    }
    if (i < PORS_DEEP_LEAF_COUNT_CONST) {
        pors_build_leaf8_tail(tree + offsets[0] + i * (size_t)SPX_N,
                              secret_values, indices, &selected_pos,
                              (uint32_t)i, PORS_DEEP_LEAF_COUNT_CONST - i,
                              ctx, sk_addr, leaf_addr);
    }

    //layer h-1
    set_tree_height(node_addr, 1);
    i = 0;
    for (; i + 7u < PORS_SHORT_LEAF_START_CONST; i += 8u) {
        pors_build_parent8(tree + offsets[1] + i * (size_t)SPX_N,
                           tree + offsets[0] + (2u * i) * (size_t)SPX_N,
                           (uint32_t)i, 1, ctx, node_addr);
    }
    if (i < PORS_SHORT_LEAF_START_CONST) {
        pors_build_parent8_tail(tree + offsets[1] + i * (size_t)SPX_N,
                                tree + offsets[0] + (2u * i) * (size_t)SPX_N,
                                (uint32_t)i, 1,
                                PORS_SHORT_LEAF_START_CONST - i,
                                ctx, node_addr);
        i = PORS_SHORT_LEAF_START_CONST;
    }
    for (; i < PORS_BASE_WIDTH_CONST;) {
        const size_t lane_count =
            PORS_BASE_WIDTH_CONST - i < 8u ? PORS_BASE_WIDTH_CONST - i : 8u;
        const uint32_t leaf_idx = PORS_SHORT_LEAF_START_CONST + (uint32_t)i;
        unsigned char *dst = tree + offsets[1] + i * (size_t)SPX_N;

        if (lane_count == 8u) {
            pors_build_leaf8(dst, secret_values, indices, &selected_pos,
                             leaf_idx, ctx, sk_addr, leaf_addr);
        } else {
            pors_build_leaf8_tail(dst, secret_values, indices, &selected_pos,
                                  leaf_idx, lane_count,
                                  ctx, sk_addr, leaf_addr);
        }
        i += lane_count;
    }

    //layer h-2 ... 0
    for (height = 2; height <= PORS_TREE_HEIGHT_CONST; height++) {
        const size_t count = pors_level_count(height);
        const unsigned char *child = tree + offsets[height - 1u];
        unsigned char *dst = tree + offsets[height];

        set_tree_height(node_addr, height);
        i = 0;
        for (; i + 7u < count; i += 8u) {
            pors_build_parent8(dst + i * (size_t)SPX_N,
                               child + (2u * i) * (size_t)SPX_N,
                               (uint32_t)i, height, ctx, node_addr);
        }
        if (i < count) {
            pors_build_parent8_tail(dst + i * (size_t)SPX_N,
                                    child + (2u * i) * (size_t)SPX_N,
                                    (uint32_t)i, height, count - i,
                                    ctx, node_addr);
        }
    }

    if (selected_pos != SPX_PORS_FP_K) {
        free(tree);
        return -1;
    }

    //return the auth node values
    for (i = 0; i < auth_len; i++) {
        uint32_t node_height;

        if (auth_nodes[i].level > PORS_TREE_HEIGHT_CONST) {
            free(tree);
            return -1;
        }

        node_height = PORS_TREE_HEIGHT_CONST - auth_nodes[i].level;
        if (auth_nodes[i].index >= pors_level_count(node_height)) {
            free(tree);
            return -1;
        }

        memcpy(auth_values + i * (size_t)SPX_N,
               tree + offsets[node_height] + auth_nodes[i].index * (size_t)SPX_N,
               SPX_N);
    }

    memcpy(root, tree + offsets[PORS_TREE_HEIGHT_CONST], SPX_N);
    free(tree);
    return 0;
}

//put the secret value in the tree
static int pors_generate_leaf_nodes(pors_node_value *deep_nodes, size_t *deep_len,
                                    pors_node_value *upper_nodes, size_t *upper_len,
                                    const uint32_t *indices,
                                    const unsigned char *secrets,
                                    const spx_ctx *ctx,
                                    const uint32_t fors_addr[8])
{
    uint32_t leaf_addr[8] = {0};
    uint32_t addrx8[8 * 8];
    unsigned int i;

    *deep_len = 0;
    *upper_len = 0;

    copy_keypair_addr(leaf_addr, fors_addr);
    set_type(leaf_addr, SPX_ADDR_TYPE_FORSTREE);
    set_tree_height(leaf_addr, 0);

    for (i = 0; i + 7u < SPX_PORS_FP_K; i += 8u) {
        uint32_t pos[8];
        unsigned char leaf[8 * SPX_N];

        for (unsigned int j = 0; j < 8u; j++) {
            pos[j] = indices[i + j];
        }

        pors_addrx8_from_indices(addrx8, leaf_addr, pos);
        thashx8(leaf + 0 * SPX_N, leaf + 1 * SPX_N,
                leaf + 2 * SPX_N, leaf + 3 * SPX_N,
                leaf + 4 * SPX_N, leaf + 5 * SPX_N,
                leaf + 6 * SPX_N, leaf + 7 * SPX_N,
                secrets + (size_t)(i + 0u) * SPX_N,
                secrets + (size_t)(i + 1u) * SPX_N,
                secrets + (size_t)(i + 2u) * SPX_N,
                secrets + (size_t)(i + 3u) * SPX_N,
                secrets + (size_t)(i + 4u) * SPX_N,
                secrets + (size_t)(i + 5u) * SPX_N,
                secrets + (size_t)(i + 6u) * SPX_N,
                secrets + (size_t)(i + 7u) * SPX_N,
                1, ctx, addrx8);

        for (unsigned int j = 0; j < 8u; j++) {
            const uint32_t idx = indices[i + j];

            if (idx < 2u * PORS_SHORT_LEAF_START_CONST) {
                deep_nodes[*deep_len].pos = idx;
                memcpy(deep_nodes[*deep_len].value, leaf + j * SPX_N, SPX_N);
                (*deep_len)++;
            } else {
                upper_nodes[*upper_len].pos = PORS_SHORT_LEAF_START_CONST +
                                              (idx - 2u * PORS_SHORT_LEAF_START_CONST);
                memcpy(upper_nodes[*upper_len].value, leaf + j * SPX_N, SPX_N);
                (*upper_len)++;
            }
        }
    }

    for (; i < SPX_PORS_FP_K; i++) {
        const uint32_t idx = indices[i];
        unsigned char leaf[SPX_N];

        set_tree_index(leaf_addr, idx);
        thash(leaf, secrets + (size_t)i * SPX_N, 1, ctx, leaf_addr);

        if (idx < 2u * PORS_SHORT_LEAF_START_CONST) {
            deep_nodes[*deep_len].pos = idx;
            memcpy(deep_nodes[*deep_len].value, leaf, SPX_N);
            (*deep_len)++;
        } else {
            upper_nodes[*upper_len].pos = PORS_SHORT_LEAF_START_CONST +
                                          (idx - 2u * PORS_SHORT_LEAF_START_CONST);
            memcpy(upper_nodes[*upper_len].value, leaf, SPX_N);
            (*upper_len)++;
        }
    }

    return 0;
}

//Algorithm 18
static int pors_compute_root_from_sig(unsigned char *root,
                                      const uint32_t *indices,
                                      const unsigned char *secrets,
                                      const unsigned char *auth_values,
                                      size_t auth_len,
                                      const spx_ctx *ctx,
                                      const uint32_t fors_addr[8])
{
    pors_node_value current[SPX_PORS_FP_K];
    pors_node_value upper[SPX_PORS_FP_K];
    pors_node_value parents[SPX_PORS_FP_K];
    pors_node_value merged[SPX_PORS_FP_K];
    size_t current_len;
    size_t upper_len;
    size_t auth_cursor = 0;
    uint32_t level;

    if (pors_generate_leaf_nodes(current, &current_len, upper, &upper_len,
                                 indices, secrets, ctx, fors_addr) != 0) {
        return -1;
    }

    for (level = PORS_TREE_HEIGHT_CONST; level > 0; level--) {
        size_t j = 0;
        size_t parent_len = 0;
        unsigned char parent_inputs[8 * 2 * SPX_N];
        uint32_t parent_pos[8];
        uint32_t node_addr[8] = {0};
        unsigned int batch_len = 0;

        copy_keypair_addr(node_addr, fors_addr);
        set_type(node_addr, SPX_ADDR_TYPE_FORSTREE);
        set_tree_height(node_addr, PORS_TREE_HEIGHT_CONST - level + 1u);

        while (j < current_len) {
            const uint32_t node = current[j].pos;
            unsigned char *buffer = parent_inputs + batch_len * 2u * SPX_N;

            parents[parent_len].pos = node >> 1;
            parent_pos[batch_len] = parents[parent_len].pos;

            if (j + 1 < current_len && current[j + 1].pos == (node ^ 1u)) {//sib nodes both in auth
                memcpy(buffer, current[j].value, SPX_N);
                memcpy(buffer + SPX_N, current[j + 1].value, SPX_N);
                j += 2;
            } else {
                if (auth_cursor >= auth_len) {
                    return -1;
                }

                if ((node & 1u) == 0) {//auth node at right
                    memcpy(buffer, current[j].value, SPX_N);
                    memcpy(buffer + SPX_N, auth_values + auth_cursor * SPX_N, SPX_N);
                } else {//auth node at left
                    memcpy(buffer, auth_values + auth_cursor * SPX_N, SPX_N);
                    memcpy(buffer + SPX_N, current[j].value, SPX_N);
                }
                auth_cursor++;
                j++;
            }

            parent_len++;
            batch_len++;

            if (batch_len == 8u) {
                pors_hash_parent_batch(parents + parent_len - batch_len,
                                       parent_inputs, parent_pos, batch_len,
                                       ctx, node_addr);
                batch_len = 0;
            }
        }

        if (batch_len != 0u) {
            pors_hash_parent_batch(parents + parent_len - batch_len,
                                   parent_inputs, parent_pos, batch_len,
                                   ctx, node_addr);
        }

        {
            size_t left = 0;//traversal parents
            size_t right = 0;//traversal upper
            size_t out_len = 0;

            //sort by pos(idx in layer)
            while (left < parent_len || right < upper_len) {
                if (right >= upper_len ||
                    (left < parent_len && parents[left].pos < upper[right].pos)) {
                    merged[out_len++] = parents[left++];
                } else {
                    merged[out_len++] = upper[right++];
                }
            }

            memcpy(current, merged, out_len * sizeof(pors_node_value));
            current_len = out_len;
            upper_len = 0;
        }
    }

    if (current_len != 1 || auth_cursor != auth_len) {
        return -1;
    }

    memcpy(root, current[0].value, SPX_N);
    return 0;
}

//Algorithm 16
void pors_fp_sign(unsigned char *sig, unsigned char *pk,
                  const unsigned char *m, const spx_ctx *ctx,
                  const uint32_t fors_addr[8],
                  const unsigned char *R)
{
    pors_node_pos auth_nodes[PORS_AUTH_NODE_CAPACITY];
    uint32_t indices[SPX_PORS_FP_K];
    size_t auth_len = 0;
    uint32_t ctr = 0;
    unsigned char *secret_values;
    unsigned char *auth_values;

    while (1) {
        if (ctr == UINT32_MAX) {
            memset(sig, 0, SPX_PORS_FP_BYTES);
            memset(pk, 0, SPX_N);
            return;
        }

        if (pors_message_to_indices(indices, R, m, ctr) != 0) {
            memset(sig, 0, SPX_PORS_FP_BYTES);
            memset(pk, 0, SPX_N);
            return;
        }

        pors_octopus_positions(auth_nodes, &auth_len, indices);
        if (auth_len <= SPX_PORS_FP_MAX_AUTH_NODES) {
            break;
        }

        ctr++;
    }
    ull_to_bytes(sig, COUNTER_SIZE, ctr);
    secret_values = sig + COUNTER_SIZE;
    auth_values = secret_values + (size_t)SPX_PORS_FP_K * SPX_N;

    if (pors_build_tree(pk, auth_values, secret_values, indices,
                             auth_nodes, auth_len, ctx, fors_addr) != 0) {
        memset(sig, 0, SPX_PORS_FP_BYTES);
        memset(pk, 0, SPX_N);
        return;
    }

    if (auth_len < SPX_PORS_FP_MAX_AUTH_NODES) {
        memset(auth_values + auth_len * (size_t)SPX_N, 0,
               (size_t)(SPX_PORS_FP_MAX_AUTH_NODES - auth_len) * SPX_N);
    }
}

//Algorithm 17
void pors_fp_pk_from_sig(unsigned char *pk, const unsigned char *sig,
                         const unsigned char *m, const spx_ctx *ctx,
                         const uint32_t fors_addr[8],
                         const unsigned char *R)
{
    pors_node_pos auth_nodes[PORS_AUTH_NODE_CAPACITY];
    uint32_t indices[SPX_PORS_FP_K];
    uint32_t ctr;
    size_t auth_len = 0;
    const unsigned char *secrets;
    const unsigned char *auth_values;

    ctr = (uint32_t)bytes_to_ull(sig, COUNTER_SIZE);
    if (ctr == UINT32_MAX ||
        pors_message_to_indices(indices, R, m, ctr) != 0) {
        memset(pk, 0, SPX_N);
        return;
    }

    pors_octopus_positions(auth_nodes, &auth_len, indices);
    if (auth_len > SPX_PORS_FP_MAX_AUTH_NODES) {
        memset(pk, 0, SPX_N);
        return;
    }

    secrets = sig + COUNTER_SIZE;
    auth_values = secrets + (size_t)SPX_PORS_FP_K * SPX_N;

    if (pors_compute_root_from_sig(pk, indices, secrets, auth_values, auth_len,
                                   ctx, fors_addr) != 0) {
        memset(pk, 0, SPX_N);
    }
}
