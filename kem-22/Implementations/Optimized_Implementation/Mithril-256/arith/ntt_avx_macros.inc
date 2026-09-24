.macro shuffle8 r0, r1, r2, r3
    vperm2i128 ymm\r2, ymm\r0, ymm\r1, 0x20
    vperm2i128 ymm\r3, ymm\r0, ymm\r1, 0x31
.endm

.macro shuffle4 r0, r1, r2, r3
    vpunpcklqdq ymm\r2, ymm\r0, ymm\r1
    vpunpckhqdq ymm\r3, ymm\r0, ymm\r1
.endm

.macro shuffle2 r0, r1, r2, r3
    vmovsldup ymm\r2, ymm\r1
    vpblendd ymm\r2, ymm\r0, ymm\r2, 0xaa
    vpsrlq ymm\r0, ymm\r0, 32
    vpblendd ymm\r3, ymm\r0, ymm\r1, 0xaa
.endm

.macro reduce_low_oct x, tmp, tmp2
    vpsrad ymm\tmp, ymm\x, 31
    vpaddd ymm\tmp2, ymm0, ymm0
    vpand ymm\tmp, ymm\tmp, ymm\tmp2
    vpsubd ymm\tmp, ymm\tmp, ymm0
    vpaddd ymm\x, ymm\x, ymm\tmp
.endm
