.intel_syntax noprefix

.text

/*
 * void poly_basemul_add32(poly *r, const poly *f, const poly *g,
 *                         int32_t prime, int32_t primeinv)
 *
 * SysV ABI:
 *   rdi = r, rsi = f, rdx = g, ecx = prime, r8d = primeinv
 *
 * For each coefficient:
 *   t    = ((int64_t)f[i] * g[i]
 *           + (int64_t)(int32_t)((int64_t)f[i] * g[i]) * primeinv * prime) >> 32
 *   r[i] = conditional_reduce32(r[i] + t, prime)
 */
.macro montgomery_reduce_qwords x, tmp
    vpmuldq ymm\tmp, ymm\x, ymm0
    vpmuldq ymm\tmp, ymm\tmp, ymm1
    vpaddq  ymm\x, ymm\x, ymm\tmp
    vpsrlq  ymm\x, ymm\x, 32
.endm

.macro reduce_low_oct x, tmp, tmp2
    vpsrad ymm\tmp, ymm\x, 31
    vpand  ymm\tmp, ymm\tmp, ymm3
    vpsubd ymm\tmp, ymm\tmp, ymm1
    vpaddd ymm\x, ymm\x, ymm\tmp
.endm

.macro reduce_pair a, b
    reduce_low_oct \a, 6, 7
    reduce_low_oct \b, 6, 7
.endm

.macro reduce_4acc a, b, c, d
    reduce_low_oct \a, 4, 6
    reduce_low_oct \b, 4, 6
    reduce_low_oct \c, 4, 6
    reduce_low_oct \d, 4, 6
.endm

.macro reduce_5acc a, b, c, d, e
    reduce_low_oct \a, 4, 6
    reduce_low_oct \b, 4, 6
    reduce_low_oct \c, 4, 6
    reduce_low_oct \d, 4, 6
    reduce_low_oct \e, 4, 6
.endm

.macro awin_row_dot_term off
    vmovdqu ymm4, YMMWORD PTR [rsi + rax + \off]
    vmovdqu ymm5, YMMWORD PTR [rdx + rax + \off]

    vpsrlq ymm6, ymm4, 32
    vpsrlq ymm7, ymm5, 32

    vpmuldq ymm4, ymm4, ymm5
    vpmuldq ymm6, ymm6, ymm7

    montgomery_reduce_qwords 4, 8
    montgomery_reduce_qwords 6, 8

    vpsllq ymm6, ymm6, 32
    vpor   ymm4, ymm4, ymm6

    vpaddd ymm12, ymm12, ymm4
.endm

.macro awin_2row_dot_term off
    vmovdqu ymm5, YMMWORD PTR [rdx + rax + \off]
    vmovdqu ymm4, YMMWORD PTR [rsi + rax + \off]
    vmovdqu ymm10, YMMWORD PTR [rsi + rax + \off + 512]

    vpsrlq ymm7, ymm5, 32
    vpsrlq ymm6, ymm4, 32
    vpsrlq ymm11, ymm10, 32

    vpmuldq ymm4, ymm4, ymm5
    vpmuldq ymm10, ymm10, ymm5
    vpmuldq ymm6, ymm6, ymm7
    vpmuldq ymm11, ymm11, ymm7

    vpmuldq ymm8, ymm4, ymm0
    vpmuldq ymm14, ymm10, ymm0
    vpmuldq ymm9, ymm6, ymm0
    vpmuldq ymm15, ymm11, ymm0

    vpmuldq ymm8, ymm8, ymm1
    vpmuldq ymm14, ymm14, ymm1
    vpmuldq ymm9, ymm9, ymm1
    vpmuldq ymm15, ymm15, ymm1

    vpaddq ymm4, ymm4, ymm8
    vpaddq ymm10, ymm10, ymm14
    vpaddq ymm6, ymm6, ymm9
    vpaddq ymm11, ymm11, ymm15

    vpsrlq ymm4, ymm4, 32
    vpsrlq ymm10, ymm10, 32
    vpsrlq ymm6, ymm6, 32
    vpsrlq ymm11, ymm11, 32

    vpsllq ymm6, ymm6, 32
    vpsllq ymm11, ymm11, 32
    vpor   ymm4, ymm4, ymm6
    vpor   ymm10, ymm10, ymm11

    vpaddd ymm12, ymm12, ymm4
    vpaddd ymm13, ymm13, ymm10
.endm

.macro awin_4row_dot_one acc, rowoff
    vmovdqu ymm4, YMMWORD PTR [rsi + rax + \rowoff]
    vpsrlq ymm6, ymm4, 32

    vpmuldq ymm4, ymm4, ymm5
    vpmuldq ymm6, ymm6, ymm7

    montgomery_reduce_qwords 4, 8
    montgomery_reduce_qwords 6, 8

    vpsllq ymm6, ymm6, 32
    vpor   ymm4, ymm4, ymm6

    vpaddd ymm\acc, ymm\acc, ymm4
.endm

.macro awin_4row_dot_term off
    vmovdqu ymm5, YMMWORD PTR [rdx + rax + \off]
    vpsrlq ymm7, ymm5, 32

    awin_4row_dot_one 12, \off
    awin_4row_dot_one 13, \off + 512
    awin_4row_dot_one 14, \off + 1024
    awin_4row_dot_one 15, \off + 1536
.endm

.macro awin_5row_dot_term off
    vmovdqu ymm5, YMMWORD PTR [rdx + rax + \off]
    vpsrlq ymm7, ymm5, 32

    awin_4row_dot_one 11, \off
    awin_4row_dot_one 12, \off + 512
    awin_4row_dot_one 13, \off + 1024
    awin_4row_dot_one 14, \off + 1536
    awin_4row_dot_one 15, \off + 2048
.endm

.macro awin_raw_acc acc_e, acc_o, rowoff, off
    vmovdqu   ymm2, YMMWORD PTR [rsi + rax + \rowoff + \off]
    vmovshdup ymm3, ymm2
    vpmuldq   ymm2, ymm2, ymm14
    vpmuldq   ymm3, ymm3, ymm15
    vpaddq    ymm\acc_e, ymm\acc_e, ymm2
    vpaddq    ymm\acc_o, ymm\acc_o, ymm3
.endm

.macro awin_raw_2row_dot_term off
    vmovdqu   ymm14, YMMWORD PTR [rdx + rax + \off]
    vmovshdup ymm15, ymm14
    awin_raw_acc 4, 6, 0, \off
    awin_raw_acc 5, 7, 512, \off
.endm

.macro awin_raw_1row_dot_term off
    vmovdqu   ymm14, YMMWORD PTR [rdx + rax + \off]
    vmovshdup ymm15, ymm14
    awin_raw_acc 4, 5, 0, \off
.endm

.macro awin_raw_4row_dot_term off
    vmovdqu   ymm14, YMMWORD PTR [rdx + rax + \off]
    vmovshdup ymm15, ymm14
    awin_raw_acc 4, 8, 0, \off
    awin_raw_acc 5, 9, 512, \off
    awin_raw_acc 6, 10, 1024, \off
    awin_raw_acc 7, 11, 1536, \off
.endm

.macro awin_raw_5row_dot_term off
    vmovdqu   ymm14, YMMWORD PTR [rdx + rax + \off]
    vmovshdup ymm15, ymm14
    awin_raw_acc 4, 9, 0, \off
    awin_raw_acc 5, 10, 512, \off
    awin_raw_acc 6, 11, 1024, \off
    awin_raw_acc 7, 12, 1536, \off
    awin_raw_acc 8, 13, 2048, \off
.endm

.macro awin_raw_reduce acc_e, acc_o
    vpmuldq ymm14, ymm\acc_e, ymm0
    vpmuldq ymm15, ymm\acc_o, ymm0
    vpmuldq ymm14, ymm14, ymm1
    vpmuldq ymm15, ymm15, ymm1
    vpaddq  ymm\acc_e, ymm\acc_e, ymm14
    vpaddq  ymm\acc_o, ymm\acc_o, ymm15
    vpsrlq  ymm\acc_e, ymm\acc_e, 32
    vpsrlq  ymm\acc_o, ymm\acc_o, 32
    vpsllq  ymm\acc_o, ymm\acc_o, 32
    vpor    ymm\acc_e, ymm\acc_e, ymm\acc_o
    reduce_low_oct \acc_e, 14, 15
.endm

.globl poly_basemul_add32
.type poly_basemul_add32, @function
poly_basemul_add32:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1          /* prime */
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0          /* primeinv */
    vpaddd ymm3, ymm1, ymm1          /* 2 * prime */

    xor eax, eax
.Lbasemul_add_loop:
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rdx + rax]

    vpsrlq ymm6, ymm4, 32
    vpsrlq ymm7, ymm5, 32

    vpmuldq ymm4, ymm4, ymm5         /* products for lanes 0,2,4,6 */
    vpmuldq ymm6, ymm6, ymm7         /* products for lanes 1,3,5,7 */

    montgomery_reduce_qwords 4, 8
    montgomery_reduce_qwords 6, 8

    vpsllq ymm6, ymm6, 32
    vpor   ymm4, ymm4, ymm6          /* eight reduced int32 coefficients */

    vpaddd ymm4, ymm4, YMMWORD PTR [rdi + rax]
    reduce_low_oct 4, 6, 7
    vmovdqu YMMWORD PTR [rdi + rax], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lbasemul_add_loop

    vzeroupper
    ret
.size poly_basemul_add32, .-poly_basemul_add32

/*
 * void poly_basemul32_avx(poly *r, poly *f, poly *g,
 *                          int32_t prime, int32_t primeinv)
 *
 * Same ABI as poly_basemul32: rdi=r, rsi=f, rdx=g, ecx=prime, r8d=primeinv
 * r[i] = montgomery_mul32(f[i], g[i]) — bit-for-bit with scalar poly_basemul32.
 */
.globl poly_basemul32_avx
.type poly_basemul32_avx, @function
poly_basemul32_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1          /* prime */
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0          /* primeinv */

    xor eax, eax
.Lbasemul_only_loop:
    vmovdqu ymm4, YMMWORD PTR [rsi + rax]
    vmovdqu ymm5, YMMWORD PTR [rdx + rax]

    vpsrlq ymm6, ymm4, 32
    vpsrlq ymm7, ymm5, 32

    vpmuldq ymm4, ymm4, ymm5
    vpmuldq ymm6, ymm6, ymm7

    montgomery_reduce_qwords 4, 8
    montgomery_reduce_qwords 6, 8

    vpsllq ymm6, ymm6, 32
    vpor   ymm4, ymm4, ymm6

    vmovdqu YMMWORD PTR [rdi + rax], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lbasemul_only_loop

    vzeroupper
    ret
.size poly_basemul32_avx, .-poly_basemul32_avx

/*
 * void ring_mul_Awin_row_avx(poly *r, const poly *row, const ring_element *b,
 *                            int k, int32_t prime, int32_t primeinv)
 *
 * SysV ABI:
 *   rdi = r
 *   rsi = row, a contiguous array of k poly values
 *   rdx = b,   a ring_element with b->x[0..k-1]
 *   ecx = k
 *   r8d = prime
 *   r9d = primeinv
 *
 * Computes one Awin row dot product in NTT domain:
 *   r = sum_{j=0}^{k-1} row[j] .* b[j]
 *
 * The result matches k sequential poly_basemul_add32 calls starting from zero.
 */
.globl ring_mul_Awin_row_avx
.type ring_mul_Awin_row_avx, @function
ring_mul_Awin_row_avx:
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1          /* prime */
    vmovd xmm0, r9d
    vpbroadcastd ymm0, xmm0          /* primeinv */
    vpaddd ymm3, ymm1, ymm1          /* 2 * prime */

    xor eax, eax                     /* byte offset inside each poly */
.Lawin_row_block_loop:
    vpxor ymm12, ymm12, ymm12        /* accumulator for 8 coefficients */
    mov r10, rsi                     /* current row[j] */
    mov r11, rdx                     /* current b->x[j] */
    mov r9d, ecx                     /* j counter */

.Lawin_row_inner_loop:
    vmovdqu ymm4, YMMWORD PTR [r10 + rax]
    vmovdqu ymm5, YMMWORD PTR [r11 + rax]

    vpsrlq ymm6, ymm4, 32
    vpsrlq ymm7, ymm5, 32

    vpmuldq ymm4, ymm4, ymm5
    vpmuldq ymm6, ymm6, ymm7

    montgomery_reduce_qwords 4, 8
    montgomery_reduce_qwords 6, 8

    vpsllq ymm6, ymm6, 32
    vpor   ymm4, ymm4, ymm6

    vpaddd ymm12, ymm12, ymm4
    reduce_low_oct 12, 6, 7

    add r10, 512
    add r11, 512
    dec r9d
    jne .Lawin_row_inner_loop

    vmovdqu YMMWORD PTR [rdi + rax], ymm12

    add eax, 32
    cmp eax, 512
    jne .Lawin_row_block_loop

    ret
.size ring_mul_Awin_row_avx, .-ring_mul_Awin_row_avx

/*
 * K-specialized row-dot kernels. Same ABI as ring_mul_Awin_row_avx, but ecx is
 * ignored because k is fixed by the symbol.
 */
.globl ring_mul_Awin_row_k5_avx
.type ring_mul_Awin_row_k5_avx, @function
ring_mul_Awin_row_k5_avx:
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r9d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_row_k5_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    awin_raw_1row_dot_term 0
    awin_raw_1row_dot_term 512
    awin_raw_1row_dot_term 1024
    awin_raw_1row_dot_term 1536
    awin_raw_1row_dot_term 2048
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 5
    vmovdqu YMMWORD PTR [rdi + rax], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_row_k5_block_loop

    ret
.size ring_mul_Awin_row_k5_avx, .-ring_mul_Awin_row_k5_avx

.globl ring_mul_Awin_row_k9_avx
.type ring_mul_Awin_row_k9_avx, @function
ring_mul_Awin_row_k9_avx:
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r9d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_row_k9_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    awin_raw_1row_dot_term 0
    awin_raw_1row_dot_term 512
    awin_raw_1row_dot_term 1024
    awin_raw_1row_dot_term 1536
    awin_raw_1row_dot_term 2048
    awin_raw_1row_dot_term 2560
    awin_raw_1row_dot_term 3072
    awin_raw_1row_dot_term 3584
    awin_raw_1row_dot_term 4096
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 5
    vmovdqu YMMWORD PTR [rdi + rax], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_row_k9_block_loop

    ret
.size ring_mul_Awin_row_k9_avx, .-ring_mul_Awin_row_k9_avx

.globl ring_mul_Awin_row_k17_avx
.type ring_mul_Awin_row_k17_avx, @function
ring_mul_Awin_row_k17_avx:
    vmovd xmm1, r8d
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r9d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_row_k17_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    awin_raw_1row_dot_term 0
    awin_raw_1row_dot_term 512
    awin_raw_1row_dot_term 1024
    awin_raw_1row_dot_term 1536
    awin_raw_1row_dot_term 2048
    awin_raw_1row_dot_term 2560
    awin_raw_1row_dot_term 3072
    awin_raw_1row_dot_term 3584
    awin_raw_1row_dot_term 4096
    awin_raw_1row_dot_term 4608
    awin_raw_1row_dot_term 5120
    awin_raw_1row_dot_term 5632
    awin_raw_1row_dot_term 6144
    awin_raw_1row_dot_term 6656
    awin_raw_1row_dot_term 7168
    awin_raw_1row_dot_term 7680
    awin_raw_1row_dot_term 8192
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 5
    vmovdqu YMMWORD PTR [rdi + rax], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_row_k17_block_loop

    ret
.size ring_mul_Awin_row_k17_avx, .-ring_mul_Awin_row_k17_avx

.globl ring_mul_Awin_4rows_rev_k5_avx
.type ring_mul_Awin_4rows_rev_k5_avx, @function
ring_mul_Awin_4rows_rev_k5_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_4rows_rev_k5_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    vpxor ymm8, ymm8, ymm8
    vpxor ymm9, ymm9, ymm9
    vpxor ymm10, ymm10, ymm10
    vpxor ymm11, ymm11, ymm11
    awin_raw_4row_dot_term 0
    awin_raw_4row_dot_term 512
    awin_raw_4row_dot_term 1024
    awin_raw_4row_dot_term 1536
    awin_raw_4row_dot_term 2048
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 8
    awin_raw_reduce 5, 9
    awin_raw_reduce 6, 10
    awin_raw_reduce 7, 11
    vmovdqu YMMWORD PTR [rdi + rax], ymm7
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 1024], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 1536], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_4rows_rev_k5_block_loop

    ret
.size ring_mul_Awin_4rows_rev_k5_avx, .-ring_mul_Awin_4rows_rev_k5_avx

.globl ring_mul_Awin_4rows_rev_k9_avx
.type ring_mul_Awin_4rows_rev_k9_avx, @function
ring_mul_Awin_4rows_rev_k9_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_4rows_rev_k9_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    vpxor ymm8, ymm8, ymm8
    vpxor ymm9, ymm9, ymm9
    vpxor ymm10, ymm10, ymm10
    vpxor ymm11, ymm11, ymm11
    awin_raw_4row_dot_term 0
    awin_raw_4row_dot_term 512
    awin_raw_4row_dot_term 1024
    awin_raw_4row_dot_term 1536
    awin_raw_4row_dot_term 2048
    awin_raw_4row_dot_term 2560
    awin_raw_4row_dot_term 3072
    awin_raw_4row_dot_term 3584
    awin_raw_4row_dot_term 4096
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 8
    awin_raw_reduce 5, 9
    awin_raw_reduce 6, 10
    awin_raw_reduce 7, 11
    vmovdqu YMMWORD PTR [rdi + rax], ymm7
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 1024], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 1536], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_4rows_rev_k9_block_loop

    ret
.size ring_mul_Awin_4rows_rev_k9_avx, .-ring_mul_Awin_4rows_rev_k9_avx

.globl ring_mul_Awin_4rows_rev_k17_avx
.type ring_mul_Awin_4rows_rev_k17_avx, @function
ring_mul_Awin_4rows_rev_k17_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_4rows_rev_k17_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    vpxor ymm8, ymm8, ymm8
    vpxor ymm9, ymm9, ymm9
    vpxor ymm10, ymm10, ymm10
    vpxor ymm11, ymm11, ymm11
    awin_raw_4row_dot_term 0
    awin_raw_4row_dot_term 512
    awin_raw_4row_dot_term 1024
    awin_raw_4row_dot_term 1536
    awin_raw_4row_dot_term 2048
    awin_raw_4row_dot_term 2560
    awin_raw_4row_dot_term 3072
    awin_raw_4row_dot_term 3584
    awin_raw_4row_dot_term 4096
    awin_raw_4row_dot_term 4608
    awin_raw_4row_dot_term 5120
    awin_raw_4row_dot_term 5632
    awin_raw_4row_dot_term 6144
    awin_raw_4row_dot_term 6656
    awin_raw_4row_dot_term 7168
    awin_raw_4row_dot_term 7680
    awin_raw_4row_dot_term 8192
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 8
    awin_raw_reduce 5, 9
    awin_raw_reduce 6, 10
    awin_raw_reduce 7, 11
    vmovdqu YMMWORD PTR [rdi + rax], ymm7
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 1024], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 1536], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_4rows_rev_k17_block_loop

    ret
.size ring_mul_Awin_4rows_rev_k17_avx, .-ring_mul_Awin_4rows_rev_k17_avx

.globl ring_mul_Awin_5rows_rev_k5_avx
.type ring_mul_Awin_5rows_rev_k5_avx, @function
ring_mul_Awin_5rows_rev_k5_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_5rows_rev_k5_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    vpxor ymm8, ymm8, ymm8
    vpxor ymm9, ymm9, ymm9
    vpxor ymm10, ymm10, ymm10
    vpxor ymm11, ymm11, ymm11
    vpxor ymm12, ymm12, ymm12
    vpxor ymm13, ymm13, ymm13
    awin_raw_5row_dot_term 0
    awin_raw_5row_dot_term 512
    awin_raw_5row_dot_term 1024
    awin_raw_5row_dot_term 1536
    awin_raw_5row_dot_term 2048
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 9
    awin_raw_reduce 5, 10
    awin_raw_reduce 6, 11
    awin_raw_reduce 7, 12
    awin_raw_reduce 8, 13
    vmovdqu YMMWORD PTR [rdi + rax], ymm8
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm7
    vmovdqu YMMWORD PTR [rdi + rax + 1024], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 1536], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 2048], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_5rows_rev_k5_block_loop

    ret
.size ring_mul_Awin_5rows_rev_k5_avx, .-ring_mul_Awin_5rows_rev_k5_avx

.globl ring_mul_Awin_5rows_rev_k9_avx
.type ring_mul_Awin_5rows_rev_k9_avx, @function
ring_mul_Awin_5rows_rev_k9_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_5rows_rev_k9_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    vpxor ymm8, ymm8, ymm8
    vpxor ymm9, ymm9, ymm9
    vpxor ymm10, ymm10, ymm10
    vpxor ymm11, ymm11, ymm11
    vpxor ymm12, ymm12, ymm12
    vpxor ymm13, ymm13, ymm13
    awin_raw_5row_dot_term 0
    awin_raw_5row_dot_term 512
    awin_raw_5row_dot_term 1024
    awin_raw_5row_dot_term 1536
    awin_raw_5row_dot_term 2048
    awin_raw_5row_dot_term 2560
    awin_raw_5row_dot_term 3072
    awin_raw_5row_dot_term 3584
    awin_raw_5row_dot_term 4096
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 9
    awin_raw_reduce 5, 10
    awin_raw_reduce 6, 11
    awin_raw_reduce 7, 12
    awin_raw_reduce 8, 13
    vmovdqu YMMWORD PTR [rdi + rax], ymm8
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm7
    vmovdqu YMMWORD PTR [rdi + rax + 1024], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 1536], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 2048], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_5rows_rev_k9_block_loop

    ret
.size ring_mul_Awin_5rows_rev_k9_avx, .-ring_mul_Awin_5rows_rev_k9_avx

.globl ring_mul_Awin_5rows_rev_k17_avx
.type ring_mul_Awin_5rows_rev_k17_avx, @function
ring_mul_Awin_5rows_rev_k17_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_5rows_rev_k17_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    vpxor ymm8, ymm8, ymm8
    vpxor ymm9, ymm9, ymm9
    vpxor ymm10, ymm10, ymm10
    vpxor ymm11, ymm11, ymm11
    vpxor ymm12, ymm12, ymm12
    vpxor ymm13, ymm13, ymm13
    awin_raw_5row_dot_term 0
    awin_raw_5row_dot_term 512
    awin_raw_5row_dot_term 1024
    awin_raw_5row_dot_term 1536
    awin_raw_5row_dot_term 2048
    awin_raw_5row_dot_term 2560
    awin_raw_5row_dot_term 3072
    awin_raw_5row_dot_term 3584
    awin_raw_5row_dot_term 4096
    awin_raw_5row_dot_term 4608
    awin_raw_5row_dot_term 5120
    awin_raw_5row_dot_term 5632
    awin_raw_5row_dot_term 6144
    awin_raw_5row_dot_term 6656
    awin_raw_5row_dot_term 7168
    awin_raw_5row_dot_term 7680
    awin_raw_5row_dot_term 8192
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 9
    awin_raw_reduce 5, 10
    awin_raw_reduce 6, 11
    awin_raw_reduce 7, 12
    awin_raw_reduce 8, 13
    vmovdqu YMMWORD PTR [rdi + rax], ymm8
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm7
    vmovdqu YMMWORD PTR [rdi + rax + 1024], ymm6
    vmovdqu YMMWORD PTR [rdi + rax + 1536], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 2048], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_5rows_rev_k17_block_loop

    ret
.size ring_mul_Awin_5rows_rev_k17_avx, .-ring_mul_Awin_5rows_rev_k17_avx

.globl ring_mul_Awin_2rows_rev_k5_avx
.type ring_mul_Awin_2rows_rev_k5_avx, @function
ring_mul_Awin_2rows_rev_k5_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_2rows_rev_k5_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    awin_raw_2row_dot_term 0
    awin_raw_2row_dot_term 512
    awin_raw_2row_dot_term 1024
    awin_raw_2row_dot_term 1536
    awin_raw_2row_dot_term 2048
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 6
    awin_raw_reduce 5, 7
    vmovdqu YMMWORD PTR [rdi + rax], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_2rows_rev_k5_block_loop

    ret
.size ring_mul_Awin_2rows_rev_k5_avx, .-ring_mul_Awin_2rows_rev_k5_avx

.globl ring_mul_Awin_2rows_rev_k9_avx
.type ring_mul_Awin_2rows_rev_k9_avx, @function
ring_mul_Awin_2rows_rev_k9_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_2rows_rev_k9_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    awin_raw_2row_dot_term 0
    awin_raw_2row_dot_term 512
    awin_raw_2row_dot_term 1024
    awin_raw_2row_dot_term 1536
    awin_raw_2row_dot_term 2048
    awin_raw_2row_dot_term 2560
    awin_raw_2row_dot_term 3072
    awin_raw_2row_dot_term 3584
    awin_raw_2row_dot_term 4096
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 6
    awin_raw_reduce 5, 7
    vmovdqu YMMWORD PTR [rdi + rax], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_2rows_rev_k9_block_loop

    ret
.size ring_mul_Awin_2rows_rev_k9_avx, .-ring_mul_Awin_2rows_rev_k9_avx

.globl ring_mul_Awin_2rows_rev_k17_avx
.type ring_mul_Awin_2rows_rev_k17_avx, @function
ring_mul_Awin_2rows_rev_k17_avx:
    vmovd xmm1, ecx
    vpbroadcastd ymm1, xmm1
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vpaddd ymm3, ymm1, ymm1

    xor eax, eax
.Lawin_2rows_rev_k17_block_loop:
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    awin_raw_2row_dot_term 0
    awin_raw_2row_dot_term 512
    awin_raw_2row_dot_term 1024
    awin_raw_2row_dot_term 1536
    awin_raw_2row_dot_term 2048
    awin_raw_2row_dot_term 2560
    awin_raw_2row_dot_term 3072
    awin_raw_2row_dot_term 3584
    awin_raw_2row_dot_term 4096
    awin_raw_2row_dot_term 4608
    awin_raw_2row_dot_term 5120
    awin_raw_2row_dot_term 5632
    awin_raw_2row_dot_term 6144
    awin_raw_2row_dot_term 6656
    awin_raw_2row_dot_term 7168
    awin_raw_2row_dot_term 7680
    awin_raw_2row_dot_term 8192
    vpaddd ymm3, ymm1, ymm1
    awin_raw_reduce 4, 6
    awin_raw_reduce 5, 7
    vmovdqu YMMWORD PTR [rdi + rax], ymm5
    vmovdqu YMMWORD PTR [rdi + rax + 512], ymm4

    add eax, 32
    cmp eax, 512
    jne .Lawin_2rows_rev_k17_block_loop

    ret
.size ring_mul_Awin_2rows_rev_k17_avx, .-ring_mul_Awin_2rows_rev_k17_avx

.section .note.GNU-stack,"",@progbits
