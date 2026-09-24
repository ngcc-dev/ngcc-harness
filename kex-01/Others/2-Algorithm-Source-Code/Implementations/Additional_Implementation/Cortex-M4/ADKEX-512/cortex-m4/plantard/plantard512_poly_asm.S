#if (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD)   /* DKE config gating (auto-added) */
/*
 * DKE-512 (N=512, Q=7681) Cortex-M4 optimized poly serialization.
 * 13-bit coefficient packing: 8 coefficients → 13 bytes.
 *
 * Adapted from DKE-128 poly_asm.S (12-bit) for DKE-512's 13-bit format.
 * Uses packed LDR for coefficient loading, bit-field extraction for packing.
 *
 * Functions:
 *   poly_tobytes_asm    — serialize 512 coefficients to 832 bytes (13-bit)
 *   poly_frombytes_asm  — deserialize 832 bytes to 512 coefficients
 */
#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

/*
 * poly_tobytes_asm: 13-bit packing, 8 coeffs → 13 bytes.
 *
 * 13-bit layout for coefficients c0..c7 (104 bits = 13 bytes):
 *   byte  0: c0[7:0]
 *   byte  1: c0[12:8] | c1[2:0]<<5
 *   byte  2: c1[10:3]
 *   byte  3: c1[12:11] | c2[5:0]<<2
 *   byte  4: c2[12:6] | c3[0]<<7
 *   byte  5: c3[8:1]
 *   byte  6: c3[12:9] | c4[3:0]<<4
 *   byte  7: c4[11:4]
 *   byte  8: c4[12] | c5[6:0]<<1
 *   byte  9: c5[12:7] | c6[1:0]<<6
 *   byte 10: c6[9:2]
 *   byte 11: c6[12:10] | c7[4:0]<<3
 *   byte 12: c7[12:5]
 *
 * Input:  r0 = output bytes, r1 = input coefficients (int16_t[512])
 * Uses word-at-a-time stores where possible, byte stores at boundaries.
 */
.global poly_tobytes_asm
.type poly_tobytes_asm, %function
.align 2
poly_tobytes_asm:
    push {r4-r11, lr}
    mov r12, #64         @ loop count: 512/8 = 64 iterations

.L_tobytes_loop:
    @ Load 8 coefficients (4 × LDR = 8 × int16, packed pairs)
    ldr r2, [r1], #4     @ r2 = c1:c0
    ldr r3, [r1], #4     @ r3 = c3:c2
    ldr r4, [r1], #4     @ r4 = c5:c4
    ldr r5, [r1], #4     @ r5 = c7:c6

    @ Extract individual 13-bit coefficients (mask 0x1FFF)
    ubfx r6, r2, #0, #13    @ c0
    ubfx r7, r2, #16, #13   @ c1
    ubfx r8, r3, #0, #13    @ c2
    ubfx r9, r3, #16, #13   @ c3
    ubfx r10, r4, #0, #13   @ c4
    ubfx r11, r4, #16, #13  @ c5
    ubfx r14, r5, #0, #13   @ c6 (using lr)

    @ Pack bytes 0-3 (32 bits): c0[7:0] | c0[12:8]|c1[2:0]<<5 | c1[10:3] | c1[12:11]|c2[5:0]<<2
    orr r2, r6, r7, lsl #13       @ c0 | c1<<13 (bits 0-25)
    orr r2, r2, r8, lsl #26       @ | c2<<26 (bits 26-31, partial)
    str r2, [r0], #4

    @ Pack bytes 4-7 (32 bits): c2[12:6]|c3[0]<<7 | c3[8:1] | c3[12:9]|c4[3:0]<<4 | c4[11:4]
    lsr r2, r8, #6                @ c2>>6 (remaining 7 bits)
    orr r2, r2, r9, lsl #7        @ | c3<<7
    orr r2, r2, r10, lsl #20      @ | c4<<20
    str r2, [r0], #4

    @ Pack bytes 8-11 (32 bits): c4[12]|c5[6:0]<<1 | c5[12:7]|c6[1:0]<<6 | c6[9:2] | c6[12:10]|c7[4:0]<<3
    lsr r2, r10, #12              @ c4>>12 (remaining 1 bit)
    orr r2, r2, r11, lsl #1       @ | c5<<1
    orr r2, r2, r14, lsl #14      @ | c6<<14

    ubfx r3, r5, #16, #13         @ c7
    orr r2, r2, r3, lsl #27       @ | c7<<27 (partial)
    str r2, [r0], #4

    @ Pack byte 12: c7[12:5]
    lsr r2, r3, #5
    strb r2, [r0], #1

    subs r12, r12, #1
    bne .L_tobytes_loop

    pop {r4-r11, pc}
.size poly_tobytes_asm, . - poly_tobytes_asm


/*
 * poly_frombytes_asm: 13-bit unpacking, 13 bytes → 8 coefficients.
 *
 * Input:  r0 = output coefficients (int16_t[512]), r1 = input bytes
 */
.global poly_frombytes_asm
.type poly_frombytes_asm, %function
.align 2
poly_frombytes_asm:
    push {r4-r11, lr}
    mov r12, #64         @ loop count: 512/8 = 64

.L_frombytes_loop:
    @ Load 13 bytes: 3 × LDR (12 bytes) + 1 × LDRB (1 byte)
    ldr r2, [r1], #4     @ bytes 0-3
    ldr r3, [r1], #4     @ bytes 4-7
    ldr r4, [r1], #4     @ bytes 8-11
    ldrb r5, [r1], #1    @ byte 12

    @ Extract 8 × 13-bit coefficients
    @ c0 = bits[0:12]
    ubfx r6, r2, #0, #13

    @ c1 = bits[13:25]
    ubfx r7, r2, #13, #13

    @ c2 = bits[26:31] | bits[32:38] (crosses word boundary)
    lsr r8, r2, #26
    orr r8, r8, r3, lsl #6
    ubfx r8, r8, #0, #13

    @ c3 = bits[39:51] → word1[7:19]
    lsr r9, r3, #7
    ubfx r9, r9, #0, #13

    @ c4 = bits[52:64] → word1[20:31] | word2[0:0]
    lsr r10, r3, #20
    orr r10, r10, r4, lsl #12
    ubfx r10, r10, #0, #13

    @ c5 = bits[65:77] → word2[1:13]
    lsr r11, r4, #1
    ubfx r11, r11, #0, #13

    @ c6 = bits[78:90] → word2[14:26]
    lsr r14, r4, #14
    ubfx r14, r14, #0, #13

    @ c7 = bits[91:103] → word2[27:31] | byte12[0:7]
    lsr r4, r4, #27
    orr r4, r4, r5, lsl #5
    ubfx r4, r4, #0, #13

    @ Store 8 coefficients (packed pairs)
    pkhbt r2, r6, r7, lsl #16    @ c1:c0
    str r2, [r0], #4
    pkhbt r2, r8, r9, lsl #16    @ c3:c2
    str r2, [r0], #4
    pkhbt r2, r10, r11, lsl #16  @ c5:c4
    str r2, [r0], #4
    pkhbt r2, r14, r4, lsl #16   @ c7:c6
    str r2, [r0], #4

    subs r12, r12, #1
    bne .L_frombytes_loop

    pop {r4-r11, pc}
.size poly_frombytes_asm, . - poly_frombytes_asm

#endif /* (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD) -- DKE config gating (auto-added) */
