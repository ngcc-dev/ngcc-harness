.intel_syntax noprefix
.global fp2_mul511_asm
.global fp2_sqr511_asm

.text
.p2align 4

fp2_mul511_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 320

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 64]
    call fiat_p511_add_asm

    lea rdi, [rsp + 64]
    mov rsi, r14
    lea rdx, [r14 + 64]
    call fiat_p511_add_asm

    lea rdi, [rsp + 128]
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 64]
    call fiat_p511_mul_asm

    lea rdi, [rsp + 192]
    lea rsi, [r13 + 64]
    lea rdx, [r14 + 64]
    call fiat_p511_mul_asm

    lea rdi, [rsp + 256]
    mov rsi, r13
    mov rdx, r14
    call fiat_p511_mul_asm

    lea rdi, [r12 + 64]
    lea rsi, [rsp + 128]
    lea rdx, [rsp + 192]
    call fiat_p511_sub_asm

    lea rdi, [r12 + 64]
    lea rsi, [r12 + 64]
    lea rdx, [rsp + 256]
    call fiat_p511_sub_asm

    mov rdi, r12
    lea rsi, [rsp + 256]
    lea rdx, [rsp + 192]
    call fiat_p511_sub_asm

    add rsp, 320
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

fp2_sqr511_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 128

    mov r12, rdi
    mov r13, rsi

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 64]
    call fiat_p511_add_asm

    lea rdi, [rsp + 64]
    mov rsi, r13
    lea rdx, [r13 + 64]
    call fiat_p511_sub_asm

    lea rdi, [r12 + 64]
    mov rsi, r13
    lea rdx, [r13 + 64]
    call fiat_p511_mul_asm

    lea rdi, [r12 + 64]
    lea rsi, [r12 + 64]
    lea rdx, [r12 + 64]
    call fiat_p511_add_asm

    mov rdi, r12
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 64]
    call fiat_p511_mul_asm

    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
