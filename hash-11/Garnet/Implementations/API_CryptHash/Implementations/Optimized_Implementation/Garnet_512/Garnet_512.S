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
    .quad 0xce4f49d38fe0830a, 0xa4b8433d99d491ed    # state[0]
    .quad 0x4ca1daedfbff54fa, 0x73317b70d2b67ab3    # state[1]
    .quad 0xa70e9223fbb3de14, 0xb9dbff2ff29f4466    # state[2]
    .quad 0x43cd103aecc958c5, 0x01df8af21eceedf5    # state[3]  (已修正)
    .quad 0x4d165d1cd86cc8d4, 0x6a32489ae800f834    # state[4]
    .quad 0x05f9522bf88d6a96, 0x2224c4159c4951cc    # state[5]
    .quad 0xa1d714b1ac3c0e6c, 0x6db6b4e6b6337716    # state[6]
    .quad 0x661d5963f16a276a, 0x681866d4c5ca710d    # state[7]
    .quad 0x57133677381d8b01, 0xf68990ae9cf6618e    # state[8]
    .quad 0x4ce6b5a186f5b5ce, 0xfee8f188dc038a49    # state[9]
    .quad 0x5e9cf253dcd91cfd, 0x577a951517e7a95a    # state[10]
    .quad 0x92de402ccb02f387, 0x40dfc12faa2d68d5    # state[11]
    .quad 0xab13fd4cbb26505d, 0xd82968bd47091988    # state[12] (已修正)
    .quad 0xf72e1a744a39c552, 0x5d459579a983f285    # state[13] (已修正)
    .quad 0x3a8a941b569abc22, 0xcae4079778f0af79    # state[14] (已修正)
    .quad 0x6fe36fc7a21df058, 0xe45f3a0f93bdfaa6    # state[15]

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
    lea     rcx, [rbx + 128]
    cmp     rcx, rax
    ja      .L_absorb_done

    movdqu  xmm0, [r12 + rbx + 0]
    movdqu  xmm1, [r12 + rbx + 16]
    movdqu  xmm2, [r12 + rbx + 32]
    movdqu  xmm3, [r12 + rbx + 48]
    movdqu  xmm4, [r12 + rbx + 64]
    movdqu  xmm5, [r12 + rbx + 80]
    movdqu  xmm6, [r12 + rbx + 96]
    movdqu  xmm7, [r12 + rbx + 112]

    movdqa  xmm15, [r15 + 0]
    pxor    xmm15, xmm0
    movdqa  [r15 + 0], xmm15

    movdqa  xmm15, [r15 + 48]
    pxor    xmm15, xmm1
    movdqa  [r15 + 48], xmm15

    movdqa  xmm15, [r15 + 80]
    pxor    xmm15, xmm2
    movdqa  [r15 + 80], xmm15

    movdqa  xmm15, [r15 + 96]
    pxor    xmm15, xmm3
    movdqa  [r15 + 96], xmm15

    movdqa  xmm15, [r15 + 144]
    pxor    xmm15, xmm4
    movdqa  [r15 + 144], xmm15

    movdqa  xmm15, [r15 + 160]
    pxor    xmm15, xmm5
    movdqa  [r15 + 160], xmm15

    movdqa  xmm15, [r15 + 192]
    pxor    xmm15, xmm6
    movdqa  [r15 + 192], xmm15

    movdqa  xmm15, [r15 + 240]
    pxor    xmm15, xmm7
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

    add     ebx, 128
    jmp     .L_absorb

.L_absorb_done:
    # ── 残缺块逻辑 ──
    sub     rsp, 128
    pxor    xmm0, xmm0
    movdqa  [rsp + 0], xmm0
    movdqa  [rsp + 16], xmm0
    movdqa  [rsp + 32], xmm0
    movdqa  [rsp + 48], xmm0
    movdqa  [rsp + 64], xmm0
    movdqa  [rsp + 80], xmm0
    movdqa  [rsp + 96], xmm0
    movdqa  [rsp + 112], xmm0

    mov     rsi, rsp
    mov     rax, [rsp + 128]

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
    inc     rcx

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

    movdqa  xmm15, [r15 + 48]
    pxor    xmm15, [rsp + 16]
    movdqa  [r15 + 48], xmm15

    movdqa  xmm15, [r15 + 80]
    pxor    xmm15, [rsp + 32]
    movdqa  [r15 + 80], xmm15

    movdqa  xmm15, [r15 + 96]
    pxor    xmm15, [rsp + 48]
    movdqa  [r15 + 96], xmm15

    movdqa  xmm15, [r15 + 144]
    pxor    xmm15, [rsp + 64]
    movdqa  [r15 + 144], xmm15

    movdqa  xmm15, [r15 + 160]
    pxor    xmm15, [rsp + 80]
    movdqa  [r15 + 160], xmm15

    movdqa  xmm15, [r15 + 192]
    pxor    xmm15, [rsp + 96]
    movdqa  [r15 + 192], xmm15

    movdqa  xmm15, [r15 + 240]
    pxor    xmm15, [rsp + 112]
    movdqa  [r15 + 240], xmm15
# 除个别状态外，几乎所有状态的初始 IV 拷贝都已经对齐。然而，state[3]、state[12]、state[13]、state[14] 发生了 64 位的高低位颠倒
    #add     rsp, 128                 # 释放局部 msg 数组
    #add     rsp, 8                   # 还原由 push rax 占用的 8 字节
    #pop     rbp                      # 弹出恢复 rbp
    #pop     r15
    #pop     r14
    #pop     r13
    #pop     r12
    #pop     rbx
    #ret

    xor     ebp, ebp                 # ebp 重新计数
.L_perm_partial:
    mov     rdi, r15
    pxor    xmm0, xmm0
    mov     esi, ebp
    call    permutation_p_4x4_asm
    inc     ebp
    cmp     ebp, 10
    jb      .L_perm_partial

    add     rsp, 128

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