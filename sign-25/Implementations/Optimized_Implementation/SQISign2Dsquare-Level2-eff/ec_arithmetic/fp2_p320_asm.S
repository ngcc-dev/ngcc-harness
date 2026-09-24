.intel_syntax noprefix
.global fp2_mul320_asm
.global fp2_sqr320_asm

.text
.p2align 4

fp2_mul320_asm:
    push r12
    push r13
    push r14
    sub rsp, 208

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    lea rdi, [rsp + 0]
    mov rsi, r13
    mov rdx, r14
    call fiat_p320_mul_asm

    lea rdi, [rsp + 40]
    lea rsi, [r13 + 40]
    lea rdx, [r14 + 40]
    call fiat_p320_mul_asm

    lea rdi, [rsp + 80]
    mov rsi, r13
    lea rdx, [r13 + 40]
    call fiat_p320_add_asm

    lea rdi, [rsp + 120]
    mov rsi, r14
    lea rdx, [r14 + 40]
    call fiat_p320_add_asm

    lea rdi, [rsp + 160]
    lea rsi, [rsp + 80]
    lea rdx, [rsp + 120]
    call fiat_p320_mul_asm

    lea rdi, [rsp + 80]
    lea rsi, [rsp + 160]
    lea rdx, [rsp + 0]
    call fiat_p320_sub_asm

    lea rdi, [r12 + 40]
    lea rsi, [rsp + 80]
    lea rdx, [rsp + 40]
    call fiat_p320_sub_asm

    mov rdi, r12
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 40]
    call fiat_p320_sub_asm

    add rsp, 208
    pop r14
    pop r13
    pop r12
    ret

fp2_sqr320_asm:
    push r12
    push r13
    push r14
    sub rsp, 128

    mov r12, rdi
    mov r13, rsi

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 40]
    call fiat_p320_add_asm

    lea rdi, [rsp + 40]
    mov rsi, r13
    lea rdx, [r13 + 40]
    call fiat_p320_sub_asm

    lea rdi, [rsp + 80]
    mov rsi, r13
    lea rdx, [r13 + 40]
    call fiat_p320_mul_asm

    mov rdi, r12
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 40]
    call fiat_p320_mul_asm

    lea rdi, [r12 + 40]
    lea rsi, [rsp + 80]
    lea rdx, [rsp + 80]
    call fiat_p320_add_asm

    add rsp, 128
    pop r14
    pop r13
    pop r12
    ret

.section .note.GNU-stack,"",@progbits
