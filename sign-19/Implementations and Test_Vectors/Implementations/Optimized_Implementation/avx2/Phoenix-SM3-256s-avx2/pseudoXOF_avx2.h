#ifndef PSEUDOXOF_AVX2_H
#define PSEUDOXOF_AVX2_H

#include <stdint.h>

/**
 * AVX2-optimized pseudoXOF for 4-way parallel processing
 * 
 * Prerequisites:
 * - msg_len_bits must be byte-aligned (msg_len_bits % 8 == 0)
 * - All 4 inputs have the same length
 * 
 * Performance:
 * - Uses sm3x8_avx2 internally (4 active lanes + 4 dummy lanes)
 * - True AVX2 parallelism for GWOTS 4-way chain computation
 */
void pseudoXOFx4_avx2_aligned(
    unsigned char *out0, unsigned char *out1,
    unsigned char *out2, unsigned char *out3,
    unsigned long long output_len_bits,
    const unsigned char *in0, const unsigned char *in1,
    const unsigned char *in2, const unsigned char *in3,
    unsigned long long input_len_bits);

/**
 * AVX2-optimized pseudoXOF for 8-way parallel processing
 * 
 * Prerequisites:
 * - msg_len_bits must be byte-aligned (msg_len_bits % 8 == 0)
 * - All 8 inputs have the same length
 * 
 * Performance:
 * - Processes 8 counters in parallel using sm3x8_avx2
 * - Ideal for generating multiple SM3 blocks efficiently
 */
void pseudoXOFx8_avx2_aligned(
    unsigned char *out0, unsigned char *out1,
    unsigned char *out2, unsigned char *out3,
    unsigned char *out4, unsigned char *out5,
    unsigned char *out6, unsigned char *out7,
    unsigned long long output_len_bits,  // Output length in bits (same for all)
    const unsigned char *in0, const unsigned char *in1,
    const unsigned char *in2, const unsigned char *in3,
    const unsigned char *in4, const unsigned char *in5,
    const unsigned char *in6, const unsigned char *in7,
    unsigned long long input_len_bits);  // Input length in bits (same for all)

/**
 * AVX2-optimized pseudoXOF for single input (scalar-like interface)
 * 
 * Internally uses 8-way parallelism to generate multiple blocks efficiently
 * when output_len_bits > 256.
 */
void pseudoXOF_avx2_aligned(
    unsigned long long output_len_bits,
    const unsigned char *msg,
    unsigned long long msg_len_bits,
    unsigned char *output);

#endif