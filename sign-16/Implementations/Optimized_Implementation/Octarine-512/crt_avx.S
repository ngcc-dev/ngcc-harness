.intel_syntax noprefix

.macro crt8 off
    vmovdqu ymm0, YMMWORD PTR [rsi + rax + \off]   # a mod p1
    vmovdqu ymm1, YMMWORD PTR [rdx + rax + \off]   # b mod p2
    vpsubd ymm2, ymm1, ymm0                        # d = b - a

    # t = (b-a) * p1^{-1} mod p2.
    # p1^{-1} * 2^32 mod p2 is 0x80000, so one Montgomery reduction is enough.
    # m = low32((d << 19) * -p2^{-1}) = low32(d * 0xbff80000).
    vpmulld ymm5, ymm2, ymm14

    vpmuldq ymm3, ymm5, ymm13                      # even lanes: m * p2
    vpsrlq  ymm4, ymm5, 32
    vpmuldq ymm4, ymm4, ymm13                      # odd lanes: m * p2
    vpsrlq  ymm3, ymm3, 32
    vpsrlq  ymm4, ymm4, 32
    vpsllq ymm4, ymm4, 32
    vpblendd ymm5, ymm3, ymm4, 0xaa                # high32(m * p2)

    vpsrad ymm6, ymm2, 13                          # high32(d << 19)
    vpaddd ymm5, ymm5, ymm6
    vpslld ymm6, ymm2, 19                          # carry from low32((d << 19) + m*p2)
    vpcmpeqd ymm6, ymm6, YMMWORD PTR [rip + zero_vec]
    vpandn ymm6, ymm6, ymm15
    vpaddd ymm5, ymm5, ymm6                        # signed Montgomery result

    # The Montgomery result is already centered.  If t were normalized with
    # t += p2, centering would subtract p1*p2; these cancel modulo q.
    vpmulld ymm6, ymm5, ymm12
    vpaddd  ymm6, ymm6, ymm0

    vpaddd ymm6, ymm6, ymm9
    vpand  ymm6, ymm6, ymm8
    vpsubd ymm6, ymm6, ymm9

    vmovdqu YMMWORD PTR [rdi + rax + \off], ymm6
.endm

.text
.globl poly_crt_avx
.type poly_crt_avx, @function
poly_crt_avx:
    vmovdqa ymm15, YMMWORD PTR [rip + one_vec]
    vmovdqa ymm14, YMMWORD PTR [rip + mont_mconst_vec]
    vmovdqa ymm13, YMMWORD PTR [rip + p2_vec]
    vmovdqa ymm12, YMMWORD PTR [rip + p1_modq_vec]
    vmovdqa ymm9,  YMMWORD PTR [rip + q_half_vec]
    vmovdqa ymm8,  YMMWORD PTR [rip + q_mask_vec]

    xor eax, eax
.p2align 5
.Lcrt_loop:
    crt8 0
    crt8 32
    add rax, 64
    cmp rax, 4096
    jb .Lcrt_loop

    vzeroupper
    ret
.size poly_crt_avx, .-poly_crt_avx

.section .rodata
.p2align 5
mont_mconst_vec:
    .long 0xbff80000, 0xbff80000, 0xbff80000, 0xbff80000
    .long 0xbff80000, 0xbff80000, 0xbff80000, 0xbff80000
p2_vec:
    .long 0x3fff5801, 0x3fff5801, 0x3fff5801, 0x3fff5801
    .long 0x3fff5801, 0x3fff5801, 0x3fff5801, 0x3fff5801
p1_modq_vec:
    .long 0x00ff7801, 0x00ff7801, 0x00ff7801, 0x00ff7801
    .long 0x00ff7801, 0x00ff7801, 0x00ff7801, 0x00ff7801
q_half_vec:
    .long 0x00800000, 0x00800000, 0x00800000, 0x00800000
    .long 0x00800000, 0x00800000, 0x00800000, 0x00800000
q_mask_vec:
    .long 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff
    .long 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff
one_vec:
    .long 0x00000001, 0x00000001, 0x00000001, 0x00000001
    .long 0x00000001, 0x00000001, 0x00000001, 0x00000001
zero_vec:
    .long 0x00000000, 0x00000000, 0x00000000, 0x00000000
    .long 0x00000000, 0x00000000, 0x00000000, 0x00000000

.section .note.GNU-stack,"",@progbits
