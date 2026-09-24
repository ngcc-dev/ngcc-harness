.syntax unified
.cpu cortex-m4
.thumb

.macro montreduce q, qinv, a, tmp
  smulbb \tmp, \a, \qinv
  smlabb \a, \q, \tmp, \a
.endm

.macro double_fqinv det, tmp, tmp2, tmp3, loop, q, qinv
    // det_low inversion
    movw \tmp, #867
    movt \tmp, #1 // tmp_high = t
    smulbb \tmp2, \det, \tmp
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = a
    // loop 1-7
    movw \loop, #7
    1:
        smultt \tmp, \tmp, \tmp2
        montreduce \q, \qinv, \tmp, \tmp3 // tmp_high = t
        smultt \tmp2, \tmp2, \tmp2
        montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = a
    subs.w \loop, #1
    bne.w 1b
    // loop 8
    smultt \tmp2, \tmp2, \tmp2
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = a
    // loop 9
    smultt \tmp, \tmp, \tmp2
    montreduce \q, \qinv, \tmp, \tmp3 // tmp_high = t
    smultt \tmp2, \tmp2, \tmp2
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = a 
    // loop 10
    smultt \tmp2, \tmp2, \tmp2
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = a
    // loop 11
    smultt \tmp, \tmp, \tmp2
    montreduce \q, \qinv, \tmp, \tmp3 // tmp_high = t
    smultt \tmp2, \tmp2, \tmp2
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = a          
    // loop 12
    smultt \tmp, \tmp, \tmp2
    montreduce \q, \qinv, \tmp, \tmp3 // tmp_high = t
    // result in tmp_high

    //det_high inversion
    movw \tmp2, #867
    movt \tmp2, #1 // tmp2_high = t
    smultb \det, \det, \tmp2
    montreduce \q, \qinv, \det, \tmp3 // det_high = a
    // loop 1-7
    movw \loop, #7
    2:
        smultt \tmp2, \tmp2, \det
        montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = t
        smultt \det, \det, \det
        montreduce \q, \qinv, \det, \tmp3 // det_high = a
    subs.w \loop, #1
    bne.w 2b
    // loop 8
    smultt \det, \det, \det
    montreduce \q, \qinv, \det, \tmp3 // det_high = a
    // loop 9
    smultt \tmp2, \tmp2, \det
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = t
    smultt \det, \det, \det
    montreduce \q, \qinv, \det, \tmp3 // det_high = a 
    // loop 10
    smultt \det, \det, \det
    montreduce \q, \qinv, \det, \tmp3 // det_high = a
    // loop 11
    smultt \tmp2, \tmp2, \det
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = t
    smultt \det, \det, \det
    montreduce \q, \qinv, \det, \tmp3 // det_high = a          
    // loop 12
    smultt \tmp2, \tmp2, \det
    montreduce \q, \qinv, \tmp2, \tmp3 // tmp2_high = t
    // result in tmp2_high

    pkhtb \det, \tmp2, \tmp, asr#16
.endm

.global fqinv_asm
.type fqinv_asm, %function
.align 2
fqinv_asm:
    push {r4-r11, lr}

    rptr  .req r0
    q     .req r7
    qinv  .req r8
    tmp   .req r9
    tmp2  .req r10
    tmp3  .req r11
    tmp4  .req r12
    loop  .req r14

    movw q, #3457
    movw qinv, #52607 //-q^(-1), mont reduce

    ldr r1, [rptr]
    double_fqinv r1, tmp, tmp2, tmp3, loop, q, qinv
    str.w r1, [rptr]

    pop {r4-r11, pc}



//qinv stores -qinv
.macro double_baseinv rptr, b01, b23, a01, a23, zeta, tmp, tmp2, tmp3, tmp4, q, qinv, loop
     // det=fqmul(a1*a1)
	smultt \b01, \a01, \a01
    # a3*a3
    smultt \b23, \a23, \a23
    montreduce \q, \qinv, \b01, \tmp3
    montreduce \q, \qinv, \b23, \tmp3
     // 计算-zeta,-zeta一个放\zeta(低16bit)，一个放\tmp(高16bit)
    lsr \tmp, \zeta, #16
    neg \zeta, \zeta
    neg \tmp, \tmp
    // a1*a1*-zeta
    smulbt \b01, \zeta, \b01
    smulbt \b23, \tmp, \b23
    montreduce \q, \qinv, \b01, \tmp3
    montreduce \q, \qinv, \b23, \tmp3
    // a0*a0
    smulbb \tmp, \a01, \a01
    smulbb \tmp2, \a23, \a23
    montreduce \q, \qinv, \tmp, \tmp3
    montreduce \q, \qinv, \tmp2, \tmp3
    // det + a0*a0
    uadd16 \b01, \tmp, \b01
    uadd16 \b23, \tmp2, \b23
    // b01||b23
    pkhtb \b01, \b23, \b01, asr#16
    //fqinv(det)
    double_fqinv \b01, \tmp, \tmp2, \tmp3, \loop, \q, \qinv
    //暂存det 用于等会计算r
    mov \tmp4, \b01
    //b[0] = fqmul(a[0], det)
    smulbb \tmp, \a01, \b01
    montreduce \q, \qinv, \tmp, \tmp3
    //b[1] = fqmul(-a[1], det)
    lsr \tmp2, \a01, #16
    neg \tmp2, \tmp2
    //不能改b01,后面要用到
    smulbb \tmp2, \tmp2, \b01
    montreduce \q, \qinv, \tmp2, \tmp3
    // tmp2=b[0]||b[1](左低右高)
    pkhtb \tmp2, \tmp2, \tmp, asr#16
    str.w \tmp2, [\rptr], #4
    //b[2] = fqmul(a[2], det)
    smulbt \tmp, \a23, \b01
    montreduce \q, \qinv, \tmp, \tmp3
    //b[3] = fqmul(-a[3], det)
    lsr \tmp2, \a23, #16
    neg \tmp2, \tmp2
    smulbt \b23, \tmp2, \b01
    montreduce \q, \qinv, \b23, \tmp3
    // b23=b[2]||b[3](左低右高)
    pkhtb \b23, \b23, \tmp, asr#16
	str.w \b23, [\rptr], #4
    
    //r = (uint16_t)det;
	//r = (uint32_t)(-r) >> 31;
	//return r - 1;
    lsr \tmp2, \tmp4, #16
    uxth \tmp, \tmp4
    uxth \tmp2, \tmp2
    neg \tmp, \tmp
    neg \tmp2, \tmp2
    lsr \tmp, #31
    add \tmp, \tmp, \tmp2, lsr#31
    // return value store in tmp4
    sub \tmp4, \tmp, #2 
.endm

.global baseinv_asm
.type baseinv_asm, %function
.align 2
baseinv_asm:
    push {r4-r11, lr}

    rptr  .req r0
	polya  .req r1
	zetaptr .req r2
	poly0 .req r3
	poly1 .req r4
	res0  .req r5
	res1  .req r6
    q     .req r7
    qinv  .req r8
    tmp   .req r9
    tmp2  .req r10
    tmp3  .req r11
    tmp4  .req r12
    loop  .req r14
	zeta  .req r14
    movw q, #3457
    movw qinv, #52607 //-q^(-1), mont reduce

    movw tmp, #0
    movw loop, #192
	3:
        vmov s0, loop
		vmov s1, tmp
		ldr poly0, [polya], #4
        ldr poly1, [polya], #4
        ldr.w zeta, [zetaptr], #4
        double_baseinv rptr, res0, res1, poly0, poly1, zeta, tmp, tmp2, tmp3, tmp4, q, qinv, loop
        vmov loop, s0 // recover tmp，changed above
        vmov tmp, s1
        adds.w tmp, tmp, tmp4
    subs.w loop, #1
    bne.w 3b
        
    mov r0, tmp // return value
    pop {r4-r11, pc}

