#include "reed_solomon.h"

#include <stdint.h>
#include <string.h>

#include "crypto_memset.h"
#include "gf.h"
#include "parameters.h"
#include "rs_generator_constants_generated.h"

/*
 * x86_64 optimized erasure-aware Reed-Solomon implementation for HARE.
 *
 * This file is used only by Optimized_Implementation targets. It keeps the
 * same API and the same HARE RS-with-erasures semantics as the structurally
 * hardened reference decoder: syndromes, erasure locator, modified syndromes,
 * BM with n-k-h active iterations, total locator, evaluator, and Forney-style
 * correction.
 *
 * Optimizations relative to ref/reed_solomon.c:
 *   - optimized targets link x86_64/avx256/gf.c, replacing GF(2^8)
 *     multiplication by a PCLMULQDQ-based implementation;
 *   - RS generator polynomials are compile-time constants per HARE instance,
 *     avoiding repeated generator construction during systematic encoding.
 *
 * Side-channel boundary: this file preserves the reference decoder's fixed
 * public loop bounds and mask-selection structure. It does not claim a formal
 * whole-KEM constant-time proof; compiler transformations and cache behavior
 * still require separate audit.
 */

/**
 * Return alpha^exp in GF(2^8), normalizing negative exponents modulo the field order.
 */
static uint16_t gf_pow_alpha(int exp) {
    while (exp < 0) {
        exp += PARAM_GF_MUL_ORDER;
    }
    return gf_exp[exp % PARAM_GF_MUL_ORDER];
}

/**
 * Expand a one-bit predicate into a 16-bit all-zero/all-one mask.
 */
static uint16_t ct_mask_u16(uint16_t bit) {
    return (uint16_t)(0u - (uint16_t)(bit & 1u));
}

/**
 * Select a or b under a constant-time mask.
 */
static uint16_t ct_select_u16(uint16_t mask, uint16_t a, uint16_t b) {
    return (uint16_t)((a & mask) | (b & (uint16_t)~mask));
}

/**
 * Return 1 if x is nonzero and 0 otherwise without branches on x.
 */
static uint16_t ct_nonzero_u16(uint16_t x) {
    uint32_t v = (uint32_t)x;
    return (uint16_t)(((v | (0u - v)) >> 31) & 1u);
}

/**
 * Return 1 if a equals b and 0 otherwise using constant-time predicates.
 */
static uint16_t ct_eq_u16(uint16_t a, uint16_t b) {
    return (uint16_t)(ct_nonzero_u16((uint16_t)(a ^ b)) ^ 1u);
}

/**
 * Return 1 if a is less than b for small unsigned values.
 */
static uint16_t ct_lt_u16(uint16_t a, uint16_t b) {
    return (uint16_t)((((uint32_t)a - (uint32_t)b) >> 31) & 1u);
}

/**
 * Return 1 if a is less than or equal to b for small unsigned values.
 */
static uint16_t ct_le_u16(uint16_t a, uint16_t b) {
    return (uint16_t)(ct_lt_u16(b, a) ^ 1u);
}

/**
 * Return 1 if a is greater than b for small unsigned values.
 */
static uint16_t ct_gt_u16(uint16_t a, uint16_t b) {
    return ct_lt_u16(b, a);
}

/**
 * Select an array element by masked scan instead of secret-dependent indexing.
 */
static uint16_t ct_select_from_array(const uint16_t *a, uint16_t len, uint16_t idx) {
    uint16_t out = 0;

    for (uint16_t i = 0; i < len; ++i) {
        const uint16_t mask = ct_mask_u16(ct_eq_u16(i, idx));
        out ^= (uint16_t)(a[i] & mask);
    }

    return out;
}



/**
 * Compute the shortened Reed-Solomon generator polynomial for this parameter set.
 */
void compute_generator_poly(uint16_t *poly) {
    memcpy(poly, rs_generator_poly_const, (size_t)PARAM_G * sizeof(uint16_t));
}

/**
 * Evaluate the received Reed-Solomon word at consecutive powers of alpha.
 */
static void compute_syndromes(uint16_t *syndromes, const uint8_t *cdw) {
    for (uint16_t i = 0; i < PARAM_RS_PARITY; ++i) {
        const uint16_t ai = gf_pow_alpha((int)(i + 1u));
        uint16_t xpow = 1;
        uint16_t accum = 0;

        for (uint16_t j = 0; j < PARAM_N1; ++j) {
            accum ^= gf_mul(cdw[j], xpow);
            xpow = gf_mul(xpow, ai);
        }
        syndromes[i] = accum;
    }
}

/**
 * Build the erasure locator polynomial gamma(x) from RM erasure flags.
 */
static void compute_erasure_locator(uint16_t *gamma,
                                    uint16_t *erasure_count,
                                    const uint8_t *erasures) {
    memset(gamma, 0, (size_t)(PARAM_RS_PARITY + 1u) * sizeof(uint16_t));
    gamma[0] = 1;
    *erasure_count = 0;

    for (uint16_t pos = 0; pos < PARAM_N1; ++pos) {
        const uint16_t eta = gf_pow_alpha((int)pos);
        const uint16_t is_erasure = (erasures == NULL) ? 0u : ct_nonzero_u16((uint16_t)erasures[pos]);
        const uint16_t mask = ct_mask_u16(is_erasure);

        for (uint16_t j = PARAM_RS_PARITY; j > 0; --j) {
            const uint16_t updated = (uint16_t)(gamma[j] ^ gf_mul(eta, gamma[j - 1u]));
            gamma[j] = ct_select_u16(mask, updated, gamma[j]);
        }
        *erasure_count = (uint16_t)(*erasure_count + is_erasure);
    }
}

/**
 * Multiply S(x) by gamma(x) to form erasure-aware modified syndromes.
 */
static void compute_modified_syndromes(uint16_t *modified,
                                       const uint16_t *syndromes,
                                       const uint16_t *gamma) {
    for (uint16_t i = 0; i < PARAM_RS_PARITY; ++i) {
        uint16_t acc = 0;

        for (uint16_t j = 0; j <= i; ++j) {
            acc ^= gf_mul(syndromes[i - j], gamma[j]);
        }
        modified[i] = acc;
    }
}

/**
 * Run the masked Berlekamp-Massey recurrence to recover the error locator.
 */
static uint16_t compute_error_locator(uint16_t *sigma,
                                      const uint16_t *modified,
                                      uint16_t erasure_count_sat) {
    uint16_t sigma_copy[PARAM_RS_PARITY + 1u];
    uint16_t x_sigma_p[PARAM_RS_PARITY + 1u];
    uint16_t bm_sequence[PARAM_RS_PARITY];
    uint16_t degree = 0;
    uint16_t prev_degree = 0;
    uint16_t pp = (uint16_t)-1;
    uint16_t prev_discrepancy = 1;
    const uint16_t active_iterations = (uint16_t)(PARAM_RS_PARITY - erasure_count_sat);

    memset(sigma, 0, (size_t)(PARAM_RS_PARITY + 1u) * sizeof(uint16_t));
    memset(sigma_copy, 0, sizeof(sigma_copy));
    memset(x_sigma_p, 0, sizeof(x_sigma_p));
    memset(bm_sequence, 0, sizeof(bm_sequence));

    sigma[0] = 1;
    x_sigma_p[1] = 1;

    /*
     * HARE specifies BM on T_h, ..., T_{n-k-1}, i.e. n-k-h iterations.
     * Build that shifted sequence with masked loads so the secret erasure count
     * is not used as a direct memory index in the BM loop.
     */
    for (uint16_t mu = 0; mu < PARAM_RS_PARITY; ++mu) {
        bm_sequence[mu] = ct_select_from_array(modified,
                                               PARAM_RS_PARITY,
                                               (uint16_t)(erasure_count_sat + mu));
    }

    for (uint16_t mu = 0; mu < PARAM_RS_PARITY; ++mu) {
        uint16_t discrepancy = bm_sequence[mu];
        uint16_t saved_degree;
        uint16_t deg_x;
        uint16_t candidate_degree;
        uint16_t nonzero_discrepancy;
        uint16_t active;
        uint16_t update;
        uint16_t update_mask;
        uint16_t discrepancy_mask;
        uint16_t shifted_x_sigma_p[PARAM_RS_PARITY + 1u];
        uint16_t shifted_sigma_copy[PARAM_RS_PARITY + 1u];

        memcpy(sigma_copy, sigma, sizeof(sigma_copy));
        saved_degree = degree;

        for (uint16_t i = 1; i <= mu; ++i) {
            discrepancy ^= gf_mul(sigma[i], bm_sequence[mu - i]);
        }

        active = ct_lt_u16(mu, active_iterations);
        discrepancy &= ct_mask_u16(active);
        nonzero_discrepancy = ct_nonzero_u16(discrepancy);
        discrepancy_mask = ct_mask_u16((uint16_t)(active & nonzero_discrepancy));

        {
            const uint16_t factor = gf_mul(discrepancy, gf_inverse(prev_discrepancy));
            for (uint16_t i = 1; i <= PARAM_RS_PARITY; ++i) {
                const uint16_t delta = gf_mul(factor, x_sigma_p[i]);
                sigma[i] ^= (uint16_t)(delta & discrepancy_mask);
            }
        }

        deg_x = (uint16_t)(mu - pp);
        candidate_degree = (uint16_t)(deg_x + prev_degree);
        update = (uint16_t)(active & nonzero_discrepancy & ct_gt_u16(candidate_degree, degree));
        update_mask = ct_mask_u16(update);

        shifted_x_sigma_p[0] = 0;
        shifted_sigma_copy[0] = 0;
        for (uint16_t i = PARAM_RS_PARITY; i > 0; --i) {
            shifted_x_sigma_p[i] = x_sigma_p[i - 1u];
            shifted_sigma_copy[i] = sigma_copy[i - 1u];
        }

        for (uint16_t i = 0; i <= PARAM_RS_PARITY; ++i) {
            x_sigma_p[i] = ct_select_u16(update_mask, shifted_sigma_copy[i], shifted_x_sigma_p[i]);
        }

        degree = ct_select_u16(update_mask, candidate_degree, degree);
        pp = ct_select_u16(update_mask, mu, pp);
        prev_discrepancy = ct_select_u16(update_mask, discrepancy, prev_discrepancy);
        prev_degree = ct_select_u16(update_mask, saved_degree, prev_degree);
    }

    return degree;
}

/**
 * Multiply error and erasure locators to form the total locator Lambda(x).
 */
static void multiply_locator(uint16_t *lambda,
                             const uint16_t *sigma,
                             const uint16_t *gamma) {
    memset(lambda, 0, (size_t)(PARAM_RS_PARITY + 1u) * sizeof(uint16_t));

    for (uint16_t i = 0; i <= PARAM_RS_PARITY; ++i) {
        for (uint16_t j = 0; (j <= PARAM_RS_PARITY) && ((uint16_t)(i + j) <= PARAM_RS_PARITY); ++j) {
            lambda[i + j] ^= gf_mul(sigma[i], gamma[j]);
        }
    }
}

/**
 * Compute the Forney evaluator polynomial S(x)Lambda(x) mod x^(n-k).
 */
static void compute_error_evaluator(uint16_t *evaluator,
                                    const uint16_t *syndromes,
                                    const uint16_t *lambda) {
    for (uint16_t i = 0; i < PARAM_RS_PARITY; ++i) {
        uint16_t acc = 0;

        for (uint16_t j = 0; j <= i; ++j) {
            acc ^= gf_mul(syndromes[j], lambda[i - j]);
        }
        evaluator[i] = acc;
    }
}

/**
 * Evaluate a polynomial at x over GF(2^8).
 */
static uint16_t eval_poly(const uint16_t *poly, uint16_t degree, uint16_t x) {
    uint16_t acc = 0;
    uint16_t xpow = 1;

    for (uint16_t i = 0; i <= degree; ++i) {
        acc ^= gf_mul(poly[i], xpow);
        xpow = gf_mul(xpow, x);
    }

    return acc;
}

/**
 * Evaluate the formal derivative of the total locator polynomial.
 */
static uint16_t eval_locator_derivative(const uint16_t *lambda, uint16_t x) {
    uint16_t acc = 0;
    uint16_t xpow = 1;

    for (uint16_t i = 1; i <= PARAM_RS_PARITY; ++i) {
        const uint16_t odd_mask = ct_mask_u16((uint16_t)(i & 1u));
        acc ^= (uint16_t)(gf_mul(lambda[i], xpow) & odd_mask);
        xpow = gf_mul(xpow, x);
    }

    return acc;
}

/**
 * Apply Forney correction at all public RS positions using masked updates.
 */
static void correct_by_forney(uint8_t *cdw,
                              const uint16_t *lambda,
                              const uint16_t *evaluator,
                              uint16_t valid_decode) {
    const uint16_t valid_mask = ct_mask_u16(valid_decode);

    for (uint16_t pos = 0; pos < PARAM_N1; ++pos) {
        const uint16_t location = gf_pow_alpha((int)pos);
        const uint16_t inv_location = gf_inverse(location);
        const uint16_t locator_value = eval_poly(lambda, PARAM_RS_PARITY, inv_location);
        const uint16_t numerator = eval_poly(evaluator, (uint16_t)(PARAM_RS_PARITY - 1u), inv_location);
        const uint16_t denominator = eval_locator_derivative(lambda, inv_location);
        const uint16_t is_root = ct_eq_u16(locator_value, 0);
        const uint16_t has_denominator = ct_nonzero_u16(denominator);
        const uint16_t apply = (uint16_t)(is_root & has_denominator);
        const uint16_t apply_mask = (uint16_t)(valid_mask & ct_mask_u16(apply));
        const uint16_t value = gf_mul(numerator, gf_inverse(denominator));

        cdw[pos] ^= (uint8_t)(value & apply_mask);
    }
}

/**
 * Encode a PARAM_K-byte message into a shortened Reed-Solomon codeword.
 */
void reed_solomon_encode(uint64_t *cdw, const uint8_t *msg) {
    uint16_t rs_poly[PARAM_G];
    uint16_t tmp[PARAM_G];
    uint8_t msg_bytes[PARAM_K];
    uint8_t cdw_bytes[PARAM_N1];

    memset(msg_bytes, 0, sizeof(msg_bytes));
    memset(cdw_bytes, 0, sizeof(cdw_bytes));
    memset(tmp, 0, sizeof(tmp));
    compute_generator_poly(rs_poly);

    memcpy(msg_bytes, msg, PARAM_K);

    for (uint32_t i = 0; i < PARAM_K; ++i) {
        const uint8_t gate = (uint8_t)(msg_bytes[PARAM_K - 1u - i] ^ cdw_bytes[PARAM_RS_PARITY - 1u]);

        for (uint32_t j = 0; j < PARAM_G; ++j) {
            tmp[j] = gf_mul(gate, rs_poly[j]);
        }
        for (uint32_t k = PARAM_RS_PARITY - 1u; k > 0; --k) {
            cdw_bytes[k] = (uint8_t)(cdw_bytes[k - 1u] ^ tmp[k]);
        }
        cdw_bytes[0] = (uint8_t)tmp[0];
    }

    memcpy(cdw_bytes + PARAM_RS_PARITY, msg_bytes, PARAM_K);
    memcpy(cdw, cdw_bytes, PARAM_N1);
}

/**
 * Decode a shortened Reed-Solomon codeword with RM-provided erasure positions.
 */
void reed_solomon_decode(uint8_t *msg, uint64_t *cdw, const uint8_t *erasures) {
    uint8_t cdw_bytes[PARAM_N1];
    uint16_t syndromes[PARAM_RS_PARITY];
    uint16_t modified_syndromes[PARAM_RS_PARITY];
    uint16_t gamma[PARAM_RS_PARITY + 1u];
    uint16_t sigma[PARAM_RS_PARITY + 1u];
    uint16_t lambda[PARAM_RS_PARITY + 1u];
    uint16_t evaluator[PARAM_RS_PARITY];
    uint16_t erasure_count = 0;
    uint16_t erasure_count_sat;
    uint16_t valid_decode;

    memset(cdw_bytes, 0, sizeof(cdw_bytes));
    memset(syndromes, 0, sizeof(syndromes));
    memset(modified_syndromes, 0, sizeof(modified_syndromes));
    memset(gamma, 0, sizeof(gamma));
    memset(sigma, 0, sizeof(sigma));
    memset(lambda, 0, sizeof(lambda));
    memset(evaluator, 0, sizeof(evaluator));

    memcpy(cdw_bytes, cdw, PARAM_N1);
    compute_syndromes(syndromes, cdw_bytes);
    compute_erasure_locator(gamma, &erasure_count, erasures);

    erasure_count_sat = ct_select_u16(ct_mask_u16(ct_le_u16(erasure_count, PARAM_RS_PARITY)),
                                      erasure_count,
                                      PARAM_RS_PARITY);
    valid_decode = ct_le_u16(erasure_count, PARAM_RS_PARITY);

    compute_modified_syndromes(modified_syndromes, syndromes, gamma);
    (void)compute_error_locator(sigma, modified_syndromes, erasure_count_sat);
    multiply_locator(lambda, sigma, gamma);
    compute_error_evaluator(evaluator, syndromes, lambda);
    correct_by_forney(cdw_bytes, lambda, evaluator, valid_decode);

    memcpy(msg, cdw_bytes + PARAM_RS_PARITY, PARAM_K);

    memset_zero(cdw_bytes, sizeof(cdw_bytes));
    memset_zero(syndromes, sizeof(syndromes));
    memset_zero(modified_syndromes, sizeof(modified_syndromes));
    memset_zero(gamma, sizeof(gamma));
    memset_zero(sigma, sizeof(sigma));
    memset_zero(lambda, sizeof(lambda));
    memset_zero(evaluator, sizeof(evaluator));
}
