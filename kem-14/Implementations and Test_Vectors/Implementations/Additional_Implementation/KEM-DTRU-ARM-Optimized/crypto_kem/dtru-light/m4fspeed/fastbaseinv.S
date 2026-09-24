.syntax unified
.cpu cortex-m4
.thumb

.macro montreduce q, qinv, a, tmp
    smulbb \tmp, \a, \qinv
    smlabb \a, \q, \tmp, \a
.endm

.macro double_fqinv det, tmp, tmp2, tmp3, loop, q, qinv
    movw \tmp, #19
    movt \tmp, #1
    smulbb \tmp2, \det, \tmp
    montreduce \q, \qinv, \tmp2, \tmp3
    
    movw \loop, #8
    1:
        smultt \tmp, \tmp, \tmp2
        montreduce \q, \qinv, \tmp, \tmp3
        smultt \tmp2, \tmp2, \tmp2
        montreduce \q, \qinv, \tmp2, \tmp3
    subs.w \loop, #1
    bne.w 1b
    
    smultt \tmp2, \tmp2, \tmp2
    montreduce \q, \qinv, \tmp2, \tmp3
    
    smultt \tmp, \tmp, \tmp2
    montreduce \q, \qinv, \tmp, \tmp3

    movw \tmp2, #19
    movt \tmp2, #1
    smultb \det, \det, \tmp2
    montreduce \q, \qinv, \det, \tmp3
    
    movw \loop, #8
    2:
        smultt \tmp2, \tmp2, \det
        montreduce \q, \qinv, \tmp2, \tmp3
        smultt \det, \det, \det
        montreduce \q, \qinv, \det, \tmp3
    subs.w \loop, #1
    bne.w 2b
    
    smultt \det, \det, \det
    montreduce \q, \qinv, \det, \tmp3
    
    smultt \tmp2, \tmp2, \det
    montreduce \q, \qinv, \tmp2, \tmp3

    pkhtb \det, \tmp2, \tmp, asr #16
.endm

.macro single_fqinv det, tmp, tmp2, tmp3, loop, q, qinv
    movw \tmp, #19
    movt \tmp, #1
    smulbb \tmp2, \det, \tmp
    montreduce \q, \qinv, \tmp2, \tmp3
    
    movw \loop, #8
    3:
        smultt \tmp, \tmp, \tmp2
        montreduce \q, \qinv, \tmp, \tmp3
        smultt \tmp2, \tmp2, \tmp2
        montreduce \q, \qinv, \tmp2, \tmp3
    subs.w \loop, #1
    bne.w 3b
    
    smultt \tmp2, \tmp2, \tmp2
    montreduce \q, \qinv, \tmp2, \tmp3
    
    smultt \tmp, \tmp, \tmp2
    montreduce \q, \qinv, \tmp, \tmp3
    
    asr \det, \tmp, #16
.endm

.global fqinv_asm
.type fqinv_asm, %function
.align 2
fqinv_asm:
    push {r4-r11, lr}

    rptr  .req r0
    q     .req r7
    qinv  .req r8
    tmp   .req r9
    tmp2  .req r10
    tmp3  .req r11
    loop  .req r14

    movw q, #769
    movw qinv, #767 

    ldr r1, [rptr]
    double_fqinv r1, tmp, tmp2, tmp3, loop, q, qinv
    str r1, [rptr]

    .unreq rptr
    .unreq q
    .unreq qinv
    .unreq tmp
    .unreq tmp2
    .unreq tmp3
    .unreq loop
    pop {r4-r11, pc}

.global fqinv_asm_single
.type fqinv_asm_single, %function
.align 2
fqinv_asm_single:
    push {r4-r11, lr}

    rptr  .req r0
    q     .req r7
    qinv  .req r8
    tmp   .req r9
    tmp2  .req r10
    tmp3  .req r11
    loop  .req r14

    movw q, #769
    movw qinv, #767 

    ldrh r1, [rptr]
    single_fqinv r1, tmp, tmp2, tmp3, loop, q, qinv
    strh r1, [rptr]

    .unreq rptr
    .unreq q
    .unreq qinv
    .unreq tmp
    .unreq tmp2
    .unreq tmp3
    .unreq loop
    pop {r4-r11, pc}