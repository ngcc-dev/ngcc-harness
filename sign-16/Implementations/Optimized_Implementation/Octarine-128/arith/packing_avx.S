.intel_syntax noprefix

.section .rodata
.align 16
.Lunpack24_shuf:
    .byte 0, 1, 2, 0x80, 3, 4, 5, 0x80
    .byte 6, 7, 8, 0x80, 9, 10, 11, 0x80

.align 32
.Lunpack2_shuf:
    .byte 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3
    .byte 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4
.align 32
.Lunpack2_shift:
    .long 0, 2, 4, 6, 0, 2, 4, 6

.align 32
.Lunpack3_shuf:
    .byte 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 4
    .byte 1, 2, 3, 4, 1, 2, 3, 4, 2, 3, 4, 5, 2, 3, 4, 5
.align 32
.Lunpack3_shift:
    .long 0, 3, 6, 1, 4, 7, 2, 5

.align 32
.Lunpack5_shuf:
    .byte 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 4, 1, 2, 3, 4
    .byte 2, 3, 4, 5, 3, 4, 5, 6, 3, 4, 5, 6, 4, 5, 6, 7
.align 32
.Lunpack5_shift:
    .long 0, 5, 2, 7, 4, 1, 6, 3

.align 32
.Lunpack6_shuf:
    .byte 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5
    .byte 3, 4, 5, 6, 3, 4, 5, 6, 4, 5, 6, 7, 5, 6, 7, 8
.align 32
.Lunpack6_shift:
    .long 0, 6, 4, 2, 0, 6, 4, 2

.align 32
.Lunpack9_shuf:
    .byte 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
    .byte 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10
.align 32
.Lunpack9_shift:
    .long 0, 1, 2, 3, 4, 5, 6, 7

.align 32
.Lunpack10_shuf:
    .byte 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
    .byte 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10, 8, 9, 10, 11
.align 32
.Lunpack10_shift:
    .long 0, 2, 4, 6, 0, 2, 4, 6

.align 32
.Lunpack11_shuf:
    .byte 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 4, 5, 6, 7
    .byte 5, 6, 7, 8, 6, 7, 8, 9, 8, 9, 10, 11, 9, 10, 11, 12
.align 32
.Lunpack11_shift:
    .long 0, 3, 6, 1, 4, 7, 2, 5

.align 32
.Lunpack13_shuf:
    .byte 0, 1, 2, 3, 1, 2, 3, 4, 3, 4, 5, 6, 4, 5, 6, 7
    .byte 6, 7, 8, 9, 8, 9, 10, 11, 9, 10, 11, 12, 11, 12, 13, 14
.align 32
.Lunpack13_shift:
    .long 0, 5, 2, 7, 4, 1, 6, 3

.align 32
.Lunpack14_shuf:
    .byte 0, 1, 2, 3, 1, 2, 3, 4, 3, 4, 5, 6, 5, 6, 7, 8
    .byte 7, 8, 9, 10, 8, 9, 10, 11, 10, 11, 12, 13, 12, 13, 14, 15
.align 32
.Lunpack14_shift:
    .long 0, 6, 4, 2, 0, 6, 4, 2

.align 32
.Lunpack19_shuf:
    .byte 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 7, 8, 9, 10
    .byte 0, 1, 2, 3, 2, 3, 4, 5, 5, 6, 7, 8, 7, 8, 9, 10
.align 32
.Lunpack19_shift:
    .long 0, 3, 6, 1, 4, 7, 2, 5

.align 32
.Lunpack20_shuf:
    .byte 0, 1, 2, 3, 2, 3, 4, 5, 5, 6, 7, 8, 7, 8, 9, 10
    .byte 0, 1, 2, 3, 2, 3, 4, 5, 5, 6, 7, 8, 7, 8, 9, 10
.align 32
.Lunpack20_shift:
    .long 0, 4, 0, 4, 0, 4, 0, 4

.align 32
.Lunpack22_shuf:
    .byte 0, 1, 2, 3, 2, 3, 4, 5, 5, 6, 7, 8, 8, 9, 10, 11
    .byte 0, 1, 2, 3, 2, 3, 4, 5, 5, 6, 7, 8, 8, 9, 10, 11
.align 32
.Lunpack22_shift:
    .long 0, 6, 4, 2, 0, 6, 4, 2

.align 16
.Lpack8_offset:
    .long 127, 127, 127, 127
.align 32
.Lpack8_mask:
    .long 255, 255, 255, 255, 255, 255, 255, 255

.align 32
.Lpack2_offset:
    .long 1, 1, 1, 1, 1, 1, 1, 1
.align 32
.Lpack2_mask:
    .long 3, 3, 3, 3, 3, 3, 3, 3
.align 32
.Lpack2_maddub:
    .byte 1, 4, 1, 4, 1, 4, 1, 4
    .byte 1, 4, 1, 4, 1, 4, 1, 4
    .byte 1, 4, 1, 4, 1, 4, 1, 4
    .byte 1, 4, 1, 4, 1, 4, 1, 4
.align 32
.Lpack2_maddwd:
    .word 1, 16, 1, 16, 1, 16, 1, 16
    .word 1, 16, 1, 16, 1, 16, 1, 16

.align 32
.Lpack3_offset:
    .long 3, 3, 3, 3, 3, 3, 3, 3
.align 32
.Lpack3_mask:
    .long 7, 7, 7, 7, 7, 7, 7, 7
.align 16
.Lpack3_pairmul:
    .word 1, 8, 1, 8, 1, 8, 1, 8
.align 16
.Lpack3_maddub:
    .byte 1, 8, 1, 8, 1, 8, 1, 8
    .byte 1, 8, 1, 8, 1, 8, 1, 8
.align 16
.Lpack3_quadmul:
    .word 1, 64, 1, 64, 1, 64, 1, 64
.align 16
.Lpack3_octmul:
    .word 1, 4096, 1, 4096, 1, 4096, 1, 4096
.align 32
.Lpack3_lo:
    .quad 0x000000000000003f, 0x000000000000003f
    .quad 0x000000000000003f, 0x000000000000003f
.align 32
.Lpack3_hi:
    .quad 0x0000003f00000000, 0x0000003f00000000
    .quad 0x0000003f00000000, 0x0000003f00000000
.align 16
.Lpack3_shuf:
    .byte 0, 1, 2, 0x80, 0x80, 0x80, 0x80, 0x80
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
.align 16
.Lpack3_store_shuf:
    .byte 0, 1, 2, 4, 5, 6, 0x80, 0x80
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80

.align 32
.Lpack5_offset:
    .long 15, 15, 15, 15, 15, 15, 15, 15
.align 32
.Lpack5_mask:
    .long 31, 31, 31, 31, 31, 31, 31, 31
.align 16
.Lpack5_pairmul:
    .word 1, 32, 1, 32, 1, 32, 1, 32
.align 32
.Lpack5_lo:
    .quad 0x00000000000003ff, 0x00000000000003ff
    .quad 0x00000000000003ff, 0x00000000000003ff
.align 32
.Lpack5_hi:
    .quad 0x000003ff00000000, 0x000003ff00000000
    .quad 0x000003ff00000000, 0x000003ff00000000
.align 32
.Lpack6_offset:
    .long 31, 31, 31, 31, 31, 31, 31, 31
.align 32
.Lpack6_mask:
    .long 63, 63, 63, 63, 63, 63, 63, 63
.align 16
.Lpack6_pairmul:
    .word 1, 64, 1, 64, 1, 64, 1, 64
.align 32
.Lpack6_lo:
    .quad 0x0000000000000fff, 0x0000000000000fff
    .quad 0x0000000000000fff, 0x0000000000000fff
.align 32
.Lpack6_hi:
    .quad 0x00000fff00000000, 0x00000fff00000000
    .quad 0x00000fff00000000, 0x00000fff00000000
.align 16
.Lpack6_shuf:
    .byte 0, 1, 2, 8, 9, 10, 0x80, 0x80
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80

.align 32
.Lpack9_offset:
    .long 255, 255, 255, 255, 255, 255, 255, 255
.align 32
.Lpack9_mask:
    .long 511, 511, 511, 511, 511, 511, 511, 511
.align 16
.Lpack9_pairmul:
    .word 1, 512, 1, 512, 1, 512, 1, 512
.align 32
.Lpack9_lo:
    .quad 0x000000000003ffff, 0x000000000003ffff
    .quad 0x000000000003ffff, 0x000000000003ffff
.align 32
.Lpack9_hi:
    .quad 0x0003ffff00000000, 0x0003ffff00000000
    .quad 0x0003ffff00000000, 0x0003ffff00000000
.align 32
.Lpack10_offset:
    .long 511, 511, 511, 511, 511, 511, 511, 511
.align 32
.Lpack10_mask:
    .long 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
.align 16
.Lpack10_pairmul:
    .word 1, 1024, 1, 1024, 1, 1024, 1, 1024
.align 32
.Lpack10_lo:
    .quad 0x00000000000fffff, 0x00000000000fffff
    .quad 0x00000000000fffff, 0x00000000000fffff
.align 32
.Lpack10_hi:
    .quad 0x000fffff00000000, 0x000fffff00000000
    .quad 0x000fffff00000000, 0x000fffff00000000
.align 16
.Lpack10_shuf:
    .byte 0, 1, 2, 3, 4, 8, 9, 10
    .byte 11, 12, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80

.align 32
.Lpack11_offset:
    .long 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
.align 32
.Lpack11_mask:
    .long 2047, 2047, 2047, 2047, 2047, 2047, 2047, 2047
.align 16
.Lpack11_pairmul:
    .word 1, 2048, 1, 2048, 1, 2048, 1, 2048
.align 32
.Lpack11_lo:
    .quad 0x00000000003fffff, 0x00000000003fffff
    .quad 0x00000000003fffff, 0x00000000003fffff
.align 32
.Lpack11_hi:
    .quad 0x003fffff00000000, 0x003fffff00000000
    .quad 0x003fffff00000000, 0x003fffff00000000
.align 32
.Lpack13_offset:
    .long 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095
.align 32
.Lpack13_mask:
    .long 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191
.align 16
.Lpack13_pairmul:
    .word 1, 8192, 1, 8192, 1, 8192, 1, 8192
.align 32
.Lpack13_lo:
    .quad 0x0000000003ffffff, 0x0000000003ffffff
    .quad 0x0000000003ffffff, 0x0000000003ffffff
.align 32
.Lpack13_hi:
    .quad 0x03ffffff00000000, 0x03ffffff00000000
    .quad 0x03ffffff00000000, 0x03ffffff00000000
.align 32
.Lpack19_offset:
    .long 262143, 262143, 262143, 262143
    .long 262143, 262143, 262143, 262143
.align 32
.Lpack19_mask:
    .long 524287, 524287, 524287, 524287
    .long 524287, 524287, 524287, 524287
.align 32
.Lpack19_pair_lo:
    .quad 0x000000000007ffff, 0x000000000007ffff
    .quad 0x000000000007ffff, 0x000000000007ffff
.align 32
.Lpack19_pair_hi:
    .quad 0x0007ffff00000000, 0x0007ffff00000000
    .quad 0x0007ffff00000000, 0x0007ffff00000000
.align 32
.Lpack19_q_base_shr:
    .quad 0, 26, 14, 64
.align 32
.Lpack19_q_add1_shl:
    .quad 38, 12, 64, 64
.align 32
.Lpack19_q_add2_shl:
    .quad 64, 50, 64, 64

.align 32
.Lpack20_offset:
    .long 524287, 524287, 524287, 524287
    .long 524287, 524287, 524287, 524287
.align 32
.Lpack20_mask:
    .long 1048575, 1048575, 1048575, 1048575
    .long 1048575, 1048575, 1048575, 1048575
.align 32
.Lpack20_lo:
    .quad 0x00000000000fffff, 0x00000000000fffff
    .quad 0x00000000000fffff, 0x00000000000fffff
.align 32
.Lpack20_hi:
    .quad 0x000fffff00000000, 0x000fffff00000000
    .quad 0x000fffff00000000, 0x000fffff00000000

.align 32
.Lpack22_offset:
    .long 2097151, 2097151, 2097151, 2097151
    .long 2097151, 2097151, 2097151, 2097151
.align 32
.Lpack22_mask:
    .long 4194303, 4194303, 4194303, 4194303
    .long 4194303, 4194303, 4194303, 4194303
.align 32
.Lpack22_lo:
    .quad 0x00000000003fffff, 0x00000000003fffff
    .quad 0x00000000003fffff, 0x00000000003fffff
.align 32
.Lpack22_hi:
    .quad 0x003fffff00000000, 0x003fffff00000000
    .quad 0x003fffff00000000, 0x003fffff00000000

.align 16
.Lunpack24_offset:
    .long 0x7fffff, 0x7fffff, 0x7fffff, 0x7fffff

.align 32
.Lpack24_offset:
    .long 0x7fffff, 0x7fffff, 0x7fffff, 0x7fffff
    .long 0x7fffff, 0x7fffff, 0x7fffff, 0x7fffff
.align 32
.Lpack24_shuf:
    .byte 0, 1, 2, 4, 5, 6, 8, 9
    .byte 10, 12, 13, 14, 0x80, 0x80, 0x80, 0x80
    .byte 0, 1, 2, 4, 5, 6, 8, 9
    .byte 10, 12, 13, 14, 0x80, 0x80, 0x80, 0x80

.align 32
.Lpack14_offset:
    .long 8191, 8191, 8191, 8191
    .long 8191, 8191, 8191, 8191
.align 32
.Lpack14_mask:
    .long 16383, 16383, 16383, 16383
    .long 16383, 16383, 16383, 16383

.align 16
.Lpack14_pairmul:
    .word 1, 16384, 1, 16384, 1, 16384, 1, 16384

.align 16
.Lpack14_lo28:
    .quad 0x000000000fffffff, 0x000000000fffffff

.align 16
.Lpack14_hi28:
    .quad 0x0fffffff00000000, 0x0fffffff00000000

.align 16
.Lpack14_shuf:
    .byte 0, 1, 2, 3, 4, 5, 6, 8
    .byte 9, 10, 11, 12, 13, 14, 0x80, 0x80

.text

.macro STORE3 x, off
    vmovd eax, \x
    mov WORD PTR [rdi + r9 + \off], ax
    shr eax, 16
    mov BYTE PTR [rdi + r9 + \off + 2], al
.endm

.macro STORE5 x, off
    vmovd eax, \x
    mov DWORD PTR [rdi + r9 + \off], eax
    vpextrb eax, \x, 4
    mov BYTE PTR [rdi + r9 + \off + 4], al
.endm

.macro STORE6 x, off
    vmovd eax, \x
    mov DWORD PTR [rdi + r9 + \off], eax
    vpextrw eax, \x, 2
    mov WORD PTR [rdi + r9 + \off + 4], ax
.endm

.macro STORE9 x, off
    vmovq QWORD PTR [rdi + r9 + \off], \x
    vpextrb eax, \x, 8
    mov BYTE PTR [rdi + r9 + \off + 8], al
.endm

.macro STORE10 x, off
    vmovq QWORD PTR [rdi + r9 + \off], \x
    vpextrw eax, \x, 4
    mov WORD PTR [rdi + r9 + \off + 8], ax
.endm

.macro STORE11 x, off
    vmovq QWORD PTR [rdi + r9 + \off], \x
    vpextrw eax, \x, 4
    mov WORD PTR [rdi + r9 + \off + 8], ax
    vpextrb eax, \x, 10
    mov BYTE PTR [rdi + r9 + \off + 10], al
.endm

.macro STORE13 x, off
    vmovq QWORD PTR [rdi + r9 + \off], \x
    vpextrd eax, \x, 2
    mov DWORD PTR [rdi + r9 + \off + 8], eax
    vpextrb eax, \x, 12
    mov BYTE PTR [rdi + r9 + \off + 12], al
.endm

.macro STORE12 x, off
    vmovq QWORD PTR [rdi + r9 + \off], \x
    vpextrd eax, \x, 2
    mov DWORD PTR [rdi + r9 + \off + 8], eax
.endm

.macro PACK16_Q4 dst, tmp, inoff, offreg, pairreg, loreg, hireg, shift
    vmovdqu \dst, YMMWORD PTR [rsi + r8 + \inoff]
    vmovdqu \tmp, YMMWORD PTR [rsi + r8 + \inoff + 32]
    vpsubd \dst, \offreg, \dst
    vpsubd \tmp, \offreg, \tmp
    vpackusdw \dst, \dst, \tmp
    vpermq \dst, \dst, 0xd8
    vpmaddwd \dst, \dst, \pairreg
    vpand \tmp, \dst, \hireg
    vpand \dst, \dst, \loreg
    vpsrlq \tmp, \tmp, \shift
    vpor \dst, \dst, \tmp
.endm

.macro PACK16_Q4_MASK dst, tmp, inoff, offreg, maskreg, pairreg, loreg, hireg, shift
    vmovdqu \dst, YMMWORD PTR [rsi + r8 + \inoff]
    vmovdqu \tmp, YMMWORD PTR [rsi + r8 + \inoff + 32]
    vpsubd \dst, \offreg, \dst
    vpsubd \tmp, \offreg, \tmp
    vpand \dst, \dst, \maskreg
    vpand \tmp, \tmp, \maskreg
    vpackusdw \dst, \dst, \tmp
    vpermq \dst, \dst, 0xd8
    vpmaddwd \dst, \dst, \pairreg
    vpand \tmp, \dst, \hireg
    vpand \dst, \dst, \loreg
    vpsrlq \tmp, \tmp, \shift
    vpor \dst, \dst, \tmp
.endm

.macro PACK_ODD_BLEND qv, tmp, hi, lshift, rshift
    vpsrldq \tmp, \qv, 8
    vpsllq \tmp, \tmp, \lshift
    vpor \tmp, \tmp, \qv
    vpsrlq \hi, \qv, \rshift
    vpblendd \qv, \tmp, \hi, 0xcc
.endm

.macro LOAD_EXACT dst, bytes, base=0
.if \bytes == 2
    movzx eax, WORD PTR [rsi + r8 + \base]
    vmovd \dst, eax
.elseif \bytes == 3
    movzx eax, WORD PTR [rsi + r8 + \base]
    movzx edx, BYTE PTR [rsi + r8 + \base + 2]
    shl edx, 16
    or eax, edx
    vmovd \dst, eax
.elseif \bytes == 5
    vmovd \dst, DWORD PTR [rsi + r8 + \base]
    vpinsrb \dst, \dst, BYTE PTR [rsi + r8 + \base + 4], 4
.elseif \bytes == 6
    vmovd \dst, DWORD PTR [rsi + r8 + \base]
    vpinsrw \dst, \dst, WORD PTR [rsi + r8 + \base + 4], 2
.elseif \bytes == 9
    vmovq \dst, QWORD PTR [rsi + r8 + \base]
    vpinsrb \dst, \dst, BYTE PTR [rsi + r8 + \base + 8], 8
.elseif \bytes == 10
    vmovq \dst, QWORD PTR [rsi + r8 + \base]
    vpinsrw \dst, \dst, WORD PTR [rsi + r8 + \base + 8], 4
.elseif \bytes == 11
    vmovq \dst, QWORD PTR [rsi + r8 + \base]
    vpinsrw \dst, \dst, WORD PTR [rsi + r8 + \base + 8], 4
    vpinsrb \dst, \dst, BYTE PTR [rsi + r8 + \base + 10], 10
.elseif \bytes == 13
    vmovq \dst, QWORD PTR [rsi + r8 + \base]
    vpinsrd \dst, \dst, DWORD PTR [rsi + r8 + \base + 8], 2
    vpinsrb \dst, \dst, BYTE PTR [rsi + r8 + \base + 12], 12
.elseif \bytes == 14
    vmovq \dst, QWORD PTR [rsi + r8 + \base]
    vpinsrd \dst, \dst, DWORD PTR [rsi + r8 + \base + 8], 2
    vpinsrw \dst, \dst, WORD PTR [rsi + r8 + \base + 12], 6
.endif
.endm

.macro UNPACK_YMM
    vpshufb ymm0, ymm0, ymm15
    vpsrlvd ymm0, ymm0, ymm14
    vpand ymm0, ymm0, ymm13
    vpsubd ymm0, ymm12, ymm0
.endm

.macro UNPACK8_FUNC bits, safe
.globl poly_unpack\bits\()_avx
.type poly_unpack\bits\()_avx, @function
poly_unpack\bits\()_avx:
    vmovdqa ymm15, YMMWORD PTR [rip + .Lunpack\bits\()_shuf]
    vmovdqa ymm14, YMMWORD PTR [rip + .Lunpack\bits\()_shift]
    vmovdqa ymm13, YMMWORD PTR [rip + .Lpack\bits\()_mask]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack\bits\()_offset]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lunpack\bits\()_loop:
    cmp r8, \safe
    jae .Lunpack\bits\()_tail
    vmovdqu xmm0, XMMWORD PTR [rsi + r8]
    vinserti128 ymm0, ymm0, xmm0, 1
    UNPACK_YMM
    vmovdqu YMMWORD PTR [rdi + r9], ymm0
    add r8, \bits
    add r9, 32
    cmp r9, 4096
    jb .Lunpack\bits\()_loop
    vzeroupper
    ret
    .p2align 4
.Lunpack\bits\()_tail:
    LOAD_EXACT xmm0, \bits
    vinserti128 ymm0, ymm0, xmm0, 1
    UNPACK_YMM
    vmovdqu YMMWORD PTR [rdi + r9], ymm0
    add r8, \bits
    add r9, 32
    cmp r9, 4096
    jb .Lunpack\bits\()_tail
    vzeroupper
    ret
.size poly_unpack\bits\()_avx, .-poly_unpack\bits\()_avx
.endm

.macro UNPACK8_LARGE_FUNC bits, highbase, tailbytes, safe
.globl poly_unpack\bits\()_avx
.type poly_unpack\bits\()_avx, @function
poly_unpack\bits\()_avx:
    vmovdqa ymm15, YMMWORD PTR [rip + .Lunpack\bits\()_shuf]
    vmovdqa ymm14, YMMWORD PTR [rip + .Lunpack\bits\()_shift]
    vmovdqa ymm13, YMMWORD PTR [rip + .Lpack\bits\()_mask]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack\bits\()_offset]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lunpack\bits\()_loop:
    cmp r8, \safe
    jae .Lunpack\bits\()_tail
    vmovdqu xmm0, XMMWORD PTR [rsi + r8]
    vinserti128 ymm0, ymm0, XMMWORD PTR [rsi + r8 + \highbase], 1
    UNPACK_YMM
    vmovdqu YMMWORD PTR [rdi + r9], ymm0
    add r8, \bits
    add r9, 32
    cmp r9, 4096
    jb .Lunpack\bits\()_loop
    vzeroupper
    ret
    .p2align 4
.Lunpack\bits\()_tail:
    vmovdqu xmm0, XMMWORD PTR [rsi + r8]
    LOAD_EXACT xmm1, \tailbytes, \highbase
    vinserti128 ymm0, ymm0, xmm1, 1
    UNPACK_YMM
    vmovdqu YMMWORD PTR [rdi + r9], ymm0
    add r8, \bits
    add r9, 32
    cmp r9, 4096
    jb .Lunpack\bits\()_tail
    vzeroupper
    ret
.size poly_unpack\bits\()_avx, .-poly_unpack\bits\()_avx
.endm

.macro PACK19_QWORD_BLOCK inoff, outoff, wide_tail=1
    vmovdqu ymm0, YMMWORD PTR [rsi + r8 + \inoff]
    vpsubd ymm0, ymm14, ymm0

    vpand ymm1, ymm0, ymm13
    vpand ymm0, ymm0, ymm12
    vpsrlq ymm0, ymm0, 13
    vpor ymm0, ymm0, ymm1

    vpermq ymm1, ymm0, 0x34
    vpsrlvq ymm1, ymm1, ymm11
    vpermq ymm2, ymm0, 0x09
    vpsllvq ymm2, ymm2, ymm10
    vpor ymm1, ymm1, ymm2
    vpermq ymm2, ymm0, 0x0c
    vpsllvq ymm2, ymm2, ymm9
    vpor ymm1, ymm1, ymm2

.if \wide_tail
    vmovdqu YMMWORD PTR [rdi + r9 + \outoff], ymm1
.else
    vmovdqu XMMWORD PTR [rdi + r9 + \outoff], xmm1
    vextracti128 xmm2, ymm1, 1
    vmovd eax, xmm2
    mov WORD PTR [rdi + r9 + \outoff + 16], ax
    shr eax, 16
    mov BYTE PTR [rdi + r9 + \outoff + 18], al
.endif
.endm

.macro PACK19_QWORD_TWO_BLOCKS
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm4, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm4, ymm14, ymm4

    vpand ymm1, ymm0, ymm13
    vpand ymm5, ymm4, ymm13
    vpand ymm0, ymm0, ymm12
    vpand ymm4, ymm4, ymm12
    vpsrlq ymm0, ymm0, 13
    vpsrlq ymm4, ymm4, 13
    vpor ymm0, ymm0, ymm1
    vpor ymm4, ymm4, ymm5

    vpermq ymm1, ymm0, 0x34
    vpermq ymm5, ymm4, 0x34
    vpsrlvq ymm1, ymm1, ymm11
    vpsrlvq ymm5, ymm5, ymm11

    vpermq ymm2, ymm0, 0x09
    vpermq ymm6, ymm4, 0x09
    vpsllvq ymm2, ymm2, ymm10
    vpsllvq ymm6, ymm6, ymm10
    vpor ymm1, ymm1, ymm2
    vpor ymm5, ymm5, ymm6

    vpermq ymm2, ymm0, 0x0c
    vpermq ymm6, ymm4, 0x0c
    vpsllvq ymm2, ymm2, ymm9
    vpsllvq ymm6, ymm6, ymm9
    vpor ymm1, ymm1, ymm2
    vpor ymm5, ymm5, ymm6

    vmovdqu YMMWORD PTR [rdi + r9], ymm1
    vmovdqu YMMWORD PTR [rdi + r9 + 19], ymm5
.endm

.macro PACK22_TWO_BLOCKS
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm4, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm4, ymm14, ymm4
    vpand ymm1, ymm0, ymm11
    vpand ymm5, ymm4, ymm11
    vpand ymm0, ymm0, ymm12
    vpand ymm4, ymm4, ymm12
    vpsrlq ymm1, ymm1, 10
    vpsrlq ymm5, ymm5, 10
    vpor ymm0, ymm0, ymm1
    vpor ymm4, ymm4, ymm5

    vpsrldq ymm2, ymm0, 8
    vpsrldq ymm6, ymm4, 8
    vpsllq ymm2, ymm2, 44
    vpsllq ymm6, ymm6, 44
    vpor ymm2, ymm2, ymm0
    vpor ymm6, ymm6, ymm4
    vpsrlq ymm1, ymm0, 20
    vpsrlq ymm5, ymm4, 20
    vpblendd ymm2, ymm2, ymm1, 0xcc
    vpblendd ymm6, ymm6, ymm5, 0xcc

    vmovdqu XMMWORD PTR [rdi + r9], xmm2
    vextracti128 XMMWORD PTR [rdi + r9 + 11], ymm2, 1
    vmovdqu XMMWORD PTR [rdi + r9 + 22], xmm6
    vextracti128 XMMWORD PTR [rdi + r9 + 33], ymm6, 1
.endm

UNPACK8_FUNC 2, 241
UNPACK8_FUNC 3, 369
UNPACK8_FUNC 5, 625
UNPACK8_FUNC 6, 753
UNPACK8_FUNC 9, 1137
UNPACK8_FUNC 10, 1265
UNPACK8_FUNC 11, 1393
UNPACK8_FUNC 13, 1649
UNPACK8_FUNC 14, 1777
UNPACK8_LARGE_FUNC 19, 9, 10, 2408
UNPACK8_LARGE_FUNC 20, 10, 10, 2535
UNPACK8_LARGE_FUNC 22, 11, 11, 2790

.globl poly_pack2_avx
.type poly_pack2_avx, @function
poly_pack2_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack2_offset]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack2_mask]
    vmovdqa ymm13, YMMWORD PTR [rip + .Lpack2_maddub]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack2_maddwd]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack2_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpand ymm0, ymm0, ymm11
    vpand ymm1, ymm1, ymm11
    vpackusdw ymm0, ymm0, ymm1
    vpermq ymm0, ymm0, 0xd8
    vextracti128 xmm1, ymm0, 1
    vpackuswb xmm0, xmm0, xmm1
    vpmaddubsw xmm0, xmm0, xmm13
    vpmaddwd xmm0, xmm0, xmm12
    vpackusdw xmm0, xmm0, xmm0
    vpackuswb xmm0, xmm0, xmm0
    vmovd DWORD PTR [rdi + r9], xmm0
    add r8, 64
    add r9, 4
    cmp r8, 4096
    jb .Lpack2_loop
    vzeroupper
    ret
.size poly_pack2_avx, .-poly_pack2_avx

.globl poly_pack3_avx
.type poly_pack3_avx, @function
poly_pack3_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack3_offset]
    vmovdqa ymm9, YMMWORD PTR [rip + .Lpack3_mask]
    vmovdqa xmm13, XMMWORD PTR [rip + .Lpack3_maddub]
    vmovdqa xmm12, XMMWORD PTR [rip + .Lpack3_quadmul]
    vmovdqa xmm11, XMMWORD PTR [rip + .Lpack3_octmul]
    vmovdqa xmm10, XMMWORD PTR [rip + .Lpack3_store_shuf]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack3_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpand ymm0, ymm0, ymm9
    vpand ymm1, ymm1, ymm9
    vpackusdw ymm0, ymm0, ymm1
    vpermq ymm0, ymm0, 0xd8
    vextracti128 xmm1, ymm0, 1
    vpackuswb xmm0, xmm0, xmm1
    vpmaddubsw xmm0, xmm0, xmm13
    vpmaddwd xmm0, xmm0, xmm12
    vpackusdw xmm0, xmm0, xmm0
    vpmaddwd xmm0, xmm0, xmm11
    vpshufb xmm1, xmm0, xmm10
    vmovq QWORD PTR [rdi + r9], xmm1
    add r8, 64
    add r9, 6
    cmp r8, 4032
    jb .Lpack3_loop
.Lpack3_last:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpand ymm0, ymm0, ymm9
    vpand ymm1, ymm1, ymm9
    vpackusdw ymm0, ymm0, ymm1
    vpermq ymm0, ymm0, 0xd8
    vextracti128 xmm1, ymm0, 1
    vpackuswb xmm0, xmm0, xmm1
    vpmaddubsw xmm0, xmm0, xmm13
    vpmaddwd xmm0, xmm0, xmm12
    vpackusdw xmm0, xmm0, xmm0
    vpmaddwd xmm0, xmm0, xmm11
    STORE3 xmm0, 0
    vpextrd eax, xmm0, 1
    mov WORD PTR [rdi + r9 + 3], ax
    shr eax, 16
    mov BYTE PTR [rdi + r9 + 5], al
    vzeroupper
    ret
.size poly_pack3_avx, .-poly_pack3_avx

.globl poly_pack5_avx
.type poly_pack5_avx, @function
poly_pack5_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack5_offset]
    vbroadcasti128 ymm13, XMMWORD PTR [rip + .Lpack5_pairmul]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack5_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack5_hi]
    vmovdqa ymm10, YMMWORD PTR [rip + .Lpack5_mask]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack5_loop:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 22
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 20, 44
    vmovq QWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 5], ymm0, 1
    add r8, 64
    add r9, 10
    cmp r8, 4032
    jb .Lpack5_loop
.Lpack5_last:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 22
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 20, 44
    STORE5 xmm0, 0
    vextracti128 xmm1, ymm0, 1
    STORE5 xmm1, 5
    vzeroupper
    ret
.size poly_pack5_avx, .-poly_pack5_avx

.globl poly_pack6_avx
.type poly_pack6_avx, @function
poly_pack6_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack6_offset]
    vbroadcasti128 ymm13, XMMWORD PTR [rip + .Lpack6_pairmul]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack6_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack6_hi]
    vbroadcasti128 ymm10, XMMWORD PTR [rip + .Lpack6_shuf]
    vmovdqa ymm9, YMMWORD PTR [rip + .Lpack6_mask]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack6_loop:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm9, ymm13, ymm12, ymm11, 20
    PACK16_Q4_MASK ymm2, ymm3, 64, ymm14, ymm9, ymm13, ymm12, ymm11, 20
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10
    vmovq QWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 6], ymm0, 1
    vmovq QWORD PTR [rdi + r9 + 12], xmm2
    vextracti128 XMMWORD PTR [rdi + r9 + 18], ymm2, 1
    add r8, 128
    add r9, 24
    cmp r8, 3968
    jb .Lpack6_loop
.Lpack6_last:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm9, ymm13, ymm12, ymm11, 20
    PACK16_Q4_MASK ymm2, ymm3, 64, ymm14, ymm9, ymm13, ymm12, ymm11, 20
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10
    STORE6 xmm0, 0
    vextracti128 xmm1, ymm0, 1
    STORE6 xmm1, 6
    STORE6 xmm2, 12
    vextracti128 xmm3, ymm2, 1
    STORE6 xmm3, 18
    vzeroupper
    ret
.size poly_pack6_avx, .-poly_pack6_avx

.globl poly_unpack24_avx
.type poly_unpack24_avx, @function
poly_unpack24_avx:
    vmovdqa xmm14, XMMWORD PTR [rip + .Lunpack24_shuf]
    vmovdqa xmm15, XMMWORD PTR [rip + .Lunpack24_offset]
    xor eax, eax
    xor edx, edx

    .p2align 4
.Lunpack24_loop:
    vmovq xmm0, QWORD PTR [rsi + rax]
    vpinsrd xmm0, xmm0, DWORD PTR [rsi + rax + 8], 2
    vmovq xmm1, QWORD PTR [rsi + rax + 12]
    vpinsrd xmm1, xmm1, DWORD PTR [rsi + rax + 20], 2
    vpshufb xmm0, xmm0, xmm14
    vpshufb xmm1, xmm1, xmm14
    vpsubd xmm0, xmm15, xmm0
    vpsubd xmm1, xmm15, xmm1
    vmovdqu XMMWORD PTR [rdi + rdx], xmm0
    vmovdqu XMMWORD PTR [rdi + rdx + 16], xmm1
    add rax, 24
    add rdx, 32
    cmp rdx, 4096
    jb .Lunpack24_loop

    vzeroupper
    ret
.size poly_unpack24_avx, .-poly_unpack24_avx

.globl poly_pack24_avx
.type poly_pack24_avx, @function
poly_pack24_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack24_offset]
    vmovdqa ymm13, YMMWORD PTR [rip + .Lpack24_shuf]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack24_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpshufb ymm0, ymm0, ymm13
    vpshufb ymm1, ymm1, ymm13
    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 12], ymm0, 1
    vmovdqu XMMWORD PTR [rdi + r9 + 24], xmm1
    vextracti128 XMMWORD PTR [rdi + r9 + 36], ymm1, 1
    add r8, 64
    add r9, 48
    cmp r8, 4032
    jb .Lpack24_loop

    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpshufb ymm0, ymm0, ymm13
    vpshufb ymm1, ymm1, ymm13
    STORE12 xmm0, 0
    vextracti128 xmm2, ymm0, 1
    STORE12 xmm2, 12
    STORE12 xmm1, 24
    vextracti128 xmm2, ymm1, 1
    STORE12 xmm2, 36
    vzeroupper
    ret
.size poly_pack24_avx, .-poly_pack24_avx

.globl poly_pack8_avx
.type poly_pack8_avx, @function
poly_pack8_avx:
    vbroadcasti128 ymm14, XMMWORD PTR [rip + .Lpack8_offset]
    vmovdqa ymm13, YMMWORD PTR [rip + .Lpack8_mask]
    xor r8d, r8d
    xor r9d, r9d

    .p2align 4
.Lpack8_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpand ymm0, ymm0, ymm13
    vpand ymm1, ymm1, ymm13
    vpackusdw ymm0, ymm0, ymm1
    vpermq ymm0, ymm0, 0xd8
    vextracti128 xmm1, ymm0, 1
    vpackuswb xmm0, xmm0, xmm1
    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    add r8, 64
    add r9, 16
    cmp r8, 4096
    jb .Lpack8_loop

    vzeroupper
    ret
.size poly_pack8_avx, .-poly_pack8_avx

.globl poly_unpack8_avx
.type poly_unpack8_avx, @function
poly_unpack8_avx:
    vmovdqa xmm15, XMMWORD PTR [rip + .Lpack8_offset]
    xor eax, eax
    xor edx, edx

    .p2align 4
.Lunpack8_loop:
    vmovd xmm0, DWORD PTR [rsi + rax]
    vpmovzxbd xmm0, xmm0
    vpsubd xmm0, xmm15, xmm0
    vmovdqu XMMWORD PTR [rdi + rdx], xmm0
    add rax, 4
    add rdx, 16
    cmp rdx, 4096
    jb .Lunpack8_loop

    vzeroupper
    ret
.size poly_unpack8_avx, .-poly_unpack8_avx

.globl poly_pack9_avx
.type poly_pack9_avx, @function
poly_pack9_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack9_offset]
    vbroadcasti128 ymm13, XMMWORD PTR [rip + .Lpack9_pairmul]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack9_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack9_hi]
    vmovdqa ymm10, YMMWORD PTR [rip + .Lpack9_mask]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack9_loop:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 14
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 36, 28
    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 9], ymm0, 1
    add r8, 64
    add r9, 18
    cmp r8, 4032
    jb .Lpack9_loop
.Lpack9_last:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 14
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 36, 28
    STORE9 xmm0, 0
    vextracti128 xmm1, ymm0, 1
    STORE9 xmm1, 9
    vzeroupper
    ret
.size poly_pack9_avx, .-poly_pack9_avx

.globl poly_pack10_avx
.type poly_pack10_avx, @function
poly_pack10_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack10_offset]
    vbroadcasti128 ymm13, XMMWORD PTR [rip + .Lpack10_pairmul]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack10_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack10_hi]
    vbroadcasti128 ymm10, XMMWORD PTR [rip + .Lpack10_shuf]
    vmovdqa ymm9, YMMWORD PTR [rip + .Lpack10_mask]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack10_loop:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm9, ymm13, ymm12, ymm11, 12
    PACK16_Q4_MASK ymm2, ymm3, 64, ymm14, ymm9, ymm13, ymm12, ymm11, 12
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10
    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 10], ymm0, 1
    vmovdqu XMMWORD PTR [rdi + r9 + 20], xmm2
    vextracti128 XMMWORD PTR [rdi + r9 + 30], ymm2, 1
    add r8, 128
    add r9, 40
    cmp r8, 3968
    jb .Lpack10_loop
.Lpack10_last:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm9, ymm13, ymm12, ymm11, 12
    PACK16_Q4_MASK ymm2, ymm3, 64, ymm14, ymm9, ymm13, ymm12, ymm11, 12
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10
    STORE10 xmm0, 0
    vextracti128 xmm1, ymm0, 1
    STORE10 xmm1, 10
    STORE10 xmm2, 20
    vextracti128 xmm3, ymm2, 1
    STORE10 xmm3, 30
    vzeroupper
    ret
.size poly_pack10_avx, .-poly_pack10_avx

.globl poly_pack11_avx
.type poly_pack11_avx, @function
poly_pack11_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack11_offset]
    vbroadcasti128 ymm13, XMMWORD PTR [rip + .Lpack11_pairmul]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack11_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack11_hi]
    vmovdqa ymm10, YMMWORD PTR [rip + .Lpack11_mask]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack11_loop:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 10
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 44, 20
    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 11], ymm0, 1
    add r8, 64
    add r9, 22
    cmp r8, 4032
    jb .Lpack11_loop
.Lpack11_last:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 10
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 44, 20
    STORE11 xmm0, 0
    vextracti128 xmm1, ymm0, 1
    STORE11 xmm1, 11
    vzeroupper
    ret
.size poly_pack11_avx, .-poly_pack11_avx

.globl poly_pack13_avx
.type poly_pack13_avx, @function
poly_pack13_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack13_offset]
    vbroadcasti128 ymm13, XMMWORD PTR [rip + .Lpack13_pairmul]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack13_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack13_hi]
    vmovdqa ymm10, YMMWORD PTR [rip + .Lpack13_mask]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack13_loop:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 6
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 52, 12
    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 13], ymm0, 1
    add r8, 64
    add r9, 26
    cmp r8, 4032
    jb .Lpack13_loop
.Lpack13_last:
    PACK16_Q4_MASK ymm0, ymm1, 0, ymm14, ymm10, ymm13, ymm12, ymm11, 6
    PACK_ODD_BLEND ymm0, ymm1, ymm2, 52, 12
    STORE13 xmm0, 0
    vextracti128 xmm1, ymm0, 1
    STORE13 xmm1, 13
    vzeroupper
    ret
.size poly_pack13_avx, .-poly_pack13_avx

.globl poly_pack14_avx
.type poly_pack14_avx, @function
poly_pack14_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack14_offset]
    vmovdqa ymm9, YMMWORD PTR [rip + .Lpack14_mask]
    vbroadcasti128 ymm13, XMMWORD PTR [rip + .Lpack14_pairmul]
    vbroadcasti128 ymm12, XMMWORD PTR [rip + .Lpack14_lo28]
    vbroadcasti128 ymm11, XMMWORD PTR [rip + .Lpack14_hi28]
    vbroadcasti128 ymm10, XMMWORD PTR [rip + .Lpack14_shuf]
    xor r8d, r8d
    xor r9d, r9d

    .p2align 4
.Lpack14_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vmovdqu ymm2, YMMWORD PTR [rsi + r8 + 64]
    vmovdqu ymm3, YMMWORD PTR [rsi + r8 + 96]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpsubd ymm2, ymm14, ymm2
    vpsubd ymm3, ymm14, ymm3
    vpand ymm0, ymm0, ymm9
    vpand ymm1, ymm1, ymm9
    vpand ymm2, ymm2, ymm9
    vpand ymm3, ymm3, ymm9
    vpackusdw ymm0, ymm0, ymm1
    vpackusdw ymm2, ymm2, ymm3
    vpermq ymm0, ymm0, 0xd8
    vpermq ymm2, ymm2, 0xd8
    vpmaddwd ymm0, ymm0, ymm13
    vpmaddwd ymm2, ymm2, ymm13

    vpand ymm1, ymm0, ymm11
    vpand ymm0, ymm0, ymm12
    vpand ymm3, ymm2, ymm11
    vpand ymm2, ymm2, ymm12
    vpsrlq ymm1, ymm1, 4
    vpsrlq ymm3, ymm3, 4
    vpor ymm0, ymm0, ymm1
    vpor ymm2, ymm2, ymm3
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10

    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 14], ymm0, 1
    vmovdqu XMMWORD PTR [rdi + r9 + 28], xmm2
    vextracti128 XMMWORD PTR [rdi + r9 + 42], ymm2, 1

    add r8, 128
    add r9, 56
    cmp r8, 3968
    jb .Lpack14_loop

.Lpack14_last:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm1, YMMWORD PTR [rsi + r8 + 32]
    vmovdqu ymm2, YMMWORD PTR [rsi + r8 + 64]
    vmovdqu ymm3, YMMWORD PTR [rsi + r8 + 96]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm1, ymm14, ymm1
    vpsubd ymm2, ymm14, ymm2
    vpsubd ymm3, ymm14, ymm3
    vpand ymm0, ymm0, ymm9
    vpand ymm1, ymm1, ymm9
    vpand ymm2, ymm2, ymm9
    vpand ymm3, ymm3, ymm9
    vpackusdw ymm0, ymm0, ymm1
    vpackusdw ymm2, ymm2, ymm3
    vpermq ymm0, ymm0, 0xd8
    vpermq ymm2, ymm2, 0xd8
    vpmaddwd ymm0, ymm0, ymm13
    vpmaddwd ymm2, ymm2, ymm13

    vpand ymm1, ymm0, ymm11
    vpand ymm0, ymm0, ymm12
    vpand ymm3, ymm2, ymm11
    vpand ymm2, ymm2, ymm12
    vpsrlq ymm1, ymm1, 4
    vpsrlq ymm3, ymm3, 4
    vpor ymm0, ymm0, ymm1
    vpor ymm2, ymm2, ymm3
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10

    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 14], ymm0, 1
    vmovdqu XMMWORD PTR [rdi + r9 + 28], xmm2
    vextracti128 xmm3, ymm2, 1
    vmovq QWORD PTR [rdi + r9 + 42], xmm3
    vpextrd eax, xmm3, 2
    mov DWORD PTR [rdi + r9 + 50], eax
    vpextrw eax, xmm3, 6
    mov WORD PTR [rdi + r9 + 54], ax

    vzeroupper
    ret
.size poly_pack14_avx, .-poly_pack14_avx

.globl poly_pack19_avx
.type poly_pack19_avx, @function
poly_pack19_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack19_offset]
    vmovdqa ymm13, YMMWORD PTR [rip + .Lpack19_pair_lo]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack19_pair_hi]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack19_q_base_shr]
    vmovdqa ymm10, YMMWORD PTR [rip + .Lpack19_q_add1_shl]
    vmovdqa ymm9, YMMWORD PTR [rip + .Lpack19_q_add2_shl]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack19_loop:
    PACK19_QWORD_TWO_BLOCKS
    add r8, 64
    add r9, 38
    PACK19_QWORD_TWO_BLOCKS
    add r8, 64
    add r9, 38
    cmp r8, 3968
    jb .Lpack19_loop
    PACK19_QWORD_TWO_BLOCKS
    add r8, 64
    add r9, 38
    PACK19_QWORD_BLOCK 0, 0
    PACK19_QWORD_BLOCK 32, 19, 0
    vzeroupper
    ret
.size poly_pack19_avx, .-poly_pack19_avx

.globl poly_pack20_avx
.type poly_pack20_avx, @function
poly_pack20_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack20_offset]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack20_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack20_hi]
    vbroadcasti128 ymm10, XMMWORD PTR [rip + .Lpack10_shuf]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack20_loop:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm2, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm2, ymm14, ymm2
    vpand ymm1, ymm0, ymm11
    vpand ymm0, ymm0, ymm12
    vpand ymm3, ymm2, ymm11
    vpand ymm2, ymm2, ymm12
    vpsrlq ymm1, ymm1, 12
    vpsrlq ymm3, ymm3, 12
    vpor ymm0, ymm0, ymm1
    vpor ymm2, ymm2, ymm3
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10
    vmovdqu XMMWORD PTR [rdi + r9], xmm0
    vextracti128 XMMWORD PTR [rdi + r9 + 10], ymm0, 1
    vmovdqu XMMWORD PTR [rdi + r9 + 20], xmm2
    vextracti128 XMMWORD PTR [rdi + r9 + 30], ymm2, 1
    add r8, 64
    add r9, 40
    cmp r8, 4032
    jb .Lpack20_loop
.Lpack20_last:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm2, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm2, ymm14, ymm2
    vpand ymm1, ymm0, ymm11
    vpand ymm0, ymm0, ymm12
    vpand ymm3, ymm2, ymm11
    vpand ymm2, ymm2, ymm12
    vpsrlq ymm1, ymm1, 12
    vpsrlq ymm3, ymm3, 12
    vpor ymm0, ymm0, ymm1
    vpor ymm2, ymm2, ymm3
    vpshufb ymm0, ymm0, ymm10
    vpshufb ymm2, ymm2, ymm10
    STORE10 xmm0, 0
    vextracti128 xmm1, ymm0, 1
    STORE10 xmm1, 10
    STORE10 xmm2, 20
    vextracti128 xmm3, ymm2, 1
    STORE10 xmm3, 30
    vzeroupper
    ret
.size poly_pack20_avx, .-poly_pack20_avx

.globl poly_pack22_avx
.type poly_pack22_avx, @function
poly_pack22_avx:
    vmovdqa ymm14, YMMWORD PTR [rip + .Lpack22_offset]
    vmovdqa ymm12, YMMWORD PTR [rip + .Lpack22_lo]
    vmovdqa ymm11, YMMWORD PTR [rip + .Lpack22_hi]
    xor r8d, r8d
    xor r9d, r9d
    .p2align 4
.Lpack22_loop:
    PACK22_TWO_BLOCKS
    add r8, 64
    add r9, 44
    PACK22_TWO_BLOCKS
    add r8, 64
    add r9, 44
    PACK22_TWO_BLOCKS
    add r8, 64
    add r9, 44
    cmp r8, 4032
    jb .Lpack22_loop
.Lpack22_last:
    vmovdqu ymm0, YMMWORD PTR [rsi + r8]
    vmovdqu ymm4, YMMWORD PTR [rsi + r8 + 32]
    vpsubd ymm0, ymm14, ymm0
    vpsubd ymm4, ymm14, ymm4
    vpand ymm1, ymm0, ymm11
    vpand ymm5, ymm4, ymm11
    vpand ymm0, ymm0, ymm12
    vpand ymm4, ymm4, ymm12
    vpsrlq ymm1, ymm1, 10
    vpsrlq ymm5, ymm5, 10
    vpor ymm0, ymm0, ymm1
    vpor ymm4, ymm4, ymm5

    vpsrldq ymm2, ymm0, 8
    vpsrldq ymm6, ymm4, 8
    vpsllq ymm2, ymm2, 44
    vpsllq ymm6, ymm6, 44
    vpor ymm2, ymm2, ymm0
    vpor ymm6, ymm6, ymm4
    vpsrlq ymm1, ymm0, 20
    vpsrlq ymm5, ymm4, 20
    vpblendd ymm2, ymm2, ymm1, 0xcc
    vpblendd ymm6, ymm6, ymm5, 0xcc

    vmovdqu XMMWORD PTR [rdi + r9], xmm2
    vextracti128 XMMWORD PTR [rdi + r9 + 11], ymm2, 1
    STORE11 xmm6, 22
    vextracti128 xmm5, ymm6, 1
    STORE11 xmm5, 33
    vzeroupper
    ret
.size poly_pack22_avx, .-poly_pack22_avx
