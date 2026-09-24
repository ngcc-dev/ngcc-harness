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
	vmovdqa oaep1296_16xq(%rip),       %ymm14
	vmovdqa oaep1296_16xhalf(%rip),    %ymm12
	vmovdqa oaep1296_16xneghalf(%rip), %ymm11
	mov     $9, %ecx

.p2align 5
.Lbaseadd_loop:
	BASEADD16 0
	BASEADD16 32
	BASEADD16 64
	BASEADD16 96
	BASEADD16 128
	BASEADD16 160
	BASEADD16 192
	BASEADD16 224
	BASEADD16 256

	add $288, %rsi
	add $288, %rdx
	add $288, %rdi
	dec %ecx
	jnz .Lbaseadd_loop

	vzeroupper
	ret
.size poly_baseadd_asm, .-poly_baseadd_asm

.section .note.GNU-stack,"",@progbits
