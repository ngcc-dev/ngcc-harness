// fp_core1024.s

.intel_syntax noprefix
.global fiat_p1024_add_asm
.global fiat_p1024_sub_asm
.global fiat_p1024_mul_asm
.global fiat_p1024_square_asm

.text
.p2align 5


.L_P_TABLE:
    .quad 0xffffffffffffffff 
    .quad 0xffffffffffffffff 
    .quad 0xffffffffffffffff 
    .quad 0xffffffffffffffff 
    .quad 0xffffffffffffffff 
    .quad 0xffffffffffffffff 
    .quad 0xffffffffffffffff 
    .quad 0xffffffffffffffff 
    .quad 0x644ea17826cb7963 
    .quad 0xbe9a1c1509209a83 
    .quad 0x44919b54f24e1f63 
    .quad 0x8e8887d93a4d0293 
    .quad 0xea7a411c54b01504 
    .quad 0x03fc8db5315de69a 
    .quad 0x355ad1adc6eb4970 
    .quad 0x85aa094b24b141fc 

.L_P_P1_TABLE:
    .quad 0x644ea17826cb7964 
    .quad 0xbe9a1c1509209a83 
    .quad 0x44919b54f24e1f63
    .quad 0x8e8887d93a4d0293 
    .quad 0xea7a411c54b01504 
    .quad 0x03fc8db5315de69a 
    .quad 0x355ad1adc6eb4970 
    .quad 0x85aa094b24b141fc 


fiat_p1024_add_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 128 

    .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
        mov rax, [rsi + \k*8]
        .if \k == 0
            add rax, [rdx + \k*8]
        .else
            adc rax, [rdx + \k*8]
        .endif
        mov [rsp + \k*8], rax
    .endr
    setc cl

    lea r12, [rip + .L_P_TABLE] 

    .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
        mov rax, [rsp + \k*8]
        .if \k == 0
            sub rax, [r12 + \k*8]
        .else
            sbb rax, [r12 + \k*8]
        .endif
        mov [rdi + \k*8], rax
    .endr

    movzx rcx, cl
    sbb rcx, 0 

    .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
        mov rax, [rdi + \k*8]
        mov rbx, [rsp + \k*8]
        cmovc rax, rbx
        mov [rdi + \k*8], rax
    .endr

    add rsp, 128
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p1024_sub_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 128

    .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
        mov rax, [rsi + \k*8]
        .if \k == 0
            sub rax, [rdx + \k*8]
        .else
            sbb rax, [rdx + \k*8]
        .endif
        mov [rsp + \k*8], rax
    .endr

    sbb rax, rax 
    lea r12, [rip + .L_P_TABLE] 

    mov rcx, [r12 + 8*8];  and rcx, rax
    mov rbx, [r12 + 9*8];  and rbx, rax
    mov rbp, [r12 + 10*8]; and rbp, rax
    mov r8,  [r12 + 11*8]; and r8,  rax
    mov r9,  [r12 + 12*8]; and r9,  rax
    mov r10, [r12 + 13*8]; and r10, rax
    mov r11, [r12 + 14*8]; and r11, rax
    mov r12, [r12 + 15*8]; and r12, rax 

    .irp k, 0, 1, 2, 3, 4, 5, 6, 7
        mov r13, [rsp + \k*8]
        .if \k == 0
            add r13, rax
        .else
            adc r13, rax
        .endif
        mov [rdi + \k*8], r13
    .endr

    mov r13, [rsp + 8*8];  adc r13, rcx; mov [rdi + 8*8],  r13
    mov r13, [rsp + 9*8];  adc r13, rbx; mov [rdi + 9*8],  r13
    mov r13, [rsp + 10*8]; adc r13, rbp; mov [rdi + 10*8], r13
    mov r13, [rsp + 11*8]; adc r13, r8;  mov [rdi + 11*8], r13
    mov r13, [rsp + 12*8]; adc r13, r9;  mov [rdi + 12*8], r13
    mov r13, [rsp + 13*8]; adc r13, r10; mov [rdi + 13*8], r13
    mov r13, [rsp + 14*8]; adc r13, r11; mov [rdi + 14*8], r13
    mov r13, [rsp + 15*8]; adc r13, r12; mov [rdi + 15*8], r13

    add rsp, 128
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p1024_mul_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    
    sub rsp, 272

    mov r14, rdi 
    mov r15, rsi
    mov rdi, rdx 
    lea r13, [rip + .L_P_P1_TABLE] 

    xor rax, rax
    .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
        mov [rsp + \k*8], rax
    .endr

    .irp i, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15

        lea rbp, [rsp + \i*8] 

        mov rdx, [r15 + \i*8] 
        xor rax, rax          
        
        .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
            mulx r8, r9, [rdi + \k*8]
            .if \k == 0
                adox r9, [rbp + \k*8]
                mov [rbp + \k*8], r9
                mov rcx, r8
            .else
                adcx r9, rcx
                adox r9, [rbp + \k*8]
                mov [rbp + \k*8], r9
                mov rcx, r8
            .endif
        .endr
        
        mov r9, 0
        adcx r9, rcx
        adox r9, [rbp + 128] 
        mov [rbp + 128], r9
        
        mov r9, 0
        adcx r9, r9
        adox r9, [rbp + 136] 
        mov [rbp + 136], r9

        mov rdx, [rbp + 0] 
        xor rax, rax 

        .irp k, 0, 1, 2, 3, 4, 5, 6, 7
            mulx r8, r9, [r13 + \k*8]
            .if \k == 0
                adox r9, [rbp + 64 + \k*8] 
                mov [rbp + 64 + \k*8], r9
                mov rcx, r8
            .else
                adcx r9, rcx
                adox r9, [rbp + 64 + \k*8]
                mov [rbp + 64 + \k*8], r9
                mov rcx, r8
            .endif
        .endr
        
        mov r9, 0
        adcx r9, rcx
        adox r9, [rbp + 128]
        mov [rbp + 128], r9
        
        mov r9, 0
        adcx r9, r9
        adox r9, [rbp + 136]
        mov [rbp + 136], r9
    .endr 

    lea r12, [rip + .L_P_TABLE]

    .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
        mov rax, [rsp + 128 + \k*8] 
        .if \k == 0
            sub rax, [r12 + \k*8]
        .else
            sbb rax, [r12 + \k*8]
        .endif
        mov [rsp + \k*8], rax 
    .endr

    mov rax, [rsp + 256] 
    sbb rax, 0 

    .irp k, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
        mov rax, [rsp + \k*8]    
        mov rbx, [rsp + 128 + \k*8] 
        cmovc rax, rbx
        mov [r14 + \k*8], rax
    .endr

    add rsp, 272
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p1024_square_asm:
    mov rdx, rsi 
    jmp fiat_p1024_mul_asm
.section .note.GNU-stack,"",@progbits
