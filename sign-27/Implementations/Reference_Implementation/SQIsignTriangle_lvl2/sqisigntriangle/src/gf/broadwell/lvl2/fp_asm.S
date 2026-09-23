#include <sqisign_namespace.h>
.intel_syntax noprefix

.set pbytes,32
.set plimbs,4

#ifdef __APPLE__
.section __TEXT,__const
#else
.section .rodata
#endif
p_plus_1: .quad 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0120000000000000

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif

#include <asm_preamble.h>

.text
.p2align 4,,15

.global fp_add
fp_add:
  push   r12
  xor    rax, rax
  mov    r8, [rsi]
  mov    r9, [rsi+8]
  mov    r10, [rsi+16]
  mov    r11, [rsi+24]
  mov    r12, [rsi+32]
  add    r8, [rdx]
  adc    r9, [rdx+8]
  adc    r10, [rdx+16]
  adc    r11, [rdx+24]
  adc    r12, [rdx+32]
  mov    rax, r12
  shr    rax, 57
  neg    rax
  mov    rdx, [rip+p+32]
  and    rdx, rax
  sub    r8, rax
  sbb    r9, rax
  sbb    r10, rax
  sbb    r11, rax
  sbb    r12, rdx
  mov    rax, r12
  shr    rax, 57
  neg    rax
  mov    rdx, [rip+p+32]
  and    rdx, rax
  sub    r8, rax
  sbb    r9, rax
  sbb    r10, rax
  sbb    r11, rax
  sbb    r12, rdx
  mov    [rdi], r8
  mov    [rdi+8], r9
  mov    [rdi+16], r10
  mov    [rdi+24], r11
  mov    [rdi+32], r12
  pop    r12
  ret

.global fp_sub
fp_sub:
  push   r12
  xor    rax, rax
  mov    r8, [rsi]
  mov    r9, [rsi+8]
  mov    r10, [rsi+16]
  mov    r11, [rsi+24]
  mov    r12, [rsi+32]
  sub    r8, [rdx]
  sbb    r9, [rdx+8]
  sbb    r10, [rdx+16]
  sbb    r11, [rdx+24]
  sbb    r12, [rdx+32]
  sbb    rax, 0
  mov    rdx, [rip+p+32]
  and    rdx, rax
  add    r8, rax
  adc    r9, rax
  adc    r10, rax
  adc    r11, rax
  adc    r12, rdx
  mov    rax, r12
  sar    rax, 57
  mov    rdx, [rip+p+32]
  and    rdx, rax
  add    r8, rax
  adc    r9, rax
  adc    r10, rax
  adc    r11, rax
  adc    r12, rdx
  mov    [rdi], r8
  mov    [rdi+8], r9
  mov    [rdi+16], r10
  mov    [rdi+24], r11
  mov    [rdi+32], r12
  pop    r12
  ret

.macro MULADD64x320 M1, Z0, Z1, Z2, Z3, Z4, Z5, T0, T1, C
    xor    \C, \C
    mulx   \T0, \T1, \M1
    adox   \Z0, \T1
    adox   \Z1, \T0
    mulx   \T0, \T1, 8\M1
    adcx   \Z1, \T1
    adox   \Z2, \T0
    mulx   \T0, \T1, 16\M1
    adcx   \Z2, \T1
    adox   \Z3, \T0
    mulx   \T0, \T1, 24\M1
    adcx   \Z3, \T1
    adox   \Z4, \T0
    mulx   \T0, \T1, 32\M1
    adcx   \Z4, \T1
    adox   \Z5, \T0
    adc    \Z5, 0
.endm

.macro MULADD64x64 M1, Z0, Z1, Z2, Z3, Z4, T0, T1
    xor    \T0, \T0
    mulx   \T0, \T1, \M1
    adox   \Z3, \T1
    adox   \Z4, \T0
.endm

.macro FPMUL320x320 M0, M1, Z0, Z1, Z2, Z3, Z4, Z5, T0, T1
    mov    rdx, \Z0
    MULADD64x64 [rip+p_plus_1+32], \Z1, \Z2, \Z3, \Z4, \Z5, \T0, \T1
    mov    rdx, 8\M0
    MULADD64x320 \M1, \Z1, \Z2, \Z3, \Z4, \Z5, \Z0, \T0, \T1, \Z0
    mov    rdx, \Z1
    MULADD64x64 [rip+p_plus_1+32], \Z2, \Z3, \Z4, \Z5, \Z0, \T0, \T1
    mov    rdx, 16\M0
    MULADD64x320 \M1, \Z2, \Z3, \Z4, \Z5, \Z0, \Z1, \T0, \T1, \Z1
    mov    rdx, \Z2
    MULADD64x64 [rip+p_plus_1+32], \Z3, \Z4, \Z5, \Z0, \Z1, \T0, \T1
    mov    rdx, 24\M0
    MULADD64x320 \M1, \Z3, \Z4, \Z5, \Z0, \Z1, \Z2, \T0, \T1, \Z2
    mov    rdx, \Z3
    MULADD64x64 [rip+p_plus_1+32], \Z4, \Z5, \Z0, \Z1, \Z2, \T0, \T1
    mov    rdx, 32\M0
    MULADD64x320 \M1, \Z4, \Z5, \Z0, \Z1, \Z2, \Z3, \T0, \T1, \Z3
    mov    rdx, \Z4
    MULADD64x64 [rip+p_plus_1+32], \Z5, \Z0, \Z1, \Z2, \Z3, \T0, \T1
.endm

.global fp_mul
fp_mul:
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rcx, rdx
  mov    rdx, [rcx]
  mulx   r9, r8, [rsi]
  xor    rax, rax
  mulx   r10, r11, [rsi+8]
  adcx   r9, r11
  mulx   r11, r12, [rsi+16]
  adcx   r10, r12
  mulx   r12, r15, [rsi+24]
  adcx   r11, r15
  mulx   r13, r14, [rsi+32]
  adcx   r12, r14
  adc    r13, 0
  FPMUL320x320 [rcx], [rsi], r8, r9, r10, r11, r12, r13, r14, r15
  mov    [rdi], r13
  mov    [rdi+8], r8
  mov    [rdi+16], r9
  mov    [rdi+24], r10
  mov    [rdi+32], r11
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  ret

.global fp_sqr
fp_sqr:
    mov rdx, rsi
    jmp fp_mul

.global fp2_mul_c0
fp2_mul_c0:
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rcx, rdx
  mov    r8, [rip+p2]
  mov    r9, [rip+p2+8]
  mov    r10, r9
  mov    r11, r9
  mov    r12, [rip+p2+32]
  mov    rax, [rcx+40]
  sub    r8, rax
  mov    rax, [rcx+48]
  sbb    r9, rax
  mov    rax, [rcx+56]
  sbb    r10, rax
  mov    rax, [rcx+64]
  sbb    r11, rax
  mov    rax, [rcx+72]
  sbb    r12, rax
  mov    [rdi], r8
  mov    [rdi+8], r9
  mov    [rdi+16], r10
  mov    [rdi+24], r11
  mov    [rdi+32], r12
  mov    rdx, [rcx]
  mulx   r9, r8, [rsi]
  xor    rax, rax
  mulx   r10, r11, [rsi+8]
  adcx   r9, r11
  mulx   r11, r12, [rsi+16]
  adcx   r10, r12
  mulx   r12, r15, [rsi+24]
  adcx   r11, r15
  mulx   r13, r14, [rsi+32]
  adcx   r12, r14
  adc    r13, 0
  mov    rdx, [rdi]
  MULADD64x320 [rsi+40], r8, r9, r10, r11, r12, r13, r14, r15, r14
  mov    rdx, r8
  MULADD64x64 [rip+p_plus_1+32], r9, r10, r11, r12, r13, r14, r15
  mov    rdx, [rcx+8]
  MULADD64x320 [rsi], r9, r10, r11, r12, r13, r8, r14, r15, r8
  mov    rdx, [rdi+8]
  MULADD64x320 [rsi+40], r9, r10, r11, r12, r13, r8, r14, r15, r14
  mov    rdx, r9
  MULADD64x64 [rip+p_plus_1+32], r10, r11, r12, r13, r8, r14, r15
  mov    rdx, [rcx+16]
  MULADD64x320 [rsi], r10, r11, r12, r13, r8, r9, r14, r15, r9
  mov    rdx, [rdi+16]
  MULADD64x320 [rsi+40], r10, r11, r12, r13, r8, r9, r14, r15, r14
  mov    rdx, r10
  MULADD64x64 [rip+p_plus_1+32], r11, r12, r13, r8, r9, r14, r15
  mov    rdx, [rcx+24]
  MULADD64x320 [rsi], r11, r12, r13, r8, r9, r10, r14, r15, r10
  mov    rdx, [rdi+24]
  MULADD64x320 [rsi+40], r11, r12, r13, r8, r9, r10, r14, r15, r14
  mov    rdx, r11
  MULADD64x64 [rip+p_plus_1+32], r12, r13, r8, r9, r10, r14, r15
  mov    rdx, [rcx+32]
  MULADD64x320 [rsi], r12, r13, r8, r9, r10, r11, r14, r15, r11
  mov    rdx, [rdi+32]
  MULADD64x320 [rsi+40], r12, r13, r8, r9, r10, r11, r14, r15, r14
  mov    rdx, r12
  MULADD64x64 [rip+p_plus_1+32], r13, r8, r9, r10, r11, r14, r15
  mov    [rdi], r13
  mov    [rdi+8], r8
  mov    [rdi+16], r9
  mov    [rdi+24], r10
  mov    [rdi+32], r11
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  ret

.global fp2_mul_c1
fp2_mul_c1:
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rcx, rdx
  mov    rdx, [rcx+40]
  mulx   r9, r8, [rsi]
  xor    rax, rax
  mulx   r10, r11, [rsi+8]
  adcx   r9, r11
  mulx   r11, r12, [rsi+16]
  adcx   r10, r12
  mulx   r12, r15, [rsi+24]
  adcx   r11, r15
  mulx   r13, r14, [rsi+32]
  adcx   r12, r14
  adc    r13, 0
  mov    rdx, [rcx]
  MULADD64x320 [rsi+40], r8, r9, r10, r11, r12, r13, r14, r15, r14
  mov    rdx, r8
  MULADD64x64 [rip+p_plus_1+32], r9, r10, r11, r12, r13, r14, r15
  mov    rdx, [rcx+48]
  MULADD64x320 [rsi], r9, r10, r11, r12, r13, r8, r14, r15, r8
  mov    rdx, [rcx+8]
  MULADD64x320 [rsi+40], r9, r10, r11, r12, r13, r8, r14, r15, r14
  mov    rdx, r9
  MULADD64x64 [rip+p_plus_1+32], r10, r11, r12, r13, r8, r14, r15
  mov    rdx, [rcx+56]
  MULADD64x320 [rsi], r10, r11, r12, r13, r8, r9, r14, r15, r9
  mov    rdx, [rcx+16]
  MULADD64x320 [rsi+40], r10, r11, r12, r13, r8, r9, r14, r15, r14
  mov    rdx, r10
  MULADD64x64 [rip+p_plus_1+32], r11, r12, r13, r8, r9, r14, r15
  mov    rdx, [rcx+64]
  MULADD64x320 [rsi], r11, r12, r13, r8, r9, r10, r14, r15, r10
  mov    rdx, [rcx+24]
  MULADD64x320 [rsi+40], r11, r12, r13, r8, r9, r10, r14, r15, r14
  mov    rdx, r11
  MULADD64x64 [rip+p_plus_1+32], r12, r13, r8, r9, r10, r14, r15
  mov    rdx, [rcx+72]
  MULADD64x320 [rsi], r12, r13, r8, r9, r10, r11, r14, r15, r11
  mov    rdx, [rcx+32]
  MULADD64x320 [rsi+40], r12, r13, r8, r9, r10, r11, r14, r15, r14
  mov    rdx, r12
  MULADD64x64 [rip+p_plus_1+32], r13, r8, r9, r10, r11, r14, r15
  mov    [rdi], r13
  mov    [rdi+8], r8
  mov    [rdi+16], r9
  mov    [rdi+24], r10
  mov    [rdi+32], r11
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  ret

.global fp2_sq_c0
fp2_sq_c0:
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rdx, [rsi]
  mov    r9, [rsi+8]
  mov    r10, [rsi+16]
  mov    r11, [rsi+24]
  mov    r12, [rsi+32]
  add    rdx, [rsi+40]
  adc    r9, [rsi+48]
  adc    r10, [rsi+56]
  adc    r11, [rsi+64]
  adc    r12, [rsi+72]
  mov    [rdi], rdx
  mov    [rdi+8], r9
  mov    [rdi+16], r10
  mov    [rdi+24], r11
  mov    [rdi+32], r12
  mov    r8, [rsi]
  mov    r9, [rsi+8]
  mov    r10, [rsi+16]
  mov    r11, [rsi+24]
  mov    r12, [rsi+32]
  sub    r8, [rsi+40]
  sbb    r9, [rsi+48]
  sbb    r10, [rsi+56]
  sbb    r11, [rsi+64]
  sbb    r12, [rsi+72]
  mov    rax, [rip+p2]
  add    r8, rax
  mov    rax, [rip+p2+8]
  adc    r9, rax
  adc    r10, rax
  adc    r11, rax
  adc    r12, [rip+p2+32]
  mov    [rdi+40], r8
  mov    [rdi+48], r9
  mov    [rdi+56], r10
  mov    [rdi+64], r11
  mov    [rdi+72], r12
  mov    rdx, [rdi]
  mulx   r9, r8, [rdi+40]
  xor    rax, rax
  mulx   r10, r11, [rdi+48]
  adcx   r9, r11
  mulx   r11, r12, [rdi+56]
  adcx   r10, r12
  mulx   r12, r15, [rdi+64]
  adcx   r11, r15
  mulx   r13, r14, [rdi+72]
  adcx   r12, r14
  adc    r13, 0
  FPMUL320x320 [rdi], [rdi+40], r8, r9, r10, r11, r12, r13, r14, r15
  mov    [rdi], r13
  mov    [rdi+8], r8
  mov    [rdi+16], r9
  mov    [rdi+24], r10
  mov    [rdi+32], r11
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  ret

.global fp2_sq_c1
fp2_sq_c1:
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rdx, [rsi]
  mov    r9, [rsi+8]
  mov    r10, [rsi+16]
  mov    r11, [rsi+24]
  mov    r12, [rsi+32]
  add    rdx, rdx
  adc    r9, r9
  adc    r10, r10
  adc    r11, r11
  adc    r12, r12
  sub    rsp, 40
  mov    [rsp+8], r9
  mov    [rsp+16], r10
  mov    [rsp+24], r11
  mov    [rsp+32], r12
  mulx   r9, r8, [rsi+40]
  xor    rax, rax
  mulx   r10, r11, [rsi+48]
  adcx   r9, r11
  mulx   r11, r12, [rsi+56]
  adcx   r10, r12
  mulx   r12, r15, [rsi+64]
  adcx   r11, r15
  mulx   r13, r14, [rsi+72]
  adcx   r12, r14
  adc    r13, 0
  FPMUL320x320 [rsp], [rsi+40], r8, r9, r10, r11, r12, r13, r14, r15
  add    rsp, 40
  mov    [rdi], r13
  mov    [rdi+8], r8
  mov    [rdi+16], r9
  mov    [rdi+24], r10
  mov    [rdi+32], r11
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  ret

