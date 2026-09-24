.intel_syntax noprefix

.macro montgomery_reduce_vec out, acc_e, acc_o
    vpmuldq ymm8, ymm\acc_e, ymm1
    vpmuldq ymm9, ymm\acc_o, ymm1
    vpmuldq ymm8, ymm8, ymm0
    vpmuldq ymm9, ymm9, ymm0
    vpaddq ymm\acc_e, ymm\acc_e, ymm8
    vpaddq ymm\acc_o, ymm\acc_o, ymm9
    vmovshdup ymm\acc_e, ymm\acc_e
    vpblendd ymm\out, ymm\acc_e, ymm\acc_o, 0xaa
.endm

.macro conditional_reduce_vec x
    vpsrad ymm4, ymm\x, 31
    vpand ymm4, ymm4, ymm1
    vpsubd ymm4, ymm4, ymm0
    vpaddd ymm\x, ymm\x, ymm4
.endm

.text

.globl poly_basemul32_avx
.type poly_basemul32_avx, @function
poly_basemul32_avx:
    vmovd xmm0, ecx
    vpbroadcastd ymm0, xmm0
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1

    xor eax, eax
.Lbasemul_loop:
    vmovdqu ymm2, YMMWORD PTR [rsi + rax]
    vmovdqu ymm3, YMMWORD PTR [rdx + rax]
    vmovshdup ymm4, ymm2
    vmovshdup ymm5, ymm3
    vpmuldq ymm6, ymm2, ymm3
    vpmuldq ymm7, ymm4, ymm5
    montgomery_reduce_vec 6, 6, 7
    vmovdqu YMMWORD PTR [rdi + rax], ymm6
    add rax, 32
    cmp rax, 4096
    jb .Lbasemul_loop

    vzeroupper
    ret
.size poly_basemul32_avx, .-poly_basemul32_avx

.globl poly_add32_avx
.type poly_add32_avx, @function
poly_add32_avx:
    vmovd xmm0, ecx
    vpbroadcastd ymm0, xmm0
    lea eax, [rcx + rcx]
    vmovd xmm1, eax
    vpbroadcastd ymm1, xmm1

    xor eax, eax
.Ladd32_loop:
    vmovdqu ymm2, YMMWORD PTR [rsi + rax]
    vpaddd ymm2, ymm2, YMMWORD PTR [rdx + rax]
    conditional_reduce_vec 2
    vmovdqu YMMWORD PTR [rdi + rax], ymm2
    add rax, 32
    cmp rax, 4096
    jb .Ladd32_loop

    vzeroupper
    ret
.size poly_add32_avx, .-poly_add32_avx

.globl poly_add_avx
.type poly_add_avx, @function
poly_add_avx:
    xor eax, eax
.Ladd_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + rax]
    vpaddd ymm0, ymm0, YMMWORD PTR [rdx + rax]
    vmovdqu YMMWORD PTR [rdi + rax], ymm0
    add rax, 32
    cmp rax, 4096
    jb .Ladd_loop

    vzeroupper
    ret
.size poly_add_avx, .-poly_add_avx

.globl poly_sub32_avx
.type poly_sub32_avx, @function
poly_sub32_avx:
    vmovd xmm0, ecx
    vpbroadcastd ymm0, xmm0
    lea eax, [rcx + rcx]
    vmovd xmm1, eax
    vpbroadcastd ymm1, xmm1

    xor eax, eax
.Lsub32_loop:
    vmovdqu ymm2, YMMWORD PTR [rsi + rax]
    vpsubd ymm2, ymm2, YMMWORD PTR [rdx + rax]
    conditional_reduce_vec 2
    vmovdqu YMMWORD PTR [rdi + rax], ymm2
    add rax, 32
    cmp rax, 4096
    jb .Lsub32_loop

    vzeroupper
    ret
.size poly_sub32_avx, .-poly_sub32_avx

.globl poly_sub_avx
.type poly_sub_avx, @function
poly_sub_avx:
    xor eax, eax
.Lsub_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + rax]
    vpsubd ymm0, ymm0, YMMWORD PTR [rdx + rax]
    vmovdqu YMMWORD PTR [rdi + rax], ymm0
    add rax, 32
    cmp rax, 4096
    jb .Lsub_loop

    vzeroupper
    ret
.size poly_sub_avx, .-poly_sub_avx

.globl poly_reduce_pow2_avx
.type poly_reduce_pow2_avx, @function
poly_reduce_pow2_avx:
    mov ecx, edx
    mov eax, 1
    shl eax, cl
    lea r8d, [rax - 1]
    shr eax, 1
    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1

    xor eax, eax
.Lreduce_pow2_loop:
    vmovdqu ymm2, YMMWORD PTR [rsi + rax]
    vpaddd ymm2, ymm2, ymm0
    vpand ymm2, ymm2, ymm1
    vpsubd ymm2, ymm2, ymm0
    vmovdqu YMMWORD PTR [rdi + rax], ymm2
    add rax, 32
    cmp rax, 4096
    jb .Lreduce_pow2_loop

    vzeroupper
    ret
.size poly_reduce_pow2_avx, .-poly_reduce_pow2_avx

.globl poly_conditional_final_reduce32_avx
.type poly_conditional_final_reduce32_avx, @function
poly_conditional_final_reduce32_avx:
    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    lea eax, [rsi - 1]
    shr eax, 1
    lea r8d, [rax - 1]
    lea r9d, [rax + 1]
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1
    vmovd xmm2, r9d
    vpbroadcastd ymm2, xmm2
    vpcmpeqd ymm7, ymm7, ymm7

    xor eax, eax
.Lfinal_reduce_loop:
    vmovdqu ymm3, YMMWORD PTR [rdi + rax]
    vpaddd ymm4, ymm3, ymm1
    vpsrad ymm4, ymm4, 31
    vpsubd ymm5, ymm3, ymm2
    vpsrad ymm5, ymm5, 31
    vpxor ymm5, ymm5, ymm7
    vpand ymm4, ymm4, ymm0
    vpand ymm5, ymm5, ymm0
    vpsubd ymm4, ymm4, ymm5
    vpaddd ymm3, ymm3, ymm4
    vmovdqu YMMWORD PTR [rdi + rax], ymm3
    add rax, 32
    cmp rax, 4096
    jb .Lfinal_reduce_loop

    vzeroupper
    ret
.size poly_conditional_final_reduce32_avx, .-poly_conditional_final_reduce32_avx

.globl poly_round_xtoy_avx
.type poly_round_xtoy_avx, @function
poly_round_xtoy_avx:
    mov r10d, ecx
    mov r8d, edx
    sub r8d, r10d
    mov r9d, r8d
    dec r9d

    mov eax, 1
    mov ecx, r9d
    shl eax, cl
    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0

    mov eax, 1
    mov ecx, r10d
    shl eax, cl
    dec eax
    vmovd xmm1, eax
    vpbroadcastd ymm1, xmm1
    vmovd xmm2, r8d

    xor eax, eax
.Lround_xtoy_loop:
    vmovdqu ymm3, YMMWORD PTR [rsi + rax]
    vpaddd ymm3, ymm3, ymm0
    vpsrad ymm3, ymm3, xmm2
    vpand ymm3, ymm3, ymm1
    vmovdqu YMMWORD PTR [rdi + rax], ymm3
    add rax, 32
    cmp rax, 4096
    jb .Lround_xtoy_loop

    vzeroupper
    ret
.size poly_round_xtoy_avx, .-poly_round_xtoy_avx

.globl poly_compress_avx
.type poly_compress_avx, @function
poly_compress_avx:
    vmovd xmm1, esi
    xor eax, eax
.Lcompress_loop:
    vmovdqu ymm0, YMMWORD PTR [rdi + rax]
    vpsrad ymm0, ymm0, xmm1
    vmovdqu YMMWORD PTR [rdi + rax], ymm0
    add rax, 32
    cmp rax, 4096
    jb .Lcompress_loop

    vzeroupper
    ret
.size poly_compress_avx, .-poly_compress_avx

.globl poly_decompress_avx
.type poly_decompress_avx, @function
poly_decompress_avx:
    vmovd xmm1, esi
    xor eax, eax
.Ldecompress_loop:
    vmovdqu ymm0, YMMWORD PTR [rdi + rax]
    vpslld ymm0, ymm0, xmm1
    vmovdqu YMMWORD PTR [rdi + rax], ymm0
    add rax, 32
    cmp rax, 4096
    jb .Ldecompress_loop

    vzeroupper
    ret
.size poly_decompress_avx, .-poly_decompress_avx

.globl poly_shift_avx
.type poly_shift_avx, @function
poly_shift_avx:
    vmovd xmm1, esi
    xor eax, eax
.Lshift_loop:
    vmovdqu ymm0, YMMWORD PTR [rdi + rax]
    vmovdqu ymm2, YMMWORD PTR [rdi + rax + 32]
    vmovdqu ymm3, YMMWORD PTR [rdi + rax + 64]
    vmovdqu ymm4, YMMWORD PTR [rdi + rax + 96]
    vpslld ymm0, ymm0, xmm1
    vpslld ymm2, ymm2, xmm1
    vpslld ymm3, ymm3, xmm1
    vpslld ymm4, ymm4, xmm1
    vmovdqu YMMWORD PTR [rdi + rax], ymm0
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm2
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm3
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm4
    add rax, 128
    cmp rax, 4096
    jb .Lshift_loop

    vzeroupper
    ret
.size poly_shift_avx, .-poly_shift_avx

.globl poly_power2round_avx
.type poly_power2round_avx, @function
poly_power2round_avx:
    mov r10d, ecx

    mov eax, 1
    lea ecx, [r10d - 1]
    shl eax, cl
    mov r8d, eax
    lea r9d, [rax + rax - 1]

    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vmovd xmm1, r9d
    vpbroadcastd ymm1, xmm1
    vmovd xmm2, r10d

    xor eax, eax
.Lpower2round_loop:
    vmovdqu ymm3, YMMWORD PTR [rdx + rax]
    vpaddd ymm4, ymm3, ymm0
    vpand ymm4, ymm4, ymm1
    vpsubd ymm4, ymm4, ymm0
    vpsubd ymm5, ymm3, ymm4
    vpsrad ymm5, ymm5, xmm2
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rsi + rax], ymm5
    add rax, 32
    cmp rax, 4096
    jb .Lpower2round_loop

    vzeroupper
    ret
.size poly_power2round_avx, .-poly_power2round_avx

.globl poly_apply_hint_avx
.type poly_apply_hint_avx, @function
poly_apply_hint_avx:
    vpxor ymm0, ymm0, ymm0
    vpcmpeqd ymm7, ymm7, ymm7
    vpsubd ymm6, ymm0, ymm7
    vpaddd ymm5, ymm6, ymm6

    xor eax, eax
.Lapply_hint_loop:
    vmovdqu ymm1, YMMWORD PTR [rdi + rax]
    vmovdqu ymm2, YMMWORD PTR [rsi + rax]
    vmovdqu ymm3, YMMWORD PTR [rdx + rax]

    vpsrad ymm2, ymm2, 31
    vpand ymm2, ymm2, ymm5
    vpsubd ymm2, ymm6, ymm2

    vpcmpeqd ymm4, ymm3, ymm0
    vpxor ymm4, ymm4, ymm7
    vpand ymm2, ymm2, ymm4
    vpaddd ymm1, ymm1, ymm2

    vmovdqu YMMWORD PTR [rdi + rax], ymm1
    add rax, 32
    cmp rax, 4096
    jb .Lapply_hint_loop

    vzeroupper
    ret
.size poly_apply_hint_avx, .-poly_apply_hint_avx

.section .note.GNU-stack,"",@progbits
