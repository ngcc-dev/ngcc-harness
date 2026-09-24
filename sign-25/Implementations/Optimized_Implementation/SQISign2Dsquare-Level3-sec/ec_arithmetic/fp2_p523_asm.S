.intel_syntax noprefix
.global fp2_mul523_asm
.global fp2_sqr523_asm

.text
.p2align 4

fp2_mul523_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 368

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 72]
    call fiat_p523_add_asm

    lea rdi, [rsp + 72]
    mov rsi, r14
    lea rdx, [r14 + 72]
    call fiat_p523_add_asm

    lea rdi, [rsp + 144]
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 72]
    call fiat_p523_mul_asm

    lea rdi, [rsp + 216]
    lea rsi, [r13 + 72]
    lea rdx, [r14 + 72]
    call fiat_p523_mul_asm

    lea rdi, [rsp + 288]
    mov rsi, r13
    mov rdx, r14
    call fiat_p523_mul_asm

    lea rdi, [r12 + 72]
    lea rsi, [rsp + 144]
    lea rdx, [rsp + 216]
    call fiat_p523_sub_asm

    lea rdi, [r12 + 72]
    lea rsi, [r12 + 72]
    lea rdx, [rsp + 288]
    call fiat_p523_sub_asm

    mov rdi, r12
    lea rsi, [rsp + 288]
    lea rdx, [rsp + 216]
    call fiat_p523_sub_asm

    add rsp, 368
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

fp2_sqr523_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 144

    mov r12, rdi
    mov r13, rsi

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 72]
    call fiat_p523_add_asm

    lea rdi, [rsp + 72]
    mov rsi, r13
    lea rdx, [r13 + 72]
    call fiat_p523_sub_asm

    lea rdi, [r12 + 72]
    mov rsi, r13
    lea rdx, [r13 + 72]
    call fiat_p523_mul_asm

    lea rdi, [r12 + 72]
    lea rsi, [r12 + 72]
    lea rdx, [r12 + 72]
    call fiat_p523_add_asm

    mov rdi, r12
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 72]
    call fiat_p523_mul_asm

    add rsp, 144
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
