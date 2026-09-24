.intel_syntax noprefix
.text
.p2align 4

.L_P0:    .quad 0xffffffffffffffff
.L_P1:    .quad 0xffffffffffffffff
.L_P2:    .quad 0x4c9e5305185121ff
.L_P3:    .quad 0x3d19960596fb1e8c
.L_P4:    .quad 0x000000000000465e

.L_P2_p1: .quad 0x4c9e530518512200
.L_P3_p1: .quad 0x3d19960596fb1e8c
.L_P4_p1: .quad 0x000000000000465e

.L_4P_0:  .quad 0xfffffffffffffffc
.L_4P_1:  .quad 0xffffffffffffffff
.L_4P_2:  .quad 0x32794c14614487ff
.L_4P_3:  .quad 0xf46658165bec7a31
.L_4P_4:  .quad 0x0000000000011978

.L_8P_0:  .quad 0xfffffffffffffff8
.L_8P_1:  .quad 0xffffffffffffffff
.L_8P_2:  .quad 0x64f29828c2890fff
.L_8P_3:  .quad 0xe8ccb02cb7d8f462
.L_8P_4:  .quad 0x00000000000232f1

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

.global fp2_mul271_asm
.global fp2_sqr271_asm


.macro FPDBLMUL_ROUND offset, t0, t1, t2, t3, t4, t5, t6
    mov rdx, [r15 + \offset*8]
    xor rax, rax
    mulx rcx, rbx, [rdi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rdi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rdi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rdi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mulx rcx, rbx, [rdi + 4*8];  adox \t4, rbx;  adcx \t5, rcx
    mov rbx, 0;                  adox \t5, rbx;  adcx \t6, rbx;  adox \t6, rbx

    mov rdx, [rbp + \offset*8]
    xor rax, rax
    mulx rcx, rbx, [rsi + 0*8];  adox \t0, rbx;  adcx \t1, rcx
    mulx rcx, rbx, [rsi + 1*8];  adox \t1, rbx;  adcx \t2, rcx
    mulx rcx, rbx, [rsi + 2*8];  adox \t2, rbx;  adcx \t3, rcx
    mulx rcx, rbx, [rsi + 3*8];  adox \t3, rbx;  adcx \t4, rcx
    mulx rcx, rbx, [rsi + 4*8];  adox \t4, rbx;  adcx \t5, rcx
    mov rbx, 0;                  adox \t5, rbx;  adcx \t6, rbx;  adox \t6, rbx

    mov rdx, \t0
    xor rax, rax
    mulx rcx, rbx, [rip + .L_P2_p1]; adox \t2, rbx; adcx \t3, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox \t3, rbx; adcx \t4, rcx
    mulx rcx, rbx, [rip + .L_P4_p1]; adox \t4, rbx; adcx \t5, rcx
    mov rbx, 0;                      adox \t5, rbx; adcx \t6, rbx; adox \t6, rbx
    
    mov \t0, 0
.endm


.macro FINAL_SUB t0, t1, t2, t3, t4
    mov rax, \t0
    mov rbx, \t1
    mov rcx, \t2
    mov rdx, \t3
    mov r11, \t4

    sub rax, [rip + .L_P0]
    sbb rbx, [rip + .L_P1]
    sbb rcx, [rip + .L_P2]
    sbb rdx, [rip + .L_P3]
    sbb r11, [rip + .L_P4]

    cmovnc \t0, rax
    cmovnc \t1, rbx
    cmovnc \t2, rcx
    cmovnc \t3, rdx
    cmovnc \t4, r11
.endm


fp2_mul271_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 88
    mov [rsp + 64], rdi 
    mov [rsp + 72], rsi 
    mov [rsp + 80], rdx 

    mov rdi, rdx
    mov r8,  [rip + .L_8P_0]
    mov r9,  [rip + .L_8P_1]
    mov r10, [rip + .L_8P_2]
    mov r11, [rip + .L_8P_3]
    mov r12, [rip + .L_8P_4]
    sub r8,  [rdi + 40];  mov [rsp + 0],  r8
    sbb r9,  [rdi + 48];  mov [rsp + 8],  r9
    sbb r10, [rdi + 56];  mov [rsp + 16], r10
    sbb r11, [rdi + 64];  mov [rsp + 24], r11
    sbb r12, [rdi + 72];  mov [rsp + 32], r12

    mov r15, [rsp + 72]      
    mov rdi, [rsp + 80]      
    lea rbp, [r15 + 40]      
    lea rsi, [rsp + 0]       

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14
    FPDBLMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14
    FPDBLMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r8
    FPDBLMUL_ROUND 2, r10, r11, r12, r13, r14, r8,  r9
    FPDBLMUL_ROUND 3, r11, r12, r13, r14, r8,  r9,  r10
    FPDBLMUL_ROUND 4, r12, r13, r14, r8,  r9,  r10, r11
    
    FINAL_SUB r13, r14, r8, r9, r10
    mov [rsp + 0],  r13; mov [rsp + 8],  r14; mov [rsp + 16], r8; mov [rsp + 24], r9; mov [rsp + 32], r10

    mov r15, [rsp + 72]      
    mov rsi, [rsp + 80]      
    lea rbp, [r15 + 40]     
    lea rdi, [rsi + 40]      

    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14
    FPDBLMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14
    FPDBLMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r8
    FPDBLMUL_ROUND 2, r10, r11, r12, r13, r14, r8,  r9
    FPDBLMUL_ROUND 3, r11, r12, r13, r14, r8,  r9,  r10
    FPDBLMUL_ROUND 4, r12, r13, r14, r8,  r9,  r10, r11

    FINAL_SUB r13, r14, r8, r9, r10
    
    mov rdi, [rsp + 64]  
    mov [rdi + 40], r13; mov [rdi + 48], r14; mov [rdi + 56], r8; mov [rdi + 64], r9; mov [rdi + 72], r10
    mov r13, [rsp + 0];  mov r14, [rsp + 8];  mov r8,  [rsp + 16]; mov r9,  [rsp + 24]; mov r10, [rsp + 32]
    mov [rdi + 0],  r13; mov [rdi + 8],  r14; mov [rdi + 16], r8;  mov [rdi + 24], r9;  mov [rdi + 32], r10

    add rsp, 88
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fp2_sqr271_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 136
    mov [rsp + 120], rdi 
    mov [rsp + 128], rsi 

    mov r8, [rsi + 0];  add r8, [rsi + 40];  mov [rsp + 0],  r8
    mov r9, [rsi + 8];  adc r9, [rsi + 48];  mov [rsp + 8],  r9
    mov r10,[rsi + 16]; adc r10,[rsi + 56];  mov [rsp + 16], r10
    mov r11,[rsi + 24]; adc r11,[rsi + 64];  mov [rsp + 24], r11
    mov r12,[rsi + 32]; adc r12,[rsi + 72];  mov [rsp + 32], r12

    mov r8, [rsi + 0];  sub r8, [rsi + 40]
    mov r9, [rsi + 8];  sbb r9, [rsi + 48]
    mov r10,[rsi + 16]; sbb r10,[rsi + 56]
    mov r11,[rsi + 24]; sbb r11,[rsi + 64]
    mov r12,[rsi + 32]; sbb r12,[rsi + 72]

    add r8, [rip + .L_4P_0]; mov [rsp + 40], r8
    adc r9, [rip + .L_4P_1]; mov [rsp + 48], r9
    adc r10,[rip + .L_4P_2]; mov [rsp + 56], r10
    adc r11,[rip + .L_4P_3]; mov [rsp + 64], r11
    adc r12,[rip + .L_4P_4]; mov [rsp + 72], r12

    mov r8, [rsi + 0];  add r8, r8;          mov [rsp + 80], r8
    mov r9, [rsi + 8];  adc r9, r9;          mov [rsp + 88], r9
    mov r10,[rsi + 16]; adc r10, r10;        mov [rsp + 96], r10
    mov r11,[rsi + 24]; adc r11, r11;        mov [rsp + 104],r11
    mov r12,[rsi + 32]; adc r12, r12;        mov [rsp + 112],r12

    lea r15, [rsp + 0]       
    lea rdi, [rsp + 40]      
    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14
    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r14, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r14, r8,  r9,  r10
    FPMUL_ROUND 4, r12, r13, r14, r8,  r9,  r10, r11
    
    FINAL_SUB r13, r14, r8, r9, r10
    mov [rsp + 0], r13; mov [rsp + 8], r14; mov [rsp + 16], r8; mov [rsp + 24], r9; mov [rsp + 32], r10

    lea r15, [rsp + 80]      
    mov rsi, [rsp + 128]
    lea rdi, [rsi + 40]      
    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor r14, r14
    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, r14
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r14, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r14, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r14, r8,  r9,  r10
    FPMUL_ROUND 4, r12, r13, r14, r8,  r9,  r10, r11

    FINAL_SUB r13, r14, r8, r9, r10
    
    mov rdi, [rsp + 120] 
    mov [rdi + 40], r13; mov [rdi + 48], r14; mov [rdi + 56], r8; mov [rdi + 64], r9; mov [rdi + 72], r10
    mov r13, [rsp + 0];  mov r14, [rsp + 8];  mov r8,  [rsp + 16]; mov r9,  [rsp + 24]; mov r10, [rsp + 32]
    mov [rdi + 0],  r13; mov [rdi + 8],  r14; mov [rdi + 16], r8;  mov [rdi + 24], r9;  mov [rdi + 32], r10

    add rsp, 136
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret

.section .note.GNU-stack,"",@progbits
