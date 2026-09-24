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

#include "params.h"
#include "poly.h"


/**
 * @brief Performs modular reduction with respect to Q=8191
 * 
 * This function implements an efficient modular reduction algorithm optimized for Q=8191.
 * It handles both positive and negative inputs correctly by using bitwise operations
 * and double reduction steps to ensure the result fits within the desired range.
 * 
 * The algorithm uses the fact that 8191 = 2^13 - 1, which allows for efficient
 * modular reduction using bit shifts and masks instead of expensive division.
 * 
 * @param x Input 32-bit signed integer value to be reduced modulo Q
 * @return uint16_t Result of x mod Q, in the range [0, Q-1]
 * 
 * Algorithm steps:
 * 1. Take absolute value of input (handling negative case later)
 * 2. Perform two rounds of (shift + mask) operation using 13-bit chunks
 * 3. Reduce final result to range [0, Q-1]
 * 4. Handle negative inputs by computing Q - result when needed
 */
static inline uint16_t standard_mod(int32_t x) {
    uint32_t res = (x < 0) ? (uint32_t)(-(int32_t)x) : (uint64_t)x;

    res = (res >> 13) + (res & 0x1FFFU);
    res = (res >> 13) + (res & 0x1FFFU);

    res = (res >= Q) ? res - Q : res;

    if (x < 0 && res != 0) {
        res = Q - res;
    }
    return (uint16_t)res; 
}

/**
 * @brief Performs central modular reduction
 * 
 * This function computes the central modulo operation, which reduces the input value
 * to the symmetric range [-Q/2, Q/2). It first applies standard modular reduction
 * to get a value in [0, Q), then adjusts the result to the central range by
 * subtracting Q when the result exceeds Q/2.
 * 
 * Since Q=8191, the threshold is 4095 (which is floor(Q/2)). Values greater than
 * 4095 will be mapped to negative values in the range [-4096, -1] by subtracting Q.
 * 
 * @param a Input integer value to be reduced
 * @return int16_t Result of central modular reduction in range [-Q/2, Q/2)
 */
static inline int16_t central_mod(int32_t a) {
    int16_t res = standard_mod(a);
    
    if (res > 4095) {
        res -= Q;
    }
    
    return res;
}

/**
 * @brief Performs modular reduction for a uint16_t value with respect to Q=8191
 * 
 * This function implements an efficient modular reduction algorithm optimized for Q=8191.
 * It takes advantage of the fact that 8191 = 2^13 - 1, allowing for fast computation
 * using bit shifts and masks instead of costly division operations.
 * 
 * The algorithm works by:
 * 1. Shifting the input right by 13 bits to get higher-order bits
 * 2. Masking the input with 0x1FFFU (0b1111111111111 = 2^13 - 1) to get lower 13 bits
 * 3. Adding these two parts together
 * 4. Adjusting the result if it's greater than or equal to Q
 * 
 * @param x Input 16-bit unsigned integer to be reduced
 * @return int16_t Result of x mod Q, in the range [0, Q-1]
 */
static inline int16_t mod_uint16(uint16_t x) {
    int16_t result = (x >> 13) + (x & 0x1FFFU);
    result = (result >= Q) ? (result - Q) : result;
    
    return result;
}

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
 * @brief Convert a polynomial to a negacyclic matrix
 * 
 * This function transforms a polynomial into its corresponding negacyclic matrix representation.
 * In a negacyclic matrix, each row is a cyclic shift of the previous row with sign changes.
 * Specifically, the matrix is constructed such that the (i,j) entry is:
 * - poly[j-i] if j >= i (direct coefficient)
 * - -poly[N+j-i] if j < i (wrapped coefficient with negation due to x^N = -1 property)
 *
 * @param poly Input polynomial coefficients of length N
 * @param matrix Output negacyclic matrix of size N×N stored in row-major order
 */
void poly_to_negacyclic_matrix(const int16_t *poly, int16_t *matrix) {
    int i, j;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            if (j >= i) {
                matrix[i*N+j] = poly[j-i];
            } else { 
                matrix[i*N+j] = -poly[N+j-i];
            }
        }
    }
}

/**
 * @brief Generate a block negacyclic matrix from an input polynomial vector
 * 
 * This function creates a block negacyclic matrix of size (k1*N) × (k2*N) from 
 * an input vector of length k1*k2*N. The matrix consists of k1*k2 blocks, where
 * each block is an N×N negacyclic matrix.
 * 
 * The matrix has the structure:
 * [ A_{0,0}  A_{0,1}  ...  A_{0,k2-1} ]
 * [ A_{1,0}  A_{1,1}  ...  A_{1,k2-1} ]
 * [  ...     ...     ...    ...       ]
 * [ A_{k1-1,0} A_{k1-1,1} ... A_{k1-1,k2-1}]
 * 
 * Where each A_{i,j} is an N×N negacyclic matrix formed from vec[(i*k2+j)*N : (i*k2+j+1)*N].
 * 
 * @param vec Input vector of length k1*k2*N
 * @param mat Output matrix of size (k1*N) × (k2*N) stored as 1D array in row-major order
 * @param k1 Number of row blocks
 * @param k2 Number of column blocks
 */
void polys_to_block_negacyclic_matrix(const int16_t *vec, int16_t *mat, int k1, int k2) {
    for (int blk_i = 0; blk_i < k1; blk_i++) {
        for (int blk_j = 0; blk_j < k2; blk_j++) {
            // Index of the current block in the input vector
            int vec_start = (blk_i * k2 + blk_j) * N;
            
            // Generate the negacyclic sub-matrix for this block
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    // Block starts at row blk_i*N, col blk_j*N
                    int row = blk_i * N + i;
                    int col = blk_j * N + j;
                    
                    // Calculate the negacyclic matrix element
                    if (j >= i) {
                        // Direct mapping: coefficient of x^(j-i) in original polynomial
                        mat[row * k2 * N + col] = vec[vec_start + j - i];
                    } else {
                        // Wrap-around with negation: due to x^n = -1 in the ring
                        mat[row * k2 * N + col] = -vec[vec_start + N - i + j];
                    }
                }
            }
        }
    }
}

void vector_mul_2(int16_t *polyvec, size_t len) 
{
    for (int i = 0; i < len; ++i) {
        polyvec[i] = (polyvec[i]<<1);
    }
}

void vector_central_modulo_q(int16_t *polyvec, size_t len) 
{
    for (int i = 0; i < len; ++i) {
        polyvec[i] = central_mod(polyvec[i]);
    }
}

void vector_mod_2(int16_t *polyvec, size_t len) 
{
    for (int i = 0; i < len; ++i) {
        polyvec[i] = (polyvec[i]&1);
    }
}

void poly_vec_add(const int16_t *a, const int16_t *b, int16_t *c, size_t k) {
    int16_t i;
    for (i = 0; i < k*N; i++) {
        c[i] = standard_mod(a[i] + b[i]);
    }
}

static void poly_mul(const int16_t *a, const int16_t *b, int16_t *c) {
    int32_t t[2 * N];
    for (int i = 0; i < 2 * N; i++) {
        t[i] = 0;
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            t[i + j] += (int32_t)(a[i] * b[j]);
        }
    }

    // x^P = 1 fold upper half
    for (int i = 0; i < N; i++) {
        c[i]  = standard_mod(t[i] - t[i + N]);
    }
}

/**
 * Compute the inner product of two polynomial vectors
 * @param x: input polynomial vector (len polynomials)
 * @param y: input polynomial vector (len polynomials)  
 * @param result: output polynomial (the inner product result)
 * @param len: number of polynomials in each vector
 */
void poly_vec_inner_product(const int16_t *x, const int16_t *y, int16_t *result, size_t len) {
    // Initialize result polynomial to zero
    for (int i = 0; i < N; i++) {
        result[i] = 0;
    }
    
    // Calculate inner product: sum of x[i] * y[i] for i = 0 to len-1
    for (size_t i = 0; i < len; i++) {
        // Get pointers to the current polynomials
        const int16_t *x_poly = &x[i * N];  // x[i] polynomial
        const int16_t *y_poly = &y[i * N];  // y[i] polynomial
        
        // Temporary storage for polynomial multiplication result
        int16_t temp[N];
        
        // Perform polynomial multiplication: temp = x[i] * y[i]
        poly_mul(x_poly, y_poly, temp);
        
        // Add to the accumulated result
        for (int j = 0; j < N; j++) {
            result[j] = standard_mod(result[j] + temp[j]);
        }
    }
}

/**
 * @brief Polynomial matrix-vector multiplication
 * 
 * Computes the product of a polynomial matrix A and a polynomial vector x,
 * storing the result in polynomial vector y.
 * Matrix A has dimensions nrows × ncols, vector x has length ncols, 
 * and vector y has length nrows.
 * Each element is a polynomial of degree N-1 (with N coefficients).
 * 
 * @param A Input polynomial matrix stored in row-major order, each polynomial has N coefficients
 * @param x Input polynomial vector, each polynomial has N coefficients
 * @param y Output polynomial vector, each polynomial has N coefficients
 * @param nrows Number of rows in matrix A
 * @param ncols Number of columns in matrix A
 */
void poly_mat_vec_mul(const int16_t *A, const int16_t *x, int16_t *y, int nrows, int ncols) {
    // Initialize output vector to zero
    for (int i = 0; i < nrows * N; i++) {
        y[i] = 0;
    }
    
    // Compute y = A * x
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            // Get coefficient array for polynomial A[row][col]
            const int16_t *A_poly = A + (row * ncols + col) * N;
            // Get coefficient array for polynomial x[col]
            const int16_t *x_poly = x + col * N;
            // Get coefficient array for output polynomial y[row]
            int16_t *y_poly = y + row * N;
            
            // Temporary storage for polynomial multiplication result
            int16_t temp[N];
            
            // Perform polynomial multiplication: A[row][col] * x[col]
            poly_mul(A_poly, x_poly, temp);
            
            // Add result to y[row]
            for (int i = 0; i < N; i++) {
                y_poly[i] = standard_mod(y_poly[i] + temp[i]);
            }
        }
    }
}

/**
 * Multiply a polynomial row vector by a polynomial matrix: y = x * A
 * @param x: input polynomial row vector (nrows polynomials)
 * @param A: input polynomial matrix (nrows x ncols polynomials)
 * @param y: output polynomial row vector (ncols polynomials)
 * @param nrows: number of elements in x (and rows of A)
 * @param ncols: number of columns of A (and elements in y)
 */
void poly_vec_mat_mul(int16_t *x, int16_t *A, int16_t *y, size_t nrows, size_t ncols) {
    size_t i, j, k;
    
    // Initialize result vector to zero
    for (j = 0; j < ncols * N; j++) {
        y[j] = 0;
    }
    
    // Compute y[j] = sum over i of (x[i] * A[i][j])
    for (j = 0; j < ncols; j++) {  // For each column of A
        for (i = 0; i < nrows; i++) {  // For each row of A
            // Calculate x[i] * A[i][j] (polynomial multiplication)
            int16_t temp[N];
            
            // Get pointers to the polynomials
            int16_t *x_poly = &x[i * N];           // x[i] polynomial
            int16_t *A_poly = &A[(i * ncols + j) * N];  // A[i][j] polynomial
            
            // Perform polynomial multiplication: temp = x[i] * A[i][j]
            poly_mul(x_poly, A_poly, temp);
            
            // Add to y[j] (accumulate results)
            for (k = 0; k < N; k++) {
                y[j * N + k] = standard_mod(y[j * N + k]+temp[k]);
            }
        }
    }
}

/**
 * @brief Multiply two  matrices
 * 
 * This function computes the product of two integer matrices A and B.
 * Matrix A is of size nrows × ncols, and matrix B is of size ncols × nout.
 * The result is a matrix of size nrows × nout.
 * 
 * The matrices are stored in row-major order as 1D arrays.
 * 
 * @param A First matrix of size nrows × ncols stored as 1D array
 * @param B Second matrix of size ncols × nout stored as 1D array
 * @param C Output matrix of size nrows × nout stored as 1D array
 * @param nrows Number of rows in matrix A
 * @param ncols Number of columns in matrix A (and rows in matrix B)
 * @param nout Number of columns in matrix B
 */
void mat_mul(const int16_t *A, 
                const int16_t *B, 
                int16_t       *C, 
                int           nrows, 
                int           ncols, 
                int           nout) {
    // Compute C = A * B
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < nout; col++) {
            // For each output element C[row][col], compute dot product of row and column
            int32_t sum = 0;
            for (int k = 0; k < ncols; k++) {
                // A[row][k] is at index row*ncols + k
                // B[k][col] is at index k*nout + col
                sum += (int32_t)(A[row * ncols + k] * B[k * nout + col]);
            }
            // C[row][col] is at index row*nout + col
            C[row * nout + col] = standard_mod(sum);
        }
    }
}

/**
 * @brief Add two integer matrices
 * 
 * This function computes the sum of two integer matrices A and B.
 * Both matrices are of size nrows × ncols.
 * The result is stored in matrix C.
 * 
 * The matrices are stored in row-major order as 1D arrays.
 * 
 * @param A First matrix of size nrows × ncols stored as 1D array
 * @param B Second matrix of size nrows × ncols stored as 1D array
 * @param C Output matrix of size nrows × ncols stored as 1D array
 * @param nrows Number of rows in the matrices
 * @param ncols Number of columns in the matrices
 */
void mat_add(const int16_t *A, const int16_t *B, int16_t *C, int nrows, int ncols) {
    // Compute C = A + B
    for (int i = 0; i < nrows * ncols; i++) {
        C[i] = standard_mod(A[i] + B[i]);
    }
}

void mat_sub(const int16_t *A, const int16_t *B, int16_t *C, int nrows, int ncols) {
    // Compute C = A - B
    for (int i = 0; i < nrows * ncols; i++) {
        C[i] = standard_mod(A[i] - B[i]);
    }
}

void compress(const int16_t *vec, size_t len, int16_t *out, uint8_t d) {
    for (int i = 0; i < len; ++i) {
        out[i] = vec[i]>>(d);
    }
}

void decompress(const int16_t *vec, size_t len, int16_t *out, uint8_t d) {
    for (int i = 0; i < len; ++i) {
        int tmp  = (vec[i]<<(d)) + (1<<(d-1));
        out[i] = tmp >= (Q-1)?(Q-1):tmp;
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

void encode_vector(const int16_t *in,  uint8_t *out, size_t inlen, uint8_t d) {
    serialize_lsb(in, out, inlen, d);
}

void decode_vector(const uint8_t *in, int16_t *out, size_t outlen, uint8_t d) {
    deserialize_lsb(in, out, outlen, d);
}


void encode_4bit_lsb(const int16_t *data, size_t n, uint8_t *out) {
    for (size_t i = 0; i < n; i += 2) {
        out[i >> 1] = (data[i] & 0x0F) | ((data[i + 1] & 0x0F) << 4);
    }
}

void decode_4bit_lsb(const uint8_t *in, size_t n, int16_t *out) {
    for (size_t i = 0; i < n; i += 2) {
        out[i    ] = in[i >> 1] & 0x0F;
        out[i + 1] = (in[i >> 1] >> 4) & 0x0F;
    }
}

void encode_6bit_lsb(const int16_t* input, size_t n, uint8_t* output) {
    for (size_t i = 0; i < n; i += 4, output += 3) {
        uint8_t x0 = input[i+0] & 0x3F; // 6 bits
        uint8_t x1 = input[i+1] & 0x3F;
        uint8_t x2 = input[i+2] & 0x3F;
        uint8_t x3 = input[i+3] & 0x3F;

        output[0] = x0 | (x1 << 6);               // x0[0:5] + x1[0:1] at bits 6-7
        output[1] = (x1 >> 2) | (x2 << 4);        // x1[2:5] + x2[0:3]
        output[2] = (x2 >> 4) | (x3 << 2);        // x2[4:5] + x3[0:5]
    }
}

void decode_6bit_lsb(const uint8_t* input, size_t n, int16_t* output) {
    for (size_t i = 0; i < n; i += 4, input += 3) {
        uint8_t b0 = input[0];
        uint8_t b1 = input[1];
        uint8_t b2 = input[2];

        output[i+0] = b0 & 0x3F;                  // x0 = bits 0-5 of b0
        output[i+1] = ((b0 >> 6) & 0x03) | ((b1 & 0x0F) << 2); // x1 = b0[6:7] + b1[0:3]
        output[i+2] = ((b1 >> 4) & 0x0F) | ((b2 & 0x03) << 4); // x2 = b1[4:7] + b2[0:1]
        output[i+3] = (b2 >> 2) & 0x3F;           // x3 = b2[2:7]
    }
}

void encode_10bit_lsb(const int16_t *data, size_t n, uint8_t *out) {
    for (size_t i = 0; i < n; i += 4) {
        out[(i >> 2) * 5 + 0] = (uint8_t)(data[i    ] & 0xFF);
        out[(i >> 2) * 5 + 1] = (uint8_t)((data[i    ] >> 8) | ((data[i + 1] & 0x3F) << 2));
        out[(i >> 2) * 5 + 2] = (uint8_t)((data[i + 1] >> 6) | ((data[i + 2] & 0x0F) << 4));
        out[(i >> 2) * 5 + 3] = (uint8_t)((data[i + 2] >> 4) | ((data[i + 3] & 0x03) << 6));
        out[(i >> 2) * 5 + 4] = (uint8_t)(data[i + 3] >> 2);
    }
}

void decode_10bit_lsb(const uint8_t *in, size_t n, int16_t *out) {
    for (size_t i = 0; i < n; i += 4) {
        out[i    ] = (uint16_t)( in[(i >> 2) * 5 + 0]        | ((uint16_t)in[(i >> 2) * 5 + 1] << 8) ) & 0x3FF;
        out[i + 1] = (uint16_t)((in[(i >> 2) * 5 + 1] >> 2) | ((uint16_t)in[(i >> 2) * 5 + 2] << 6)) & 0x3FF;
        out[i + 2] = (uint16_t)((in[(i >> 2) * 5 + 2] >> 4) | ((uint16_t)in[(i >> 2) * 5 + 3] << 4)) & 0x3FF;
        out[i + 3] = (uint16_t)((in[(i >> 2) * 5 + 3] >> 6) | ((uint16_t)in[(i >> 2) * 5 + 4] << 2)) & 0x3FF;
    }
}


/**
 * @brief Encode 11-bit values using least significant bit packing
 * 
 * This function packs 8 eleven-bit (11-bit) values into 11 bytes.
 * Each input value is assumed to be 11 bits or less. For every 8 input values,
 * it creates 11 output bytes containing all the bits of the input values
 * packed in little-endian bit order.
 * 
 * The algorithm works by taking 8 consecutive 11-bit values (x0-x7) and
 * distributing their bits across 11 output bytes. This provides efficient
 * space utilization when working with values that don't align to byte boundaries.
 * 
 * @param input Input array of 16-bit integers, each containing an 11-bit value
 * @param n Number of input values to process (must be a multiple of 8)
 * @param output Output array of 8-bit bytes to store the packed 11-bit values
 * 
 * Bit layout:
 * - x0: bits [0-10] -> output[0]: [0-7], output[1]: [0-2]
 * - x1: bits [0-10] -> output[1]: [3-7], output[2]: [0-4] 
 * - x2: bits [0-10] -> output[2]: [5-7], output[3]: [0-7], output[4]: [0]
 * - x3: bits [0-10] -> output[4]: [1-7], output[5]: [0-3]
 * - x4: bits [0-10] -> output[5]: [4-7], output[6]: [0-6]
 * - x5: bits [0-10] -> output[6]: [7], output[7]: [0-7], output[8]: [0-1]
 * - x6: bits [0-10] -> output[8]: [2-7], output[9]: [0-4]
 * - x7: bits [0-10] -> output[9]: [5-7], output[10]: [0-7]
 */
void encode_11bit_lsb(const int16_t* input, size_t n, uint8_t* output) {
    for (size_t i = 0; i < n; i += 8, output += 11) {
        // Extract 11-bit values from input (mask with 0x7FF = 0b11111111111 = 2^11-1)
        uint16_t x0 = input[i + 0] & 0x7FF;
        uint16_t x1 = input[i + 1] & 0x7FF;
        uint16_t x2 = input[i + 2] & 0x7FF;
        uint16_t x3 = input[i + 3] & 0x7FF;
        uint16_t x4 = input[i + 4] & 0x7FF;
        uint16_t x5 = input[i + 5] & 0x7FF;
        uint16_t x6 = input[i + 6] & 0x7FF;
        uint16_t x7 = input[i + 7] & 0x7FF;

        // Pack 8 eleven-bit values into 11 bytes following LSB-first bit ordering
        output[0]  = (uint8_t)(x0 & 0xFF);                          // x0[0-7] (bits 0-7 of x0)
        output[1]  = (uint8_t)(((x0 >> 8) & 0x07) | ((x1 << 3) & 0xF8)); // x0[8-10] (bits 8-10 of x0) + x1[0-4] (bits 0-4 of x1)
        output[2]  = (uint8_t)(((x1 >> 5) & 0x3F) | ((x2 << 6) & 0xC0)); // x1[5-10] (bits 5-10 of x1) + x2[0-1] (bits 0-1 of x2)
        output[3]  = (uint8_t)((x2 >> 2) & 0xFF);                     // x2[2-9] (bits 2-9 of x2)
        output[4]  = (uint8_t)(((x2 >> 10) & 0x01) | ((x3 << 1) & 0xFE)); // x2[10] (bit 10 of x2) + x3[0-6] (bits 0-6 of x3)
        output[5]  = (uint8_t)(((x3 >> 7) & 0x0F) | ((x4 << 4) & 0xF0)); // x3[7-10] (bits 7-10 of x3) + x4[0-3] (bits 0-3 of x4)
        output[6]  = (uint8_t)(((x4 >> 4) & 0x7F) | ((x5 << 7) & 0x80)); // x4[4-10] (bits 4-10 of x4) + x5[0] (bit 0 of x5)
        output[7]  = (uint8_t)((x5 >> 1) & 0xFF);                     // x5[1-8] (bits 1-8 of x5)
        output[8]  = (uint8_t)(((x5 >> 9) & 0x03) | ((x6 << 2) & 0xFC)); // x5[9-10] (bits 9-10 of x5) + x6[0-5] (bits 0-5 of x6)
        output[9]  = (uint8_t)(((x6 >> 6) & 0x1F) | ((x7 << 5) & 0xE0)); // x6[6-10] (bits 6-10 of x6) + x7[0-2] (bits 0-2 of x7)
        output[10] = (uint8_t)((x7 >> 3) & 0xFF);                     // x7[3-10] (bits 3-10 of x7)
    }
}

/**
 * Decodes 11-bit LSB (Least Significant Bit) format data
 * 
 * This function decodes packed 11-bit values from a byte array back to 16-bit signed integers.
 * Every 11 bytes in the input contain 8 packed 11-bit values, which are unpacked to fill
 * 8 positions in the output array. The process repeats for every chunk of n elements.
 * 
 * Bit packing layout (for each 11-byte group):
 * - Bytes 0-1: contain bits for x0 (11 bits: 0-7 from byte 0, 0-2 from byte 1)
 * - Bytes 1-2: contain bits for x1 (11 bits: 3-7 from byte 1 shifted left 5, 0-4 from byte 2 shifted right 3)
 * - And so on...
 * 
 * @param input Input byte array containing 11-bit packed data in LSB-first format
 * @param n Number of output elements to decode (must be multiple of 8)
 * @param output Output array of 16-bit signed integers to store decoded 11-bit values
 * 
 * Note: Each 11-bit value is masked with 0x7FF (2047 in decimal) to ensure only 11 bits are kept
 */
void decode_11bit_lsb(const uint8_t* input, size_t n, int16_t* output) {
    // Process 8 elements at a time (each group of 8 requires 11 bytes of input)
    for (size_t i = 0; i < n; i += 8, input += 11) {
        // Extract 8 11-bit values from 11 input bytes
        uint16_t x0 = (input[0] | ((uint16_t)(input[1] & 0x07) << 8)) & 0x7FF;      // x0: 8 bits from input[0] + 3 bits from input[1]
        uint16_t x1 = ((input[1] >> 3) | ((uint16_t)(input[2] & 0x3F) << 5)) & 0x7FF; // x1: 5 bits from input[1] + 6 bits from input[2]
        uint16_t x2 = (((input[2] >> 6) & 0x03) | ((uint16_t)input[3] << 2) | ((uint16_t)(input[4] & 0x01) << 10)) & 0x7FF; // x2: 2 bits from input[2] + 8 bits from input[3] + 1 bit from input[4]
        uint16_t x3 = ((input[4] >> 1) | ((uint16_t)(input[5] & 0x0F) << 7)) & 0x7FF; // x3: 7 bits from input[4] + 4 bits from input[5]
        uint16_t x4 = ((input[5] >> 4) | ((uint16_t)input[6] << 4)) & 0x7FF;         // x4: 4 bits from input[5] + 7 bits from input[6]
        uint16_t x5 = (((input[6] >> 7) & 0x01) | ((uint16_t)input[7] << 1) | ((uint16_t)(input[8] & 0x03) << 9)) & 0x7FF; // x5: 1 bit from input[6] + 8 bits from input[7] + 2 bits from input[8]
        uint16_t x6 = ((input[8] >> 2) | ((uint16_t)(input[9] & 0x1F) << 6)) & 0x7FF; // x6: 6 bits from input[8] + 5 bits from input[9]
        uint16_t x7 = ((input[9] >> 5) | ((uint16_t)input[10] << 3)) & 0x7FF;        // x7: 3 bits from input[9] + 8 bits from input[10]

        // Store the decoded 11-bit values in the output array
        output[i + 0] = x0;
        output[i + 1] = x1;
        output[i + 2] = x2;
        output[i + 3] = x3;
        output[i + 4] = x4;
        output[i + 5] = x5;
        output[i + 6] = x6;
        output[i + 7] = x7;
    }
}


void encode_noise_vector(const int16_t * in, uint8_t* out, size_t inlen) {
    int16_t poly_tmp[inlen];
    for (size_t i = 0; i < inlen; i++) {
        poly_tmp[i] = in[i] + ETA;
    }
    encode_4bit_lsb(poly_tmp, inlen, out);
}

void decode_noise_vector(const uint8_t* in, int16_t* out, size_t outlen) {
    decode_4bit_lsb(in, outlen, out);
    for (size_t i = 0; i < outlen; i++) {
        out[i] -= ETA;
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
void encode_msg(const int16_t *mat, uint8_t msg[MSG_BYTES]) 
{
    for (int i = 0; i < MSG_BYTES; ++i) {
        // Construct byte: b0<<0 | b1<<1 | ... | b7<<7
        msg[i] = (mat[i*8 + 0] << 0) |
                 (mat[i*8 + 1] << 1) |
                 (mat[i*8 + 2] << 2) |
                 (mat[i*8 + 3] << 3) |
                 (mat[i*8 + 4] << 4) |
                 (mat[i*8 + 5] << 5) |
                 (mat[i*8 + 6] << 6) |
                 (mat[i*8 + 7] << 7);
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
void decode_msg(const uint8_t msg[MSG_BYTES], int16_t mat[N*N]) 
{
    // Process each of the 16 message bytes
    for (int i = 0; i < MSG_BYTES; ++i) {
        uint8_t b = msg[i];
        // Map each bit of the byte to a polynomial coefficient
        mat[i*8 + 0] = ((b >> 0) & 1U) ? DELTA : 0;
        mat[i*8 + 1] = ((b >> 1) & 1U) ? DELTA : 0;
        mat[i*8 + 2] = ((b >> 2) & 1U) ? DELTA : 0;
        mat[i*8 + 3] = ((b >> 3) & 1U) ? DELTA : 0;
        mat[i*8 + 4] = ((b >> 4) & 1U) ? DELTA : 0;
        mat[i*8 + 5] = ((b >> 5) & 1U) ? DELTA : 0;
        mat[i*8 + 6] = ((b >> 6) & 1U) ? DELTA : 0;
        mat[i*8 + 7] = ((b >> 7) & 1U) ? DELTA : 0;
    }
}

