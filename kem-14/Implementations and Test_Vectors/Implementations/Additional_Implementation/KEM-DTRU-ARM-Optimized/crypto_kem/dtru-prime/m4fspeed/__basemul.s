.syntax unified
.thumb

.macro compute_c1 out, a0, a1, a2, b0, b1, b2, twiddle, Qprime, Q, tmp, tmp2
    smull \tmp, \out, \a2, \b2
    mul \tmp2, \tmp, \Qprime
    smlal \tmp, \out, \tmp2, \Q
    smull \tmp, \out, \out, \twiddle
    smlal \tmp, \out, \a0, \b1
    smlal \tmp, \out, \a1, \b0
    mul \tmp2, \tmp, \Qprime
    smlal \tmp, \out, \tmp2, \Q
.endm

.macro compute_c2 out, a0, a1, a2, b0, b1, b2, twiddle, Qprime, Q, tmp, tmp2
    smull \tmp, \out, \a0, \b2
    smlal \tmp, \out, \a1, \b1
    smlal \tmp, \out, \a2, \b0
    mul \tmp2, \tmp, \Qprime
    smlal \tmp, \out, \tmp2, \Q
.endm

.macro compute_c0 out, a0, a1, a2, b0, b1, b2, twiddle, Qprime, Q, tmp, tmp2
    smull \tmp, \out, \a1, \b2
    smlal \tmp, \out, \a2, \b1
    mul \tmp2, \tmp, \Qprime
    smlal \tmp, \out, \tmp2, \Q
    smull \tmp, \out, \out, \twiddle
    smlal \tmp, \out, \a0, \b0
    mul \tmp2, \tmp, \Qprime
    smlal \tmp, \out, \tmp2, \Q
.endm

.global asm_poly_basemul_prime_opt
.type asm_poly_basemul_prime_opt,%function
.align 2
asm_poly_basemul_prime_opt:
    ptr_c .req R0
    out .req R1
    ptr_a .req R1
    ptr_b .req R2
    ptr_zeta .req R3
    zeta .req R3
    q .req R4
    qinv .req R5
    a_0 .req R6
    a_1 .req R7
    a_2 .req R8
    b_0 .req R9
    b_1 .req R10
    b_2 .req R11
    tmp_0 .req R12
    tmp_1 .req R14

    push.w {r4-r11, r14}

    add ptr_zeta, #382*4

    movw q, #0xff01
    movt q, #0x00ff                 //Q

    movw qinv, #0xFEFF
    movt qinv, #0xFFFE              // Qinv

    vmov s2,ptr_a
    vmov s3,ptr_zeta

    mov tmp_0, #384
    vmov s5, tmp_0
    1:
        vmov ptr_a,s2
        ldr a_0,[ptr_a],#4
        ldr a_1,[ptr_a],#4
        ldr a_2,[ptr_a],#4

        ldr b_0,[ptr_b],#4
        ldr b_1,[ptr_b],#4
        ldr b_2,[ptr_b],#4
        vmov s2,ptr_a

        vmov tmp_0,s3
        ldr zeta, [tmp_0], #4
        vmov s3,tmp_0
        vmov s4,zeta

        compute_c1 out, a_0, a_1, a_2, b_0, b_1, b_2, zeta, qinv, q, tmp_0, tmp_1
        str out,[ptr_c,#4]
        compute_c2 out, a_0, a_1, a_2, b_0, b_1, b_2, zeta, qinv, q, tmp_0, tmp_1
        str out,[ptr_c,#4*2]
        compute_c0 out, a_0, a_1, a_2, b_0, b_1, b_2, zeta, qinv, q, tmp_0, tmp_1
        str out,[ptr_c],#4*3

        //next 3
        vmov ptr_a,s2
        ldr a_0,[ptr_a],#4
        ldr a_1,[ptr_a],#4
        ldr a_2,[ptr_a],#4

        ldr b_0,[ptr_b],#4
        ldr b_1,[ptr_b],#4
        ldr b_2,[ptr_b],#4
        vmov s2,ptr_a

        vmov zeta,s4
        rsbs zeta, zeta, #0
        vmov s4,zeta
        compute_c1 out, a_0, a_1, a_2, b_0, b_1, b_2, zeta, qinv, q, tmp_0, tmp_1
        str out,[ptr_c,#4]
        compute_c2 out, a_0, a_1, a_2, b_0, b_1, b_2, zeta, qinv, q, tmp_0, tmp_1
        str out,[ptr_c,#4*2]
        compute_c0 out, a_0, a_1, a_2, b_0, b_1, b_2, zeta, qinv, q, tmp_0, tmp_1
        str out,[ptr_c],#4*3
       
    vmov tmp_0, s5
    subs tmp_0, #1
    vmov s5, tmp_0
    bne 1b

    //restore registers
    pop.w {r4-r11, pc}

    .unreq ptr_c
    .unreq ptr_a
    .unreq out 
    .unreq ptr_b 
    .unreq ptr_zeta 
    .unreq zeta 
    .unreq q
    .unreq qinv 
    .unreq a_0 
    .unreq a_1 
    .unreq a_2 
    .unreq b_0 
    .unreq b_1 
    .unreq b_2 
    .unreq tmp_0 
    .unreq tmp_1 
.size asm_poly_basemul_prime_opt, .-asm_poly_basemul_prime_opt
