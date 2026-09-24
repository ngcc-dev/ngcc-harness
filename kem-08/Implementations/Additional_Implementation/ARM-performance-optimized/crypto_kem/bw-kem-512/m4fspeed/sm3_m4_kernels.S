	.syntax unified
	.thumb

	.section .text.sm3_store8_be_m4, "ax", %progbits
	.global sm3_store8_be_m4
	.type sm3_store8_be_m4, %function
	.thumb_func

/* void sm3_store8_be_m4(unsigned char *out, const uint32_t in[8])
 *
 * r0 = out, 4-byte aligned
 * r1 = in,  4-byte aligned
 */
sm3_store8_be_m4:
	ldmia.w	r1!, {r2, r3, r12}
	rev	r2, r2
	rev	r3, r3
	rev	r12, r12
	stmia.w	r0!, {r2, r3, r12}

	ldmia.w	r1!, {r2, r3, r12}
	rev	r2, r2
	rev	r3, r3
	rev	r12, r12
	stmia.w	r0!, {r2, r3, r12}

	ldmia.w	r1!, {r2, r3}
	rev	r2, r2
	rev	r3, r3
	stmia.w	r0!, {r2, r3}
	bx	lr

	.size sm3_store8_be_m4, . - sm3_store8_be_m4
