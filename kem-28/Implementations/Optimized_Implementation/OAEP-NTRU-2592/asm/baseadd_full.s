.text

.macro CENTER16 reg tmp
	vpcmpgtw %ymm12, \reg, \tmp
	vpand    %ymm14, \tmp, \tmp
	vpsubw   \tmp, \reg, \reg
	vpcmpgtw \reg, %ymm11, \tmp
	vpand    %ymm14, \tmp, \tmp
	vpaddw   \tmp, \reg, \reg
.endm

.macro BASEADD16 offset
	vmovdqu \offset(%rsi), %ymm0
	vpaddw  \offset(%rdx), %ymm0, %ymm0
	CENTER16 %ymm0, %ymm1
	vmovdqu %ymm0, \offset(%rdi)
.endm

.global poly_baseadd_asm
.type poly_baseadd_asm, @function
poly_baseadd_asm:
	vmovdqa oaep2592_16xq(%rip), %ymm14
	vmovdqa baseadd_16xupper(%rip), %ymm12
	vmovdqa baseadd_16xlower(%rip), %ymm11
	mov     $27, %ecx

.p2align 5
.Lbaseadd_loop:
	BASEADD16 0
	BASEADD16 32
	BASEADD16 64
	BASEADD16 96
	BASEADD16 128
	BASEADD16 160

	add $192, %rsi
	add $192, %rdx
	add $192, %rdi
	dec %ecx
	jnz .Lbaseadd_loop

	vzeroupper
	ret
.size poly_baseadd_asm, .-poly_baseadd_asm

.section .rodata
.p2align 5
baseadd_16xupper:
	.short 14255, 14255, 14255, 14255, 14255, 14255, 14255, 14255
	.short 14255, 14255, 14255, 14255, 14255, 14255, 14255, 14255
baseadd_16xlower:
	.short -14256, -14256, -14256, -14256, -14256, -14256, -14256, -14256
	.short -14256, -14256, -14256, -14256, -14256, -14256, -14256, -14256

.section .note.GNU-stack,"",@progbits
