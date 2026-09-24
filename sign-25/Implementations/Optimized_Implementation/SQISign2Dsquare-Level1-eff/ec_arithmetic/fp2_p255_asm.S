.intel_syntax noprefix
.global fp2_mul255_asm
.global fp2_sqr255_asm

.text
.p2align 4

.L_P0:    .quad 0xffffffffffffffff
.L_P1:    .quad 0xffffffffffffffff
.L_P2:    .quad 0xa7ecc14ec3fa83c7
.L_P3:    .quad 0x62d7f37f9815e5fc

.L_P2_p1: .quad 0xa7ecc14ec3fa83c8
.L_P3_p1: .quad 0x62d7f37f9815e5fc


.macro FPMUL_ROUND offset, t0, t1, t2, t3, t4, t5
    mov rdx, [r15 + \offset*8]
    xor rax, rax
    mulx rcx, rbx, [rdi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rdi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rdi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rdi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mov rbx, 0;                  adox \t4, rbx;  adcx \t5, rbx;  adox \t5, rbx

    mov rdx, \t0  
    xor rax, rax
    mulx rcx, rbx, [rip + .L_P2_p1]; adox \t2, rbx; adcx \t3, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox \t3, rbx; adcx \t4, rcx
    mov rbx, 0;                      adox \t4, rbx; adcx \t5, rbx; adox \t5, rbx
    
    mov \t0, 0
.endm


.macro FPDBLMUL_ROUND offset, t0, t1, t2, t3, t4, t5
    mov rdx, [r15 + \offset*8]
    xor rax, rax
    mulx rcx, rbx, [rdi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rdi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rdi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rdi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mov rbx, 0;                  adox \t4, rbx;  adcx \t5, rbx;  adox \t5, rbx

    mov rdx, [rbp + \offset*8]
    xor rax, rax
    mulx rcx, rbx, [rsi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rsi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rsi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rsi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mov rbx, 0;                  adox \t4, rbx;  adcx \t5, rbx;  adox \t5, rbx

    mov rdx, \t0
    xor rax, rax
    mulx rcx, rbx, [rip + .L_P2_p1]; adox \t2, rbx; adcx \t3, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox \t3, rbx; adcx \t4, rcx
    mov rbx, 0;                      adox \t4, rbx; adcx \t5, rbx; adox \t5, rbx
    
    mov \t0, 0
.endm


.macro REDUCE_P t0, t1, t2, t3
    mov rax, \t0
    mov rbx, \t1
    mov rcx, \t2
    mov rdx, \t3

    sub rax, [rip + .L_P0]
    sbb rbx, [rip + .L_P1]
    sbb rcx, [rip + .L_P2]
    sbb rdx, [rip + .L_P3]

    cmovnc \t0, rax
    cmovnc \t1, rbx
    cmovnc \t2, rcx
    cmovnc \t3, rdx
.endm



fp2_mul255_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 56
    mov [rsp + 32], rdi 
    mov [rsp + 40], rsi 
    mov [rsp + 48], rdx 

    mov r8,  [rip + .L_P0]; sub r8,  [rdx + 32]; mov [rsp + 0],  r8
    mov r9,  [rip + .L_P1]; sbb r9,  [rdx + 40]; mov [rsp + 8],  r9
    mov r10, [rip + .L_P2]; sbb r10, [rdx + 48]; mov [rsp + 16], r10
    mov r11, [rip + .L_P3]; sbb r11, [rdx + 56]; mov [rsp + 24], r11

    mov r15, [rsp + 40]      
    mov rdi, [rsp + 48]      
    lea rbp, [r15 + 32]    
    lea rsi, [rsp + 0]      

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13
    FPDBLMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13
    FPDBLMUL_ROUND 1, r9,  r10, r11, r12, r13, r8
    FPDBLMUL_ROUND 2, r10, r11, r12, r13, r8,  r9
    FPDBLMUL_ROUND 3, r11, r12, r13, r8,  r9,  r10
    
    REDUCE_P r12, r13, r8, r9

    movq xmm0, r12
    movq xmm1, r13
    movq xmm2, r8
    movq xmm3, r9

    mov r15, [rsp + 40]     
    mov rsi, [rsp + 48]    
    lea rbp, [r15 + 32]      
    lea rdi, [rsi + 32]      

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13
    FPDBLMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13
    FPDBLMUL_ROUND 1, r9,  r10, r11, r12, r13, r8
    FPDBLMUL_ROUND 2, r10, r11, r12, r13, r8,  r9
    FPDBLMUL_ROUND 3, r11, r12, r13, r8,  r9,  r10

    REDUCE_P r12, r13, r8, r9

    mov rdi, [rsp + 32]      
    
    mov [rdi + 32], r12; mov [rdi + 40], r13; mov [rdi + 48], r8; mov [rdi + 56], r9
    
    movq r12, xmm0
    movq r13, xmm1
    movq r8,  xmm2
    movq r9,  xmm3
    mov [rdi + 0],  r12; mov [rdi + 8],  r13; mov [rdi + 16], r8; mov [rdi + 24], r9

    add rsp, 56
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fp2_sqr255_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 120 
    mov [rsp + 96], rdi 
    mov [rsp + 104], rsi 

    mov r8, [rsi + 0];  add r8, [rsi + 32]
    mov r9, [rsi + 8];  adc r9, [rsi + 40]
    mov r10,[rsi + 16]; adc r10,[rsi + 48]
    mov r11,[rsi + 24]; adc r11,[rsi + 56]
    mov rax, r8; mov rbx, r9; mov rcx, r10; mov rdx, r11
    sub rax, [rip + .L_P0]; sbb rbx, [rip + .L_P1]; sbb rcx, [rip + .L_P2]; sbb rdx, [rip + .L_P3]
    cmovnc r8, rax; cmovnc r9, rbx; cmovnc r10, rcx; cmovnc r11, rdx
    mov [rsp + 0], r8; mov [rsp + 8], r9; mov [rsp + 16], r10; mov [rsp + 24], r11

    mov r8, [rsi + 0];  sub r8, [rsi + 32]
    mov r9, [rsi + 8];  sbb r9, [rsi + 40]
    mov r10,[rsi + 16]; sbb r10,[rsi + 48]
    mov r11,[rsi + 24]; sbb r11,[rsi + 56]
    sbb r12, r12 
    mov rax, [rip + .L_P0]; and rax, r12
    mov rbx, [rip + .L_P1]; and rbx, r12
    mov rcx, [rip + .L_P2]; and rcx, r12
    mov rdx, [rip + .L_P3]; and rdx, r12
    add r8, rax; adc r9, rbx; adc r10, rcx; adc r11, rdx
    mov [rsp + 32], r8; mov [rsp + 40], r9; mov [rsp + 48], r10; mov [rsp + 56], r11

    mov r8, [rsi + 0];  add r8, r8
    mov r9, [rsi + 8];  adc r9, r9
    mov r10,[rsi + 16]; adc r10, r10
    mov r11,[rsi + 24]; adc r11, r11
    mov rax, r8; mov rbx, r9; mov rcx, r10; mov rdx, r11
    sub rax, [rip + .L_P0]; sbb rbx, [rip + .L_P1]; sbb rcx, [rip + .L_P2]; sbb rdx, [rip + .L_P3]
    cmovnc r8, rax; cmovnc r9, rbx; cmovnc r10, rcx; cmovnc r11, rdx
    mov [rsp + 64], r8; mov [rsp + 72], r9; mov [rsp + 80], r10; mov [rsp + 88], r11

    lea r15, [rsp + 0]       
    lea rdi, [rsp + 32]      
    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13
    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r8,  r9,  r10
    
    REDUCE_P r12, r13, r8, r9
    
    movq xmm0, r12; movq xmm1, r13; movq xmm2, r8; movq xmm3, r9

    lea r15, [rsp + 64]      
    mov rsi, [rsp + 104]   
    lea rdi, [rsi + 32]      

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13
    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r8,  r9,  r10

    REDUCE_P r12, r13, r8, r9

    mov rdi, [rsp + 96] 
    
    mov [rdi + 32], r12; mov [rdi + 40], r13; mov [rdi + 48], r8; mov [rdi + 56], r9
    
    movq r12, xmm0; movq r13, xmm1; movq r8, xmm2; movq r9, xmm3
    mov [rdi + 0],  r12; mov [rdi + 8],  r13; mov [rdi + 16], r8; mov [rdi + 24], r9

    add rsp, 120
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


.section .note.GNU-stack,"",@progbits
