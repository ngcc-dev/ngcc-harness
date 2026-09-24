#include "consts.h"

.text
.global cdecl(pointwise_avx)
cdecl(pointwise_avx):
	vmovdqa		_8XQINV*4(%rcx), %ymm0
	vmovdqa		_8XQ*4(%rcx), %ymm1

	xor		%eax, %eax
.Lpointwise_loop:
	vmovdqa		(%rsi), %ymm2
	vmovdqa		(%rdx), %ymm10
	vpsrlq		$32, %ymm2, %ymm3
	vpsrlq		$32, %ymm10, %ymm11

	vpmuldq		%ymm2, %ymm10, %ymm2
	vpmuldq		%ymm3, %ymm11, %ymm3

	vpmuludq	%ymm0, %ymm2, %ymm10
	vpmuludq	%ymm0, %ymm3, %ymm11
	vpmuludq	%ymm1, %ymm10, %ymm10
	vpmuludq	%ymm1, %ymm11, %ymm11
	vpsubq		%ymm10, %ymm2, %ymm2
	vpsubq		%ymm11, %ymm3, %ymm3
	vpsrlq		$32, %ymm2, %ymm2

	vpblendd	$0xAA, %ymm3, %ymm2, %ymm2
	vmovdqa		%ymm2, (%rdi)

	add		$32, %rdi
	add		$32, %rsi
	add		$32, %rdx
	add		$1, %eax
	cmp		$(_POINTWISE_NBLOCKS), %eax
	jb		.Lpointwise_loop

	ret

.section .note.GNU-stack,"",@progbits
