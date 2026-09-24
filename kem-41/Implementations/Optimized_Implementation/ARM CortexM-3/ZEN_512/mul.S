.syntax unified
.thumb

.macro umull64 c0, c1, c2, c3, a0, a1, b0, b1, tmp1, tmp2
    umull \tmp1, \tmp2, \a0, \b1
    umlal \tmp1, \tmp2, \a1, \b0
    umull \c0, \c1, \a0, \b0
    umull \c2, \c3, \a1, \b1
    adds.w \c1, \c1, \tmp1
    adcs.w \c2, \c2, \tmp2
    adc.w \c3, \c3, #0
.endm

.global umull64_shr58
.type umull64_shr58, %function
.align 2
umull64_shr58:
    push {r4}

    /* r0 == x low */
    /* r1 == x high */
    /* r2 == y low */
    /* r3 == y high */

    /* compute x * y */
    /* res == r3 | r1 | r2 | r0 */
    umull64 r0, r2, r1, r3, r0, r1, r2, r3, r4, r12

    lsr.w r12, r1, #26
    lsr.w r4, r2, #26
    orr.w r0, r4, r1, lsl #6
    orr.w r1, r12, r3, lsl #6

    pop {r4}
    bx lr
.size umull64_shr58, .-umull64_shr58

.macro fqmul_asm re, a, b, qinv, q, tmp
    mul.w \re, \a, \b
    mul.w \tmp, \re, \qinv
    sxth \tmp, \tmp
    mul.w \tmp, \tmp, \q
    sub.w \re, \re, \tmp
    asr.w \re, \re, #16
.endm

.macro karatsuba_mul4 addr_r, addr_a, addr_b
    //push {k4_a0,k4_a1,k4_b0,k4_b1,tmp0,tmp1,tmp2}
    sub.w sp, #6*4

    /* p0[0] == sp ; p0[1] == sp + 1*4 ; p0[2] == sp + 2*4
     * p1[0] == sp + 3*4 ; p1[1] == sp + 4*4 ; p1[2] == sp + 5*4
     */

    /* p0 */
    ldr.w k4_a0, [\addr_a]
    ldr.w k4_a1, [\addr_a, #1*4]
    ldr.w k4_b0, [\addr_b]
    ldr.w k4_b1, [\addr_b, #1*4]
    fqmul_asm tmp0, k4_a0, k4_b0, qinv, q, tmp1
    fqmul_asm tmp2, k4_a1, k4_b1, qinv, q, tmp1
    add.w k4_a0, k4_a0, k4_a1
    add.w k4_b0, k4_b0, k4_b1
    add.w tmp1, tmp0, tmp2
    fqmul_asm k4_a0, k4_a0, k4_b0, qinv, q, k4_b0
    sub.w tmp1, k4_a0, tmp1
    str.w tmp0, [sp]
    str.w tmp1, [sp, #1*4]
    str.w tmp2, [sp, #2*4]
    str.w tmp0, [\addr_r]       // -> r[0]
    str.w tmp1, [\addr_r, #1*4] // -> r[1]

    /* p1 */
    ldr.w k4_a2, [\addr_a, #2*4]
    ldr.w k4_a3, [\addr_a, #3*4]
    ldr.w k4_b2, [\addr_b, #2*4]
    ldr.w k4_b3, [\addr_b, #3*4]
    fqmul_asm tmp0, k4_a2, k4_b2, qinv, q, tmp1
    fqmul_asm tmp2, k4_a3, k4_b3, qinv, q, tmp1
    add.w k4_a2, k4_a2, k4_a3
    add.w k4_b2, k4_b2, k4_b3
    add.w tmp1, tmp0, tmp2
    fqmul_asm k4_a2, k4_a2, k4_b2, qinv, q, k4_b2
    sub.w tmp1, k4_a2, tmp1
    str.w tmp1, [sp, #4*4]
    str.w tmp2, [sp, #5*4]
    str.w tmp0, [sp, #3*4]
    str.w tmp1, [\addr_r, #5*4] // -> r[5]
    str.w tmp2, [\addr_r, #6*4] // -> r[6]

    /* pm */
    ldr.w k4_a0, [\addr_a]
    ldr.w k4_a1, [\addr_a, #1*4]
    ldr.w k4_a2, [\addr_a, #2*4]
    ldr.w k4_a3, [\addr_a, #3*4]
    add.w k4_a0, k4_a0, k4_a2
    add.w k4_a1, k4_a1, k4_a3
    
    ldr.w k4_b0, [\addr_b]
    ldr.w k4_b1, [\addr_b, #1*4]
    ldr.w tmp0, [\addr_b, #2*4]
    ldr.w tmp1, [\addr_b, #3*4]
    add.w k4_b0, k4_b0, tmp0
    add.w k4_b1, k4_b1, tmp1

    fqmul_asm tmp0, k4_a0, k4_b0, qinv, q, tmp1
    fqmul_asm tmp2, k4_a1, k4_b1, qinv, q, tmp1
    add.w k4_a0, k4_a0, k4_a1
    add.w k4_b0, k4_b0, k4_b1
    add.w tmp1, tmp0, tmp2
    fqmul_asm k4_a0, k4_a0, k4_b0, qinv, q, k4_b0
    sub.w tmp1, k4_a0, tmp1
    
    ldr.w k4_a0, [sp]       
    ldr.w k4_a1, [sp, #2*4] // p0[2]
    ldr.w k4_b0, [sp, #3*4] // p1[0]
    ldr.w k4_b1, [sp, #5*4] // p1[2]
    add.w k4_a0, k4_a0, k4_b0
    add.w tmp0, tmp0, k4_a1
    add.w k4_b1, k4_b1, k4_a1
    add.w tmp2, tmp2, k4_b0
    sub.w tmp0, tmp0, k4_a0 // tmp0 -> r[2]
    sub.w tmp2, tmp2, k4_b1 // tmp2 -> r[4]
    
    ldr.w k4_a0, [sp, #1*4]
    ldr.w k4_a1, [sp, #4*4]
    sub.w tmp1, tmp1, k4_a0
    sub.w tmp1, tmp1, k4_a1 // tmp1 -> r[3]
    
    str.w tmp0, [\addr_r, #2*4]   // -> r[2]
    str.w tmp1, [\addr_r, #3*4]   // -> r[3]
    str.w tmp2, [\addr_r, #4*4]   // -> r[4]
    
    add.w sp, #6*4
    //pop {k4_a0,k4_a1,k4_b0,k4_b1,tmp0,tmp1,tmp2}
.endm

.macro karatsuba_mul8 addr_r, addr_a, addr_b
    //push {k8_a0,k8_a1,k8_a2,k8_a3,tmp0,tmp1,tmp2,tmp3,tmp4}
    sub.w sp, sp, #32*4

    str.w \addr_r, [sp, #29*4]
    str.w \addr_a, [sp, #30*4]
    str.w \addr_b, [sp, #31*4]

    /* as[0] == sp
     * bs[0] == sp + 4*4
     * p0[0] == sp + 8*4
     * p1[0] == sp + 15*4
     * pm[0] == sp + 22*4
     */

    /* as bs */
    ldr.w k8_a0, [\addr_a]
    ldr.w k8_a1, [\addr_a, #1*4]
    ldr.w k8_a2, [\addr_a, #2*4]
    ldr.w k8_a3, [\addr_a, #3*4]
    ldr.w tmp0, [\addr_a, #4*4]
    ldr.w tmp1, [\addr_a, #5*4]
    ldr.w tmp2, [\addr_a, #6*4]
    ldr.w tmp3, [\addr_a, #7*4]
    add.w tmp0, tmp0, k8_a0
    add.w tmp1, tmp1, k8_a1
    add.w tmp2, tmp2, k8_a2
    add.w tmp3, tmp3, k8_a3
    str.w tmp0, [sp]
    str.w tmp1, [sp, #1*4]
    str.w tmp2, [sp, #2*4]
    str.w tmp3, [sp, #3*4]

    ldr.w k8_b0, [\addr_b]
    ldr.w k8_b1, [\addr_b, #1*4]
    ldr.w k8_b2, [\addr_b, #2*4]
    ldr.w k8_b3, [\addr_b, #3*4]
    ldr.w tmp0, [\addr_b, #4*4]
    ldr.w tmp1, [\addr_b, #5*4]
    ldr.w tmp2, [\addr_b, #6*4]
    ldr.w tmp3, [\addr_b, #7*4]
    add.w tmp0, tmp0, k8_b0
    add.w tmp1, tmp1, k8_b1
    add.w tmp2, tmp2, k8_b2
    add.w tmp3, tmp3, k8_b3
    str.w tmp0, [sp, #4*4]
    str.w tmp1, [sp, #5*4]
    str.w tmp2, [sp, #6*4]
    str.w tmp3, [sp, #7*4]

    /* karatsuba_mul4 */
    add.w \addr_r, sp, #8*4
    karatsuba_mul4 \addr_r, \addr_a, \addr_b

    add.w \addr_r, sp, #15*4
    add.w \addr_a, \addr_a, #4*4
    add.w \addr_b, \addr_b, #4*4
    karatsuba_mul4 \addr_r, \addr_a, \addr_b

    add.w \addr_r, sp, #22*4
    add.w \addr_a, sp, #0
    add.w \addr_b, sp, #4*4
    karatsuba_mul4 \addr_r, \addr_a, \addr_b

    /* as[0] == sp
     * bs[0] == sp + 4*4
     * p0[0] == sp + 8*4
     * p1[0] == sp + 15*4
     * pm[0] == sp + 22*4
     */

    ldr.w tmp0, [sp, #8*4]
    ldr.w tmp1, [sp, #9*4]
    ldr.w tmp2, [sp, #10*4]
    ldr.w tmp3, [sp, #11*4]
    ldr.w k8_a0, [sp, #18*4]
    ldr.w k8_a1, [sp, #19*4]
    ldr.w k8_a2, [sp, #20*4]
    ldr.w k8_a3, [sp, #21*4]

    ldr.w \addr_r, [sp, #29*4]
    ldr.w \addr_a, [sp, #30*4]
    ldr.w \addr_b, [sp, #31*4]

    /* r[0]-r[3] ; r[11]-r[14] */
    str.w tmp0, [\addr_r]
    str.w tmp1, [\addr_r, #1*4]
    str.w tmp2, [\addr_r, #2*4]
    str.w tmp3, [\addr_r, #3*4]
    str.w k8_a0, [\addr_r, #11*4]
    str.w k8_a1, [\addr_r, #12*4]
    str.w k8_a2, [\addr_r, #13*4]
    str.w k8_a3, [\addr_r, #14*4]

    /* r[4]-r[10] */
    ldr.w tmp0, [sp, #22*4]
    ldr.w tmp1, [sp, #8*4]
    ldr.w tmp2, [sp, #15*4]     // tmp2 == p1[0]
    ldr.w tmp3, [sp, #12*4]     // tmp3 == p0[4]
    ldr.w k8_a0, [sp, #23*4]
    ldr.w k8_a1, [sp, #9*4]
    ldr.w k8_a2, [sp, #16*4]    // k8_a2 == p1[1]
    ldr.w k8_a3, [sp, #13*4]    // k8_a3 == p0[5]

    add.w tmp0, tmp0, tmp3
    add.w tmp1, tmp1, tmp2
    add.w k8_a0, k8_a0, k8_a3
    add.w k8_a1, k8_a1, k8_a2
    sub.w tmp0, tmp0, tmp1      // -> r[4]
    sub.w k8_a0, k8_a0, k8_a1   // -> r[5]

    ldr.w tmp1, [sp, #26*4]   // pm[4]
    ldr.w k8_a1, [sp, #19*4]  // p1[4]
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, k8_a1
    sub.w tmp2, tmp2, tmp3     // -> r[8]

    ldr.w tmp1, [sp, #27*4]   // pm[5]
    ldr.w k8_a1, [sp, #20*4]  // p1[5]
    add.w k8_a2, k8_a2, tmp1
    add.w k8_a3, k8_a3, k8_a1
    sub.w k8_a2, k8_a2, k8_a3     // -> r[9]

    str.w tmp0, [\addr_r, #4*4]
    str.w k8_a0, [\addr_r, #5*4]
    str.w tmp2, [\addr_r, #8*4]
    str.w k8_a2, [\addr_r, #9*4]

    ldr.w tmp0, [sp, #14*4]  // p0[6]
    ldr.w tmp1, [sp, #24*4]  // pm[2]
    ldr.w tmp2, [sp, #10*4]  // p0[2]
    ldr.w tmp3, [sp, #17*4]  // p1[2]
    ldr.w k8_a0, [sp, #28*4]  // pm[6]
    ldr.w k8_a1, [sp, #21*4]  // p1[6]
    ldr.w k8_a2, [sp, #11*4]  // p0[3]
    ldr.w k8_a3, [sp, #18*4]  // p1[3]

    add.w tmp2, tmp2, tmp3  // p0[2] + p1[2]
    add.w k8_a1, tmp0, k8_a1  // p0[6] + p1[6]
    add.w k8_a2, k8_a2, k8_a3  // p0[3] + p1[3]

    add.w k8_a0, tmp3, k8_a0  // p1[2] + pm[6]
    add.w tmp1, tmp0, tmp1  // p0[6] + pm[2]
    ldr.w k8_a3, [sp, #25*4] // pm[3]

    sub.w k8_a0, k8_a0, k8_a1  // -> r[10]
    sub.w tmp1, tmp1, tmp2  // -> r[6]
    sub.w k8_a3, k8_a3, k8_a2  // -> r[7]

    str.w k8_a0, [\addr_r, #10*4]
    str.w tmp1, [\addr_r, #6*4]
    str.w k8_a3, [\addr_r, #7*4]

    add.w sp, sp, #32*4
    //pop {k8_a0,k8_a1,k8_a2,k8_a3,tmp0,tmp1,tmp2,tmp3,tmp4}
.endm

.macro base_mul addr_r, addr_a, addr_b, zeta
    push {r0-r2,r7-r10,r12}
    sub.w sp, sp, #96*4

    str.w \addr_r, [sp, #61*4]
    str.w \addr_a, [sp, #62*4]
    str.w \addr_b, [sp, #63*4]

    /* as[0] == sp
     * bs[0] == sp + 8*4
     * p0[0] == sp + 16*4
     * p1[0] == sp + 31*4
     * pm[0] == sp + 46*4
     */

     /* a[0] == sp + 64*4
      * b[0] == sp + 80*4
      */

    /* as bs */
    ldrsh.w r7, [\addr_a]
    ldrsh.w r8, [\addr_a, #1*2]
    ldrsh.w r9, [\addr_a, #2*2]
    ldrsh.w r10, [\addr_a, #3*2]
    ldrsh.w tmp0, [\addr_a, #8*2]
    ldrsh.w tmp1, [\addr_a, #9*2]
    ldrsh.w tmp2, [\addr_a, #10*2]
    ldrsh.w tmp3, [\addr_a, #11*2]
    str.w r7, [sp, #64*4]
    str.w r8, [sp, #65*4]
    str.w r9, [sp, #66*4]
    str.w r10, [sp, #67*4]
    str.w tmp0, [sp, #72*4]
    str.w tmp1, [sp, #73*4]
    str.w tmp2, [sp, #74*4]
    str.w tmp3, [sp, #75*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp]
    str.w tmp1, [sp, #1*4]
    str.w tmp2, [sp, #2*4]
    str.w tmp3, [sp, #3*4]

    ldrsh.w r7, [\addr_a, #4*2]
    ldrsh.w r8, [\addr_a, #5*2]
    ldrsh.w r9, [\addr_a, #6*2]
    ldrsh.w r10, [\addr_a, #7*2]
    ldrsh.w tmp0, [\addr_a, #12*2]
    ldrsh.w tmp1, [\addr_a, #13*2]
    ldrsh.w tmp2, [\addr_a, #14*2]
    ldrsh.w tmp3, [\addr_a, #15*2]
    str.w r7, [sp, #68*4]
    str.w r8, [sp, #69*4]
    str.w r9, [sp, #70*4]
    str.w r10, [sp, #71*4]
    str.w tmp0, [sp, #76*4]
    str.w tmp1, [sp, #77*4]
    str.w tmp2, [sp, #78*4]
    str.w tmp3, [sp, #79*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp, #4*4]
    str.w tmp1, [sp, #5*4]
    str.w tmp2, [sp, #6*4]
    str.w tmp3, [sp, #7*4]

    ldrsh.w r7, [\addr_b]
    ldrsh.w r8, [\addr_b, #1*2]
    ldrsh.w r9, [\addr_b, #2*2]
    ldrsh.w r10, [\addr_b, #3*2]
    ldrsh.w tmp0, [\addr_b, #8*2]
    ldrsh.w tmp1, [\addr_b, #9*2]
    ldrsh.w tmp2, [\addr_b, #10*2]
    ldrsh.w tmp3, [\addr_b, #11*2]
    str.w r7, [sp, #80*4]
    str.w r8, [sp, #81*4]
    str.w r9, [sp, #82*4]
    str.w r10, [sp, #83*4]
    str.w tmp0, [sp, #88*4]
    str.w tmp1, [sp, #89*4]
    str.w tmp2, [sp, #90*4]
    str.w tmp3, [sp, #91*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp, #8*4]
    str.w tmp1, [sp, #9*4]
    str.w tmp2, [sp, #10*4]
    str.w tmp3, [sp, #11*4]

    ldrsh.w r7, [\addr_b, #4*2]
    ldrsh.w r8, [\addr_b, #5*2]
    ldrsh.w r9, [\addr_b, #6*2]
    ldrsh.w r10, [\addr_b, #7*2]
    ldrsh.w tmp0, [\addr_b, #12*2]
    ldrsh.w tmp1, [\addr_b, #13*2]
    ldrsh.w tmp2, [\addr_b, #14*2]
    ldrsh.w tmp3, [\addr_b, #15*2]
    str.w r7, [sp, #84*4]
    str.w r8, [sp, #85*4]
    str.w r9, [sp, #86*4]
    str.w r10, [sp, #87*4]
    str.w tmp0, [sp, #92*4]
    str.w tmp1, [sp, #93*4]
    str.w tmp2, [sp, #94*4]
    str.w tmp3, [sp, #95*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp, #12*4]
    str.w tmp1, [sp, #13*4]
    str.w tmp2, [sp, #14*4]
    str.w tmp3, [sp, #15*4]

    /* karatsuba_mul8 */
    add.w \addr_r, sp, #16*4
    add.w \addr_a, sp, #64*4
    add.w \addr_b, sp, #80*4
    karatsuba_mul8 \addr_r, \addr_a, \addr_b

    add.w \addr_r, sp, #31*4
    add.w \addr_a, \addr_a, #8*4
    add.w \addr_b, \addr_b, #8*4
    karatsuba_mul8 \addr_r, \addr_a, \addr_b

    add.w \addr_r, sp, #46*4
    add.w \addr_a, sp, #0
    add.w \addr_b, sp, #8*4
    karatsuba_mul8 \addr_r, \addr_a, \addr_b

    /* as[0] == sp
     * bs[0] == sp + 8*4
     * p0[0] == sp + 16*4
     * p1[0] == sp + 31*4
     * pm[0] == sp + 46*4
     */

    ldr.w \addr_r, [sp, #61*4]
    mov.w mont, #171
    /* r[1] r[9] */
    ldr.w tmp0, [sp, #47*4]
    ldr.w tmp1, [sp, #55*4] // pm
    ldr.w tmp2, [sp, #17*4]
    ldr.w tmp3, [sp, #25*4] // p0
    ldr.w r7, [sp, #32*4]
    ldr.w r8, [sp, #40*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[1]
    sub.w tmp1, tmp1, r8    // cross[9]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[1]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[9]
    strh.w tmp2, [\addr_r, #1*2]
    strh.w tmp3, [\addr_r, #9*2]

    /* r[2] r[10] */
    ldr.w tmp0, [sp, #48*4]
    ldr.w tmp1, [sp, #56*4] // pm
    ldr.w tmp2, [sp, #18*4]
    ldr.w tmp3, [sp, #26*4] // p0
    ldr.w r7, [sp, #33*4]
    ldr.w r8, [sp, #41*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[2]
    sub.w tmp1, tmp1, r8    // cross[10]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[2]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[10]
    strh.w tmp2, [\addr_r, #2*2]
    strh.w tmp3, [\addr_r, #10*2]

    /* r[3] r[11] */
    ldr.w tmp0, [sp, #49*4]
    ldr.w tmp1, [sp, #57*4] // pm
    ldr.w tmp2, [sp, #19*4]
    ldr.w tmp3, [sp, #27*4] // p0
    ldr.w r7, [sp, #34*4]
    ldr.w r8, [sp, #42*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[3]
    sub.w tmp1, tmp1, r8    // cross[11]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[3]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[11]
    strh.w tmp2, [\addr_r, #3*2]
    strh.w tmp3, [\addr_r, #11*2]

    /* r[4] r[12] */
    ldr.w tmp0, [sp, #50*4]
    ldr.w tmp1, [sp, #58*4] // pm
    ldr.w tmp2, [sp, #20*4]
    ldr.w tmp3, [sp, #28*4] // p0
    ldr.w r7, [sp, #35*4]
    ldr.w r8, [sp, #43*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[4]
    sub.w tmp1, tmp1, r8    // cross[12]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[4]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[12]
    strh.w tmp2, [\addr_r, #4*2]
    strh.w tmp3, [\addr_r, #12*2]

    /* r[5] r[13] */
    ldr.w tmp0, [sp, #51*4]
    ldr.w tmp1, [sp, #59*4] // pm
    ldr.w tmp2, [sp, #21*4]
    ldr.w tmp3, [sp, #29*4] // p0
    ldr.w r7, [sp, #36*4]
    ldr.w r8, [sp, #44*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[5]
    sub.w tmp1, tmp1, r8    // cross[13]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[5]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[13]
    strh.w tmp2, [\addr_r, #5*2]
    strh.w tmp3, [\addr_r, #13*2]

    /* r[6] r[14] */
    ldr.w tmp0, [sp, #52*4]
    ldr.w tmp1, [sp, #60*4] // pm
    ldr.w tmp2, [sp, #22*4]
    ldr.w tmp3, [sp, #30*4] // p0
    ldr.w r7, [sp, #37*4]
    ldr.w r8, [sp, #45*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[6]
    sub.w tmp1, tmp1, r8    // cross[14]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[6]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[4]
    strh.w tmp2, [\addr_r, #6*2]
    strh.w tmp3, [\addr_r, #14*2]

    /* r[7] r[15] */
    ldr.w tmp0, [sp, #53*4] // pm
    ldr.w tmp2, [sp, #23*4] // p0
    ldr.w r7, [sp, #38*4]   // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp0, tmp0, r7    // cross[7]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[7]
    fqmul_asm tmp3, tmp0, mont, qinv, q, r10  // tmp3 -> r[15]
    strh.w tmp2, [\addr_r, #7*2]
    strh.w tmp3, [\addr_r, #15*2]

    /* r[0] r[8] */
    ldr.w tmp0, [sp, #46*4]
    ldr.w tmp1, [sp, #54*4] // pm
    ldr.w tmp2, [sp, #16*4]
    ldr.w tmp3, [sp, #24*4] // p0
    ldr.w r7, [sp, #31*4]
    ldr.w r8, [sp, #39*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[0]
    sub.w tmp1, tmp1, r8    // cross[8]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[0]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[8]
    strh.w tmp3, [\addr_r, #8*2]
    strh.w tmp2, [\addr_r], #16*2

    ldr.w \addr_a, [sp, #62*4]
    ldr.w \addr_b, [sp, #63*4]
    add.w \addr_a, \addr_a, #16*2
    add.w \addr_b, \addr_b, #16*2

    add.w sp, sp, #96*4
    pop {r0-r2,r7-r10,r12}
.endm

//void poly_basemul_asm(int16_t *r,  int16_t *a,  int16_t *b, const int16_t f[128]);
.global poly_basemul_asm
.type poly_basemul_asm, %function
.align 2
poly_basemul_asm:

addr_r .req r3
addr_a .req r14
addr_b .req r11
tmp0 .req r0
tmp1 .req r1
tmp2 .req r2
tmp3 .req r12
addr_zeta .req r3
zeta .req r4
qinv .req r5
q .req r6
k4_a0 .req r7
k4_a1 .req r8
k4_b0 .req r9
k4_b1 .req r10
k4_a2 .req r9
k4_a3 .req r10
k4_b2 .req r7
k4_b3 .req r8
mont .req r11
loop .req r14
k8_a0 .req r7
k8_a1 .req r8
k8_a2 .req r9
k8_a3 .req r10
k8_b0 .req r7
k8_b1 .req r8
k8_b2 .req r9
k8_b3 .req r10

    push {r4-r11, lr}
    sub.w sp, sp, #5*4

    add.w r3, r3, #128
    str.w r3, [sp, #1*4]
    str.w r0, [sp, #2*4]
    str.w r1, [sp, #3*4]
    str.w r2, [sp, #4*4]

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    mov.w loop, #64
    str.w loop, [sp]
    basemul_lp:
        ldr.w addr_zeta, [sp, #1*4]
        ldrsh.w zeta, [addr_zeta], #2
        str.w addr_zeta, [sp, #1*4]

        ldr.w addr_r, [sp, #2*4]
        ldr.w addr_a, [sp, #3*4]
        ldr.w addr_b, [sp, #4*4]

        base_mul addr_r, addr_a, addr_b, zeta
        rsb.w zeta, zeta, #0
        base_mul addr_r, addr_a, addr_b, zeta

        str.w addr_r, [sp, #2*4]
        str.w addr_a, [sp, #3*4]
        str.w addr_b, [sp, #4*4]

        ldr.w loop, [sp]
        subs.w loop, loop, #1
        str.w loop, [sp]
        bne.w basemul_lp

.unreq addr_r
.unreq addr_a
.unreq addr_b
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq tmp3
.unreq addr_zeta
.unreq zeta
.unreq qinv
.unreq q
.unreq k4_a0
.unreq k4_a1
.unreq k4_b0
.unreq k4_b1
.unreq k4_a2
.unreq k4_a3
.unreq k4_b2
.unreq k4_b3
.unreq mont
.unreq loop
.unreq k8_a0
.unreq k8_a1
.unreq k8_a2
.unreq k8_a3
.unreq k8_b0
.unreq k8_b1
.unreq k8_b2
.unreq k8_b3

    add.w sp, sp, #5*4
    pop {r4-r11, pc}

.size poly_basemul_asm, .-poly_basemul_asm

.macro mq_finish re, tmp
    mov.w \tmp, #19
    fqmul_asm \re, \re, \tmp, qinv, q, \tmp
    asr.w \tmp, \re, #15
    and.w \tmp, \tmp, q
    add.w \re, \re, \tmp
.endm

.macro base_mul_mq addr_r, addr_a, addr_b, zeta
    push {r0-r2,r7-r10,r12}
    sub.w sp, sp, #96*4

    str.w \addr_r, [sp, #61*4]
    str.w \addr_a, [sp, #62*4]
    str.w \addr_b, [sp, #63*4]

    /* as[0] == sp
     * bs[0] == sp + 8*4
     * p0[0] == sp + 16*4
     * p1[0] == sp + 31*4
     * pm[0] == sp + 46*4
     */

     /* a[0] == sp + 64*4
      * b[0] == sp + 80*4
      */

    /* as bs */
    ldrsh.w r7, [\addr_a]
    ldrsh.w r8, [\addr_a, #1*2]
    ldrsh.w r9, [\addr_a, #2*2]
    ldrsh.w r10, [\addr_a, #3*2]
    ldrsh.w tmp0, [\addr_a, #8*2]
    ldrsh.w tmp1, [\addr_a, #9*2]
    ldrsh.w tmp2, [\addr_a, #10*2]
    ldrsh.w tmp3, [\addr_a, #11*2]
    str.w r7, [sp, #64*4]
    str.w r8, [sp, #65*4]
    str.w r9, [sp, #66*4]
    str.w r10, [sp, #67*4]
    str.w tmp0, [sp, #72*4]
    str.w tmp1, [sp, #73*4]
    str.w tmp2, [sp, #74*4]
    str.w tmp3, [sp, #75*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp]
    str.w tmp1, [sp, #1*4]
    str.w tmp2, [sp, #2*4]
    str.w tmp3, [sp, #3*4]

    ldrsh.w r7, [\addr_a, #4*2]
    ldrsh.w r8, [\addr_a, #5*2]
    ldrsh.w r9, [\addr_a, #6*2]
    ldrsh.w r10, [\addr_a, #7*2]
    ldrsh.w tmp0, [\addr_a, #12*2]
    ldrsh.w tmp1, [\addr_a, #13*2]
    ldrsh.w tmp2, [\addr_a, #14*2]
    ldrsh.w tmp3, [\addr_a, #15*2]
    str.w r7, [sp, #68*4]
    str.w r8, [sp, #69*4]
    str.w r9, [sp, #70*4]
    str.w r10, [sp, #71*4]
    str.w tmp0, [sp, #76*4]
    str.w tmp1, [sp, #77*4]
    str.w tmp2, [sp, #78*4]
    str.w tmp3, [sp, #79*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp, #4*4]
    str.w tmp1, [sp, #5*4]
    str.w tmp2, [sp, #6*4]
    str.w tmp3, [sp, #7*4]

    ldrsh.w r7, [\addr_b]
    ldrsh.w r8, [\addr_b, #1*2]
    ldrsh.w r9, [\addr_b, #2*2]
    ldrsh.w r10, [\addr_b, #3*2]
    ldrsh.w tmp0, [\addr_b, #8*2]
    ldrsh.w tmp1, [\addr_b, #9*2]
    ldrsh.w tmp2, [\addr_b, #10*2]
    ldrsh.w tmp3, [\addr_b, #11*2]
    str.w r7, [sp, #80*4]
    str.w r8, [sp, #81*4]
    str.w r9, [sp, #82*4]
    str.w r10, [sp, #83*4]
    str.w tmp0, [sp, #88*4]
    str.w tmp1, [sp, #89*4]
    str.w tmp2, [sp, #90*4]
    str.w tmp3, [sp, #91*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp, #8*4]
    str.w tmp1, [sp, #9*4]
    str.w tmp2, [sp, #10*4]
    str.w tmp3, [sp, #11*4]

    ldrsh.w r7, [\addr_b, #4*2]
    ldrsh.w r8, [\addr_b, #5*2]
    ldrsh.w r9, [\addr_b, #6*2]
    ldrsh.w r10, [\addr_b, #7*2]
    ldrsh.w tmp0, [\addr_b, #12*2]
    ldrsh.w tmp1, [\addr_b, #13*2]
    ldrsh.w tmp2, [\addr_b, #14*2]
    ldrsh.w tmp3, [\addr_b, #15*2]
    str.w r7, [sp, #84*4]
    str.w r8, [sp, #85*4]
    str.w r9, [sp, #86*4]
    str.w r10, [sp, #87*4]
    str.w tmp0, [sp, #92*4]
    str.w tmp1, [sp, #93*4]
    str.w tmp2, [sp, #94*4]
    str.w tmp3, [sp, #95*4]
    add.w tmp0, tmp0, r7
    add.w tmp1, tmp1, r8
    add.w tmp2, tmp2, r9
    add.w tmp3, tmp3, r10
    str.w tmp0, [sp, #12*4]
    str.w tmp1, [sp, #13*4]
    str.w tmp2, [sp, #14*4]
    str.w tmp3, [sp, #15*4]

    /* karatsuba_mul8 */
    add.w \addr_r, sp, #16*4
    add.w \addr_a, sp, #64*4
    add.w \addr_b, sp, #80*4
    karatsuba_mul8 \addr_r, \addr_a, \addr_b

    add.w \addr_r, sp, #31*4
    add.w \addr_a, \addr_a, #8*4
    add.w \addr_b, \addr_b, #8*4
    karatsuba_mul8 \addr_r, \addr_a, \addr_b

    add.w \addr_r, sp, #46*4
    add.w \addr_a, sp, #0
    add.w \addr_b, sp, #8*4
    karatsuba_mul8 \addr_r, \addr_a, \addr_b

    /* as[0] == sp
     * bs[0] == sp + 8*4
     * p0[0] == sp + 16*4
     * p1[0] == sp + 31*4
     * pm[0] == sp + 46*4
     */

    ldr.w \addr_r, [sp, #61*4]
    mov.w mont, #171
    /* r[1] r[9] */
    ldr.w tmp0, [sp, #47*4]
    ldr.w tmp1, [sp, #55*4] // pm
    ldr.w tmp2, [sp, #17*4]
    ldr.w tmp3, [sp, #25*4] // p0
    ldr.w r7, [sp, #32*4]
    ldr.w r8, [sp, #40*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[1]
    sub.w tmp1, tmp1, r8    // cross[9]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[1]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[9]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp2, [\addr_r, #1*2]
    strh.w tmp3, [\addr_r, #9*2]

    /* r[2] r[10] */
    ldr.w tmp0, [sp, #48*4]
    ldr.w tmp1, [sp, #56*4] // pm
    ldr.w tmp2, [sp, #18*4]
    ldr.w tmp3, [sp, #26*4] // p0
    ldr.w r7, [sp, #33*4]
    ldr.w r8, [sp, #41*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[2]
    sub.w tmp1, tmp1, r8    // cross[10]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[2]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[10]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp2, [\addr_r, #2*2]
    strh.w tmp3, [\addr_r, #10*2]

    /* r[3] r[11] */
    ldr.w tmp0, [sp, #49*4]
    ldr.w tmp1, [sp, #57*4] // pm
    ldr.w tmp2, [sp, #19*4]
    ldr.w tmp3, [sp, #27*4] // p0
    ldr.w r7, [sp, #34*4]
    ldr.w r8, [sp, #42*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[3]
    sub.w tmp1, tmp1, r8    // cross[11]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[3]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[11]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp2, [\addr_r, #3*2]
    strh.w tmp3, [\addr_r, #11*2]

    /* r[4] r[12] */
    ldr.w tmp0, [sp, #50*4]
    ldr.w tmp1, [sp, #58*4] // pm
    ldr.w tmp2, [sp, #20*4]
    ldr.w tmp3, [sp, #28*4] // p0
    ldr.w r7, [sp, #35*4]
    ldr.w r8, [sp, #43*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[4]
    sub.w tmp1, tmp1, r8    // cross[12]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[4]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[12]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp2, [\addr_r, #4*2]
    strh.w tmp3, [\addr_r, #12*2]

    /* r[5] r[13] */
    ldr.w tmp0, [sp, #51*4]
    ldr.w tmp1, [sp, #59*4] // pm
    ldr.w tmp2, [sp, #21*4]
    ldr.w tmp3, [sp, #29*4] // p0
    ldr.w r7, [sp, #36*4]
    ldr.w r8, [sp, #44*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[5]
    sub.w tmp1, tmp1, r8    // cross[13]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[5]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[13]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp2, [\addr_r, #5*2]
    strh.w tmp3, [\addr_r, #13*2]

    /* r[6] r[14] */
    ldr.w tmp0, [sp, #52*4]
    ldr.w tmp1, [sp, #60*4] // pm
    ldr.w tmp2, [sp, #22*4]
    ldr.w tmp3, [sp, #30*4] // p0
    ldr.w r7, [sp, #37*4]
    ldr.w r8, [sp, #45*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[6]
    sub.w tmp1, tmp1, r8    // cross[14]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[6]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[4]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp2, [\addr_r, #6*2]
    strh.w tmp3, [\addr_r, #14*2]

    /* r[7] r[15] */
    ldr.w tmp0, [sp, #53*4] // pm
    ldr.w tmp2, [sp, #23*4] // p0
    ldr.w r7, [sp, #38*4]   // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp0, tmp0, r7    // cross[7]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[7]
    fqmul_asm tmp3, tmp0, mont, qinv, q, r10  // tmp3 -> r[15]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp2, [\addr_r, #7*2]
    strh.w tmp3, [\addr_r, #15*2]

    /* r[0] r[8] */
    ldr.w tmp0, [sp, #46*4]
    ldr.w tmp1, [sp, #54*4] // pm
    ldr.w tmp2, [sp, #16*4]
    ldr.w tmp3, [sp, #24*4] // p0
    ldr.w r7, [sp, #31*4]
    ldr.w r8, [sp, #39*4] // p1

    sub.w tmp0, tmp0, tmp2
    sub.w tmp1, tmp1, tmp3
    sub.w tmp0, tmp0, r7    // cross[0]
    sub.w tmp1, tmp1, r8    // cross[8]

    fqmul_asm r7, r7, \zeta, qinv, q, r9
    fqmul_asm r8, r8, \zeta, qinv, q, r10
    fqmul_asm tmp1, tmp1, \zeta, qinv, q, r9
    add.w tmp2, tmp2, r7
    add.w tmp3, tmp3, r8
    add.w tmp2, tmp2, tmp1
    add.w tmp3, tmp3, tmp0
    fqmul_asm tmp2, tmp2, mont, qinv, q, r9   // tmp2 -> r[0]
    fqmul_asm tmp3, tmp3, mont, qinv, q, r10  // tmp3 -> r[8]
    mq_finish tmp2, r9
    mq_finish tmp3, r10
    strh.w tmp3, [\addr_r, #8*2]
    strh.w tmp2, [\addr_r], #16*2

    ldr.w \addr_a, [sp, #62*4]
    ldr.w \addr_b, [sp, #63*4]
    add.w \addr_a, \addr_a, #16*2
    add.w \addr_b, \addr_b, #16*2

    add.w sp, sp, #96*4
    pop {r0-r2,r7-r10,r12}
.endm

//void poly_basemul_mq_asm(int16_t *r,  int16_t *a,  int16_t *b, const int16_t f[128]);
.global poly_basemul_mq_asm
.type poly_basemul_mq_asm, %function
.align 2
poly_basemul_mq_asm:

addr_r .req r3
addr_a .req r14
addr_b .req r11
tmp0 .req r0
tmp1 .req r1
tmp2 .req r2
tmp3 .req r12
addr_zeta .req r3
zeta .req r4
qinv .req r5
q .req r6
k4_a0 .req r7
k4_a1 .req r8
k4_b0 .req r9
k4_b1 .req r10
k4_a2 .req r9
k4_a3 .req r10
k4_b2 .req r7
k4_b3 .req r8
mont .req r11
loop .req r14
k8_a0 .req r7
k8_a1 .req r8
k8_a2 .req r9
k8_a3 .req r10
k8_b0 .req r7
k8_b1 .req r8
k8_b2 .req r9
k8_b3 .req r10

    push {r4-r11, lr}
    sub.w sp, sp, #5*4

    add.w r3, r3, #128
    str.w r3, [sp, #1*4]
    str.w r0, [sp, #2*4]
    str.w r1, [sp, #3*4]
    str.w r2, [sp, #4*4]

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    mov.w loop, #64
    str.w loop, [sp]
    basemul_mq_lp:
        ldr.w addr_zeta, [sp, #1*4]
        ldrsh.w zeta, [addr_zeta], #2
        str.w addr_zeta, [sp, #1*4]

        ldr.w addr_r, [sp, #2*4]
        ldr.w addr_a, [sp, #3*4]
        ldr.w addr_b, [sp, #4*4]

        base_mul_mq addr_r, addr_a, addr_b, zeta
        rsb.w zeta, zeta, #0
        base_mul_mq addr_r, addr_a, addr_b, zeta

        str.w addr_r, [sp, #2*4]
        str.w addr_a, [sp, #3*4]
        str.w addr_b, [sp, #4*4]

        ldr.w loop, [sp]
        subs.w loop, loop, #1
        str.w loop, [sp]
        bne.w basemul_mq_lp

.unreq addr_r
.unreq addr_a
.unreq addr_b
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq tmp3
.unreq addr_zeta
.unreq zeta
.unreq qinv
.unreq q
.unreq k4_a0
.unreq k4_a1
.unreq k4_b0
.unreq k4_b1
.unreq k4_a2
.unreq k4_a3
.unreq k4_b2
.unreq k4_b3
.unreq mont
.unreq loop
.unreq k8_a0
.unreq k8_a1
.unreq k8_a2
.unreq k8_a3
.unreq k8_b0
.unreq k8_b1
.unreq k8_b2
.unreq k8_b3

    add.w sp, sp, #5*4
    pop {r4-r11, pc}

.size poly_basemul_mq_asm, .-poly_basemul_mq_asm

.macro base_inv addr_r, addr_a, zeta
    sub.w sp, sp, #57*4

    str.w addr_r, [sp]
    str.w addr_a, [sp, #1*4]
    str.w qinv_array, [sp, #2*4]

    /* pe15[0] == sp + 3*4
     * po15[0] == sp + 18*4
     * ae[0] == sp + 33*4
     * ao[0] == sp + 41*4
     */

    ldrsh.w t_0, [\addr_a]
    ldrsh.w t_1, [\addr_a, #2*2]
    ldrsh.w t_2, [\addr_a, #4*2]
    ldrsh.w t_3, [\addr_a, #6*2]
    ldrsh.w t_4, [\addr_a, #8*2]
    ldrsh.w t_5, [\addr_a, #10*2]
    ldrsh.w t_6, [\addr_a, #12*2]
    ldrsh.w t_7, [\addr_a, #14*2]
    str.w t_0, [sp, #33*4]
    str.w t_1, [sp, #34*4]
    str.w t_2, [sp, #35*4]
    str.w t_3, [sp, #36*4]
    str.w t_4, [sp, #37*4]
    str.w t_5, [sp, #38*4]
    str.w t_6, [sp, #39*4]
    str.w t_7, [sp, #40*4]

    ldrsh.w t_0, [\addr_a, #1*2]
    ldrsh.w t_1, [\addr_a, #3*2]
    ldrsh.w t_2, [\addr_a, #5*2]
    ldrsh.w t_3, [\addr_a, #7*2]
    ldrsh.w t_4, [\addr_a, #9*2]
    ldrsh.w t_5, [\addr_a, #11*2]
    ldrsh.w t_6, [\addr_a, #13*2]
    ldrsh.w t_7, [\addr_a, #15*2]
    str.w t_0, [sp, #41*4]
    str.w t_1, [sp, #42*4]
    str.w t_2, [sp, #43*4]
    str.w t_3, [sp, #44*4]
    str.w t_4, [sp, #45*4]
    str.w t_5, [sp, #46*4]
    str.w t_6, [sp, #47*4]
    str.w t_7, [sp, #48*4]

    add.w \addr_r, sp, #3*4
    add.w \addr_a, sp, #33*4
    add.w addr_b, sp, #33*4
    karatsuba_mul8 \addr_r, \addr_a, addr_b

    add.w \addr_r, sp, #18*4
    add.w \addr_a, sp, #41*4
    add.w addr_b, sp, #41*4
    karatsuba_mul8 \addr_r, \addr_a, addr_b

    /* pe15[0] == sp + 3*4
     * po15[0] == sp + 18*4
     * ae[0] == sp + 33*4
     * ao[0] == sp + 41*4
     * he[0] == sp + 49*4
     * ho[0] == sp + 53*4
     */

    /* h[0] - h[3] */
    ldr.w t_0, [sp, #11*4]
    ldr.w t_1, [sp, #12*4]
    ldr.w t_2, [sp, #13*4]
    ldr.w t_3, [sp, #14*4]
    ldr.w t_4, [sp, #25*4]
    ldr.w t_5, [sp, #26*4]
    ldr.w t_6, [sp, #27*4]
    ldr.w t_7, [sp, #28*4]

    sub.w t_0, t_0, t_4
    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6
    sub.w t_3, t_3, t_7
    fqmul_asm t_0, t_0, \zeta, qinv, q, t_4
    fqmul_asm t_1, t_1, \zeta, qinv, q, t_5
    fqmul_asm t_2, t_2, \zeta, qinv, q, t_6
    fqmul_asm t_3, t_3, \zeta, qinv, q, t_7

    ldr.w t_4, [sp, #3*4]
    ldr.w t_5, [sp, #4*4]
    ldr.w t_6, [sp, #5*4]
    ldr.w t_7, [sp, #6*4]
    sub.w t_0, t_4, t_0
    sub.w t_1, t_5, t_1
    sub.w t_2, t_6, t_2
    sub.w t_3, t_7, t_3

    ldr.w t_5, [sp, #18*4]
    ldr.w t_6, [sp, #19*4]
    ldr.w t_7, [sp, #20*4]

    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6
    sub.w t_3, t_3, t_7

    str.w t_0, [sp, #49*4]
    str.w t_1, [sp, #53*4]
    str.w t_2, [sp, #50*4]
    str.w t_3, [sp, #54*4]

    /* h[4] - h[7] */
    ldr.w t_0, [sp, #15*4]
    ldr.w t_1, [sp, #16*4]
    ldr.w t_2, [sp, #17*4]
    ldr.w t_3, [sp, #32*4]
    ldr.w t_4, [sp, #29*4]
    ldr.w t_5, [sp, #30*4]
    ldr.w t_6, [sp, #31*4]


    sub.w t_0, t_0, t_4
    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6

    fqmul_asm t_0, t_0, \zeta, qinv, q, t_4
    fqmul_asm t_1, t_1, \zeta, qinv, q, t_5
    fqmul_asm t_2, t_2, \zeta, qinv, q, t_6
    fqmul_asm t_3, t_3, \zeta, qinv, q, t_7

    ldr.w t_4, [sp, #7*4]
    ldr.w t_5, [sp, #8*4]
    ldr.w t_6, [sp, #9*4]
    ldr.w t_7, [sp, #10*4]
    sub.w t_0, t_4, t_0
    sub.w t_1, t_5, t_1
    sub.w t_2, t_6, t_2
    add.w t_3, t_7, t_3

    ldr.w t_4, [sp, #21*4]
    ldr.w t_5, [sp, #22*4]
    ldr.w t_6, [sp, #23*4]
    ldr.w t_7, [sp, #24*4]

    sub.w t_0, t_0, t_4
    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6
    sub.w t_3, t_3, t_7

    str.w t_0, [sp, #51*4]
    str.w t_1, [sp, #55*4]
    str.w t_2, [sp, #52*4]
    str.w t_3, [sp, #56*4]

    /* pe15[0] == sp + 3*4
     * po15[0] == sp + 18*4
     * ae[0] == sp + 33*4
     * ao[0] == sp + 41*4
     * he[0] == sp + 49*4
     * ho[0] == sp + 53*4
     * pe7[0] == sp + 3*4
     * po7[0] == sp + 10*4
     * b[0] == sp + 17*4
     */

    add.w \addr_r, sp, #3*4
    add.w \addr_a, sp, #49*4
    add.w addr_b, sp, #49*4
    karatsuba_mul4 \addr_r, \addr_a, addr_b

    add.w \addr_r, sp, #10*4
    add.w \addr_a, sp, #53*4
    add.w addr_b, sp, #53*4
    karatsuba_mul4 \addr_r, \addr_a, addr_b

    ldr.w t_0, [sp, #7*4]
    ldr.w t_1, [sp, #8*4]
    ldr.w t_2, [sp, #9*4]
    ldr.w t_3, [sp, #16*4]
    ldr.w t_4, [sp, #13*4]
    ldr.w t_5, [sp, #14*4]
    ldr.w t_6, [sp, #15*4]

    sub.w t_0, t_0, t_4
    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6

    fqmul_asm t_0, t_0, \zeta, qinv, q, t_4
    fqmul_asm t_1, t_1, \zeta, qinv, q, t_5
    fqmul_asm t_2, t_2, \zeta, qinv, q, t_6
    fqmul_asm t_3, t_3, \zeta, qinv, q, t_7

    ldr.w t_4, [sp, #3*4]
    ldr.w t_5, [sp, #4*4]
    ldr.w t_6, [sp, #5*4]
    ldr.w t_7, [sp, #6*4]

    sub.w t_0, t_4, t_0
    sub.w t_1, t_5, t_1
    sub.w t_2, t_6, t_2
    add.w t_3, t_7, t_3

    ldr.w t_5, [sp, #10*4]
    ldr.w t_6, [sp, #11*4]
    ldr.w t_7, [sp, #12*4]

    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6
    sub.w t_3, t_3, t_7

    str.w t_0, [sp, #17*4]
    str.w t_1, [sp, #18*4]
    str.w t_2, [sp, #19*4]
    str.w t_3, [sp, #20*4]  // b[0] - b[3]

    fqmul_asm t_4, t_0, t_0, qinv, q, t_6  // q0e
    fqmul_asm t_5, t_2, t_2, qinv, q, t_7  // q2e
    fqmul_asm t_6, t_0, t_2, qinv, q, t_7
    mov.w t_7, #342
    fqmul_asm t_6, t_6, t_7, qinv, q, t_7  // q1e

    fqmul_asm t_0, t_1, t_1, qinv, q, t_7  // q0o
    fqmul_asm t_2, t_3, t_3, qinv, q, t_7  // q2o
    fqmul_asm t_1, t_1, t_3, qinv, q, t_7
    mov.w t_7, #342
    fqmul_asm t_1, t_1, t_7, qinv, q, t_7  // q1o

    sub.w t_5, t_5, t_1
    fqmul_asm t_2, t_2, \zeta, qinv, q, t_7
    fqmul_asm t_5, t_5, \zeta, qinv, q, t_7
    sub.w t_0, t_0, t_2
    sub.w t_4, t_4, t_5  // c0
    sub.w t_6, t_6, t_0  // c1

    fqmul_asm t_0, t_6, t_6, qinv, q, t_7
    fqmul_asm t_0, t_0, \zeta, qinv, q, t_7  // e
    fqmul_asm t_1, t_4, t_4, qinv, q, t_7  // t
    add.w t_0, t_0, t_1
    and.w t_2, q, t_0, asr #15
    add.w t_0, t_0, t_2  // e
    lsl.w t_0, t_0, #1   // e
    ldr.w r0, [sp, #2*4]
    ldrsh.w t_0, [r0, t_0] // t_0 == qinv[e]

    fqmul_asm t_4, t_0, t_4, qinv, q, t_7 // c0
    fqmul_asm t_6, t_0, t_6, qinv, q, t_7
    mov.w t_7, #171
    rsb.w t_7, t_7, #0
    fqmul_asm t_6, t_6, t_7, qinv, q, t_7 // c1

    /* pe15[0] == sp + 3*4
     * po15[0] == sp + 18*4
     * ae[0] == sp + 33*4
     * ao[0] == sp + 41*4
     * he[0] == sp + 49*4
     * ho[0] == sp + 53*4
     * pe7[0] == sp + 3*4
     * po7[0] == sp + 10*4
     * b[0] == sp + 17*4
     * f[0] == sp + 17*4
     */
    ldr.w t_0, [sp, #17*4]
    ldr.w t_1, [sp, #18*4]
    ldr.w t_2, [sp, #19*4]
    ldr.w t_3, [sp, #20*4]  // b[0] - b[3]

    fqmul_asm t_5, t_6, t_2, qinv, q, t_7
    fqmul_asm t_5, t_5, \zeta, qinv, q, t_7 // f0
    fqmul_asm r0, t_4, t_0, qinv, q, t_7  // t
    sub.w t_5, r0, t_5                      // f0

    fqmul_asm r0, t_4, t_2, qinv, q, t_7
    fqmul_asm r1, t_6, t_0, qinv, q, t_7
    add.w t_0, r0, r1                       // f2

    fqmul_asm t_2, t_6, t_3, qinv, q, t_7
    fqmul_asm t_2, t_2, \zeta, qinv, q, t_7 // f0
    fqmul_asm r0, t_4, t_1, qinv, q, t_7  // t
    sub.w t_2, t_2, r0                      // f1

    fqmul_asm r0, t_4, t_3, qinv, q, t_7
    fqmul_asm r1, t_6, t_1, qinv, q, t_7
    add.w t_7, r0, r1
    mov.w t_4, #171
    rsb.w t_4, t_4, #0
    fqmul_asm t_7, t_7, t_4, qinv, q, t_4    // f3

    str.w t_5, [sp, #17*4]
    str.w t_2, [sp, #18*4]
    str.w t_0, [sp, #19*4]
    str.w t_7, [sp, #20*4]  // f[0] - f[3]

    add.w \addr_r, sp, #3*4
    add.w \addr_a, sp, #17*4
    add.w addr_b, sp, #49*4
    karatsuba_mul4 \addr_r, \addr_a, addr_b

    add.w \addr_r, sp, #10*4
    add.w \addr_a, sp, #17*4
    add.w addr_b, sp, #53*4
    karatsuba_mul4 \addr_r, \addr_a, addr_b

    /* pe15[0] == sp + 3*4
     * po15[0] == sp + 18*4
     * ae[0] == sp + 33*4
     * ao[0] == sp + 41*4
     * he[0] == sp + 49*4
     * ho[0] == sp + 53*4
     * pe7[0] == sp + 3*4
     * po7[0] == sp + 10*4
     * b[0] == sp + 17*4
     * f[0] == sp + 17*4
     * g == sp + 49*4
     */

    ldr.w t_0, [sp, #7*4]
    ldr.w t_1, [sp, #14*4]
    ldr.w t_2, [sp, #8*4]
    ldr.w t_3, [sp, #15*4]

    ldr.w t_4, [sp, #3*4]
    ldr.w t_5, [sp, #10*4]
    ldr.w t_6, [sp, #4*4]
    ldr.w t_7, [sp, #11*4]

    fqmul_asm t_0, t_0, \zeta, qinv, q, r0
    fqmul_asm t_1, t_1, \zeta, qinv, q, r0
    fqmul_asm t_2, t_2, \zeta, qinv, q, r0
    fqmul_asm t_3, t_3, \zeta, qinv, q, r0

    sub.w t_0, t_4, t_0
    sub.w t_1, t_1, t_5
    sub.w t_2, t_6, t_2
    sub.w t_3, t_3, t_7
    str.w t_0, [sp, #49*4]
    str.w t_1, [sp, #50*4]
    str.w t_2, [sp, #51*4]
    str.w t_3, [sp, #52*4]

    ldr.w t_0, [sp, #9*4]
    ldr.w t_1, [sp, #16*4]
    ldr.w t_2, [sp, #6*4]
    ldr.w t_3, [sp, #13*4]

    ldr.w t_4, [sp, #5*4]
    ldr.w t_5, [sp, #12*4]
    mov.w t_7, #171
    rsb.w t_7, t_7, #0

    fqmul_asm t_0, t_0, \zeta, qinv, q, r0
    fqmul_asm t_1, t_1, \zeta, qinv, q, r0
    fqmul_asm t_3, t_3, t_7, qinv, q, r0

    sub.w t_0, t_4, t_0
    sub.w t_1, t_1, t_5

    str.w t_0, [sp, #53*4]
    str.w t_1, [sp, #54*4]
    str.w t_2, [sp, #55*4]
    str.w t_3, [sp, #56*4]

    add.w \addr_r, sp, #3*4
    add.w \addr_a, sp, #49*4
    add.w addr_b, sp, #33*4
    karatsuba_mul8 \addr_r, \addr_a, addr_b

    add.w \addr_r, sp, #18*4
    add.w \addr_a, sp, #49*4
    add.w addr_b, sp, #41*4
    karatsuba_mul8 \addr_r, \addr_a, addr_b

    /* pe15[0] == sp + 3*4
     * po15[0] == sp + 18*4
     */

    ldr.w \addr_r, [sp]
    ldr.w \addr_a, [sp, #1*4]
    add.w \addr_a, \addr_a, #16*2

    /* r[8] r[10] r[12] r[14] */
    ldr.w t_0, [sp, #7*4]
    ldr.w t_1, [sp, #8*4]
    ldr.w t_2, [sp, #9*4]
    ldr.w t_3, [sp, #10*4]
    ldr.w t_4, [sp, #15*4]
    ldr.w t_5, [sp, #16*4]
    ldr.w t_6, [sp, #17*4]

    fqmul_asm t_4, t_4, \zeta, qinv, q, r2
    fqmul_asm t_5, t_5, \zeta, qinv, q, r2
    fqmul_asm t_6, t_6, \zeta, qinv, q, r2

    sub.w t_0, t_0, t_4
    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6

    strh.w t_0, [\addr_r, #8*2]
    strh.w t_1, [\addr_r, #10*2]
    strh.w t_2, [\addr_r, #12*2]
    strh.w t_3, [\addr_r, #14*2]

    /* r[1] r[3] r[5] r[7] */
    ldr.w t_0, [sp, #18*4]
    ldr.w t_1, [sp, #19*4]
    ldr.w t_2, [sp, #20*4]
    ldr.w t_3, [sp, #21*4]
    ldr.w t_4, [sp, #26*4]
    ldr.w t_5, [sp, #27*4]
    ldr.w t_6, [sp, #28*4]
    ldr.w t_7, [sp, #29*4]

    fqmul_asm t_4, t_4, \zeta, qinv, q, r2
    fqmul_asm t_5, t_5, \zeta, qinv, q, r2
    fqmul_asm t_6, t_6, \zeta, qinv, q, r2
    fqmul_asm t_7, t_7, \zeta, qinv, q, r2

    sub.w t_0, t_4, t_0
    sub.w t_1, t_5, t_1
    sub.w t_2, t_6, t_2
    sub.w t_3, t_7, t_3
    strh.w t_0, [\addr_r, #1*2]
    strh.w t_1, [\addr_r, #3*2]
    strh.w t_2, [\addr_r, #5*2]
    strh.w t_3, [\addr_r, #7*2]

    /* r[9] r[11] r[13] r[15] */
    ldr.w t_0, [sp, #22*4]
    ldr.w t_1, [sp, #23*4]
    ldr.w t_2, [sp, #24*4]
    ldr.w t_3, [sp, #25*4]
    ldr.w t_4, [sp, #30*4]
    ldr.w t_5, [sp, #31*4]
    ldr.w t_6, [sp, #32*4]

    mov.w t_7, #171
    rsb.w t_7, t_7, #0

    fqmul_asm t_4, t_4, \zeta, qinv, q, r2
    fqmul_asm t_5, t_5, \zeta, qinv, q, r2
    fqmul_asm t_6, t_6, \zeta, qinv, q, r2
    fqmul_asm t_3, t_3, t_7, qinv, q, r2

    sub.w t_0, t_4, t_0
    sub.w t_1, t_5, t_1
    sub.w t_2, t_6, t_2

    strh.w t_0, [\addr_r, #9*2]
    strh.w t_1, [\addr_r, #11*2]
    strh.w t_2, [\addr_r, #13*2]
    strh.w t_3, [\addr_r, #15*2]

    /* r[0] r[2] r[4] r[6] */
    ldr.w t_0, [sp, #3*4]
    ldr.w t_1, [sp, #4*4]
    ldr.w t_2, [sp, #5*4]
    ldr.w t_3, [sp, #6*4]
    ldr.w t_4, [sp, #11*4]
    ldr.w t_5, [sp, #12*4]
    ldr.w t_6, [sp, #13*4]
    ldr.w t_7, [sp, #14*4]

    fqmul_asm t_4, t_4, \zeta, qinv, q, r2
    fqmul_asm t_5, t_5, \zeta, qinv, q, r2
    fqmul_asm t_6, t_6, \zeta, qinv, q, r2
    fqmul_asm t_7, t_7, \zeta, qinv, q, r2

    sub.w t_0, t_0, t_4
    sub.w t_1, t_1, t_5
    sub.w t_2, t_2, t_6
    sub.w t_3, t_3, t_7
    strh.w t_1, [\addr_r, #2*2]
    strh.w t_2, [\addr_r, #4*2]
    strh.w t_3, [\addr_r, #6*2]
    strh.w t_0, [\addr_r], #16*2

    add.w sp, sp, #57*4
.endm

//void poly_base_inv_asm(int16_t *r,  int16_t *a,  const int16_t f[128], int16_t *qinv);
.global poly_base_inv_asm
.type poly_base_inv_asm, %function
.align 2
poly_base_inv_asm:

addr_r .req r0
addr_a .req r1
addr_b .req r2
addr_zeta .req r2
t_0 .req r3
t_1 .req r4
t_2 .req r5
t_3 .req r6
qinv .req r7
q .req r8
t_4 .req r9
t_5 .req r10
t_6 .req r11
t_7 .req r12
zeta .req r14
loop .req r11
qinv_array .req r3

k4_a0 .req r3
k4_a1 .req r4
k4_b0 .req r5
k4_b1 .req r6
k4_a2 .req r5
k4_a3 .req r6
k4_b2 .req r3
k4_b3 .req r4
k8_a0 .req r3
k8_a1 .req r4
k8_a2 .req r5
k8_a3 .req r6
k8_b0 .req r3
k8_b1 .req r4
k8_b2 .req r5
k8_b3 .req r6
tmp0 .req r9
tmp1 .req r10
tmp2 .req r11
tmp3 .req r12

    push {r4-r11, lr}
    sub.w sp, sp, #3*4

    add.w addr_zeta, addr_zeta, #128
    str.w addr_zeta, [sp, #1*4]
    str.w qinv_array, [sp, #2*4]

    movw qinv, #:lower16:0xfffffd01
    movt qinv, #:upper16:0xfffffd01
    movw q, #769

    mov.w loop, #64
    str.w loop, [sp]
    baseinv_mq_lp:


        ldr.w addr_zeta, [sp, #1*4]
        ldrsh.w zeta, [addr_zeta], #2
        str.w addr_zeta, [sp, #1*4]


        ldr.w qinv_array, [sp, #2*4]
        rsb.w zeta, zeta, #0
        base_inv addr_r, addr_a, zeta

        ldr.w qinv_array, [sp, #2*4]
        rsb.w zeta, zeta, #0
        base_inv addr_r, addr_a, zeta

        ldr.w loop, [sp]
        subs.w loop, loop, #1
        str.w loop, [sp]
        bne.w baseinv_mq_lp



.unreq addr_r
.unreq addr_a
.unreq addr_b
.unreq addr_zeta
.unreq t_0
.unreq t_1
.unreq t_2
.unreq t_3
.unreq qinv
.unreq q
.unreq t_4
.unreq t_5
.unreq t_6
.unreq t_7
.unreq zeta
.unreq loop
.unreq qinv_array
.unreq k4_a0
.unreq k4_a1
.unreq k4_b0
.unreq k4_b1
.unreq k4_a2
.unreq k4_a3
.unreq k4_b2
.unreq k4_b3
.unreq k8_a0
.unreq k8_a1
.unreq k8_a2
.unreq k8_a3
.unreq k8_b0
.unreq k8_b1
.unreq k8_b2
.unreq k8_b3
.unreq tmp0
.unreq tmp1
.unreq tmp2
.unreq tmp3

    add.w sp, sp, #3*4
    pop {r4-r11, pc}
.size poly_base_inv_asm, .-poly_base_inv_asm
