.intel_syntax noprefix
.global fp2_mul332_asm
.global fp2_sqr332_asm

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

.L_4P_0:  .quad 0xfffffffffffffffc
.L_4P_1:  .quad 0xffffffffffffffff
.L_4P_2:  .quad 0xea37c3ffffffffff
.L_4P_3:  .quad 0x0398c2a490a03ed8
.L_4P_4:  .quad 0x43345d3891cc870e
.L_4P_5:  .quad 0x0000000000002e8c

.L_8P_0:  .quad 0xfffffffffffffff8
.L_8P_1:  .quad 0xffffffffffffffff
.L_8P_2:  .quad 0xd46f87ffffffffff
.L_8P_3:  .quad 0x0731854921407db1
.L_8P_4:  .quad 0x8668ba7123990e1c
.L_8P_5:  .quad 0x0000000000005d18

.macro FPDBLMUL_ROUND offset, t0, t1, t2, t3, t4, t5, t6, t7
    mov rdx, [rbp + \offset*8]
    xor rax, rax
    mulx rcx, rbx, [rdi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rdi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rdi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rdi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mulx rcx, rbx, [rdi + 4*8];  adox \t4, rbx;  adcx \t5, rcx
    mulx rcx, rbx, [rdi + 5*8];  adox \t5, rbx;  adcx \t6, rcx
    mov rbx, 0;                  adox \t6, rbx;  adcx \t7, rbx;  adox \t7, rbx

    mov rdx, [rbp + 48 + \offset*8]
    xor rax, rax
    mulx rcx, rbx, [rsi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rsi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rsi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rsi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mulx rcx, rbx, [rsi + 4*8];  adox \t4, rbx;  adcx \t5, rcx
    mulx rcx, rbx, [rsi + 5*8];  adox \t5, rbx;  adcx \t6, rcx
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

.macro FINAL_SUB t0, t1, t2, t3, t4, t5
    mov rax, \t0; mov rbx, \t1; mov rcx, \t2
    mov rdx, \t3; mov r12, \t4; mov r13, \t5

    sub rax, [rip + .L_P0]
    sbb rbx, [rip + .L_P1]
    sbb rcx, [rip + .L_P2]
    sbb rdx, [rip + .L_P3]
    sbb r12, [rip + .L_P4]
    sbb r13, [rip + .L_P5]

    cmovnc \t0, rax; cmovnc \t1, rbx; cmovnc \t2, rcx
    cmovnc \t3, rdx; cmovnc \t4, r12; cmovnc \t5, r13
.endm

fp2_mul332_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 104
    mov [rsp + 96], rdi

    mov r8,  [rip + .L_8P_0]; sub r8,  [rdx + 48]; mov [rsp + 0],  r8
    mov r9,  [rip + .L_8P_1]; sbb r9,  [rdx + 56]; mov [rsp + 8],  r9
    mov r10, [rip + .L_8P_2]; sbb r10, [rdx + 64]; mov [rsp + 16], r10
    mov r11, [rip + .L_8P_3]; sbb r11, [rdx + 72]; mov [rsp + 24], r11
    mov r12, [rip + .L_8P_4]; sbb r12, [rdx + 80]; mov [rsp + 32], r12
    mov r13, [rip + .L_8P_5]; sbb r13, [rdx + 88]; mov [rsp + 40], r13

    mov rbp, rsi
    mov rdi, rdx
    lea rsi, [rsp + 0]

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14; xor r15, r15
    FPDBLMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14, r15
    FPDBLMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r15, r8
    FPDBLMUL_ROUND 2, r10, r11, r12, r13, r14, r15, r8,  r9
    FPDBLMUL_ROUND 3, r11, r12, r13, r14, r15, r8,  r9,  r10
    FPDBLMUL_ROUND 4, r12, r13, r14, r15, r8,  r9,  r10, r11
    FPDBLMUL_ROUND 5, r13, r14, r15, r8,  r9,  r10, r11, r12

    FINAL_SUB r14, r15, r8, r9, r10, r11

    mov [rsp + 48], r14; mov [rsp + 56], r15; mov [rsp + 64], r8
    mov [rsp + 72], r9;  mov [rsp + 80], r10; mov [rsp + 88], r11

    mov rsi, rdi
    lea rdi, [rdi + 48]

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14; xor r15, r15
    FPDBLMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14, r15
    FPDBLMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r15, r8
    FPDBLMUL_ROUND 2, r10, r11, r12, r13, r14, r15, r8,  r9
    FPDBLMUL_ROUND 3, r11, r12, r13, r14, r15, r8,  r9,  r10
    FPDBLMUL_ROUND 4, r12, r13, r14, r15, r8,  r9,  r10, r11
    FPDBLMUL_ROUND 5, r13, r14, r15, r8,  r9,  r10, r11, r12

    FINAL_SUB r14, r15, r8, r9, r10, r11

    mov rdi, [rsp + 96]
    mov [rdi + 48], r14; mov [rdi + 56], r15; mov [rdi + 64], r8
    mov [rdi + 72], r9;  mov [rdi + 80], r10; mov [rdi + 88], r11
    mov r14, [rsp + 48]; mov r15, [rsp + 56]; mov r8,  [rsp + 64]
    mov r9,  [rsp + 72]; mov r10, [rsp + 80]; mov r11, [rsp + 88]
    mov [rdi + 0],  r14; mov [rdi + 8],  r15; mov [rdi + 16], r8
    mov [rdi + 24], r9;  mov [rdi + 32], r10; mov [rdi + 40], r11

    add rsp, 104
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret

fp2_sqr332_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 96

    mov r12, rdi
    mov r13, rsi

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 48]
    call fiat_p332_add_asm

    lea rdi, [rsp + 48]
    mov rsi, r13
    lea rdx, [r13 + 48]
    call fiat_p332_sub_asm

    lea rdi, [r12 + 48]
    mov rsi, r13
    lea rdx, [r13 + 48]
    call fiat_p332_mul_asm

    lea rdi, [r12 + 48]
    lea rsi, [r12 + 48]
    lea rdx, [r12 + 48]
    call fiat_p332_add_asm

    mov rdi, r12
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 48]
    call fiat_p332_mul_asm

    add rsp, 96
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
