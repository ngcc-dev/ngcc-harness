#include "pseudoXOF_avx2.h"
#include "sm3_internal.h"
#include "auxfunc.h"
#include <stdlib.h>
#include <string.h>

/**
 * Helper function: normalize (truncate to specified bit length)
 * 
 * For pseudoXOF: clears bits beyond total_bits in the output
 */
static void normalize_local(unsigned char *input, unsigned long long total_bits) {
    const unsigned int byte_len = (total_bits + 7) / 8;
    const unsigned int remainder_bits = total_bits % 8;
    
    // Clear partial bits in last byte if needed
    if (remainder_bits != 0 && byte_len > 0) {
        unsigned char mask = (1 << remainder_bits) - 1;
        input[byte_len - 1] &= mask;
    }
    
    // Note: For pseudoXOF outputs that are multiples of 256 bits,
    // we don't need to clear extra bytes (they're not allocated)
}

/**
 * Helper function to append counter to message (byte-aligned case)
 */
static void append_counter_byte_aligned(
    unsigned char *buf,
    const unsigned char *msg,
    unsigned int msg_len_bytes,
    unsigned int ct)
{
    memcpy(buf, msg, msg_len_bytes);
    buf[msg_len_bytes + 0] = (ct >> 24) & 0xFF;
    buf[msg_len_bytes + 1] = (ct >> 16) & 0xFF;
    buf[msg_len_bytes + 2] = (ct >> 8) & 0xFF;
    buf[msg_len_bytes + 3] = ct & 0xFF;
}

/**
 * 4-way parallel pseudoXOF for byte-aligned inputs
 * 
 * Correct logic:
 * - Each lane produces its own output independently
 * - For lane i with total_blocks_needed:
 *   - ct = 1, 2, ..., total_blocks_needed
 *   - Each ct produces a 32-byte hash block
 *   - Concatenate all blocks and truncate to output_len_bits
 * 
 * AVX2 optimization strategy:
 * - Uses sm3x8_avx2 internally with 4 active lanes + 4 dummy lanes
 * - Each iteration: 4 active lanes × 1 counter per lane = 4 SM3 calls in parallel via AVX2
 * - Dummy lanes (4-7) reuse lane 0 input to avoid branch misprediction
 */
void pseudoXOFx4_avx2_aligned(
    unsigned char *out0, unsigned char *out1,
    unsigned char *out2, unsigned char *out3,
    unsigned long long output_len_bits,
    const unsigned char *in0, const unsigned char *in1,
    const unsigned char *in2, const unsigned char *in3,
    unsigned long long input_len_bits)
{
    const unsigned int msg_len_bytes = input_len_bits / 8;
    const unsigned int msg_len_with_counter = msg_len_bytes + 4;
    const unsigned int total_blocks_needed = (output_len_bits + 255) / 256;

    unsigned char *output_buf[4];
    for (int i = 0; i < 4; i++) {
        output_buf[i] = (unsigned char *)malloc(total_blocks_needed * 32);
        if (output_buf[i] == NULL) {
            for (int j = 0; j < i; j++) free(output_buf[j]);
            return;
        }
    }

    for (unsigned int block_idx = 0; block_idx < total_blocks_needed; block_idx++) {
        unsigned int ct = block_idx + 1;

        unsigned char buf0[msg_len_with_counter];
        unsigned char buf1[msg_len_with_counter];
        unsigned char buf2[msg_len_with_counter];
        unsigned char buf3[msg_len_with_counter];

        append_counter_byte_aligned(buf0, in0, msg_len_bytes, ct);
        append_counter_byte_aligned(buf1, in1, msg_len_bytes, ct);
        append_counter_byte_aligned(buf2, in2, msg_len_bytes, ct);
        append_counter_byte_aligned(buf3, in3, msg_len_bytes, ct);

        unsigned char hash0[32], hash1[32], hash2[32], hash3[32];
        unsigned char dummy4[32], dummy5[32], dummy6[32], dummy7[32];

        sm3x8_avx2(hash0, hash1, hash2, hash3,
                   dummy4, dummy5, dummy6, dummy7,
                   buf0, buf1, buf2, buf3,
                   buf0, buf1, buf2, buf3,
                   msg_len_with_counter);

        memcpy(output_buf[0] + block_idx * 32, hash0, 32);
        memcpy(output_buf[1] + block_idx * 32, hash1, 32);
        memcpy(output_buf[2] + block_idx * 32, hash2, 32);
        memcpy(output_buf[3] + block_idx * 32, hash3, 32);
    }

    const unsigned int output_bytes = (output_len_bits + 7) / 8;
    memcpy(out0, output_buf[0], output_bytes);
    memcpy(out1, output_buf[1], output_bytes);
    memcpy(out2, output_buf[2], output_bytes);
    memcpy(out3, output_buf[3], output_bytes);

    if (output_len_bits % 256 != 0) {
        normalize_local(out0, output_len_bits);
        normalize_local(out1, output_len_bits);
        normalize_local(out2, output_len_bits);
        normalize_local(out3, output_len_bits);
    }

    for (int i = 0; i < 4; i++) {
        free(output_buf[i]);
    }
}

/**
 * 8-way parallel pseudoXOF for byte-aligned inputs
 * 
 * Correct logic:
 * - Each lane produces its own output independently
 * - For lane i with total_blocks_needed:
 *   - ct = 1, 2, ..., total_blocks_needed
 *   - Each ct produces a 32-byte hash block
 *   - Concatenate all blocks and truncate to output_len_bits
 * 
 * AVX2 optimization strategy:
 * - Process 8 counters at once (ct=1..8) for all 8 lanes
 * - Each iteration: 8 lanes × 1 counter per lane = 8 SM3 calls in parallel
 */
void pseudoXOFx8_avx2_aligned(
    unsigned char *out0, unsigned char *out1,
    unsigned char *out2, unsigned char *out3,
    unsigned char *out4, unsigned char *out5,
    unsigned char *out6, unsigned char *out7,
    unsigned long long output_len_bits,
    const unsigned char *in0, const unsigned char *in1,
    const unsigned char *in2, const unsigned char *in3,
    const unsigned char *in4, const unsigned char *in5,
    const unsigned char *in6, const unsigned char *in7,
    unsigned long long input_len_bits)
{
    const unsigned int msg_len_bytes = input_len_bits / 8;
    const unsigned int msg_len_with_counter = msg_len_bytes + 4;
    const unsigned int total_blocks_needed = (output_len_bits + 255) / 256;  // blocks needed per lane
    
    // Allocate output accumulation buffers for each lane
    unsigned char *output_buf[8];
    for (int i = 0; i < 8; i++) {
        output_buf[i] = (unsigned char *)malloc(total_blocks_needed * 32);
        if (output_buf[i] == NULL) {
            // Cleanup and return on failure
            for (int j = 0; j < i; j++) free(output_buf[j]);
            return;
        }
    }
    
    // Process blocks: each iteration handles ct=iter+1 for all 8 lanes
    for (unsigned int block_idx = 0; block_idx < total_blocks_needed; block_idx++) {
        unsigned int ct = block_idx + 1;  // Counter for this block
        
        // Prepare 8 input buffers with the SAME counter but different messages
        unsigned char buf0[msg_len_with_counter];
        unsigned char buf1[msg_len_with_counter];
        unsigned char buf2[msg_len_with_counter];
        unsigned char buf3[msg_len_with_counter];
        unsigned char buf4[msg_len_with_counter];
        unsigned char buf5[msg_len_with_counter];
        unsigned char buf6[msg_len_with_counter];
        unsigned char buf7[msg_len_with_counter];
        
        // Append counter ct to each lane's input
        append_counter_byte_aligned(buf0, in0, msg_len_bytes, ct);
        append_counter_byte_aligned(buf1, in1, msg_len_bytes, ct);
        append_counter_byte_aligned(buf2, in2, msg_len_bytes, ct);
        append_counter_byte_aligned(buf3, in3, msg_len_bytes, ct);
        append_counter_byte_aligned(buf4, in4, msg_len_bytes, ct);
        append_counter_byte_aligned(buf5, in5, msg_len_bytes, ct);
        append_counter_byte_aligned(buf6, in6, msg_len_bytes, ct);
        append_counter_byte_aligned(buf7, in7, msg_len_bytes, ct);
        
        // Call sm3x8_avx2 for parallel hashing (8 lanes, same counter)
        unsigned char hash0[32], hash1[32], hash2[32], hash3[32];
        unsigned char hash4[32], hash5[32], hash6[32], hash7[32];
        
        sm3x8_avx2(hash0, hash1, hash2, hash3,
                   hash4, hash5, hash6, hash7,
                   buf0, buf1, buf2, buf3,
                   buf4, buf5, buf6, buf7,
                   msg_len_with_counter);
        
        // Store block_idx-th hash for each lane
        memcpy(output_buf[0] + block_idx * 32, hash0, 32);
        memcpy(output_buf[1] + block_idx * 32, hash1, 32);
        memcpy(output_buf[2] + block_idx * 32, hash2, 32);
        memcpy(output_buf[3] + block_idx * 32, hash3, 32);
        memcpy(output_buf[4] + block_idx * 32, hash4, 32);
        memcpy(output_buf[5] + block_idx * 32, hash5, 32);
        memcpy(output_buf[6] + block_idx * 32, hash6, 32);
        memcpy(output_buf[7] + block_idx * 32, hash7, 32);
    }
    
    // Extract final outputs (truncate to output_len_bits)
    const unsigned int output_bytes = (output_len_bits + 7) / 8;
    memcpy(out0, output_buf[0], output_bytes);
    memcpy(out1, output_buf[1], output_bytes);
    memcpy(out2, output_buf[2], output_bytes);
    memcpy(out3, output_buf[3], output_bytes);
    memcpy(out4, output_buf[4], output_bytes);
    memcpy(out5, output_buf[5], output_bytes);
    memcpy(out6, output_buf[6], output_bytes);
    memcpy(out7, output_buf[7], output_bytes);
    
    // Normalize if needed (clear partial bits in last byte)
    if (output_len_bits % 256 != 0) {
        normalize_local(out0, output_len_bits);
        normalize_local(out1, output_len_bits);
        normalize_local(out2, output_len_bits);
        normalize_local(out3, output_len_bits);
        normalize_local(out4, output_len_bits);
        normalize_local(out5, output_len_bits);
        normalize_local(out6, output_len_bits);
        normalize_local(out7, output_len_bits);
    }
    
    // Free buffers
    for (int i = 0; i < 8; i++) {
        free(output_buf[i]);
    }
}

/**
 * Single-input pseudoXOF using AVX2 internally
 */
void pseudoXOF_avx2_aligned(
    unsigned long long output_len_bits,
    const unsigned char *msg,
    unsigned long long msg_len_bits,
    unsigned char *output)
{
    const unsigned int msg_len_bytes = msg_len_bits / 8;
    const unsigned int msg_len_with_counter = msg_len_bytes + 4;
    const unsigned int total_blocks = (output_len_bits + 255) / 256;
    
    // Allocate output buffer
    unsigned char *output_buffer = (unsigned char *)malloc(total_blocks * 32);
    
    // Process in groups of 8 counters
    const unsigned int avx2_iterations = (total_blocks + 7) / 8;
    
    for (unsigned int iter = 0; iter < avx2_iterations; iter++) {
        // Prepare 8 buffers with counters ct = iter*8 + lane + 1
        // But all buffers use the SAME input message
        unsigned char buf[msg_len_with_counter];
        
        // We'll use sm3x8_avx2 with duplicate inputs
        // This is less efficient but simpler to implement
        for (int lane = 0; lane < 8; lane++) {
            unsigned int ct = iter * 8 + lane + 1;
            append_counter_byte_aligned(buf, msg, msg_len_bytes, ct);
            
            // Call scalar sm3 for simplicity (can optimize later)
            sm3(output_buffer + (iter * 8 + lane) * 32, buf, msg_len_with_counter);
        }
    }
    
    // Alternative: Use sm3x8_avx2 with 8 different counters
    // Prepare 8 buffers
    for (unsigned int iter = 0; iter < avx2_iterations; iter++) {
        unsigned char buf0[msg_len_with_counter];
        unsigned char buf1[msg_len_with_counter];
        unsigned char buf2[msg_len_with_counter];
        unsigned char buf3[msg_len_with_counter];
        unsigned char buf4[msg_len_with_counter];
        unsigned char buf5[msg_len_with_counter];
        unsigned char buf6[msg_len_with_counter];
        unsigned char buf7[msg_len_with_counter];
        
        for (int lane = 0; lane < 8; lane++) {
            unsigned int ct = iter * 8 + lane + 1;
            unsigned char *buf_lane = (lane == 0) ? buf0 :
                                      (lane == 1) ? buf1 :
                                      (lane == 2) ? buf2 :
                                      (lane == 3) ? buf3 :
                                      (lane == 4) ? buf4 :
                                      (lane == 5) ? buf5 :
                                      (lane == 6) ? buf6 : buf7;
            append_counter_byte_aligned(buf_lane, msg, msg_len_bytes, ct);
        }
        
        // AVX2 parallel hash
        unsigned char hash0[32], hash1[32], hash2[32], hash3[32];
        unsigned char hash4[32], hash5[32], hash6[32], hash7[32];
        
        sm3x8_avx2(hash0, hash1, hash2, hash3,
                   hash4, hash5, hash6, hash7,
                   buf0, buf1, buf2, buf3,
                   buf4, buf5, buf6, buf7,
                   msg_len_with_counter);
        
        // Store results
        for (int lane = 0; lane < 8; lane++) {
            unsigned int block_idx = iter * 8 + lane;
            if (block_idx < total_blocks) {
                const unsigned char *hash = (lane == 0) ? hash0 :
                                           (lane == 1) ? hash1 :
                                           (lane == 2) ? hash2 :
                                           (lane == 3) ? hash3 :
                                           (lane == 4) ? hash4 :
                                           (lane == 5) ? hash5 :
                                           (lane == 6) ? hash6 : hash7;
                memcpy(output_buffer + block_idx * 32, hash, 32);
            }
        }
    }
    
    // Extract output
    const unsigned int output_bytes = (output_len_bits + 7) / 8;
    memcpy(output, output_buffer, output_bytes);
    
    // Normalize if needed
    if (output_len_bits % 256 != 0) {
        normalize_local(output, output_len_bits);
    }
    
    free(output_buffer);
}