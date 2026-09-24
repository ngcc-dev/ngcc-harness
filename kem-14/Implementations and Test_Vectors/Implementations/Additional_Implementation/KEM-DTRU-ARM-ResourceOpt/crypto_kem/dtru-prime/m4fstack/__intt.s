.syntax unified
.thumb

.macro _montgomery_inv_zeta res, zeta, q, qinv,aj, ajl, s0
    vmov \zeta,\s0

    mov.w \res,\aj
    add.w \aj,\res,\ajl
    sub.w \ajl, \res, \ajl

    smull \zeta, \res, \zeta, \ajl
    mul.w \ajl, \zeta, \qinv
    smlal \zeta, \res, \ajl, \q  

    mov.w \ajl,\res
.endm

.macro last_layer_inntt res, zeta1, zeta2, q, qinv, aj, ajl, ajll, temp1, temp2, temp3, temp4, s0, s1
    vmov \zeta1, \s0
    vmov \zeta2, \s1

    sub.w \temp1, \aj, \ajl

    smull \zeta1, \res, \zeta1, \temp1
    mul.w \temp1, \zeta1, \qinv
    smlal \zeta1, \res, \temp1, \q              // res = z[2]

    mov.w \zeta1, \aj                           // zeta1 = z[0]
    mov.w \temp1, \ajl                          // temp1 = z[1]
    mov.w \temp3, \ajll                         // temp3  = a[j + 2 * len]

    add.w \ajll, \ajll, \zeta1
    add.w \ajll, \ajll, \temp1
    
    smull \zeta2, \aj, \zeta2, \ajll
    mul.w \ajll, \zeta2, \qinv
    smlal \zeta2, \aj, \ajll, \q    

    sub.w \temp4, \temp3, \temp1
    add.w \temp4, \temp4, \res

    vmov \zeta2, \s1
    smull \zeta2, \ajl, \zeta2, \temp4
    mul.w \temp4, \zeta2, \qinv
    smlal \zeta2, \ajl, \temp4, \q   

    sub.w \temp4, \temp3, \zeta1
    sub.w \temp4, \temp4, \res

    vmov \zeta2, \s1
    smull \zeta2, \ajll, \zeta2, \temp4
    mul.w \temp4, \zeta2, \qinv
    smlal \zeta2, \ajll, \temp4, \q  
.endm

.global asm_invntt_toNUMBER_CONDSH
.type asm_invntt_toNUMBER_CONDSH,%function
.align 2
asm_invntt_toNUMBER_CONDSH:

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

    vmov s0,ptr_zeta
    vmov s1,ptr_p

    mov temp_h,ptr_zeta
    .rept 192
        vldm temp_h!, {s2-s4}
        .rept 3
            ldr pol0, [ptr_p] 
            ldr pol1, [ptr_p, #3*1*4]  
            ldr pol2, [ptr_p, #3*2*4] 
            ldr pol3, [ptr_p, #3*3*4]   

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol1, s2
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol2, pol3, s3

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol2, s4
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol1, pol3, s4

            str pol1, [ptr_p, #3*1*4]  
            str pol2, [ptr_p, #3*2*4] 
            str pol3, [ptr_p, #3*3*4] 
            str pol0, [ptr_p], #4
        .endr
        add.w ptr_p,#3*3*4
    .endr

    vmov ptr_p,s1
    vmov ptr_zeta,s0

    mov.w temp_h, #576
    mov.w temp_l, #4
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

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol1, s2
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol2, pol3, s3
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol4, pol5, s4
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol6, pol7, s5

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol2, s6
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol1, pol3, s6
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol4, pol6, s7
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol5, pol7, s7

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol4, s8
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol1, pol5, s8
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol2, pol6, s8
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol3, pol7, s8

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

    vmov ptr_p,s1
    vmov ptr_zeta,s0

    mov.w temp_h, #744
    mov.w temp_l, #4
    mla.w temp_h, temp_h, temp_l, ptr_zeta

    .rep 3
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

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol1, s2
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol2, pol3, s3
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol4, pol5, s4
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol6, pol7, s5

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol2, s6
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol1, pol3, s6
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol4, pol6, s7
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol5, pol7, s7

            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol0, pol4, s8
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol1, pol5, s8
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol2, pol6, s8
            _montgomery_inv_zeta temp_l, zeta, q, qinv, pol3, pol7, s8

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

    vmov ptr_p,s1
    vmov ptr_zeta,s0

    mov.w temp_h, #765
    mov.w temp_l, #4
    mla.w temp_h, temp_h, temp_l, ptr_zeta

    vldm temp_h!, {s2-s3}

    .rept 768
        ldr.w pol0, [ptr_p]
        ldr.w pol1, [ptr_p, #768*1*4]
        add.w ptr_p, ptr_p, #768*2*4
        ldr.w pol2, [ptr_p]
        sub.w ptr_p, ptr_p, #768*2*4

        last_layer_inntt temp_l, pol3, pol4, q, qinv, pol0, pol1, pol2, pol5, pol6, pol7, temp_h, s2, s3

        str.w pol1, [ptr_p, #768*1*4]
        add.w ptr_p, ptr_p, #768*2*4
        str.w pol2, [ptr_p]
        sub.w ptr_p, ptr_p, #768*2*4
        str.w pol0, [ptr_p], #4
    .endr

    pop.w {r4-r11, pc}

    .unreq ptr_p    
   .unreq ptr_zeta 
   .unreq zeta     
   .unreq qinv     
   .unreq q        
   .unreq  pol4     
   .unreq pol0     
   .unreq pol1     
   .unreq pol2     
   .unreq pol3     
   .unreq temp_h   
   .unreq temp_l   
   .unreq pol5     
   .unreq pol6     
   .unreq pol7 

.size asm_invntt_toNUMBER_CONDSH, .-asm_invntt_toNUMBER_CONDSH