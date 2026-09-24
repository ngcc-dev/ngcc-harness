/* ============================================================
   BiT-512  pointwise ops (n=1024, q=520193) — AVX2 asm
   Complete NTT => plain element-wise Montgomery multiply, signed,
   canonicalized to [0,q) (matches the scalar reference).

   ymm0=q  ymm1=qinv(+)  ymm12..15,5,6=scratch
   ============================================================ */

    .text

/* signed Montgomery multiply of two general vectors:
   ymm12 := montred(ymm\a * ymm\b)  canonical in [0,q).
   q=ymm0, qinv(+)=ymm1 ; scratch 5,6,13,14,15 */
.macro montmul a, b
    vpmuldq   %ymm\b, %ymm\a, %ymm12        /* pe = ae*be */
    vmovshdup %ymm\a, %ymm5
    vmovshdup %ymm\b, %ymm6
    vpmuldq   %ymm6,  %ymm5,  %ymm13        /* po = ao*bo */
    vpmuldq   %ymm1,  %ymm12, %ymm14        /* m_e = lo(pe)*qinv */
    vpmuldq   %ymm1,  %ymm13, %ymm15        /* m_o */
    vpmuldq   %ymm0,  %ymm14, %ymm14        /* m_e*q */
    vpmuldq   %ymm0,  %ymm15, %ymm15        /* m_o*q */
    vmovshdup %ymm12, %ymm12                /* pe hi */
    vmovshdup %ymm13, %ymm13                /* po hi */
    vpblendd  $0xAA,  %ymm13, %ymm12, %ymm12 /* prodhi */
    vmovshdup %ymm14, %ymm14
    vmovshdup %ymm15, %ymm15
    vpblendd  $0xAA,  %ymm15, %ymm14, %ymm14 /* mqhi */
    vpsubd    %ymm14, %ymm12, %ymm12        /* W in (-q,q) */
    vpsrad    $31,    %ymm12, %ymm13        /* -> [0,q) */
    vpand     %ymm0,  %ymm13, %ymm13
    vpaddd    %ymm13, %ymm12, %ymm12
.endm

/* ntt_pointwise_mul_raw(r=rdi, a=rsi, b=rdx) : r = montred(a*b) in [0,q) */
    .global ntt_pointwise_mul_raw
ntt_pointwise_mul_raw:
    vmovdqa   bit512_q(%rip),    %ymm0
    vmovdqa   bit512_qinv(%rip), %ymm1
    mov       $128, %ecx
1:
    vmovdqa   (%rsi), %ymm4
    vmovdqa   (%rdx), %ymm3
    montmul 4, 3
    vmovdqa   %ymm12, (%rdi)
    add       $32, %rsi
    add       $32, %rdx
    add       $32, %rdi
    dec       %ecx
    jnz       1b
    ret

/* ntt_pointwise_acc_raw(r=rdi, a=rsi, b=rdx) : r = (r + montred(a*b)) mod q in [0,q) */
    .global ntt_pointwise_acc_raw
ntt_pointwise_acc_raw:
    vmovdqa   bit512_q(%rip),    %ymm0
    vmovdqa   bit512_qinv(%rip), %ymm1
    mov       $128, %ecx
1:
    vmovdqa   (%rsi), %ymm4
    vmovdqa   (%rdx), %ymm3
    montmul 4, 3
    vpaddd    (%rdi), %ymm12, %ymm12        /* r_old + W   (both [0,q)) */
    vpsubd    %ymm0,  %ymm12, %ymm12        /* -q -> [-q,q) */
    vpsrad    $31,    %ymm12, %ymm13
    vpand     %ymm0,  %ymm13, %ymm13
    vpaddd    %ymm13, %ymm12, %ymm12        /* [0,q) */
    vmovdqa   %ymm12, (%rdi)
    add       $32, %rsi
    add       $32, %rdx
    add       $32, %rdi
    dec       %ecx
    jnz       1b
    ret

/* ntt_montgomery_lift(r=rdi) : r = montred(r * R^2) in [0,q) */
    .global ntt_montgomery_lift
ntt_montgomery_lift:
    vmovdqa   bit512_q(%rip),    %ymm0
    vmovdqa   bit512_qinv(%rip), %ymm1
    vmovdqa   bit512_r2_h(%rip), %ymm3
    mov       $128, %ecx
1:
    vmovdqa   (%rdi), %ymm4
    montmul 4, 3
    vmovdqa   %ymm12, (%rdi)
    add       $32, %rdi
    dec       %ecx
    jnz       1b
    ret

    .section .note.GNU-stack,"",@progbits
