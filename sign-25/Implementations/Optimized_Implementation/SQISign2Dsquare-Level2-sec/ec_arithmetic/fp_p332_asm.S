// fp332_core.s

.intel_syntax noprefix
.global fiat_p332_add_asm
.global fiat_p332_sub_asm
.global fiat_p332_mul_asm
.global fiat_p332_square_asm

.text
.p2align 4

.L_P0:    .quad 0xffffffffffffffff
.L_P1:    .quad 0xffffffffffffffff
.L_P2:    .quad 0x3a8df0ffffffffff
.L_P3:    .quad 0x80e630a924280fb6
.L_P4:    .quad 0x10cd174e247321c3
.L_P5:    .quad 0x0000000000000ba3

.L_P2_p1: .quad 0x3a8df10000000000
.L_P3_p1: .quad 0x80e630a924280fb6
.L_P4_p1: .quad 0x10cd174e247321c3
.L_P5_p1: .quad 0x0000000000000ba3


fiat_p332_add_asm:
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov r8,  [rsi + 0];  add r8,  [rdx + 0]
    mov r9,  [rsi + 8];  adc r9,  [rdx + 8]
    mov r10, [rsi + 16]; adc r10, [rdx + 16]
    mov r11, [rsi + 24]; adc r11, [rdx + 24]
    mov r12, [rsi + 32]; adc r12, [rdx + 32]
    mov r13, [rsi + 40]; adc r13, [rdx + 40]
    
    setc r14b; movzx r14, r14b 

    mov rax, r8;  sub rax, [rip + .L_P0]
    mov rcx, r9;  sbb rcx, [rip + .L_P1]
    mov rbp, r10; sbb rbp, [rip + .L_P2]
    mov rsi, r11; sbb rsi, [rip + .L_P3]
    mov rdx, r12; sbb rdx, [rip + .L_P4]
    mov r15, r13; sbb r15, [rip + .L_P5]

    sbb r14, 0
    cmovnc r8, rax
    cmovnc r9, rcx
    cmovnc r10, rbp
    cmovnc r11, rsi
    cmovnc r12, rdx
    cmovnc r13, r15

    mov [rdi + 0],  r8;  mov [rdi + 8],  r9;  mov [rdi + 16], r10
    mov [rdi + 24], r11; mov [rdi + 32], r12; mov [rdi + 40], r13

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret


fiat_p332_sub_asm:
    push rbp
    push r12
    push r13
    push r14  

    mov r8,  [rsi + 0];  sub r8,  [rdx + 0]
    mov r9,  [rsi + 8];  sbb r9,  [rdx + 8]
    mov r10, [rsi + 16]; sbb r10, [rdx + 16]
    mov r11, [rsi + 24]; sbb r11, [rdx + 24]
    mov r12, [rsi + 32]; sbb r12, [rdx + 32] 
    mov r13, [rsi + 40]; sbb r13, [rdx + 40] 

    sbb rax, rax 

    mov rcx, [rip + .L_P2]; and rcx, rax
    mov rsi, [rip + .L_P3]; and rsi, rax
    mov rdx, [rip + .L_P4]; and rdx, rax
    mov r14, [rip + .L_P5]; and r14, rax

    add r8,  rax; adc r9,  rax; adc r10, rcx
    adc r11, rsi; adc r12, rdx; adc r13, r14

    mov [rdi + 0],  r8;  mov [rdi + 8],  r9;  mov [rdi + 16], r10
    mov [rdi + 24], r11; mov [rdi + 32], r12; mov [rdi + 40], r13

    pop r14
    pop r13
    pop r12
    pop rbp
    ret


.macro FPMUL_ROUND offset, t0, t1, t2, t3, t4, t5, t6, t7
    mov rdx, [rsi + \offset*8]   
    xor rax, rax                 
    mulx rcx, rbx, [rdi + 0*8];  adox \t0, rbx;  adcx \t1, rcx 
    mulx rcx, rbx, [rdi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rdi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rdi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mulx rcx, rbx, [rdi + 4*8];  adox \t4, rbx;  adcx \t5, rcx
    mulx rcx, rbx, [rdi + 5*8];  adox \t5, rbx;  adcx \t6, rcx
    mov rbx, 0;                  adox \t6, rbx;  adcx \t7, rbx;  adox \t7, rbx

    mov rdx, \t0            
    xor rax, rax
    mulx rcx, rbx, [rip + .L_P2_p1]; adox \t2, rbx; adcx \t3, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox \t3, rbx; adcx \t4, rcx
    mulx rcx, rbx, [rip + .L_P4_p1]; adox \t4, rbx; adcx \t5, rcx
    mulx rcx, rbx, [rip + .L_P5_p1]; adox \t5, rbx; adcx \t6, rcx
    

    mov rbx, 0;                      adox \t6, rbx; adcx \t7, rbx; adox \t7, rbx
    
    mov \t0, 0                   
.endm


fiat_p332_mul_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    push rdi          

    mov rdi, rdx            

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14; xor r15, r15

    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14, r15
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r15, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r14, r15, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r14, r15, r8,  r9,  r10
    FPMUL_ROUND 4, r12, r13, r14, r15, r8,  r9,  r10, r11
    FPMUL_ROUND 5, r13, r14, r15, r8,  r9,  r10, r11, r12

    mov rax, r14; mov rbx, r15; mov rcx, r8; mov rdx, r9; mov rsi, r10; mov rbp, r11

    sub rax, [rip + .L_P0]
    sbb rbx, [rip + .L_P1]
    sbb rcx, [rip + .L_P2]
    sbb rdx, [rip + .L_P3]
    sbb rsi, [rip + .L_P4]
    sbb rbp, [rip + .L_P5]
    
    cmovnc r14, rax
    cmovnc r15, rbx
    cmovnc r8,  rcx
    cmovnc r9,  rdx
    cmovnc r10, rsi
    cmovnc r11, rbp

    pop rdi
    mov [rdi + 0],  r14; mov [rdi + 8],  r15; mov [rdi + 16], r8
    mov [rdi + 24], r9;  mov [rdi + 32], r10; mov [rdi + 40], r11

    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p332_square_asm:
    mov rdx, rsi 
    jmp fiat_p332_mul_asm

.section .note.GNU-stack,"",@progbits
