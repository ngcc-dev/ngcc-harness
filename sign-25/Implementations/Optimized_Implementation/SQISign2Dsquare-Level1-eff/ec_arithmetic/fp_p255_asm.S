// fp_p255_asm.s

.intel_syntax noprefix
.text

.global fiat_p255_add_asm
.global fiat_p255_sub_asm
.global fiat_p255_mul_asm
.global fiat_p255_square_asm
.global fiat_p255_from_montgomery_asm
.global fiat_p255_to_montgomery_asm



// void fiat_p255_add_asm(uint64_t *out, const uint64_t *arg1, const uint64_t *arg2);
fiat_p255_add_asm:
    mov r8,  [rsi + 0]      
    add r8,  [rdx + 0]       
    
    mov r9,  [rsi + 8]
    adc r9,  [rdx + 8]      
    
    mov r10, [rsi + 16]
    adc r10, [rdx + 16]
    
    mov r11, [rsi + 24]
    adc r11, [rdx + 24]

    setc byte ptr [rsp - 1]
    
    mov rax, r8
    sub rax, -1
    
    mov rcx, r9
    sbb rcx, -1
    
    mov rsi, r10
    sbb rsi, [rip + .L_P2]
    
    mov rdx, r11
    sbb rdx, [rip + .L_P3]

    sbb byte ptr [rsp - 1], 0

    cmovnc r8, rax          
    cmovnc r9, rcx          
    cmovnc r10, rsi          
    cmovnc r11, rdx          

    mov [rdi + 0], r8
    mov [rdi + 8], r9
    mov [rdi + 16], r10
    mov [rdi + 24], r11

    ret


// void fiat_p255_sub_asm(uint64_t *out, const uint64_t *arg1, const uint64_t *arg2);
fiat_p255_sub_asm:
    mov r8,  [rsi + 0]
    sub r8,  [rdx + 0]      
    
    mov r9,  [rsi + 8]
    sbb r9,  [rdx + 8]      
    
    mov r10, [rsi + 16]
    sbb r10, [rdx + 16]
    
    mov r11, [rsi + 24]
    sbb r11, [rdx + 24]    

    sbb rax, rax            

    mov rsi, 0xa7ecc14ec3fa83c7 
    and rsi, rax                 
    
    mov rdx, 0x62d7f37f9815e5fc 
    and rdx, rax                 
    
    add r8,  rax
    adc r9,  rax
    adc r10, rsi
    adc r11, rdx

    mov [rdi + 0],  r8
    mov [rdi + 8],  r9
    mov [rdi + 16], r10
    mov [rdi + 24], r11
    ret


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


fiat_p255_mul_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    mov r14, rdi
    mov r15, rsi
    mov rdi, rdx
    
    xor r8, r8; xor r9, r9; xor r10, r10; xor r11, r11; xor r12, r12; xor r13, r13
    FPMUL_ROUND 0, r8,  r9,  r10, r11, r12, r13
    FPMUL_ROUND 1, r9,  r10, r11, r12, r13, r8
    FPMUL_ROUND 2, r10, r11, r12, r13, r8,  r9
    FPMUL_ROUND 3, r11, r12, r13, r8,  r9,  r10

    REDUCE_P r12, r13, r8, r9

    mov [r14 + 0],  r12
    mov [r14 + 8],  r13
    mov [r14 + 16], r8
    mov [r14 + 24], r9

    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p255_square_asm:
    mov rdx, rsi 
    jmp fiat_p255_mul_asm 


.L_MONT_R2:
    .quad 0xb72f360b899325e6 
    .quad 0xb9b0613be5d0b8bf 
    .quad 0x3b043bcf4d7492ff 
    .quad 0x4939383cef249da9 

fiat_p255_from_montgomery_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    
    mov r8,  [rsi + 0*8]
    mov r9,  [rsi + 1*8]
    mov r10, [rsi + 2*8]
    mov r11, [rsi + 3*8]
    xor r12, r12
    xor r13, r13
    
    mov rdx, r8
    xor rax, rax 
    mulx rcx, rbx, [rip + .L_P2_p1]; adox r10, rbx; adcx r11, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox r11, rbx; adcx r12, rcx
    mov rbx, 0;                      adox r12, rbx; adcx r13, rbx; adox r13, rbx
    
    mov rdx, r9
    xor r14, r14 
    mulx rcx, rbx, [rip + .L_P2_p1]; adox r11, rbx; adcx r12, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox r12, rbx; adcx r13, rcx
    mov rbx, 0;                      adox r13, rbx; adcx r14, rbx; adox r14, rbx
    
    mov rdx, r10
    xor r15, r15 
    mulx rcx, rbx, [rip + .L_P2_p1]; adox r12, rbx; adcx r13, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox r13, rbx; adcx r14, rcx
    mov rbx, 0;                      adox r14, rbx; adcx r15, rbx; adox r15, rbx
    
    mov rdx, r11
    xor r8, r8   
    mulx rcx, rbx, [rip + .L_P2_p1]; adox r13, rbx; adcx r14, rcx
    mulx rcx, rbx, [rip + .L_P3_p1]; adox r14, rbx; adcx r15, rcx
    mov rbx, 0;                      adox r15, rbx; adcx r8, rbx; adox r8, rbx
    
    mov rax, r12; sub rax, [rip + .L_P0] 
    mov rbx, r13; sbb rbx, [rip + .L_P1] 
    mov rcx, r14; sbb rcx, [rip + .L_P2] 
    mov rdx, r15; sbb rdx, [rip + .L_P3] 
    sbb r8, 0     

    cmovnc r12, rax
    cmovnc r13, rbx
    cmovnc r14, rcx
    cmovnc r15, rdx
    
    mov [rdi + 0*8], r12
    mov [rdi + 1*8], r13
    mov [rdi + 2*8], r14
    mov [rdi + 3*8], r15
    
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p255_to_montgomery_asm:
    lea rdx, [rip + .L_MONT_R2]
    
    jmp fiat_p255_mul_asm
.section .note.GNU-stack,"",@progbits
