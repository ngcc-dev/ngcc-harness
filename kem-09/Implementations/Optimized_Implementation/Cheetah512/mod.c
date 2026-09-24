/**
 * @file mod.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-03-18
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <stdint.h>

#include "params.h"
#include "mod.h"



/**
 * Computes standard modular operation
 * 
 * This function performs standard modular reduction ensuring the result is always non-negative.
 * For negative inputs, it shifts the result into the range [0, Q-1].
 * 
 * @param x Input 32-bit integer, can be positive, negative, or zero
 * @return Returns the standard modular result of x mod Q, in the range [0, Q-1]
 */
uint16_t standard_mod(int32_t x) {
    // Calculate basic modular operation result
    int32_t r = x % Q;
    
    // Handle negative case: if r is negative, add Q to make it positive
    // (r >> 31) yields -1 (all 1s) when r is negative, 0 when r is positive
    // Adding Q after bitwise AND operation adjusts negative results
    return r + ((r >> 31) & Q);  
}

// Constant-time centered modulo 7681
// Input: any int32_t
// Output: in [-3840, 3840]
int16_t centered_mod(int32_t a) {
    int32_t r = a % Q;
    // Make non-negative: add 7681 if r < 0
    r += (r >> 31) & Q;          // r \in [0, 7680]

    // Center: subtract 7681 if r > 3840 (i.e., r >= 3841)
    r -= Q & (~((r - 3841) >> 31));

    return r;
}

/**
 * @brief Performs modular reduction of a 16-bit unsigned integer by Q
 * 
 * This function computes x mod Q for a given 16-bit unsigned integer x.
 * It simply returns the remainder when x is divided by Q.
 * 
 * @param x The input 16-bit unsigned integer to be reduced modulo Q
 * @return int16_t The result of x mod Q, which will be in the range [0, Q-1]
 */
int16_t mod_uint16(uint16_t x) {
    return x % Q;
}
