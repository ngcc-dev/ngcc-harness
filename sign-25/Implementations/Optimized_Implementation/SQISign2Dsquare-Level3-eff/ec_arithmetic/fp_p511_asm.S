// fp_core511.s

.intel_syntax noprefix
.global fiat_p511_add_asm
.global fiat_p511_sub_asm
.global fiat_p511_mul_asm
.global fiat_p511_square_asm

.text
.p2align 4

.L_P0:    .quad 0xffffffffffffffff
.L_P1:    .quad 0xffffffffffffffff
.L_P2:    .quad 0xffffffffffffffff
.L_P3:    .quad 0xffffffffffffffff
.L_P4:    .quad 0x4c46ef90acec987f
.L_P5:    .quad 0xb8d8a1c1c552969b
.L_P6:    .quad 0x234a5c454b6e0020
.L_P7:    .quad 0x4c54193130bb665f

.L_P4_p1: .quad 0x4c46ef90acec9880
.L_P5_p1: .quad 0xb8d8a1c1c552969b
.L_P6_p1: .quad 0x234a5c454b6e0020
.L_P7_p1: .quad 0x4c54193130bb665f


fiat_p511_add_asm:
    push r12; push r13; push r14; push r15
    
    mov r8,  [rsi + 0];  add r8,  [rdx + 0]
    mov r9,  [rsi + 8];  adc r9,  [rdx + 8]
    mov r10, [rsi + 16]; adc r10, [rdx + 16]
    mov r11, [rsi + 24]; adc r11, [rdx + 24]
    mov r12, [rsi + 32]; adc r12, [rdx + 32]
    mov r13, [rsi + 40]; adc r13, [rdx + 40]
    mov r14, [rsi + 48]; adc r14, [rdx + 48]
    mov r15, [rsi + 56]; adc r15, [rdx + 56]
    setc cl; movzx rcx, cl 

    mov rax, r8;  sub rax, [rip + .L_P0]; mov [rdi + 0],  rax
    mov rax, r9;  sbb rax, [rip + .L_P1]; mov [rdi + 8],  rax
    mov rax, r10; sbb rax, [rip + .L_P2]; mov [rdi + 16], rax
    mov rax, r11; sbb rax, [rip + .L_P3]; mov [rdi + 24], rax
    mov rax, r12; sbb rax, [rip + .L_P4]; mov [rdi + 32], rax
    mov rax, r13; sbb rax, [rip + .L_P5]; mov [rdi + 40], rax
    mov rax, r14; sbb rax, [rip + .L_P6]; mov [rdi + 48], rax
    mov rax, r15; sbb rax, [rip + .L_P7]; mov [rdi + 56], rax
    
    sbb rcx, 0 

    mov rax, [rdi + 0];  cmovc rax, r8;  mov [rdi + 0],  rax
    mov rax, [rdi + 8];  cmovc rax, r9;  mov [rdi + 8],  rax
    mov rax, [rdi + 16]; cmovc rax, r10; mov [rdi + 16], rax
    mov rax, [rdi + 24]; cmovc rax, r11; mov [rdi + 24], rax
    mov rax, [rdi + 32]; cmovc rax, r12; mov [rdi + 32], rax
    mov rax, [rdi + 40]; cmovc rax, r13; mov [rdi + 40], rax
    mov rax, [rdi + 48]; cmovc rax, r14; mov [rdi + 48], rax
    mov rax, [rdi + 56]; cmovc rax, r15; mov [rdi + 56], rax

    pop r15; pop r14; pop r13; pop r12
    ret


fiat_p511_sub_asm:
    push rbp; push rbx; push r12; push r13; push r14; push r15

    mov r8,  [rsi + 0];  sub r8,  [rdx + 0]
    mov r9,  [rsi + 8];  sbb r9,  [rdx + 8]
    mov r10, [rsi + 16]; sbb r10, [rdx + 16]
    mov r11, [rsi + 24]; sbb r11, [rdx + 24]
    mov r12, [rsi + 32]; sbb r12, [rdx + 32]
    mov r13, [rsi + 40]; sbb r13, [rdx + 40]
    mov r14, [rsi + 48]; sbb r14, [rdx + 48]
    mov r15, [rsi + 56]; sbb r15, [rdx + 56]
    
    sbb rax, rax 

    mov rcx, [rip + .L_P4]; and rcx, rax
    mov rbx, [rip + .L_P5]; and rbx, rax
    mov rbp, [rip + .L_P6]; and rbp, rax
    mov rsi, [rip + .L_P7]; and rsi, rax

    add r8, rax; adc r9, rax; adc r10, rax; adc r11, rax
    adc r12, rcx; adc r13, rbx; adc r14, rbp; adc r15, rsi
    
    mov [rdi + 0],  r8;  mov [rdi + 8],  r9;  mov [rdi + 16], r10; mov [rdi + 24], r11
    mov [rdi + 32], r12; mov [rdi + 40], r13; mov [rdi + 48], r14; mov [rdi + 56], r15
    
    pop r15; pop r14; pop r13; pop r12; pop rbx; pop rbp
    ret


.macro FPMUL_ROUND offset, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9
    mov rdx, [rsp]
    mov rdx, [rdx + \offset*8]   
    xor rax, rax                 
    mulx rcx, rbx, [rdi + 0*8];  adox \t0, rbx;  adcx \t1, rcx 
    mulx rcx, rbx, [rdi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rdi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rdi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mulx rcx, rbx, [rdi + 4*8];  adox \t4, rbx;  adcx \t5, rcx
    mulx rcx, rbx, [rdi + 5*8];  adox \t5, rbx;  adcx \t6, rcx
    mulx rcx, rbx, [rdi + 6*8];  adox \t6, rbx;  adcx \t7, rcx
    mulx rcx, rbx, [rdi + 7*8];  adox \t7, rbx;  adcx \t8, rcx
    mov rbx, 0;                  adox \t8, rbx;  adcx \t9, rbx;  adox \t9, rbx

    mov rdx, \t0                 
    xor rax, rax
    mulx rcx, rbx, [rip + .L_P4_p1]; adox \t4, rbx; adcx \t5, rcx
    mulx rcx, rbx, [rip + .L_P5_p1]; adox \t5, rbx; adcx \t6, rcx
    mulx rcx, rbx, [rip + .L_P6_p1]; adox \t6, rbx; adcx \t7, rcx
    mulx rcx, rbx, [rip + .L_P7_p1]; adox \t7, rbx; adcx \t8, rcx
    
    mov rbx, 0;                      adox \t8, rbx; adcx \t9, rbx; adox \t9, rbx
    mov \t0, 0                   
.endm


fiat_p511_mul_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 16
    mov [rsp + 8], rdi 
    mov [rsp + 0], rsi 
    mov rdi, rdx    

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; 
    xor r13, r13; xor r14, r14; xor r15, r15; xor rbp, rbp; xor rsi, rsi

    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14, r15, rbp, rsi
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r15, rbp, rsi, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r14, r15, rbp, rsi, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r14, r15, rbp, rsi, r8,  r9,  r10
    FPMUL_ROUND 4, r12, r13, r14, r15, rbp, rsi, r8,  r9,  r10, r11
    FPMUL_ROUND 5, r13, r14, r15, rbp, rsi, r8,  r9,  r10, r11, r12
    FPMUL_ROUND 6, r14, r15, rbp, rsi, r8,  r9,  r10, r11, r12, r13
    FPMUL_ROUND 7, r15, rbp, rsi, r8,  r9,  r10, r11, r12, r13, r14

    mov rax, rbp; sub rax, [rip + .L_P0]
    mov rbx, rsi; sbb rbx, [rip + .L_P1]
    mov rcx, r8;  sbb rcx, [rip + .L_P2]
    mov rdx, r9;  sbb rdx, [rip + .L_P3]

    push r10; push r11; push r12; push r13
    
    sbb r10, [rip + .L_P4]
    sbb r11, [rip + .L_P5]
    sbb r12, [rip + .L_P6]
    sbb r13, [rip + .L_P7]

    sbb r14, 0 

    cmovnc rbp, rax
    cmovnc rsi, rbx
    cmovnc r8,  rcx
    cmovnc r9,  rdx

    pop rax; pop rbx; pop rcx; pop rdx
    
    cmovnc rdx, r10 
    cmovnc rcx, r11
    cmovnc rbx, r12 
    cmovnc rax, r13

    mov rdi, [rsp + 8] 
    mov [rdi + 0*8], rbp
    mov [rdi + 1*8], rsi
    mov [rdi + 2*8], r8
    mov [rdi + 3*8], r9
    mov [rdi + 4*8], rdx
    mov [rdi + 5*8], rcx
    mov [rdi + 6*8], rbx
    mov [rdi + 7*8], rax

    add rsp, 16
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p511_square_asm:
    mov rdx, rsi 
    jmp fiat_p511_mul_asm
.section .note.GNU-stack,"",@progbits
