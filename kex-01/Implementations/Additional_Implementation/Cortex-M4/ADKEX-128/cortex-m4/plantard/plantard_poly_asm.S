#if (DKE_MODE != 512) && defined(DKE_USE_CORTEX_M4_PLANTARD)   /* DKE config gating (auto-added) */
/******************************************************************************
* Integrating the improved Plantard arithmetic into ML-KEM.
*
* Efficient Plantard arithmetic enables a faster ML-KEM implementation with the 
* same stack usage.
*
* See the paper at https://eprint.iacr.org/2022/956.pdf for more details.
*
* @author   Junhao Huang, BNU-HKBU United International College, Zhuhai, China
*           jhhuang_nuaa@126.com
*
* @date     September 2022
******************************************************************************/
#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

.macro doublebasemul_frombytes_asm rptr, bptr, zeta, poly0, poly1, poly3, tmp, tmp2, q, qa, qinv
	ldr.w \poly0, [\bptr], #4

	smulwt \tmp, \zeta, \poly1 
	smlabt \tmp, \tmp, \q, \qa  
	smultt \tmp, \poly0, \tmp  
	smlabb \tmp, \poly0, \poly1, \tmp 
	// a1*b1*zeta+a0*b0
	plant_red \q, \qa, \qinv, \tmp
	// r[0] in upper half of tmp
	
	smuadx \tmp2, \poly0, \poly1 
	plant_red \q, \qa, \qinv, \tmp2

	// r[1] in upper half of tmp2
	pkhtb \tmp, \tmp2, \tmp, asr#16
	str \tmp, [rptr], #4

	neg \zeta, \zeta

	ldr.w \poly0, [\bptr], #4
	//basemul(r->coeffs + 4 * i + 2, a->coeffs + 4 * i + 2, b->coeffs + 4 * i + 2, - zetas[64 + i]);
	smulwt \tmp, \zeta, \poly3 
	smlabt \tmp, \tmp, \q, \qa  
	smultt \tmp, \poly0, \tmp  
	smlabb \tmp, \poly0, \poly3, \tmp 
	plant_red \q, \qa, \qinv, \tmp
	// r[0] in upper half of tmp
	
	smuadx \tmp2, \poly0, \poly3 
	plant_red \q, \qa, \qinv, \tmp2
	// r[1] in upper half of tmp2
	pkhtb \tmp, \tmp2, \tmp, asr#16
	str \tmp, [rptr], #4
.endm

.macro doublebasemul_frombytes_asm_acc rptr, bptr, zeta, poly0, poly1, poly3, res0, tmp, tmp2, q, qa, qinv

	ldr \poly0, [\bptr], #4
	
	ldr \res0, [\rptr]
	smulwt \tmp, \zeta, \poly1 
	// b_1*zeta*qinv*plant_const; in low half
	smlabt \tmp, \tmp, \q, \qa  
	// b_1*zeta
	smultt \tmp, \poly0, \tmp  
	//a_1*b_1*zeta <2^32
	smlabb \tmp, \poly0, \poly1, \tmp 
	// a1*b1*zeta+a0*b0
	plant_red \q, \qa, \qinv, \tmp
	// r[0] in upper half of tmp
	
	smuadx \tmp2, \poly0, \poly1 
	plant_red \q, \qa, \qinv, \tmp2

	// r[1] in upper half of tmp2
	pkhtb \tmp, \tmp2, \tmp, asr#16
	uadd16 \res0, \res0, \tmp
	str \res0, [\rptr], #4

	neg \zeta, \zeta
	
	ldr \poly0, [\bptr], #4
	ldr \res0, [\rptr]

	smulwt \tmp, \zeta, \poly3 
	smlabt \tmp, \tmp, \q, \qa  
	smultt \tmp, \poly0, \tmp  
	smlabb \tmp, \poly0, \poly3, \tmp 
	plant_red \q, \qa, \qinv, \tmp
	// r[0] in upper half of tmp
	
	smuadx \tmp2, \poly0, \poly3 
	plant_red \q, \qa, \qinv, \tmp2
	// r[1] in upper half of tmp2
	pkhtb \tmp, \tmp2, \tmp, asr#16
	uadd16 \res0, \res0, \tmp
	str \res0, [\rptr], #4
.endm

// reduce 2 registers
.macro deserialize aptr, tmp, tmp2, tmp3, t0, t1
	ldrb.w \tmp, [\aptr, #2]
	ldrh.w \tmp2, [\aptr, #3]
	ldrb.w \tmp3, [\aptr, #5]
	ldrh.w \t0, [\aptr], #6

	ubfx.w \t1, \t0, #12, #4
	ubfx.w \t0, \t0, #0, #12
	orr \t1, \t1, \tmp, lsl #4
	orr \t0, \t0, \t1, lsl #16
	//tmp is free now
	ubfx.w \t1, \tmp2, #12, #4
	ubfx.w \tmp, \tmp2, #0, #12
	orr \t1, \t1, \tmp3, lsl #4
	orr \t1, \tmp, \t1, lsl #16
.endm

// void frombytes_mul_asm(int16_t *r, const int16_t *b, const unsigned char *a, const int32_t zetas[64])
.global frombytes_mul_asm
.type frombytes_mul_asm, %function
.align 2
frombytes_mul_asm:
	push {r4-r11, r14}

	rptr    .req r0
	bptr    .req r1
	aptr    .req r2
	zetaptr .req r3
	t0      .req r4
	t1      .req r5
	tmp     .req r6
	tmp2    .req r7
	tmp3    .req r8
	q       .req r9
	qa      .req r10
	qinv    .req r11
	zeta    .req r12
	ctr     .req r14

	movw qa, #26632
	movt  q, #3329  
	### qinv=0x6ba8f301
	movw qinv, #62209
	movt qinv, #27560

	add ctr, rptr, #64*4*2
	1:
		ldr.w zeta, [zetaptr], #4
		deserialize aptr, tmp, tmp2, tmp3, t0, t1

		doublebasemul_frombytes_asm rptr, bptr, zeta, tmp3, t0, t1, tmp, tmp2, q, qa, qinv

	cmp.w rptr, ctr
	bne.w 1b

	pop {r4-r11, pc}
.size frombytes_mul_asm, . -frombytes_mul_asm

// void frombytes_mul_asm_acc(int16_t *r, const int16_t *b, const unsigned char *a, const int32_t zetas[64])
.global frombytes_mul_asm_acc
.type frombytes_mul_asm_acc, %function
.align 2
frombytes_mul_asm_acc:
	push {r4-r11, r14}

	rptr    .req r0
	bptr    .req r1
	aptr    .req r2
	zetaptr .req r3
	t0      .req r4
	t1      .req r5
	tmp     .req r6
	tmp2    .req r7
	tmp3    .req r8
	q       .req r9
	qa      .req r10
	qinv    .req r11
	zeta    .req r12
	ctr     .req r14

	movw qa, #26632
	movt  q, #3329  
	### qinv=0x6ba8f301
	movw qinv, #62209
	movt qinv, #27560

	add ctr, rptr, #64*4*2
	1:
		ldr.w zeta, [zetaptr], #4
		deserialize aptr, tmp, tmp2, tmp3, t0, t1
		vmov s0, ctr
		doublebasemul_frombytes_asm_acc rptr, bptr, zeta, tmp3, t0, t1, ctr, tmp, tmp2, q, qa, qinv
		vmov ctr, s0
	cmp.w rptr, ctr
	bne.w 1b

	pop {r4-r11, pc}
.size frombytes_mul_asm_acc, . - frombytes_mul_asm_acc 

.unreq rptr   
.unreq bptr   
.unreq aptr   
.unreq zetaptr
.unreq t0     
.unreq t1     
.unreq tmp    
.unreq tmp2   
.unreq tmp3   
.unreq q      
.unreq qa     
.unreq qinv   
.unreq zeta   
.unreq ctr    

.align 2
.global poly_tobytes_asm
.type poly_tobytes_asm, %function
poly_tobytes_asm:
    push {r4-r9, lr}
	rptr    .req r0
	aptr    .req r1
	pol0    .req r2
	pol1    .req r3
	pol2    .req r4
	pol3    .req r5
	pol4    .req r6
	pol5    .req r7
	pol6    .req r8
	tmp     .req r9
	loop    .req r12

    movw loop, #32
1:
	ldr.w pol0, [aptr], 4
	ldr.w pol1, [aptr], 4
	ldr.w pol2, [aptr], 4
	ldr.w pol3, [aptr], 4

	// Match bytes: c0, (c0 >> 8) | (c1 << 4), c1 >> 4.
	ubfx.w tmp, pol0, #0, #12
    ubfx.w pol4, pol0, #16, #12
	orr.w pol4, tmp, pol4, lsl #12
	
	//pol1: 32+4+12=48; pol5: 16
	orr.w pol4, pol4, pol1, lsl #24
	ubfx.w tmp, pol1, #8, #4
	ubfx.w pol5, pol1, #16, #12
	orr.w pol5, tmp, pol5, lsl #4

	//pol2: 16+12+4
	orr.w pol5, pol5, pol2, lsl #16
	ubfx.w tmp, pol2, #16, #12
	orr.w pol5, pol5, tmp, lsl #28

	//pol3: 8+12+12=32
	lsr pol6, tmp, #4
	ubfx.w tmp, pol3, #0, #12
	orr.w pol6, pol6, tmp, lsl #8
	ubfx.w tmp, pol3, #16, #12
	orr.w pol6, pol6, tmp, lsl #20

	str.w pol4, [rptr], #4
	str.w pol5, [rptr], #4
	str.w pol6, [rptr], #4
	subs loop, #1
	bne.w 1b

    pop {r4-r9, pc}
.size poly_tobytes_asm, . - poly_tobytes_asm

.unreq rptr
.unreq aptr
.unreq pol0
.unreq pol1
.unreq pol2
.unreq pol3
.unreq pol4
.unreq pol5
.unreq pol6
.unreq tmp
.unreq loop


// 4-bit *3329/16=4-bit *280=4-bit
.macro expand_signal_byte out, byte, lo, hi, q
	and.w \lo, \byte, #0x0f
	lsr.w \hi, \byte, #4
	smulbt \lo, \lo, \q
	smulbt \hi, \hi, \q
	lsr \lo, #4
	pkhbt \out, \lo, \hi, lsl #12
.endm

.align 2
.global poly_decompress_floor_asm
.type poly_decompress_floor_asm, %function
poly_decompress_floor_asm:
	push {r4-r10, lr}

	rptr .req r0
	aptr .req r1
	in0  .req r2
	in1  .req r3
	out0 .req r4
	out1 .req r5
	out2 .req r6
	out3 .req r7
	lo   .req r8
	hi   .req r9
	q    .req r10
	loop .req r12

	movw loop, #32
	movt q, #3329
1:
	ldr.w in0, [aptr], #4

	uxtb.w in1, in0
	expand_signal_byte out0, in1, lo, hi, q

	ubfx.w in1, in0, #8, #8
	expand_signal_byte out1, in1, lo, hi, q

	ubfx.w in1, in0, #16, #8
	expand_signal_byte out2, in1, lo, hi, q

	ubfx.w in1, in0, #24, #8
	expand_signal_byte out3, in1, lo, hi, q

	stm rptr!, {out0-out3}
	subs.w loop, loop, #1
	bne.w 1b

	pop {r4-r10, pc}
.size poly_decompress_floor_asm, . - poly_decompress_floor_asm

.unreq rptr
.unreq aptr
.unreq in0
.unreq in1
.unreq out0
.unreq out1
.unreq out2
.unreq out3
.unreq lo
.unreq hi
.unreq loop
.unreq q

.macro expand_rand_pair out, byte, bit0, bit1, tmp
	ubfx.w \out, \byte, #\bit0, #1
	ubfx.w \tmp, \byte, #\bit1, #1
	rsb.w \out, \out, #0
	rsb.w \tmp, \tmp, #0
	pkhbt \out, \out, \tmp, lsl #16
.endm

.align 2
.global rand_to_poly_asm
.type rand_to_poly_asm, %function
rand_to_poly_asm:
	push {r4-r8, lr}

	rptr .req r0
	aptr .req r1
	byte .req r2
	tmp  .req r3
	out0 .req r4
	out1 .req r5
	out2 .req r6
	out3 .req r7
	loop .req r8

	movw loop, #8
1:
	ldr.w byte, [aptr], #4

	expand_rand_pair out0, byte, 0, 1, tmp
	expand_rand_pair out1, byte, 2, 3, tmp
	expand_rand_pair out2, byte, 4, 5, tmp
	expand_rand_pair out3, byte, 6, 7, tmp

	stm rptr!, {out0-out3}
	expand_rand_pair out0, byte, 8+0, 8+1, tmp
	expand_rand_pair out1, byte, 8+2, 8+3, tmp
	expand_rand_pair out2, byte, 8+4, 8+5, tmp
	expand_rand_pair out3, byte, 8+6, 8+7, tmp

	stm rptr!, {out0-out3}
	expand_rand_pair out0, byte, 16+0, 16+1, tmp
	expand_rand_pair out1, byte, 16+2, 16+3, tmp
	expand_rand_pair out2, byte, 16+4, 16+5, tmp
	expand_rand_pair out3, byte, 16+6, 16+7, tmp

	stm rptr!, {out0-out3}
	expand_rand_pair out0, byte, 24+0, 24+1, tmp
	expand_rand_pair out1, byte, 24+2, 24+3, tmp
	expand_rand_pair out2, byte, 24+4, 24+5, tmp
	expand_rand_pair out3, byte, 24+6, 24+7, tmp

	stm rptr!, {out0-out3}
	subs.w loop, loop, #1
	bne.w 1b

	pop {r4-r8, pc}
.size rand_to_poly_asm, . - rand_to_poly_asm

.unreq rptr
.unreq aptr
.unreq byte
.unreq tmp
.unreq out0
.unreq out1
.unreq out2
.unreq out3
.unreq loop

.macro collect_parity_pair out, word, bitpos, mask, tmp
	and.w \tmp, \word, \mask
	ubfx.w \word, \tmp, #16, #1
	orr.w \out, \out, \tmp, lsl #\bitpos
	orr.w \out, \out, \word, lsl #(\bitpos + 1)
.endm

.align 2
.global mod2_asm
.type mod2_asm, %function
mod2_asm:
	push {r4-r8, lr}

	ssptr .req r0
	kptr  .req r1
	w0    .req r2
	w1    .req r3
	w2    .req r4
	w3    .req r5
	out   .req r6
	mask  .req r7
	tmp   .req r8
	loop  .req r12

	movw mask, #1
	movt mask, #1
	movw loop, #32

1:
	ldm kptr!, {w0-w3}
	movs out, #0

	collect_parity_pair out, w0, 0, mask, tmp
	collect_parity_pair out, w1, 2, mask, tmp
	collect_parity_pair out, w2, 4, mask, tmp
	collect_parity_pair out, w3, 6, mask, tmp

	strb out, [ssptr], #1
	subs.w loop, loop, #1
	bne.w 1b

	pop {r4-r8, pc}
.size mod2_asm, . - mod2_asm

.unreq ssptr
.unreq kptr
.unreq w0
.unreq w1
.unreq w2
.unreq w3
.unreq out
.unreq mask
.unreq tmp
.unreq loop



.align 2
.global DKE_verify_asm
.type DKE_verify_asm, %function
DKE_verify_asm:
    push {r4-r8, lr}

    aptr    .req r0
    bptr    .req r1
    len     .req r2
    acc     .req r3
    aword0  .req r4
    aword1  .req r5
    bword0  .req r6
    bword1  .req r7
    diff    .req r8

    movs acc, #0

    // Main loop: compare 8 bytes per iteration. Loads are grouped first so
    // the arithmetic can run while the next memory accesses are issued.
1:
    ldm aptr!, {aword0, aword1}
    ldm bptr!, {bword0, bword1}

    eor diff, aword0, bword0
    orr acc, acc, diff
    eor diff, aword1, bword1
    orr acc, acc, diff

    subs len, len, #8
    bne 1b

    // Collapse any nonzero lane to bit 0, then return 1 on mismatch, 0 on
    // equality. This matches the C contract exactly.

    orr acc, acc, acc, lsr #16
    orr acc, acc, acc, lsr #8
    and acc, acc, #0xff
    rsb r0, acc, #0
    lsr r0, r0, #31

    .unreq aptr
    .unreq bptr
    .unreq len
    .unreq acc
    .unreq aword0
    .unreq aword1
    .unreq bword0
    .unreq bword1
    .unreq diff

    pop {r4-r8, pc}
.size DKE_verify_asm, . - DKE_verify_asm

.align 2
.global DKE_cmov_asm
.type DKE_cmov_asm, %function
DKE_cmov_asm:
    push {r4-r7, lr}

    rptr    .req r0
    xptr    .req r1
    len     .req r2
    cmask   .req r3
    rword0  .req r4
    xword0  .req r5
    diff0   .req r6
    rword1  .req r7
    xword1  .req r12

    // b is required to be in {0, 1}; convert it into a full-word mask.
    rsb cmask, cmask, #0

    // Main loop: process 8 bytes per iteration. Bitwise ops update four
    // byte lanes in parallel, which is the SIMD-style fast path on M4 here.
1:
    ldr rword1, [rptr, #4]
	ldr rword0, [rptr]
    ldr xword1, [xptr, #4]
    ldr xword0, [xptr], #8

    eor diff0, rword0, xword0
    and diff0, diff0, cmask
    eor rword0, rword0, diff0

    eor diff0, rword1, xword1
    and diff0, diff0, cmask
    eor rword1, rword1, diff0

    str rword0, [rptr], #4
    str rword1, [rptr], #4
    subs len, len, #8
    bne 1b
    .unreq rptr
    .unreq xptr
    .unreq len
    .unreq cmask
    .unreq rword0
    .unreq xword0
    .unreq diff0
    .unreq rword1
    .unreq xword1

    pop {r4-r7, pc}
.size DKE_cmov_asm, . - DKE_cmov_asm

.macro doublebasemul_frombytes_asm_16_32 rptr_tmp, bptr, zeta, poly0, poly2, poly1, poly3, tmp, q, qa, qinv
  ldr \poly0, [\bptr], #4
  ldr \poly2, [\bptr], #4

  smulwt \tmp, \zeta, \poly1 
	smlabt \tmp, \tmp, \q, \qa  
	smultt \tmp, \poly0, \tmp  
	smlabb \tmp, \poly0, \poly1, \tmp 
  str \tmp, [\rptr_tmp], #4

  smuadx \tmp, \poly0, \poly1
  str \tmp, [\rptr_tmp], #4

  neg \zeta, \zeta

  smulwt \tmp, \zeta, \poly3 
	smlabt \tmp, \tmp, \q, \qa  
	smultt \tmp, \poly2, \tmp  
	smlabb \tmp, \poly2, \poly3, \tmp 
  str \tmp, [\rptr_tmp], #4

  smuadx \tmp, \poly2, \poly3
  str \tmp, [\rptr_tmp], #4
.endm

.macro doublebasemul_frombytes_asm_acc_32_32 rptr_tmp, bptr, zeta, poly0, poly1, poly3, res0, tmp, q, qa, qinv
  ldr \poly0, [\bptr], #4
  ldr \res0, [\rptr_tmp]
  
  smulwt \tmp, \zeta, \poly1 
	smlabt \tmp, \tmp, \q, \qa  
	smlatt \tmp, \poly0, \tmp, \res0
	smlabb \tmp, \poly0, \poly1, \tmp 

  str \tmp, [\rptr_tmp], #4

  ldr \res0, [\rptr_tmp]
  smladx \tmp, \poly0, \poly1, \res0
  str \tmp, [\rptr_tmp], #4

  neg \zeta, \zeta

  ldr \poly0, [\bptr], #4
  ldr \res0, [\rptr_tmp]
  
  smulwt \tmp, \zeta, \poly3 
	smlabt \tmp, \tmp, \q, \qa  
	smlatt \tmp, \poly0, \tmp, \res0
	smlabb \tmp, \poly0, \poly3, \tmp 

  str \tmp, [\rptr_tmp], #4

  ldr \res0, [\rptr_tmp]
  smladx \tmp, \poly0, \poly3, \res0
  str \tmp, [\rptr_tmp], #4
.endm

.macro doublebasemul_frombytes_asm_acc_32_16 rptr_tmp, rptr, bptr, zeta, poly0, poly1, poly3, res0, tmp, q, qa, qinv
  ldr \poly0, [\bptr], #4
  ldr \res0, [\rptr_tmp], #4
  
  smulwt \tmp, \zeta, \poly1 
	smlabt \tmp, \tmp, \q, \qa  
	smlatt \tmp, \poly0, \tmp, \res0
	smlabb \tmp, \poly0, \poly1, \tmp 
  plant_red \q, \qa, \qinv, \tmp

  ldr \res0, [\rptr_tmp], #4
  smladx \res0, \poly0, \poly1, \res0
  plant_red \q, \qa, \qinv, \res0

  pkhtb \res0, \res0, \tmp, asr#16
  str \res0, [\rptr], #4

  neg \zeta, \zeta

  ldr \poly0, [\bptr], #4
  ldr \res0, [\rptr_tmp], #4
  
  smulwt \tmp, \zeta, \poly3 
	smlabt \tmp, \tmp, \q, \qa  
	smlatt \tmp, \poly0, \tmp, \res0
	smlabb \tmp, \poly0, \poly3, \tmp 
  plant_red \q, \qa, \qinv, \tmp

  ldr \res0, [\rptr_tmp], #4
  smladx \res0, \poly0, \poly3, \res0
  plant_red \q, \qa, \qinv, \res0

  pkhtb \res0, \res0, \tmp, asr#16
  str \res0, [\rptr], #4
.endm 


// void frombytes_mul_asm_16_32(int32_t *r_tmp, const int16_t *b, const unsigned char *c, const int32_t zetas[64])
.global frombytes_mul_asm_16_32
.type frombytes_mul_asm_16_32, %function
.align 2
frombytes_mul_asm_16_32:
  push {r4-r11, r14}

  rptr_tmp .req r0
  bptr     .req r1
  aptr     .req r2
  zetaptr  .req r3
  t0       .req r4
	t1       .req r5
	tmp      .req r6
	tmp2     .req r7
	tmp3     .req r8
	q        .req r9
	qa       .req r10
	qinv     .req r11
	zeta     .req r12
	ctr      .req r14

  movw qa, #26632
	movt  q, #3329  
	### qinv=0x6ba8f301
	movw qinv, #62209
	movt qinv, #27560

  add ctr, rptr_tmp, #64*4*4
  1:
    ldr.w zeta, [zetaptr], #4
    deserialize aptr, tmp, tmp2, tmp3, t0, t1

    doublebasemul_frombytes_asm_16_32 rptr_tmp, bptr, zeta, tmp, tmp2, t0, t1, tmp3, q, qa, qinv

    cmp.w rptr_tmp, ctr
    bne.w 1b

pop {r4-r11, pc}
.size frombytes_mul_asm_16_32, . -frombytes_mul_asm_16_32

// void frombytes_mul_asm_acc_32_32(int32_t *r_tmp, const int16_t *b, const unsigned char *c, const int32_t zetas[64])
.global frombytes_mul_asm_acc_32_32
.type frombytes_mul_asm_acc_32_32, %function
.align 2
frombytes_mul_asm_acc_32_32:
  push {r4-r11, r14}

  rptr_tmp .req r0
  bptr     .req r1
  aptr     .req r2
  zetaptr  .req r3
  t0       .req r4
	t1       .req r5
	tmp      .req r6
	tmp2     .req r7
	tmp3     .req r8
	q        .req r9
	qa       .req r10
	qinv     .req r11
	zeta     .req r12
	ctr      .req r14

  movw qa, #26632
	movt  q, #3329  
	### qinv=0x6ba8f301
	movw qinv, #62209
	movt qinv, #27560

  add ctr, rptr_tmp, #64*4*4
  1:
    ldr.w zeta, [zetaptr], #4
    deserialize aptr, tmp, tmp2, tmp3, t0, t1

    doublebasemul_frombytes_asm_acc_32_32 rptr_tmp, bptr, zeta, tmp3, t0, t1, tmp, tmp2, q, qa, qinv
    cmp.w rptr_tmp, ctr
    bne.w 1b

pop {r4-r11, pc}
.size frombytes_mul_asm_acc_32_32, . - frombytes_mul_asm_acc_32_32

.unreq rptr_tmp

// void frombytes_mul_asm_acc_32_16(int16_t *r, const int16_t *b, const unsigned char *c, const int32_t zetas[64], const int32_t *r_tmp)
.global frombytes_mul_asm_acc_32_16
.type frombytes_mul_asm_acc_32_16, %function
.align 2
frombytes_mul_asm_acc_32_16:
  push {r4-r11, r14}

  rptr     .req r0
  bptr     .req r1
  aptr     .req r2
  zetaptr  .req r3
  t0       .req r4
	t1       .req r5
	tmp      .req r6
	tmp2     .req r7
	tmp3     .req r8
	q        .req r9
	qa       .req r10
	qinv     .req r11
	zeta     .req r12
	ctr      .req r14
  rptr_tmp .req r3

  movw qa, #26632
	movt  q, #3329  
	### qinv=0x6ba8f301
	movw qinv, #62209
	movt qinv, #27560

  ldr.w tmp, [sp, #9*4] // load rptr_tmp from stack
  vmov s1, tmp
  
  add ctr, tmp, #64*4*4
  1:
    ldr.w zeta, [zetaptr], #4
    deserialize aptr, tmp, tmp2, tmp3, t0, t1
    vmov s2, zetaptr
    vmov rptr_tmp, s1
    doublebasemul_frombytes_asm_acc_32_16 rptr_tmp, rptr, bptr, zeta, tmp3, t0, t1, tmp, tmp2, q, qa, qinv
    vmov s1, rptr_tmp
    cmp.w rptr_tmp, ctr
    vmov zetaptr, s2
    bne.w 1b

pop {r4-r11, pc}
.size frombytes_mul_asm_acc_32_16, . - frombytes_mul_asm_acc_32_16
#endif /* (DKE_MODE != 512) && defined(DKE_USE_CORTEX_M4_PLANTARD) -- DKE config gating (auto-added) */
