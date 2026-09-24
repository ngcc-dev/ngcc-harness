.intel_syntax noprefix
.global fp2_mul1070_asm
.global fp2_sqr1070_asm

.text
.p2align 4

fp2_mul1070_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 688

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 136]
    call fiat_p1070_add_asm

    lea rdi, [rsp + 136]
    mov rsi, r14
    lea rdx, [r14 + 136]
    call fiat_p1070_add_asm

    lea rdi, [rsp + 272]
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 136]
    call fiat_p1070_mul_asm

    lea rdi, [rsp + 408]
    lea rsi, [r13 + 136]
    lea rdx, [r14 + 136]
    call fiat_p1070_mul_asm

    lea rdi, [rsp + 544]
    mov rsi, r13
    mov rdx, r14
    call fiat_p1070_mul_asm

    lea rdi, [r12 + 136]
    lea rsi, [rsp + 272]
    lea rdx, [rsp + 408]
    call fiat_p1070_sub_asm

    lea rdi, [r12 + 136]
    lea rsi, [r12 + 136]
    lea rdx, [rsp + 544]
    call fiat_p1070_sub_asm

    mov rdi, r12
    lea rsi, [rsp + 544]
    lea rdx, [rsp + 408]
    call fiat_p1070_sub_asm

    add rsp, 688
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

fp2_sqr1070_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 272

    mov r12, rdi
    mov r13, rsi

    lea rdi, [rsp + 0]
    mov rsi, r13
    lea rdx, [r13 + 136]
    call fiat_p1070_add_asm

    lea rdi, [rsp + 136]
    mov rsi, r13
    lea rdx, [r13 + 136]
    call fiat_p1070_sub_asm

    lea rdi, [r12 + 136]
    mov rsi, r13
    lea rdx, [r13 + 136]
    call fiat_p1070_mul_asm

    lea rdi, [r12 + 136]
    lea rsi, [r12 + 136]
    lea rdx, [r12 + 136]
    call fiat_p1070_add_asm

    mov rdi, r12
    lea rsi, [rsp + 0]
    lea rdx, [rsp + 136]
    call fiat_p1070_mul_asm

    add rsp, 272
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.section .note.GNU-stack,"",@progbits
