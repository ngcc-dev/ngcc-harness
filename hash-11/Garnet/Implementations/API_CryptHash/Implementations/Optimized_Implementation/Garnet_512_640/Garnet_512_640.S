.intel_syntax noprefix
    .text

    .section .rodata
    .balign 16
C0: .quad 0x80DC1CD1C4C6628B, 0xC90FDAA22168C234
C1: .quad 0x3B139B22020BBEA6, 0x29024E088A67CC74
C2: .quad 0xCD3A431BEF9519B3, 0x514A08798E3404DD
C3: .quad 0x6D51C2454FE1356D, 0x302B0A6DF25F1437

RED_MC4: .quad 0x0, 0x87
RED_MD4: .quad 0x0000000000000000, 0x0000000020008005
MASK_LO: .quad 0x0000000000000000, 0xFFFFFFFFFFFFFFFF

FIB4:
    .quad 1,1,2, 3,5,8, 13,21,34, 55,89,144
    .quad 233,377,610, 987,1597,2584, 4181,6765,10946
    .quad 17711,28657,46368, 75025,121393,196418
    .quad 317811,514229,832040, 1346269,2178309,3524578
    .quad 5702887,9227465,14930352, 24157817,39088169,63245986
    .quad 102334155,165580141,267914296
    .quad 433494437,701408733,1134903170
    .quad 1836311903,2971215073,4807526976


ST_IV:
    .quad 0x311cceecf76770f1, 0xffdfea80e8157eb4
    .quad 0xa9af0b6d22d121bd, 0x486dae24482aa00a
    .quad 0x9f2fe6c33ab00be5, 0x306eda4994baba9c
    .quad 0x190a811b2f39580e, 0x5c2173082db5ddea
    .quad 0xf6818434230a38a3, 0xc4f59e2ab098316c
    .quad 0xb2f34e3108b93899, 0x8452307e9b13b8ae
    .quad 0x8cd00f5e6f965b39, 0xd6559e703c0ffad4
    .quad 0xb9bc125c20abb2a6, 0xf4139d666be07b74
    .quad 0xb9e97c07cb92a584, 0xf1c65f707fb1d7a7
    .quad 0x0c8649adbb445f7b, 0xa217af9dc3b9c169
    .quad 0xe33cd10720739e0a, 0x0dc0cdb2822afe55
    .quad 0x728e9fc267980396, 0xe060d07351d77ec8
    .quad 0xe900478b90718d8f, 0x8cc7e03f0c09af90
    .quad 0x533a33dac888ed08, 0x80dd4e1f759fa6b9
    .quad 0xd626dbde1d3bbe21, 0x454bf20854edf819
    .quad 0x1a9fe56ae1482d24, 0xa8dd28acb21d7197
    .text

.macro GF128_MUL_MC4 src t1 t2
    movdqa  \t1, \src
    psllq   \t1, 1
    movdqa  \t2, \src
    psrlq   \t2, 63
    pshufd  \t2, \t2, 0x4E
    lea     rcx, [rip + MASK_LO]
    pand    \t2, [rcx]
    pxor    \t1, \t2
    pextrq  rcx, \src, 1
    shr     rcx, 63
    lea     rax, [rip + RED_MC4]
    movq    \t2, [rax + rcx*8]
    pxor    \t1, \t2
    movdqa  \src, \t1
.endm

.macro GF128_MUL_MD4 src t1 t2
    movdqa  \t1, \src
    psllq   \t1, 1
    movdqa  \t2, \src
    psrlq   \t2, 63
    pshufd  \t2, \t2, 0x4E
    lea     rcx, [rip + MASK_LO]
    pand    \t2, [rcx]
    pxor    \t1, \t2
    pextrq  rcx, \src, 1
    shr     rcx, 63
    lea     rax, [rip + RED_MD4]
    movq    \t2, [rax + rcx*8]
    pxor    \t1, \t2
    movdqa  \src, \t1
.endm

/* MIX_COLUMN_4 — rdi=state, clobbers xmm0-xmm15, rcx, rax */
.macro MIX_COLUMN_4
    movdqa  xmm8,  [rdi + 0]
    movdqa  xmm9,  [rdi + 64]
    movdqa  xmm10, [rdi + 128]
    movdqa  xmm11, [rdi + 192]

    movdqa  xmm0, xmm8;   GF128_MUL_MC4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MC4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MC4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MC4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 0], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 64], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 128], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 192], xmm7

    movdqa  xmm8,  [rdi + 16]
    movdqa  xmm9,  [rdi + 80]
    movdqa  xmm10, [rdi + 144]
    movdqa  xmm11, [rdi + 208]

    movdqa  xmm0, xmm8;   GF128_MUL_MC4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MC4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MC4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MC4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 16], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 80], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 144], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 208], xmm7

    movdqa  xmm8,  [rdi + 32]
    movdqa  xmm9,  [rdi + 96]
    movdqa  xmm10, [rdi + 160]
    movdqa  xmm11, [rdi + 224]

    movdqa  xmm0, xmm8;   GF128_MUL_MC4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MC4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MC4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MC4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 32], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 96], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 160], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 224], xmm7

    movdqa  xmm8,  [rdi + 48]
    movdqa  xmm9,  [rdi + 112]
    movdqa  xmm10, [rdi + 176]
    movdqa  xmm11, [rdi + 240]

    movdqa  xmm0, xmm8;   GF128_MUL_MC4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MC4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MC4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MC4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 48], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 112], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 176], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 240], xmm7
.endm

.macro MIX_DIAGONAL_4
    movdqa  xmm8,  [rdi + 0]
    movdqa  xmm9,  [rdi + 80]
    movdqa  xmm10, [rdi + 160]
    movdqa  xmm11, [rdi + 240]

    movdqa  xmm0, xmm8;   GF128_MUL_MD4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MD4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MD4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MD4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 0], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 80], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 160], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 240], xmm7

    movdqa  xmm8,  [rdi + 16]
    movdqa  xmm9,  [rdi + 96]
    movdqa  xmm10, [rdi + 176]
    movdqa  xmm11, [rdi + 192]

    movdqa  xmm0, xmm8;   GF128_MUL_MD4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MD4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MD4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MD4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 16], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 96], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 176], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 192], xmm7

    movdqa  xmm8,  [rdi + 32]
    movdqa  xmm9,  [rdi + 112]
    movdqa  xmm10, [rdi + 128]
    movdqa  xmm11, [rdi + 208]

    movdqa  xmm0, xmm8;   GF128_MUL_MD4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MD4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MD4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MD4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 32], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 112], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 128], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 208], xmm7

    movdqa  xmm8,  [rdi + 48]
    movdqa  xmm9,  [rdi + 64]
    movdqa  xmm10, [rdi + 144]
    movdqa  xmm11, [rdi + 224]

    movdqa  xmm0, xmm8;   GF128_MUL_MD4 xmm0, xmm12, xmm13
    movdqa  xmm1, xmm9;   GF128_MUL_MD4 xmm1, xmm12, xmm14
    movdqa  xmm2, xmm10;  GF128_MUL_MD4 xmm2, xmm12, xmm15
    movdqa  xmm3, xmm11;  GF128_MUL_MD4 xmm3, xmm12, xmm13

    movdqa  xmm4, xmm0;   pxor xmm4, xmm1
    pxor    xmm4, xmm9;   pxor xmm4, xmm10;  pxor xmm4, xmm11
    movdqa  [rdi + 48], xmm4
    movdqa  xmm5, xmm8;   pxor xmm5, xmm1
    pxor    xmm5, xmm2;   pxor xmm5, xmm10;  pxor xmm5, xmm11
    movdqa  [rdi + 64], xmm5
    movdqa  xmm6, xmm8;   pxor xmm6, xmm9
    pxor    xmm6, xmm2;   pxor xmm6, xmm3;   pxor xmm6, xmm11
    movdqa  [rdi + 144], xmm6
    movdqa  xmm7, xmm0;   pxor xmm7, xmm8
    pxor    xmm7, xmm9;   pxor xmm7, xmm10;  pxor xmm7, xmm3
    movdqa  [rdi + 224], xmm7
.endm

    .balign 16
    .globl permutation_p_4x4_asm
    .type  permutation_p_4x4_asm, @function
permutation_p_4x4_asm:
    push    rbx
    lea     rdx, [rdi + 256]
.Lsub_loop:
    sub     rdx, 16
    movdqa  xmm1, [rdx]
    aesenc  xmm1, xmm0
    movdqa  [rdx], xmm1
    cmp     rdx, rdi
    ja      .Lsub_loop

    MIX_COLUMN_4

    mov     eax, esi
    imul    eax, 24
    lea     rcx, [rip + FIB4]
    mov     r8,  [rcx + rax]
    mov     r9,  [rcx + rax + 8]
    mov     r10, [rcx + rax + 16]
    movq    xmm1, r8
    movq    xmm2, r9
    movq    xmm3, r10

    movdqa  xmm15, [rdi + 112]

    pxor    xmm15, xmm1

    movdqa  [rdi + 112], xmm15
    movdqa  xmm15, [rdi + 176]
    pxor    xmm15, xmm2
    movdqa  [rdi + 176], xmm15
    movdqa  xmm15, [rdi + 240]
    pxor    xmm15, xmm3
    movdqa  [rdi + 240], xmm15

    MIX_DIAGONAL_4

    pop     rbx
    ret
    .size permutation_p_4x4_asm, .-permutation_p_4x4_asm

    .balign 16
    .globl garnet_512
    .type  garnet_512, @function
garnet_512:
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    push    rbp                      # 保护 rbp (ebp) (6个寄存器共48字节)
    # 移除 sub rsp, 8。这可以保证 [rsp] 加上后续压入的 8 字节 rax 后，栈保持 16 字节对齐

    mov     r12, rdi                 # r12 = message pointer
    mov     r13, rsi                 # r13 = mlength (bit_len)
    mov     r14, rdx                 # r14 = digest pointer
    mov     r15, rcx                 # r15 = state pointer

    # 计算 byte_num = (mlength + 7) >> 3
    mov     rax, r13
    add     rax, 7
    shr     rax, 3
    push    rax                      # [rsp] = byte_num

    # 初始化 state (ST_IV 拷贝)
    lea     rsi, [rip + ST_IV]
    mov     rdi, r15
    mov     ecx, 32
    rep movsq

    xor     ebx, ebx                 # ebx = bytes consumed (i)

.L_absorb:
    mov     rax, [rsp]
    lea     rcx, [rbx + 80]
    cmp     rcx, rax
    ja      .L_absorb_done

    movdqu  xmm0, [r12 + rbx + 0]
    movdqu  xmm1, [r12 + rbx + 16]
    movdqu  xmm2, [r12 + rbx + 32]
    movdqu  xmm3, [r12 + rbx + 48]
    movdqu  xmm4, [r12 + rbx + 64]

    movdqa  xmm15, [r15 + 0]
    pxor    xmm15, xmm0
    movdqa  [r15 + 0], xmm15


    movdqa  xmm15, [r15 + 80]
    pxor    xmm15, xmm1
    movdqa  [r15 + 80], xmm15

    movdqa  xmm15, [r15 + 160]
    pxor    xmm15, xmm2
    movdqa  [r15 + 160], xmm15

    movdqa  xmm15, [r15 + 192]
    pxor    xmm15, xmm3
    movdqa  [r15 + 192], xmm15

    movdqa  xmm15, [r15 + 240]
    pxor    xmm15, xmm4
    movdqa  [r15 + 240], xmm15

    xor     ebp, ebp                 # ebp 作为吸收置换计数器
.L_perm_a:
    mov     rdi, r15
    pxor    xmm0, xmm0               # zero_key
    mov     esi, ebp
    call    permutation_p_4x4_asm
    inc     ebp
    cmp     ebp, 10
    jb      .L_perm_a

    add     ebx, 80
    jmp     .L_absorb

.L_absorb_done:
    # ── 残缺块逻辑 ──
    sub     rsp, 80
    pxor    xmm0, xmm0
    movdqa  [rsp + 0], xmm0
    movdqa  [rsp + 16], xmm0
    movdqa  [rsp + 32], xmm0
    movdqa  [rsp + 48], xmm0
    movdqa  [rsp + 64], xmm0

    mov     rsi, rsp
    mov     rax, [rsp + 80]

.L_partial_16:
    lea     rdx, [rbx + 16]
    cmp     rdx, rax
    ja      .L_partial_last
    movdqu  xmm0, [r12 + rbx]
    movdqa  [rsi], xmm0
    add     rsi, 16
    add     rbx, 16
    jmp     .L_partial_16

.L_partial_last:
    test    al, 0xf
    jz      .L_partial_test_padding

    sub     rsp, 16
    pxor    xmm0, xmm0
    movdqa  [rsp], xmm0

    mov     rcx, rax
    and     rcx, 0xf

    mov     rdi, rsp
    lea     rdx, [r12 + rbx]
.L_copy_last_bytes:
    mov     r8b, [rdx]
    mov     [rdi], r8b
    inc     rdx
    inc     rdi
    dec     rcx
    jnz     .L_copy_last_bytes

    movdqa  xmm0, [rsp]
    add     rsp, 16
    movdqa  [rsi], xmm0
    jmp     .L_partial_absorb

.L_partial_test_padding:
    test    r13b, 7
    jnz     .L_partial_absorb
    mov     byte ptr [rsi], 0x80

.L_partial_absorb:
    movdqa  xmm15, [r15 + 0]
    pxor    xmm15, [rsp]
    movdqa  [r15 + 0], xmm15


    movdqa  xmm15, [r15 + 80]
    pxor    xmm15, [rsp + 16]
    movdqa  [r15 + 80], xmm15

    movdqa  xmm15, [r15 + 160]
    pxor    xmm15, [rsp + 32]
    movdqa  [r15 + 160], xmm15

    movdqa  xmm15, [r15 + 192]
    pxor    xmm15, [rsp + 48]
    movdqa  [r15 + 192], xmm15

    movdqa  xmm15, [r15 + 240]
    pxor    xmm15, [rsp + 64]
    movdqa  [r15 + 240], xmm15

    xor     ebp, ebp                 # ebp 重新计数
.L_perm_partial:
    mov     rdi, r15
    pxor    xmm0, xmm0
    mov     esi, ebp
    call    permutation_p_4x4_asm
    inc     ebp
    cmp     ebp, 10
    jb      .L_perm_partial

    add     rsp, 80

.L_partial_done:
    # add     rsp, 8                   # 还原由 push rax 占用的 8 字节
    # pop     rbp                      # 弹出恢复 rbp
    # pop     r15
    # pop     r14
    # pop     r13
    # pop     r12
    # pop     rbx
    # ret

    movq    xmm0, r13
    movdqa  xmm15, [r15 + 0]
    pxor    xmm15, xmm0
    movdqa  [r15 + 0], xmm15

    movdqa  xmm15, [r15 + 80]
    pxor    xmm15, xmm0
    movdqa  [r15 + 80], xmm15

    movdqa  xmm15, [r15 + 160]
    pxor    xmm15, xmm0
    movdqa  [r15 + 160], xmm15

    movdqa  xmm15, [r15 + 240]
    pxor    xmm15, xmm0
    movdqa  [r15 + 240], xmm15

    xor     ebp, ebp                 # 中间置换阶段 (14轮，key=1)
.L_sq1:
    mov     rdi, r15
    mov     rax, 1
    movq    xmm0, rax
    mov     esi, ebp
    call    permutation_p_4x4_asm
    inc     ebp
    cmp     ebp, 14
    jb      .L_sq1

    xor     ebp, ebp                 # 挤出阶段 (10轮，key=2)
.L_sq2:
    mov     rdi, r15
    mov     rax, 2
    movq    xmm0, rax
    mov     esi, ebp
    call    permutation_p_4x4_asm
    inc     ebp
    cmp     ebp, 10
    jb      .L_sq2

    # 提取摘要
    movdqu  xmm0, [r15 + 0]
    movdqu  [r14 + 0], xmm0
    movdqu  xmm0, [r15 + 112]
    movdqu  [r14 + 16], xmm0
    movdqu  xmm0, [r15 + 144]
    movdqu  [r14 + 32], xmm0
    movdqu  xmm0, [r15 + 224]
    movdqu  [r14 + 48], xmm0

    add     rsp, 8                   # 还原由 push rax 占用的 8 字节
    pop     rbp                      # 弹出恢复 rbp
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret
    .size garnet_512, .-garnet_512

.global pad_message
.type pad_message, @function
pad_message:
    mov rcx, rsi
    and rcx, 7                # rcx = bit_idx = bit_len % 8
    mov rax, rsi
    shr rax, 3                # rax = bit_len / 8

    test rcx, rcx             # 判断 bit_idx 是否为 0
    jnz .L_bit_idx_not_zero

    # 情况 A：bit_idx == 0
    # 此时只需在 msg[rax] 处写入 0x80
    mov byte ptr [rdi + rax], 0x80
    ret

.L_bit_idx_not_zero:
    # 情况 B：bit_idx != 0
    mov r9, rcx               # r9 = 保存原始的 bit_idx
    movzx rdx, byte ptr [rdi + rax] # rdx = msg[rax] (零扩展读取)

    # 1. 右移 (8 - bit_idx)
    mov rcx, 8
    sub rcx, r9               # rcx = 8 - r9
    shr dl, cl                # dl >>= cl (cl 对应 rcx 的低8位)

    # 追加 '1' 比特
    shl dl, 1
    or dl, 1

    # 左移 (7 - bit_idx)
    mov rcx, 7
    sub rcx, r9               # rcx = 7 - r9
    shl dl, cl                # dl <<= cl

    # 写回缓冲区
    mov byte ptr [rdi + rax], dl
    ret


# === Diagnostic Wrapper Functions ===

    .globl test_gf128_mul_mc4
    .type test_gf128_mul_mc4, @function
test_gf128_mul_mc4:
    GF128_MUL_MC4 xmm0, xmm1, xmm2
    ret

    .globl test_gf128_mul_md4
    .type test_gf128_mul_md4, @function
test_gf128_mul_md4:
    GF128_MUL_MD4 xmm0, xmm1, xmm2
    ret

    .globl test_mix_column
    .type test_mix_column, @function
test_mix_column:
    MIX_COLUMN_4
    ret

    .globl test_mix_diagonal
    .type test_mix_diagonal, @function
test_mix_diagonal:
    MIX_DIAGONAL_4
    ret

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif