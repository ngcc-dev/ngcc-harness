#include <stdint.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include "poly.h"
#include "ntt.h"
#include "randombytes.h"
#include "reduce.h"
#include "sampler.h"
#include "symmetric.h"
#include "toomcook.h"
#include "bch_codec.h"
#ifdef LORE_USE_NEON_CORE
#include "simd/neon_core.h"
#endif

/*************************************************
* Name:        center_mod_257
*
* Description: 32-bit branchless centered reduction optimized for Q=257.
* Reduces a 32-bit integer to the range [-128, 128].
*
* Arguments:   - int32_t x: the input 32-bit integer to be reduced
*
* Returns:     the centered reduced 16-bit integer
**************************************************/
static inline int16_t center_mod_257(int32_t x) {
    // Fast reduction using Fermat prime property.
    int32_t t = (x & 255) - (x >> 8); 
    t = (t & 255) - (t >> 8);         

    // Map to [-128, 128]
    t += (t >> 31) & 257;             

    t -= (((128 - t) >> 31) & 257);   
    
    return (int16_t)t;
}


#if LORE_LEVEL > 1
static struct bch_control *lore_bch_ctx = NULL;
/*************************************************
* Name:        ensure_bch_init
*
* Description: Initializes the BCH codec context dynamically if it has 
* not been initialized yet.
*
* Arguments:   None
**************************************************/
static void ensure_bch_init() {
    if (lore_bch_ctx == NULL) {
        lore_bch_ctx = init_bch(LORE_BCH_M, LORE_BCH_T, 0);
    }
}
#endif
/*************************************************
* Name:        poly_from_sparse
*
* Description: Converts a sparse polynomial to a dense polynomial.
*
* Arguments:   - poly *r:          pointer to the output dense polynomial
* - const poly_sparse *s: pointer to the input sparse polynomial
**************************************************/
void poly_from_sparse(poly *r, const poly_sparse *s) {
    memset(r->coeffs, 0, LORE_N * sizeof(int16_t));
    for (int i = 0; i < s->n_coeffs; ++i) {
        r->coeffs[s->pos[i]] = s->val[i];
    }
}


/*************************************************
* Name:        poly_add_modt
*
* Description: Adds two polynomials in the ring Z_t[X]/(X^N+1).
*
* Arguments:   - poly *r:       pointer to the output polynomial r = a + b
* - const poly *a: pointer to the first polynomial
* - const poly *b: pointer to the second polynomial
**************************************************/
void poly_add_modt(poly *r, const poly *a, const poly *b) {
    // SWAR processing: 4-way parallel arithmetic.
    uint64_t *r64 = (uint64_t *)r->coeffs;
    const uint64_t *a64 = (const uint64_t *)a->coeffs;
    const uint64_t *b64 = (const uint64_t *)b->coeffs;

   // Parallel mask setup.
    uint64_t mask = (LORE_T == 4) ? 0x0003000300030003ULL : 0x0001000100010001ULL;

    for(int j = 0; j < LORE_N / 4; ++j) {
        // Parallel addition without overflow risk.
        uint64_t sum = a64[j] + b64[j];
        
        uint64_t val = sum & mask; 

        if (LORE_T == 4) {
            // Branchless centering for T=4: map 3 to -1.
            
            uint64_t bit0 = val & 0x0001000100010001ULL;
            uint64_t bit1 = (val >> 1) & 0x0001000100010001ULL;
            
            uint64_t cond = bit0 & bit1; 
            
            r64[j] = val - (cond << 2);
        } else {
            // For T=2, result requires no shift.
            r64[j] = val;
        }
    }
}


/*************************************************
* Name:        poly_sparse_mul_modt
*
* Description: Multiplies a dense polynomial A with a sparse polynomial B,
* with the result being mod (t, x^n+1).
*
* Arguments:   - poly *r:             pointer to the output polynomial r = a_dense * b_sparse
* - const poly *a_dense:   pointer to the dense polynomial A
* - const poly_sparse *b_sparse: pointer to the sparse polynomial B
**************************************************/
void poly_sparse_mul_modt(poly *r, const poly *a_dense, const poly_sparse *b_sparse) {
    int32_t c_tmp[2 * LORE_N] = {0}; 
    
    for (int i = 0; i < b_sparse->n_coeffs; i++) {
        uint16_t degree = b_sparse->pos[i]; 
        int16_t  value  = b_sparse->val[i]; 

        if (degree >= LORE_N) continue; 
        
        for (int j = 0; j < LORE_N; j++) {
            c_tmp[degree + j] += (int32_t)a_dense->coeffs[j] * value;
        }
    }
    
    for (int j = 0; j < LORE_N; j++) {
        int32_t final_val = c_tmp[j] - c_tmp[LORE_N + j];

        int16_t val = (int16_t)(final_val & (LORE_T - 1));
        
        if (LORE_T == 4) {
            val -= ((val >> 1) & (val & 1)) << 2;
        }
        
        r->coeffs[j] = val;
    }
}





/*************************************************
* Name:        inv
*
* Description: Computes the modular inverse using the Extended Euclidean Algorithm.
*
* Arguments:   - int16_t a: the integer
* - int16_t n: the modulus
*
* Returns:     the modular inverse of a modulo n, or -1 if not invertible
**************************************************/
static int16_t inv(int16_t a, int16_t n) {
    int16_t t = 0, newt = 1;
    int16_t r = n, newr = a;
    while (newr != 0) {
        int16_t quotient = r / newr;
        int16_t temp_t = t;
        t = newt;
        newt = (int16_t)(temp_t - quotient * newt);
        int16_t temp_r = r;
        r = newr;
        newr = (int16_t)(temp_r - quotient * newr);
    }
    if (r > 1) return -1; // not invertible
    if (t < 0) t = t + n;
    return t;
}

/*************************************************
* Name:        poly_crt_combine
*
* Description: Combine a polynomial in CRT form back to Z_tq.
* This version correctly handles centered coefficients.
*
* Arguments:   - poly *r:          pointer to the output polynomial
* - const poly_crt *pc: pointer to the input CRT polynomial
**************************************************/
void poly_crt_combine(poly *r, const poly_crt *pc) {
    const int16_t q_inv_t = inv(LORE_Q, LORE_T);

    for (int i = 0; i < LORE_N; ++i) {

        int16_t a_q = pc->q_poly.coeffs[i];
        int16_t a_t = pc->t_poly.coeffs[i];

        int32_t a_q_std = a_q;
        a_q_std += (a_q_std >> 31) & LORE_Q;
        
        int32_t a_t_std = a_t;
        a_t_std += (a_t_std >> 31) & LORE_T;
        
        /* Apply Garner's CRT algorithm to reconstruct the coefficient */

        int32_t h = (a_t_std - a_q_std) & (LORE_T - 1);
        h = (h * q_inv_t) & (LORE_T - 1);
        
        /*  Reconstruct the final result*/
        int32_t result = a_q_std + (int32_t)LORE_Q * h;
        r->coeffs[i] = (int16_t)result;
    }
}
/*************************************************
* Name:        poly_crt_decompose
*
* Description: Decompose a polynomial from Z_tq to centered CRT form.
*
* Arguments:   - poly_crt *pc:   pointer to the output CRT polynomial
* - const poly *p: pointer to the input polynomial
**************************************************/
void poly_crt_decompose(poly_crt *pc, const poly *p) {
    for (int i = 0; i < LORE_N; ++i) {
        pc->q_poly.coeffs[i] = barrett_reduce(p->coeffs[i]);

        int16_t t_coeff = (int16_t)(p->coeffs[i] & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        pc->t_poly.coeffs[i] = t_coeff;
    }
}

/*************************************************
* Name:        poly_crt_add
*
* Description: Adds two CRT polynomials.
*
* Arguments:   - poly_crt *r:       pointer to the output CRT polynomial
* - const poly_crt *a: pointer to the first input CRT polynomial
* - const poly_crt *b: pointer to the second input CRT polynomial
**************************************************/
void poly_crt_add(poly_crt *r, const poly_crt *a, const poly_crt *b) {
    for (int i = 0; i < LORE_N; ++i) {
        r->q_poly.coeffs[i] = center_mod_257(a->q_poly.coeffs[i] + b->q_poly.coeffs[i]);
        
        int16_t t_coeff = (int16_t)((a->t_poly.coeffs[i] + b->t_poly.coeffs[i]) & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        r->t_poly.coeffs[i] = t_coeff;
    }
}

/*************************************************
* Name:        poly_crt_sub
*
* Description: Subtracts two CRT polynomials.
*
* Arguments:   - poly_crt *r:       pointer to the output CRT polynomial
* - const poly_crt *a: pointer to the first input CRT polynomial
* - const poly_crt *b: pointer to the second input CRT polynomial
**************************************************/
void poly_crt_sub(poly_crt *r, const poly_crt *a, const poly_crt *b) {
    for (int i = 0; i < LORE_N; ++i) {
        r->q_poly.coeffs[i] = barrett_reduce(a->q_poly.coeffs[i] - b->q_poly.coeffs[i]);

        int16_t t_coeff = (int16_t)((a->t_poly.coeffs[i] - b->t_poly.coeffs[i]) & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        r->t_poly.coeffs[i] = t_coeff;
    }
}

/*************************************************
* Name:        poly_add
*
* Description: Adds two polynomials.
*
* Arguments:   - poly *r:       pointer to the output polynomial
* - const poly *a: pointer to the first input polynomial
* - const poly *b: pointer to the second input polynomial
**************************************************/
 void poly_add(poly *r, const poly *a, const poly *b) {
#ifdef LORE_USE_NEON_CORE
     int i = 0;

     for(; i <= LORE_N - 8; i += 8) {
         const int16x8_t av = vld1q_s16(&a->coeffs[i]);
         const int16x8_t bv = vld1q_s16(&b->coeffs[i]);
         const int16x8_t sum = vaddq_s16(av, bv);
         vst1q_s16(&r->coeffs[i], lore_barrett_reduce_s16x8(sum));
     }
     for(; i < LORE_N; ++i) {
         r->coeffs[i] = barrett_reduce(a->coeffs[i] + b->coeffs[i]);
     }
#else
     for(int i=0; i<LORE_N; ++i) {
         r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
     }
     for(int i=0; i<LORE_N; ++i) {
         r->coeffs[i] = barrett_reduce(r->coeffs[i]);
     }
#endif
 }


/*************************************************
* Name:        poly_frombytes
*
* Description: Deserializes a polynomial from a byte array.
*
* Arguments:   - poly *r:            pointer to the output polynomial
* - const unsigned char *a: pointer to the input byte array
**************************************************/
void poly_frombytes(poly *r, const unsigned char *a) {
    for (int i = 0; i < LORE_N / 2; i++) {
        r->coeffs[2 * i] = ((a[3 * i + 0] >> 0) | ((int16_t)a[3 * i + 1] << 8)) & 0xFFF;
        r->coeffs[2 * i + 1] = ((a[3 * i + 1] >> 4) | ((int16_t)a[3 * i + 2] << 4)) & 0xFFF;
    }
}

/*************************************************
* Name:        poly_tobytes
*
* Description: Serializes a polynomial into a byte array.
*
* Arguments:   - unsigned char *r: pointer to the output byte array
* - const poly *p:    pointer to the input polynomial
**************************************************/
void poly_tobytes(unsigned char *r, const poly *p) {
    for (int i = 0; i < LORE_N / 2; i++) {
        r[3 * i + 0] = (unsigned char)(p->coeffs[2 * i + 0] >> 0);
        r[3 * i + 1] = (unsigned char)(((p->coeffs[2 * i + 0] >> 8) & 0xFF) | (p->coeffs[2 * i + 1] << 4));
        r[3 * i + 2] = (unsigned char)(p->coeffs[2 * i + 1] >> 4);
    }
}

/*************************************************
* Name:        poly_getnoise
*
* Description: Samples a noise polynomial with a fixed weight.
*
* Arguments:   - poly_crt_vec *r_crt_vec:   pointer to the output CRT polynomial vector
* - poly_sparse *r_sparse_vec: pointer to the output sparse polynomial vector
* - unsigned char *seed:       pointer to the seed
* - unsigned char nonce:       a domain-separation nonce
**************************************************/
void poly_getnoise(poly_crt_vec *r_crt_vec, poly_sparse *r_sparse_vec, unsigned char *seed, unsigned char nonce)
{
  sample_fixed_weight(r_crt_vec, r_sparse_vec, seed, nonce);
}

/*************************************************
* Name:        poly_getnoise_uniform
*
* Description: Sample a polynomial with coefficients uniformly random from
* U[-(t-1)/2, (t-1)/2].
*
* Arguments:   - poly *r:                pointer to output polynomial
* - uint16_t t:             the parameter t
* - const unsigned char *seed: pointer to input seed
* - unsigned char nonce:      a domain-separation nonce
**************************************************/
void poly_getnoise_uniform(poly *r, uint16_t t, const unsigned char *seed, unsigned char nonce)
{
    uint8_t buf[LORE_N * 2];
    prf(buf, sizeof(buf), seed, nonce);
    memset(r->coeffs, 0, LORE_N * sizeof(int16_t)); /* ensure fully initialized on early exit */

    int ctr = 0; 
    unsigned int buf_pos = 0; 

    // Direct sampling in [-(t-1)/2, (t-1)/2].
    uint16_t range = t;
    uint16_t limit = (uint16_t)((0x100 / range) * range);

    while(ctr < LORE_N) {
        if (buf_pos >= sizeof(buf)) break; 
        uint8_t val = buf[buf_pos++];
        
        if (val < limit) {
            // Ensure zero mean.
            int16_t t_coeff = (int16_t)(val % range);
            if (t == 4) {
                t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
            }
            r->coeffs[ctr++] = t_coeff;
        }
    }
}
/*************************************************
* Name:        poly_ntt
*
* Description: Applies Number Theoretic Transform (NTT) to a polynomial.
*
* Arguments:   - poly *r: pointer to the polynomial
**************************************************/
void poly_ntt(poly *r) {
    ntt(r->coeffs);
    poly_reduce(r);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Applies inverse NTT to a polynomial.
*
* Arguments:   - poly *r: pointer to the polynomial
**************************************************/
void poly_invntt_tomont(poly *r) {
    invntt_tomont(r->coeffs);
}

/*************************************************
* Name:        poly_pointwise_montgomery
*
* Description: Pointwise multiplication of two polynomials in the NTT domain.
**************************************************/
void poly_pointwise_montgomery(poly *r, const poly *a, const poly *b) {
    poly_mul_ntt(r->coeffs, a->coeffs, b->coeffs);
}

/*************************************************
* Name:        pack_t_bits
*
* Description: Packs the t-part indices into a byte array.
*
* Arguments:   - unsigned char *r:      pointer to the output byte array
* - const uint8_t *t_indices: pointer to the input t_indices
**************************************************/
#if LORE_R_BITS > 0
void pack_t_bits(unsigned char *r, const uint8_t *t_indices) {
    memset(r, 0, LORE_POLY_COMPRESSED_BYTES_T);
    for (size_t i = 0; i < LORE_N; i++) {
        for (size_t j = 0; j < LORE_R_BITS; j++) {
            if (((t_indices[i] >> j) & 1)) {
                r[(i * LORE_R_BITS + j) / 8] |= (unsigned char)(1 << ((i * LORE_R_BITS + j) % 8));
            }
        }
    }
}

/*************************************************
* Name:        unpack_t_bits
*
* Description: Unpacks the t-part indices from a byte array.
*
* Arguments:   - uint8_t *t_indices:    pointer to the output t_indices
* - const unsigned char *r: pointer to the input byte array
**************************************************/
void unpack_t_bits(uint8_t *t_indices, const unsigned char *r) {
    memset(t_indices, 0, LORE_N);
    for (size_t i = 0; i < LORE_N; i++) {
        for (size_t j = 0; j < LORE_R_BITS; j++) {
            if (((r[(i * LORE_R_BITS + j) / 8] >> ((i * LORE_R_BITS + j) % 8)) & 1)) {
               t_indices[i] |= (uint8_t)(1 << j);
            }
        }
    }
}
#endif

/*************************************************
* Name:        find_closest_r_idx
*
* Description: Finds the index of the closest value in the set R = {3i+1}
* to a given coefficient x_t mod t.
*
* Arguments:   - int16_t x_t: the input coefficient
*
* Returns:     the index of the closest value in R
**************************************************/
int16_t find_closest_r_idx(int16_t x_t) {
    #if LORE_T == 4
        // Rounding to S_R={0,2} under modular distance.
        // Tie is broken by choosing the smaller representative.
        // This function returns the index in S_R:
        // x_t=0,1,3 -> index 0 -> S_R value 0
        // x_t=2     -> index 1 -> S_R value 2
        return ((x_t & 3) == 2);
    #else
        (void)x_t;
        return 0;
    #endif
}



// static const int16_t Q_INV_T = 1;

/*************************************************
* Name:        poly_decode_msg_crt
*
* Description: Decodes a message from a CRT polynomial.
*
* Arguments:   - unsigned char *msg:   pointer to the output message
* - const poly_crt *r_crt: pointer to the input CRT polynomial
**************************************************/
void poly_decode_msg_crt(unsigned char *msg, const poly_crt *r_crt) {
    const int32_t TQ = LORE_T * LORE_Q;
    const int32_t TQ_HALF = TQ / 2;
    memset(msg, 0, LORE_MSG_BYTES);

#if LORE_LEVEL > 1
    ensure_bch_init();
    unsigned char recv_codeword[LORE_CODEWORD_BYTES] = {0};
    int decode_bytes = LORE_CODEWORD_BYTES;
#endif

// ================= Level 1: Repetition Code =================
#if LORE_LEVEL == 1
    for (int i = 0; i < LORE_MSG_BYTES; ++i) {
        for (int j = 0; j < 8; ++j) {
            int32_t dist_to_0 = 0;
            int32_t dist_to_1 = 0;
            
            for (int k = 0; k < LORE_K_VEC; k++) {
                int idx = (i * 8 + j) * LORE_K_VEC + k;
                int32_t a_q = r_crt->q_poly.coeffs[idx];
                int32_t a_t = r_crt->t_poly.coeffs[idx];

                /* Constant time reductions to [-Q/2, Q/2] and [-T/2, T/2] */
                a_q = a_q % LORE_Q;
                a_q += (a_q >> 31) & LORE_Q;
                a_q -= ((LORE_Q / 2 - a_q) >> 31) & LORE_Q;

                a_t = a_t % LORE_T;
                a_t += (a_t >> 31) & LORE_T;
                a_t -= ((LORE_T / 2 - a_t) >> 31) & LORE_T;

                int32_t h = (a_t - a_q) % LORE_T;
                h += (h >> 31) & LORE_T;
                h -= ((LORE_T / 2 - h) >> 31) & LORE_T;
                // CRT expansion.
                int32_t val = a_q + LORE_Q * h;
                val %= TQ;
                if (val < 0) val += TQ;
                
                int32_t d0 = val;
                if (d0 > TQ_HALF) d0 = TQ - d0; 
                int32_t d1 = abs(val - TQ_HALF); 

                dist_to_0 += d0;
                dist_to_1 += d1;
            }

            int32_t diff = dist_to_1 - dist_to_0;
            uint32_t mask = (uint32_t)(diff >> 31); 
            msg[i] |= (unsigned char)( (mask & 1) << j );
        }
    }

// ================= Level 2, 3, 4: BCH =================
#else
    for (int i = 0; i < decode_bytes * 8; ++i) {
        int32_t a_q = r_crt->q_poly.coeffs[i];
        int32_t a_t = r_crt->t_poly.coeffs[i];

        a_q %= LORE_Q;
        if (a_q > LORE_Q / 2) a_q -= LORE_Q;
        else if (a_q < -LORE_Q / 2) a_q += LORE_Q;

        a_t %= LORE_T;
        if (a_t > LORE_T / 2) a_t -= LORE_T;
        else if (a_t < -LORE_T / 2) a_t += LORE_T;

        int32_t h = a_t - a_q;
        h %= LORE_T;
        if (h > LORE_T / 2) h -= LORE_T;
        else if (h < -LORE_T / 2) h += LORE_T;

        // CRT expansion.
        int32_t val = a_q + LORE_Q * h;
        val %= TQ;
        if (val < 0) val += TQ;
        
        int32_t d0 = val;
        if (d0 > TQ_HALF) d0 = TQ - d0;
        int32_t d1 = abs(val - TQ_HALF);
        
        int32_t diff = d1 - d0;
        uint32_t mask = (uint32_t)diff >> 31; 
        recv_codeword[i / 8] |= (unsigned char)(mask << (i % 8));
    }

    unsigned char *recv_data = recv_codeword;
    unsigned char *recv_ecc = recv_codeword + LORE_MSG_BYTES;
    unsigned int errloc[LORE_BCH_T];
    
    int num_err = decode_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, recv_ecc, NULL, NULL, errloc);
    if (num_err > 0) {
        correct_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, errloc, num_err);
    }
    if (num_err >= 0) {
        memcpy(msg, recv_data, LORE_MSG_BYTES);
    } else {
        memset(msg, 0, LORE_MSG_BYTES);
    }
#endif
}
/*************************************************
* Name:        poly_encode_msg (Algorithm 9)
*
* Description: Encodes a message into a polynomial.
*
* Arguments:   - poly *r:                pointer to the output polynomial
* - const unsigned char *msg: pointer to the input message
**************************************************/
void poly_encode_msg(poly *r, const unsigned char *msg) {
    const int32_t val = (LORE_T * LORE_Q ) / 2; 
    memset(r->coeffs, 0, sizeof(int16_t) * LORE_N);

#if LORE_LEVEL == 1
    for (int i = 0; i < LORE_MSG_BYTES; ++i) { 
        for (int j = 0; j < 8; ++j) {
            int bit = (msg[i] & (1 << j)) ? 1 : 0;
            for (int k = 0; k < LORE_K_VEC; k++) {
                if (bit) r->coeffs[(i * 8 + j) * LORE_K_VEC + k] = (int16_t)val;
            }
        }
    }
#else
    ensure_bch_init();
    unsigned char ecc[LORE_ECC_BYTES] = {0};
    unsigned char codeword[LORE_CODEWORD_BYTES] = {0};

    encode_bch(lore_bch_ctx, msg, LORE_MSG_BYTES, ecc);
    memcpy(codeword, msg, LORE_MSG_BYTES);
    memcpy(codeword + LORE_MSG_BYTES, ecc, LORE_ECC_BYTES);

    for (int i = 0; i < LORE_CODEWORD_BYTES; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (codeword[i] & (1 << j)) {
                r->coeffs[8 * i + j] = (int16_t)val;
            }
        }
    }
#endif


}
/*************************************************
* Name:        poly_decode_msg (Algorithm 11)
*
* Description: Decodes a message from a polynomial.
*
* Arguments:   - unsigned char *msg: pointer to the output message
* - const poly *r:      pointer to the input polynomial
**************************************************/
void poly_decode_msg(unsigned char *msg, const poly *r) {
    const int32_t TQ = LORE_T * LORE_Q;
    
    memset(msg, 0, LORE_MSG_BYTES);

#if LORE_LEVEL == 1
    for (int i = 0; i < LORE_MSG_BYTES; ++i) { 
        for (int j = 0; j < 8; ++j) {
            int32_t S = 0;
            for (int k = 0; k < LORE_K_VEC; k++) {
                int32_t val = r->coeffs[(i * 8 + j) * LORE_K_VEC + k];
                if (val > TQ / 2) val -= TQ;
                S += val; 
            }
            if (abs(S) >= TQ / 2) { 
                msg[i] |= (unsigned char)(1 << j);
            }
        }
    }
#else
    const int32_t threshold = TQ / 4;
    ensure_bch_init();
    unsigned char recv_codeword[LORE_CODEWORD_BYTES] = {0};

    for (int i = 0; i < LORE_CODEWORD_BYTES; ++i) {
        for (int j = 0; j < 8; ++j) {
            int32_t val = r->coeffs[i * 8 + j];
            if (val > TQ / 2) val -= TQ;
            if (abs(val) > threshold) {
                recv_codeword[i] |= (unsigned char)(1 << j);
            }
        }
    }

    unsigned char *recv_data = recv_codeword;
    unsigned char *recv_ecc = recv_codeword + LORE_MSG_BYTES;
    unsigned int errloc[LORE_BCH_T];
    
    int num_err = decode_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, recv_ecc, NULL, NULL, errloc);
    if (num_err > 0) {
        correct_bch(lore_bch_ctx, recv_data, LORE_MSG_BYTES, errloc, num_err);
    }
    
    if (num_err >= 0) {
        memcpy(msg, recv_data, LORE_MSG_BYTES);
    } else {
        memset(msg, 0, LORE_MSG_BYTES);
    }
#endif
}
/*************************************************
* Name:        poly_add_scaled_msg
*
* Description: Adds a message to a CRT polynomial. (Optimized version)
*
* Arguments:   - poly_crt *r:          pointer to the CRT polynomial
* - const poly *msg_poly: pointer to the message polynomial
**************************************************/
void poly_add_scaled_msg(poly_crt *r, const poly *msg_poly) {
    for(int i = 0; i < LORE_N; ++i) {
        int32_t m_val = msg_poly->coeffs[i]; 

        // --- q-part addition ---
        int32_t q_sum = r->q_poly.coeffs[i] + m_val;
        r->q_poly.coeffs[i] = center_mod_257(q_sum);

        // --- t-part addition ---
        int16_t t_coeff = (int16_t)((r->t_poly.coeffs[i] + m_val) & (LORE_T - 1));
        if (LORE_T == 4) {
            t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
        }
        r->t_poly.coeffs[i] = t_coeff;
    }
}

/*************************************************
* Name:        rej_uniform_q
*
* Description: Rejection sampling for q-part coefficients.
* Consumes bytes from a buffer and generates uniform integers mod Q.
*
* Arguments:   - int16_t *r:           pointer to output buffer
* - unsigned int len:     requested number of integers
* - const uint8_t *buf:   pointer to input buffer
* - unsigned int buflen:  length of input buffer in bytes
*
* Returns:     number of sampled integers (at most len)
**************************************************/
unsigned int rej_uniform_q(int16_t *r,
                           unsigned int len,
                           const uint8_t *buf,
                           unsigned int buflen)
{
  unsigned int ctr = 0, pos = 0;
  uint16_t val;

  while(ctr < len && pos + 2 <= buflen) {
    val = (uint16_t)(buf[pos] | ((uint16_t)buf[pos+1] << 8));
    pos += 2;

    if (val < 65535) {
      r[ctr++] = barrett_reduce((int16_t)val);
    }
  }
  return ctr;
}

/*************************************************
* Name:        rej_uniform_t
*
* Description: Rejection sampling for t-part coefficients.
* Consumes bytes from a buffer and generates uniform integers mod T.
*
* Arguments:   - int16_t *r:           pointer to output buffer
* - unsigned int len:     requested number of integers
* - const uint8_t *buf:   pointer to input buffer
* - unsigned int buflen:  length of input buffer in bytes
*
* Returns:     number of sampled integers (at most len)
**************************************************/
unsigned int rej_uniform_t(int16_t *r,
                           unsigned int len,
                           const uint8_t *buf,
                           unsigned int buflen)
{
  unsigned int ctr = 0, pos = 0;
  const uint16_t t_limit = (0x100 / LORE_T) * LORE_T;

  while(ctr < len && pos < buflen) {
    if (buf[pos] < t_limit) {
      int16_t t_coeff = (int16_t)(buf[pos] & (LORE_T - 1));
      if (LORE_T == 4) {
          t_coeff -= ((t_coeff >> 1) & (t_coeff & 1)) << 2;
      }
      r[ctr++] = t_coeff;
    }
    pos++;
  }
  return ctr;
}
/*************************************************
* Name:        poly_mul_modt
*
* Description: Karatsuba multiplication of two polynomials in Z_t[X]/(X^N+1).
*
* Arguments:   - poly *r:       pointer to the output polynomial
* - const poly *a: pointer to the first input polynomial
* - const poly *b: pointer to the second input polynomial
**************************************************/
void poly_mul_modt(poly *r, const poly *a, const poly *b) {
    int16_t res_full[LORE_N];
    memset(res_full, 0, LORE_N * sizeof(int16_t));
    poly_mul_acc(a->coeffs, b->coeffs, res_full); // Underlying polynomial multiplication.

    for (int i = 0; i < LORE_N; i++) {
        int32_t final_val = res_full[i]; 
        
        int16_t val = (int16_t)(final_val & (LORE_T - 1));
        if (LORE_T == 4) {
            val -= ((val >> 1) & (val & 1)) << 2;
        }
        
        r->coeffs[i] = val;
    }
}

/*************************************************
* Name:        print_poly
*
* Description: Prints the coefficients of a polynomial for debugging.
*
* Arguments:   - const char* name: the name of the polynomial
* - const poly* p:      the polynomial to print
**************************************************/
void print_poly(const char* name, const poly* p) {
    printf("--- Poly: %s ---\n", name);
    for (int i = 0; i < LORE_N; ++i) {
        printf("%5d ", p->coeffs[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("-----------------------------------\n");
}

/*************************************************
* Name:        print_poly_crt
*
* Description: Prints the coefficients of a CRT polynomial for debugging.
*
* Arguments:   - const char* name:   the name of the polynomial
* - const poly_crt* p_crt: the CRT polynomial to print
**************************************************/
void print_poly_crt(const char* name, const poly_crt* p_crt) {
    printf("--- Poly CRT: %s ---\n", name);
    printf("q_poly:\n");
    for (int i = 0; i < LORE_N; ++i) {
        printf("%5d ", p_crt->q_poly.coeffs[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\nt_poly:\n");
    for (int i = 0; i < LORE_N; ++i) {
        printf("%5d ", p_crt->t_poly.coeffs[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("-----------------------------------\n");
}


/*************************************************
* Name:        lore_poly_pack_q_split
*
* Description: Splits the q-part of a polynomial into a main buffer and an overflow buffer.
*
* Arguments:   - unsigned char main_buf[LORE_N]:      output, the fixed-size main data buffer
* - unsigned char *overflow_buf:       output, buffer for overflow bits
* - const poly *p:                     input, the polynomial to pack
*
* Returns:     the number of overflow bits generated
**************************************************/
size_t lore_poly_pack_q_split(unsigned char main_buf[LORE_N], unsigned char *overflow_buf, const poly *p) {
    int total_overflow_bits = 0;

    for (int j = 0; j < LORE_N; ++j) {
        int16_t coeff = p->coeffs[j];
        /* Constant time reduction */
        coeff = barrett_reduce(coeff);
        coeff += (coeff >> 15) & LORE_Q;

        if (coeff < 255) {
            main_buf[j] = (unsigned char)coeff;
        } else {
            main_buf[j] = 0xFF; // Mark as overflow

            // Ensure the target byte is clean before writing the first overflow bit
            if ((total_overflow_bits % 8) == 0) {
                overflow_buf[total_overflow_bits / 8] = 0;
            }
            
            if (coeff == 256) {
                // If the coefficient is 256, set the corresponding bit to 1
                overflow_buf[total_overflow_bits / 8] |= (unsigned char)(1 << (total_overflow_bits % 8));
            }
            // If the coefficient is 255, we do nothing; the bit remains 0
            total_overflow_bits++;
        }
    }
    return (size_t)total_overflow_bits;
}


/*************************************************
* Name:        lore_poly_unpack_q_split
*
* Description: Reconstructs a polynomial from a main buffer and an overflow buffer.
*
* Arguments:   - poly *p:                         output, the reconstructed polynomial
* - const unsigned char main_buf[LORE_N]: input, the main data buffer
* - const unsigned char *overflow_buf:   input, the stream of overflow bits
* - int *overflow_bit_pos:           input/output, the current bit position in the overflow stream
**************************************************/
void lore_poly_unpack_q_split(poly *p, const unsigned char main_buf[LORE_N], const unsigned char *overflow_buf, int *overflow_bit_pos) {
    for (int j = 0; j < LORE_N; ++j) {
        unsigned char byte = main_buf[j];
        if (byte == 0xFF) {
            int bit = (overflow_buf[*overflow_bit_pos / 8] >> (*overflow_bit_pos % 8)) & 1;
            (*overflow_bit_pos)++;
            p->coeffs[j] = (int16_t)(255 + bit);
        } else {
            p->coeffs[j] = byte;
        }
    }
}


/*************************************************
* Name:        poly_mul_schoolbook_q
* Description: Normal domain schoolbook multiplication mod (Q, X^N+1)
**************************************************/
void poly_mul_schoolbook_q(poly *r, const poly *a, const poly *b) {
    int32_t c_tmp[2 * LORE_N] = {0};
    
    for (int i = 0; i < LORE_N; i++) {
        for (int j = 0; j < LORE_N; j++) {
            c_tmp[i + j] += (int32_t)a->coeffs[i] * b->coeffs[j];
        }
    }
    
    for (int j = 0; j < LORE_N; j++) {
        int32_t val = c_tmp[j] - c_tmp[j + LORE_N]; 
        
        // Apply 32-bit reduction.
        r->coeffs[j] = center_mod_257(val);
    }
}
