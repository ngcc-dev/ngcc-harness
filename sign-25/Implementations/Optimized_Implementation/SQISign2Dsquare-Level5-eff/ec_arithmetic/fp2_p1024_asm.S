.intel_syntax noprefix
.global fp2_mul1024_asm
.global fp2_sqr1024_asm

.text
.p2align 4

fp2_mul1024_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 640

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 128]
    call fiat_p1024_add_asm

    lea rdi, [rsp + 128]
    mov rsi, r14
    lea rdx, [r14 + 128]
    call fiat_p1024_add_asm

    lea rdi, [rsp + 256]
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 128]
    call fiat_p1024_mul_asm

    lea rdi, [rsp + 384]
    lea rsi, [r13 + 128]
    lea rdx, [r14 + 128]
    call fiat_p1024_mul_asm

    lea rdi, [rsp + 512]
    mov rsi, r13
    mov rdx, r14
    call fiat_p1024_mul_asm

    lea rdi, [r12 + 128]
    lea rsi, [rsp + 256]
    lea rdx, [rsp + 384]
    call fiat_p1024_sub_asm

    lea rdi, [r12 + 128]
    lea rsi, [r12 + 128]
    lea rdx, [rsp + 512]
    call fiat_p1024_sub_asm

    mov rdi, r12
    lea rsi, [rsp + 512]
    lea rdx, [rsp + 384]
    call fiat_p1024_sub_asm

    add rsp, 640
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

fp2_sqr1024_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 256

    mov r12, rdi
    mov r13, rsi

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 128]
    call fiat_p1024_add_asm

    lea rdi, [rsp + 128]
    mov rsi, r13
    lea rdx, [r13 + 128]
    call fiat_p1024_sub_asm

    lea rdi, [r12 + 128]
    mov rsi, r13
    lea rdx, [r13 + 128]
    call fiat_p1024_mul_asm

    lea rdi, [r12 + 128]
    lea rsi, [r12 + 128]
    lea rdx, [r12 + 128]
    call fiat_p1024_add_asm

    mov rdi, r12
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 128]
    call fiat_p1024_mul_asm

    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
