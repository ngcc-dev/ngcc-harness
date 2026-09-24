/**
 * @file poly.c
 * @author chengdongtao (chengdongtao2010@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-01-29
 * 
 * @copyright Copyright (c) 2026
 * 
 */


#include <stdint.h>
#include <stddef.h>
#include <immintrin.h>

#include "params.h"
#include "mod.h"
#include "poly.h"
#include "ntt.h"


/**
 * Samples vector elements from the input buffer 
 * 
 * This function reads data from a byte buffer in 2-byte chunks, performs rejection
 * sampling based on a threshold, and converts the accepted values to [0,Q-1]
 * 
 * The function processes the input buffer in pairs of bytes, combining them into
 * 16-bit values. Only values below REJECT_THRESHOLD are accepted 
 * 
 * @param buf Input byte buffer containing raw data to be sampled
 * @param vec Output vector to store processed sampling results
 * @param len Desired output vector length and maximum sampling count
 * @return No return value (void)
 */
void sample_vector(const uint8_t *buf, int16_t *vec, size_t len) {
    size_t count = 0;
    
    // Process input buffer in 2-byte chunks until reaching target length or processing 4x length of data
    for (size_t i = 0; i < 3*len && count < len; i += 2) {
        // Combine two bytes into a 16-bit value (little-endian)
        uint16_t x = ((uint16_t)  (buf[i + 1]|buf[i] << 8));
        
        // Check if sampled value is below rejection threshold
        if (x < REJECT_THRESHOLD) {
            // Apply modular reduction and store in result vector
            vec[count++] = mod_uint16(x);
        }
    }
}

/**
 * @brief Sample a noise vector from input buffer according to centered binomial distribution
 * 
 * This function generates a noise vector by sampling from a distribution that approximates
 * a centered binomial distribution. For each coefficient, it takes 2*ETA bits from the buffer,
 * splits them into two groups of ETA bits each, sums the bits in each group separately,
 * and returns the difference (pos_sum - neg_sum).
 * 
 * @param buf Input byte buffer containing random bit data to sample from
 * @param noisevec Output vector to store the generated noise coefficients
 * @param len Number of coefficients to generate in the output vector
 * 
 * @note Each coefficient will have a value in the range [-ETA, ETA]. The distribution
 *       follows a binomial-like pattern centered at 0.
 */
void sample_noise_vector(const uint8_t *buf, int16_t *noisevec, size_t len) {
    size_t coeff_idx = 0;
    size_t bit_pos = 0;  // Track position in bits across the entire buffer
    
    // Generate each coefficient using a sliding window of 2*ETA bits
    for (coeff_idx = 0; coeff_idx < len; coeff_idx++) {
        int8_t pos_sum = 0, neg_sum = 0;
        
        // Calculate the 2*ETA bits for this coefficient: ETA for positive part, ETA for negative part
        for (int b = 0; b < ETA; b++) {
            // Extract bit at position (bit_pos + b) from the buffer
            size_t byte_idx = (bit_pos + b) >> 3;
            size_t bit_idx = (bit_pos + b) & 7;
            pos_sum += (buf[byte_idx] >> bit_idx) & 1;
        }
        
        for (int b = ETA; b < 2*ETA; b++) {
            // Extract bit at position (bit_pos + b) from the buffer
            size_t byte_idx = (bit_pos + b) >> 3;
            size_t bit_idx = (bit_pos + b) & 7;
            neg_sum += (buf[byte_idx] >> bit_idx) & 1;
        }
        
        noisevec[coeff_idx] = pos_sum - neg_sum;
        
        // Move to next 2*ETA-bit window
        bit_pos += 2*ETA;
    }
}

/**
 * @brief Multiplies each element in the vector by 2
 * 
 * This function performs an in-place multiplication of all elements in the input vector by 2.
 * The operation is implemented using a left bit shift by 1 position (equivalent to multiplying by 2).
 * 
 * @param vec Pointer to the input vector containing int16_t values to be multiplied
 * @param len Length of the vector, specifying the number of elements to process
 * @return No return value (void function)
 */
void vector_mul_2(int16_t *vec, size_t len) 
{
    // Iterate through each element in the vector and multiply by 2 using left bit shift
    for (int i = 0; i < len; ++i) {
        vec[i] = (vec[i]<<1);
    }
}

/**
 * @brief Applies centered modular reduction to each element in the input vector
 * 
 * This function iterates through each element of the input vector and applies 
 * the centered_mod function to perform centered modular reduction.
 * The result replaces the original value in the vector, modifying it in-place.
 * 
 * @param vec Pointer to the input vector containing int16_t values to be reduced
 * @param len Length of the vector, specifying the number of elements to process
 * @return No return value (void function)
 */
void vector_centered_mod(int16_t *vec, size_t len) 
{
    for (int i = 0; i < len; ++i) {
        vec[i] = centered_mod(vec[i]);
    }
}

/**
 * @brief Performs modulo 2 operation on each element in the input vector
 * 
 * This function applies a bitwise AND operation with 1 to each element in the input vector,
 * effectively computing the modulo 2 of each element. This operation keeps only the least
 * significant bit of each element, resulting in all elements being either 0 or 1.
 * 
 * @param vec Pointer to the input vector containing int16_t values to perform modulo 2 operation
 * @param len Length of the vector, indicating the number of elements to process
 * @return No return value (void function)
 */
void vector_mod_2(int16_t *vec, size_t len) 
{
    // Iterate through each element in the vector and perform modulo 2 operation using bitwise AND
    for (int i = 0; i < len; ++i) {
        vec[i] = (vec[i]&1);
    }
}

/**
 * @brief Adds two polynomial vectors component-wise and stores the result
 * 
 * This function performs addition of two polynomial vectors 'a' and 'b', each containing 'k' polynomials
 * of degree N-1. The result of the addition is stored in the output vector 'c'. Each coefficient
 * is reduced modulo Q using the standard modular operation to ensure the result remains in the field.
 * 
 * @param a Pointer to the first input polynomial vector of length k*N
 * @param b Pointer to the second input polynomial vector of length k*N
 * @param c Pointer to the output polynomial vector of length k*N to store the sum
 * @param k Number of polynomials in each vector (vector dimension)
 * 
 * @note Each polynomial has N coefficients, so the total number of operations is k*N additions
 * @note The standard_mod function ensures all results are properly reduced modulo Q
 */
void poly_vec_add(const int16_t *a, const int16_t *b, int16_t *c, size_t k) {
    int16_t i;
    for (i = 0; i < k*N; i++) {
        c[i] = standard_mod(a[i] + b[i]);
    }
}

/**
 * @brief Computes the sum of multiple polynomials
 * 
 * This function adds together multiple polynomials, where each polynomial has N coefficients.
 * All polynomial coefficients are stored consecutively in the input array 'polys'.
 * The result is stored in the 'sum' array, which contains the element-wise sum of all input polynomials.
 * 
 * @param polys Pointer to the input array containing coefficients of multiple polynomials
 *              Each polynomial has N coefficients and there are numpolys polynomials
 * @param sum Output array of size N to store the sum of all polynomials
 * @param numpolys Number of polynomials to sum together
 * @return No return value (void function)
 * 
 * The function first initializes the sum array to zero, then iterates through each polynomial
 * and accumulates their coefficients at corresponding positions.
 */
static void polys_sum(const int16_t *polys, int16_t sum[N],  size_t numpolys) {
    int16_t i;
    // Initialize sum array to zeros
    for (i = 0; i < N; i++) {
        sum[i] = 0;
    }
    // Add each polynomial's coefficients to the sum array
    for (i = 0; i < numpolys; i++) {
        poly_vec_add(sum, polys + i*N, sum, 1);
    }
}

/**
 * @brief Performs pointwise multiplication of two polynomials
 * 
 * This function multiplies two polynomials element-wise (coefficient-wise) and applies
 * standard modular reduction to each product. For each index i from 0 to N-1,
 * it computes c[i] = (a[i] * b[i]) mod Q using the standard_mod function.
 * 
 * @param a Pointer to the first polynomial array of length N containing int16_t coefficients
 * @param b Pointer to the second polynomial array of length N containing int16_t coefficients
 * @param c Output polynomial array of length N to store the pointwise multiplication results
 * 
 * @note The function performs in-place modular reduction using standard_mod to ensure
 *       all results remain in the proper field [0, Q-1]
 * @note All arrays must have at least N elements
 */
static void poly_pointwise_mul(const int16_t *a, const int16_t *b, int16_t *c) {
    int16_t i;
    // Perform element-wise multiplication followed by standard modular reduction
    for (i = 0; i < N; i++) {
        c[i] = standard_mod(a[i] * b[i]);
    }
}

/**
 * @brief Performs Number Theoretic Transform (NTT) on a vector of polynomials
 * 
 * This function applies the forward Number Theoretic Transform to each polynomial
 * in a vector containing k polynomials. Each polynomial has N coefficients.
 * 
 * @param polyvec Pointer to the polynomial vector containing k polynomials, each with N coefficients
 * @param k Number of polynomials in the vector
 * @return No return value (void function)
 * 
 * The function iterates through each polynomial in the vector and applies the NTT transformation
 * using the ntt_640 function with forward direction parameter.
 */
static void poly_vec_ntt(int16_t *polyvec, size_t k) {
    for (int i = 0; i < k; i++) {
        ntt_640(polyvec + i*N, NTT_FORWARD);
    }
}

/**
 * @brief Performs inverse number theoretic transform (INTT) on a vector of polynomials
 * 
 * This function applies the inverse number theoretic transform to each polynomial 
 * in a polynomial vector, converting from frequency domain back to time domain.
 * 
 * @param polyvec Pointer to the polynomial vector array, where each element is 
 *                an array of N polynomial coefficients
 * @param k       Number of polynomials in the vector
 * @return No return value
 */
static void poly_vec_intt(int16_t *polyvec, size_t k) {
    // Iterate through each polynomial in the vector and apply inverse number theoretic transform
    for (int i = 0; i < k; i++) {
        ntt_640(polyvec + i*N, NTT_INVERSE);
    }
}

/**
 * @brief  This function computes the inner product (dot product) of two polynomial vectors a and b,
 * where the coefficients of the input polynomials have already been transformed via forward NTT.
 * It performs element-wise polynomial multiplication (pointwise multiplication in frequency domain),
 * then applies inverse NTT and sums the results to obtain the inner product.
 * 
 * @param a First polynomial vector, containing CHEETAH_K polynomials, each with N NTT-transformed coefficients
 * @param b Second polynomial vector, containing CHEETAH_K polynomials, each with N NTT-transformed coefficients
 * @param c Output polynomial storing the inner product result (single polynomial)
 * 
 * @note The input polynomial coefficients must be NTT-transformed so that multiplication can be performed directly in the frequency domain
 * @note The result is transformed back to time domain via INTT, then all polynomial coefficients are summed to get the final result
 */
void poly_vec_pwmul(const int16_t *a, const int16_t *b, int16_t *c ) {
    int16_t tmp[CHEETAH_K*N];
    // Perform pointwise multiplication (multiplication in frequency domain) for each pair of polynomials
    for (int i = 0; i < CHEETAH_K; i++) {
        poly_pointwise_mul(a+i*N, b+i*N, tmp+i*N);
    }
    // Apply inverse NTT transform to all result polynomials
    poly_vec_intt(tmp, CHEETAH_K);
    // Sum all polynomial coefficients to get the final inner product result
    polys_sum(tmp, c, CHEETAH_K);
}

/**
 * @brief Performs polynomial matrix-vector multiplication y = A * x, 
 * where the input polynomial coefficients must have been transformed via forward NTT
 * 
 * This function implements matrix-vector multiplication between polynomial matrix A 
 * and vector x, operating in the frequency domain. Matrix A is a CHEETAH_K x CHEETAH_K polynomial matrix,
 * and vector x is a polynomial vector of length CHEETAH_K. The polynomial coefficients in both 
 * matrix and vector have been transformed via NTT.
 * 
 * @param A Input polynomial matrix of dimensions CHEETAH_K x CHEETAH_K, 
 *          each element is a polynomial with N coefficients that have been NTT-transformed
 * @param x Input polynomial vector of length CHEETAH_K, each element is a polynomial 
 *          with N coefficients that have been NTT-transformed
 * @param y Output polynomial vector of length CHEETAH_K, storing the result of 
 *          matrix-vector multiplication
 */
void poly_mat_vec_pwmul(const int16_t *A, const int16_t *x, int16_t *y) {
    for (int i = 0; i < CHEETAH_K; i++) {
        poly_vec_pwmul(A+i*CHEETAH_K*N, x, y+i*N);
    }
}

/**
 * @brief Performs polynomial vector-matrix multiplication y = x * A, 
 * where the input polynomial coefficients must have been transformed via forward NTT
 * 
 * This function computes the operation y = x * A where A is a matrix of polynomials,
 * x is an input vector of polynomials, and y is the resulting output vector.
 * 
 * @param A Input polynomial matrix of dimensions CHEETAH_K x CHEETAH_K, 
 *          each element is a polynomial with N coefficients that have been NTT-transformed
 * @param x Input polynomial vector of length CHEETAH_K, each element is a polynomial 
 *          with N coefficients that have been NTT-transformed
 * @param y Output polynomial vector of length CHEETAH_K, storing the result of 
 *          matrix-vector multiplication
 */
void poly_vec_mat_pwmul(const int16_t *x, const int16_t *A,  int16_t *y) {
    // Temporary storage for intermediate calculations
    int16_t tmp[CHEETAH_K*N];
    
    // Outer loop: iterate over each row of the output vector
    for  (int i = 0; i < CHEETAH_K; i++) {
        // Inner loop: compute pointwise multiplication of row i of A with vector x
        for (int j = 0; j < CHEETAH_K; j++) {
             // Perform pointwise multiplication: A[j][i] * x[j]
             poly_pointwise_mul(A+j*CHEETAH_K*N+i*N, x+j*N, tmp+j*N);
        }
        // Apply inverse number theoretic transform to temporary results
        poly_vec_intt(tmp, CHEETAH_K);
        // Sum the transformed polynomials to get the final result for position i
        polys_sum(tmp, y+i*N, CHEETAH_K);
    }
}


/**
 * @brief Compresses a vector by right-shifting each element by d bits
 * 
 * This function performs compression on an input vector by applying a right shift operation
 * to each element. The right shift operation reduces the precision of each element by d bits,
 * effectively dividing each value by 2^d and truncating the fractional part.
 * 
 * @param vec Input vector of 16-bit signed integers to be compressed
 * @param len Length of the input vector (number of elements to process)
 * @param out Output vector of 16-bit signed integers to store the compressed results
 * @param d Number of bits to right-shift each element (compression factor)
 * 
 * The function processes each element independently, applying the same bit shift operation
 * to achieve uniform compression across all elements in the vector.
 */
void compress(const int16_t *vec, size_t len, int16_t *out, uint8_t d) {
    for (int i = 0; i < len; ++i) {
        out[i] = vec[i]>>(d);
    }
}

/**
 * Decompresses a vector by shifting and rounding
 * 
 * This function performs decompression on a vector of compressed values by left-shifting
 * each element by 'd' bits and adding a rounding offset of 2^(d-1). If the result exceeds
 * Q-1, it is clamped to Q-1.
 * 
 * @param vec Input vector of compressed values
 * @param len Length of the input vector
 * @param out Output vector to store decompressed values
 * @param d Number of bits to shift during decompression
 */
void decompress(const int16_t *vec, size_t len, int16_t *out, uint8_t d) {
    for (int i = 0; i < len; ++i) {
        int tmp  = (vec[i]<<(d)) + (1<<(d-1));  // Shift left by d bits and add rounding offset
        out[i] = tmp >= (Q-1)?(Q-1):tmp;        // Clamp result to maximum value of Q-1
    }
}

/**
 * @brief Serializes integer vector into a byte array in LSB-first order
 * 
 * This function packs the lower 'd' bits of each integer in the input vector 
 * into a byte array in a bit-packed format. Bits are arranged in LSB-first order,
 * meaning the least significant bit of each integer is placed first.
 * 
 * @param in   Pointer to the input array containing integer values
 * @param out  Output byte array to store the serialized data
 * @param n    Number of integers to serialize
 * @param d    Number of bits to serialize from each integer (bit depth)
 * 
 * The function calculates the required number of output bytes as (n * d + 7) / 8,
 * initializes the output buffer to zero, and then packs the bits sequentially.
 * Each integer contributes 'd' bits to the output, with bits ordered from
 * least significant to most significant within each integer.
 */
static inline void serialize_lsb(const int16_t *in, uint8_t* out, uint16_t n, uint8_t d) {
    // Calculate total number of bytes needed and initialize output buffer
    size_t total_bytes = (n * d + 7) >> 3;
    for (size_t i = 0; i < total_bytes; ++i) {
        out[i] = 0;
    }

    size_t bit = 0;
    for (size_t i = 0; i < n; ++i) {
        for (uint8_t b = 0; b < d; ++b, ++bit) {
            // Extract the b-th bit of in[i] and place it at the correct position in output
            out[bit >> 3] |= ((in[i] >> b) & 1U) << (bit & 7);
        }
    }
}

/**
 * @brief Deserialize integer vector from byte stream using LSB-first ordering
 * 
 * This function extracts integer values from an input byte stream by reading
 * bits in least significant bit first order. Each integer is represented using
 * 'd' bits, which are collected sequentially from the input data.
 * 
 * @param in  Pointer to the input byte stream containing serialized data
 * @param out Pointer to the output integer vector to store decoded values
 * @param n   Number of integers to deserialize
 * @param d   Number of bits used to represent each integer
 * 
 * The function processes 'n' integers, extracting 'd' bits for each integer
 * from the input stream in LSB-first bit ordering.
 */
static inline void deserialize_lsb(const uint8_t *in, int16_t *out, uint16_t n, uint8_t d) {
    size_t bit = 0;
    for (size_t i = 0; i < n; ++i) {
        uint16_t val = 0;
        /* Extract d bits to form the current coefficient value */
        for (unsigned b = 0; b < d; ++b, ++bit){
            val |= ((in[bit >> 3] >> (bit & 7)) & 1U) << b;
        }
        out[i] = val;
    }
}

/**
 * @brief Encodes an integer vector into a byte array using LSB-first serialization
 * 
 * This function takes an input vector of 16-bit integers and serializes it into
 * a byte array by packing the lower 'd' bits of each integer in LSB-first order.
 * The serialization algorithm uses the helper function serialize_lsb to perform
 * the actual bit packing operation.
 * 
 * @param[in]  in     Pointer to the input array of 16-bit integers to be encoded
 * @param[out] out    Output byte array to store the serialized data
 * @param[in]  inlen  Number of elements in the input integer array
 * @param[in]  d      Bit depth - number of lower bits to extract from each input integer
 * 
 * @return No return value (void function)
 * 
 * @note The output array size should be at least ceil(inlen * d / 8) bytes to accommodate all data
 * @note Uses LSB-first bit ordering during serialization
 */
void encode_vector(const int16_t *in,  uint8_t *out, size_t inlen, uint8_t d) {
    serialize_lsb(in, out, inlen, d);
}

/**
 * @brief Decodes a vector from a byte stream using LSB-first serialization
 * 
 * This function deserializes integer values from an input byte stream by reading
 * bits in least significant bit first order. Each integer is reconstructed using
 * 'd' bits extracted sequentially from the input data. This is the reverse operation
 * of encode_vector.
 * 
 * @param in     Pointer to the input byte stream containing serialized data
 * @param out    Pointer to the output integer vector to store decoded values
 * @param outlen Number of integers to deserialize from the input stream
 * @param d      Number of bits used to represent each integer in the serialized format
 * 
 * @note This function calls deserialize_lsb internally to perform the actual bit extraction
 * @note The output vector must have sufficient space to store 'outlen' integer values
 */
void decode_vector(const uint8_t *in, int16_t *out, size_t outlen, uint8_t d) {
    deserialize_lsb(in, out, outlen, d);
}


/**
 * @brief AVX2 optimized - Encodes 16-bit integers to 8-bit bytes
 * 
 * Processes 16 int16_t values per iteration using packus_epi16
 */
void encode_8bit_lsb(const int16_t *data, size_t datalen, uint8_t *out) {
    size_t i = 0;
    size_t simd_len = datalen & ~15UL;  // Round down to multiple of 16
    
    // SIMD processing: 16 int16_t -> 16 uint8_t per iteration
    for (; i < simd_len; i += 16) {
        // Load 16 int16_t values (32 bytes) into AVX2 register
        __m256i v = _mm256_loadu_si256((__m256i*)(data + i));
        
        // Pack 16-bit to 8-bit with unsigned saturation
        // Result: 32 uint8_t values, but only lower 16 are valid
        __m256i p = _mm256_packus_epi16(v, v);
        
        // Permute to consolidate valid bytes into lower 128 bits
        __m256i p2 = _mm256_permute4x64_epi64(p, 0xD8);
        
        // Store lower 128 bits (16 bytes)
        _mm_storeu_si128((__m128i*)(out + i), _mm256_castsi256_si128(p2));
    }
    
    // Scalar tail processing
    for (; i < datalen; i++) {
        out[i] = (uint8_t)(data[i] & 0xFF);
    }
}

/**
 * @brief AVX2 optimized - Decodes 8-bit bytes to 16-bit integers
 * 
 * Processes 16 uint8_t values per iteration using unpack with zero extension
 */
void decode_8bit_lsb(const uint8_t *in, size_t outlen, int16_t *out) {
    size_t i = 0;
    size_t simd_len = outlen & ~15UL;  // Round down to multiple of 16
    __m256i zero = _mm256_setzero_si256();
    
    // SIMD processing: 16 uint8_t -> 16 int16_t per iteration
    for (; i < simd_len; i += 16) {
        // Load 16 bytes into 128-bit register and extend to 256-bit
        __m128i v128 = _mm_loadu_si128((__m128i*)(in + i));
        __m256i v = _mm256_castsi128_si256(v128);
        
        // Unpack low 8 bytes: zero-extend to 16-bit (8 int16_t)
        __m256i lo = _mm256_unpacklo_epi8(v, zero);
        
        // Unpack high 8 bytes: zero-extend to 16-bit (8 int16_t)
        __m256i hi = _mm256_unpackhi_epi8(v, zero);
        
        // Extract and store lower 128 bits from each (16 bytes total for 16 int16_t)
        _mm_storeu_si128((__m128i*)(out + i), _mm256_castsi256_si128(lo));
        _mm_storeu_si128((__m128i*)(out + i + 8), _mm256_castsi256_si128(hi));
    }
    
    // Scalar tail processing
    for (; i < outlen; i++) {
        out[i] = (int16_t)in[i];
    }
}


void encode_11bit_lsb(const int16_t* input, size_t n, uint8_t* output) {
    // Process 256 samples per iteration (32 groups of 8)
    for (size_t i = 0; i < n; i += 256) {
        // Load 256 int16_t = 512 bytes = 16 x __m256i
        __m256i v[16];
        for (int j = 0; j < 16; ++j) {
            v[j] = _mm256_load_si256((__m256i*)(input + i + j * 16));
        }

        // Mask to 11 bits (0x7FF)
        const __m256i mask11 = _mm256_set1_epi16(0x7FF);
        for (int j = 0; j < 16; ++j) {
            v[j] = _mm256_and_si256(v[j], mask11);
        }

        // Store to aligned temp buffer for fast access
        uint16_t buf[256] __attribute__((aligned(32)));
        for (int j = 0; j < 16; ++j) {
            _mm256_store_si256((__m256i*)(buf + j * 16), v[j]);
        }

        // Unroll 32 groups of 8 samples → 11 bytes each
        #pragma GCC unroll 32
        for (int g = 0; g < 32; ++g) {
            const int idx = g * 8;
            const int out_idx = (i / 8 + g) * 11;
            uint8_t* out = output + out_idx;

            uint16_t x0 = buf[idx + 0];
            uint16_t x1 = buf[idx + 1];
            uint16_t x2 = buf[idx + 2];
            uint16_t x3 = buf[idx + 3];
            uint16_t x4 = buf[idx + 4];
            uint16_t x5 = buf[idx + 5];
            uint16_t x6 = buf[idx + 6];
            uint16_t x7 = buf[idx + 7];

            // Use word-level packing (same as original, but compiler can optimize better in loop)
            out[0]  = (uint8_t)(x0);
            out[1]  = (uint8_t)(((x0 >> 8) & 0x07) | (x1 << 3));
            out[2]  = (uint8_t)(((x1 >> 5) & 0x3F) | (x2 << 6));
            out[3]  = (uint8_t)(x2 >> 2);
            out[4]  = (uint8_t)(((x2 >> 10) & 0x01) | (x3 << 1));
            out[5]  = (uint8_t)(((x3 >> 7) & 0x0F) | (x4 << 4));
            out[6]  = (uint8_t)(((x4 >> 4) & 0x7F) | (x5 << 7));
            out[7]  = (uint8_t)(x5 >> 1);
            out[8]  = (uint8_t)(((x5 >> 9) & 0x03) | (x6 << 2));
            out[9]  = (uint8_t)(((x6 >> 6) & 0x1F) | (x7 << 5));
            out[10] = (uint8_t)(x7 >> 3);
        }
    }
}

void decode_11bit_lsb(const uint8_t* input, size_t n, int16_t* output) {
    for (size_t i = 0; i < n; i += 256) {
        const uint8_t* src = input + (i / 8) * 11;

        // Load 352 bytes as 11 x __m256i
        __m256i b[11];
        for (int j = 0; j < 11; ++j) {
            b[j] = _mm256_load_si256((__m256i*)(src + j * 32));
        }

        // Flatten to aligned buffer
        uint8_t buf[352] __attribute__((aligned(32)));
        for (int j = 0; j < 11; ++j) {
            _mm256_store_si256((__m256i*)(buf + j * 32), b[j]);
        }

        // Unpack 32 groups
        #pragma GCC unroll 32
        for (int g = 0; g < 32; ++g) {
            const int in_idx = g * 11;
            const int out_idx = i + g * 8;
            const uint8_t* in = buf + in_idx;

            uint16_t x0 = (in[0] | ((uint16_t)(in[1] & 0x07) << 8));
            uint16_t x1 = ((in[1] >> 3) | ((uint16_t)(in[2] & 0x3F) << 5));
            uint16_t x2 = (((in[2] >> 6) & 0x03) | ((uint16_t)in[3] << 2) | ((uint16_t)(in[4] & 0x01) << 10));
            uint16_t x3 = ((in[4] >> 1) | ((uint16_t)(in[5] & 0x0F) << 7));
            uint16_t x4 = ((in[5] >> 4) | ((uint16_t)in[6] << 4));
            uint16_t x5 = (((in[6] >> 7) & 0x01) | ((uint16_t)in[7] << 1) | ((uint16_t)(in[8] & 0x03) << 9));
            uint16_t x6 = ((in[8] >> 2) | ((uint16_t)(in[9] & 0x1F) << 6));
            uint16_t x7 = ((in[9] >> 5) | ((uint16_t)in[10] << 3));

            // Mask to 11 bits (defensive)
            output[out_idx + 0] = x0 & 0x7FF;
            output[out_idx + 1] = x1 & 0x7FF;
            output[out_idx + 2] = x2 & 0x7FF;
            output[out_idx + 3] = x3 & 0x7FF;
            output[out_idx + 4] = x4 & 0x7FF;
            output[out_idx + 5] = x5 & 0x7FF;
            output[out_idx + 6] = x6 & 0x7FF;
            output[out_idx + 7] = x7 & 0x7FF;
        }
    }
}

void encode_13bit_lsb(const uint16_t* data, size_t datalen, uint8_t* out) {
    // Process 256 samples per iteration (416 output bytes)
    for (size_t i = 0; i < datalen; i += 256) {
        // Load 256 uint16_t = 512 bytes = 16 x __m256i
        __m256i v[16];
        for (int j = 0; j < 16; ++j) {
            v[j] = _mm256_load_si256((__m256i*)(data + i + j * 16));
        }

        // Mask to 13 bits
        const __m256i mask13 = _mm256_set1_epi16(0x1FFF);
        for (int j = 0; j < 16; ++j) {
            v[j] = _mm256_and_si256(v[j], mask13);
        }

        // Store to aligned buffer for fast access
        uint16_t buf[256] __attribute__((aligned(32)));
        for (int j = 0; j < 16; ++j) {
            _mm256_store_si256((__m256i*)(buf + j * 16), v[j]);
        }

        // Unroll 32 groups of 8 samples
        #pragma GCC unroll 32
        for (int g = 0; g < 32; ++g) {
            const int idx = g * 8;
            const int out_idx = (i / 8 + g) * 13;
            uint8_t* pt = out + out_idx;

            // Zero 13 bytes (compiler may optimize to rep stosb or pxor)
            uint64_t* p64 = (uint64_t*)pt;
            p64[0] = 0;
            p64[1] = 0;
            pt[12] = 0;

            uint16_t v0 = buf[idx + 0];
            uint16_t v1 = buf[idx + 1];
            uint16_t v2 = buf[idx + 2];
            uint16_t v3 = buf[idx + 3];
            uint16_t v4 = buf[idx + 4];
            uint16_t v5 = buf[idx + 5];
            uint16_t v6 = buf[idx + 6];
            uint16_t v7 = buf[idx + 7];

            // Use word-level operations instead of bit-by-bit
            pt[0] = (uint8_t)(v0);
            pt[1] = (uint8_t)((v0 >> 8) | (v1 << 5));
            pt[2] = (uint8_t)(v1 >> 3);
            pt[3] = (uint8_t)((v1 >> 11) | (v2 << 2));
            pt[4] = (uint8_t)((v2 >> 6) | (v3 << 7));
            pt[5] = (uint8_t)(v3 >> 1);
            pt[6] = (uint8_t)((v3 >> 9) | (v4 << 4));
            pt[7] = (uint8_t)(v4 >> 4);
            pt[8] = (uint8_t)((v4 >> 12) | (v5 << 1));
            pt[9] = (uint8_t)((v5 >> 7) | (v6 << 6));
            pt[10] = (uint8_t)(v6 >> 2);
            pt[11] = (uint8_t)((v6 >> 10) | (v7 << 3));
            pt[12] = (uint8_t)(v7 >> 5);
        }
    }
}


void decode_13bit_lsb(const uint8_t* in, size_t outlen, uint16_t* out) {
    for (size_t i = 0; i < outlen; i += 256) {
        const uint8_t* src = in + (i / 8) * 13;

        // Load 416 bytes as 13 x __m256i
        __m256i b[13];
        for (int j = 0; j < 13; ++j) {
            b[j] = _mm256_load_si256((__m256i*)(src + j * 32));
        }

        // Flatten to aligned buffer
        uint8_t buf[416] __attribute__((aligned(32)));
        for (int j = 0; j < 13; ++j) {
            _mm256_store_si256((__m256i*)(buf + j * 32), b[j]);
        }

        // Unpack 32 groups
        #pragma GCC unroll 32
        for (int g = 0; g < 32; ++g) {
            const int in_idx = g * 13;
            const int out_idx = i + g * 8;
            const uint8_t* pt = buf + in_idx;

            uint16_t v0 = (pt[0]) |
                          (pt[1] << 8);

            uint16_t v1 = ((pt[1] >> 5) & 0x07) |
                          (pt[2] << 3) |
                          ((pt[3] & 0x03) << 11);

            uint16_t v2 = ((pt[3] >> 2) & 0x3F) |
                          ((pt[4] & 0x7F) << 6);

            uint16_t v3 = ((pt[4] >> 7) & 0x01) |
                          (pt[5] << 1) |
                          ((pt[6] & 0x0F) << 9);

            uint16_t v4 = ((pt[6] >> 4) & 0x0F) |
                          (pt[7] << 4) |
                          ((pt[8] & 0x01) << 12);

            uint16_t v5 = ((pt[8] >> 1) & 0x7F) |
                          ((pt[9] & 0x3F) << 7);

            uint16_t v6 = ((pt[9] >> 6) & 0x03) |
                          (pt[10] << 2) |
                          ((pt[11] & 0x07) << 10);

            uint16_t v7 = ((pt[11] >> 3) & 0x1F) |
                          (pt[12] << 5);

            // Mask to 13 bits (defensive)
            out[out_idx + 0] = v0 & 0x1FFF;
            out[out_idx + 1] = v1 & 0x1FFF;
            out[out_idx + 2] = v2 & 0x1FFF;
            out[out_idx + 3] = v3 & 0x1FFF;
            out[out_idx + 4] = v4 & 0x1FFF;
            out[out_idx + 5] = v5 & 0x1FFF;
            out[out_idx + 6] = v6 & 0x1FFF;
            out[out_idx + 7] = v7 & 0x1FFF;
        }
    }
}


/**
 * Encodes a message matrix into a compact byte array
 * 
 * This function encodes a matrix with binary elements (0 or 1) into 
 * a compact byte representation. Each group of 8 elements is packed into 
 * a single byte, with the first coefficient becoming the least significant bit.
 * 
 * The encoding maps 8 matrix elements to 1 byte:
 * - mat[i*8 + 0] -> bit 0 (LSB)
 * - mat[i*8 + 1] -> bit 1
 * - mat[i*8 + 2] -> bit 2
 * - mat[i*8 + 3] -> bit 3
 * - mat[i*8 + 4] -> bit 4
 * - mat[i*8 + 5] -> bit 5
 * - mat[i*8 + 6] -> bit 6
 * - mat[i*8 + 7] -> bit 7 (MSB)
 * 
 * @param mat Input matrix with binary elements (0 or 1)
 * @param msg Output buffer of size MSG_BYTES to store encoded data
 */
void encode_msg(const int16_t *msgpoly, uint8_t msg[MSG_BYTES]) 
{
    for (int i = 0; i < MSG_BYTES; ++i) {
        // Construct byte: b0<<0 | b1<<1 | ... | b7<<7
        msg[i] = (msgpoly[i*8 + 0] << 0) |
                 (msgpoly[i*8 + 1] << 1) |
                 (msgpoly[i*8 + 2] << 2) |
                 (msgpoly[i*8 + 3] << 3) |
                 (msgpoly[i*8 + 4] << 4) |
                 (msgpoly[i*8 + 5] << 5) |
                 (msgpoly[i*8 + 6] << 6) |
                 (msgpoly[i*8 + 7] << 7);
    }
}

/**
 *  Decodes a message into a matrix
 * 
 * This function takes a message and decodes it into a binary matrix. 
 * If a bit is set (1), the matrix element is set to DELTA, 
 * otherwise it is set to 0. Each byte is expanded into 8 elements
 * 
 * @param msg  Pointer to the message to decode
 * @param mat Pointer to the matrix
 */
void decode_msg(const uint8_t msg[MSG_BYTES], int16_t *msgpoly) 
{
    // Process each of the 16 message bytes
    for (int i = 0; i < MSG_BYTES; ++i) {
        uint8_t b = msg[i];
        // Map each bit of the byte to a polynomial coefficient
        msgpoly[i*8 + 0] = ((b >> 0) & 1U) ? DELTA : 0;
        msgpoly[i*8 + 1] = ((b >> 1) & 1U) ? DELTA : 0;
        msgpoly[i*8 + 2] = ((b >> 2) & 1U) ? DELTA : 0;
        msgpoly[i*8 + 3] = ((b >> 3) & 1U) ? DELTA : 0;
        msgpoly[i*8 + 4] = ((b >> 4) & 1U) ? DELTA : 0;
        msgpoly[i*8 + 5] = ((b >> 5) & 1U) ? DELTA : 0;
        msgpoly[i*8 + 6] = ((b >> 6) & 1U) ? DELTA : 0;
        msgpoly[i*8 + 7] = ((b >> 7) & 1U) ? DELTA : 0;
    }
}

