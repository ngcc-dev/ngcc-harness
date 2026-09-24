.syntax unified
.thumb

.macro _first_layer_NTT res, zeta, q, qinv, aj, ajl, ajll, temp, s0
    vmov \zeta, \s0

    sub.w \temp, \ajl, \ajll

    smull \zeta, \res, \zeta, \temp
    mul.w \temp, \zeta, \qinv
    smlal \zeta, \res, \temp, \q               // res = z[2]

    mov.w \zeta, \ajl                          // z[0]
    mov.w \temp, \ajll                         // z[1]

    sub.w \ajl, \aj, \zeta
    sub.w \ajl, \ajl, \res

    add.w \ajll, \aj, \zeta
    add.w \ajll, \ajll, \temp

    sub.w \aj, \aj, \temp
    add.w \aj, \aj, \res
.endm

.macro _num_zeta res, zeta, q, qinv,aj, ajl, s0
    vmov \zeta,\s0

    smull \zeta, \res, \zeta, \ajl
    mul.w \ajl, \zeta, \qinv
    smlal \zeta, \res, \ajl, \q  // res=t

    sub.w \ajl, \aj, \res
    add.w \aj, \aj, \res
.endm



.global asm_ntt_n1087
.type asm_ntt_n1087,%function
.align 2
asm_ntt_n1087:

    ptr_p     .req R0
    ptr_zeta  .req R1
    zeta      .req R1
    qinv      .req R2
    q         .req R3
    pol4      .req R4
    pol0      .req R5
    pol1      .req R6
    pol2      .req R7
    pol3      .req R8
    temp_h    .req R9
    temp_l    .req R10
    pol5     .req R11
    pol6     .req R12
    pol7     .req R14

    push.w {r4-r11, r14}

    movw q, #0xff01
    movt q, #0x00ff               // Q

    movw qinv, #0xFEFF
    movt qinv, #0xFFFE            // Qinv

    vmov s0, ptr_zeta
    vmov s1,ptr_p

    // layer 1
    vldr s2, [ptr_zeta, #1*4]

    add.w pol3, ptr_p, #768*2*4
    .rept 768
        ldr.w pol0, [ptr_p]
        ldr.w pol1, [ptr_p, #768*1*4]
        add.w ptr_p, ptr_p, #768*2*4
        ldr.w pol2, [ptr_p]
        sub.w ptr_p, ptr_p, #768*2*4

        _first_layer_NTT temp_l, zeta, q, qinv, pol0, pol1, pol2, temp_h, s2

        str.w pol1, [ptr_p, #768*1*4]
        add.w ptr_p, ptr_p, #768*2*4
        str.w pol2, [ptr_p]
        sub.w ptr_p, ptr_p, #768*2*4
        str.w pol0, [ptr_p], #4
    .endr

    vmov ptr_p, s1
    vmov ptr_zeta, s0
    mov.w temp_h,#2
    mov.w temp_l,#4
    mla.w temp_h, temp_h, temp_l, ptr_zeta
    .rept 3
        vldm temp_h!, {s2-s8}
        .rept 96
            ldr pol0, [ptr_p] 
            ldr pol1, [ptr_p, #96*1*4]   
            ldr pol2, [ptr_p, #96*2*4]   
            ldr pol3, [ptr_p, #96*3*4]   
            ldr pol4, [ptr_p, #96*4*4]   
            ldr pol5, [ptr_p, #96*5*4]   
            ldr pol6, [ptr_p, #96*6*4] 
            ldr pol7, [ptr_p, #96*7*4]   

            _num_zeta temp_l, zeta, q, qinv, pol0, pol4, s2
            _num_zeta temp_l, zeta, q, qinv, pol1, pol5, s2
            _num_zeta temp_l, zeta, q, qinv, pol2, pol6, s2
            _num_zeta temp_l, zeta, q, qinv, pol3, pol7, s2

            _num_zeta temp_l, zeta, q, qinv, pol0, pol2, s3
            _num_zeta temp_l, zeta, q, qinv, pol1, pol3, s3
            _num_zeta temp_l, zeta, q, qinv, pol4, pol6, s4
            _num_zeta temp_l, zeta, q, qinv, pol5, pol7, s4

            _num_zeta temp_l, zeta, q, qinv, pol0, pol1, s5
            _num_zeta temp_l, zeta, q, qinv, pol2, pol3, s6
            _num_zeta temp_l, zeta, q, qinv, pol4, pol5, s7
            _num_zeta temp_l, zeta, q, qinv, pol6, pol7, s8

            str pol1, [ptr_p, #96*1*4]   
            str pol2, [ptr_p, #96*2*4]   
            str pol3, [ptr_p, #96*3*4]   
            str pol4, [ptr_p, #96*4*4]   
            str pol5, [ptr_p, #96*5*4]   
            str pol6, [ptr_p, #96*6*4] 
            str pol7, [ptr_p, #96*7*4]
            str pol0, [ptr_p], #4
        .endr
        add ptr_p, ptr_p, #96*7*4
    .endr

    vmov ptr_p, s1
    vmov ptr_zeta, s0
    mov.w temp_h,#23
    mov.w temp_l,#4
    mla.w temp_h, temp_h, temp_l, ptr_zeta
    .rept 24
        vldm temp_h!, {s2-s8}
        .rept 12
            ldr pol0, [ptr_p] 
            ldr pol1, [ptr_p, #12*1*4]   
            ldr pol2, [ptr_p, #12*2*4] 
            ldr pol3, [ptr_p, #12*3*4] 
            ldr pol4, [ptr_p, #12*4*4] 
            ldr pol5, [ptr_p, #12*5*4]   
            ldr pol6, [ptr_p, #12*6*4]  
            ldr pol7, [ptr_p, #12*7*4]   

            _num_zeta temp_l, zeta, q, qinv, pol0, pol4, s2
            _num_zeta temp_l, zeta, q, qinv, pol1, pol5, s2
            _num_zeta temp_l, zeta, q, qinv, pol2, pol6, s2
            _num_zeta temp_l, zeta, q, qinv, pol3, pol7, s2

            _num_zeta temp_l, zeta, q, qinv, pol0, pol2, s3
            _num_zeta temp_l, zeta, q, qinv, pol1, pol3, s3
            _num_zeta temp_l, zeta, q, qinv, pol4, pol6, s4
            _num_zeta temp_l, zeta, q, qinv, pol5, pol7, s4

            _num_zeta temp_l, zeta, q, qinv, pol0, pol1, s5
            _num_zeta temp_l, zeta, q, qinv, pol2, pol3, s6
            _num_zeta temp_l, zeta, q, qinv, pol4, pol5, s7
            _num_zeta temp_l, zeta, q, qinv, pol6, pol7, s8

            str pol1, [ptr_p, #12*1*4]   
            str pol2, [ptr_p, #12*2*4] 
            str pol3, [ptr_p, #12*3*4] 
            str pol4, [ptr_p, #12*4*4] 
            str pol5, [ptr_p, #12*5*4]   
            str pol6, [ptr_p, #12*6*4]  
            str pol7, [ptr_p, #12*7*4]  
            str pol0, [ptr_p], #4
        .endr
        add ptr_p, ptr_p, #12*7*4
    .endr

    vmov ptr_p, s1
    vmov ptr_zeta, s0
    mov.w temp_h,#191
    mov.w temp_l,#4
    mla.w temp_h, temp_h, temp_l, ptr_zeta
    .rept 192
        vldm temp_h!, {s2-s4}
        .rept 3
            ldr pol0, [ptr_p] 
            ldr pol1, [ptr_p, #3*1*4]   
            ldr pol2, [ptr_p, #3*2*4] 
            ldr pol3, [ptr_p, #3*3*4] 

            _num_zeta temp_l, zeta, q, qinv,pol0, pol2, s2
            _num_zeta temp_l, zeta, q, qinv,pol1, pol3, s2

            _num_zeta temp_l, zeta, q, qinv,pol0, pol1, s3
            _num_zeta temp_l, zeta, q, qinv,pol2, pol3, s4

            str pol1, [ptr_p, #3*1*4]   
            str pol2, [ptr_p, #3*2*4] 
            str pol3, [ptr_p, #3*3*4] 
            str pol0, [ptr_p], #4
        .endr
        add ptr_p,ptr_p,#3*3*4
    .endr

    pop.w {r4-r11, pc}

    .unreq ptr_p
    .unreq ptr_zeta
    .unreq qinv
    .unreq q
    .unreq zeta
    .unreq pol0
    .unreq pol1
    .unreq pol2
    .unreq pol3
    .unreq temp_h
    .unreq temp_l
.size asm_ntt_n1087, .-asm_ntt_n1087