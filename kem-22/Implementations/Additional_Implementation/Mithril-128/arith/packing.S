.intel_syntax noprefix

.text
.globl poly_pack_2_16_avx2
.type poly_pack_2_16_avx2, @function

# void poly_pack_2_16_avx2(unsigned char *b, const poly *r)
# rdi = b
# rsi = r
# assume r->coeffs offset = 0, uint16_t coeffs[128]

poly_pack_2_16_avx2:
    # ymm6 = 1
    # ymm7 = 3
    vpcmpeqw ymm6, ymm6, ymm6
    vpsrlw   ymm6, ymm6, 15
    vpsllw   ymm7, ymm6, 1
    vpor     ymm7, ymm7, ymm6

.macro PACK16 inoff, outoff
    vmovdqu  ymm0, YMMWORD PTR [rsi + \inoff]

    # c = (1 - coeff) & 3 = (coeff & 3) ^ 1
    vpand    ymm0, ymm0, ymm7
    vpxor    ymm0, ymm0, ymm6

    # word = (c << 7) | (c << 14)
    vpsllw   ymm1, ymm0, 7
    vpsllw   ymm0, ymm0, 14
    vpor     ymm0, ymm0, ymm1

    # 16 个 2-bit c → 4 字节
    vpmovmskb eax, ymm0
    mov      DWORD PTR [rdi + \outoff], eax
.endm

    PACK16 0,   0
    PACK16 32,  4
    PACK16 64,  8
    PACK16 96,  12
    PACK16 128, 16
    PACK16 160, 20
    PACK16 192, 24
    PACK16 224, 28

    vzeroupper
    ret

.size poly_pack_2_16_avx2, .-poly_pack_2_16_avx2

.text
.globl poly_pack_11_16_avx2
.type poly_pack_11_16_avx2, @function

# void poly_pack_11_16_avx2(unsigned char *b, const poly *r)
# SysV ABI:
#   rdi = b
#   rsi = r
#
# Assumption:
#   r->coeffs starts at offset 0
#   coeffs are uint16_t coeffs[128]

poly_pack_11_16_avx2:
    # ymm6 = 1023 = 0x03ff
    # ymm7 = 2047 = 0x07ff
    # ymm5 = [1, 2048, 1, 2048, ...] as uint16_t words
    vpcmpeqw ymm4, ymm4, ymm4      # ymm4 = 0xffff

    vpsrlw   ymm6, ymm4, 6         # 0x03ff = 1023
    vpsrlw   ymm7, ymm4, 5         # 0x07ff = 2047

    vpsrlw   ymm1, ymm4, 15        # 1
    vpsllw   ymm2, ymm1, 11        # 2048
    vpblendw ymm5, ymm1, ymm2, 0xaa # word pattern: 1,2048,1,2048,...
    vmovdqu  ymm8, YMMWORD PTR [rip + .Lqmask_low32]
    vmovdqu  ymm9, YMMWORD PTR [rip + .Lqmask_high32]

.macro STORE11IMG_LAST xreg, outoff
    vmovq   r8, \xreg
    vpextrd r9d, \xreg, 2
    mov     QWORD PTR [rdi + \outoff], r8
    mov     WORD PTR [rdi + \outoff + 8], r9w
    shr     r9d, 16
    mov     BYTE PTR [rdi + \outoff + 10], r9b
.endm

.macro PACK16_11_Q inoff, outoff
    vmovdqu  ymm0, YMMWORD PTR [rsi + \inoff]
    vpand    ymm0, ymm0, ymm7
    vpxor    ymm0, ymm0, ymm6       # (1023 - coeff) mod 2048 = coeff ^ 1023
    vpmaddwd ymm0, ymm0, ymm5       # P0=c0|c1<<11, P1=c2|c3<<11, ...

    # Pack each adjacent pair of 22-bit P values into one 44-bit qword:
    # q = P_even | (P_odd << 22).  ymm0 qwords are [P_even | P_odd<<32].
    vpand    ymm2, ymm0, ymm8       # low dword = P_even
    vpand    ymm3, ymm0, ymm9       # high dword = P_odd << 32
    vpsrlq   ymm3, ymm3, 10         # P_odd << 22
    vpor     ymm2, ymm2, ymm3

    # Build two 16-byte store images per YMM lane:
    #   q_even | (q_odd << 44), q_odd >> 20
    # Only the first 11 bytes are live.  The extra bytes are overwritten by
    # the next overlapped store.
    vpsrldq  ymm3, ymm2, 8
    vpsllq   ymm3, ymm3, 44
    vpor     ymm3, ymm3, ymm2
    vpsrlq   ymm4, ymm2, 20
    vpblendd ymm3, ymm3, ymm4, 0xcc
    vmovdqu  XMMWORD PTR [rdi + \outoff], xmm3
    vextracti128 XMMWORD PTR [rdi + \outoff + 11], ymm3, 1
.endm

.macro PACK16_11_Q_FINAL inoff, outoff
    vmovdqu  ymm0, YMMWORD PTR [rsi + \inoff]
    vpand    ymm0, ymm0, ymm7
    vpxor    ymm0, ymm0, ymm6
    vpmaddwd ymm0, ymm0, ymm5
    vpand    ymm2, ymm0, ymm8
    vpand    ymm3, ymm0, ymm9
    vpsrlq   ymm3, ymm3, 10
    vpor     ymm2, ymm2, ymm3
    vpsrldq  ymm3, ymm2, 8
    vpsllq   ymm3, ymm3, 44
    vpor     ymm3, ymm3, ymm2
    vpsrlq   ymm4, ymm2, 20
    vpblendd ymm3, ymm3, ymm4, 0xcc
    vmovdqu  XMMWORD PTR [rdi + \outoff], xmm3
    vextracti128 xmm4, ymm3, 1
    STORE11IMG_LAST xmm4, (\outoff + 11)
.endm

    PACK16_11_Q 0,   0
    PACK16_11_Q 32,  22
    PACK16_11_Q 64,  44
    PACK16_11_Q 96,  66
    PACK16_11_Q 128, 88
    PACK16_11_Q 160, 110
    PACK16_11_Q 192, 132
    PACK16_11_Q_FINAL 224, 154

    vzeroupper
    ret

.size poly_pack_11_16_avx2, .-poly_pack_11_16_avx2

.text
.globl poly_unpack_2_16_avx2
.type poly_unpack_2_16_avx2, @function

# void poly_unpack_2_16_avx2(poly *r, const unsigned char *b)
# SysV ABI:
#   rdi = r
#   rsi = b
#
# Assumption:
#   r->coeffs starts at offset 0
#   coeffs are uint16_t coeffs[128]
#
# Implements:
#   t0 =  x       & 3
#   t1 = (x >> 2) & 3
#   t2 = (x >> 4) & 3
#   t3 = (x >> 6) & 3
#   coeff = (1 - t) & (RRLWR_PKE_Q - 1)

poly_unpack_2_16_avx2:
    # ymm15 = 1
    vpcmpeqw ymm15, ymm15, ymm15
    vpsrlw   ymm15, ymm15, 15

    # ymm14 = 3
    vpsllw   ymm14, ymm15, 1
    vpor     ymm14, ymm14, ymm15

    # ymm13 = qmask = 8191
    mov      eax, 8191
    vmovd    xmm13, eax
    vpbroadcastw ymm13, xmm13

# Decode 16 packed bytes into 64 uint16_t coefficients.
#
# Input:
#   b[inoff .. inoff+15]
#
# Output:
#   r->coeffs[outoff/2 .. outoff/2+63]
#
.macro UNPACK16B_2 inoff, outoff
    # ymm0 = 16 bytes zero-extended to 16 uint16_t words
    # word j contains packed byte x_j
    vpmovzxbw ymm0, XMMWORD PTR [rsi + \inoff]

    # Extract four 2-bit streams.
    #
    # ymm1 = x & 3
    # ymm2 = (x >> 2) & 3
    # ymm3 = (x >> 4) & 3
    # ymm4 = (x >> 6) & 3
    vmovdqa ymm1, ymm0
    vpand   ymm1, ymm1, ymm14

    vmovdqa ymm2, ymm0
    vpsrlw  ymm2, ymm2, 2
    vpand   ymm2, ymm2, ymm14

    vmovdqa ymm3, ymm0
    vpsrlw  ymm3, ymm3, 4
    vpand   ymm3, ymm3, ymm14

    vpsrlw  ymm4, ymm0, 6

    # coeff = (1 - t) & qmask
    vpsubw  ymm1, ymm15, ymm1
    vpand   ymm1, ymm1, ymm13

    vpsubw  ymm2, ymm15, ymm2
    vpand   ymm2, ymm2, ymm13

    vpsubw  ymm3, ymm15, ymm3
    vpand   ymm3, ymm3, ymm13

    vpsubw  ymm4, ymm15, ymm4
    vpand   ymm4, ymm4, ymm13

    # Now:
    #   ymm1 = coeff0 of each input byte
    #   ymm2 = coeff1 of each input byte
    #   ymm3 = coeff2 of each input byte
    #   ymm4 = coeff3 of each input byte
    #
    # Need memory order:
    #   byte0: c0,c1,c2,c3
    #   byte1: c0,c1,c2,c3
    #   ...

    # Interleave c0/c1 and c2/c3 as words.
    vpunpcklwd ymm5, ymm1, ymm2
    vpunpckhwd ymm6, ymm1, ymm2

    vpunpcklwd ymm7, ymm3, ymm4
    vpunpckhwd ymm8, ymm3, ymm4

    # Build groups c0,c1,c2,c3.
    vpunpckldq ymm9,  ymm5, ymm7
    vpunpckhdq ymm10, ymm5, ymm7

    vpunpckldq ymm11, ymm6, ymm8
    vpunpckhdq ymm12, ymm6, ymm8

    # Lane-local result currently:
    #
    # ymm9  = groups 0,1 | groups 8,9
    # ymm10 = groups 2,3 | groups 10,11
    # ymm11 = groups 4,5 | groups 12,13
    # ymm12 = groups 6,7 | groups 14,15
    #
    # Reorder 128-bit lanes into contiguous memory order:
    #   store0 = groups 0..3
    #   store1 = groups 4..7
    #   store2 = groups 8..11
    #   store3 = groups 12..15
    vperm2i128 ymm5, ymm9,  ymm10, 0x20
    vperm2i128 ymm6, ymm11, ymm12, 0x20
    vperm2i128 ymm7, ymm9,  ymm10, 0x31
    vperm2i128 ymm8, ymm11, ymm12, 0x31

    vmovdqu YMMWORD PTR [rdi + \outoff],      ymm5
    vmovdqu YMMWORD PTR [rdi + \outoff + 32], ymm6
    vmovdqu YMMWORD PTR [rdi + \outoff + 64], ymm7
    vmovdqu YMMWORD PTR [rdi + \outoff + 96], ymm8
.endm

    # b[0..15]  -> coeffs[0..63]
    UNPACK16B_2 0, 0

    # b[16..31] -> coeffs[64..127]
    UNPACK16B_2 16, 128

    vzeroupper
    ret

.size poly_unpack_2_16_avx2, .-poly_unpack_2_16_avx2


.text
.globl poly_unpack_11_16_avx2
.type poly_unpack_11_16_avx2, @function

# void poly_unpack_11_16_avx2(poly *r, const unsigned char *b)
# rdi = r
# rsi = b
#
# RRLWR_N = 128
# coeffs are uint16_t coeffs[128]
#
# packed size = 128 * 11 / 8 = 176 bytes

poly_unpack_11_16_avx2:
    vmovdqu ymm10, YMMWORD PTR [rip + .Lshuf11_lo]
    vmovdqu ymm11, YMMWORD PTR [rip + .Lshift11_lo]
    vmovdqu ymm12, YMMWORD PTR [rip + .Lshuf11_hi]
    vmovdqu ymm13, YMMWORD PTR [rip + .Lshift11_hi]
    vmovdqu ymm14, YMMWORD PTR [rip + .Lmask11]
    vmovdqu ymm15, YMMWORD PTR [rip + .Lsub11]

.macro UNPACK16_11_2LANE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vinserti128 ymm0, ymm0, XMMWORD PTR [rsi + \inoff + 11], 1

    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpxor   ymm2, ymm2, ymm15
    vpxor   ymm3, ymm3, ymm15

    vpackusdw ymm2, ymm2, ymm3
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm2
.endm

.macro UNPACK16_11_2LANE_SAFE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vmovdqu xmm1, XMMWORD PTR [rsi + \inoff + 6]
    vpsrldq xmm1, xmm1, 5
    vinserti128 ymm0, ymm0, xmm1, 1

    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpxor   ymm2, ymm2, ymm15
    vpxor   ymm3, ymm3, ymm15

    vpackusdw ymm2, ymm2, ymm3
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm2
.endm

    UNPACK16_11_2LANE 0,   0
    UNPACK16_11_2LANE 22,  32
    UNPACK16_11_2LANE 44,  64
    UNPACK16_11_2LANE 66,  96
    UNPACK16_11_2LANE 88,  128
    UNPACK16_11_2LANE 110, 160
    UNPACK16_11_2LANE 132, 192
    UNPACK16_11_2LANE_SAFE 154, 224

    vzeroupper
    ret

.size poly_unpack_11_16_avx2, .-poly_unpack_11_16_avx2

.text
.globl poly_unpack_13_16_avx2
.type poly_unpack_13_16_avx2, @function

# void poly_unpack_13_16_avx2(poly *r, const unsigned char *b)
# rdi = r
# rsi = b
#
# packed size = 128 * 13 / 8 = 208 bytes
# coeffs are uint16_t coeffs[128]

poly_unpack_13_16_avx2:
    vmovdqu ymm10, YMMWORD PTR [rip + .Lshuf13_lo]
    vmovdqu ymm11, YMMWORD PTR [rip + .Lshift13_lo]
    vmovdqu ymm12, YMMWORD PTR [rip + .Lshuf13_hi]
    vmovdqu ymm13, YMMWORD PTR [rip + .Lshift13_hi]
    vmovdqu ymm14, YMMWORD PTR [rip + .Lmask13]
    vmovdqu ymm15, YMMWORD PTR [rip + .Lsub13]

.macro UNPACK16_13_2LANE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vinserti128 ymm0, ymm0, XMMWORD PTR [rsi + \inoff + 13], 1

    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpxor   ymm2, ymm2, ymm15
    vpxor   ymm3, ymm3, ymm15

    vpackusdw ymm2, ymm2, ymm3
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm2
.endm

.macro UNPACK16_13_2LANE_SAFE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vmovdqu xmm1, XMMWORD PTR [rsi + \inoff + 10]
    vpsrldq xmm1, xmm1, 3
    vinserti128 ymm0, ymm0, xmm1, 1

    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpxor   ymm2, ymm2, ymm15
    vpxor   ymm3, ymm3, ymm15

    vpackusdw ymm2, ymm2, ymm3
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm2
.endm

    UNPACK16_13_2LANE 0,   0
    UNPACK16_13_2LANE 26,  32
    UNPACK16_13_2LANE 52,  64
    UNPACK16_13_2LANE 78,  96
    UNPACK16_13_2LANE 104, 128
    UNPACK16_13_2LANE 130, 160
    UNPACK16_13_2LANE 156, 192
    UNPACK16_13_2LANE_SAFE 182, 224

    vzeroupper
    ret

.size poly_unpack_13_16_avx2, .-poly_unpack_13_16_avx2

.text
.globl poly_pack_1_32_avx2
.type poly_pack_1_32_avx2, @function

# void poly_pack_1_32_avx2(uint8_t *b, const int32_t r[128])
# b[i] = bitpack((-r[j]) & 1), equivalent to coeff bit 0 for valid inputs.
poly_pack_1_32_avx2:
    mov     r11d, 0x88888888

.macro PACK8_1_32 inoff, outoff
    vmovdqu ymm0, YMMWORD PTR [rsi + \inoff]
    vpslld  ymm0, ymm0, 31
    vpmovmskb eax, ymm0
    pext    eax, eax, r11d
    mov     BYTE PTR [rdi + \outoff], al
.endm

    PACK8_1_32 0,   0
    PACK8_1_32 32,  1
    PACK8_1_32 64,  2
    PACK8_1_32 96,  3
    PACK8_1_32 128, 4
    PACK8_1_32 160, 5
    PACK8_1_32 192, 6
    PACK8_1_32 224, 7
    PACK8_1_32 256, 8
    PACK8_1_32 288, 9
    PACK8_1_32 320, 10
    PACK8_1_32 352, 11
    PACK8_1_32 384, 12
    PACK8_1_32 416, 13
    PACK8_1_32 448, 14
    PACK8_1_32 480, 15

    vzeroupper
    ret
.size poly_pack_1_32_avx2, .-poly_pack_1_32_avx2

.text
.globl poly_unpack_1_32_avx2
.type poly_unpack_1_32_avx2, @function

# void poly_unpack_1_32_avx2(int32_t r[128], const uint8_t *b)
# r[j] = -bit(j)
poly_unpack_1_32_avx2:
    vmovdqu ymm15, YMMWORD PTR [rip + .Ldword_bitmask_1]

.macro UNPACK8_1_32 inoff, outoff
    movzx   eax, BYTE PTR [rsi + \inoff]
    vmovd   xmm0, eax
    vpbroadcastd ymm0, xmm0
    vpand   ymm0, ymm0, ymm15
    vpcmpeqd ymm0, ymm0, ymm15
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm0
.endm

    UNPACK8_1_32 0,  0
    UNPACK8_1_32 1,  32
    UNPACK8_1_32 2,  64
    UNPACK8_1_32 3,  96
    UNPACK8_1_32 4,  128
    UNPACK8_1_32 5,  160
    UNPACK8_1_32 6,  192
    UNPACK8_1_32 7,  224
    UNPACK8_1_32 8,  256
    UNPACK8_1_32 9,  288
    UNPACK8_1_32 10, 320
    UNPACK8_1_32 11, 352
    UNPACK8_1_32 12, 384
    UNPACK8_1_32 13, 416
    UNPACK8_1_32 14, 448
    UNPACK8_1_32 15, 480

    vzeroupper
    ret
.size poly_unpack_1_32_avx2, .-poly_unpack_1_32_avx2

.text
.globl poly_pack_3_32_avx2
.type poly_pack_3_32_avx2, @function

# void poly_pack_3_32_avx2(uint8_t *b, const int32_t r[128])
poly_pack_3_32_avx2:
    vmovdqu ymm7, YMMWORD PTR [rip + .Ldword_3]
    vmovdqu ymm8, YMMWORD PTR [rip + .Ldword_7]
    vmovdqu ymm9, YMMWORD PTR [rip + .Ldword_shift_pack3]

.macro PACK8_3_32 inoff, outoff
    vmovdqu ymm0, YMMWORD PTR [rsi + \inoff]
    vpsubd  ymm0, ymm7, ymm0
    vpand   ymm0, ymm0, ymm8
    vpsllvd ymm0, ymm0, ymm9
    vextracti128 xmm1, ymm0, 1
    vpor    xmm0, xmm0, xmm1
    vpsrldq xmm1, xmm0, 8
    vpor    xmm0, xmm0, xmm1
    vpsrldq xmm1, xmm0, 4
    vpor    xmm0, xmm0, xmm1
    vmovd   eax, xmm0
    mov     WORD PTR [rdi + \outoff], ax
    shr     eax, 16
    mov     BYTE PTR [rdi + \outoff + 2], al
.endm

    PACK8_3_32 0,   0
    PACK8_3_32 32,  3
    PACK8_3_32 64,  6
    PACK8_3_32 96,  9
    PACK8_3_32 128, 12
    PACK8_3_32 160, 15
    PACK8_3_32 192, 18
    PACK8_3_32 224, 21
    PACK8_3_32 256, 24
    PACK8_3_32 288, 27
    PACK8_3_32 320, 30
    PACK8_3_32 352, 33
    PACK8_3_32 384, 36
    PACK8_3_32 416, 39
    PACK8_3_32 448, 42
    PACK8_3_32 480, 45

    vzeroupper
    ret
.size poly_pack_3_32_avx2, .-poly_pack_3_32_avx2

.text
.globl poly_unpack_3_32_avx2
.type poly_unpack_3_32_avx2, @function

# void poly_unpack_3_32_avx2(int32_t r[128], const uint8_t *b)
poly_unpack_3_32_avx2:
    vmovdqu ymm7, YMMWORD PTR [rip + .Ldword_3]
    vmovdqu ymm8, YMMWORD PTR [rip + .Ldword_7]
    vmovdqu ymm9, YMMWORD PTR [rip + .Ldword_shift_pack3]

.macro UNPACK8_3_32 inoff, outoff
    movzx   eax, WORD PTR [rsi + \inoff]
    movzx   ecx, BYTE PTR [rsi + \inoff + 2]
    shl     ecx, 16
    or      eax, ecx
    vmovd   xmm0, eax
    vpbroadcastd ymm0, xmm0
    vpsrlvd ymm0, ymm0, ymm9
    vpand   ymm0, ymm0, ymm8
    vpsubd  ymm0, ymm7, ymm0
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm0
.endm

    UNPACK8_3_32 0,  0
    UNPACK8_3_32 3,  32
    UNPACK8_3_32 6,  64
    UNPACK8_3_32 9,  96
    UNPACK8_3_32 12, 128
    UNPACK8_3_32 15, 160
    UNPACK8_3_32 18, 192
    UNPACK8_3_32 21, 224
    UNPACK8_3_32 24, 256
    UNPACK8_3_32 27, 288
    UNPACK8_3_32 30, 320
    UNPACK8_3_32 33, 352
    UNPACK8_3_32 36, 384
    UNPACK8_3_32 39, 416
    UNPACK8_3_32 42, 448
    UNPACK8_3_32 45, 480

    vzeroupper
    ret
.size poly_unpack_3_32_avx2, .-poly_unpack_3_32_avx2

.text
.globl poly_pack_8_32_avx2
.type poly_pack_8_32_avx2, @function

# void poly_pack_8_32_avx2(uint8_t *b, const int32_t r[128])
poly_pack_8_32_avx2:
    vmovdqu ymm7, YMMWORD PTR [rip + .Ldword_127]
    vmovdqu ymm8, YMMWORD PTR [rip + .Ldword_255]

.macro PACK16_8_32 inoff, outoff
    vmovdqu ymm0, YMMWORD PTR [rsi + \inoff]
    vmovdqu ymm1, YMMWORD PTR [rsi + \inoff + 32]
    vpsubd  ymm0, ymm7, ymm0
    vpsubd  ymm1, ymm7, ymm1
    vpand   ymm0, ymm0, ymm8
    vpand   ymm1, ymm1, ymm8
    vpackusdw ymm0, ymm0, ymm1
    vpermq  ymm0, ymm0, 0xd8
    vextracti128 xmm1, ymm0, 1
    vpackuswb xmm0, xmm0, xmm1
    vmovdqu XMMWORD PTR [rdi + \outoff], xmm0
.endm

    PACK16_8_32 0,   0
    PACK16_8_32 64,  16
    PACK16_8_32 128, 32
    PACK16_8_32 192, 48
    PACK16_8_32 256, 64
    PACK16_8_32 320, 80
    PACK16_8_32 384, 96
    PACK16_8_32 448, 112

    vzeroupper
    ret
.size poly_pack_8_32_avx2, .-poly_pack_8_32_avx2

.text
.globl poly_unpack_8_32_avx2
.type poly_unpack_8_32_avx2, @function

# void poly_unpack_8_32_avx2(int32_t r[128], const uint8_t *b)
poly_unpack_8_32_avx2:
    vmovdqu ymm7, YMMWORD PTR [rip + .Ldword_127]

.macro UNPACK8_8_32 inoff, outoff
    vpmovzxbd ymm0, QWORD PTR [rsi + \inoff]
    vpsubd  ymm0, ymm7, ymm0
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm0
.endm

    UNPACK8_8_32 0,   0
    UNPACK8_8_32 8,   32
    UNPACK8_8_32 16,  64
    UNPACK8_8_32 24,  96
    UNPACK8_8_32 32,  128
    UNPACK8_8_32 40,  160
    UNPACK8_8_32 48,  192
    UNPACK8_8_32 56,  224
    UNPACK8_8_32 64,  256
    UNPACK8_8_32 72,  288
    UNPACK8_8_32 80,  320
    UNPACK8_8_32 88,  352
    UNPACK8_8_32 96,  384
    UNPACK8_8_32 104, 416
    UNPACK8_8_32 112, 448
    UNPACK8_8_32 120, 480

    vzeroupper
    ret
.size poly_unpack_8_32_avx2, .-poly_unpack_8_32_avx2

.text
.globl poly_pack_2_32_avx2
.type poly_pack_2_32_avx2, @function

# void poly_pack_2_32_avx2(uint8_t *b, const int32_t r[128])
poly_pack_2_32_avx2:
    vmovdqu ymm7, YMMWORD PTR [rip + .Ldword_1]
    vmovdqu ymm8, YMMWORD PTR [rip + .Ldword_3]
    vmovdqu ymm9, YMMWORD PTR [rip + .Ldword_shift_pack2]

.macro PACK8_2_32 inoff, outoff
    vmovdqu ymm0, YMMWORD PTR [rsi + \inoff]
    vpsubd  ymm0, ymm7, ymm0
    vpand   ymm0, ymm0, ymm8
    vpsllvd ymm0, ymm0, ymm9
    vextracti128 xmm1, ymm0, 1
    vpor    xmm0, xmm0, xmm1
    vpsrldq xmm1, xmm0, 8
    vpor    xmm0, xmm0, xmm1
    vpsrldq xmm1, xmm0, 4
    vpor    xmm0, xmm0, xmm1
    vmovd   eax, xmm0
    mov     WORD PTR [rdi + \outoff], ax
.endm

    PACK8_2_32 0,   0
    PACK8_2_32 32,  2
    PACK8_2_32 64,  4
    PACK8_2_32 96,  6
    PACK8_2_32 128, 8
    PACK8_2_32 160, 10
    PACK8_2_32 192, 12
    PACK8_2_32 224, 14
    PACK8_2_32 256, 16
    PACK8_2_32 288, 18
    PACK8_2_32 320, 20
    PACK8_2_32 352, 22
    PACK8_2_32 384, 24
    PACK8_2_32 416, 26
    PACK8_2_32 448, 28
    PACK8_2_32 480, 30

    vzeroupper
    ret
.size poly_pack_2_32_avx2, .-poly_pack_2_32_avx2

.text
.globl poly_unpack_2_32_avx2
.type poly_unpack_2_32_avx2, @function

# void poly_unpack_2_32_avx2(int32_t r[128], const uint8_t *b)
poly_unpack_2_32_avx2:
    vmovdqu ymm7, YMMWORD PTR [rip + .Ldword_1]
    vmovdqu ymm8, YMMWORD PTR [rip + .Ldword_3]
    vmovdqu ymm9, YMMWORD PTR [rip + .Ldword_shift_pack2]

.macro UNPACK8_2_32 inoff, outoff
    movzx   eax, WORD PTR [rsi + \inoff]
    vmovd   xmm0, eax
    vpbroadcastd ymm0, xmm0
    vpsrlvd ymm0, ymm0, ymm9
    vpand   ymm0, ymm0, ymm8
    vpsubd  ymm0, ymm7, ymm0
    vmovdqu YMMWORD PTR [rdi + \outoff], ymm0
.endm

    UNPACK8_2_32 0,  0
    UNPACK8_2_32 2,  32
    UNPACK8_2_32 4,  64
    UNPACK8_2_32 6,  96
    UNPACK8_2_32 8,  128
    UNPACK8_2_32 10, 160
    UNPACK8_2_32 12, 192
    UNPACK8_2_32 14, 224
    UNPACK8_2_32 16, 256
    UNPACK8_2_32 18, 288
    UNPACK8_2_32 20, 320
    UNPACK8_2_32 22, 352
    UNPACK8_2_32 24, 384
    UNPACK8_2_32 26, 416
    UNPACK8_2_32 28, 448
    UNPACK8_2_32 30, 480

    vzeroupper
    ret
.size poly_unpack_2_32_avx2, .-poly_unpack_2_32_avx2

.text
.globl poly_pack_11_32_avx2
.type poly_pack_11_32_avx2, @function

# void poly_pack_11_32_avx2(uint8_t *b, const int32_t r[128])
poly_pack_11_32_avx2:
    vpcmpeqw ymm4, ymm4, ymm4
    vpsrlw   ymm1, ymm4, 15
    vpsllw   ymm2, ymm1, 11
    vpblendw ymm5, ymm1, ymm2, 0xaa
    vmovdqu  ymm6, YMMWORD PTR [rip + .Ldword_1023]
    vmovdqu  ymm7, YMMWORD PTR [rip + .Ldword_2047]
    vmovdqu  ymm8, YMMWORD PTR [rip + .Lqmask_low32]
    vmovdqu  ymm9, YMMWORD PTR [rip + .Lqmask_high32]

.macro PACK32_11_Q inoff, outoff
    vmovdqu  ymm0, YMMWORD PTR [rsi + \inoff]
    vmovdqu  ymm1, YMMWORD PTR [rsi + \inoff + 32]
    vpsubd   ymm0, ymm6, ymm0
    vpsubd   ymm1, ymm6, ymm1
    vpand    ymm0, ymm0, ymm7
    vpand    ymm1, ymm1, ymm7
    vpackusdw ymm0, ymm0, ymm1
    vpermq   ymm0, ymm0, 0xd8
    vpmaddwd ymm0, ymm0, ymm5
    vpand    ymm2, ymm0, ymm8
    vpand    ymm3, ymm0, ymm9
    vpsrlq   ymm3, ymm3, 10
    vpor     ymm2, ymm2, ymm3
    vpsrldq  ymm3, ymm2, 8
    vpsllq   ymm3, ymm3, 44
    vpor     ymm3, ymm3, ymm2
    vpsrlq   ymm4, ymm2, 20
    vpblendd ymm3, ymm3, ymm4, 0xcc
    vmovdqu  XMMWORD PTR [rdi + \outoff], xmm3
    vextracti128 XMMWORD PTR [rdi + \outoff + 11], ymm3, 1
.endm

.macro PACK32_11_Q_FINAL inoff, outoff
    vmovdqu  ymm0, YMMWORD PTR [rsi + \inoff]
    vmovdqu  ymm1, YMMWORD PTR [rsi + \inoff + 32]
    vpsubd   ymm0, ymm6, ymm0
    vpsubd   ymm1, ymm6, ymm1
    vpand    ymm0, ymm0, ymm7
    vpand    ymm1, ymm1, ymm7
    vpackusdw ymm0, ymm0, ymm1
    vpermq   ymm0, ymm0, 0xd8
    vpmaddwd ymm0, ymm0, ymm5
    vpand    ymm2, ymm0, ymm8
    vpand    ymm3, ymm0, ymm9
    vpsrlq   ymm3, ymm3, 10
    vpor     ymm2, ymm2, ymm3
    vpsrldq  ymm3, ymm2, 8
    vpsllq   ymm3, ymm3, 44
    vpor     ymm3, ymm3, ymm2
    vpsrlq   ymm4, ymm2, 20
    vpblendd ymm3, ymm3, ymm4, 0xcc
    vmovdqu  XMMWORD PTR [rdi + \outoff], xmm3
    vextracti128 xmm4, ymm3, 1
    STORE11IMG_LAST xmm4, (\outoff + 11)
.endm

    PACK32_11_Q 0,   0
    PACK32_11_Q 64,  22
    PACK32_11_Q 128, 44
    PACK32_11_Q 192, 66
    PACK32_11_Q 256, 88
    PACK32_11_Q 320, 110
    PACK32_11_Q 384, 132
    PACK32_11_Q_FINAL 448, 154

    vzeroupper
    ret
.size poly_pack_11_32_avx2, .-poly_pack_11_32_avx2

.macro STORE_DWORD_UNPACK outoff
    vmovdqu XMMWORD PTR [rdi + \outoff], xmm2
    vmovdqu XMMWORD PTR [rdi + \outoff + 16], xmm3
    vextracti128 xmm4, ymm2, 1
    vmovdqu XMMWORD PTR [rdi + \outoff + 32], xmm4
    vextracti128 xmm4, ymm3, 1
    vmovdqu XMMWORD PTR [rdi + \outoff + 48], xmm4
.endm

.text
.globl poly_unpack_11_32_avx2
.type poly_unpack_11_32_avx2, @function

# void poly_unpack_11_32_avx2(int32_t r[128], const uint8_t *b)
poly_unpack_11_32_avx2:
    vmovdqu ymm10, YMMWORD PTR [rip + .Lshuf11_lo]
    vmovdqu ymm11, YMMWORD PTR [rip + .Lshift11_lo]
    vmovdqu ymm12, YMMWORD PTR [rip + .Lshuf11_hi]
    vmovdqu ymm13, YMMWORD PTR [rip + .Lshift11_hi]
    vmovdqu ymm14, YMMWORD PTR [rip + .Lmask11]
    vmovdqu ymm15, YMMWORD PTR [rip + .Lsub11]

.macro UNPACK16_11_32_2LANE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vinserti128 ymm0, ymm0, XMMWORD PTR [rsi + \inoff + 11], 1
    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpsubd  ymm2, ymm15, ymm2
    vpsubd  ymm3, ymm15, ymm3
    STORE_DWORD_UNPACK \outoff
.endm

.macro UNPACK16_11_32_2LANE_SAFE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vmovdqu xmm1, XMMWORD PTR [rsi + \inoff + 6]
    vpsrldq xmm1, xmm1, 5
    vinserti128 ymm0, ymm0, xmm1, 1
    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpsubd  ymm2, ymm15, ymm2
    vpsubd  ymm3, ymm15, ymm3
    STORE_DWORD_UNPACK \outoff
.endm

    UNPACK16_11_32_2LANE 0,   0
    UNPACK16_11_32_2LANE 22,  64
    UNPACK16_11_32_2LANE 44,  128
    UNPACK16_11_32_2LANE 66,  192
    UNPACK16_11_32_2LANE 88,  256
    UNPACK16_11_32_2LANE 110, 320
    UNPACK16_11_32_2LANE 132, 384
    UNPACK16_11_32_2LANE_SAFE 154, 448

    vzeroupper
    ret
.size poly_unpack_11_32_avx2, .-poly_unpack_11_32_avx2

.text
.globl poly_unpack_13_32_avx2
.type poly_unpack_13_32_avx2, @function

# void poly_unpack_13_32_avx2(int32_t r[128], const uint8_t *b)
poly_unpack_13_32_avx2:
    vmovdqu ymm10, YMMWORD PTR [rip + .Lshuf13_lo]
    vmovdqu ymm11, YMMWORD PTR [rip + .Lshift13_lo]
    vmovdqu ymm12, YMMWORD PTR [rip + .Lshuf13_hi]
    vmovdqu ymm13, YMMWORD PTR [rip + .Lshift13_hi]
    vmovdqu ymm14, YMMWORD PTR [rip + .Lmask13]
    vmovdqu ymm15, YMMWORD PTR [rip + .Lsub13]

.macro UNPACK16_13_32_2LANE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vinserti128 ymm0, ymm0, XMMWORD PTR [rsi + \inoff + 13], 1
    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpsubd  ymm2, ymm15, ymm2
    vpsubd  ymm3, ymm15, ymm3
    STORE_DWORD_UNPACK \outoff
.endm

.macro UNPACK16_13_32_2LANE_SAFE inoff, outoff
    vmovdqu xmm0, XMMWORD PTR [rsi + \inoff]
    vmovdqu xmm1, XMMWORD PTR [rsi + \inoff + 10]
    vpsrldq xmm1, xmm1, 3
    vinserti128 ymm0, ymm0, xmm1, 1
    vpshufb ymm2, ymm0, ymm10
    vpshufb ymm3, ymm0, ymm12
    vpsrlvd ymm2, ymm2, ymm11
    vpsrlvd ymm3, ymm3, ymm13
    vpand   ymm2, ymm2, ymm14
    vpand   ymm3, ymm3, ymm14
    vpsubd  ymm2, ymm15, ymm2
    vpsubd  ymm3, ymm15, ymm3
    STORE_DWORD_UNPACK \outoff
.endm

    UNPACK16_13_32_2LANE 0,   0
    UNPACK16_13_32_2LANE 26,  64
    UNPACK16_13_32_2LANE 52,  128
    UNPACK16_13_32_2LANE 78,  192
    UNPACK16_13_32_2LANE 104, 256
    UNPACK16_13_32_2LANE 130, 320
    UNPACK16_13_32_2LANE 156, 384
    UNPACK16_13_32_2LANE_SAFE 182, 448

    vzeroupper
    ret
.size poly_unpack_13_32_avx2, .-poly_unpack_13_32_avx2

.section .rodata
.p2align 5

# For each 8-coeff block, create 8 dword windows.
#
# coeff starts:
#   i:       0  1  2  3  4  5  6  7
#   byte:    0  1  2  4  5  6  8  9
#
# lower lane handles coeff 0..3
# upper lane handles coeff 4..7
.Lqmask_low32:
    .quad 0x00000000ffffffff,0x00000000ffffffff,0x00000000ffffffff,0x00000000ffffffff
.Lqmask_high32:
    .quad 0xffffffff00000000,0xffffffff00000000,0xffffffff00000000,0xffffffff00000000

.Lshuf11_lo:
    .byte 0,1,2,3,   1,2,3,4,   2,3,4,5,   4,5,6,7
    .byte 0,1,2,3,   1,2,3,4,   2,3,4,5,   4,5,6,7
.Lshuf11_hi:
    .byte 5,6,7,8,   6,7,8,9,   8,9,10,11, 9,10,11,12
    .byte 5,6,7,8,   6,7,8,9,   8,9,10,11, 9,10,11,12
.Lshift11_lo:
    .long 0,3,6,1, 0,3,6,1
.Lshift11_hi:
    .long 4,7,2,5, 4,7,2,5

.Lshuf13_lo:
    .byte 0,1,2,3,     1,2,3,4,      3,4,5,6,      4,5,6,7
    .byte 0,1,2,3,     1,2,3,4,      3,4,5,6,      4,5,6,7
.Lshuf13_hi:
    .byte 6,7,8,9,     8,9,10,11,    9,10,11,12,   11,12,0x80,0x80
    .byte 6,7,8,9,     8,9,10,11,    9,10,11,12,   11,12,0x80,0x80
.Lshift13_lo:
    .long 0,5,2,7, 0,5,2,7
.Lshift13_hi:
    .long 4,1,6,3, 4,1,6,3

.Lshuf11:
    .byte 0,1,2,3,   1,2,3,4,   2,3,4,5,   4,5,6,7
    .byte 5,6,7,8,   6,7,8,9,   8,9,10,11, 9,10,11,12

# bit offsets = (11*i) mod 8
.Lshift11:
    .long 0, 3, 6, 1, 4, 7, 2, 5

.Lmask11:
    .long 0x7ff, 0x7ff, 0x7ff, 0x7ff
    .long 0x7ff, 0x7ff, 0x7ff, 0x7ff

.Lsub11:
    .long 1023, 1023, 1023, 1023
    .long 1023, 1023, 1023, 1023

.p2align 5
.Lshuf13:
    .byte 0,1,2,3,     1,2,3,4,      3,4,5,6,      4,5,6,7
    .byte 6,7,8,9,     8,9,10,11,    9,10,11,12,   11,12,0x80,0x80

.Lshift13:
    .long 0, 5, 2, 7, 4, 1, 6, 3

.Lmask13:
    .long 0x1fff, 0x1fff, 0x1fff, 0x1fff
    .long 0x1fff, 0x1fff, 0x1fff, 0x1fff

.Lsub13:
    .long 4095, 4095, 4095, 4095
    .long 4095, 4095, 4095, 4095

.p2align 5
.Ldword_bitmask_1:
    .long 1,2,4,8,16,32,64,128

.Ldword_1:
    .long 1,1,1,1,1,1,1,1

.Ldword_3:
    .long 3,3,3,3,3,3,3,3

.Ldword_7:
    .long 7,7,7,7,7,7,7,7

.Ldword_127:
    .long 127,127,127,127,127,127,127,127

.Ldword_255:
    .long 255,255,255,255,255,255,255,255

.Ldword_1023:
    .long 1023,1023,1023,1023,1023,1023,1023,1023

.Ldword_2047:
    .long 2047,2047,2047,2047,2047,2047,2047,2047

.Ldword_shift_pack2:
    .long 0,2,4,6,8,10,12,14

.Ldword_shift_pack3:
    .long 0,3,6,9,12,15,18,21

.section .note.GNU-stack,"",@progbits
