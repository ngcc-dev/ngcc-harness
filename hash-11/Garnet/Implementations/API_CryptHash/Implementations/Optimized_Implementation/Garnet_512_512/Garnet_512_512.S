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

/* 【修正】全新 Garnet-512 (counter0 规范) 预计算初始状态 ST_IV (.quad low, high) */
ST_IV:
    .quad 0x782b1463e5b448c2, 0xde30d1b89ddaa011    # state[0]
    .quad 0xd92183bde01eaf39, 0x634ee0b3bf88f484    # state[1]
    .quad 0xf8fb07c0f0d96f62, 0x149d3ee603859d70    # state[2]
    .quad 0x4c92e211237707ef, 0xf48dce216aa60cac    # state[3]
    .quad 0xd2543469c3db12b7, 0x7396d12e4c6dbbb7    # state[4]
    .quad 0x6d127c0b76361e62, 0x8a6a75f1e6bbc959    # state[5]
    .quad 0x833bfb64f3bce994, 0x530cf1efa5661a70    # state[6]
    .quad 0x0e267b18c6f6db4c, 0xb0389a5d5e9c9970    # state[7]
    .quad 0x05239d1166b906cb, 0x67117fbd723084d2    # state[8]
    .quad 0xe907c5dd6ca25474, 0xcbbac776493d5c27    # state[9]
    .quad 0x6d0064b8f5707068, 0xaff5e27600e5bc3f    # state[10]
    .quad 0xc58584622d4b401e, 0x9b1ac6e440a1ef11    # state[11]
    .quad 0x42e77c2f23ac28fb, 0x92918651c356f4b8    # state[12]
    .quad 0x09e68e582255f277, 0x7e5c987ec3973036    # state[13]
    .quad 0x87947a0718e95f65, 0x8b07f08ed4e24d6c    # state[14]
    .quad 0xf742ee05141a3f41, 0xfa7f68f3b89fc0c7    # state[15]

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
    lea     rcx, [rbx + 64]          # 【修改】速率调整为符合规范的 512 位 (64 字节)
    cmp     rcx, rax
    ja      .L_absorb_done

    movdqu  xmm0, [r12 + rbx + 0]    # 【修改】仅加载对角线吸入的 4 个 128 位消息分组
    movdqu  xmm1, [r12 + rbx + 16]
    movdqu  xmm2, [r12 + rbx + 32]
    movdqu  xmm3, [r12 + rbx + 48]

    movdqa  xmm15, [r15 + 0]         # 【修改】标准对角线吸收模式：
    pxor    xmm15, xmm0              # state[0]  ^= msg[0]
    movdqa  [r15 + 0], xmm15

    movdqa  xmm15, [r15 + 80]        # state[5]  ^= msg[1]
    pxor    xmm15, xmm1
    movdqa  [r15 + 80], xmm15

    movdqa  xmm15, [r15 + 160]       # state[10] ^= msg[2]
    pxor    xmm15, xmm2
    movdqa  [r15 + 160], xmm15

    movdqa  xmm15, [r15 + 240]       # state[15] ^= msg[3]
    pxor    xmm15, xmm3
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

    add     ebx, 64                  # 【修改】主吸收步长缩减为 64 字节
    jmp     .L_absorb

.L_absorb_done:
    # ── 残缺块逻辑 ──
    sub     rsp, 64                  # 【修改】局部栈帧大小从 128 缩减为 64
    pxor    xmm0, xmm0
    movdqa  [rsp + 0], xmm0          # 【修改】仅需对 64 字节缓冲区进行置零
    movdqa  [rsp + 16], xmm0
    movdqa  [rsp + 32], xmm0
    movdqa  [rsp + 48], xmm0

    mov     rsi, rsp
    mov     rax, [rsp + 64]          # 【修改】 byte_num 重载偏移由 128 变更为 64

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
    # 【修改】删除了原本多余的 inc rcx，使之安全拷贝 byte_num & 0xf 字节，防止越界/混入噪声

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
    movdqa  xmm15, [r15 + 0]         # 【修改】对角线吸收模式：
    pxor    xmm15, [rsp]             # state[0]  ^= msg[0]
    movdqa  [r15 + 0], xmm15

    movdqa  xmm15, [r15 + 80]        # state[5]  ^= msg[1]
    pxor    xmm15, [rsp + 16]
    movdqa  [r15 + 80], xmm15

    movdqa  xmm15, [r15 + 160]       # state[10] ^= msg[2]
    pxor    xmm15, [rsp + 32]
    movdqa  [r15 + 160], xmm15

    movdqa  xmm15, [r15 + 240]       # state[15] ^= msg[3]
    pxor    xmm15, [rsp + 48]
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

    add     rsp, 64                  # 【修改】释放栈空间从 128 缩减为 64

.L_partial_done:
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

    xor     ebp, ebp                 # 中间处理阶段 (14轮，key=1)
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
    movdqu  [r14 + 0], xmm0          # digest[0]
    movdqu  xmm0, [r15 + 112]
    movdqu  [r14 + 16], xmm0         # digest[16]
    movdqu  xmm0, [r15 + 144]
    movdqu  [r14 + 32], xmm0         # digest[32]
    movdqu  xmm0, [r15 + 224]
    movdqu  [r14 + 48], xmm0         # digest[48]

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