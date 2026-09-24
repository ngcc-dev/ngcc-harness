.intel_syntax noprefix

.text

.macro montgomery_reduce_qwords x, tmp
    vpmuldq ymm\tmp, ymm\x, ymm0
    vpmuldq ymm\tmp, ymm\tmp, ymm1
    vpaddq  ymm\x, ymm\x, ymm\tmp
    vpsrlq  ymm\x, ymm\x, 32
.endm

.macro montgomery_reduce_pair lo, hi, tlo, thi
    vpmuldq ymm\tlo, ymm\lo, ymm0
    vpmuldq ymm\thi, ymm\hi, ymm0
    vpmuldq ymm\tlo, ymm\tlo, ymm1
    vpmuldq ymm\thi, ymm\thi, ymm1
    vpaddq  ymm\lo, ymm\lo, ymm\tlo
    vpaddq  ymm\hi, ymm\hi, ymm\thi
    vpsrlq  ymm\lo, ymm\lo, 32
    vpsrlq  ymm\hi, ymm\hi, 32
.endm

.macro conditional_reduce_dwords x, tmp
    vpsrad ymm\tmp, ymm\x, 31
    vpand  ymm\tmp, ymm\tmp, ymm3
    vpsubd ymm\tmp, ymm\tmp, ymm1
    vpaddd ymm\x, ymm\x, ymm\tmp
.endm

.macro basemul_block off, lo, g0, hi, g1, tlo, thi
    vmovdqu ymm\lo, YMMWORD PTR [rsi + rax + \off]
    vmovdqu ymm\g0, YMMWORD PTR [rdx + rax + \off]
    vpsrlq  ymm\hi, ymm\lo, 32
    vpsrlq  ymm\g1, ymm\g0, 32
    vpmuldq ymm\lo, ymm\lo, ymm\g0
    vpmuldq ymm\hi, ymm\hi, ymm\g1
    montgomery_reduce_pair \lo, \hi, \tlo, \thi
    vpsllq  ymm\hi, ymm\hi, 32
    vpor    ymm\lo, ymm\lo, ymm\hi
    vmovdqu YMMWORD PTR [rdi + rax + \off], ymm\lo
.endm

.macro binary_raw4 op
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rsi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rsi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rsi + rax + 96]
    \op ymm4, ymm4, YMMWORD PTR [rdx + rax]
    \op ymm5, ymm5, YMMWORD PTR [rdx + rax + 32]
    \op ymm6, ymm6, YMMWORD PTR [rdx + rax + 64]
    \op ymm7, ymm7, YMMWORD PTR [rdx + rax + 96]
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

.macro binary_reduce4 op
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm6, YMMWORD PTR [rsi + rax + 32]
    vmovdqu ymm8, YMMWORD PTR [rsi + rax + 64]
    vmovdqu ymm10, YMMWORD PTR [rsi + rax + 96]
    \op ymm4, ymm4, YMMWORD PTR [rdx + rax]
    \op ymm6, ymm6, YMMWORD PTR [rdx + rax + 32]
    \op ymm8, ymm8, YMMWORD PTR [rdx + rax + 64]
    \op ymm10, ymm10, YMMWORD PTR [rdx + rax + 96]
    vpsrad ymm5, ymm4, 31
    vpsrad ymm7, ymm6, 31
    vpsrad ymm9, ymm8, 31
    vpsrad ymm11, ymm10, 31
    vpand  ymm5, ymm5, ymm3
    vpand  ymm7, ymm7, ymm3
    vpand  ymm9, ymm9, ymm3
    vpand  ymm11, ymm11, ymm3
    vpsubd ymm5, ymm5, ymm1
    vpsubd ymm7, ymm7, ymm1
    vpsubd ymm9, ymm9, ymm1
    vpsubd ymm11, ymm11, ymm1
    vpaddd ymm4, ymm4, ymm5
    vpaddd ymm6, ymm6, ymm7
    vpaddd ymm8, ymm8, ymm9
    vpaddd ymm10, ymm10, ymm11
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm8
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm10
.endm

.macro unary_pow2_4
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rsi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rsi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rsi + rax + 96]
    vpaddd  ymm4, ymm4, ymm0
    vpaddd  ymm5, ymm5, ymm0
    vpaddd  ymm6, ymm6, ymm0
    vpaddd  ymm7, ymm7, ymm0
    vpand   ymm4, ymm4, ymm1
    vpand   ymm5, ymm5, ymm1
    vpand   ymm6, ymm6, ymm1
    vpand   ymm7, ymm7, ymm1
    vpsubd  ymm4, ymm4, ymm0
    vpsubd  ymm5, ymm5, ymm0
    vpsubd  ymm6, ymm6, ymm0
    vpsubd  ymm7, ymm7, ymm0
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

.macro round_xtoy4
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rsi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rsi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rsi + rax + 96]
    vpaddd  ymm4, ymm4, ymm0
    vpaddd  ymm5, ymm5, ymm0
    vpaddd  ymm6, ymm6, ymm0
    vpaddd  ymm7, ymm7, ymm0
    vpsravd ymm4, ymm4, ymm2
    vpsravd ymm5, ymm5, ymm2
    vpsravd ymm6, ymm6, ymm2
    vpsravd ymm7, ymm7, ymm2
    vpand   ymm4, ymm4, ymm1
    vpand   ymm5, ymm5, ymm1
    vpand   ymm6, ymm6, ymm1
    vpand   ymm7, ymm7, ymm1
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

.macro round_xtoy4_imm shift
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rsi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rsi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rsi + rax + 96]
    vpaddd  ymm4, ymm4, ymm0
    vpaddd  ymm5, ymm5, ymm0
    vpaddd  ymm6, ymm6, ymm0
    vpaddd  ymm7, ymm7, ymm0
    vpsrad  ymm4, ymm4, \shift
    vpsrad  ymm5, ymm5, \shift
    vpsrad  ymm6, ymm6, \shift
    vpsrad  ymm7, ymm7, \shift
    vpand   ymm4, ymm4, ymm1
    vpand   ymm5, ymm5, ymm1
    vpand   ymm6, ymm6, ymm1
    vpand   ymm7, ymm7, ymm1
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

.macro reduce_round_xtoy4
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rsi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rsi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rsi + rax + 96]
    vpaddd  ymm4, ymm4, ymm0
    vpaddd  ymm5, ymm5, ymm0
    vpaddd  ymm6, ymm6, ymm0
    vpaddd  ymm7, ymm7, ymm0
    vpand   ymm4, ymm4, ymm1
    vpand   ymm5, ymm5, ymm1
    vpand   ymm6, ymm6, ymm1
    vpand   ymm7, ymm7, ymm1
    vpsubd  ymm4, ymm4, ymm0
    vpsubd  ymm5, ymm5, ymm0
    vpsubd  ymm6, ymm6, ymm0
    vpsubd  ymm7, ymm7, ymm0
    vpaddd  ymm4, ymm4, ymm2
    vpaddd  ymm5, ymm5, ymm2
    vpaddd  ymm6, ymm6, ymm2
    vpaddd  ymm7, ymm7, ymm2
    vpsravd ymm4, ymm4, ymm3
    vpsravd ymm5, ymm5, ymm3
    vpsravd ymm6, ymm6, ymm3
    vpsravd ymm7, ymm7, ymm3
    vpand   ymm4, ymm4, ymm8
    vpand   ymm5, ymm5, ymm8
    vpand   ymm6, ymm6, ymm8
    vpand   ymm7, ymm7, ymm8
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

.macro reduce_round_xtoy4_13_11
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rsi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rsi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rsi + rax + 96]
    vpaddd  ymm4, ymm4, ymm0
    vpaddd  ymm5, ymm5, ymm0
    vpaddd  ymm6, ymm6, ymm0
    vpaddd  ymm7, ymm7, ymm0
    vpand   ymm4, ymm4, ymm1
    vpand   ymm5, ymm5, ymm1
    vpand   ymm6, ymm6, ymm1
    vpand   ymm7, ymm7, ymm1
    vpsubd  ymm4, ymm4, ymm0
    vpsubd  ymm5, ymm5, ymm0
    vpsubd  ymm6, ymm6, ymm0
    vpsubd  ymm7, ymm7, ymm0
    vpaddd  ymm4, ymm4, ymm2
    vpaddd  ymm5, ymm5, ymm2
    vpaddd  ymm6, ymm6, ymm2
    vpaddd  ymm7, ymm7, ymm2
    vpsrad  ymm4, ymm4, 2
    vpsrad  ymm5, ymm5, 2
    vpsrad  ymm6, ymm6, 2
    vpsrad  ymm7, ymm7, 2
    vpand   ymm4, ymm4, ymm8
    vpand   ymm5, ymm5, ymm8
    vpand   ymm6, ymm6, ymm8
    vpand   ymm7, ymm7, ymm8
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

.macro shift_inplace4 op
    vmovdqu ymm4, YMMWORD PTR [rdi + rax]
    vmovdqu ymm5, YMMWORD PTR [rdi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rdi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rdi + rax + 96]
    \op ymm4, ymm4, ymm0
    \op ymm5, ymm5, ymm0
    \op ymm6, ymm6, ymm0
    \op ymm7, ymm7, ymm0
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

.macro shift_inplace4_imm op, shift
    vmovdqu ymm4, YMMWORD PTR [rdi + rax]
    vmovdqu ymm5, YMMWORD PTR [rdi + rax + 32]
    vmovdqu ymm6, YMMWORD PTR [rdi + rax + 64]
    vmovdqu ymm7, YMMWORD PTR [rdi + rax + 96]
    \op ymm4, ymm4, \shift
    \op ymm5, ymm5, \shift
    \op ymm6, ymm6, \shift
    \op ymm7, ymm7, \shift
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 64], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 96], ymm7
.endm

/*
 * void poly_basemul32(poly *r, poly *f, poly *g,
 *                     int32_t prime, int32_t primeinv)
 *
 * SysV ABI: rdi = r, rsi = f, rdx = g, ecx = prime, r8d = primeinv
 */
.globl poly_basemul32
.type poly_basemul32, @function
poly_basemul32:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0

    xor eax, eax
.Lpoly_basemul32_loop:
    basemul_block 0, 4, 5, 6, 7, 8, 9
    basemul_block 32, 10, 11, 12, 13, 14, 15

    add eax, 64
    cmp eax, 512
    jne .Lpoly_basemul32_loop

    ret
.size poly_basemul32, .-poly_basemul32

/*
 * void poly_add32(poly *r, poly *f, poly *g, int32_t prime)
 *
 * SysV ABI: rdi = r, rsi = f, rdx = g, ecx = prime
 */
.globl poly_add32
.type poly_add32, @function
poly_add32:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lpoly_add32_loop:
    binary_reduce4 vpaddd

    add eax, 128
    cmp eax, 512
    jne .Lpoly_add32_loop

    ret
.size poly_add32, .-poly_add32

/*
 * void poly_add(poly *r, poly *f, poly *g)
 *
 * SysV ABI: rdi = r, rsi = f, rdx = g
 */
.globl poly_add
.type poly_add, @function
poly_add:
    xor eax, eax
.Lpoly_add_loop:
    binary_raw4 vpaddd

    add eax, 128
    cmp eax, 512
    jne .Lpoly_add_loop

    ret
.size poly_add, .-poly_add

/*
 * void poly_sub32(poly *r, poly *f, poly *g, int32_t prime)
 *
 * SysV ABI: rdi = r, rsi = f, rdx = g, ecx = prime
 */
.globl poly_sub32
.type poly_sub32, @function
poly_sub32:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lpoly_sub32_loop:
    binary_reduce4 vpsubd

    add eax, 128
    cmp eax, 512
    jne .Lpoly_sub32_loop

    ret
.size poly_sub32, .-poly_sub32

/*
 * void poly_sub(poly *r, poly *f, poly *g)
 *
 * SysV ABI: rdi = r, rsi = f, rdx = g
 */
.globl poly_sub
.type poly_sub, @function
poly_sub:
    xor eax, eax
.Lpoly_sub_loop:
    binary_raw4 vpsubd

    add eax, 128
    cmp eax, 512
    jne .Lpoly_sub_loop

    ret
.size poly_sub, .-poly_sub

/*
 * void poly_reduce_pow2(poly *r, poly *f, int32_t d)
 *
 * SysV ABI: rdi = r, rsi = f, edx = d
 */
.globl poly_reduce_pow2
.type poly_reduce_pow2, @function
poly_reduce_pow2:
    mov ecx, edx
    mov eax, 1
    shl eax, cl
    lea r8d, [rax - 1]
    shr eax, 1

    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0          /* 2^(d-1) */
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1          /* 2^d - 1 */

    xor eax, eax
.Lpoly_reduce_pow2_loop:
    unary_pow2_4

    add eax, 128
    cmp eax, 512
    jne .Lpoly_reduce_pow2_loop

    ret
.size poly_reduce_pow2, .-poly_reduce_pow2

/*
 * void poly_conditional_final_reduce32(poly *r, int32_t prime)
 *
 * SysV ABI: rdi = r, esi = prime
 */
.globl poly_conditional_final_reduce32
.type poly_conditional_final_reduce32, @function
poly_conditional_final_reduce32:
    mov eax, esi
    sub eax, 1
    sar eax, 1                       /* offset = (prime - 1) >> 1 */
    lea r8d, [rax - 1]               /* offset - 1 */
    lea r9d, [rax + 1]               /* offset + 1 */

    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0          /* prime */
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1          /* offset - 1 */
    vmovd xmm2, r9d
    vpbroadcastd ymm2, xmm2          /* offset + 1 */

    xor eax, eax
.Lpoly_conditional_final_reduce32_loop:
    vmovdqu ymm4, YMMWORD PTR [rdi + rax]
    vmovdqu ymm7, YMMWORD PTR [rdi + rax + 32]

    vpaddd ymm5, ymm4, ymm1
    vpaddd ymm8, ymm7, ymm1
    vpsrad ymm5, ymm5, 31
    vpsrad ymm8, ymm8, 31
    vpand  ymm5, ymm5, ymm0
    vpand  ymm8, ymm8, ymm0

    vpsubd ymm6, ymm4, ymm2
    vpsubd ymm9, ymm7, ymm2
    vpsrad ymm6, ymm6, 31
    vpsrad ymm9, ymm9, 31
    vpandn ymm6, ymm6, ymm0
    vpandn ymm9, ymm9, ymm0

    vpsubd ymm5, ymm5, ymm6
    vpsubd ymm8, ymm8, ymm9
    vpaddd ymm4, ymm4, ymm5
    vpaddd ymm7, ymm7, ymm8
    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm7

    add eax, 64
    cmp eax, 512
    jne .Lpoly_conditional_final_reduce32_loop

    ret
.size poly_conditional_final_reduce32, .-poly_conditional_final_reduce32

/*
 * void poly_final_reduce_pow2(poly *r, int32_t prime, int32_t d)
 *
 * In-place equivalent of:
 *   poly_conditional_final_reduce32(r, prime);
 *   poly_reduce_pow2(r, r, d);
 *
 * SysV ABI: rdi = r, esi = prime, edx = d
 */
.globl poly_final_reduce_pow2
.type poly_final_reduce_pow2, @function
poly_final_reduce_pow2:
    mov eax, esi
    sub eax, 1
    sar eax, 1
    lea r8d, [rax - 1]               /* offset - 1 */
    lea r9d, [rax + 1]               /* offset + 1 */

    mov ecx, edx
    mov eax, 1
    shl eax, cl
    lea r10d, [rax - 1]              /* 2^d - 1 */
    shr eax, 1                       /* 2^(d-1) */

    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0          /* prime */
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1          /* offset - 1 */
    vmovd xmm2, r9d
    vpbroadcastd ymm2, xmm2          /* offset + 1 */
    vmovd xmm3, eax
    vpbroadcastd ymm3, xmm3          /* 2^(d-1) */
    vmovd xmm10, r10d
    vpbroadcastd ymm10, xmm10        /* 2^d - 1 */

    xor eax, eax
.Lpoly_final_reduce_pow2_loop:
    vmovdqu ymm4, YMMWORD PTR [rdi + rax]
    vmovdqu ymm7, YMMWORD PTR [rdi + rax + 32]

    vpaddd ymm5, ymm4, ymm1
    vpaddd ymm8, ymm7, ymm1
    vpsrad ymm5, ymm5, 31
    vpsrad ymm8, ymm8, 31
    vpand  ymm5, ymm5, ymm0
    vpand  ymm8, ymm8, ymm0

    vpsubd ymm6, ymm4, ymm2
    vpsubd ymm9, ymm7, ymm2
    vpsrad ymm6, ymm6, 31
    vpsrad ymm9, ymm9, 31
    vpandn ymm6, ymm6, ymm0
    vpandn ymm9, ymm9, ymm0

    vpsubd ymm5, ymm5, ymm6
    vpsubd ymm8, ymm8, ymm9
    vpaddd ymm4, ymm4, ymm5
    vpaddd ymm7, ymm7, ymm8

    vpaddd ymm4, ymm4, ymm3
    vpaddd ymm7, ymm7, ymm3
    vpand  ymm4, ymm4, ymm10
    vpand  ymm7, ymm7, ymm10
    vpsubd ymm4, ymm4, ymm3
    vpsubd ymm7, ymm7, ymm3

    vmovdqu YMMWORD PTR [rdi + rax], ymm4
    vmovdqu YMMWORD PTR [rdi + rax + 32], ymm7

    add eax, 64
    cmp eax, 512
    jne .Lpoly_final_reduce_pow2_loop

    ret
.size poly_final_reduce_pow2, .-poly_final_reduce_pow2

/*
 * void poly_reduce_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y)
 *
 * In-place safe equivalent of:
 *   poly_reduce_pow2(r, f, x);
 *   poly_round_xtoy(r, r, x, y);
 *
 * SysV ABI: rdi = r, rsi = f, edx = x, ecx = y
 */
.globl poly_reduce_round_xtoy
.type poly_reduce_round_xtoy, @function
poly_reduce_round_xtoy:
    cmp edx, 13
    jne .Lpoly_reduce_round_xtoy_generic
    cmp ecx, 11
    jne .Lpoly_reduce_round_xtoy_generic

    mov eax, 4096
    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0          /* 2^(13-1) */
    mov eax, 8191
    vmovd xmm1, eax
    vpbroadcastd ymm1, xmm1          /* 2^13 - 1 */
    mov eax, 2
    vmovd xmm2, eax
    vpbroadcastd ymm2, xmm2          /* 1 << (13 - 11 - 1) */
    mov eax, 2047
    vmovd xmm8, eax
    vpbroadcastd ymm8, xmm8          /* 2^11 - 1 */

    xor eax, eax
.Lpoly_reduce_round_xtoy_13_11_loop:
    reduce_round_xtoy4_13_11

    add eax, 128
    cmp eax, 512
    jne .Lpoly_reduce_round_xtoy_13_11_loop

    ret

.Lpoly_reduce_round_xtoy_generic:
    mov r8d, edx
    sub r8d, ecx                     /* shift = x - y */

    mov r9d, 1
    shl r9d, cl
    sub r9d, 1                       /* round mask = (1 << y) - 1 */

    mov ecx, edx
    sub ecx, 1
    mov eax, 1
    shl eax, cl                      /* reduce half = 1 << (x - 1) */
    mov r10d, eax
    shl r10d, 1
    sub r10d, 1                      /* reduce mask = (1 << x) - 1 */

    mov ecx, r8d
    sub ecx, 1
    mov r11d, 1
    shl r11d, cl                     /* round addend = 1 << (shift - 1) */

    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0
    vmovd xmm1, r10d
    vpbroadcastd ymm1, xmm1
    vmovd xmm2, r11d
    vpbroadcastd ymm2, xmm2
    vmovd xmm3, r8d
    vpbroadcastd ymm3, xmm3
    vmovd xmm8, r9d
    vpbroadcastd ymm8, xmm8

    xor eax, eax
.Lpoly_reduce_round_xtoy_loop:
    reduce_round_xtoy4

    add eax, 128
    cmp eax, 512
    jne .Lpoly_reduce_round_xtoy_loop

    ret
.size poly_reduce_round_xtoy, .-poly_reduce_round_xtoy

/*
 * void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y)
 *
 * SysV ABI: rdi = r, rsi = f, edx = x, ecx = y
 */
.globl poly_round_xtoy
.type poly_round_xtoy, @function
poly_round_xtoy:
    cmp edx, 13
    jne .Lpoly_round_xtoy_check_11_1
    cmp ecx, 11
    je .Lpoly_round_xtoy_13_11

.Lpoly_round_xtoy_check_11_1:
    cmp edx, 11
    jne .Lpoly_round_xtoy_generic
    cmp ecx, 1
    je .Lpoly_round_xtoy_11_1

.Lpoly_round_xtoy_generic:
    mov r8d, edx
    sub r8d, ecx                     /* shift = x - y */

    mov r9d, 1
    shl r9d, cl
    sub r9d, 1                       /* mask = (1 << y) - 1 */

    mov ecx, r8d
    sub ecx, 1
    mov eax, 1
    shl eax, cl                      /* addend = 1 << (x - (y + 1)) */

    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0
    vmovd xmm1, r9d
    vpbroadcastd ymm1, xmm1
    vmovd xmm2, r8d
    vpbroadcastd ymm2, xmm2

    xor eax, eax
.Lpoly_round_xtoy_loop:
    round_xtoy4

    add eax, 128
    cmp eax, 512
    jne .Lpoly_round_xtoy_loop

    ret

.Lpoly_round_xtoy_13_11:
    mov eax, 2
    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0
    mov eax, 2047
    vmovd xmm1, eax
    vpbroadcastd ymm1, xmm1

    xor eax, eax
.Lpoly_round_xtoy_13_11_loop:
    round_xtoy4_imm 2

    add eax, 128
    cmp eax, 512
    jne .Lpoly_round_xtoy_13_11_loop

    ret

.Lpoly_round_xtoy_11_1:
    mov eax, 512
    vmovd xmm0, eax
    vpbroadcastd ymm0, xmm0
    vpcmpeqd ymm1, ymm1, ymm1
    vpsrld ymm1, ymm1, 31

    xor eax, eax
.Lpoly_round_xtoy_11_1_loop:
    round_xtoy4_imm 10

    add eax, 128
    cmp eax, 512
    jne .Lpoly_round_xtoy_11_1_loop

    ret
.size poly_round_xtoy, .-poly_round_xtoy

/*
 * void poly_compress(poly *r, int32_t x)
 *
 * SysV ABI: rdi = r, esi = x
 */
.globl poly_compress
.type poly_compress, @function
poly_compress:
    cmp esi, 8
    je .Lpoly_compress_8
    cmp esi, 3
    je .Lpoly_compress_3

    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    xor eax, eax
.Lpoly_compress_loop:
    shift_inplace4 vpsravd

    add eax, 128
    cmp eax, 512
    jne .Lpoly_compress_loop

    ret

.Lpoly_compress_8:
    xor eax, eax
.Lpoly_compress_8_loop:
    shift_inplace4_imm vpsrad, 8

    add eax, 128
    cmp eax, 512
    jne .Lpoly_compress_8_loop

    ret

.Lpoly_compress_3:
    xor eax, eax
.Lpoly_compress_3_loop:
    shift_inplace4_imm vpsrad, 3

    add eax, 128
    cmp eax, 512
    jne .Lpoly_compress_3_loop

    ret
.size poly_compress, .-poly_compress

/*
 * void poly_decompress(poly *r, int32_t x)
 *
 * SysV ABI: rdi = r, esi = x
 */
.globl poly_decompress
.type poly_decompress, @function
poly_decompress:
    cmp esi, 8
    je .Lpoly_decompress_8
    cmp esi, 3
    je .Lpoly_decompress_3

    vmovd xmm0, esi
    vpbroadcastd ymm0, xmm0

    xor eax, eax
.Lpoly_decompress_loop:
    shift_inplace4 vpsllvd

    add eax, 128
    cmp eax, 512
    jne .Lpoly_decompress_loop

    ret

.Lpoly_decompress_8:
    xor eax, eax
.Lpoly_decompress_8_loop:
    shift_inplace4_imm vpslld, 8

    add eax, 128
    cmp eax, 512
    jne .Lpoly_decompress_8_loop

    ret

.Lpoly_decompress_3:
    xor eax, eax
.Lpoly_decompress_3_loop:
    shift_inplace4_imm vpslld, 3

    add eax, 128
    cmp eax, 512
    jne .Lpoly_decompress_3_loop

    ret
.size poly_decompress, .-poly_decompress

.section .note.GNU-stack,"",@progbits
