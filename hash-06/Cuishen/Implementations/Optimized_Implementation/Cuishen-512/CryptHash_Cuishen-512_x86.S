/*
 * Linux x86-64 AVX2+BMI2 assembly baseline for Cuishen-512.
 *
 * This is a mechanically split GCC 11.4 -O3 assembly output generated from:
 *   Cuishen-512/CryptHash_Cuishen-512-c-avx2.c
 *
 * Build with AVX2/BMI/BMI2/ADX enabled; do not use AVX-512 flags here.
 */

	.file	"CryptHash_Cuishen-512-c-avx2.c"
	.intel_syntax noprefix
# GNU C11 (Ubuntu 11.4.0-1ubuntu1~22.04.3) version 11.4.0 (x86_64-linux-gnu)
#	compiled by GNU C version 11.4.0, GMP version 6.2.1, MPFR version 4.1.0, MPC version 1.2.1, isl version isl-0.24-GMP

# GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
# options passed: -mavx2 -mbmi -mbmi2 -madx -masm=intel -mtune=generic -march=x86-64 -O3 -std=c11 -fasynchronous-unwind-tables -fstack-protector-strong -fstack-clash-protection -fcf-protection
	.text
	.p2align 4
	.globl	CryptHash
	.type	CryptHash, @function
CryptHash:
.LFB5496:
	.cfi_startproc
	endbr64	
	push	rbp	#
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	mov	rbp, rsp	#,
	.cfi_def_cfa_register 6
	push	r15	#
	push	r14	#
	push	r13	#
	push	r12	#
	push	rbx	#
	and	rsp, -32	#,
	sub	rsp, 608	#,
	.cfi_offset 15, -24
	.cfi_offset 14, -32
	.cfi_offset 13, -40
	.cfi_offset 12, -48
	.cfi_offset 3, -56
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:391:               unsigned long long msg_len_bits, unsigned char *digest) {
	mov	QWORD PTR 56[rsp], rsi	# %sfp, msg
	mov	QWORD PTR 48[rsp], rdx	# %sfp, msg_len_bits
	mov	QWORD PTR 32[rsp], rcx	# %sfp, digest
	mov	rax, QWORD PTR fs:40	# tmp1825, MEM[(<address-space-1> long unsigned int *)40B]
	mov	QWORD PTR 600[rsp], rax	# D.33754, tmp1825
	xor	eax, eax	# tmp1825
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:401:   if (digest_len_bits != DIGEST_BIT_LENGTH || digest == 0) {
	cmp	edi, 512	# tmp1820,
	jne	.L48	#,
	test	rcx, rcx	# digest
	je	.L48	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:404:   if (msg_len_bits != 0ULL && msg == 0) {
	test	rdx, rdx	# msg_len_bits
	je	.L54	#,
	test	rsi, rsi	# msg
	je	.L48	#,
.L54:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:418:   rem_bits = msg_len_bits & 1023ULL;
	mov	rax, QWORD PTR 48[rsp]	# msg_len_bits, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:408:   t[0] = IV_1[0];
	vmovdqa	ymm0, YMMWORD PTR .LC0[rip]	# tmp1118,
	vmovdqa	YMMWORD PTR 224[rsp], ymm0	# MEM <vector(4) long unsigned int> [(long unsigned int *)&t], tmp1118
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:418:   rem_bits = msg_len_bits & 1023ULL;
	mov	rsi, rax	# rem_bits, msg_len_bits
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:412:   b[0] = IV_2[0];
	vmovdqa	ymm1, YMMWORD PTR .LC10[rip]	# tmp1119,
	vmovdqa	YMMWORD PTR 256[rsp], ymm1	# MEM <vector(4) long unsigned int> [(long unsigned int *)&b], tmp1119
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:418:   rem_bits = msg_len_bits & 1023ULL;
	and	esi, 1023	# rem_bits,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:422:   for (block = 0ULL; block < full_blocks; ++block) {
	shr	rax, 10	# full_blocks,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:418:   rem_bits = msg_len_bits & 1023ULL;
	mov	QWORD PTR 40[rsp], rsi	# %sfp, rem_bits
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:422:   for (block = 0ULL; block < full_blocks; ++block) {
	mov	QWORD PTR 24[rsp], rax	# %sfp, full_blocks
	je	.L49	#,
	add	rax, 1	# tmp1120,
	lea	rsi, PI[rip+512]	# tmp1811,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:420:   processed_bits = MESSAGE_BLOCK_BITS;
	mov	QWORD PTR 80[rsp], 1024	# %sfp,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:422:   for (block = 0ULL; block < full_blocks; ++block) {
	movabs	r10, -3877023728269904647	# bk3,
	sal	rax, 10	# tmp1120,
	mov	QWORD PTR 88[rsp], rsi	# %sfp, tmp1811
	movabs	r11, -5447325171898657010	# bk2,
	movabs	r9, 8218451384177679172	# bk1,
	mov	QWORD PTR 64[rsp], rax	# %sfp, tmp1120
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:419:   p = msg;
	mov	rax, QWORD PTR 56[rsp]	# msg, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:422:   for (block = 0ULL; block < full_blocks; ++block) {
	movabs	r15, 4970063267203923202	# bk0,
	vmovdqa	ymm2, YMMWORD PTR .LC0[rip]	# keep t as [t0,t1,t2,t3] for full-block setup
	vmovdqa	ymm7, YMMWORD PTR .LC2[rip]	# tmp1814,
	vmovdqa	ymm3, YMMWORD PTR .LC3[rip]	# tmp1815,
	vmovdqa	ymm4, YMMWORD PTR .LC4[rip]	# tmp1816,
	vmovdqa	ymm5, YMMWORD PTR .LC5[rip]	# tmp1817,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:419:   p = msg;
	mov	QWORD PTR 72[rsp], rax	# %sfp, msg
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:422:   for (block = 0ULL; block < full_blocks; ++block) {
	vmovdqa	ymm6, YMMWORD PTR .LC6[rip]	# tmp1818,
	.p2align 4,,10
	.p2align 3



# Cuishen-512 AVX2 v2 setup.
#
# Main changes versus v1:
#   * state layout: ymm0=[x0,x1,y0,y1], ymm1=[x2,x3,y2,y3]
#   * message group layout in memory: [b,a,c,d]
#   * block-key layout: ymm12=[b0,b1,b3,b2]
#   * PI table: 16-byte entries [PI[i],0], broadcast to [PI,0,PI,0]
#
# Entry contract is unchanged from the supplied v1 fragments:
#   ymm2               = t[0..3]
#   r15,r9,r11,r10     = b[0..3]
#   ymm3..ymm6         = original shift-count vectors
#   ymm7                = byte-swap mask
#   [rsp+72]            = message-block pointer
#   [rsp+80]            = c1 / processed_bits
#   [rsp+64]            = end processed-bits value
#
# The outer loop leaves the next hash B in physical [b0,b1,b3,b2] order.
.L7:
	vmovq	xmm12, r15
	vpinsrq	xmm12, xmm12, r9, 1
	vmovq	xmm13, r11
	vpinsrq	xmm13, xmm13, r10, 1
	vinserti128	ymm12, ymm12, xmm13, 1
	vpermilpd	ymm12, ymm12, 0x06       # [b0,b1,b3,b2]

	# Convert the compiler-created [a,a,b,b] shift vectors once to
	# [a,b,a,b], matching the x/x/y/y state layout.
	vpermq	ymm3, ymm3, 0xD8
	vpermq	ymm4, ymm4, 0xD8
	vpermq	ymm5, ymm5, 0xD8
	vpermq	ymm6, ymm6, 0xD8

.L7_bk_ready:
	mov	rsi, QWORD PTR 72[rsp]

	# t -> x/x/y/y state.  Bottom lane flips t3 bit 0 for domain separation.
	vpermq	ymm1, ymm2, 0xEE             # [t2,t3,t2,t3]
	vpxor	ymm1, ymm1, YMMWORD PTR .LC9[rip]

	# Load and byte-swap the 16 message words.  ymm7 remains intact.
	vmovdqu	ymm10, YMMWORD PTR [rsi]
	vpshufb	ymm10, ymm10, ymm7          # M0=[m0,m1,m2,m3]
	vmovdqu	ymm9, YMMWORD PTR 32[rsi]
	vpshufb	ymm9, ymm9, ymm7            # M1=[m4,m5,m6,m7]
	vmovdqu	ymm8, YMMWORD PTR 64[rsi]
	vpshufb	ymm8, ymm8, ymm7            # M2=[m8,m9,m10,m11]
	vmovdqu	ymm11, YMMWORD PTR 96[rsi]
	vpshufb	ymm11, ymm11, ymm7          # M3=[m12,m13,m14,m15]

	# Logical groups are still A/B/C/D, but physical storage is [b,a,c,d].
	# This lets the round loop fetch each duplicated 128-bit pair with one
	# VBROADCASTI128, instead of a YMM load plus two cross-lane permutations.
	vperm2i128	ymm13, ymm8, ymm10, 0x20 # [m8,m9,m0,m1]
	vpermilpd	ymm13, ymm13, 0x09        # [m9,m8,m0,m1]
	vmovdqa	YMMWORD PTR 96[rsp], ymm13

	vperm2i128	ymm13, ymm11, ymm9, 0x31 # [m14,m15,m6,m7]
	vpermilpd	ymm13, ymm13, 0x09        # [m15,m14,m6,m7]
	vmovdqa	YMMWORD PTR 128[rsp], ymm13

	vperm2i128	ymm13, ymm9, ymm11, 0x20 # [m4,m5,m12,m13]
	vpermilpd	ymm13, ymm13, 0x09        # [m5,m4,m12,m13]
	vmovdqa	YMMWORD PTR 160[rsp], ymm13

	vperm2i128	ymm13, ymm8, ymm10, 0x31 # [m10,m11,m2,m3]
	vpermilpd	ymm13, ymm13, 0x09        # [m11,m10,m2,m3]
	vmovdqa	YMMWORD PTR 192[rsp], ymm13

	# Current B pair vectors in alternating-lane form.
	vperm2i128	ymm9, ymm12, ymm12, 0x00 # [b0,b1,b0,b1]
	vperm2i128	ymm10, ymm12, ymm12, 0x11# [b3,b2,b3,b2]

	mov	rsi, QWORD PTR 80[rsp]          # C
	vpermq	ymm0, ymm2, 0x50             # old [t0,t0,t1,t1]
	vpermq	ymm0, ymm0, 0xD8             # [x0,x1,y0,y1]

	lea	rcx, 96[rsp]                    # A/B/C/D base

	# D pair XORs carried into A.  Swapping a/b does not change these XORs.
	mov	r10, QWORD PTR 96[rcx]
	xor	r10, QWORD PTR 104[rcx]
	mov	r8, QWORD PTR 112[rcx]
	xor	r8, QWORD PTR 120[rcx]

	# ymm14=[C,0,C,0].
	vmovq	xmm14, rsi
	vinserti128	ymm14, ymm14, xmm14, 1

	lea	rdx, cuishen_pi_avx2[rip]
	lea	rdi, 1024[rdx]

	.p2align 5

# Cuishen-512 AVX2 hot loop.
# The PI constant table is embedded in this file.

.macro CUISHEN_MSG_UPDATE_HI_V2 target, sx, sy
	# Physical input [b,a,c,d]; physical output [f3,f2,f0,f1].
	rorx	r12, QWORD PTR [rcx+\target+8], 35
	rorx	rbx, \sx, 58
	add	r12, rbx
	rorx	r13, QWORD PTR [rcx+\target], 1
	rorx	rbx, \sx, 52
	add	r13, rbx
	rorx	r14, QWORD PTR [rcx+\target+16], 50
	rorx	rbx, \sy, 45
	add	r14, rbx
	rorx	r15, QWORD PTR [rcx+\target+24], 21
	rorx	rbx, \sy, 36
	add	r15, rbx
	mov	QWORD PTR [rcx+\target], r15
	mov	QWORD PTR [rcx+\target+8], r14
	mov	QWORD PTR [rcx+\target+16], r12
	mov	QWORD PTR [rcx+\target+24], r13
	xor	r14, r15
	xor	r12, r13
.endm

.macro CUISHEN_MSG_UPDATE_LO_V2 target, sx, sy
	rorx	r8, QWORD PTR [rcx+\target+8], 35
	rorx	rbx, \sx, 58
	add	r8, rbx
	rorx	r9, QWORD PTR [rcx+\target], 1
	rorx	rbx, \sx, 52
	add	r9, rbx
	rorx	r10, QWORD PTR [rcx+\target+16], 50
	rorx	rbx, \sy, 45
	add	r10, rbx
	rorx	r11, QWORD PTR [rcx+\target+24], 21
	rorx	rbx, \sy, 36
	add	r11, rbx
	mov	QWORD PTR [rcx+\target], r11
	mov	QWORD PTR [rcx+\target+8], r10
	mov	QWORD PTR [rcx+\target+16], r8
	mov	QWORD PTR [rcx+\target+24], r9
	xor	r10, r11
	xor	r8, r9
.endm

.L5:
	# Four block-key recurrences at once.
	# Input Q=[b0,b1,b3,b2], output Q'=[b4,b5,b7,b6].
	vpsrlq	ymm13, ymm12, 37
	vpsllq	ymm2, ymm12, 27
	vpor	ymm13, ymm13, ymm2
	vpermq	ymm11, ymm12, 0x8D
	vpxor	ymm11, ymm11, ymm13
	vpermq	ymm2, ymm11, 0
	vpxor	ymm2, ymm2, ymm13
	vpblendd	ymm13, ymm11, ymm2, 0x30

	# ---- round 0 / A ----
	# AB term: [m0,m1]*2 ^ [b0,b1]*2 ^ [PI,C]*2 ^ state01.
	vbroadcasti128	ymm11, XMMWORD PTR 16[rcx]
	vpxor	ymm11, ymm11, ymm9
	vbroadcasti128	ymm15, XMMWORD PTR [rdx]
	vshufpd	ymm15, ymm15, ymm14, 0x00
	vpxor	ymm11, ymm11, ymm15
	vpxor	ymm11, ymm11, ymm0
	vpsrlvq	ymm15, ymm11, ymm5
	vpsllvq	ymm11, ymm11, ymm6
	vpor	ymm11, ymm11, ymm15

	# DC term: [m9,m8]*2 ^ [b3,b2]*2 ^ pairxor(state23).
	vbroadcasti128	ymm2, XMMWORD PTR [rcx]
	vpxor	ymm2, ymm2, ymm10
	vpermilpd	ymm15, ymm1, 0x05
	vpxor	ymm15, ymm15, ymm1
	vpxor	ymm15, ymm15, ymm2
	vpsrlvq	ymm2, ymm15, ymm3
	vpsllvq	ymm15, ymm15, ymm4
	vpor	ymm2, ymm2, ymm15
	vpaddq	ymm0, ymm11, ymm2
	CUISHEN_MSG_UPDATE_HI_V2 0, r10, r8

	# ---- round 1 / B ----
	# B01=[b1,b2]*2.
	vpunpckhqdq	ymm15, ymm9, ymm10
	vbroadcasti128	ymm11, XMMWORD PTR 48[rcx]
	vpxor	ymm11, ymm11, ymm15
	vbroadcasti128	ymm15, XMMWORD PTR 16[rdx]
	vpxor	ymm15, ymm15, ymm14
	vpxor	ymm11, ymm11, ymm15
	vpxor	ymm11, ymm11, ymm1
	vpsrlvq	ymm15, ymm11, ymm5
	vpsllvq	ymm11, ymm11, ymm6
	vpor	ymm11, ymm11, ymm15

	# Cache next B01=[b4,b5]*2; round-1 B23=[b4,b3]*2.
	vperm2i128	ymm9, ymm13, ymm13, 0x00
	vpunpcklqdq	ymm15, ymm9, ymm10
	vbroadcasti128	ymm2, XMMWORD PTR 32[rcx]
	vpxor	ymm2, ymm2, ymm15
	vpermilpd	ymm15, ymm0, 0x05
	vpxor	ymm15, ymm15, ymm0
	vpxor	ymm15, ymm15, ymm2
	vpsrlvq	ymm2, ymm15, ymm3
	vpsllvq	ymm15, ymm15, ymm4
	vpor	ymm2, ymm2, ymm15
	vpaddq	ymm1, ymm11, ymm2
	CUISHEN_MSG_UPDATE_LO_V2 32, r14, r12

	# C2=rol2(C), C4=rol4(C); ymm14=[C2,0,C2,0].
	rorx	rax, rsi, 62
	rorx	rsi, rsi, 60
	vmovq	xmm14, rax
	vinserti128	ymm14, ymm14, xmm14, 1

	# ---- round 2 / C ----
	# B01=[b2,b3]*2.
	vpermilpd	ymm15, ymm10, 0x05
	vbroadcasti128	ymm11, XMMWORD PTR 80[rcx]
	vpxor	ymm11, ymm11, ymm15
	vbroadcasti128	ymm15, XMMWORD PTR 32[rdx]
	vshufpd	ymm15, ymm15, ymm14, 0x00
	vpxor	ymm11, ymm11, ymm15
	vpxor	ymm11, ymm11, ymm0
	vpsrlvq	ymm15, ymm11, ymm5
	vpsllvq	ymm11, ymm11, ymm6
	vpor	ymm11, ymm11, ymm15

	# B23=[b5,b4]*2.
	vpermilpd	ymm15, ymm9, 0x05
	vbroadcasti128	ymm2, XMMWORD PTR 64[rcx]
	vpxor	ymm2, ymm2, ymm15
	vpermilpd	ymm15, ymm1, 0x05
	vpxor	ymm15, ymm15, ymm1
	vpxor	ymm15, ymm15, ymm2
	vpsrlvq	ymm2, ymm15, ymm3
	vpsllvq	ymm15, ymm15, ymm4
	vpor	ymm2, ymm2, ymm15
	vpaddq	ymm0, ymm11, ymm2
	CUISHEN_MSG_UPDATE_HI_V2 64, r10, r8

	# ---- round 3 / D ----
	# B01=[b3,b4]*2.
	vpunpcklqdq	ymm15, ymm10, ymm9
	vbroadcasti128	ymm11, XMMWORD PTR 112[rcx]
	vpxor	ymm11, ymm11, ymm15
	vbroadcasti128	ymm15, XMMWORD PTR 48[rdx]
	vpxor	ymm15, ymm15, ymm14
	vpxor	ymm11, ymm11, ymm15
	vpxor	ymm11, ymm11, ymm1
	vpsrlvq	ymm15, ymm11, ymm5
	vpsllvq	ymm11, ymm11, ymm6
	vpor	ymm11, ymm11, ymm15

	# Cache next B23=[b7,b6]*2; round-3 B23=[b6,b5]*2.
	vperm2i128	ymm10, ymm13, ymm13, 0x11
	vpunpckhqdq	ymm15, ymm10, ymm9
	vbroadcasti128	ymm2, XMMWORD PTR 96[rcx]
	vpxor	ymm2, ymm2, ymm15
	vpermilpd	ymm15, ymm0, 0x05
	vpxor	ymm15, ymm15, ymm0
	vpxor	ymm15, ymm15, ymm2
	vpsrlvq	ymm2, ymm15, ymm3
	vpsllvq	ymm15, ymm15, ymm4
	vpor	ymm2, ymm2, ymm15
	vpaddq	ymm1, ymm11, ymm2
	CUISHEN_MSG_UPDATE_LO_V2 96, r14, r12

	vmovdqa	ymm12, ymm13
	add	rdx, 64
	vmovq	xmm14, rsi
	vinserti128	ymm14, ymm14, xmm14, 1
	cmp	rdx, rdi
	jne	.L5

	# x/x/y/y state -> canonical t and b.
	vperm2i128	ymm2, ymm0, ymm1, 0x20
	vperm2i128	ymm12, ymm0, ymm1, 0x31
	vpxor	ymm2, ymm2, YMMWORD PTR 224[rsp]
	vmovdqa	ymm15, YMMWORD PTR 224[rsp]
	vpxor	ymm15, ymm15, YMMWORD PTR .LC9[rip]
	vpxor	ymm12, ymm12, ymm15
	vmovdqa	YMMWORD PTR 224[rsp], ymm2

	sub	QWORD PTR 72[rsp], -128
	add	QWORD PTR 80[rsp], 1024
	mov	rax, QWORD PTR 80[rsp]

	# Keep B in physical [b0,b1,b3,b2] order across outer blocks.
	vpermilpd	ymm12, ymm12, 0x06
	cmp	rax, QWORD PTR 64[rsp]
	je	.L6
	jmp	.L7_bk_ready

.purgem CUISHEN_MSG_UPDATE_HI_V2
.purgem CUISHEN_MSG_UPDATE_LO_V2

.L6:
	vmovdqa	YMMWORD PTR 224[rsp], ymm2
	mov	rax, QWORD PTR 24[rsp]

	# The outer loop keeps B as [b0,b1,b3,b2]; restore canonical order for
	# the compiler-generated finalization code and its live-out convention.
	vpermilpd	ymm0, ymm12, 0x06
	# full_block v2 converted rotate-count vectors to [a,b,a,b].  The
	# tail/finalization fragments are still compiler-shaped and expect
	# [a,a,b,b], so convert them back at the full-block exit.
	vpermq	ymm3, ymm3, 0xD8
	vpermq	ymm4, ymm4, 0xD8
	vpermq	ymm5, ymm5, 0xD8
	vpermq	ymm6, ymm6, 0xD8
	vmovq	r15, xmm0
	vpextrq	r9, xmm0, 1
	vextracti128	xmm13, ymm0, 1
	vmovq	r11, xmm13
	vpextrq	r10, xmm13, 1
	vmovdqa	YMMWORD PTR 256[rsp], ymm0

	sal	rax, 7
	add	QWORD PTR 56[rsp], rax

.L4:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:178:   const unsigned long long rem_bytes = rem_bits >> 3;
	mov	rax, QWORD PTR 40[rsp]	# rem_bits, %sfp
	mov	rdi, rax	# tmp1308, rem_bits
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:193:   if (partial_bytes != 0U || rem_tail_bits != 0U) {
	mov	r9d, eax	# tmp1309, rem_bits
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:179:   const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
	mov	rsi, rax	# _5380, rem_bits
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:181:   const unsigned int rem_tail_bits = (unsigned int)(rem_bits & 7ULL);
	mov	r8d, eax	# _5383, rem_bits
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:178:   const unsigned long long rem_bytes = rem_bits >> 3;
	shr	rdi, 3	# tmp1308,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:179:   const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
	shr	rsi, 6	# _5380,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:181:   const unsigned int rem_tail_bits = (unsigned int)(rem_bits & 7ULL);
	and	r8d, 7	# _5383,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:193:   if (partial_bytes != 0U || rem_tail_bits != 0U) {
	or	r9d, edi	# tmp1309, tmp1308
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:179:   const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
	mov	edx, esi	# zero_from, _5380
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:180:   const unsigned int partial_bytes = (unsigned int)(rem_bytes & 7ULL);
	and	edi, 7	# _5382,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:193:   if (partial_bytes != 0U || rem_tail_bits != 0U) {
	and	r9d, 7	# _5379,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:428:   if (rem_bits > FINAL_TAIL_BITS) {
	cmp	rax, 768	# rem_bits,
	ja	.L197	#,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR 416[rsp], ymm2	# MEM[(__m256i * {ref-all})&final_key], _681
	vmovdqa	YMMWORD PTR 448[rsp], ymm0	# MEM[(__m256i * {ref-all})&final_key + 32B], _683
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:185:   for (; word + 3U < full_words; word += 4U) {
	cmp	rsi, 3	# _5380,
	jbe	.L51	#,
	lea	r10d, -4[rsi]	# _4672,
	mov	rbx, QWORD PTR 56[rsp]	# msg, %sfp
	xor	eax, eax	# ivtmp.113
	lea	r11, 480[rsp]	# tmp1813,
	mov	ecx, r10d	# tmp1515, _4672
	shr	ecx, 2	# tmp1515,
	add	ecx, 1	#,
	sal	rcx, 5	# _4676,
	.p2align 4,,10
	.p2align 3
.L29:
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:590:   return (__m256i) __builtin_ia32_pshufb256 ((__v32qi)__X,
	vmovdqu	ymm2, YMMWORD PTR [rbx+rax]	# tmp2055, MEM[(const __m256i_u * {ref-all})_726]
	vpshufb	ymm0, ymm2, ymm7	# tmp1519, tmp2055, tmp1814
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR [r11+rax], ymm0	# MEM[(__m256i * {ref-all})&final_key + 64B + ivtmp.113_4668 * 1], tmp1519
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:185:   for (; word + 3U < full_words; word += 4U) {
	add	rax, 32	# ivtmp.113,
	cmp	rax, rcx	# ivtmp.113, _4676
	jne	.L29	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:185:   for (; word + 3U < full_words; word += 4U) {
	and	r10d, -4	# tmp1524,
	lea	eax, 4[r10]	# word,
.L28:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	cmp	eax, esi	# word, _5380
	jnb	.L33	#,
	mov	rbx, QWORD PTR 56[rsp]	# msg, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	lea	r10d, 0[0+rax*8]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	mov	r11d, eax	# word, word
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	mov	rcx, r10	#,
	mov	r10, QWORD PTR [rbx+r10]	# MEM <long unsigned int> [(const unsigned char *)_5251], MEM <long unsigned int> [(const unsigned char *)_5251]
	bswap	r10		# _5256
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	mov	QWORD PTR 480[rsp+r11*8], r10	# *_5254, _5256
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	lea	r10d, 1[rax]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	cmp	r10d, esi	# word, _5380
	jnb	.L33	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	lea	r11d, 8[rcx]	#,
	mov	r11, QWORD PTR [rbx+r11]	# MEM <long unsigned int> [(const unsigned char *)_5271], MEM <long unsigned int> [(const unsigned char *)_5271]
	bswap	r11		# _5276
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	mov	QWORD PTR 480[rsp+r10*8], r11	# *_5274, _5276
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	lea	r10d, 2[rax]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	cmp	r10d, esi	# word, _5380
	jnb	.L33	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	lea	r11d, 16[rcx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	add	eax, 3	#,
	mov	r11, QWORD PTR [rbx+r11]	# MEM <long unsigned int> [(const unsigned char *)_5283], MEM <long unsigned int> [(const unsigned char *)_5283]
	bswap	r11		# _5288
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	mov	QWORD PTR 480[rsp+r10*8], r11	# *_5286, _5288
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	cmp	eax, esi	# word, _5380
	jnb	.L33	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	add	ecx, 24	#,
	mov	rcx, QWORD PTR [rbx+rcx]	# MEM <long unsigned int> [(const unsigned char *)_739], MEM <long unsigned int> [(const unsigned char *)_739]
	bswap	rcx		# _1089
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	mov	QWORD PTR 480[rsp+rax*8], rcx	# *_742, _1089
.L33:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:193:   if (partial_bytes != 0U || rem_tail_bits != 0U) {
	test	r9d, r9d	# _5379
	je	.L32	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:194:     const unsigned long long byte_offset = full_words << 3;
	lea	rax, 0[0+rsi*8]	# byte_offset,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	test	edi, edi	# _5382
	je	.L52	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	mov	rbx, QWORD PTR 56[rsp]	# msg, %sfp
	movzx	edx, BYTE PTR [rbx+rax]	# *_5179, *_5179
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rdx, 56	# w,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 1	# _5382,
	je	.L34	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 1[rbx+rax]	# *_5191, *_5191
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 48	# tmp1543,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rdx, rcx	# w, tmp1543
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 2	# _5382,
	je	.L34	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 2[rbx+rax]	# *_5203, *_5203
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 40	# tmp1545,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rdx, rcx	# w, tmp1545
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 3	# _5382,
	je	.L34	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 3[rbx+rax]	# *_5215, *_5215
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 32	# tmp1547,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rdx, rcx	# w, tmp1547
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 4	# _5382,
	je	.L34	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 4[rbx+rax]	# *_5227, *_5227
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 24	# tmp1549,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rdx, rcx	# w, tmp1549
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 5	# _5382,
	je	.L34	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 5[rbx+rax]	# *_5239, *_5239
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 16	# tmp1551,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rdx, rcx	# w, tmp1551
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 7	# _5382,
	jne	.L34	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 6[rbx+rax]	# *_752, *_752
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 8	# tmp1553,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rdx, rcx	# w, tmp1553
.L34:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:201:     if (rem_tail_bits != 0U) {
	test	r8d, r8d	# _5383
	je	.L36	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	mov	r9d, 8	# tmp1558,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	add	rax, QWORD PTR 56[rsp]	# tmp1555, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	mov	r10d, edi	# _5382, _5382
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	mov	ecx, 255	# tmp1560,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	sub	r9d, r8d	# tmp1557, _5383
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	shlx	ecx, ecx, r9d	# tmp1559, tmp1560, tmp1557
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	and	cl, BYTE PTR [rax+r10]	# tmp1561, *_766
	movzx	eax, cl	#, tmp1561
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:205:            << (56U - 8U * partial_bytes);
	mov	ecx, 7	# tmp1564,
	sub	ecx, edi	# tmp1563, _5382
	sal	ecx, 3	# tmp1565,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:205:            << (56U - 8U * partial_bytes);
	shlx	rax, rax, rcx	# tmp1566, tmp1562, tmp1565
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	or	rdx, rax	# w, tmp1566
.L36:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:208:     out[full_words] = w;
	mov	QWORD PTR 480[rsp+rsi*8], rdx	# *_777, w
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:209:     zero_from = full_words + 1U;
	lea	edx, 1[rsi]	# zero_from,
.L32:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	eax, edx	# zero_from, zero_from
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	dl, 3	# zero_from,
	je	.L27	#,
	cmp	edx, 11	# zero_from,
	ja	.L27	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5006,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 1[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L27	#,
	cmp	eax, 11	# word,
	ja	.L27	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5017,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 2[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L27	#,
	cmp	eax, 11	# word,
	ja	.L27	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5028,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 3[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L27	#,
	cmp	eax, 11	# word,
	ja	.L27	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5039,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 4[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	dl, 3	# zero_from,
	je	.L37	#,
	cmp	eax, 11	# word,
	ja	.L37	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5050,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 5[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L37	#,
	cmp	eax, 11	# word,
	ja	.L37	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5061,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 6[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L37	#,
	cmp	eax, 11	# word,
	ja	.L37	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5072,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 7[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L37	#,
	cmp	eax, 11	# word,
	ja	.L37	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5083,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 8[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	dl, 3	# zero_from,
	je	.L40	#,
	cmp	eax, 11	# word,
	ja	.L39	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5094,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 9[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L40	#,
	cmp	eax, 11	# word,
	ja	.L39	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 480[rsp+rax*8], 0	# *_5105,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 10[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L40	#,
	cmp	eax, 11	# word,
	ja	.L39	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 568[rsp], 0	# MEM[(uint64_t *)&final_key + 152B],

.L39:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:288:   uint64_t m2 = message_key[2];
	mov	rax, QWORD PTR 432[rsp]	# nm2, MEM[(const uint64_t *)&final_key + 16B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:286:   uint64_t m0 = message_key[0];
	mov	r10, QWORD PTR 416[rsp]	# nm2, MEM[(const uint64_t *)&final_key]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:305:   uint64_t bk3 = block_key[3];
	vpxor	xmm0, xmm0, xmm0	# v01
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:308:   const CUISHEN_AVX2_PI_QUAL uint64_t *pi_ptr = PI;
	lea	rdi, PI[rip]	# pi_ptr,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:287:   uint64_t m1 = message_key[1];
	mov	r9, QWORD PTR 424[rsp]	# nm3, MEM[(const uint64_t *)&final_key + 8B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:290:   uint64_t m4 = message_key[4];
	mov	r15, QWORD PTR 448[rsp]	# m4, MEM[(const uint64_t *)&final_key + 32B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:288:   uint64_t m2 = message_key[2];
	mov	QWORD PTR 128[rsp], rax	# %sfp, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:289:   uint64_t m3 = message_key[3];
	mov	rax, QWORD PTR 440[rsp]	# nm3, MEM[(const uint64_t *)&final_key + 24B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:292:   uint64_t m6 = message_key[6];
	mov	r8, QWORD PTR 464[rsp]	# nm2, MEM[(const uint64_t *)&final_key + 48B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:302:   uint64_t bk0 = block_key[0];
	mov	rsi, QWORD PTR 544[rsp]	# bk0, MEM[(const uint64_t *)&final_key + 128B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:289:   uint64_t m3 = message_key[3];
	mov	QWORD PTR 160[rsp], rax	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:291:   uint64_t m5 = message_key[5];
	mov	rax, QWORD PTR 456[rsp]	# m5, MEM[(const uint64_t *)&final_key + 40B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:303:   uint64_t bk1 = block_key[1];
	mov	rcx, QWORD PTR 552[rsp]	# bk1, MEM[(const uint64_t *)&final_key + 136B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:304:   uint64_t bk2 = block_key[2];
	mov	rdx, QWORD PTR 560[rsp]	# bk2, MEM[(const uint64_t *)&final_key + 144B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:291:   uint64_t m5 = message_key[5];
	mov	QWORD PTR 168[rsp], rax	# %sfp, m5
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:293:   uint64_t m7 = message_key[7];
	mov	rax, QWORD PTR 472[rsp]	# nm3, MEM[(const uint64_t *)&final_key + 56B]
	mov	QWORD PTR 184[rsp], rax	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:294:   uint64_t m8 = message_key[8];
	mov	rax, QWORD PTR 480[rsp]	# m8, MEM[(const uint64_t *)&final_key + 64B]
	mov	QWORD PTR 112[rsp], rax	# %sfp, m8
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:295:   uint64_t m9 = message_key[9];
	mov	rax, QWORD PTR 488[rsp]	# m9, MEM[(const uint64_t *)&final_key + 72B]
	mov	QWORD PTR 192[rsp], rax	# %sfp, m9
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:296:   uint64_t m10 = message_key[10];
	mov	rax, QWORD PTR 496[rsp]	# m10, MEM[(const uint64_t *)&final_key + 80B]
	mov	QWORD PTR 120[rsp], rax	# %sfp, m10
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:297:   uint64_t m11 = message_key[11];
	mov	rax, QWORD PTR 504[rsp]	# m11, MEM[(const uint64_t *)&final_key + 88B]
	mov	QWORD PTR 152[rsp], rax	# %sfp, m11
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:298:   uint64_t m12 = message_key[12];
	mov	rax, QWORD PTR 512[rsp]	# nm2, MEM[(const uint64_t *)&final_key + 96B]
	mov	QWORD PTR 136[rsp], rax	# %sfp, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:299:   uint64_t m13 = message_key[13];
	mov	rax, QWORD PTR 520[rsp]	# nm3, MEM[(const uint64_t *)&final_key + 104B]
	mov	QWORD PTR 176[rsp], rax	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:300:   uint64_t m14 = message_key[14];
	mov	rax, QWORD PTR 528[rsp]	# m14, MEM[(const uint64_t *)&final_key + 112B]
	mov	QWORD PTR 104[rsp], rax	# %sfp, m14
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:301:   uint64_t m15 = message_key[15];
	mov	rax, QWORD PTR 536[rsp]	# m15, MEM[(const uint64_t *)&final_key + 120B]
	mov	QWORD PTR 144[rsp], rax	# %sfp, m15
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:305:   uint64_t bk3 = block_key[3];
	mov	rax, QWORD PTR 568[rsp]	# bk3, MEM[(const uint64_t *)&final_key + 152B]
	vmovdqa	ymm1, YMMWORD PTR .LC1[rip]	# v23,
	mov	QWORD PTR 96[rsp], r10	# %sfp, nm2
	mov	r10, QWORD PTR 48[rsp]	# msg_len_bits, %sfp
	.p2align 4,,10
	.p2align 3

.L43:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r14, QWORD PTR 112[rsp]	# m8, %sfp
	mov	r11, r9	# tmp1672, nm3
	mov	r13, QWORD PTR 96[rsp]	# nm2, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm2, ymm1, ymm1, 1	# tmp1660, v23, v23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, rcx	# tmp1672, bk1
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm2, ymm2, ymm1	# tmp1663, tmp1660, v23
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, r14	# lane3.0_838, m8
	xor	r11, r10	# tmp1672, msg_len_bits
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r14, r14, 35	# tmp1684, m8,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rbx, rdx	# lane3.0_838, bk2
	vmovq	xmm9, rbx	# lane3.0_838, lane3.0_838
	mov	rbx, QWORD PTR 192[rsp]	# lane1.2_839, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rbx, rax	# lane1.2_839, bk3
	vmovq	xmm8, rbx	# lane1.2_839, lane1.2_839
	mov	rbx, QWORD PTR 120[rsp]	# m1011, %sfp
	xor	rbx, QWORD PTR 152[rsp]	# m1011, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm8, xmm8, xmm9	# tmp1666, lane1.2_839, lane3.0_838
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, rbx, 58	# tmp1685, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	add	r14, r12	# nm2, tmp1685
	mov	r12, r9	# m1, nm3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpermq	ymm8, ymm8, 0x50	# tmp1664, tmp1666
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r11	# lane3.0_850, tmp1672
	mov	r11, r13	# tmp1673, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r9, QWORD PTR 192[rsp], 1	# tmp1686, %sfp,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, rsi	# tmp1673, bk0
	xor	r11, QWORD PTR [rdi]	# tmp1673, MEM[(const uint64_t *)pi_ptr_1280]
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm2, ymm2, ymm8	# tmp1667, tmp1663, tmp1664
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, rbx, 52	# tmp1687, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm8, r11	# lane1.2_851, tmp1673
	add	r9, rbx	# nm3, tmp1687
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, r12, 21	# tmp1690, m1,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm8, xmm8, xmm9	# tmp1676, lane1.2_851, lane3.0_850
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm2, ymm3	# tmp1668, tmp1667, tmp1815
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rsi, rsi, 37	# tmp1692, bk0,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rsi, rcx	# bk0, bk1
	mov	r11, QWORD PTR 128[rsp]	# m23, %sfp
	xor	r11, QWORD PTR 160[rsp]	# m23, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm2, ymm2, ymm4	# tmp1670, tmp1667, tmp1816
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpermq	ymm8, ymm8, 0x50	# tmp1674, tmp1676
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, r11, 45	# tmp1689, m23,
	rorx	r11, r11, 36	# tmp1691, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	add	r11, r12	# m9, tmp1690
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r12, QWORD PTR 104[rsp]	# m14, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 192[rsp], r11	# %sfp, m9
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm0, ymm0, ymm8	# tmp1677, v01, tmp1674
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm2, ymm2, ymm10	# tmp1682, tmp1670, tmp1668
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, r13, 50	# tmp1688, m0,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, r12	# lane3.0_898, m14
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm8, ymm0, ymm5	# tmp1678, tmp1677, tmp1817
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	add	r13, rbx	# m8, tmp1689
	mov	QWORD PTR 96[rsp], r14	# %sfp, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, rax	# lane3.0_898, bk3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm0, ymm0, ymm6	# tmp1680, tmp1677, tmp1818
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r14, r9	# m23, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 112[rsp], r13	# %sfp, m8
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r11	# lane3.0_898, lane3.0_898
	mov	r11, QWORD PTR 144[rsp]	# lane1.2_899, %sfp
	mov	rbx, QWORD PTR 184[rsp]	# lane3.0_910, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm0, ymm0, ymm8	# tmp1683, tmp1680, tmp1678
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r13, QWORD PTR 192[rsp]	# m1011, %sfp
	xor	r11, rsi	# lane1.2_899, bk0
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm2, ymm2, ymm0	# _863, tmp1682, tmp1683
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rbx, rdx	# lane3.0_910, bk2
	vmovq	xmm0, r11	# lane1.2_899, lane1.2_899
	mov	r11, rcx	# tmp1705, bk1
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm8, ymm2, ymm2, 1	# tmp1693, _863, _863,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rcx, rcx, 37	# tmp1725, bk1,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, r10	# tmp1705, msg_len_bits
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm0, xmm0, xmm9	# tmp1699, lane1.2_899, lane3.0_898
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm8, ymm8, ymm2	# tmp1696, tmp1693, _863
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rcx, rdx	# bk1, bk2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpermq	ymm0, ymm0, 0x50	# tmp1697, tmp1699
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, r8	# rk0, nm2
	xor	r11, QWORD PTR 8[rdi]	# lane1.2_911, MEM[(const uint64_t *)pi_ptr_1280 + 8B]
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm8, ymm8, ymm0	# tmp1700, tmp1696, tmp1697
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, rbx	# lane3.0_910, lane3.0_910
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vmovq	xmm0, r11	# lane1.2_911, lane1.2_911
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, r13	# m1011, m1011
	mov	r11, r14	# m23, m23
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm0, xmm0, xmm9	# tmp1709, lane1.2_911, lane3.0_910
	mov	r13, r8	# m6, nm2
	vpermq	ymm0, ymm0, 0x50	# tmp1707, tmp1709
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r8, r12, 35	# tmp1717, m14,
	rorx	r13, r13, 50	# tmp1721, m6,
	rorx	r12, rbx, 58	# tmp1718, m1011,
	rorx	rbx, rbx, 52	# tmp1720, m1011,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm1, ymm0, ymm1	# tmp1710, tmp1707, v23
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm8, ymm3	# tmp1701, tmp1700, tmp1815
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	add	r8, r12	# nm2, tmp1718
	mov	r12, QWORD PTR 184[rsp]	# m7, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm0, ymm1, ymm5	# tmp1711, tmp1710, tmp1817
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm8, ymm8, ymm4	# tmp1703, tmp1700, tmp1816
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r14, QWORD PTR 144[rsp], 1	# tmp1719, %sfp,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	add	r14, rbx	# nm3, tmp1720
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, r11, 45	# tmp1722, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	add	rbx, r13	# m14, tmp1721
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r13, rsi	# lane3.0_953, bk0
	xor	r13, r15	# lane3.0_953, m4
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm1, ymm1, ymm6	# tmp1713, tmp1710, tmp1818
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r11, r11, 36	# tmp1724, m23,
	rorx	r12, r12, 21	# tmp1723, m7,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r13	# lane3.0_953, lane3.0_953
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	add	r12, r11	# m15, tmp1724
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r13, QWORD PTR 168[rsp]	# lane1.2_954, %sfp
	mov	r11, QWORD PTR 176[rsp]	# tmp1738, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm1, ymm1, ymm0	# tmp1716, tmp1713, tmp1711
	vpor	ymm8, ymm8, ymm10	# tmp1715, tmp1703, tmp1701
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 184[rsp], r14	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r13, rcx	# lane1.2_954, bk1
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm8, ymm8, ymm1	# _921, tmp1715, tmp1716
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, rax	# tmp1738, bk3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r14, r10, 62	# _939, msg_len_bits,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm1, r13	# lane1.2_954, lane1.2_954
	xor	r11, r14	# tmp1738, _939
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm0, ymm8, ymm8, 1	# tmp1726, _921, _921,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r13, QWORD PTR 136[rsp]	# nm2, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm1, xmm1, xmm9	# tmp1732, lane1.2_954, lane3.0_953
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm0, ymm0, ymm8	# tmp1729, tmp1726, _921
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r15, r15, 35	# tmp1750, m4,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 104[rsp], rbx	# %sfp, m14
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpermq	ymm1, ymm1, 0x50	# tmp1730, tmp1732
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r11	# lane3.0_965, tmp1738
	mov	r11, r13	# tmp1739, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, r13, 50	# tmp1754, m12,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, rdx	# tmp1739, bk2
	xor	r11, QWORD PTR 16[rdi]	# tmp1739, MEM[(const uint64_t *)pi_ptr_1280 + 16B]
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm0, ymm0, ymm1	# tmp1733, tmp1729, tmp1730
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rdx, rdx, 37	# tmp1758, bk2,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm1, r11	# lane1.2_966, tmp1739
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm0, ymm3	# tmp1734, tmp1733, tmp1815
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r10, r10, 60	# msg_len_bits, msg_len_bits,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, QWORD PTR 184[rsp]	# m23, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 144[rsp], r12	# %sfp, m15
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm1, xmm1, xmm9	# tmp1742, lane1.2_966, lane3.0_965
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm0, ymm0, ymm4	# tmp1736, tmp1733, tmp1816
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpermq	ymm1, ymm1, 0x50	# tmp1740, tmp1742
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, r8	# m23, nm2
	xor	rbx, r12	# m1011, m15
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm1, ymm1, ymm2	# tmp1743, tmp1740, _863
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rdx, rax	# bk2, bk3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, rbx, 58	# tmp1751, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	add	r15, r12	# nm2, tmp1751
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, rbx, 52	# tmp1753, m1011,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm2, ymm1, ymm5	# tmp1744, tmp1743, tmp1817
	mov	r12, QWORD PTR 176[rsp]	# m13, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm1, ymm1, ymm6	# tmp1746, tmp1743, tmp1818
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm0, ymm0, ymm10	# tmp1748, tmp1736, tmp1734
	add	rdi, 32	# pi_ptr,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 136[rsp], r15	# %sfp, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r15, QWORD PTR 168[rsp], 1	# tmp1752, %sfp,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	add	r15, rbx	# nm3, tmp1753
	mov	QWORD PTR 176[rsp], r15	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, r12, 21	# tmp1756, m13,
	rorx	r15, r11, 45	# tmp1755, m23,
	rorx	r11, r11, 36	# tmp1757, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	add	r15, r13	# m4, tmp1754
	lea	r13, [r12+r11]	# m5,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm1, ymm1, ymm2	# tmp1749, tmp1746, tmp1744
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 168[rsp], r13	# %sfp, m5
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r12, QWORD PTR 120[rsp]	# m10, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm0, ymm0, ymm1	# v01, tmp1748, tmp1749
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r13, QWORD PTR 128[rsp]	# nm2, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm1, ymm0, ymm0, 1	# tmp1759, v01, v01,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, r12	# lane3.0_1004, m10
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm1, ymm1, ymm0	# tmp1762, tmp1759, v01
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rbx, rcx	# lane3.0_1004, bk1
	mov	r11, r13	# tmp1771, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, r13, 50	# tmp1788, m2,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, rbx	# lane3.0_1004, lane3.0_1004
	mov	rbx, QWORD PTR 152[rsp]	# lane1.2_1005, %sfp
	xor	r11, rax	# tmp1771, bk3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rax, rax, 37	# tmp1792, bk3,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, r14	# rk0, _939
	xor	r11, QWORD PTR -8[rdi]	# lane1.2_1017, MEM[(const uint64_t *)_4657 + -8B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r14, r12, 35	# tmp1784, m10,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rbx, rdx	# lane1.2_1005, bk2
	xor	rax, rsi	# bk3, bk0
	vmovq	xmm2, rbx	# lane1.2_1005, lane1.2_1005
	mov	rbx, QWORD PTR 160[rsp]	# lane3.0_1016, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm2, xmm2, xmm9	# tmp1765, lane1.2_1005, lane3.0_1004
	vpermq	ymm2, ymm2, 0x50	# tmp1763, tmp1765
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rbx, rsi	# lane3.0_1016, bk0
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm1, ymm1, ymm2	# tmp1766, tmp1762, tmp1763
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, rbx	# lane3.0_1016, lane3.0_1016
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vmovq	xmm2, r11	# lane1.2_1017, lane1.2_1017
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, QWORD PTR 168[rsp]	# m1011, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm1, ymm3	# tmp1767, tmp1766, tmp1815
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm2, xmm2, xmm9	# tmp1775, lane1.2_1017, lane3.0_1016
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, QWORD PTR 136[rsp]	# m23, %sfp
	xor	rbx, r15	# m1011, m4
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpermq	ymm2, ymm2, 0x50	# tmp1773, tmp1775
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, QWORD PTR 176[rsp]	# m23, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, rbx, 58	# tmp1785, m1011,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm2, ymm2, ymm8	# tmp1776, tmp1773, _921
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	add	r14, r12	# nm2, tmp1785
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, rbx, 52	# tmp1787, m1011,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm8, ymm2, ymm5	# tmp1777, tmp1776, tmp1817
	mov	r12, QWORD PTR 160[rsp]	# m3, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 128[rsp], r14	# %sfp, nm2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm1, ymm1, ymm4	# tmp1769, tmp1766, tmp1816
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r14, QWORD PTR 152[rsp], 1	# tmp1786, %sfp,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm2, ymm2, ymm6	# tmp1779, tmp1776, tmp1818
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	add	rbx, r14	# nm3, tmp1786
	mov	QWORD PTR 160[rsp], rbx	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, r11, 45	# tmp1789, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	add	rbx, r13	# m10, tmp1788
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm1, ymm1, ymm10	# tmp1781, tmp1769, tmp1767
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 120[rsp], rbx	# %sfp, m10
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm2, ymm2, ymm8	# tmp1782, tmp1779, tmp1777
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, r12, 21	# tmp1790, m3,
	rorx	r11, r11, 36	# tmp1791, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	lea	rbx, [r12+r11]	# m11,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm1, ymm1, ymm2	# v23, tmp1781, tmp1782
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 152[rsp], rbx	# %sfp, m11
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:314:   for (; pi_ptr != pi_end;) {
	cmp	rdi, QWORD PTR 88[rsp]	# pi_ptr, %sfp
	jne	.L43	#,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:908:   return (__m256i)__builtin_ia32_punpcklqdq256 ((__v4di)__A, (__v4di)__B);
	vpunpcklqdq	ymm2, ymm0, ymm1	# tmp1794, v01, v23
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:880:   return (__m256i)__builtin_ia32_punpckhqdq256 ((__v4di)__A, (__v4di)__B);
	vpunpckhqdq	ymm0, ymm0, ymm1	# tmp1797, v01, v23
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:935:   *__P = __A;
	mov	rax, QWORD PTR 32[rsp]	# digest, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1073:   return (__m256i) __builtin_ia32_permdi256 ((__v4di)__X, __M);
	vpermq	ymm2, ymm2, 216	# tmp1795, tmp1794,
	vpermq	ymm0, ymm0, 216	# tmp1798, tmp1797,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:590:   return (__m256i) __builtin_ia32_pshufb256 ((__v32qi)__X,
	vpshufb	ymm1, ymm2, ymm7	# tmp1800, tmp1795, tmp1814
	vpshufb	ymm0, ymm0, ymm7	# tmp1802, tmp1798, tmp1814
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:935:   *__P = __A;
	vmovdqu	YMMWORD PTR [rax], ymm1	# MEM[(__m256i_u * {ref-all})digest_17(D)], tmp1800
	vmovdqu	YMMWORD PTR 32[rax], ymm0	# MEM[(__m256i_u * {ref-all})digest_17(D) + 32B], tmp1802
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:437:   return 0;
	xor	eax, eax	# <retval>
	vzeroupper

.L1:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:438: }
	mov	rdx, QWORD PTR 600[rsp]	# tmp1826, D.33754
	sub	rdx, QWORD PTR fs:40	# tmp1826, MEM[(<address-space-1> long unsigned int *)40B]
	jne	.L198	#,
	lea	rsp, -40[rbp]	#,
	pop	rbx	#
	pop	r12	#
	pop	r13	#
	pop	r14	#
	pop	r15	#
	pop	rbp	#
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret	

.L197:
	.cfi_restore_state
	lea	rbx, 288[rsp]	# tmp1812,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:182:   unsigned int word = 0;
	mov	r12, QWORD PTR 56[rsp]	# msg, %sfp
	xor	eax, eax	# tmp.63
	mov	r10, rbx	# ivtmp.102, tmp1812
	.p2align 4,,10
	.p2align 3
.L9:
	lea	ecx, 0[0+rax*8]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:185:   for (; word + 3U < full_words; word += 4U) {
	add	r10, 32	# ivtmp.102,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:929:   return *__P;
	vmovdqu	ymm0, YMMWORD PTR [r12+rcx]	# MEM[(const __m256i_u * {ref-all})_246], MEM[(const __m256i_u * {ref-all})_246]
	mov	r11, rcx	#,
	mov	ecx, eax	# word, tmp.63
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:185:   for (; word + 3U < full_words; word += 4U) {
	add	eax, 4	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:185:   for (; word + 3U < full_words; word += 4U) {
	add	ecx, 7	# tmp1314,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:590:   return (__m256i) __builtin_ia32_pshufb256 ((__v32qi)__X,
	vpshufb	ymm0, ymm0, ymm7	# tmp1312, MEM[(const __m256i_u * {ref-all})_246], tmp1814
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR -32[r10], ymm0	# MEM[(__m256i * {ref-all})_4666], tmp1312
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:185:   for (; word + 3U < full_words; word += 4U) {
	cmp	ecx, edx	# tmp1314, zero_from
	jb	.L9	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:189:   for (; word < full_words; ++word) {
	cmp	eax, esi	# tmp.63, _5380
	jnb	.L15	#,
	mov	r10d, esi	# niters.60, _5380
	sub	r10d, eax	# niters.60, tmp.63
	cmp	r10d, 1	# niters.60,
	je	.L13	#,
	lea	ecx, 32[r11]	#,
	add	rcx, QWORD PTR 56[rsp]	# vectp.65, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	vmovdqa	xmm1, XMMWORD PTR .LC7[rip]	# tmp1322,
	mov	r11d, eax	# tmp.63, tmp.63
	vmovdqu	xmm2, XMMWORD PTR [rcx]	# tmp1976, MEM <vector(2) long unsigned int> [(const unsigned char *)vectp.65_5129]
	lea	r11, [rbx+r11*8]	# vectp.68,
	mov	ebx, r10d	# bnd.61, niters.60
	shr	ebx	# bnd.61
	vpshufb	xmm0, xmm2, xmm1	# tmp1321, tmp1976, tmp1322
	vmovdqa	XMMWORD PTR [r11], xmm0	# MEM <vector(2) long unsigned int> [(uint64_t *)vectp.68_5120], tmp1321
	cmp	ebx, 1	# bnd.61,
	jne	.L199	#,
.L14:
	mov	ecx, r10d	# niters_vector_mult_vf.62, niters.60
	and	ecx, -2	# niters_vector_mult_vf.62,
	add	eax, ecx	#, niters_vector_mult_vf.62
	cmp	ecx, r10d	# niters_vector_mult_vf.62, niters.60
	je	.L15	#,
.L13:
	mov	rbx, QWORD PTR 56[rsp]	# msg, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	lea	ecx, 0[0+rax*8]	#,
	mov	rcx, QWORD PTR [rbx+rcx]	# MEM <long unsigned int> [(const unsigned char *)_5146], MEM <long unsigned int> [(const unsigned char *)_5146]
	bswap	rcx		# _5141
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	mov	QWORD PTR 288[rsp+rax*8], rcx	# *_5143, _5141
.L15:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:193:   if (partial_bytes != 0U || rem_tail_bits != 0U) {
	test	r9d, r9d	# _5379
	je	.L200	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:194:     const unsigned long long byte_offset = full_words << 3;
	lea	rdx, 0[0+rsi*8]	# byte_offset,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	test	edi, edi	# _5382
	je	.L50	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	mov	rbx, QWORD PTR 56[rsp]	# msg, %sfp
	movzx	eax, BYTE PTR [rbx+rdx]	# *_5372, *_5372
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rax, 56	# w,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 1	# _5382,
	je	.L17	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 1[rbx+rdx]	# *_1085, *_1085
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 48	# tmp1334,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rax, rcx	# w, tmp1334
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 2	# _5382,
	je	.L17	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 2[rbx+rdx]	# *_795, *_795
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 40	# tmp1336,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rax, rcx	# w, tmp1336
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 3	# _5382,
	je	.L17	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 3[rbx+rdx]	# *_4997, *_4997
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 32	# tmp1338,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rax, rcx	# w, tmp1338
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 4	# _5382,
	je	.L17	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 4[rbx+rdx]	# *_4985, *_4985
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 24	# tmp1340,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rax, rcx	# w, tmp1340
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 5	# _5382,
	je	.L17	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 5[rbx+rdx]	# *_4973, *_4973
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 16	# tmp1342,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rax, rcx	# w, tmp1342
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:198:     for (byte = 0; byte < partial_bytes; ++byte) {
	cmp	edi, 7	# _5382,
	jne	.L17	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	movzx	ecx, BYTE PTR 6[rbx+rdx]	# *_4955, *_4955
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	sal	rcx, 8	# tmp1344,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:199:       w |= (uint64_t)tail[byte_offset + byte] << (56U - 8U * byte);
	or	rax, rcx	# w, tmp1344
.L17:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:201:     if (rem_tail_bits != 0U) {
	test	r8d, r8d	# _5383
	jne	.L201	#,
.L19:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:209:     zero_from = full_words + 1U;
	lea	edx, 1[rsi]	# zero_from,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:208:     out[full_words] = w;
	mov	QWORD PTR 288[rsp+rsi*8], rax	# *_292, w
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	dl, 3	# zero_from,
	je	.L20	#,
	cmp	edx, 16	# zero_from,
	je	.L24	#,
.L21:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	eax, edx	# zero_from, zero_from
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 288[rsp+rax*8], 0	# *_5348,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 1[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L23	#,
	cmp	eax, 16	# word,
	je	.L24	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 288[rsp+rax*8], 0	# *_5356,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 2[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	al, 3	# word,
	je	.L23	#,
	cmp	eax, 16	# word,
	je	.L24	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:164:     out[word] = 0;
	mov	QWORD PTR 288[rsp+rax*8], 0	# *_5364,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	lea	eax, 3[rdx]	# word,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	cmp	edx, 12	# zero_from,
	jne	.L23	#,
.L195:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:171:     out[word] = 0;
	mov	QWORD PTR 408[rsp], 0	# MEM[(uint64_t *)&m + 120B],

.L24:
# Cuishen-512 final: state[0]=[t0,t0,t1,t1], state[1]=[t2,t2,t3,t3^1].
	vmovddup	xmm0, QWORD PTR 224[rsp]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:308:   const CUISHEN_AVX2_PI_QUAL uint64_t *pi_ptr = PI;
	lea	rdi, PI[rip]	# pi_ptr,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:286:   uint64_t m0 = message_key[0];
	mov	r15, QWORD PTR 288[rsp]	# nm2, MEM[(const uint64_t *)&m]
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vmovddup	xmm1, QWORD PTR 232[rsp]	# tmp1364, MEM[(uint64_t *)&t + 8B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:298:   uint64_t m12 = message_key[12];
	mov	rbx, QWORD PTR 384[rsp]	# nm2, MEM[(const uint64_t *)&m + 96B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:292:   uint64_t m6 = message_key[6];
	mov	r8, QWORD PTR 336[rsp]	# nm2, MEM[(const uint64_t *)&m + 48B]
	vinserti128	ymm0, ymm0, xmm1, 0x1
	vmovddup	xmm2, QWORD PTR 240[rsp]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:299:   uint64_t m13 = message_key[13];
	mov	r14, QWORD PTR 392[rsp]	# nm3, MEM[(const uint64_t *)&m + 104B]
	mov	rax, QWORD PTR 248[rsp]
	vmovq	xmm1, rax
	xor	rax, 1
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:302:   uint64_t bk0 = block_key[0];
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpinsrq	xmm1, xmm1, rax, 1
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:287:   uint64_t m1 = message_key[1];
	mov	rax, QWORD PTR 296[rsp]	# nm3, MEM[(const uint64_t *)&m + 8B]
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm1, ymm2, xmm1, 0x1
	mov	rsi, QWORD PTR 256[rsp]	# bk0, MEM[(const uint64_t *)&b]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:287:   uint64_t m1 = message_key[1];
	mov	QWORD PTR 192[rsp], rax	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:288:   uint64_t m2 = message_key[2];
	mov	rax, QWORD PTR 304[rsp]	# nm2, MEM[(const uint64_t *)&m + 16B]
	mov	QWORD PTR 120[rsp], rax	# %sfp, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:289:   uint64_t m3 = message_key[3];
	mov	rax, QWORD PTR 312[rsp]	# nm3, MEM[(const uint64_t *)&m + 24B]
	mov	QWORD PTR 152[rsp], rax	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:290:   uint64_t m4 = message_key[4];
	mov	rax, QWORD PTR 320[rsp]	# m4, MEM[(const uint64_t *)&m + 32B]
	mov	QWORD PTR 168[rsp], rax	# %sfp, m4
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:291:   uint64_t m5 = message_key[5];
	mov	rax, QWORD PTR 328[rsp]	# m5, MEM[(const uint64_t *)&m + 40B]
	mov	QWORD PTR 160[rsp], rax	# %sfp, m5
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:293:   uint64_t m7 = message_key[7];
	mov	rax, QWORD PTR 344[rsp]	# nm3, MEM[(const uint64_t *)&m + 56B]
	mov	QWORD PTR 128[rsp], rax	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:294:   uint64_t m8 = message_key[8];
	mov	rax, QWORD PTR 352[rsp]	# m8, MEM[(const uint64_t *)&m + 64B]
	mov	QWORD PTR 104[rsp], rax	# %sfp, m8
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:295:   uint64_t m9 = message_key[9];
	mov	rax, QWORD PTR 360[rsp]	# m9, MEM[(const uint64_t *)&m + 72B]
	mov	QWORD PTR 184[rsp], rax	# %sfp, m9
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:296:   uint64_t m10 = message_key[10];
	mov	rax, QWORD PTR 368[rsp]	# m10, MEM[(const uint64_t *)&m + 80B]
	mov	QWORD PTR 144[rsp], rax	# %sfp, m10
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:297:   uint64_t m11 = message_key[11];
	mov	rax, QWORD PTR 376[rsp]	# m11, MEM[(const uint64_t *)&m + 88B]
	mov	QWORD PTR 136[rsp], rax	# %sfp, m11
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:300:   uint64_t m14 = message_key[14];
	mov	rax, QWORD PTR 400[rsp]	# m14, MEM[(const uint64_t *)&m + 112B]
	mov	QWORD PTR 96[rsp], rax	# %sfp, m14
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:301:   uint64_t m15 = message_key[15];
	mov	rax, QWORD PTR 408[rsp]	# m15, MEM[(const uint64_t *)&m + 120B]
	mov	QWORD PTR 176[rsp], rax	# %sfp, m15
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:303:   uint64_t bk1 = block_key[1];
	mov	rcx, QWORD PTR 264[rsp]	# bk1, MEM[(const uint64_t *)&b + 8B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:304:   uint64_t bk2 = block_key[2];
	mov	rdx, QWORD PTR 272[rsp]	# bk2, MEM[(const uint64_t *)&b + 16B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:305:   uint64_t bk3 = block_key[3];
	mov	r9, QWORD PTR 48[rsp]	# c1, %sfp
	mov	QWORD PTR 112[rsp], r15	# %sfp, nm2
	mov	r15, rbx	# nm2, nm2
	mov	rax, QWORD PTR 280[rsp]	# bk3, MEM[(const uint64_t *)&b + 24B]
	.p2align 4,,10
	.p2align 3

.L22:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, QWORD PTR 104[rsp]	# m8, %sfp
	mov	r12, QWORD PTR 112[rsp]	# nm2, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm2, ymm1, ymm1, 1	# tmp1375, vect_lane1.78, vect_lane1.78,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm2, ymm2, ymm1	# tmp1378, tmp1375, vect_lane1.78
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r10, rbx	# lane3.0_386, m8
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, rbx, 35	# tmp1400, m8,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, rdx	# lane3.0_386, bk2
	vmovq	xmm9, r10	# lane3.0_386, lane3.0_386
	mov	r10, QWORD PTR 184[rsp]	# lane1.2_388, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1380, lane3.0_386
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, rax	# lane1.2_388, bk3
	vmovq	xmm8, r10	# lane1.2_388, lane1.2_388
	mov	r10, QWORD PTR 192[rsp]	# tmp1387, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm8, xmm8, xmm8	# tmp1381, lane1.2_388
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, rcx	# tmp1387, bk1
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm8, ymm8, xmm9, 0x1	# tmp1379, tmp1381, tmp1380
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, r10	# tmp1387, tmp1387
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm2, ymm2, ymm8	# tmp1382, tmp1378, tmp1379
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r10, QWORD PTR 120[rsp]	# m23, %sfp
	xor	r10, QWORD PTR 152[rsp]	# m23, %sfp
	xor	r11, r9	# tmp1387, c1
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm2, ymm3	# tmp1383, tmp1382, tmp1815
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r11	# lane3.0_383, tmp1387
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm2, ymm2, ymm4	# tmp1385, tmp1382, tmp1816
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, r12	# tmp1388, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, r12, 50	# tmp1404, m0,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, rsi	# tmp1388, bk0
	xor	r11, QWORD PTR [rdi]	# tmp1388, MEM[(const uint64_t *)pi_ptr_1230]
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1390, lane3.0_383
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rsi, rsi, 37	# tmp1407, bk0,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm8, r11	# lane1.2_384, tmp1388
	mov	r11, QWORD PTR 144[rsp]	# m1011, %sfp
	xor	rsi, rcx	# bk0, bk1
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm2, ymm2, ymm10	# tmp1397, tmp1385, tmp1383
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, QWORD PTR 136[rsp]	# m1011, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm8, xmm8, xmm8	# tmp1391, lane1.2_384
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, r11, 58	# tmp1399, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	add	r13, rbx	# nm2, tmp1400
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r11, r11, 52	# tmp1401, m1011,
	mov	rbx, QWORD PTR 192[rsp]	# m1, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 112[rsp], r13	# %sfp, nm2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm8, ymm8, xmm9, 0x1	# tmp1389, tmp1391, tmp1390
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, QWORD PTR 184[rsp], 1	# tmp1402, %sfp,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	add	r11, r13	# nm3, tmp1402
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r13, QWORD PTR 96[rsp]	# m14, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 192[rsp], r11	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, rbx, 21	# tmp1406, m1,
	rorx	r11, r10, 45	# tmp1403, m23,
	rorx	r10, r10, 36	# tmp1405, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	add	rbx, r10	# m9, tmp1405
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm0, ymm0, ymm8	# tmp1392, v01, tmp1389
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	add	r12, r11	# m8, tmp1403
	mov	QWORD PTR 184[rsp], rbx	# %sfp, m9
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, r13	# lane3.0_371, m14
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm8, ymm0, ymm5	# tmp1393, tmp1392, tmp1817
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, QWORD PTR 128[rsp]	# lane3.0_368, %sfp
	xor	rbx, rax	# lane3.0_371, bk3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm0, ymm0, ymm6	# tmp1395, tmp1392, tmp1818
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r10, rcx	# tmp1420, bk1
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:315:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 104[rsp], r12	# %sfp, m8
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, rbx	# lane3.0_371, lane3.0_371
	xor	r10, r9	# tmp1420, c1
	mov	rbx, QWORD PTR 176[rsp]	# lane1.2_372, %sfp
	xor	r11, rdx	# lane3.0_368, bk2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm0, ymm0, ymm8	# tmp1398, tmp1395, tmp1393
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1413, lane3.0_371
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, r8	# rk0, nm2
	xor	r10, QWORD PTR 8[rdi]	# lane1.2_369, MEM[(const uint64_t *)pi_ptr_1230 + 8B]
	xor	rbx, rsi	# lane1.2_372, bk0
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm2, ymm2, ymm0	# _105, tmp1397, tmp1398
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r12, QWORD PTR 184[rsp]	# m1011, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rcx, rcx, 37	# tmp1440, bk1,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm0, rbx	# lane1.2_372, lane1.2_372
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm8, ymm2, ymm2, 1	# tmp1408, _105, _105,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, r13, 35	# tmp1433, m14,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rcx, rdx	# bk1, bk2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm0, xmm0, xmm0	# tmp1414, lane1.2_372
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm8, ymm8, ymm2	# tmp1411, tmp1408, _105
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, QWORD PTR 176[rsp], 1	# tmp1435, %sfp,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm0, ymm0, xmm9, 0x1	# tmp1412, tmp1414, tmp1413
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r11	# lane3.0_368, lane3.0_368
	mov	r11, r12	# m1011, m1011
	mov	r12, r8	# m6, nm2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm8, ymm8, ymm0	# tmp1415, tmp1411, tmp1412
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1423, lane3.0_368
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r8, r11, 58	# tmp1432, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	add	r8, rbx	# nm2, tmp1433
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vmovq	xmm0, r10	# lane1.2_369, lane1.2_369
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r11, r11, 52	# tmp1434, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r10, QWORD PTR 112[rsp]	# m23, %sfp
	add	r13, r11	# nm3, tmp1434
	xor	r10, QWORD PTR 192[rsp]	# m23, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, r12, 50	# tmp1437, m6,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm8, ymm3	# tmp1416, tmp1415, tmp1815
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm0, xmm0, xmm0	# tmp1424, lane1.2_369
	mov	rbx, QWORD PTR 128[rsp]	# m7, %sfp
	vinserti128	ymm0, ymm0, xmm9, 0x1	# tmp1422, tmp1424, tmp1423
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r11, r10, 45	# tmp1436, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	add	r12, r11	# m14, tmp1436
	mov	QWORD PTR 96[rsp], r12	# %sfp, m14
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm1, ymm0, ymm1	# tmp1425, tmp1422, vect_lane1.78
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, r12	# m14, m14
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r10, r10, 36	# tmp1438, m23,
	rorx	rbx, rbx, 21	# tmp1439, m7,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	lea	r12, [r10+rbx]	# m15,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r10, QWORD PTR 168[rsp]	# lane3.0_355, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm0, ymm1, ymm5	# tmp1426, tmp1425, tmp1817
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, QWORD PTR 160[rsp]	# lane1.2_356, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm8, ymm8, ymm4	# tmp1418, tmp1415, tmp1816
	vpsllvq	ymm1, ymm1, ymm6	# tmp1428, tmp1425, tmp1818
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 176[rsp], r12	# %sfp, m15
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, rsi	# lane3.0_355, bk0
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, r9, 62	# _358, c1,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:316:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 128[rsp], r13	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r9, r9, 60	# c1, c1,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm1, ymm1, ymm0	# tmp1431, tmp1428, tmp1426
	vpor	ymm8, ymm8, ymm10	# tmp1430, tmp1418, tmp1416
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r10	# lane3.0_355, lane3.0_355
	xor	rbx, rcx	# lane1.2_356, bk1
	mov	r10, rax	# tmp1453, bk3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm8, ymm8, ymm1	# _148, tmp1430, tmp1431
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm1, rbx	# lane1.2_356, lane1.2_356
	mov	rbx, r15	# m12, nm2
	xor	r10, r14	# tmp1453, nm3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1446, lane3.0_355
	vpunpcklqdq	xmm1, xmm1, xmm1	# tmp1447, lane1.2_356
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, rbx, 50	# tmp1470, m12,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, r12	# tmp1453, _358
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm1, ymm1, xmm9, 0x1	# tmp1445, tmp1447, tmp1446
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm0, ymm8, ymm8, 1	# tmp1441, _148, _148,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r10	# lane3.0_352, tmp1453
	mov	r10, rdx	# tmp1454, bk2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm0, ymm0, ymm8	# tmp1444, tmp1441, _148
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rdx, rdx, 37	# tmp1473, bk2,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, r15	# tmp1454, nm2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm0, ymm0, ymm1	# tmp1448, tmp1444, tmp1445
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1456, lane3.0_352
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, QWORD PTR 16[rdi]	# tmp1454, MEM[(const uint64_t *)pi_ptr_1230 + 16B]
	vmovq	xmm1, r10	# lane1.2_353, tmp1454
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm0, ymm3	# tmp1449, tmp1448, tmp1815
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r10, r13	# m23, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, QWORD PTR 168[rsp], 35	# tmp1466, %sfp,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, r8	# m23, nm2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm1, xmm1, xmm1	# tmp1457, lane1.2_353
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, QWORD PTR 176[rsp]	# m1011, %sfp
	xor	rdx, rax	# bk2, bk3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm1, ymm1, xmm9, 0x1	# tmp1455, tmp1457, tmp1456
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r15, r11, 58	# tmp1465, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	add	r15, r13	# nm2, tmp1466
	mov	r13, r14	# m13, nm3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm1, ymm1, ymm2	# tmp1458, tmp1455, _105
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r11, r11, 52	# tmp1467, m1011,
	rorx	r14, QWORD PTR 160[rsp], 1	# tmp1468, %sfp,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	add	r14, r11	# nm3, tmp1467
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, r13, 21	# tmp1472, m13,
	rorx	r11, r10, 45	# tmp1469, m23,
	rorx	r10, r10, 36	# tmp1471, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	add	rbx, r11	# m4, tmp1469
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm2, ymm1, ymm5	# tmp1459, tmp1458, tmp1817
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm1, ymm1, ymm6	# tmp1461, tmp1458, tmp1818
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	add	r10, r13	# m5, tmp1472
	add	rdi, 32	# pi_ptr,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r13, QWORD PTR 144[rsp]	# lane3.0_340, %sfp
	mov	r11, QWORD PTR 136[rsp]	# lane1.2_341, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm0, ymm0, ymm4	# tmp1451, tmp1448, tmp1816
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 168[rsp], rbx	# %sfp, m4
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	rbx, QWORD PTR 152[rsp]	# lane3.0_337, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm1, ymm1, ymm2	# tmp1464, tmp1461, tmp1459
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:317:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 160[rsp], r10	# %sfp, m5
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r13, rcx	# lane3.0_340, bk1
	xor	r11, rdx	# lane1.2_341, bk2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm0, ymm0, ymm10	# tmp1463, tmp1451, tmp1449
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, r13	# lane3.0_340, lane3.0_340
	vmovq	xmm2, r11	# lane1.2_341, lane1.2_341
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm0, ymm0, ymm1	# v01, tmp1463, tmp1464
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	rbx, rsi	# lane3.0_337, bk0
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1479, lane3.0_340
	vpunpcklqdq	xmm2, xmm2, xmm2	# tmp1480, lane1.2_341
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1086:   return (__m256i) __builtin_ia32_permti256 ((__v4di)__X, (__v4di)__Y, __M);
	vperm2i128	ymm1, ymm0, ymm0, 1	# tmp1474, v01, v01,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r11, QWORD PTR 168[rsp]	# m1011, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm2, ymm2, xmm9, 0x1	# tmp1478, tmp1480, tmp1479
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	vmovq	xmm9, rbx	# lane3.0_337, lane3.0_337
	mov	rbx, QWORD PTR 120[rsp]	# nm2, %sfp
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm1, ymm1, ymm0	# tmp1477, tmp1474, v01
	vpxor	ymm1, ymm1, ymm2	# tmp1481, tmp1477, tmp1478
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm9, xmm9, xmm9	# tmp1489, lane3.0_337
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r11, QWORD PTR 160[rsp]	# m1011, %sfp
	mov	r10, rbx	# tmp1486, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, r11, 58	# tmp1499, m1011,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm10, ymm1, ymm3	# tmp1482, tmp1481, tmp1815
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r11, r11, 52	# tmp1501, m1011,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, rax	# tmp1486, bk3
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm1, ymm1, ymm4	# tmp1484, tmp1481, tmp1816
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	rbx, rbx, 50	# tmp1504, m2,
	rorx	rax, rax, 37	# tmp1507, bk3,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, r12	# rk0, _358
	xor	r10, QWORD PTR -8[rdi]	# lane1.2_338, MEM[(const uint64_t *)_4661 + -8B]
	xor	rax, rsi	# bk3, bk0
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r12, QWORD PTR 144[rsp], 35	# tmp1500, %sfp,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vmovq	xmm2, r10	# lane1.2_338, lane1.2_338
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	add	r13, r12	# nm2, tmp1500
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm1, ymm1, ymm10	# tmp1496, tmp1484, tmp1482
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	r10, r15	# m23, nm2
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vpunpcklqdq	xmm2, xmm2, xmm2	# tmp1490, lane1.2_338
	mov	r12, QWORD PTR 152[rsp]	# m3, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	xor	r10, r14	# m23, nm3
	mov	QWORD PTR 120[rsp], r13	# %sfp, nm2
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r13, QWORD PTR 136[rsp], 1	# tmp1502, %sfp,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:1306:   return __extension__ (__m256i)(__v4di){ __D, __C, __B, __A };
	vinserti128	ymm2, ymm2, xmm9, 0x1	# tmp1488, tmp1490, tmp1489
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	add	r11, r13	# nm3, tmp1502
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:915:   return (__m256i) ((__v4du)__A ^ (__v4du)__B);
	vpxor	ymm2, ymm2, ymm8	# tmp1491, tmp1488, _148
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 152[rsp], r11	# %sfp, nm3
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r11, r10, 45	# tmp1503, m23,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	add	rbx, r11	# m10, tmp1503
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1239:   return (__m256i) __builtin_ia32_psrlv4di ((__v4di)__X, (__v4di)__Y);
	vpsrlvq	ymm8, ymm2, ymm5	# tmp1492, tmp1491, tmp1817
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1197:   return (__m256i) __builtin_ia32_psllv4di ((__v4di)__X, (__v4di)__Y);
	vpsllvq	ymm2, ymm2, ymm6	# tmp1494, tmp1491, tmp1818
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:116:   return (x << n) | (x >> (64U - n));
	rorx	r10, r10, 36	# tmp1505, m23,
	rorx	r12, r12, 21	# tmp1506, m3,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 144[rsp], rbx	# %sfp, m10
	lea	rbx, [r10+r12]	# m11,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:576:   return (__m256i) ((__v4du)__A | (__v4du)__B);
	vpor	ymm2, ymm2, ymm8	# tmp1497, tmp1494, tmp1492
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:318:     CUISHEN_PAIR_ROUND_AVX2();
	mov	QWORD PTR 136[rsp], rbx	# %sfp, m11
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:128:   return (__m256i) ((__v4du)__A + (__v4du)__B);
	vpaddq	ymm1, ymm1, ymm2	# vect_lane1.78, tmp1496, tmp1497
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:314:   for (; pi_ptr != pi_end;) {
	cmp	rdi, QWORD PTR 88[rsp]	# pi_ptr, %sfp
	jne	.L22	#,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:908:   return (__m256i)__builtin_ia32_punpcklqdq256 ((__v4di)__A, (__v4di)__B);
	vpunpcklqdq	ymm2, ymm0, ymm1	# tmp1509, v01, vect_lane1.78
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:880:   return (__m256i)__builtin_ia32_punpckhqdq256 ((__v4di)__A, (__v4di)__B);
	vpunpckhqdq	ymm0, ymm0, ymm1	# tmp1512, v01, vect_lane1.78
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:179:   const unsigned int full_words = (unsigned int)(rem_bytes >> 3);
	xor	eax, eax	#
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avx2intrin.h:1073:   return (__m256i) __builtin_ia32_permdi256 ((__v4di)__X, __M);
	vpermq	ymm2, ymm2, 216	# tmp1510, tmp1509,
	vpermq	ymm0, ymm0, 216	# tmp1513, tmp1512,
	vpxor	ymm2, ymm2, YMMWORD PTR 224[rsp]
	vmovdqa	ymm15, YMMWORD PTR 224[rsp]
	vpxor	ymm15, ymm15, YMMWORD PTR .LC9[rip]
	vpxor	ymm0, ymm0, ymm15
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR 416[rsp], ymm2	# MEM[(__m256i * {ref-all})&final_key], tmp1510
	vmovdqa	YMMWORD PTR 448[rsp], ymm0	# MEM[(__m256i * {ref-all})&final_key + 32B], tmp1513

.L27:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	lea	edx, 3[rax]	# tmp1632,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	cmp	edx, 11	# tmp1632,
	ja	.L40	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:168:     CUISHEN_STORE_LOCAL_U64X4(out + word, zero);
	mov	edx, eax	# word, word
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vpxor	xmm0, xmm0, xmm0	# tmp1634
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	lea	ecx, 4[rax]	# word,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR 480[rsp+rdx*8], ymm0	# MEM[(__m256i * {ref-all})_256], tmp1634
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	lea	edx, 7[rax]	# tmp1635,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	cmp	edx, 11	# tmp1635,
	ja	.L41	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:168:     CUISHEN_STORE_LOCAL_U64X4(out + word, zero);
	mov	edx, ecx	# word, word
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR 480[rsp+rdx*8], ymm0	# MEM[(__m256i * {ref-all})_5155], tmp1634
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	lea	edx, 8[rax]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	test	eax, eax	# word
	jne	.L41	#,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR 480[rsp+rdx*8], ymm0	# MEM[(__m256i * {ref-all})_789], tmp1634
	mov	ecx, 4	# word,
.L41:
	mov	edx, 8	# tmp1641,
	sub	edx, eax	# tmp1640, word
	mov	eax, edx	# tmp1640, tmp1640
	and	eax, -4	# tmp1640,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	add	eax, ecx	#, word
.L40:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:170:   for (; word < out_words; ++word) {
	cmp	eax, 11	# word,
	ja	.L39	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:171:     out[word] = 0;
	mov	edx, 12	# tmp1643,
	sub	edx, eax	# tmp1644, word
	lea	rax, 480[rsp+rax*8]	# tmp1650,
	sal	rdx, 3	# tmp1645,
	mov	QWORD PTR [rax], 0	#,
	lea	rdi, 8[rax]	# tmp1657,
	mov	ecx, edx	# tmp1654, tmp1645
	sub	edx, 8	# tmp1651,
	mov	QWORD PTR -8[rax+rcx], 0	#,
	shr	edx, 3	# tmp1658,
	xor	eax, eax	# tmp1653
	mov	ecx, edx	# tmp1659, tmp1658
	rep stosq
	jmp	.L39	#

.L23:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:195:     uint64_t w = 0;
	mov	edx, eax	# zero_from, word
.L20:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:170:   for (; word < out_words; ++word) {
	cmp	edx, 16	# zero_from,
	je	.L24	#,
.L26:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:171:     out[word] = 0;
	mov	eax, edx	# zero_from, zero_from
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:171:     out[word] = 0;
	mov	QWORD PTR 288[rsp+rax*8], 0	# *_5295,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:170:   for (; word < out_words; ++word) {
	lea	eax, 1[rdx]	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:170:   for (; word < out_words; ++word) {
	cmp	edx, 15	# zero_from,
	je	.L24	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:171:     out[word] = 0;
	mov	QWORD PTR 288[rsp+rax*8], 0	# *_5302,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:170:   for (; word < out_words; ++word) {
	cmp	edx, 14	# zero_from,
	jne	.L195	#,
	jmp	.L24	#
.L200:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:163:   for (; (word & 3U) != 0U && word < out_words; ++word) {
	test	sil, 3	# _5380,
	jne	.L21	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	cmp	esi, 12	# _5380,
	jne	.L26	#,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vpxor	xmm0, xmm0, xmm0	# tmp1372
	vmovdqa	YMMWORD PTR 384[rsp], ymm0	# MEM[(__m256i * {ref-all})&m + 96B], tmp1372
	jmp	.L24	#
.L37:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	lea	edx, 3[rax]	# tmp1807,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	cmp	edx, 11	# tmp1807,
	ja	.L40	#,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:168:     CUISHEN_STORE_LOCAL_U64X4(out + word, zero);
	mov	edx, eax	# word, word
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vpxor	xmm0, xmm0, xmm0	# tmp1806
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:167:   for (; word + 3U < out_words; word += 4U) {
	lea	ecx, 4[rax]	# word,
# /usr/lib/gcc/x86_64-linux-gnu/11/include/avxintrin.h:923:   *__P = __A;
	vmovdqa	YMMWORD PTR 480[rsp+rdx*8], ymm0	# MEM[(__m256i * {ref-all})_81], tmp1806
	jmp	.L41	#
.L199:
	vmovdqu	xmm0, XMMWORD PTR 16[rcx]	# vect_load_dst_792.66, MEM <vector(2) long unsigned int> [(const unsigned char *)vectp.65_5129 + 16B]
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:190:     out[word] = load64_be(tail + 8 * word);
	vpshufb	xmm0, xmm0, xmm1	# tmp1325, vect_load_dst_792.66, tmp1322
	vmovdqa	XMMWORD PTR 16[r11], xmm0	# MEM <vector(2) long unsigned int> [(uint64_t *)vectp.68_5120 + 16B], tmp1325
	jmp	.L14	#
.L201:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	mov	r9d, 8	# tmp1349,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	add	rdx, QWORD PTR 56[rsp]	# tmp1346, %sfp
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	mov	r10d, edi	# _5382, _5382
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	mov	ecx, 255	# tmp1351,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	sub	r9d, r8d	# tmp1348, _5383
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:203:           (unsigned char)(0xFFU << (8U - rem_tail_bits));
	shlx	ecx, ecx, r9d	# tmp1350, tmp1351, tmp1348
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	and	cl, BYTE PTR [rdx+r10]	# tmp1352, *_282
	movzx	edx, cl	#, tmp1352
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:205:            << (56U - 8U * partial_bytes);
	mov	ecx, 7	# tmp1355,
	sub	ecx, edi	# tmp1354, _5382
	sal	ecx, 3	# tmp1356,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:205:            << (56U - 8U * partial_bytes);
	shlx	rdx, rdx, rcx	# tmp1357, tmp1353, tmp1356
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:204:       w |= (uint64_t)(tail[byte_offset + partial_bytes] & tail_mask)
	or	rax, rdx	# w, tmp1357
	jmp	.L19	#
.L49:
	lea	rax, PI[rip+512]	# tmp1811,
	vmovdqa	ymm7, YMMWORD PTR .LC2[rip]	# tmp1814,
	vmovdqa	ymm3, YMMWORD PTR .LC3[rip]	# tmp1815,
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:422:   for (block = 0ULL; block < full_blocks; ++block) {
	vmovdqa	ymm2, ymm0	# _681, _683
	vmovdqa	ymm0, YMMWORD PTR .LC10[rip]	# _683, IV_2
	mov	QWORD PTR 88[rsp], rax	# %sfp, tmp1811
	vmovdqa	ymm4, YMMWORD PTR .LC4[rip]	# tmp1816,
	vmovdqa	ymm5, YMMWORD PTR .LC5[rip]	# tmp1817,
	vmovdqa	ymm6, YMMWORD PTR .LC6[rip]	# tmp1818,
	jmp	.L4	#
.L52:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:195:     uint64_t w = 0;
	xor	edx, edx	# w
	jmp	.L34	#
.L48:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:402:     return -1;
	mov	eax, -1	# <retval>,
	jmp	.L1	#
.L50:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:195:     uint64_t w = 0;
	xor	eax, eax	# w
	jmp	.L17	#
.L198:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:438: }
	call	__stack_chk_fail@PLT	#
.L51:
# Cuishen-512/CryptHash_Cuishen-512-c-avx2.c:182:   unsigned int word = 0;
	xor	eax, eax	# word
	jmp	.L28	#
	.cfi_endproc
.LFE5496:
	.size	CryptHash, .-CryptHash

	.section	.rodata
	.align 32
	.type	PI, @object
	.size	PI, 512
PI:
	.quad	2611923443488327891
	.quad	1376283091369227076
	.quad	-6626703657320631856
	.quad	589684135938649225
	.quad	4983270260364809079
	.quad	-4732044268327596948
	.quad	-4563226453097033507
	.quad	4577018097722394903
	.quad	-7919907764393346277
	.quad	-3372901835766516308
	.quad	3458046377305235383
	.quad	-5124621466747896170
	.quad	-5008970055469465703
	.quad	2639559389850201335
	.quad	577009281997405206
	.quad	7163292796296056425
	.quad	-6604248873402417794
	.quad	978816653474051672
	.quad	8181858928071887598
	.quad	8886908412430539189
	.quad	-7192014163400302573
	.quad	-4192376113057462800
	.quad	-3872681057474365201
	.quad	-8180264598055020530
	.quad	7826709195534535230
	.quad	-2948318707057603801
	.quad	8696222020257471584
	.quad	-1849530343500698732
	.quad	6289444427973268544
	.quad	6181816566664597686
	.quad	-5418854872935962418
	.quad	-6821679447294154349
	.quad	-5481421631548048342
	.quad	3146262819865375222
	.quad	-3576915737219460322
	.quad	-5776224742035632292
	.quad	8805192035659515511
	.quad	4291728789379725743
	.quad	-4269438717348273773
	.quad	7050395992639777169
	.quad	5223239197735551026
	.quad	-1187721743787788879
	.quad	-2583338840804222072
	.quad	2560646590458408133
	.quad	1111667774711939641
	.quad	3317820878385061892
	.quad	7622606572595682142
	.quad	2433747285459954842
	.quad	7425481828935633136
	.quad	7661081268566896488
	.quad	-7633699050254945373
	.quad	7993620422417071076
	.quad	-5027160330510587240
	.quad	-6777524788019658378
	.quad	7406830662172479112
	.quad	-8291542420845256780
	.quad	9044536209602993854
	.quad	-2274469713981398925
	.quad	4619079818967870118
	.quad	5680070893837973254
	.quad	2017295364945281597
	.quad	4021951020350968392
	.quad	-2661650661553094501
	.quad	527891791103990651
	.section	.rodata.cst32,"aM",@progbits,32
	.align 32
.LC0:
	.quad	920042818334365536
	.quad	6347163684422533404
	.quad	8122221483861433410
	.quad	-6822605537073768748
	.align 32
.LC10:
	.quad	4970063267203923202
	.quad	8218451384177679172
	.quad	-5447325171898657010
	.quad	-3877023728269904647
	.align 32
.LC1:
	.quad	0
	.quad	0
	.quad	2
	.quad	3
	.align 32
.LC2:
	.byte	7
	.byte	6
	.byte	5
	.byte	4
	.byte	3
	.byte	2
	.byte	1
	.byte	0
	.byte	15
	.byte	14
	.byte	13
	.byte	12
	.byte	11
	.byte	10
	.byte	9
	.byte	8
	.byte	7
	.byte	6
	.byte	5
	.byte	4
	.byte	3
	.byte	2
	.byte	1
	.byte	0
	.byte	15
	.byte	14
	.byte	13
	.byte	12
	.byte	11
	.byte	10
	.byte	9
	.byte	8
	.align 32
.LC3:
	.quad	44
	.quad	44
	.quad	8
	.quad	8
	.align 32
.LC4:
	.quad	20
	.quad	20
	.quad	56
	.quad	56
	.align 32
.LC5:
	.quad	57
	.quad	57
	.quad	33
	.quad	33
	.align 32
.LC6:
	.quad	7
	.quad	7
	.quad	31
	.quad	31
	.align 32
.LC9:
	.quad	0
	.quad	0
	.quad	0
	.quad	1
	.set	.LC7,.LC2

	.section .rodata
	.p2align 5
	.globl cuishen_pi_avx2
	.hidden cuishen_pi_avx2
	.type cuishen_pi_avx2,@object
cuishen_pi_avx2:
	.quad 0x243f6a8885a308d3, 0
	.quad 0x13198a2e03707344, 0
	.quad 0xa4093822299f31d0, 0
	.quad 0x082efa98ec4e6c89, 0
	.quad 0x452821e638d01377, 0
	.quad 0xbe5466cf34e90c6c, 0
	.quad 0xc0ac29b7c97c50dd, 0
	.quad 0x3f84d5b5b5470917, 0
	.quad 0x9216d5d98979fb1b, 0
	.quad 0xd1310ba698dfb5ac, 0
	.quad 0x2ffd72dbd01adfb7, 0
	.quad 0xb8e1afed6a267e96, 0
	.quad 0xba7c9045f12c7f99, 0
	.quad 0x24a19947b3916cf7, 0
	.quad 0x0801f2e2858efc16, 0
	.quad 0x636920d871574e69, 0
	.quad 0xa458fea3f4933d7e, 0
	.quad 0x0d95748f728eb658, 0
	.quad 0x718bcd5882154aee, 0
	.quad 0x7b54a41dc25a59b5, 0
	.quad 0x9c30d5392af26013, 0
	.quad 0xc5d1b023286085f0, 0
	.quad 0xca417918b8db38ef, 0
	.quad 0x8e79dcb0603a180e, 0
	.quad 0x6c9e0e8bb01e8a3e, 0
	.quad 0xd71577c1bd314b27, 0
	.quad 0x78af2fda55605c60, 0
	.quad 0xe65525f3aa55ab94, 0
	.quad 0x5748986263e81440, 0
	.quad 0x55ca396a2aab10b6, 0
	.quad 0xb4cc5c341141e8ce, 0
	.quad 0xa15486af7c72e993, 0
	.quad 0xb3ee1411636fbc2a, 0
	.quad 0x2ba9c55d741831f6, 0
	.quad 0xce5c3e169b87931e, 0
	.quad 0xafd6ba336c24cf5c, 0
	.quad 0x7a32538128958677, 0
	.quad 0x3b8f48986b4bb9af, 0
	.quad 0xc4bfe81b66282193, 0
	.quad 0x61d809ccfb21a991, 0
	.quad 0x487cac605dec8032, 0
	.quad 0xef845d5de98575b1, 0
	.quad 0xdc262302eb651b88, 0
	.quad 0x23893e81d396acc5, 0
	.quad 0x0f6d6ff383f44239, 0
	.quad 0x2e0b4482a4842004, 0
	.quad 0x69c8f04a9e1f9b5e, 0
	.quad 0x21c66842f6e96c9a, 0
	.quad 0x670c9c61abd388f0, 0
	.quad 0x6a51a0d2d8542f68, 0
	.quad 0x960fa728ab5133a3, 0
	.quad 0x6eef0b6c137a3be4, 0
	.quad 0xba3bf0507efb2a98, 0
	.quad 0xa1f1651d39af0176, 0
	.quad 0x66ca593e82430e88, 0
	.quad 0x8cee8619456f9fb4, 0
	.quad 0x7d84a5c33b8b5ebe, 0
	.quad 0xe06f75d885c12073, 0
	.quad 0x401a449f56c16aa6, 0
	.quad 0x4ed3aa62363f7706, 0
	.quad 0x1bfedf72429b023d, 0
	.quad 0x37d0d724d00a1248, 0
	.quad 0xdb0fead349f1c09b, 0
	.quad 0x075372c980991b7b, 0
	.size cuishen_pi_avx2, .-cuishen_pi_avx2
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
