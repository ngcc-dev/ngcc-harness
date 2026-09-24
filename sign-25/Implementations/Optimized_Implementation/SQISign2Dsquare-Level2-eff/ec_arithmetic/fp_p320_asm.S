.intel_syntax noprefix
.global fiat_p320_add_asm
.global fiat_p320_sub_asm

.text
.p2align 4


fiat_p320_add_asm:
    mov r8,  [rsi + 0]
    add r8,  [rdx + 0]

    mov r9,  [rsi + 8]
    adc r9,  [rdx + 8]

    mov r10, [rsi + 16]
    adc r10, [rdx + 16]

    mov r11, [rsi + 24]
    adc r11, [rdx + 24]

    mov rax, [rsi + 32]
    adc rax, [rdx + 32]

    setc cl
    movzx rcx, cl

    mov [rdi + 0],  r8
    mov [rdi + 8],  r9
    mov [rdi + 16], r10
    mov [rdi + 24], r11
    mov [rdi + 32], rax

    sub r8, -1
    sbb r9, -1

    mov rsi, 0xb034db79ffffffff
    sbb r10, rsi

    mov rsi, 0xb07b3ed88b9b99d
    sbb r11, rsi

    mov rsi, 0x87f825b5984d21e4
    sbb rax, rsi

    sbb rcx, 0
    setnc dl
    movzx rdx, dl
    neg rdx

    mov rsi, [rdi + 0]
    xor r8, rsi
    and r8, rdx
    xor r8, rsi

    mov rsi, [rdi + 8]
    xor r9, rsi
    and r9, rdx
    xor r9, rsi

    mov rsi, [rdi + 16]
    xor r10, rsi
    and r10, rdx
    xor r10, rsi

    mov rsi, [rdi + 24]
    xor r11, rsi
    and r11, rdx
    xor r11, rsi

    mov rsi, [rdi + 32]
    xor rax, rsi
    and rax, rdx
    xor rax, rsi

    mov [rdi + 0],  r8
    mov [rdi + 8],  r9
    mov [rdi + 16], r10
    mov [rdi + 24], r11
    mov [rdi + 32], rax
    ret

fiat_p320_sub_asm:
    push rbx

    mov r8,  [rsi + 0]
    sub r8,  [rdx + 0]

    mov r9,  [rsi + 8]
    sbb r9,  [rdx + 8]

    mov r10, [rsi + 16]
    sbb r10, [rdx + 16]

    mov r11, [rsi + 24]
    sbb r11, [rdx + 24]

    mov rax, [rsi + 32]
    sbb rax, [rdx + 32]

    sbb rcx, rcx

    mov rsi, 0xb034db79ffffffff
    and rsi, rcx

    mov rdx, 0xb07b3ed88b9b99d
    and rdx, rcx

    mov rbx, 0x87f825b5984d21e4
    and rbx, rcx

    add r8,  rcx
    adc r9,  rcx
    adc r10, rsi
    adc r11, rdx
    adc rax, rbx

    mov [rdi + 0],  r8
    mov [rdi + 8],  r9
    mov [rdi + 16], r10
    mov [rdi + 24], r11
    mov [rdi + 32], rax

    pop rbx
    ret


.global fiat_p320_mul_asm
.global fiat_p320_square_asm

.text
.p2align 4

.L_P0:    .quad 0xffffffffffffffff
.L_P1:    .quad 0xffffffffffffffff
.L_P2:    .quad 0xb034db79ffffffff
.L_P3:    .quad 0xb07b3ed88b9b99d
.L_P4:    .quad 0x87f825b5984d21e4

.L_P2_p1: .quad 0xb034db7a00000000
.L_P3_p1: .quad 0xb07b3ed88b9b99d
.L_P4_p1: .quad 0x87f825b5984d21e4


.macro FPMUL_ROUND offset, t0, t1, t2, t3, t4, t5, t6
    mov rdx, [r15 + \offset*8]   
    xor rax, rax                 
    mulx rcx, rbx, [rdi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rdi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rdi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rdi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mulx rcx, rbx, [rdi + 4*8];  adox \t4, rbx;  adcx \t5, rcx
    mov rbx, 0;                  adox \t5, rbx;  adcx \t6, rbx;  adox \t6, rbx

    mov rdx, \t0                 
    xor rax, rax
    mulx rcx, rbx, [rip + .L_P2_p1]; adox \t2, rbx; adcx \t3, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox \t3, rbx; adcx \t4, rcx
    mulx rcx, rbx, [rip + .L_P4_p1]; adox \t4, rbx; adcx \t5, rcx
    

    mov rbx, 0;                      adox \t5, rbx; adcx \t6, rbx; adox \t6, rbx
    
    mov \t0, 0                   
.endm


fiat_p320_mul_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    push rdi                

    mov r15, rsi            
    mov rdi, rdx             

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14

    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r14, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r14, r8,  r9,  r10
    FPMUL_ROUND 4, r12, r13, r14, r8,  r9,  r10, r11

    mov rax, r13
    mov rbx, r14
    mov rcx, r8
    mov rdx, r9
    mov r15, r10

    sub rax, [rip + .L_P0]
    sbb rbx, [rip + .L_P1]
    sbb rcx, [rip + .L_P2]
    sbb rdx, [rip + .L_P3]
    sbb r15, [rip + .L_P4]
    
    sbb r11, 0  

    cmovnc r13, rax
    cmovnc r14, rbx
    cmovnc r8,  rcx
    cmovnc r9,  rdx
    cmovnc r10, r15

    pop rdi                 
    mov [rdi + 0],  r13
    mov [rdi + 8],  r14
    mov [rdi + 16], r8
    mov [rdi + 24], r9
    mov [rdi + 32], r10

    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret

fiat_p320_square_asm:
    mov rdx, rsi 
    jmp fiat_p320_mul_asm 



.section .note.GNU-stack,"",@progbits
