// fp255_add.s

.intel_syntax noprefix
.text


// void fiat_p271_add_asm(uint64_t *out, const uint64_t *arg1, const uint64_t *arg2);
.global fiat_p271_add_asm
fiat_p271_add_asm:
    push r12
    push r13
    push r14
    push r15

    mov r8,  [rsi + 0]
    add r8,  [rdx + 0]
    
    mov r9,  [rsi + 8]
    adc r9,  [rdx + 8]
    
    mov r10, [rsi + 16]
    adc r10, [rdx + 16]
    
    mov r11, [rsi + 24]
    adc r11, [rdx + 24]
    
    mov r12, [rsi + 32]
    adc r12, [rdx + 32]
    
    setc r13b
    movzx r13, r13b
    
    mov rax, r8
    sub rax, -1             
    
    mov rcx, r9
    sbb rcx, -1             
    
    mov r14, 0x4c9e5305185121ff
    mov r15, r10
    sbb r15, r14          
    
    mov r14, 0x3d19960596fb1e8c
    mov rdx, r11            
    sbb rdx, r14            
    
    mov r14, 0x465e
    mov rsi, r12         
    sbb rsi, r14           

    sbb r13, 0
    
    cmovnc r8, rax
    cmovnc r9, rcx
    cmovnc r10, r15
    cmovnc r11, rdx
    cmovnc r12, rsi

    mov [rdi + 0],  r8
    mov [rdi + 8],  r9
    mov [rdi + 16], r10
    mov [rdi + 24], r11
    mov [rdi + 32], r12

    pop r15
    pop r14
    pop r13
    pop r12
    ret


// void fiat_p271_sub_asm(uint64_t *out, const uint64_t *arg1, const uint64_t *arg2);
.global fiat_p271_sub_asm
fiat_p271_sub_asm:
    push r12

    mov r8,  [rsi + 0]
    sub r8,  [rdx + 0]      

    mov r9,  [rsi + 8]
    sbb r9,  [rdx + 8]     
    
    mov r10, [rsi + 16]
    sbb r10, [rdx + 16]
    
    mov r11, [rsi + 24]
    sbb r11, [rdx + 24]
    
    mov r12, [rsi + 32]
    sbb r12, [rdx + 32]     

    sbb rax, rax            
    
    mov rcx, 0x4c9e5305185121ff  
    and rcx, rax                 
    
    mov rsi, 0x3d19960596fb1e8c  
    and rsi, rax                 

    mov rdx, 0x465e              
    and rdx, rax                 
    
    add r8,  rax
    adc r9,  rax
    adc r10, rcx
    adc r11, rsi
    adc r12, rdx

    mov [rdi + 0],  r8
    mov [rdi + 8],  r9
    mov [rdi + 16], r10
    mov [rdi + 24], r11
    mov [rdi + 32], r12

    pop r12
    ret


.global fiat_p271_mul_asm
.global fiat_p271_square_asm

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

fiat_p271_mul_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    
    mov r14, rdi
    mov r15, rsi
    mov rdi, rdx
    
    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13; xor rbp, rbp

    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13, rbp
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, rbp, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, rbp, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, rbp, r8,  r9,  r10
    FPMUL_ROUND 4, r12, r13, rbp, r8,  r9,  r10, r11

    mov rax, r13
    mov rbx, rbp
    mov rcx, r8
    mov rdx, r9
    mov r15, r10 

    sub rax, [rip + .L_P0]
    sbb rbx, [rip + .L_P1]
    sbb rcx, [rip + .L_P2]
    sbb rdx, [rip + .L_P3]
    sbb r15, [rip + .L_P4]

    cmovnc r13, rax
    cmovnc rbp, rbx
    cmovnc r8,  rcx
    cmovnc r9,  rdx
    cmovnc r10, r15

    mov [r14 + 0],  r13
    mov [r14 + 8],  rbp
    mov [r14 + 16], r8
    mov [r14 + 24], r9
    mov [r14 + 32], r10

    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p271_square_asm:
    mov rdx, rsi 
    jmp fiat_p271_mul_asm 


.section .note.GNU-stack,"",@progbits
