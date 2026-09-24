/**
 * Test suite for pseudoXOF_avx2_aligned correctness verification
 * 
 * Strategy:
 * 1. Generate 8 different test inputs with various lengths
 * 2. Compute reference outputs using scalar pseudoXOF
 * 3. Compute test outputs using AVX2 parallel pseudoXOF
 * 4. Compare all outputs and report discrepancies
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "auxfunc.h"
#include "pseudoXOF_avx2.h"
#include "randombytes.h"

#define TEST_CASES 5

static void print_hex(const char *label, const unsigned char *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

static int compare_outputs(
    const unsigned char *ref,
    const unsigned char *test,
    size_t len,
    int lane_idx,
    const char *test_name)
{
    if (memcmp(ref, test, len) != 0) {
        printf("ERROR: Output mismatch for lane %d in test '%s'\n", lane_idx, test_name);
        print_hex("Reference", ref, len);
        print_hex("AVX2     ", test, len);
        return -1;
    }
    return 0;
}

/**
 * Test case 1: Short outputs (16 bytes = 128 bits)
 * - Each input requires only 1 SM3 block
 */
static int test_short_outputs(void) {
    printf("\n=== Test 1: Short outputs (128 bits) ===\n");
    
    const unsigned int input_len_bits = 40 * 8;  // 40 bytes
    const unsigned int output_len_bits = 16 * 8; // 16 bytes
    const unsigned int input_len_bytes = input_len_bits / 8;
    const unsigned int output_len_bytes = output_len_bits / 8;
    
    unsigned char inputs[8][input_len_bytes];
    unsigned char ref_outputs[8][output_len_bytes];
    unsigned char avx2_outputs[8][output_len_bytes];
    
    // Generate random inputs
    for (int i = 0; i < 8; i++) {
        randombytes(inputs[i], input_len_bytes);
    }
    
    // Compute reference outputs (scalar pseudoXOF)
    printf("Computing reference outputs...\n");
    for (int i = 0; i < 8; i++) {
        int ret = pseudoXOF(output_len_bits, inputs[i], input_len_bits, ref_outputs[i]);
        if (ret != 0) {
            printf("ERROR: scalar pseudoXOF failed for lane %d\n", i);
            return -1;
        }
    }
    
    // Compute AVX2 outputs
    printf("Computing AVX2 outputs...\n");
    pseudoXOFx8_avx2_aligned(
        avx2_outputs[0], avx2_outputs[1], avx2_outputs[2], avx2_outputs[3],
        avx2_outputs[4], avx2_outputs[5], avx2_outputs[6], avx2_outputs[7],
        output_len_bits,
        inputs[0], inputs[1], inputs[2], inputs[3],
        inputs[4], inputs[5], inputs[6], inputs[7],
        input_len_bits);
    
    // Compare outputs
    printf("Comparing outputs...\n");
    int errors = 0;
    for (int i = 0; i < 8; i++) {
        if (compare_outputs(ref_outputs[i], avx2_outputs[i], 
                           output_len_bytes, i, "short_outputs") != 0) {
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✓ Test 1 PASSED: All 8 outputs match!\n");
    } else {
        printf("✗ Test 1 FAILED: %d mismatches found\n", errors);
    }
    
    return errors;
}

/**
 * Test case 2: Standard outputs (32 bytes = 256 bits)
 * - Each input requires exactly 1 SM3 block (no truncation)
 */
static int test_standard_outputs(void) {
    printf("\n=== Test 2: Standard outputs (256 bits) ===\n");
    
    const unsigned int input_len_bits = 64 * 8;  // 64 bytes
    const unsigned int output_len_bits = 32 * 8; // 32 bytes
    const unsigned int input_len_bytes = input_len_bits / 8;
    const unsigned int output_len_bytes = output_len_bits / 8;
    
    unsigned char inputs[8][input_len_bytes];
    unsigned char ref_outputs[8][output_len_bytes];
    unsigned char avx2_outputs[8][output_len_bytes];
    
    // Generate random inputs
    for (int i = 0; i < 8; i++) {
        randombytes(inputs[i], input_len_bytes);
    }
    
    // Compute reference outputs
    printf("Computing reference outputs...\n");
    for (int i = 0; i < 8; i++) {
        int ret = pseudoXOF(output_len_bits, inputs[i], input_len_bits, ref_outputs[i]);
        if (ret != 0) {
            printf("ERROR: scalar pseudoXOF failed for lane %d\n", i);
            return -1;
        }
    }
    
    // Compute AVX2 outputs
    printf("Computing AVX2 outputs...\n");
    pseudoXOFx8_avx2_aligned(
        avx2_outputs[0], avx2_outputs[1], avx2_outputs[2], avx2_outputs[3],
        avx2_outputs[4], avx2_outputs[5], avx2_outputs[6], avx2_outputs[7],
        output_len_bits,
        inputs[0], inputs[1], inputs[2], inputs[3],
        inputs[4], inputs[5], inputs[6], inputs[7],
        input_len_bits);
    
    // Compare outputs
    printf("Comparing outputs...\n");
    int errors = 0;
    for (int i = 0; i < 8; i++) {
        if (compare_outputs(ref_outputs[i], avx2_outputs[i], 
                           output_len_bytes, i, "standard_outputs") != 0) {
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✓ Test 2 PASSED: All 8 outputs match!\n");
    } else {
        printf("✗ Test 2 FAILED: %d mismatches found\n", errors);
    }
    
    return errors;
}

/**
 * Test case 3: Long outputs (64 bytes = 512 bits)
 * - Each input requires 2 SM3 blocks (ct=1 and ct=2)
 */
static int test_long_outputs(void) {
    printf("\n=== Test 3: Long outputs (512 bits) ===\n");
    
    const unsigned int input_len_bits = 100 * 8;  // 100 bytes
    const unsigned int output_len_bits = 64 * 8;  // 64 bytes
    const unsigned int input_len_bytes = input_len_bits / 8;
    const unsigned int output_len_bytes = output_len_bits / 8;
    
    unsigned char inputs[8][input_len_bytes];
    unsigned char ref_outputs[8][output_len_bytes];
    unsigned char avx2_outputs[8][output_len_bytes];
    
    // Generate random inputs
    for (int i = 0; i < 8; i++) {
        randombytes(inputs[i], input_len_bytes);
    }
    
    // Compute reference outputs
    printf("Computing reference outputs...\n");
    for (int i = 0; i < 8; i++) {
        int ret = pseudoXOF(output_len_bits, inputs[i], input_len_bits, ref_outputs[i]);
        if (ret != 0) {
            printf("ERROR: scalar pseudoXOF failed for lane %d\n", i);
            return -1;
        }
    }
    
    // Compute AVX2 outputs
    printf("Computing AVX2 outputs...\n");
    pseudoXOFx8_avx2_aligned(
        avx2_outputs[0], avx2_outputs[1], avx2_outputs[2], avx2_outputs[3],
        avx2_outputs[4], avx2_outputs[5], avx2_outputs[6], avx2_outputs[7],
        output_len_bits,
        inputs[0], inputs[1], inputs[2], inputs[3],
        inputs[4], inputs[5], inputs[6], inputs[7],
        input_len_bits);
    
    // Compare outputs
    printf("Comparing outputs...\n");
    int errors = 0;
    for (int i = 0; i < 8; i++) {
        if (compare_outputs(ref_outputs[i], avx2_outputs[i], 
                           output_len_bytes, i, "long_outputs") != 0) {
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✓ Test 3 PASSED: All 8 outputs match!\n");
    } else {
        printf("✗ Test 3 FAILED: %d mismatches found\n", errors);
    }
    
    return errors;
}

/**
 * Test case 4: Very long outputs (128 bytes = 1024 bits)
 * - Each input requires 4 SM3 blocks (ct=1,2,3,4)
 */
static int test_very_long_outputs(void) {
    printf("\n=== Test 4: Very long outputs (1024 bits) ===\n");
    
    const unsigned int input_len_bits = 200 * 8;  // 200 bytes
    const unsigned int output_len_bits = 128 * 8; // 128 bytes
    const unsigned int input_len_bytes = input_len_bits / 8;
    const unsigned int output_len_bytes = output_len_bits / 8;
    
    unsigned char inputs[8][input_len_bytes];
    unsigned char ref_outputs[8][output_len_bytes];
    unsigned char avx2_outputs[8][output_len_bytes];
    
    // Generate random inputs
    for (int i = 0; i < 8; i++) {
        randombytes(inputs[i], input_len_bytes);
    }
    
    // Compute reference outputs
    printf("Computing reference outputs...\n");
    for (int i = 0; i < 8; i++) {
        int ret = pseudoXOF(output_len_bits, inputs[i], input_len_bits, ref_outputs[i]);
        if (ret != 0) {
            printf("ERROR: scalar pseudoXOF failed for lane %d\n", i);
            return -1;
        }
    }
    
    // Compute AVX2 outputs
    printf("Computing AVX2 outputs...\n");
    pseudoXOFx8_avx2_aligned(
        avx2_outputs[0], avx2_outputs[1], avx2_outputs[2], avx2_outputs[3],
        avx2_outputs[4], avx2_outputs[5], avx2_outputs[6], avx2_outputs[7],
        output_len_bits,
        inputs[0], inputs[1], inputs[2], inputs[3],
        inputs[4], inputs[5], inputs[6], inputs[7],
        input_len_bits);
    
    // Compare outputs
    printf("Comparing outputs...\n");
    int errors = 0;
    for (int i = 0; i < 8; i++) {
        if (compare_outputs(ref_outputs[i], avx2_outputs[i], 
                           output_len_bytes, i, "very_long_outputs") != 0) {
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✓ Test 4 PASSED: All 8 outputs match!\n");
    } else {
        printf("✗ Test 4 FAILED: %d mismatches found\n", errors);
    }
    
    return errors;
}

/**
 * Test case 5: Real-world scenario - simulating thash
 * - Input: 32-byte address + 16-byte data = 48 bytes
 * - Output: 16 bytes (128 bits)
 */
static int test_thash_simulation(void) {
    printf("\n=== Test 5: thash simulation (48 bytes → 16 bytes) ===\n");
    
    const unsigned int input_len_bits = 48 * 8;  // 48 bytes (address + data)
    const unsigned int output_len_bits = 16 * 8; // 16 bytes (SPX_N)
    const unsigned int input_len_bytes = input_len_bits / 8;
    const unsigned int output_len_bytes = output_len_bits / 8;
    
    unsigned char inputs[8][input_len_bytes];
    unsigned char ref_outputs[8][output_len_bytes];
    unsigned char avx2_outputs[8][output_len_bytes];
    
    // Simulate thash inputs: address (32 bytes) + data (16 bytes)
    for (int i = 0; i < 8; i++) {
        // Address part (simulated)
        randombytes(inputs[i], 32);
        // Data part (simulated)
        randombytes(inputs[i] + 32, 16);
    }
    
    // Compute reference outputs
    printf("Computing reference outputs...\n");
    for (int i = 0; i < 8; i++) {
        int ret = pseudoXOF(output_len_bits, inputs[i], input_len_bits, ref_outputs[i]);
        if (ret != 0) {
            printf("ERROR: scalar pseudoXOF failed for lane %d\n", i);
            return -1;
        }
    }
    
    // Compute AVX2 outputs
    printf("Computing AVX2 outputs...\n");
    pseudoXOFx8_avx2_aligned(
        avx2_outputs[0], avx2_outputs[1], avx2_outputs[2], avx2_outputs[3],
        avx2_outputs[4], avx2_outputs[5], avx2_outputs[6], avx2_outputs[7],
        output_len_bits,
        inputs[0], inputs[1], inputs[2], inputs[3],
        inputs[4], inputs[5], inputs[6], inputs[7],
        input_len_bits);
    
    // Compare outputs
    printf("Comparing outputs...\n");
    int errors = 0;
    for (int i = 0; i < 8; i++) {
        if (compare_outputs(ref_outputs[i], avx2_outputs[i], 
                           output_len_bytes, i, "thash_simulation") != 0) {
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✓ Test 5 PASSED: All 8 outputs match!\n");
    } else {
        printf("✗ Test 5 FAILED: %d mismatches found\n", errors);
    }
    
    return errors;
}

int main(void) {
    printf("===========================================\n");
    printf("pseudoXOF_avx2_aligned Correctness Test\n");
    printf("===========================================\n");
    
    int total_errors = 0;
    
    total_errors += test_short_outputs();
    total_errors += test_standard_outputs();
    total_errors += test_long_outputs();
    total_errors += test_very_long_outputs();
    total_errors += test_thash_simulation();
    
    printf("\n===========================================\n");
    printf("Test Summary\n");
    printf("===========================================\n");
    printf("Total test cases: %d\n", TEST_CASES);
    printf("Passed: %d\n", TEST_CASES - (total_errors > 0 ? 1 : 0));
    printf("Failed: %d\n", total_errors > 0 ? 1 : 0);
    
    if (total_errors == 0) {
        printf("\n✓ ALL TESTS PASSED!\n");
        printf("pseudoXOF_avx2_aligned produces correct outputs.\n");
        return 0;
    } else {
        printf("\n✗ SOME TESTS FAILED!\n");
        printf("Total mismatches: %d\n", total_errors);
        return 1;
    }
}