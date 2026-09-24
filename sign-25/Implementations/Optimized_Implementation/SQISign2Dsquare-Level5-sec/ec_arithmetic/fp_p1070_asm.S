// fp_core1070.s

.intel_syntax noprefix
.global fiat_p1070_add_asm
.global fiat_p1070_sub_asm
.global fiat_p1070_mul_asm
.global fiat_p1070_square_asm

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
    .quad 0x96050cec08ffffff 
    .quad 0x8c6c5c2ba227f9e7 
    .quad 0xc2a05a2a0a257938
    .quad 0x8180a5e0759c1a94 
    .quad 0x81e705170e1fdcd2 
    .quad 0x40ef78c31b591dff 
    .quad 0xe09b2c26a9480ac2 
    .quad 0x6ca040134d002c46 
    .quad 0x00002c2b13106aca 

.L_P_P1_TABLE:
    .quad 0x96050cec09000000 
    .quad 0x8c6c5c2ba227f9e7 
    .quad 0xc2a05a2a0a257938 
    .quad 0x8180a5e0759c1a94 
    .quad 0x81e705170e1fdcd2 
    .quad 0x40ef78c31b591dff 
    .quad 0xe09b2c26a9480ac2 
    .quad 0x6ca040134d002c46 
    .quad 0x00002c2b13106aca 


fiat_p1070_add_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 136 

    .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
        mov rax, [rsi + (\k*8)]
        .if \k == 0
            add rax, [rdx + (\k*8)]
        .else
            adc rax, [rdx + (\k*8)]
        .endif
        mov [rsp + (\k*8)], rax
    .endr
    setc cl

    lea r12, [rip + .L_P_TABLE]
    .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
        mov rax, [rsp + (\k*8)]
        .if \k == 0
            sub rax, [r12 + (\k*8)]
        .else
            sbb rax, [r12 + (\k*8)]
        .endif
        mov [rdi + (\k*8)], rax
    .endr

    movzx rcx, cl
    sbb rcx, 0 
    .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
        mov rax, [rdi + (\k*8)]
        mov rbx, [rsp + (\k*8)]
        cmovc rax, rbx
        mov [rdi + (\k*8)], rax
    .endr

    add rsp, 136
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p1070_sub_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 136

    .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
        mov rax, [rsi + (\k*8)]
        .if \k == 0
            sub rax, [rdx + (\k*8)]
        .else
            sbb rax, [rdx + (\k*8)]
        .endif
        mov [rsp + (\k*8)], rax
    .endr

    sbb rax, rax 
    lea rsi, [rip + .L_P_TABLE] 

    mov rcx, [rsi + 8*8];  and rcx, rax
    mov rbx, [rsi + 9*8];  and rbx, rax
    mov rbp, [rsi + 10*8]; and rbp, rax
    mov r8,  [rsi + 11*8]; and r8,  rax
    mov r9,  [rsi + 12*8]; and r9,  rax
    mov r10, [rsi + 13*8]; and r10, rax
    mov r11, [rsi + 14*8]; and r11, rax
    mov r12, [rsi + 15*8]; and r12, rax
    mov rdx, [rsi + 16*8]; and rdx, rax 

    .irp k, 0,1,2,3,4,5,6,7
        mov r13, [rsp + (\k*8)]
        .if \k == 0
            add r13, rax
        .else
            adc r13, rax
        .endif
        mov [rdi + (\k*8)], r13
    .endr

    mov r13, [rsp + 8*8];  adc r13, rcx; mov [rdi + 8*8], r13
    mov r13, [rsp + 9*8];  adc r13, rbx; mov [rdi + 9*8], r13
    mov r13, [rsp + 10*8]; adc r13, rbp; mov [rdi + 10*8], r13
    mov r13, [rsp + 11*8]; adc r13, r8;  mov [rdi + 11*8], r13
    mov r13, [rsp + 12*8]; adc r13, r9;  mov [rdi + 12*8], r13
    mov r13, [rsp + 13*8]; adc r13, r10; mov [rdi + 13*8], r13
    mov r13, [rsp + 14*8]; adc r13, r11; mov [rdi + 14*8], r13
    mov r13, [rsp + 15*8]; adc r13, r12; mov [rdi + 15*8], r13
    mov r13, [rsp + 16*8]; adc r13, rdx; mov [rdi + 16*8], r13

    add rsp, 136
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret


fiat_p1070_mul_asm:
    push rbx; push rbp; push r12; push r13; push r14; push r15
    sub rsp, 288 

    mov r14, rdi 
    mov r15, rsi 
    mov rdi, rdx 
    lea r13, [rip + .L_P_P1_TABLE]

    xor rax, rax
    .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35
        mov [rsp + (\k*8)], rax
    .endr

    .irp i, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
        lea rbp, [rsp + (\i*8)] 
        mov rdx, [r15 + (\i*8)] 
        xor rax, rax            

        .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
            mulx r8, r9, [rdi + (\k*8)]
            .if \k == 0
                adox r9, [rbp + (\k*8)]; mov [rbp + (\k*8)], r9; mov rcx, r8
            .else
                adcx r9, rcx; adox r9, [rbp + (\k*8)]; mov [rbp + (\k*8)], r9; mov rcx, r8
            .endif
        .endr
        mov r9, 0; adcx r9, rcx; adox r9, [rbp + 136]; mov [rbp + 136], r9
        mov r9, 0; adcx r9, r9;  adox r9, [rbp + 144]; mov [rbp + 144], r9

        mov rdx, [rbp + 0]
        xor rax, rax
        .irp k, 0,1,2,3,4,5,6,7,8
            mulx r8, r9, [r13 + (\k*8)]
            .if \k == 0
                adox r9, [rbp + 64]; mov [rbp + 64], r9; mov rcx, r8
            .else
                adcx r9, rcx; adox r9, [rbp + 64 + (\k*8)]; mov [rbp + 64 + (\k*8)], r9; mov rcx, r8
            .endif
        .endr
        mov r9, 0; adcx r9, rcx; adox r9, [rbp + 136]; mov [rbp + 136], r9
        mov r9, 0; adcx r9, r9;  adox r9, [rbp + 144]; mov [rbp + 144], r9
    .endr

    lea r12, [rip + .L_P_TABLE]

    .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
        mov rax, [rsp + 136 + (\k*8)]
        .if \k == 0
            sub rax, [r12 + (\k*8)]
        .else
            sbb rax, [r12 + (\k*8)]
        .endif
        mov [rsp + (\k*8)], rax
    .endr

    mov rax, [rsp + 272] 
    sbb rax, 0

    .irp k, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
        mov rax, [rsp + (\k*8)]     
        mov rbx, [rsp + 136 + (\k*8)] 
        cmovc rax, rbx
        mov [r14 + (\k*8)], rax
    .endr

    add rsp, 288
    pop r15; pop r14; pop r13; pop r12; pop rbp; pop rbx
    ret

fiat_p1070_square_asm:
    mov rdx, rsi
    jmp fiat_p1070_mul_asm
.section .note.GNU-stack,"",@progbits
