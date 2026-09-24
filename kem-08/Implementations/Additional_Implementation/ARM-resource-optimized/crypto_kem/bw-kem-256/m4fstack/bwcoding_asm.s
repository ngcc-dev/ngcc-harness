/*
 * Cortex-M4 BW32 encode/decode C ABI entry points.
 *
 * This file provides the public BWcoding API for IMPL=m4.  The routines are
 * fixed-flow: no secret-dependent branches or table indices.
 */

.syntax unified
.cpu cortex-m4
.thumb

// Load four 2-bit labels from int16 lanes into one word.
// Byte lane layout: dst = v0 | (v1 << 8) | (v2 << 16) | (v3 << 24).
.macro LOAD_LABEL_GROUP dst, ptr, off, tmp1, tmp2
    ldrh    \dst, [\ptr, #\off]
    ubfx    \dst, \dst, #12, #2
    ldrh    \tmp1, [\ptr, #(\off + 2)]
    ubfx    \tmp1, \tmp1, #12, #2
    orr     \dst, \dst, \tmp1, lsl #8
    ldrh    \tmp1, [\ptr, #(\off + 4)]
    ubfx    \tmp1, \tmp1, #12, #2
    orr     \dst, \dst, \tmp1, lsl #16
    ldrh    \tmp1, [\ptr, #(\off + 6)]
    ubfx    \tmp1, \tmp1, #12, #2
    orr     \dst, \dst, \tmp1, lsl #24
.endm

// Extract one byte-packed label from a LOAD_LABEL_GROUP register.
.macro GETV dst, reg, lane
.if \lane == 0
    uxtb    \dst, \reg
.elseif \lane == 1
    ubfx    \dst, \reg, #8, #8
.elseif \lane == 2
    ubfx    \dst, \reg, #16, #8
.elseif \lane == 3
    ubfx    \dst, \reg, #24, #8
.else
    .error "GETV lane must be 0..3"
.endif
.endm

// Add or subtract one byte-packed label to a scalar expression.
.macro ADDV acc, reg, lane
    GETV    r2, \reg, \lane
    add     \acc, \acc, r2
.endm

.macro SUBV acc, reg, lane
    GETV    r2, \reg, \lane
    sub     \acc, \acc, r2
.endm

// Emit selected bits from delabel linear expressions into the output word.
.macro OR_EBIT out, expr, srcbit, dstbit, tmp
    ubfx    \tmp, \expr, #\srcbit, #1
    orr     \out, \out, \tmp, lsl #\dstbit
.endm

.macro SAVE_EBIT dst, expr, srcbit
    ubfx    \dst, \expr, #\srcbit, #1
.endm

.macro XOR_SAVED_EBIT out, saved, expr, srcbit, dstbit, tmp
    ubfx    \tmp, \expr, #\srcbit, #1
    eor     \tmp, \tmp, \saved
    orr     \out, \out, \tmp, lsl #\dstbit
.endm

// Load all 32 labels into r4-r11, four byte lanes per register.
.macro BW32_LOAD_LABELS
    LOAD_LABEL_GROUP r4,  r0,  0, r1, r2
    LOAD_LABEL_GROUP r5,  r0,  8, r1, r2
    LOAD_LABEL_GROUP r6,  r0, 16, r1, r2
    LOAD_LABEL_GROUP r7,  r0, 24, r1, r2
    LOAD_LABEL_GROUP r8,  r0, 32, r1, r2
    LOAD_LABEL_GROUP r9,  r0, 40, r1, r2
    LOAD_LABEL_GROUP r10, r0, 48, r1, r2
    LOAD_LABEL_GROUP r11, r0, 56, r1, r2
.endm

// Direct delabel core.
// Input:  r0 = int16_t w[32]
// Output: r0 = uint32_t message
// Clobbers r1-r12. The caller preserves r4-r11 for the public ABI.
.macro BW32_DELABEL_BODY
    BW32_LOAD_LABELS

    movs    r0, #0

    uxtb    r1, r4
    orr     r0, r0, r1, lsl #30
    ubfx    r1, r4, #8, #2
    orr     r0, r0, r1, lsl #28

    GETV    r1, r4, 2        @ mvec[2]
    ADDV    r1, r4, 3
    SUBV    r1, r4, 0
    SUBV    r1, r4, 1
    OR_EBIT r0, r1, 1, 26, r2
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[3]
    ADDV    r1, r4, 3
    SUBV    r1, r4, 1
    SUBV    r1, r4, 2
    OR_EBIT r0, r1, 1, 25, r2
    XOR_SAVED_EBIT r0, r3, r1, 2, 27, r2
    GETV    r1, r5, 0        @ mvec[4]
    ADDV    r1, r5, 1
    SUBV    r1, r4, 0
    SUBV    r1, r4, 1
    OR_EBIT r0, r1, 1, 23, r2
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[5]
    ADDV    r1, r5, 1
    SUBV    r1, r4, 1
    SUBV    r1, r5, 0
    OR_EBIT r0, r1, 1, 22, r2
    XOR_SAVED_EBIT r0, r3, r1, 2, 24, r2
    GETV    r1, r4, 1        @ mvec[6]
    ADDV    r1, r5, 3
    SUBV    r1, r4, 3
    SUBV    r1, r5, 1
    OR_EBIT r0, r1, 1, 21, r2
    GETV    r1, r4, 2        @ mvec[7]
    ADDV    r1, r5, 0
    SUBV    r1, r4, 0
    SUBV    r1, r5, 2
    OR_EBIT r0, r1, 1, 20, r2
    GETV    r1, r6, 0        @ mvec[8]
    ADDV    r1, r6, 1
    SUBV    r1, r4, 0
    SUBV    r1, r4, 1
    OR_EBIT r0, r1, 1, 18, r2
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[9]
    ADDV    r1, r6, 1
    SUBV    r1, r4, 1
    SUBV    r1, r6, 0
    OR_EBIT r0, r1, 1, 17, r2
    XOR_SAVED_EBIT r0, r3, r1, 2, 19, r2
    GETV    r1, r4, 1        @ mvec[10]
    ADDV    r1, r6, 3
    SUBV    r1, r4, 3
    SUBV    r1, r6, 1
    OR_EBIT r0, r1, 1, 16, r2
    GETV    r1, r4, 2        @ mvec[11]
    ADDV    r1, r6, 0
    SUBV    r1, r4, 0
    SUBV    r1, r6, 2
    OR_EBIT r0, r1, 1, 15, r2
    GETV    r1, r4, 1        @ mvec[12]
    ADDV    r1, r7, 1
    SUBV    r1, r5, 1
    SUBV    r1, r6, 1
    OR_EBIT r0, r1, 1, 14, r2
    GETV    r1, r5, 0        @ mvec[13]
    ADDV    r1, r6, 0
    SUBV    r1, r4, 0
    SUBV    r1, r7, 0
    OR_EBIT r0, r1, 1, 13, r2
    GETV    r1, r4, 0        @ mvec[14]
    ADDV    r1, r4, 3
    ADDV    r1, r5, 1
    ADDV    r1, r5, 2
    ADDV    r1, r6, 1
    ADDV    r1, r6, 2
    ADDV    r1, r7, 0
    ADDV    r1, r7, 3
    SUBV    r1, r4, 1
    SUBV    r1, r4, 2
    SUBV    r1, r5, 0
    SUBV    r1, r5, 3
    SUBV    r1, r6, 0
    SUBV    r1, r6, 3
    SUBV    r1, r7, 1
    SUBV    r1, r7, 2
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[15]
    ADDV    r1, r4, 1
    ADDV    r1, r5, 2
    ADDV    r1, r5, 3
    ADDV    r1, r6, 2
    ADDV    r1, r6, 3
    ADDV    r1, r7, 0
    ADDV    r1, r7, 1
    SUBV    r1, r4, 2
    SUBV    r1, r4, 3
    SUBV    r1, r5, 0
    SUBV    r1, r5, 1
    SUBV    r1, r6, 0
    SUBV    r1, r6, 1
    SUBV    r1, r7, 2
    SUBV    r1, r7, 3
    XOR_SAVED_EBIT r0, r3, r1, 2, 12, r2
    GETV    r1, r8, 0        @ mvec[16]
    ADDV    r1, r8, 1
    SUBV    r1, r4, 0
    SUBV    r1, r4, 1
    OR_EBIT r0, r1, 1, 10, r2
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[17]
    ADDV    r1, r8, 1
    SUBV    r1, r4, 1
    SUBV    r1, r8, 0
    OR_EBIT r0, r1, 1, 9, r2
    XOR_SAVED_EBIT r0, r3, r1, 2, 11, r2
    GETV    r1, r4, 1        @ mvec[18]
    ADDV    r1, r8, 3
    SUBV    r1, r4, 3
    SUBV    r1, r8, 1
    OR_EBIT r0, r1, 1, 8, r2
    GETV    r1, r4, 2        @ mvec[19]
    ADDV    r1, r8, 0
    SUBV    r1, r4, 0
    SUBV    r1, r8, 2
    OR_EBIT r0, r1, 1, 7, r2
    GETV    r1, r4, 1        @ mvec[20]
    ADDV    r1, r9, 1
    SUBV    r1, r5, 1
    SUBV    r1, r8, 1
    OR_EBIT r0, r1, 1, 6, r2
    GETV    r1, r5, 0        @ mvec[21]
    ADDV    r1, r8, 0
    SUBV    r1, r4, 0
    SUBV    r1, r9, 0
    OR_EBIT r0, r1, 1, 5, r2
    GETV    r1, r4, 0        @ mvec[22]
    ADDV    r1, r4, 3
    ADDV    r1, r5, 1
    ADDV    r1, r5, 2
    ADDV    r1, r8, 1
    ADDV    r1, r8, 2
    ADDV    r1, r9, 0
    ADDV    r1, r9, 3
    SUBV    r1, r4, 1
    SUBV    r1, r4, 2
    SUBV    r1, r5, 0
    SUBV    r1, r5, 3
    SUBV    r1, r8, 0
    SUBV    r1, r8, 3
    SUBV    r1, r9, 1
    SUBV    r1, r9, 2
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[23]
    ADDV    r1, r4, 1
    ADDV    r1, r5, 2
    ADDV    r1, r5, 3
    ADDV    r1, r8, 2
    ADDV    r1, r8, 3
    ADDV    r1, r9, 0
    ADDV    r1, r9, 1
    SUBV    r1, r4, 2
    SUBV    r1, r4, 3
    SUBV    r1, r5, 0
    SUBV    r1, r5, 1
    SUBV    r1, r8, 0
    SUBV    r1, r8, 1
    SUBV    r1, r9, 2
    SUBV    r1, r9, 3
    XOR_SAVED_EBIT r0, r3, r1, 2, 4, r2
    GETV    r1, r4, 1        @ mvec[24]
    ADDV    r1, r10, 1
    SUBV    r1, r6, 1
    SUBV    r1, r8, 1
    OR_EBIT r0, r1, 1, 3, r2
    GETV    r1, r6, 0        @ mvec[25]
    ADDV    r1, r8, 0
    SUBV    r1, r4, 0
    SUBV    r1, r10, 0
    OR_EBIT r0, r1, 1, 2, r2
    GETV    r1, r4, 0        @ mvec[26]
    ADDV    r1, r4, 3
    ADDV    r1, r6, 1
    ADDV    r1, r6, 2
    ADDV    r1, r8, 1
    ADDV    r1, r8, 2
    ADDV    r1, r10, 0
    ADDV    r1, r10, 3
    SUBV    r1, r4, 1
    SUBV    r1, r4, 2
    SUBV    r1, r6, 0
    SUBV    r1, r6, 3
    SUBV    r1, r8, 0
    SUBV    r1, r8, 3
    SUBV    r1, r10, 1
    SUBV    r1, r10, 2
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[27]
    ADDV    r1, r4, 1
    ADDV    r1, r6, 2
    ADDV    r1, r6, 3
    ADDV    r1, r8, 2
    ADDV    r1, r8, 3
    ADDV    r1, r10, 0
    ADDV    r1, r10, 1
    SUBV    r1, r4, 2
    SUBV    r1, r4, 3
    SUBV    r1, r6, 0
    SUBV    r1, r6, 1
    SUBV    r1, r8, 0
    SUBV    r1, r8, 1
    SUBV    r1, r10, 2
    SUBV    r1, r10, 3
    XOR_SAVED_EBIT r0, r3, r1, 2, 1, r2
    GETV    r1, r4, 0        @ mvec[28]
    ADDV    r1, r5, 1
    ADDV    r1, r6, 1
    ADDV    r1, r7, 0
    ADDV    r1, r8, 1
    ADDV    r1, r9, 0
    ADDV    r1, r10, 0
    ADDV    r1, r11, 1
    SUBV    r1, r4, 1
    SUBV    r1, r5, 0
    SUBV    r1, r6, 0
    SUBV    r1, r7, 1
    SUBV    r1, r8, 0
    SUBV    r1, r9, 1
    SUBV    r1, r10, 1
    SUBV    r1, r11, 0
    SAVE_EBIT r3, r1, 2
    GETV    r1, r4, 0        @ mvec[29]
    ADDV    r1, r4, 1
    ADDV    r1, r7, 0
    ADDV    r1, r7, 1
    ADDV    r1, r9, 0
    ADDV    r1, r9, 1
    ADDV    r1, r10, 0
    ADDV    r1, r10, 1
    SUBV    r1, r5, 0
    SUBV    r1, r5, 1
    SUBV    r1, r6, 0
    SUBV    r1, r6, 1
    SUBV    r1, r8, 0
    SUBV    r1, r8, 1
    SUBV    r1, r11, 0
    SUBV    r1, r11, 1
    XOR_SAVED_EBIT r0, r3, r1, 2, 0, r2
.endm

// One fixed encode row step.
// r4 holds the remaining message bits, r5 points at bw32_rows, and r1:r0 is
// the packed base-4 accumulator. The row is masked with -(m & 1), then added
// lane-wise modulo 4 using two independent 32-bit halves.
.macro ENCODE_STEP off
    sbfx    r12, r4, #0, #1
    ldr     r2, [r5, #\off]
    ldr     r3, [r5, #(\off + 4)]
    and     r2, r2, r12
    and     r3, r3, r12
    and     r6, r0, r2
    and     r7, r1, r3
    and     r6, r6, #0x55555555
    and     r7, r7, #0x55555555
    eor     r0, r0, r2
    eor     r1, r1, r3
    eor     r0, r0, r6, lsl #1
    eor     r1, r1, r7, lsl #1
    lsr     r4, r4, #1
.endm

.text
.align 2

.global BDD
.type BDD, %function
.thumb_func
BDD:
    b       BDD_asm
.size BDD, .-BDD

.global delabel_bw32
.type delabel_bw32, %function
.thumb_func
delabel_bw32:
    push    {r4-r11, lr}
    BW32_DELABEL_BODY
    pop     {r4-r11, pc}
.size delabel_bw32, .-delabel_bw32

.global decode_bw32
.type decode_bw32, %function
.thumb_func
decode_bw32:
    push    {r4-r11, lr}
    sub     sp, sp, #68
    mov     r4, r0

    .set    off, 0
    .rept   16
        ldr     r1, [r4, #off]
        sadd16  r1, r1, r1
        sadd16  r1, r1, r1
        str     r1, [r4, #off]
        .set    off, off + 4
    .endr

    mov     r0, sp
    mov     r1, r4
    bl      BDD_asm

    mov     r0, sp
    BW32_DELABEL_BODY

    add     sp, sp, #68
    pop     {r4-r11, pc}
.size decode_bw32, .-decode_bw32

.global encode_bw32
.type encode_bw32, %function
.thumb_func
encode_bw32:
    push    {r4-r7}
    mov     r4, r0
    movs    r0, #0
    movs    r1, #0
    ldr     r5, =bw32_rows

    .set    rowoff, 0
    .rept   32
        ENCODE_STEP rowoff
        .set    rowoff, rowoff + 8
    .endr

    pop     {r4-r7}
    bx      lr
.size encode_bw32, .-encode_bw32

.section .rodata
.align 3
bw32_rows:
    .quad   0x00000000000000aa
    .quad   0x0000000000000a0a
    .quad   0x0000000000008888
    .quad   0x0000000000002222
    .quad   0x00000000000a000a
    .quad   0x0000000000880088
    .quad   0x0000000000220022
    .quad   0x0000000008080808
    .quad   0x0000000002020202
    .quad   0x00000000dddddddd
    .quad   0x0000000055555555
    .quad   0x00000000aaaaaaaa
    .quad   0x0000000a0000000a
    .quad   0x0000008800000088
    .quad   0x0000002200000022
    .quad   0x0000080800000808
    .quad   0x0000020200000202
    .quad   0x0000dddd0000dddd
    .quad   0x0000555500005555
    .quad   0x0000aaaa0000aaaa
    .quad   0x0008000800080008
    .quad   0x0002000200020002
    .quad   0x00dd00dd00dd00dd
    .quad   0x0055005500550055
    .quad   0x00aa00aa00aa00aa
    .quad   0x0d0d0d0d0d0d0d0d
    .quad   0x0505050505050505
    .quad   0x0a0a0a0a0a0a0a0a
    .quad   0x1111111111111111
    .quad   0x2222222222222222
    .quad   0x4444444444444444
    .quad   0x8888888888888888
