	.syntax unified
	.thumb
	.text

	.macro DIST off
	ldr	r2, [r1, #\off]
	and	r2, r2, r9
	usub16	r3, r10, r2
	usub16	r8, r2, r3
	sel	r3, r3, r2		@ d0 = GE? wrap : v = min(v, 4096-v)
	ssub16	r8, r2, r11
	smulbb	r2, r3, r3
	smultt	r12, r3, r3
	smulbb	r3, r8, r8
	smultt	r8, r8, r8
	.endm

	.macro FOLD0 K, tot, bd, pl, bp
	add	r2, r2, r12		@ cl0 = de0 + do0
	add	r3, r3, r8		@ cl1 = de1 + do1
	subs	r12, r3, r2
	sbc	r8, r8, r8		@ mask = take_label1 ? 0xFFFFFFFF : 0
	and	lr, r12, r8		@ diff & mask
	add	\tot, \tot, r2		@ tot += cl0
	add	\tot, \tot, lr		@ tot += diff&mask  => tot += min(cl0,cl1)
	eor	lr, r12, r8		@ diff ^ mask
	sub	lr, lr, r8		@ delta = |diff| = (diff^mask) - mask
	and	r2, r8, #1		@ chosen_label = mask & 1 (= take_label1)
	orr	\pl, \pl, r2, lsl #\K	@ pl |= label << K
	subs	r3, lr, \bd
	sbc	r2, r2, r2		@ dmask = take_delta ? -1 : 0
	and	r3, r3, r2		@ (delta-bd) & dmask
	add	\bd, \bd, r3		@ bd = min(delta, bd)
	bic	\bp, \bp, r2		@ bp &= ~dmask
	and	r3, r2, #\K		@ K & dmask  (dmask=-1 -> K, 0 -> 0)
	orr	\bp, \bp, r3		@ bp = take_delta ? K : bp
	.endm

	.macro FOLD1 K, tot, bd, pl, bp
	add	lr, r2, r8		@ cl1 = de0 + do1
	add	r2, r3, r12
	subs	r12, lr, r2		@ diff = cl1 - cl0
	sbc	r8, r8, r8		@ mask
	and	r3, r12, r8		@ diff & mask
	add	\tot, \tot, r2		@ tot += cl0
	add	\tot, \tot, r3		@ tot += min
	eor	r3, r12, r8		@ diff ^ mask
	sub	r3, r3, r8		@ delta
	and	r12, r8, #1		@ label
	orr	\pl, \pl, r12, lsl #\K	@ pl |= label << K
	subs	r12, r3, \bd		@ delta - bd
	sbc	r8, r8, r8		@ dmask
	and	r12, r12, r8		@ (delta-bd) & dmask
	add	\bd, \bd, r12		@ bd = min(delta, bd)
	bic	\bp, \bp, r8		@ bp &= ~dmask
	and	r12, r8, #\K		@ K & dmask
	orr	\bp, \bp, r12		@ bp = take_delta ? K : bp
	.endm

	.macro FINISH coset, tot, bd, pl, bp, nib
	eor	r2, \pl, \pl, lsr #2
	eor	r2, r2, r2, lsr #1
	and	r2, r2, #1		@ parity = label0^label1^label2^label3
	rsb	r2, r2, #0		@ parity_mask = parity ? -1 : 0
	and	r3, \bd, r2		@ bd & parity_mask
	add	\tot, \tot, r3		@ tot += (parity ? best_delta : 0)
	mov	r3, #1
	lsl	r3, r3, \bp		@ 1 << best_pair
	and	r3, r3, r2		@ & parity_mask
	eor	\pl, \pl, r3		@ pl ^= (parity ? 1<<best_pair : 0)
	eor	r2, \pl, \pl, lsl #1	@ nibble = ((((pl^(pl<<1))&3)
	and	r2, r2, #3		@          | ((pl>>1)&4)) << 1) | coset
	lsr	r3, \pl, #1
	and	r3, r3, #4
	orr	r2, r2, r3
	lsl	r2, r2, #1
	orr	\nib, r2, #\coset
	.endm

	.macro DECODE_BLOCK
	movs	r4, #0			@ tot0
	mov	r5, #0xFFFFFFFF
	movs	r6, #0			@ pl0
	movs	r7, #0			@ bp0
	DIST	0
	FOLD0	0, r4, r5, r6, r7
	DIST	4
	FOLD0	1, r4, r5, r6, r7
	DIST	8
	FOLD0	2, r4, r5, r6, r7
	DIST	12
	FOLD0	3, r4, r5, r6, r7
	FINISH	0, r4, r5, r6, r7, r2
	str	r4, [sp, #4]
	str	r2, [sp, #8]
	movs	r4, #0
	mov	r5, #0xFFFFFFFF
	movs	r6, #0
	movs	r7, #0
	DIST	0
	FOLD1	0, r4, r5, r6, r7
	DIST	4
	FOLD1	1, r4, r5, r6, r7
	DIST	8
	FOLD1	2, r4, r5, r6, r7
	DIST	12
	FOLD1	3, r4, r5, r6, r7
	FINISH	1, r4, r5, r6, r7, r2	@ r2 = nib1, r4 = tot1
	ldr	r3, [sp, #4]		@ tot0
	ldr	r8, [sp, #8]		@ nib0
	subs	r3, r4, r3
	sbc	r3, r3, r3		@ mask = tot1<tot0 ? -1 : 0
	eor	r12, r8, r2		@ nib0 ^ nib1
	and	r12, r12, r3
	eor	r3, r8, r12		@ nibble = mask ? nib1 : nib0
	add	r1, r1, #16
	.endm

	.global	codec_decode
	.type	codec_decode, %function
	.thumb_func
	.p2align 2
codec_decode:
	push	{r4-r11, lr}
	sub	sp, sp, #20
	movw	r9, #0x0FFF
	movt	r9, #0x0FFF		@ MASK12 = 0x0FFF0FFF
	movw	r10, #0x1000
	movt	r10, #0x1000		@ QHAT16 = 0x10001000
	movw	r11, #0x0800
	movt	r11, #0x0800		@ HALF16 = 0x08000800
	movs	r2, #16
	str	r2, [sp, #0]
.Loop:
	DECODE_BLOCK			@ block 2i -> lo nibble (r3)
	str	r3, [sp, #12]
	DECODE_BLOCK			@ block 2i+1 -> hi nibble (r3)
	ldr	r2, [sp, #12]		@ lo
	orr	r3, r2, r3, lsl #4
	strb	r3, [r0], #1
	ldr	r2, [sp, #0]
	subs	r2, r2, #1
	str	r2, [sp, #0]
	bne	.Loop
	add	sp, sp, #20
	pop	{r4-r11, pc}
	.size	codec_decode, .-codec_decode
