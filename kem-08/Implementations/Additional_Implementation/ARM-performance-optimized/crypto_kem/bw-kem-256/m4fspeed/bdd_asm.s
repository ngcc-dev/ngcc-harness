/**
 * @file bdd_opt.S
 * @brief ARM Cortex-M4 optimized Bounded Distance Decoding (BDD).
 *
 * @details
 * This file implements the BDD decoder used by BW-KEM.
 * Main optimization strategy:
 * 1. Keep the BDD_8 hot path fully register-resident in r4-r11.
 * 2. Use M4 DSP instructions for packed int16 lanes.
 * 3. Express repeated hot blocks with semantic macros and internal cores.
 * 4. Keep fixed control flow; candidate selection is mask-based.
 *
 * @target ARM Cortex-M4 (ARMv7E-M)
 * @syntax Unified Assembler Language (UAL)
 */

.syntax unified
.cpu cortex-m4
.thumb
.thumb_func

// ============================================================================
// Macro definitions
// ============================================================================

/**
 * Packed lane convention:
 *   low halfword  = x0
 *   high halfword = x1
 *
 * BDD rounding maps each signed 16-bit lane to:
 *   ((x + 2048) >> 12) << 12
 *
 * Since only the final int16 lane is kept, this is equivalent to
 *   (x + 0x0800) & 0xf000
 * lane-wise.  SADD16 performs the two lane additions in parallel.
 */
.macro LOAD_ROUND_CONST rbias, rmask
    movw    \rbias, #0x0800
    movt    \rbias, #0x0800
    movw    \rmask, #0xf000
    movt    \rmask, #0xf000
.endm

.macro ROUND_FAST reg, rbias, rmask
    sadd16  \reg, \reg, \rbias
    and     \reg, \reg, \rmask
.endm

.macro ROUND_PAIR reg0, reg1, rbias, rmask
    LOAD_ROUND_CONST \rbias, \rmask
    ROUND_FAST \reg0, \rbias, \rmask
    ROUND_FAST \reg1, \rbias, \rmask
.endm

.macro PHI_PAIR reg
    // [a,b] -> [a-b, a+b]
    sasx    \reg, \reg, \reg
.endm

.macro PHI_INV_PAIR reg
    // [a,b] -> [(a+b)/2, (b-a)/2]
    shsax   \reg, \reg, \reg
.endm

.macro DIST_ACC4 acc, a0, a1, b0, b1, tmp0, tmp1
    ssub16  \tmp0, \a0, \b0
    smlad   \acc, \tmp0, \tmp0, \acc
    ssub16  \tmp1, \a1, \b1
    smlad   \acc, \tmp1, \tmp1, \acc
.endm

.macro CT_SELECT4 dst0, dst1, src0, src1, mask, tmp
    eor     \tmp, \dst0, \src0
    and     \tmp, \tmp, \mask
    eor     \dst0, \dst0, \tmp
    eor     \tmp, \dst1, \src1
    and     \tmp, \tmp, \mask
    eor     \dst1, \dst1, \tmp
.endm

// ============================================================================
// Function: BDD_8_asm
// ============================================================================
/**
 * @brief Decode an 8-dimensional vector with BDD.
 *
 * @param r0 (w_ptr) output buffer pointer
 * @param r1 (t_ptr) input vector pointer
 *
 * @note The body is leaf-style and uses no local stack.
 * @note Register contract:
 *       R4-R5  : y1 (4 elements)
 *       R6-R7  : y2 (4 elements)
 *       R8-R9  : w1 (4 elements)
 *       R10-R11: w2 (4 elements)
 *       LR     : temporary t_ptr base
 *       R3     : output w_ptr base
 *       R0-R2, R12: arithmetic temporaries
 */
.macro BDD8_BODY
    // Contract: r0 = w_ptr, r1 = t_ptr.
    // Clobbers r0-r12 and lr; writes 8 int16 output lanes.
    mov     lr, r1          // LR = t_ptr
    mov     r3, r0          // R3 = w_ptr

    // ------------------------------------------------------------------------
    // Step 1: y1 = BDD_4(t[0:3]) -> R4, R5
    // ------------------------------------------------------------------------
    ldr     r4, [lr]        // Load t[0], t[1]
    ldr     r5, [lr, #4]    // Load t[2], t[3]
    
    ROUND_PAIR r4, r5, r0, r12
    // R4,R5 now hold y1.

    // ------------------------------------------------------------------------
    // Step 2: y2 = BDD_4(t[4:7]) -> R6, R7
    // ------------------------------------------------------------------------
    ldr     r6, [lr, #8]    // Load t[4], t[5]
    ldr     r7, [lr, #12]   // Load t[6], t[7]
    
    ROUND_PAIR r6, r7, r0, r12
    // R6,R7 now hold y2.

    // ------------------------------------------------------------------------
    // Step 3: w1 = y1 + phi(BDD(phi_inv(t_high - y1))) -> R8, R9
    // ------------------------------------------------------------------------
    // Load t_high (t[4:7]) and form the residual.
    ldr     r8, [lr, #8]
    ldr     r9, [lr, #12]
    ssub16  r8, r8, r4      // R8 = t[4,5] - y1[0,1]
    ssub16  r9, r9, r5      // R9 = t[6,7] - y1[2,3]
    
    PHI_INV_PAIR r8
    PHI_INV_PAIR r9
    
    ROUND_PAIR r8, r9, r0, r12
    
    // Apply phi(z1) and add y1 to produce w1.
    PHI_PAIR r8
    sadd16  r8, r8, r4      // + y1[0,1]
    PHI_PAIR r9
    sadd16  r9, r9, r5      // + y1[2,3]
    // R8,R9 now hold w1.

    // ------------------------------------------------------------------------
    // Step 4: w2 = y2 + phi(BDD(phi_inv(t_low - y2))) -> R10, R11
    // ------------------------------------------------------------------------
    // Load t_low (t[0:3]) and form the residual.
    ldr     r10, [lr]
    ldr     r11, [lr, #4]
    ssub16  r10, r10, r6    // R10 = t[0,1] - y2[0,1]
    ssub16  r11, r11, r7    // R11 = t[2,3] - y2[2,3]

    PHI_INV_PAIR r10
    PHI_INV_PAIR r11

    ROUND_PAIR r10, r11, r0, r12

    // Apply phi(z2) and add y2 to produce w2.
    PHI_PAIR r10
    sadd16  r10, r10, r6    // + y2[0,1]
    PHI_PAIR r11
    sadd16  r11, r11, r7    // + y2[2,3]
    // R10,R11 now hold w2.

    // ------------------------------------------------------------------------
    // Step 5: distance accumulation
    // ------------------------------------------------------------------------
    // Targets:
    // acc1 (R2)  = ||y1 - t_low||^2 + ||w1 - t_high||^2
    // acc2 (R12) = ||w2 - t_low||^2 + ||y2 - t_high||^2
    
    mov     r2, #0          // acc1 = 0
    mov     r12, #0         // acc2 = 0

    // Load t_low and compare against y1 and w2.
    ldr     r0, [lr]        // t_low[0,1]
    ldr     r1, [lr, #4]    // t_low[2,3]
    
    // acc1 += (y1 - t_low)^2
    DIST_ACC4 r2, r4, r5, r0, r1, r0, r1
    
    // Reload t_low for the w2 distance.
    ldr     r0, [lr]
    ldr     r1, [lr, #4]
    
    // acc2 += (w2 - t_low)^2
    DIST_ACC4 r12, r10, r11, r0, r1, r0, r1
    
    // Load t_high and compare against w1 and y2.
    ldr     r0, [lr, #8]    // t_high[0,1]
    ldr     r1, [lr, #12]   // t_high[2,3]
    
    // acc1 += (w1 - t_high)^2
    DIST_ACC4 r2, r8, r9, r0, r1, r0, r1
    
    // Reload t_high for the y2 distance.
    ldr     r0, [lr, #8]
    ldr     r1, [lr, #12]
    
    // acc2 += (y2 - t_high)^2
    DIST_ACC4 r12, r6, r7, r0, r1, r0, r1

    // ------------------------------------------------------------------------
    // Step 6: constant-time candidate selection
    // ------------------------------------------------------------------------
    // If acc2 < acc1 select candidate 2 (w2, y2), else candidate 1 (y1, w1).
    
    subs    r0, r12, r2     // r0 = acc2 - acc1
    asrs    r0, r0, #31     // Mask: 0xFFFFFFFF if acc2 < acc1, else 0
    
    // Select the low half: y1 vs w2.
    // y1 = y1 ^ (mask & (y1 ^ w2)), then store.
    
    CT_SELECT4 r4, r5, r10, r11, r0, r1
    str     r4, [r3]        // Write w[0,1]
    str     r5, [r3, #4]    // Write w[2,3]
    
    // Select the high half: w1 vs y2.
    // w1 = w1 ^ (mask & (w1 ^ y2)).
    
    CT_SELECT4 r8, r9, r6, r7, r0, r1
    str     r8, [r3, #8]    // Write w[4,5]
    str     r9, [r3, #12]   // Write w[6,7]
    
 .endm

.global BDD_8_asm
.type BDD_8_asm, %function
.align 2
BDD_8_asm:
    push    {r4-r11, lr}
    BDD8_BODY
    pop     {r4-r11, pc}
.size BDD_8_asm, .-BDD_8_asm


// ============================================================================
// Function: BDD_16_asm
// ============================================================================
/**
 * @brief Decode a 16-dimensional vector with BDD.
 *
 * @param r0 (w_ptr) output buffer pointer (32 bytes)
 * @param r1 (t_ptr) input vector pointer (32 bytes)
 *
 * @note Stack layout (76 bytes local + 36 bytes saved by the public wrapper):
 *       SP+0  : y1 [0:7]  (16 bytes)
 *       SP+16 : y2 [0:7]  (16 bytes)
 *       SP+32 : z1 [0:7] / w1 [0:7] (16 bytes)
 *       SP+48 : z2 [0:7] / w2 [0:7] (16 bytes)
 *       SP+64 : w_ptr
 *       SP+68 : t_ptr
 */
.type bdd16_core, %function
.align 2
bdd16_core:
    sub     sp, #76
    str     r0, [sp, #64]
    str     r1, [sp, #68]
    str     lr, [sp, #72]
    
    // ------------------------------------------------------------------------
    // 1. Recursive step: y1 = BDD_8(t_low)
    // ------------------------------------------------------------------------
    mov     r0, sp          // Write y1 to SP+0
    ldr     r1, [sp, #68]   // Input t_low
    BDD8_BODY
    
    // ------------------------------------------------------------------------
    // 2. Recursive step: y2 = BDD_8(t_high)
    // ------------------------------------------------------------------------
    add     r0, sp, #16     // Write y2 to SP+16
    ldr     r1, [sp, #68]
    add     r1, r1, #16     // Input t_high
    BDD8_BODY
    
    // ------------------------------------------------------------------------
    // 3. z1 = BDD_8(mul_phi_inv(t_high - y1))
    // ------------------------------------------------------------------------
    // Load inputs and compute the residual.
    ldr     r5, [sp, #68]
    add     r1, r5, #16
    ldmia   r1, {r6-r9}     // Load t_high -> R6-R9
    mov     r1, sp
    ldmia   r1, {r0-r3}     // Load y1 -> R0-R3
    
    ssub16  r6, r6, r0      // vec0 -= y1_0
    ssub16  r7, r7, r1      // vec1 -= y1_1
    ssub16  r8, r8, r2
    ssub16  r9, r9, r3
    
    PHI_INV_PAIR r6
    PHI_INV_PAIR r7
    PHI_INV_PAIR r8
    PHI_INV_PAIR r9
    
    // Store into the z1 work buffer and recurse.
    add     r0, sp, #32
    stmia   r0, {r6-r9}     // Store at SP+32
    
    add     r0, sp, #32
    add     r1, sp, #32
    BDD8_BODY               // Result is written back to SP+32

    // ------------------------------------------------------------------------
    // 4. z2 = BDD_8(mul_phi_inv(t_low - y2))
    // ------------------------------------------------------------------------
    // Load inputs and compute the residual.
    ldr     r5, [sp, #68]
    mov     r1, r5
    ldmia   r1, {r6-r9}     // Load t_low
    add     r1, sp, #16
    ldmia   r1, {r0-r3}     // Load y2
    
    ssub16  r6, r6, r0
    ssub16  r7, r7, r1
    ssub16  r8, r8, r2
    ssub16  r9, r9, r3
    
    PHI_INV_PAIR r6
    PHI_INV_PAIR r7
    PHI_INV_PAIR r8
    PHI_INV_PAIR r9
    
    // Store into the z2 work buffer and recurse.
    add     r0, sp, #48
    stmia   r0, {r6-r9}     // Store at SP+48
    
    add     r0, sp, #48
    add     r1, sp, #48
    BDD8_BODY               // Result is written back to SP+48

    // ------------------------------------------------------------------------
    // 5. Compute w1 = y1 + phi(z1) and accumulate dis1.
    // ------------------------------------------------------------------------
    ldr     r5, [sp, #68]
    mov     r10, #0         // acc1 = 0
    
    // 5.1 dis1 Part A: ||y1 - t_low||^2
    mov     r0, sp
    ldmia   r0, {r6-r9}     // Load y1
    mov     r0, r5
    ldmia   r0, {r0-r3}     // Load t_low
    
    ssub16  r0, r6, r0
    smlad   r10, r0, r0, r10 // acc1 += (y1-t)^2
    ssub16  r1, r7, r1
    smlad   r10, r1, r1, r10
    ssub16  r2, r8, r2
    smlad   r10, r2, r2, r10
    ssub16  r3, r9, r3
    smlad   r10, r3, r3, r10
    
    // Compute w1.
    add     r12, sp, #32
    ldmia   r12, {r0-r3}    // Load z1
    
    PHI_PAIR r0
    sadd16  r6, r6, r0      // w1 = y1 + phi(z1), stored in r6
    PHI_PAIR r1
    sadd16  r7, r7, r1
    PHI_PAIR r2
    sadd16  r8, r8, r2
    PHI_PAIR r3
    sadd16  r9, r9, r3
    
    // Save w1 for the final selection.
    add     r0, sp, #32
    stmia   r0, {r6-r9}     // w1 -> SP+32
    
    // 5.3 dis1 Part B: ||w1 - t_high||^2
    add     r0, r5, #16
    ldmia   r0, {r0-r3}     // Load t_high
    
    ssub16  r0, r6, r0
    smlad   r10, r0, r0, r10
    ssub16  r1, r7, r1
    smlad   r10, r1, r1, r10
    ssub16  r2, r8, r2
    smlad   r10, r2, r2, r10
    ssub16  r3, r9, r3
    smlad   r10, r3, r3, r10
    
    // ------------------------------------------------------------------------
    // 6. Compute w2 = y2 + phi(z2) and accumulate dis2.
    // ------------------------------------------------------------------------
    mov     r11, #0         // acc2 = 0
    
    // 6.1 dis2 Part A: ||y2 - t_high||^2
    add     r0, sp, #16
    ldmia   r0, {r6-r9}     // Load y2
    add     r0, r5, #16
    ldmia   r0, {r0-r3}     // Load t_high
    
    ssub16  r0, r6, r0
    smlad   r11, r0, r0, r11
    ssub16  r1, r7, r1
    smlad   r11, r1, r1, r11
    ssub16  r2, r8, r2
    smlad   r11, r2, r2, r11
    ssub16  r3, r9, r3
    smlad   r11, r3, r3, r11
    
    // Compute w2.
    add     r12, sp, #48
    ldmia   r12, {r0-r3}    // Load z2
    
    PHI_PAIR r0
    sadd16  r6, r6, r0      // w2 = y2 + phi(z2)
    PHI_PAIR r1
    sadd16  r7, r7, r1
    PHI_PAIR r2
    sadd16  r8, r8, r2
    PHI_PAIR r3
    sadd16  r9, r9, r3
    
    // Save w2.
    add     r0, sp, #48
    stmia   r0, {r6-r9}     // w2 -> SP+48
    
    // 6.3 dis2 Part B: ||w2 - t_low||^2
    mov     r0, r5
    ldmia   r0, {r0-r3}     // Load t_low
    
    ssub16  r0, r6, r0
    smlad   r11, r0, r0, r11
    ssub16  r1, r7, r1
    smlad   r11, r1, r1, r11
    ssub16  r2, r8, r2
    smlad   r11, r2, r2, r11
    ssub16  r3, r9, r3
    smlad   r11, r3, r3, r11
    
    // ------------------------------------------------------------------------
    // 7. Constant-time selection and output.
    // ------------------------------------------------------------------------
    ldr     r4, [sp, #64]
    subs    r12, r11, r10   // dis2 - dis1
    asrs    r12, r12, #31   // Mask = 0xFFFFFFFF if dis2 < dis1 else 0
    
    // 7.1 Select Low 8: mask ? w2 : y1.
    mov     r0, sp
    ldmia   r0, {r6-r9}     // y1
    add     r1, sp, #48
    ldmia   r1, {r0-r3}     // w2
    
    // x = a ^ (mask & (a ^ b))
    eor     r0, r0, r6
    and     r0, r0, r12
    eor     r6, r6, r0
    
    eor     r1, r1, r7
    and     r1, r1, r12
    eor     r7, r7, r1
    
    eor     r2, r2, r8
    and     r2, r2, r12
    eor     r8, r8, r2
    
    eor     r3, r3, r9
    and     r3, r3, r12
    eor     r9, r9, r3
    
    stmia   r4!, {r6-r9}    // Store output and advance pointer

    // 7.2 Select High 8: mask ? y2 : w1.
    add     r0, sp, #32
    ldmia   r0, {r6-r9}     // w1
    add     r1, sp, #16
    ldmia   r1, {r0-r3}     // y2
    
    eor     r0, r0, r6
    and     r0, r0, r12
    eor     r6, r6, r0
    
    eor     r1, r1, r7
    and     r1, r1, r12
    eor     r7, r7, r1
    
    eor     r2, r2, r8
    and     r2, r2, r12
    eor     r8, r8, r2
    
    eor     r3, r3, r9
    and     r3, r3, r12
    eor     r9, r9, r3
    
    stmia   r4, {r6-r9}     // Store high output half
    
    ldr     lr, [sp, #72]
    add     sp, #76
    bx      lr
.size bdd16_core, .-bdd16_core

.global BDD_16_asm
.type BDD_16_asm, %function
.align 2
BDD_16_asm:
    push    {r4-r11, lr}
    bl      bdd16_core
    pop     {r4-r11, pc}
.size BDD_16_asm, .-BDD_16_asm


// ============================================================================
// Function: BDD (BDD_32)
// ============================================================================
/**
 * @brief Decode a 32-dimensional vector with BDD.
 *
 * @param r0 (w_ptr) output buffer pointer (64 bytes)
 * @param r1 (t_ptr) input vector pointer (64 bytes)
 *
 * @details
 * Same recursive structure as BDD_16, with doubled data size. Repeated
 * operations are manually split into two 8-element blocks.
 *
 * @note Stack layout (136 bytes local + 36 bytes saved):
 *       SP+0   : y1 [0:15]  (32 bytes)
 *       SP+32  : y2 [0:15]  (32 bytes)
 *       SP+64  : z1 [0:15] / w1 [0:15] (32 bytes)
 *       SP+96  : z2 [0:15] / w2 [0:15] (32 bytes)
 *       SP+128 : w_ptr
 *       SP+132 : t_ptr
 */
.global BDD_asm
.type BDD_asm, %function
.align 2
BDD_asm:
    push    {r4-r11, lr}
    sub     sp, #136
    str     r0, [sp, #128]
    str     r1, [sp, #132]
    
    // ------------------------------------------------------------------------
    // 1. y1 = BDD_16(t_low)
    // ------------------------------------------------------------------------
    mov     r0, sp
    ldr     r1, [sp, #132]
    bl      bdd16_core
    
    // ------------------------------------------------------------------------
    // 2. y2 = BDD_16(t_high)
    // ------------------------------------------------------------------------
    add     r0, sp, #32
    ldr     r1, [sp, #132]
    add     r1, r1, #32
    bl      bdd16_core
    
    // ------------------------------------------------------------------------
    // 3. z1 = BDD_16(mul_phi_inv(t_high - y1))
    // ------------------------------------------------------------------------
    // --- Block 0 (Offset 0-15 bytes) ---
    ldr     r5, [sp, #132]
    add     r1, r5, #32
    ldmia   r1, {r6-r9}     // t_high[0-7]
    mov     r1, sp
    ldmia   r1, {r0-r3}     // y1[0-7]
    
    ssub16  r6, r6, r0
    ssub16  r7, r7, r1
    ssub16  r8, r8, r2
    ssub16  r9, r9, r3
    
    PHI_INV_PAIR r6
    PHI_INV_PAIR r7
    PHI_INV_PAIR r8
    PHI_INV_PAIR r9
    
    add     r0, sp, #64
    stmia   r0, {r6-r9}     // z1 part 1 input
    
    // --- Block 1 (Offset 16-31 bytes) ---
    add     r1, r5, #48
    ldmia   r1, {r6-r9}     // t_high[8-15]
    add     r1, sp, #16
    ldmia   r1, {r0-r3}     // y1[8-15]
    
    ssub16  r6, r6, r0
    ssub16  r7, r7, r1
    ssub16  r8, r8, r2
    ssub16  r9, r9, r3
    
    PHI_INV_PAIR r6
    PHI_INV_PAIR r7
    PHI_INV_PAIR r8
    PHI_INV_PAIR r9
    
    add     r0, sp, #80
    stmia   r0, {r6-r9}     // z1 part 2 input
    
    // Call BDD_16
    add     r0, sp, #64
    add     r1, sp, #64
    bl      bdd16_core      // z1 result -> SP+64
    
    // ------------------------------------------------------------------------
    // 4. z2 = BDD_16(mul_phi_inv(t_low - y2))
    // ------------------------------------------------------------------------
    // --- Block 0 ---
    ldr     r5, [sp, #132]
    mov     r1, r5
    ldmia   r1, {r6-r9}     // t_low[0-7]
    add     r1, sp, #32
    ldmia   r1, {r0-r3}     // y2[0-7]
    
    ssub16  r6, r6, r0
    ssub16  r7, r7, r1
    ssub16  r8, r8, r2
    ssub16  r9, r9, r3
    
    PHI_INV_PAIR r6
    PHI_INV_PAIR r7
    PHI_INV_PAIR r8
    PHI_INV_PAIR r9
    
    add     r0, sp, #96
    stmia   r0, {r6-r9}
    
    // --- Block 1 ---
    add     r1, r5, #16
    ldmia   r1, {r6-r9}     // t_low[8-15]
    add     r1, sp, #48
    ldmia   r1, {r0-r3}     // y2[8-15]
    
    ssub16  r6, r6, r0
    ssub16  r7, r7, r1
    ssub16  r8, r8, r2
    ssub16  r9, r9, r3
    
    PHI_INV_PAIR r6
    PHI_INV_PAIR r7
    PHI_INV_PAIR r8
    PHI_INV_PAIR r9
    
    add     r0, sp, #112
    stmia   r0, {r6-r9}
    
    // Call BDD_16
    add     r0, sp, #96
    add     r1, sp, #96
    bl      bdd16_core      // z2 result -> SP+96
    
    // ------------------------------------------------------------------------
    // 5. Compute w1 and dis1.
    // ------------------------------------------------------------------------
    ldr     r5, [sp, #132]
    mov     r10, #0         // acc1 = 0
    
    // --- Block 0: dis1 += ||y1 - t_low||^2 ---
    mov     r0, sp
    ldmia   r0, {r6-r9}     // y1
    mov     r0, r5
    ldmia   r0, {r0-r3}     // t_low
    
    ssub16  r0, r6, r0
    smlad   r10, r0, r0, r10
    ssub16  r1, r7, r1
    smlad   r10, r1, r1, r10
    ssub16  r2, r8, r2
    smlad   r10, r2, r2, r10
    ssub16  r3, r9, r3
    smlad   r10, r3, r3, r10
    
    // --- Block 0: Calc w1 ---
    add     r12, sp, #64
    ldmia   r12, {r0-r3}    // z1
    
    PHI_PAIR r0
    sadd16  r6, r6, r0      // w1[0-7]
    PHI_PAIR r1
    sadd16  r7, r7, r1
    PHI_PAIR r2
    sadd16  r8, r8, r2
    PHI_PAIR r3
    sadd16  r9, r9, r3
    
    add     r0, sp, #64
    stmia   r0, {r6-r9}     // Store w1 part 1
    
    // --- Block 0: dis1 += ||w1 - t_high||^2 ---
    add     r0, r5, #32
    ldmia   r0, {r0-r3}     // t_high
    
    ssub16  r0, r6, r0
    smlad   r10, r0, r0, r10
    ssub16  r1, r7, r1
    smlad   r10, r1, r1, r10
    ssub16  r2, r8, r2
    smlad   r10, r2, r2, r10
    ssub16  r3, r9, r3
    smlad   r10, r3, r3, r10
    
    // --- Block 1: Repeat for offset 16 ---
    add     r0, sp, #16
    ldmia   r0, {r6-r9}     // y1
    add     r0, r5, #16
    ldmia   r0, {r0-r3}     // t_low
    
    ssub16  r0, r6, r0
    smlad   r10, r0, r0, r10
    ssub16  r1, r7, r1
    smlad   r10, r1, r1, r10
    ssub16  r2, r8, r2
    smlad   r10, r2, r2, r10
    ssub16  r3, r9, r3
    smlad   r10, r3, r3, r10
    
    add     r12, sp, #80
    ldmia   r12, {r0-r3}    // z1
    
    PHI_PAIR r0
    sadd16  r6, r6, r0
    PHI_PAIR r1
    sadd16  r7, r7, r1
    PHI_PAIR r2
    sadd16  r8, r8, r2
    PHI_PAIR r3
    sadd16  r9, r9, r3
    
    add     r0, sp, #80
    stmia   r0, {r6-r9}
    
    add     r0, r5, #48
    ldmia   r0, {r0-r3}     // t_high
    
    ssub16  r0, r6, r0
    smlad   r10, r0, r0, r10
    ssub16  r1, r7, r1
    smlad   r10, r1, r1, r10
    ssub16  r2, r8, r2
    smlad   r10, r2, r2, r10
    ssub16  r3, r9, r3
    smlad   r10, r3, r3, r10
    
    // ------------------------------------------------------------------------
    // 6. Compute w2 and dis2.
    // ------------------------------------------------------------------------
    mov     r11, #0         // acc2 = 0
    
    // --- Block 0: dis2 += ||y2 - t_high||^2 ---
    add     r0, sp, #32
    ldmia   r0, {r6-r9}     // y2
    add     r0, r5, #32
    ldmia   r0, {r0-r3}     // t_high
    
    ssub16  r0, r6, r0
    smlad   r11, r0, r0, r11
    ssub16  r1, r7, r1
    smlad   r11, r1, r1, r11
    ssub16  r2, r8, r2
    smlad   r11, r2, r2, r11
    ssub16  r3, r9, r3
    smlad   r11, r3, r3, r11
    
    // --- Block 0: Calc w2 ---
    add     r12, sp, #96
    ldmia   r12, {r0-r3}    // z2
    
    PHI_PAIR r0
    sadd16  r6, r6, r0
    PHI_PAIR r1
    sadd16  r7, r7, r1
    PHI_PAIR r2
    sadd16  r8, r8, r2
    PHI_PAIR r3
    sadd16  r9, r9, r3
    
    add     r0, sp, #96
    stmia   r0, {r6-r9}
    
    // --- Block 0: dis2 += ||w2 - t_low||^2 ---
    mov     r0, r5
    ldmia   r0, {r0-r3}     // t_low
    
    ssub16  r0, r6, r0
    smlad   r11, r0, r0, r11
    ssub16  r1, r7, r1
    smlad   r11, r1, r1, r11
    ssub16  r2, r8, r2
    smlad   r11, r2, r2, r11
    ssub16  r3, r9, r3
    smlad   r11, r3, r3, r11
    
    // --- Block 1: Repeat ---
    add     r0, sp, #48
    ldmia   r0, {r6-r9}     // y2
    add     r0, r5, #48
    ldmia   r0, {r0-r3}     // t_high
    
    ssub16  r0, r6, r0
    smlad   r11, r0, r0, r11
    ssub16  r1, r7, r1
    smlad   r11, r1, r1, r11
    ssub16  r2, r8, r2
    smlad   r11, r2, r2, r11
    ssub16  r3, r9, r3
    smlad   r11, r3, r3, r11
    
    add     r12, sp, #112
    ldmia   r12, {r0-r3}    // z2
    
    PHI_PAIR r0
    sadd16  r6, r6, r0
    PHI_PAIR r1
    sadd16  r7, r7, r1
    PHI_PAIR r2
    sadd16  r8, r8, r2
    PHI_PAIR r3
    sadd16  r9, r9, r3
    
    add     r0, sp, #112
    stmia   r0, {r6-r9}
    
    add     r0, r5, #16
    ldmia   r0, {r0-r3}     // t_low
    
    ssub16  r0, r6, r0
    smlad   r11, r0, r0, r11
    ssub16  r1, r7, r1
    smlad   r11, r1, r1, r11
    ssub16  r2, r8, r2
    smlad   r11, r2, r2, r11
    ssub16  r3, r9, r3
    smlad   r11, r3, r3, r11
    
    // ------------------------------------------------------------------------
    // 7. Constant-time selection.
    // ------------------------------------------------------------------------
    ldr     r4, [sp, #128]
    subs    r12, r11, r10
    asrs    r12, r12, #31   // Mask
    
    // --- Block 0 (Low 16 elements): mask ? w2 : y1
    mov     r0, sp
    ldmia   r0, {r6-r9}     // y1
    add     r1, sp, #96
    ldmia   r1, {r0-r3}     // w2
    
    eor     r0, r0, r6
    and     r0, r0, r12
    eor     r6, r6, r0
    
    eor     r1, r1, r7
    and     r1, r1, r12
    eor     r7, r7, r1
    
    eor     r2, r2, r8
    and     r2, r2, r12
    eor     r8, r8, r2
    
    eor     r3, r3, r9
    and     r3, r3, r12
    eor     r9, r9, r3
    
    stmia   r4!, {r6-r9}
    
    // --- Block 1 (Next 16 elements)
    add     r0, sp, #16
    ldmia   r0, {r6-r9}     // y1
    add     r1, sp, #112
    ldmia   r1, {r0-r3}     // w2
    
    eor     r0, r0, r6
    and     r0, r0, r12
    eor     r6, r6, r0
    
    eor     r1, r1, r7
    and     r1, r1, r12
    eor     r7, r7, r1
    
    eor     r2, r2, r8
    and     r2, r2, r12
    eor     r8, r8, r2
    
    eor     r3, r3, r9
    and     r3, r3, r12
    eor     r9, r9, r3
    
    stmia   r4!, {r6-r9}
    
    // --- Block 2 (Low 16 of High Half): mask ? y2 : w1
    add     r0, sp, #64
    ldmia   r0, {r6-r9}     // w1
    add     r1, sp, #32
    ldmia   r1, {r0-r3}     // y2
    
    eor     r0, r0, r6
    and     r0, r0, r12
    eor     r6, r6, r0
    
    eor     r1, r1, r7
    and     r1, r1, r12
    eor     r7, r7, r1
    
    eor     r2, r2, r8
    and     r2, r2, r12
    eor     r8, r8, r2
    
    eor     r3, r3, r9
    and     r3, r3, r12
    eor     r9, r9, r3
    
    stmia   r4!, {r6-r9}
    
    // --- Block 3 (High 16 of High Half)
    add     r0, sp, #80
    ldmia   r0, {r6-r9}     // w1
    add     r1, sp, #48
    ldmia   r1, {r0-r3}     // y2
    
    eor     r0, r0, r6
    and     r0, r0, r12
    eor     r6, r6, r0
    
    eor     r1, r1, r7
    and     r1, r1, r12
    eor     r7, r7, r1
    
    eor     r2, r2, r8
    and     r2, r2, r12
    eor     r8, r8, r2
    
    eor     r3, r3, r9
    and     r3, r3, r12
    eor     r9, r9, r3
    
    stmia   r4!, {r6-r9}
    
    add     sp, #136
    pop     {r4-r11, pc}
.size BDD_asm, .-BDD_asm
