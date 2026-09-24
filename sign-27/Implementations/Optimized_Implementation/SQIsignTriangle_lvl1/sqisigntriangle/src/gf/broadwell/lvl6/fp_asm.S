#include <sqisign_namespace.h>
.intel_syntax noprefix

#ifdef __APPLE__
.section __TEXT,__const
#else
.section .rodata
#endif
p_plus_1: .quad 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x7100000000000000
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
#include <asm_preamble.h>

.text
.p2align 4,,15

.global fp_add
fp_add:
  mov    r8, [rsi]
  add    r8, [rdx]
  mov    [rdi], r8
  mov    r8, [rsi+8]
  adc    r8, [rdx+8]
  mov    [rdi+8], r8
  mov    r8, [rsi+16]
  adc    r8, [rdx+16]
  mov    [rdi+16], r8
  mov    r8, [rsi+24]
  adc    r8, [rdx+24]
  mov    [rdi+24], r8
  mov    r8, [rsi+32]
  adc    r8, [rdx+32]
  mov    [rdi+32], r8
  mov    r8, [rsi+40]
  adc    r8, [rdx+40]
  mov    [rdi+40], r8
  mov    r8, [rsi+48]
  adc    r8, [rdx+48]
  mov    [rdi+48], r8
  mov    r8, [rsi+56]
  adc    r8, [rdx+56]
  mov    [rdi+56], r8
  mov    r8, [rsi+64]
  adc    r8, [rdx+64]
  mov    [rdi+64], r8
  mov    r8, [rsi+72]
  adc    r8, [rdx+72]
  mov    [rdi+72], r8
  mov    r8, [rsi+80]
  adc    r8, [rdx+80]
  mov    [rdi+80], r8
  mov    r8, [rsi+88]
  adc    r8, [rdx+88]
  mov    [rdi+88], r8
  mov    r8, [rsi+96]
  adc    r8, [rdx+96]
  mov    [rdi+96], r8
  mov    r8, [rsi+104]
  adc    r8, [rdx+104]
  mov    [rdi+104], r8
  mov    r8, [rsi+112]
  adc    r8, [rdx+112]
  mov    [rdi+112], r8
  mov    r8, [rsi+120]
  adc    r8, [rdx+120]
  mov    [rdi+120], r8
  mov    rax, [rdi+120]
  sar    rax, 63
  mov    rdx, [rip+p+120]
  and    rdx, rax
  mov    r8, [rdi]
  sub    r8, rax
  mov    [rdi], r8
  mov    r8, [rdi+8]
  sbb    r8, rax
  mov    [rdi+8], r8
  mov    r8, [rdi+16]
  sbb    r8, rax
  mov    [rdi+16], r8
  mov    r8, [rdi+24]
  sbb    r8, rax
  mov    [rdi+24], r8
  mov    r8, [rdi+32]
  sbb    r8, rax
  mov    [rdi+32], r8
  mov    r8, [rdi+40]
  sbb    r8, rax
  mov    [rdi+40], r8
  mov    r8, [rdi+48]
  sbb    r8, rax
  mov    [rdi+48], r8
  mov    r8, [rdi+56]
  sbb    r8, rax
  mov    [rdi+56], r8
  mov    r8, [rdi+64]
  sbb    r8, rax
  mov    [rdi+64], r8
  mov    r8, [rdi+72]
  sbb    r8, rax
  mov    [rdi+72], r8
  mov    r8, [rdi+80]
  sbb    r8, rax
  mov    [rdi+80], r8
  mov    r8, [rdi+88]
  sbb    r8, rax
  mov    [rdi+88], r8
  mov    r8, [rdi+96]
  sbb    r8, rax
  mov    [rdi+96], r8
  mov    r8, [rdi+104]
  sbb    r8, rax
  mov    [rdi+104], r8
  mov    r8, [rdi+112]
  sbb    r8, rax
  mov    [rdi+112], r8
  mov    r8, [rdi+120]
  sbb    r8, rdx
  mov    [rdi+120], r8
  mov    rax, [rdi+120]
  sar    rax, 63
  mov    rdx, [rip+p+120]
  and    rdx, rax
  mov    r8, [rdi]
  sub    r8, rax
  mov    [rdi], r8
  mov    r8, [rdi+8]
  sbb    r8, rax
  mov    [rdi+8], r8
  mov    r8, [rdi+16]
  sbb    r8, rax
  mov    [rdi+16], r8
  mov    r8, [rdi+24]
  sbb    r8, rax
  mov    [rdi+24], r8
  mov    r8, [rdi+32]
  sbb    r8, rax
  mov    [rdi+32], r8
  mov    r8, [rdi+40]
  sbb    r8, rax
  mov    [rdi+40], r8
  mov    r8, [rdi+48]
  sbb    r8, rax
  mov    [rdi+48], r8
  mov    r8, [rdi+56]
  sbb    r8, rax
  mov    [rdi+56], r8
  mov    r8, [rdi+64]
  sbb    r8, rax
  mov    [rdi+64], r8
  mov    r8, [rdi+72]
  sbb    r8, rax
  mov    [rdi+72], r8
  mov    r8, [rdi+80]
  sbb    r8, rax
  mov    [rdi+80], r8
  mov    r8, [rdi+88]
  sbb    r8, rax
  mov    [rdi+88], r8
  mov    r8, [rdi+96]
  sbb    r8, rax
  mov    [rdi+96], r8
  mov    r8, [rdi+104]
  sbb    r8, rax
  mov    [rdi+104], r8
  mov    r8, [rdi+112]
  sbb    r8, rax
  mov    [rdi+112], r8
  mov    r8, [rdi+120]
  sbb    r8, rdx
  mov    [rdi+120], r8
  ret

.global fp_sub
fp_sub:
  mov    r8, [rsi]
  sub    r8, [rdx]
  mov    [rdi], r8
  mov    r8, [rsi+8]
  sbb    r8, [rdx+8]
  mov    [rdi+8], r8
  mov    r8, [rsi+16]
  sbb    r8, [rdx+16]
  mov    [rdi+16], r8
  mov    r8, [rsi+24]
  sbb    r8, [rdx+24]
  mov    [rdi+24], r8
  mov    r8, [rsi+32]
  sbb    r8, [rdx+32]
  mov    [rdi+32], r8
  mov    r8, [rsi+40]
  sbb    r8, [rdx+40]
  mov    [rdi+40], r8
  mov    r8, [rsi+48]
  sbb    r8, [rdx+48]
  mov    [rdi+48], r8
  mov    r8, [rsi+56]
  sbb    r8, [rdx+56]
  mov    [rdi+56], r8
  mov    r8, [rsi+64]
  sbb    r8, [rdx+64]
  mov    [rdi+64], r8
  mov    r8, [rsi+72]
  sbb    r8, [rdx+72]
  mov    [rdi+72], r8
  mov    r8, [rsi+80]
  sbb    r8, [rdx+80]
  mov    [rdi+80], r8
  mov    r8, [rsi+88]
  sbb    r8, [rdx+88]
  mov    [rdi+88], r8
  mov    r8, [rsi+96]
  sbb    r8, [rdx+96]
  mov    [rdi+96], r8
  mov    r8, [rsi+104]
  sbb    r8, [rdx+104]
  mov    [rdi+104], r8
  mov    r8, [rsi+112]
  sbb    r8, [rdx+112]
  mov    [rdi+112], r8
  mov    r8, [rsi+120]
  sbb    r8, [rdx+120]
  mov    [rdi+120], r8
  sbb    rax, rax
  mov    rdx, [rip+p+120]
  and    rdx, rax
  mov    r8, [rdi]
  add    r8, rax
  mov    [rdi], r8
  mov    r8, [rdi+8]
  adc    r8, rax
  mov    [rdi+8], r8
  mov    r8, [rdi+16]
  adc    r8, rax
  mov    [rdi+16], r8
  mov    r8, [rdi+24]
  adc    r8, rax
  mov    [rdi+24], r8
  mov    r8, [rdi+32]
  adc    r8, rax
  mov    [rdi+32], r8
  mov    r8, [rdi+40]
  adc    r8, rax
  mov    [rdi+40], r8
  mov    r8, [rdi+48]
  adc    r8, rax
  mov    [rdi+48], r8
  mov    r8, [rdi+56]
  adc    r8, rax
  mov    [rdi+56], r8
  mov    r8, [rdi+64]
  adc    r8, rax
  mov    [rdi+64], r8
  mov    r8, [rdi+72]
  adc    r8, rax
  mov    [rdi+72], r8
  mov    r8, [rdi+80]
  adc    r8, rax
  mov    [rdi+80], r8
  mov    r8, [rdi+88]
  adc    r8, rax
  mov    [rdi+88], r8
  mov    r8, [rdi+96]
  adc    r8, rax
  mov    [rdi+96], r8
  mov    r8, [rdi+104]
  adc    r8, rax
  mov    [rdi+104], r8
  mov    r8, [rdi+112]
  adc    r8, rax
  mov    [rdi+112], r8
  mov    r8, [rdi+120]
  adc    r8, rdx
  mov    [rdi+120], r8
  mov    rax, [rdi+120]
  sar    rax, 63
  mov    rdx, [rip+p+120]
  and    rdx, rax
  mov    r8, [rdi]
  add    r8, rax
  mov    [rdi], r8
  mov    r8, [rdi+8]
  adc    r8, rax
  mov    [rdi+8], r8
  mov    r8, [rdi+16]
  adc    r8, rax
  mov    [rdi+16], r8
  mov    r8, [rdi+24]
  adc    r8, rax
  mov    [rdi+24], r8
  mov    r8, [rdi+32]
  adc    r8, rax
  mov    [rdi+32], r8
  mov    r8, [rdi+40]
  adc    r8, rax
  mov    [rdi+40], r8
  mov    r8, [rdi+48]
  adc    r8, rax
  mov    [rdi+48], r8
  mov    r8, [rdi+56]
  adc    r8, rax
  mov    [rdi+56], r8
  mov    r8, [rdi+64]
  adc    r8, rax
  mov    [rdi+64], r8
  mov    r8, [rdi+72]
  adc    r8, rax
  mov    [rdi+72], r8
  mov    r8, [rdi+80]
  adc    r8, rax
  mov    [rdi+80], r8
  mov    r8, [rdi+88]
  adc    r8, rax
  mov    [rdi+88], r8
  mov    r8, [rdi+96]
  adc    r8, rax
  mov    [rdi+96], r8
  mov    r8, [rdi+104]
  adc    r8, rax
  mov    [rdi+104], r8
  mov    r8, [rdi+112]
  adc    r8, rax
  mov    [rdi+112], r8
  mov    r8, [rdi+120]
  adc    r8, rdx
  mov    [rdi+120], r8
  ret

.global fp_mul
fp_mul:
  push   rbx
  push   rbp
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rcx, rdx
  sub    rsp, 272
  xor    eax, eax
  mov    [rsp], rax
  mov    [rsp+8], rax
  mov    [rsp+16], rax
  mov    [rsp+24], rax
  mov    [rsp+32], rax
  mov    [rsp+40], rax
  mov    [rsp+48], rax
  mov    [rsp+56], rax
  mov    [rsp+64], rax
  mov    [rsp+72], rax
  mov    [rsp+80], rax
  mov    [rsp+88], rax
  mov    [rsp+96], rax
  mov    [rsp+104], rax
  mov    [rsp+112], rax
  mov    [rsp+120], rax
  mov    [rsp+128], rax
  mov    [rsp+136], rax
  mov    [rsp+144], rax
  mov    [rsp+152], rax
  mov    [rsp+160], rax
  mov    [rsp+168], rax
  mov    [rsp+176], rax
  mov    [rsp+184], rax
  mov    [rsp+192], rax
  mov    [rsp+200], rax
  mov    [rsp+208], rax
  mov    [rsp+216], rax
  mov    [rsp+224], rax
  mov    [rsp+232], rax
  mov    [rsp+240], rax
  mov    [rsp+248], rax
  mov    [rsp+256], rax
  mov    [rsp+264], rax
  xor    r14d, r14d
.Lmul:
  mov    rdx, [rcx + r14]
  xor    r12d, r12d
  mulx   r9, r8, [rsi]
  adcx   r8, [rsp + r14]
  mov    [rsp + r14], r8
  mulx   r11, r10, [rsi+8]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 8]
  mov    [rsp + r14 + 8], r10
  mulx   r9, r8, [rsi+16]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 16]
  mov    [rsp + r14 + 16], r8
  mulx   r11, r10, [rsi+24]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 24]
  mov    [rsp + r14 + 24], r10
  mulx   r9, r8, [rsi+32]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 32]
  mov    [rsp + r14 + 32], r8
  mulx   r11, r10, [rsi+40]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 40]
  mov    [rsp + r14 + 40], r10
  mulx   r9, r8, [rsi+48]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 48]
  mov    [rsp + r14 + 48], r8
  mulx   r11, r10, [rsi+56]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 56]
  mov    [rsp + r14 + 56], r10
  mulx   r9, r8, [rsi+64]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 64]
  mov    [rsp + r14 + 64], r8
  mulx   r11, r10, [rsi+72]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 72]
  mov    [rsp + r14 + 72], r10
  mulx   r9, r8, [rsi+80]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 80]
  mov    [rsp + r14 + 80], r8
  mulx   r11, r10, [rsi+88]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 88]
  mov    [rsp + r14 + 88], r10
  mulx   r9, r8, [rsi+96]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 96]
  mov    [rsp + r14 + 96], r8
  mulx   r11, r10, [rsi+104]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 104]
  mov    [rsp + r14 + 104], r10
  mulx   r9, r8, [rsi+112]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 112]
  mov    [rsp + r14 + 112], r8
  mulx   r11, r10, [rsi+120]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 120]
  mov    [rsp + r14 + 120], r10
  mov    r13, [rsp + r14 + 128]
  adox   r11, r12
  adcx   r11, r13
  mov    [rsp + r14 + 128], r11
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  mov    rdx, [rsp + r14]
  mulx   r9, r8, [rip+p_plus_1+120]
  mov    r13, [rsp + r14 + 120]
  add    r13, r8
  mov    [rsp + r14 + 120], r13
  mov    r13, [rsp + r14 + 128]
  adc    r13, r9
  mov    [rsp + r14 + 128], r13
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  add    r14, 8
  cmp    r14, 128
  jne    .Lmul
  mov    rax, [rsp + 248]
  sar    rax, 63
  mov    rdx, [rip+p+120]
  and    rdx, rax
  mov    r8, [rsp + 128]
  sub    r8, rax
  mov    [rdi], r8
  mov    r8, [rsp + 136]
  sbb    r8, rax
  mov    [rdi+8], r8
  mov    r8, [rsp + 144]
  sbb    r8, rax
  mov    [rdi+16], r8
  mov    r8, [rsp + 152]
  sbb    r8, rax
  mov    [rdi+24], r8
  mov    r8, [rsp + 160]
  sbb    r8, rax
  mov    [rdi+32], r8
  mov    r8, [rsp + 168]
  sbb    r8, rax
  mov    [rdi+40], r8
  mov    r8, [rsp + 176]
  sbb    r8, rax
  mov    [rdi+48], r8
  mov    r8, [rsp + 184]
  sbb    r8, rax
  mov    [rdi+56], r8
  mov    r8, [rsp + 192]
  sbb    r8, rax
  mov    [rdi+64], r8
  mov    r8, [rsp + 200]
  sbb    r8, rax
  mov    [rdi+72], r8
  mov    r8, [rsp + 208]
  sbb    r8, rax
  mov    [rdi+80], r8
  mov    r8, [rsp + 216]
  sbb    r8, rax
  mov    [rdi+88], r8
  mov    r8, [rsp + 224]
  sbb    r8, rax
  mov    [rdi+96], r8
  mov    r8, [rsp + 232]
  sbb    r8, rax
  mov    [rdi+104], r8
  mov    r8, [rsp + 240]
  sbb    r8, rax
  mov    [rdi+112], r8
  mov    r8, [rsp + 248]
  sbb    r8, rdx
  mov    [rdi+120], r8
  add    rsp, 272
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  pop    rbp
  pop    rbx
  ret

.global fp_sqr
fp_sqr:
  mov    rdx, rsi
  jmp    fp_mul

.global fp2_mul_c0
fp2_mul_c0:
  push   rbx
  push   rbp
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rcx, rdx
  sub    rsp, 656
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  xor    eax, eax
  mov    r8, [rsi+0]
  sub    r8, rbx
  mov    [rsp+272], r8
  mov    r8, [rsi+8]
  sbb    r8, rbx
  mov    [rsp+280], r8
  mov    r8, [rsi+16]
  sbb    r8, rbx
  mov    [rsp+288], r8
  mov    r8, [rsi+24]
  sbb    r8, rbx
  mov    [rsp+296], r8
  mov    r8, [rsi+32]
  sbb    r8, rbx
  mov    [rsp+304], r8
  mov    r8, [rsi+40]
  sbb    r8, rbx
  mov    [rsp+312], r8
  mov    r8, [rsi+48]
  sbb    r8, rbx
  mov    [rsp+320], r8
  mov    r8, [rsi+56]
  sbb    r8, rbx
  mov    [rsp+328], r8
  mov    r8, [rsi+64]
  sbb    r8, rbx
  mov    [rsp+336], r8
  mov    r8, [rsi+72]
  sbb    r8, rbx
  mov    [rsp+344], r8
  mov    r8, [rsi+80]
  sbb    r8, rbx
  mov    [rsp+352], r8
  mov    r8, [rsi+88]
  sbb    r8, rbx
  mov    [rsp+360], r8
  mov    r8, [rsi+96]
  sbb    r8, rbx
  mov    [rsp+368], r8
  mov    r8, [rsi+104]
  sbb    r8, rbx
  mov    [rsp+376], r8
  mov    r8, [rsi+112]
  sbb    r8, rbx
  mov    [rsp+384], r8
  mov    r8, [rsi+120]
  sbb    r8, r15
  mov    [rsp+392], r8
  sbb    rax, 0
  and    r15, rax
  mov    r8, [rsp+272]
  add    r8, rax
  mov    [rsp+272], r8
  mov    r8, [rsp+280]
  adc    r8, rax
  mov    [rsp+280], r8
  mov    r8, [rsp+288]
  adc    r8, rax
  mov    [rsp+288], r8
  mov    r8, [rsp+296]
  adc    r8, rax
  mov    [rsp+296], r8
  mov    r8, [rsp+304]
  adc    r8, rax
  mov    [rsp+304], r8
  mov    r8, [rsp+312]
  adc    r8, rax
  mov    [rsp+312], r8
  mov    r8, [rsp+320]
  adc    r8, rax
  mov    [rsp+320], r8
  mov    r8, [rsp+328]
  adc    r8, rax
  mov    [rsp+328], r8
  mov    r8, [rsp+336]
  adc    r8, rax
  mov    [rsp+336], r8
  mov    r8, [rsp+344]
  adc    r8, rax
  mov    [rsp+344], r8
  mov    r8, [rsp+352]
  adc    r8, rax
  mov    [rsp+352], r8
  mov    r8, [rsp+360]
  adc    r8, rax
  mov    [rsp+360], r8
  mov    r8, [rsp+368]
  adc    r8, rax
  mov    [rsp+368], r8
  mov    r8, [rsp+376]
  adc    r8, rax
  mov    [rsp+376], r8
  mov    r8, [rsp+384]
  adc    r8, rax
  mov    [rsp+384], r8
  mov    r8, [rsp+392]
  adc    r8, r15
  mov    [rsp+392], r8
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  xor    eax, eax
  mov    r8, [rsi+128]
  sub    r8, rbx
  mov    [rsp+400], r8
  mov    r8, [rsi+136]
  sbb    r8, rbx
  mov    [rsp+408], r8
  mov    r8, [rsi+144]
  sbb    r8, rbx
  mov    [rsp+416], r8
  mov    r8, [rsi+152]
  sbb    r8, rbx
  mov    [rsp+424], r8
  mov    r8, [rsi+160]
  sbb    r8, rbx
  mov    [rsp+432], r8
  mov    r8, [rsi+168]
  sbb    r8, rbx
  mov    [rsp+440], r8
  mov    r8, [rsi+176]
  sbb    r8, rbx
  mov    [rsp+448], r8
  mov    r8, [rsi+184]
  sbb    r8, rbx
  mov    [rsp+456], r8
  mov    r8, [rsi+192]
  sbb    r8, rbx
  mov    [rsp+464], r8
  mov    r8, [rsi+200]
  sbb    r8, rbx
  mov    [rsp+472], r8
  mov    r8, [rsi+208]
  sbb    r8, rbx
  mov    [rsp+480], r8
  mov    r8, [rsi+216]
  sbb    r8, rbx
  mov    [rsp+488], r8
  mov    r8, [rsi+224]
  sbb    r8, rbx
  mov    [rsp+496], r8
  mov    r8, [rsi+232]
  sbb    r8, rbx
  mov    [rsp+504], r8
  mov    r8, [rsi+240]
  sbb    r8, rbx
  mov    [rsp+512], r8
  mov    r8, [rsi+248]
  sbb    r8, r15
  mov    [rsp+520], r8
  sbb    rax, 0
  and    r15, rax
  mov    r8, [rsp+400]
  add    r8, rax
  mov    [rsp+400], r8
  mov    r8, [rsp+408]
  adc    r8, rax
  mov    [rsp+408], r8
  mov    r8, [rsp+416]
  adc    r8, rax
  mov    [rsp+416], r8
  mov    r8, [rsp+424]
  adc    r8, rax
  mov    [rsp+424], r8
  mov    r8, [rsp+432]
  adc    r8, rax
  mov    [rsp+432], r8
  mov    r8, [rsp+440]
  adc    r8, rax
  mov    [rsp+440], r8
  mov    r8, [rsp+448]
  adc    r8, rax
  mov    [rsp+448], r8
  mov    r8, [rsp+456]
  adc    r8, rax
  mov    [rsp+456], r8
  mov    r8, [rsp+464]
  adc    r8, rax
  mov    [rsp+464], r8
  mov    r8, [rsp+472]
  adc    r8, rax
  mov    [rsp+472], r8
  mov    r8, [rsp+480]
  adc    r8, rax
  mov    [rsp+480], r8
  mov    r8, [rsp+488]
  adc    r8, rax
  mov    [rsp+488], r8
  mov    r8, [rsp+496]
  adc    r8, rax
  mov    [rsp+496], r8
  mov    r8, [rsp+504]
  adc    r8, rax
  mov    [rsp+504], r8
  mov    r8, [rsp+512]
  adc    r8, rax
  mov    [rsp+512], r8
  mov    r8, [rsp+520]
  adc    r8, r15
  mov    [rsp+520], r8
  mov    r8, [rip+p2]
  sub    r8, [rcx+128]
  mov    [rsp+528], r8
  mov    r8, [rip+p2+8]
  sbb    r8, [rcx+136]
  mov    [rsp+536], r8
  mov    r8, [rip+p2+16]
  sbb    r8, [rcx+144]
  mov    [rsp+544], r8
  mov    r8, [rip+p2+24]
  sbb    r8, [rcx+152]
  mov    [rsp+552], r8
  mov    r8, [rip+p2+32]
  sbb    r8, [rcx+160]
  mov    [rsp+560], r8
  mov    r8, [rip+p2+40]
  sbb    r8, [rcx+168]
  mov    [rsp+568], r8
  mov    r8, [rip+p2+48]
  sbb    r8, [rcx+176]
  mov    [rsp+576], r8
  mov    r8, [rip+p2+56]
  sbb    r8, [rcx+184]
  mov    [rsp+584], r8
  mov    r8, [rip+p2+64]
  sbb    r8, [rcx+192]
  mov    [rsp+592], r8
  mov    r8, [rip+p2+72]
  sbb    r8, [rcx+200]
  mov    [rsp+600], r8
  mov    r8, [rip+p2+80]
  sbb    r8, [rcx+208]
  mov    [rsp+608], r8
  mov    r8, [rip+p2+88]
  sbb    r8, [rcx+216]
  mov    [rsp+616], r8
  mov    r8, [rip+p2+96]
  sbb    r8, [rcx+224]
  mov    [rsp+624], r8
  mov    r8, [rip+p2+104]
  sbb    r8, [rcx+232]
  mov    [rsp+632], r8
  mov    r8, [rip+p2+112]
  sbb    r8, [rcx+240]
  mov    [rsp+640], r8
  mov    r8, [rip+p2+120]
  sbb    r8, [rcx+248]
  mov    [rsp+648], r8
  xor    eax, eax
  mov    [rsp], rax
  mov    [rsp+8], rax
  mov    [rsp+16], rax
  mov    [rsp+24], rax
  mov    [rsp+32], rax
  mov    [rsp+40], rax
  mov    [rsp+48], rax
  mov    [rsp+56], rax
  mov    [rsp+64], rax
  mov    [rsp+72], rax
  mov    [rsp+80], rax
  mov    [rsp+88], rax
  mov    [rsp+96], rax
  mov    [rsp+104], rax
  mov    [rsp+112], rax
  mov    [rsp+120], rax
  mov    [rsp+128], rax
  mov    [rsp+136], rax
  mov    [rsp+144], rax
  mov    [rsp+152], rax
  mov    [rsp+160], rax
  mov    [rsp+168], rax
  mov    [rsp+176], rax
  mov    [rsp+184], rax
  mov    [rsp+192], rax
  mov    [rsp+200], rax
  mov    [rsp+208], rax
  mov    [rsp+216], rax
  mov    [rsp+224], rax
  mov    [rsp+232], rax
  mov    [rsp+240], rax
  mov    [rsp+248], rax
  mov    [rsp+256], rax
  mov    [rsp+264], rax
  xor    r14d, r14d
.Lfp2_mul_c0:
  mov    rdx, [rcx + r14]
  xor    r12d, r12d
  mulx   r9, r8, [rsp+272]
  adcx   r8, [rsp + r14]
  mov    [rsp + r14], r8
  mulx   r11, r10, [rsp+280]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 8]
  mov    [rsp + r14 + 8], r10
  mulx   r9, r8, [rsp+288]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 16]
  mov    [rsp + r14 + 16], r8
  mulx   r11, r10, [rsp+296]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 24]
  mov    [rsp + r14 + 24], r10
  mulx   r9, r8, [rsp+304]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 32]
  mov    [rsp + r14 + 32], r8
  mulx   r11, r10, [rsp+312]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 40]
  mov    [rsp + r14 + 40], r10
  mulx   r9, r8, [rsp+320]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 48]
  mov    [rsp + r14 + 48], r8
  mulx   r11, r10, [rsp+328]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 56]
  mov    [rsp + r14 + 56], r10
  mulx   r9, r8, [rsp+336]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 64]
  mov    [rsp + r14 + 64], r8
  mulx   r11, r10, [rsp+344]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 72]
  mov    [rsp + r14 + 72], r10
  mulx   r9, r8, [rsp+352]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 80]
  mov    [rsp + r14 + 80], r8
  mulx   r11, r10, [rsp+360]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 88]
  mov    [rsp + r14 + 88], r10
  mulx   r9, r8, [rsp+368]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 96]
  mov    [rsp + r14 + 96], r8
  mulx   r11, r10, [rsp+376]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 104]
  mov    [rsp + r14 + 104], r10
  mulx   r9, r8, [rsp+384]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 112]
  mov    [rsp + r14 + 112], r8
  mulx   r11, r10, [rsp+392]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 120]
  mov    [rsp + r14 + 120], r10
  mov    r13, [rsp + r14 + 128]
  adox   r11, r12
  adcx   r11, r13
  mov    [rsp + r14 + 128], r11
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  lea    rax, [rsp + r14]
  mov    rdx, [rax + 528]
  xor    r12d, r12d
  mulx   r9, r8, [rsp+400]
  adcx   r8, [rsp + r14]
  mov    [rsp + r14], r8
  mulx   r11, r10, [rsp+408]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 8]
  mov    [rsp + r14 + 8], r10
  mulx   r9, r8, [rsp+416]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 16]
  mov    [rsp + r14 + 16], r8
  mulx   r11, r10, [rsp+424]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 24]
  mov    [rsp + r14 + 24], r10
  mulx   r9, r8, [rsp+432]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 32]
  mov    [rsp + r14 + 32], r8
  mulx   r11, r10, [rsp+440]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 40]
  mov    [rsp + r14 + 40], r10
  mulx   r9, r8, [rsp+448]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 48]
  mov    [rsp + r14 + 48], r8
  mulx   r11, r10, [rsp+456]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 56]
  mov    [rsp + r14 + 56], r10
  mulx   r9, r8, [rsp+464]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 64]
  mov    [rsp + r14 + 64], r8
  mulx   r11, r10, [rsp+472]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 72]
  mov    [rsp + r14 + 72], r10
  mulx   r9, r8, [rsp+480]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 80]
  mov    [rsp + r14 + 80], r8
  mulx   r11, r10, [rsp+488]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 88]
  mov    [rsp + r14 + 88], r10
  mulx   r9, r8, [rsp+496]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 96]
  mov    [rsp + r14 + 96], r8
  mulx   r11, r10, [rsp+504]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 104]
  mov    [rsp + r14 + 104], r10
  mulx   r9, r8, [rsp+512]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 112]
  mov    [rsp + r14 + 112], r8
  mulx   r11, r10, [rsp+520]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 120]
  mov    [rsp + r14 + 120], r10
  mov    r13, [rsp + r14 + 128]
  adox   r11, r12
  adcx   r11, r13
  mov    [rsp + r14 + 128], r11
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  mov    rdx, [rsp + r14]
  mulx   r9, r8, [rip+p_plus_1+120]
  mov    r13, [rsp + r14 + 120]
  add    r13, r8
  mov    [rsp + r14 + 120], r13
  mov    r13, [rsp + r14 + 128]
  adc    r13, r9
  mov    [rsp + r14 + 128], r13
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  add    r14, 8
  cmp    r14, 128
  jne    .Lfp2_mul_c0
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    r8, [rsp+128]
  mov    [rdi], r8
  mov    r8, [rsp+136]
  mov    [rdi+8], r8
  mov    r8, [rsp+144]
  mov    [rdi+16], r8
  mov    r8, [rsp+152]
  mov    [rdi+24], r8
  mov    r8, [rsp+160]
  mov    [rdi+32], r8
  mov    r8, [rsp+168]
  mov    [rdi+40], r8
  mov    r8, [rsp+176]
  mov    [rdi+48], r8
  mov    r8, [rsp+184]
  mov    [rdi+56], r8
  mov    r8, [rsp+192]
  mov    [rdi+64], r8
  mov    r8, [rsp+200]
  mov    [rdi+72], r8
  mov    r8, [rsp+208]
  mov    [rdi+80], r8
  mov    r8, [rsp+216]
  mov    [rdi+88], r8
  mov    r8, [rsp+224]
  mov    [rdi+96], r8
  mov    r8, [rsp+232]
  mov    [rdi+104], r8
  mov    r8, [rsp+240]
  mov    [rdi+112], r8
  mov    r8, [rsp+248]
  mov    [rdi+120], r8
  add    rsp, 656
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  pop    rbp
  pop    rbx
  ret

.global fp2_mul_c1
fp2_mul_c1:
  push   rbx
  push   rbp
  push   r12
  push   r13
  push   r14
  push   r15
  mov    rcx, rdx
  sub    rsp, 656
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  xor    eax, eax
  mov    r8, [rsi+0]
  sub    r8, rbx
  mov    [rsp+272], r8
  mov    r8, [rsi+8]
  sbb    r8, rbx
  mov    [rsp+280], r8
  mov    r8, [rsi+16]
  sbb    r8, rbx
  mov    [rsp+288], r8
  mov    r8, [rsi+24]
  sbb    r8, rbx
  mov    [rsp+296], r8
  mov    r8, [rsi+32]
  sbb    r8, rbx
  mov    [rsp+304], r8
  mov    r8, [rsi+40]
  sbb    r8, rbx
  mov    [rsp+312], r8
  mov    r8, [rsi+48]
  sbb    r8, rbx
  mov    [rsp+320], r8
  mov    r8, [rsi+56]
  sbb    r8, rbx
  mov    [rsp+328], r8
  mov    r8, [rsi+64]
  sbb    r8, rbx
  mov    [rsp+336], r8
  mov    r8, [rsi+72]
  sbb    r8, rbx
  mov    [rsp+344], r8
  mov    r8, [rsi+80]
  sbb    r8, rbx
  mov    [rsp+352], r8
  mov    r8, [rsi+88]
  sbb    r8, rbx
  mov    [rsp+360], r8
  mov    r8, [rsi+96]
  sbb    r8, rbx
  mov    [rsp+368], r8
  mov    r8, [rsi+104]
  sbb    r8, rbx
  mov    [rsp+376], r8
  mov    r8, [rsi+112]
  sbb    r8, rbx
  mov    [rsp+384], r8
  mov    r8, [rsi+120]
  sbb    r8, r15
  mov    [rsp+392], r8
  sbb    rax, 0
  and    r15, rax
  mov    r8, [rsp+272]
  add    r8, rax
  mov    [rsp+272], r8
  mov    r8, [rsp+280]
  adc    r8, rax
  mov    [rsp+280], r8
  mov    r8, [rsp+288]
  adc    r8, rax
  mov    [rsp+288], r8
  mov    r8, [rsp+296]
  adc    r8, rax
  mov    [rsp+296], r8
  mov    r8, [rsp+304]
  adc    r8, rax
  mov    [rsp+304], r8
  mov    r8, [rsp+312]
  adc    r8, rax
  mov    [rsp+312], r8
  mov    r8, [rsp+320]
  adc    r8, rax
  mov    [rsp+320], r8
  mov    r8, [rsp+328]
  adc    r8, rax
  mov    [rsp+328], r8
  mov    r8, [rsp+336]
  adc    r8, rax
  mov    [rsp+336], r8
  mov    r8, [rsp+344]
  adc    r8, rax
  mov    [rsp+344], r8
  mov    r8, [rsp+352]
  adc    r8, rax
  mov    [rsp+352], r8
  mov    r8, [rsp+360]
  adc    r8, rax
  mov    [rsp+360], r8
  mov    r8, [rsp+368]
  adc    r8, rax
  mov    [rsp+368], r8
  mov    r8, [rsp+376]
  adc    r8, rax
  mov    [rsp+376], r8
  mov    r8, [rsp+384]
  adc    r8, rax
  mov    [rsp+384], r8
  mov    r8, [rsp+392]
  adc    r8, r15
  mov    [rsp+392], r8
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  xor    eax, eax
  mov    r8, [rsi+128]
  sub    r8, rbx
  mov    [rsp+400], r8
  mov    r8, [rsi+136]
  sbb    r8, rbx
  mov    [rsp+408], r8
  mov    r8, [rsi+144]
  sbb    r8, rbx
  mov    [rsp+416], r8
  mov    r8, [rsi+152]
  sbb    r8, rbx
  mov    [rsp+424], r8
  mov    r8, [rsi+160]
  sbb    r8, rbx
  mov    [rsp+432], r8
  mov    r8, [rsi+168]
  sbb    r8, rbx
  mov    [rsp+440], r8
  mov    r8, [rsi+176]
  sbb    r8, rbx
  mov    [rsp+448], r8
  mov    r8, [rsi+184]
  sbb    r8, rbx
  mov    [rsp+456], r8
  mov    r8, [rsi+192]
  sbb    r8, rbx
  mov    [rsp+464], r8
  mov    r8, [rsi+200]
  sbb    r8, rbx
  mov    [rsp+472], r8
  mov    r8, [rsi+208]
  sbb    r8, rbx
  mov    [rsp+480], r8
  mov    r8, [rsi+216]
  sbb    r8, rbx
  mov    [rsp+488], r8
  mov    r8, [rsi+224]
  sbb    r8, rbx
  mov    [rsp+496], r8
  mov    r8, [rsi+232]
  sbb    r8, rbx
  mov    [rsp+504], r8
  mov    r8, [rsi+240]
  sbb    r8, rbx
  mov    [rsp+512], r8
  mov    r8, [rsi+248]
  sbb    r8, r15
  mov    [rsp+520], r8
  sbb    rax, 0
  and    r15, rax
  mov    r8, [rsp+400]
  add    r8, rax
  mov    [rsp+400], r8
  mov    r8, [rsp+408]
  adc    r8, rax
  mov    [rsp+408], r8
  mov    r8, [rsp+416]
  adc    r8, rax
  mov    [rsp+416], r8
  mov    r8, [rsp+424]
  adc    r8, rax
  mov    [rsp+424], r8
  mov    r8, [rsp+432]
  adc    r8, rax
  mov    [rsp+432], r8
  mov    r8, [rsp+440]
  adc    r8, rax
  mov    [rsp+440], r8
  mov    r8, [rsp+448]
  adc    r8, rax
  mov    [rsp+448], r8
  mov    r8, [rsp+456]
  adc    r8, rax
  mov    [rsp+456], r8
  mov    r8, [rsp+464]
  adc    r8, rax
  mov    [rsp+464], r8
  mov    r8, [rsp+472]
  adc    r8, rax
  mov    [rsp+472], r8
  mov    r8, [rsp+480]
  adc    r8, rax
  mov    [rsp+480], r8
  mov    r8, [rsp+488]
  adc    r8, rax
  mov    [rsp+488], r8
  mov    r8, [rsp+496]
  adc    r8, rax
  mov    [rsp+496], r8
  mov    r8, [rsp+504]
  adc    r8, rax
  mov    [rsp+504], r8
  mov    r8, [rsp+512]
  adc    r8, rax
  mov    [rsp+512], r8
  mov    r8, [rsp+520]
  adc    r8, r15
  mov    [rsp+520], r8
  xor    eax, eax
  mov    [rsp], rax
  mov    [rsp+8], rax
  mov    [rsp+16], rax
  mov    [rsp+24], rax
  mov    [rsp+32], rax
  mov    [rsp+40], rax
  mov    [rsp+48], rax
  mov    [rsp+56], rax
  mov    [rsp+64], rax
  mov    [rsp+72], rax
  mov    [rsp+80], rax
  mov    [rsp+88], rax
  mov    [rsp+96], rax
  mov    [rsp+104], rax
  mov    [rsp+112], rax
  mov    [rsp+120], rax
  mov    [rsp+128], rax
  mov    [rsp+136], rax
  mov    [rsp+144], rax
  mov    [rsp+152], rax
  mov    [rsp+160], rax
  mov    [rsp+168], rax
  mov    [rsp+176], rax
  mov    [rsp+184], rax
  mov    [rsp+192], rax
  mov    [rsp+200], rax
  mov    [rsp+208], rax
  mov    [rsp+216], rax
  mov    [rsp+224], rax
  mov    [rsp+232], rax
  mov    [rsp+240], rax
  mov    [rsp+248], rax
  mov    [rsp+256], rax
  mov    [rsp+264], rax
  xor    r14d, r14d
.Lfp2_mul_c1:
  mov    rdx, [rcx + r14 + 128]
  xor    r12d, r12d
  mulx   r9, r8, [rsp+272]
  adcx   r8, [rsp + r14]
  mov    [rsp + r14], r8
  mulx   r11, r10, [rsp+280]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 8]
  mov    [rsp + r14 + 8], r10
  mulx   r9, r8, [rsp+288]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 16]
  mov    [rsp + r14 + 16], r8
  mulx   r11, r10, [rsp+296]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 24]
  mov    [rsp + r14 + 24], r10
  mulx   r9, r8, [rsp+304]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 32]
  mov    [rsp + r14 + 32], r8
  mulx   r11, r10, [rsp+312]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 40]
  mov    [rsp + r14 + 40], r10
  mulx   r9, r8, [rsp+320]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 48]
  mov    [rsp + r14 + 48], r8
  mulx   r11, r10, [rsp+328]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 56]
  mov    [rsp + r14 + 56], r10
  mulx   r9, r8, [rsp+336]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 64]
  mov    [rsp + r14 + 64], r8
  mulx   r11, r10, [rsp+344]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 72]
  mov    [rsp + r14 + 72], r10
  mulx   r9, r8, [rsp+352]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 80]
  mov    [rsp + r14 + 80], r8
  mulx   r11, r10, [rsp+360]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 88]
  mov    [rsp + r14 + 88], r10
  mulx   r9, r8, [rsp+368]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 96]
  mov    [rsp + r14 + 96], r8
  mulx   r11, r10, [rsp+376]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 104]
  mov    [rsp + r14 + 104], r10
  mulx   r9, r8, [rsp+384]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 112]
  mov    [rsp + r14 + 112], r8
  mulx   r11, r10, [rsp+392]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 120]
  mov    [rsp + r14 + 120], r10
  mov    r13, [rsp + r14 + 128]
  adox   r11, r12
  adcx   r11, r13
  mov    [rsp + r14 + 128], r11
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  mov    rdx, [rcx + r14]
  xor    r12d, r12d
  mulx   r9, r8, [rsp+400]
  adcx   r8, [rsp + r14]
  mov    [rsp + r14], r8
  mulx   r11, r10, [rsp+408]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 8]
  mov    [rsp + r14 + 8], r10
  mulx   r9, r8, [rsp+416]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 16]
  mov    [rsp + r14 + 16], r8
  mulx   r11, r10, [rsp+424]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 24]
  mov    [rsp + r14 + 24], r10
  mulx   r9, r8, [rsp+432]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 32]
  mov    [rsp + r14 + 32], r8
  mulx   r11, r10, [rsp+440]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 40]
  mov    [rsp + r14 + 40], r10
  mulx   r9, r8, [rsp+448]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 48]
  mov    [rsp + r14 + 48], r8
  mulx   r11, r10, [rsp+456]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 56]
  mov    [rsp + r14 + 56], r10
  mulx   r9, r8, [rsp+464]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 64]
  mov    [rsp + r14 + 64], r8
  mulx   r11, r10, [rsp+472]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 72]
  mov    [rsp + r14 + 72], r10
  mulx   r9, r8, [rsp+480]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 80]
  mov    [rsp + r14 + 80], r8
  mulx   r11, r10, [rsp+488]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 88]
  mov    [rsp + r14 + 88], r10
  mulx   r9, r8, [rsp+496]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 96]
  mov    [rsp + r14 + 96], r8
  mulx   r11, r10, [rsp+504]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 104]
  mov    [rsp + r14 + 104], r10
  mulx   r9, r8, [rsp+512]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 112]
  mov    [rsp + r14 + 112], r8
  mulx   r11, r10, [rsp+520]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 120]
  mov    [rsp + r14 + 120], r10
  mov    r13, [rsp + r14 + 128]
  adox   r11, r12
  adcx   r11, r13
  mov    [rsp + r14 + 128], r11
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  mov    rdx, [rsp + r14]
  mulx   r9, r8, [rip+p_plus_1+120]
  mov    r13, [rsp + r14 + 120]
  add    r13, r8
  mov    [rsp + r14 + 120], r13
  mov    r13, [rsp + r14 + 128]
  adc    r13, r9
  mov    [rsp + r14 + 128], r13
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  add    r14, 8
  cmp    r14, 128
  jne    .Lfp2_mul_c1
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    r8, [rsp+128]
  mov    [rdi], r8
  mov    r8, [rsp+136]
  mov    [rdi+8], r8
  mov    r8, [rsp+144]
  mov    [rdi+16], r8
  mov    r8, [rsp+152]
  mov    [rdi+24], r8
  mov    r8, [rsp+160]
  mov    [rdi+32], r8
  mov    r8, [rsp+168]
  mov    [rdi+40], r8
  mov    r8, [rsp+176]
  mov    [rdi+48], r8
  mov    r8, [rsp+184]
  mov    [rdi+56], r8
  mov    r8, [rsp+192]
  mov    [rdi+64], r8
  mov    r8, [rsp+200]
  mov    [rdi+72], r8
  mov    r8, [rsp+208]
  mov    [rdi+80], r8
  mov    r8, [rsp+216]
  mov    [rdi+88], r8
  mov    r8, [rsp+224]
  mov    [rdi+96], r8
  mov    r8, [rsp+232]
  mov    [rdi+104], r8
  mov    r8, [rsp+240]
  mov    [rdi+112], r8
  mov    r8, [rsp+248]
  mov    [rdi+120], r8
  add    rsp, 656
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  pop    rbp
  pop    rbx
  ret

.global fp2_sq_c0
fp2_sq_c0:
  push   rbx
  push   rbp
  push   r12
  push   r13
  push   r14
  push   r15
  sub    rsp, 656
  mov    r8, [rsi]
  add    r8, [rsi+128]
  mov    [rsp+272], r8
  mov    r8, [rsi+8]
  adc    r8, [rsi+136]
  mov    [rsp+280], r8
  mov    r8, [rsi+16]
  adc    r8, [rsi+144]
  mov    [rsp+288], r8
  mov    r8, [rsi+24]
  adc    r8, [rsi+152]
  mov    [rsp+296], r8
  mov    r8, [rsi+32]
  adc    r8, [rsi+160]
  mov    [rsp+304], r8
  mov    r8, [rsi+40]
  adc    r8, [rsi+168]
  mov    [rsp+312], r8
  mov    r8, [rsi+48]
  adc    r8, [rsi+176]
  mov    [rsp+320], r8
  mov    r8, [rsi+56]
  adc    r8, [rsi+184]
  mov    [rsp+328], r8
  mov    r8, [rsi+64]
  adc    r8, [rsi+192]
  mov    [rsp+336], r8
  mov    r8, [rsi+72]
  adc    r8, [rsi+200]
  mov    [rsp+344], r8
  mov    r8, [rsi+80]
  adc    r8, [rsi+208]
  mov    [rsp+352], r8
  mov    r8, [rsi+88]
  adc    r8, [rsi+216]
  mov    [rsp+360], r8
  mov    r8, [rsi+96]
  adc    r8, [rsi+224]
  mov    [rsp+368], r8
  mov    r8, [rsi+104]
  adc    r8, [rsi+232]
  mov    [rsp+376], r8
  mov    r8, [rsi+112]
  adc    r8, [rsi+240]
  mov    [rsp+384], r8
  mov    r8, [rsi+120]
  adc    r8, [rsi+248]
  mov    [rsp+392], r8
  mov    r8, [rsi]
  sub    r8, [rsi+128]
  mov    [rsp+400], r8
  mov    r8, [rsi+8]
  sbb    r8, [rsi+136]
  mov    [rsp+408], r8
  mov    r8, [rsi+16]
  sbb    r8, [rsi+144]
  mov    [rsp+416], r8
  mov    r8, [rsi+24]
  sbb    r8, [rsi+152]
  mov    [rsp+424], r8
  mov    r8, [rsi+32]
  sbb    r8, [rsi+160]
  mov    [rsp+432], r8
  mov    r8, [rsi+40]
  sbb    r8, [rsi+168]
  mov    [rsp+440], r8
  mov    r8, [rsi+48]
  sbb    r8, [rsi+176]
  mov    [rsp+448], r8
  mov    r8, [rsi+56]
  sbb    r8, [rsi+184]
  mov    [rsp+456], r8
  mov    r8, [rsi+64]
  sbb    r8, [rsi+192]
  mov    [rsp+464], r8
  mov    r8, [rsi+72]
  sbb    r8, [rsi+200]
  mov    [rsp+472], r8
  mov    r8, [rsi+80]
  sbb    r8, [rsi+208]
  mov    [rsp+480], r8
  mov    r8, [rsi+88]
  sbb    r8, [rsi+216]
  mov    [rsp+488], r8
  mov    r8, [rsi+96]
  sbb    r8, [rsi+224]
  mov    [rsp+496], r8
  mov    r8, [rsi+104]
  sbb    r8, [rsi+232]
  mov    [rsp+504], r8
  mov    r8, [rsi+112]
  sbb    r8, [rsi+240]
  mov    [rsp+512], r8
  mov    r8, [rsi+120]
  sbb    r8, [rsi+248]
  mov    [rsp+520], r8
  sbb    rax, rax
  mov    rdx, [rip+p+120]
  and    rdx, rax
  mov    r8, [rsp+400]
  add    r8, rax
  mov    [rsp+400], r8
  mov    r8, [rsp+408]
  adc    r8, rax
  mov    [rsp+408], r8
  mov    r8, [rsp+416]
  adc    r8, rax
  mov    [rsp+416], r8
  mov    r8, [rsp+424]
  adc    r8, rax
  mov    [rsp+424], r8
  mov    r8, [rsp+432]
  adc    r8, rax
  mov    [rsp+432], r8
  mov    r8, [rsp+440]
  adc    r8, rax
  mov    [rsp+440], r8
  mov    r8, [rsp+448]
  adc    r8, rax
  mov    [rsp+448], r8
  mov    r8, [rsp+456]
  adc    r8, rax
  mov    [rsp+456], r8
  mov    r8, [rsp+464]
  adc    r8, rax
  mov    [rsp+464], r8
  mov    r8, [rsp+472]
  adc    r8, rax
  mov    [rsp+472], r8
  mov    r8, [rsp+480]
  adc    r8, rax
  mov    [rsp+480], r8
  mov    r8, [rsp+488]
  adc    r8, rax
  mov    [rsp+488], r8
  mov    r8, [rsp+496]
  adc    r8, rax
  mov    [rsp+496], r8
  mov    r8, [rsp+504]
  adc    r8, rax
  mov    [rsp+504], r8
  mov    r8, [rsp+512]
  adc    r8, rax
  mov    [rsp+512], r8
  mov    r8, [rsp+520]
  adc    r8, rdx
  mov    [rsp+520], r8
  mov    rax, [rsp+520]
  sar    rax, 63
  mov    rdx, [rip+p+120]
  and    rdx, rax
  mov    r8, [rsp+400]
  add    r8, rax
  mov    [rsp+400], r8
  mov    r8, [rsp+408]
  adc    r8, rax
  mov    [rsp+408], r8
  mov    r8, [rsp+416]
  adc    r8, rax
  mov    [rsp+416], r8
  mov    r8, [rsp+424]
  adc    r8, rax
  mov    [rsp+424], r8
  mov    r8, [rsp+432]
  adc    r8, rax
  mov    [rsp+432], r8
  mov    r8, [rsp+440]
  adc    r8, rax
  mov    [rsp+440], r8
  mov    r8, [rsp+448]
  adc    r8, rax
  mov    [rsp+448], r8
  mov    r8, [rsp+456]
  adc    r8, rax
  mov    [rsp+456], r8
  mov    r8, [rsp+464]
  adc    r8, rax
  mov    [rsp+464], r8
  mov    r8, [rsp+472]
  adc    r8, rax
  mov    [rsp+472], r8
  mov    r8, [rsp+480]
  adc    r8, rax
  mov    [rsp+480], r8
  mov    r8, [rsp+488]
  adc    r8, rax
  mov    [rsp+488], r8
  mov    r8, [rsp+496]
  adc    r8, rax
  mov    [rsp+496], r8
  mov    r8, [rsp+504]
  adc    r8, rax
  mov    [rsp+504], r8
  mov    r8, [rsp+512]
  adc    r8, rax
  mov    [rsp+512], r8
  mov    r8, [rsp+520]
  adc    r8, rdx
  mov    [rsp+520], r8
  xor    eax, eax
  mov    [rsp], rax
  mov    [rsp+8], rax
  mov    [rsp+16], rax
  mov    [rsp+24], rax
  mov    [rsp+32], rax
  mov    [rsp+40], rax
  mov    [rsp+48], rax
  mov    [rsp+56], rax
  mov    [rsp+64], rax
  mov    [rsp+72], rax
  mov    [rsp+80], rax
  mov    [rsp+88], rax
  mov    [rsp+96], rax
  mov    [rsp+104], rax
  mov    [rsp+112], rax
  mov    [rsp+120], rax
  mov    [rsp+128], rax
  mov    [rsp+136], rax
  mov    [rsp+144], rax
  mov    [rsp+152], rax
  mov    [rsp+160], rax
  mov    [rsp+168], rax
  mov    [rsp+176], rax
  mov    [rsp+184], rax
  mov    [rsp+192], rax
  mov    [rsp+200], rax
  mov    [rsp+208], rax
  mov    [rsp+216], rax
  mov    [rsp+224], rax
  mov    [rsp+232], rax
  mov    [rsp+240], rax
  mov    [rsp+248], rax
  mov    [rsp+256], rax
  mov    [rsp+264], rax
  xor    r14d, r14d
.Lsqc0:
  lea    rax, [rsp + r14]
  mov    rdx, [rax + 272]
  xor    r12d, r12d
  mulx   r9, r8, [rsp+400]
  adcx   r8, [rsp + r14]
  mov    [rsp + r14], r8
  mulx   r11, r10, [rsp+408]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 8]
  mov    [rsp + r14 + 8], r10
  mulx   r9, r8, [rsp+416]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 16]
  mov    [rsp + r14 + 16], r8
  mulx   r11, r10, [rsp+424]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 24]
  mov    [rsp + r14 + 24], r10
  mulx   r9, r8, [rsp+432]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 32]
  mov    [rsp + r14 + 32], r8
  mulx   r11, r10, [rsp+440]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 40]
  mov    [rsp + r14 + 40], r10
  mulx   r9, r8, [rsp+448]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 48]
  mov    [rsp + r14 + 48], r8
  mulx   r11, r10, [rsp+456]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 56]
  mov    [rsp + r14 + 56], r10
  mulx   r9, r8, [rsp+464]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 64]
  mov    [rsp + r14 + 64], r8
  mulx   r11, r10, [rsp+472]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 72]
  mov    [rsp + r14 + 72], r10
  mulx   r9, r8, [rsp+480]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 80]
  mov    [rsp + r14 + 80], r8
  mulx   r11, r10, [rsp+488]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 88]
  mov    [rsp + r14 + 88], r10
  mulx   r9, r8, [rsp+496]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 96]
  mov    [rsp + r14 + 96], r8
  mulx   r11, r10, [rsp+504]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 104]
  mov    [rsp + r14 + 104], r10
  mulx   r9, r8, [rsp+512]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 112]
  mov    [rsp + r14 + 112], r8
  mulx   r11, r10, [rsp+520]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 120]
  mov    [rsp + r14 + 120], r10
  mov    r13, [rsp + r14 + 128]
  adox   r11, r12
  adcx   r11, r13
  mov    [rsp + r14 + 128], r11
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  mov    rdx, [rsp + r14]
  mulx   r9, r8, [rip+p_plus_1+120]
  mov    r13, [rsp + r14 + 120]
  add    r13, r8
  mov    [rsp + r14 + 120], r13
  mov    r13, [rsp + r14 + 128]
  adc    r13, r9
  mov    [rsp + r14 + 128], r13
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  add    r14, 8
  cmp    r14, 128
  jne    .Lsqc0
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    r8, [rsp+128]
  mov    [rdi], r8
  mov    r8, [rsp+136]
  mov    [rdi+8], r8
  mov    r8, [rsp+144]
  mov    [rdi+16], r8
  mov    r8, [rsp+152]
  mov    [rdi+24], r8
  mov    r8, [rsp+160]
  mov    [rdi+32], r8
  mov    r8, [rsp+168]
  mov    [rdi+40], r8
  mov    r8, [rsp+176]
  mov    [rdi+48], r8
  mov    r8, [rsp+184]
  mov    [rdi+56], r8
  mov    r8, [rsp+192]
  mov    [rdi+64], r8
  mov    r8, [rsp+200]
  mov    [rdi+72], r8
  mov    r8, [rsp+208]
  mov    [rdi+80], r8
  mov    r8, [rsp+216]
  mov    [rdi+88], r8
  mov    r8, [rsp+224]
  mov    [rdi+96], r8
  mov    r8, [rsp+232]
  mov    [rdi+104], r8
  mov    r8, [rsp+240]
  mov    [rdi+112], r8
  mov    r8, [rsp+248]
  mov    [rdi+120], r8
  add    rsp, 656
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  pop    rbp
  pop    rbx
  ret

.global fp2_sq_c1
fp2_sq_c1:
  push   rbx
  push   rbp
  push   r12
  push   r13
  push   r14
  push   r15
  sub    rsp, 656
  mov    r8, [rsi]
  add    r8, [rsi]
  mov    [rsp+272], r8
  mov    r8, [rsi+8]
  adc    r8, [rsi+8]
  mov    [rsp+280], r8
  mov    r8, [rsi+16]
  adc    r8, [rsi+16]
  mov    [rsp+288], r8
  mov    r8, [rsi+24]
  adc    r8, [rsi+24]
  mov    [rsp+296], r8
  mov    r8, [rsi+32]
  adc    r8, [rsi+32]
  mov    [rsp+304], r8
  mov    r8, [rsi+40]
  adc    r8, [rsi+40]
  mov    [rsp+312], r8
  mov    r8, [rsi+48]
  adc    r8, [rsi+48]
  mov    [rsp+320], r8
  mov    r8, [rsi+56]
  adc    r8, [rsi+56]
  mov    [rsp+328], r8
  mov    r8, [rsi+64]
  adc    r8, [rsi+64]
  mov    [rsp+336], r8
  mov    r8, [rsi+72]
  adc    r8, [rsi+72]
  mov    [rsp+344], r8
  mov    r8, [rsi+80]
  adc    r8, [rsi+80]
  mov    [rsp+352], r8
  mov    r8, [rsi+88]
  adc    r8, [rsi+88]
  mov    [rsp+360], r8
  mov    r8, [rsi+96]
  adc    r8, [rsi+96]
  mov    [rsp+368], r8
  mov    r8, [rsi+104]
  adc    r8, [rsi+104]
  mov    [rsp+376], r8
  mov    r8, [rsi+112]
  adc    r8, [rsi+112]
  mov    [rsp+384], r8
  mov    r8, [rsi+120]
  adc    r8, [rsi+120]
  mov    [rsp+392], r8
  xor    eax, eax
  mov    [rsp], rax
  mov    [rsp+8], rax
  mov    [rsp+16], rax
  mov    [rsp+24], rax
  mov    [rsp+32], rax
  mov    [rsp+40], rax
  mov    [rsp+48], rax
  mov    [rsp+56], rax
  mov    [rsp+64], rax
  mov    [rsp+72], rax
  mov    [rsp+80], rax
  mov    [rsp+88], rax
  mov    [rsp+96], rax
  mov    [rsp+104], rax
  mov    [rsp+112], rax
  mov    [rsp+120], rax
  mov    [rsp+128], rax
  mov    [rsp+136], rax
  mov    [rsp+144], rax
  mov    [rsp+152], rax
  mov    [rsp+160], rax
  mov    [rsp+168], rax
  mov    [rsp+176], rax
  mov    [rsp+184], rax
  mov    [rsp+192], rax
  mov    [rsp+200], rax
  mov    [rsp+208], rax
  mov    [rsp+216], rax
  mov    [rsp+224], rax
  mov    [rsp+232], rax
  mov    [rsp+240], rax
  mov    [rsp+248], rax
  mov    [rsp+256], rax
  mov    [rsp+264], rax
  xor    r14d, r14d
.Lsqc1:
  lea    rax, [rsp + r14]
  mov    rdx, [rax + 272]
  xor    r12d, r12d
  mulx   r9, r8, [rsi+128]
  adcx   r8, [rsp + r14]
  mov    [rsp + r14], r8
  mulx   r11, r10, [rsi+136]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 8]
  mov    [rsp + r14 + 8], r10
  mulx   r9, r8, [rsi+144]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 16]
  mov    [rsp + r14 + 16], r8
  mulx   r11, r10, [rsi+152]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 24]
  mov    [rsp + r14 + 24], r10
  mulx   r9, r8, [rsi+160]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 32]
  mov    [rsp + r14 + 32], r8
  mulx   r11, r10, [rsi+168]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 40]
  mov    [rsp + r14 + 40], r10
  mulx   r9, r8, [rsi+176]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 48]
  mov    [rsp + r14 + 48], r8
  mulx   r11, r10, [rsi+184]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 56]
  mov    [rsp + r14 + 56], r10
  mulx   r9, r8, [rsi+192]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 64]
  mov    [rsp + r14 + 64], r8
  mulx   r11, r10, [rsi+200]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 72]
  mov    [rsp + r14 + 72], r10
  mulx   r9, r8, [rsi+208]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 80]
  mov    [rsp + r14 + 80], r8
  mulx   r11, r10, [rsi+216]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 88]
  mov    [rsp + r14 + 88], r10
  mulx   r9, r8, [rsi+224]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 96]
  mov    [rsp + r14 + 96], r8
  mulx   r11, r10, [rsi+232]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 104]
  mov    [rsp + r14 + 104], r10
  mulx   r9, r8, [rsi+240]
  adox   r8, r11
  adcx   r8, [rsp + r14 + 112]
  mov    [rsp + r14 + 112], r8
  mulx   r11, r10, [rsi+248]
  adox   r10, r9
  adcx   r10, [rsp + r14 + 120]
  mov    [rsp + r14 + 120], r10
  mov    r13, [rsp + r14 + 128]
  adox   r11, r12
  adcx   r11, r13
  mov    [rsp + r14 + 128], r11
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  mov    rdx, [rsp + r14]
  mulx   r9, r8, [rip+p_plus_1+120]
  mov    r13, [rsp + r14 + 120]
  add    r13, r8
  mov    [rsp + r14 + 120], r13
  mov    r13, [rsp + r14 + 128]
  adc    r13, r9
  mov    [rsp + r14 + 128], r13
  mov    r13, [rsp + r14 + 136]
  adc    r13, 0
  mov    [rsp + r14 + 136], r13
  add    r14, 8
  cmp    r14, 128
  jne    .Lsqc1
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    rbx, [rip+p]
  mov    r15, [rip+p+120]
  mov    r8, [rsp+128]
  sub    r8, rbx
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  sbb    r8, rbx
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  sbb    r8, rbx
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  sbb    r8, rbx
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  sbb    r8, rbx
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  sbb    r8, rbx
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  sbb    r8, rbx
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  sbb    r8, rbx
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  sbb    r8, rbx
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  sbb    r8, rbx
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  sbb    r8, rbx
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  sbb    r8, rbx
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  sbb    r8, rbx
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  sbb    r8, rbx
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  sbb    r8, rbx
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  sbb    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  sbb    r8, 0
  mov    [rsp+256], r8
  sbb    rax, rax
  and    r15, rax
  mov    r8, [rsp+128]
  add    r8, rax
  mov    [rsp+128], r8
  mov    r8, [rsp+136]
  adc    r8, rax
  mov    [rsp+136], r8
  mov    r8, [rsp+144]
  adc    r8, rax
  mov    [rsp+144], r8
  mov    r8, [rsp+152]
  adc    r8, rax
  mov    [rsp+152], r8
  mov    r8, [rsp+160]
  adc    r8, rax
  mov    [rsp+160], r8
  mov    r8, [rsp+168]
  adc    r8, rax
  mov    [rsp+168], r8
  mov    r8, [rsp+176]
  adc    r8, rax
  mov    [rsp+176], r8
  mov    r8, [rsp+184]
  adc    r8, rax
  mov    [rsp+184], r8
  mov    r8, [rsp+192]
  adc    r8, rax
  mov    [rsp+192], r8
  mov    r8, [rsp+200]
  adc    r8, rax
  mov    [rsp+200], r8
  mov    r8, [rsp+208]
  adc    r8, rax
  mov    [rsp+208], r8
  mov    r8, [rsp+216]
  adc    r8, rax
  mov    [rsp+216], r8
  mov    r8, [rsp+224]
  adc    r8, rax
  mov    [rsp+224], r8
  mov    r8, [rsp+232]
  adc    r8, rax
  mov    [rsp+232], r8
  mov    r8, [rsp+240]
  adc    r8, rax
  mov    [rsp+240], r8
  mov    r8, [rsp+248]
  adc    r8, r15
  mov    [rsp+248], r8
  mov    r8, [rsp+256]
  adc    r8, 0
  mov    [rsp+256], r8
  mov    r8, [rsp+128]
  mov    [rdi], r8
  mov    r8, [rsp+136]
  mov    [rdi+8], r8
  mov    r8, [rsp+144]
  mov    [rdi+16], r8
  mov    r8, [rsp+152]
  mov    [rdi+24], r8
  mov    r8, [rsp+160]
  mov    [rdi+32], r8
  mov    r8, [rsp+168]
  mov    [rdi+40], r8
  mov    r8, [rsp+176]
  mov    [rdi+48], r8
  mov    r8, [rsp+184]
  mov    [rdi+56], r8
  mov    r8, [rsp+192]
  mov    [rdi+64], r8
  mov    r8, [rsp+200]
  mov    [rdi+72], r8
  mov    r8, [rsp+208]
  mov    [rdi+80], r8
  mov    r8, [rsp+216]
  mov    [rdi+88], r8
  mov    r8, [rsp+224]
  mov    [rdi+96], r8
  mov    r8, [rsp+232]
  mov    [rdi+104], r8
  mov    r8, [rsp+240]
  mov    [rdi+112], r8
  mov    r8, [rsp+248]
  mov    [rdi+120], r8
  add    rsp, 656
  pop    r15
  pop    r14
  pop    r13
  pop    r12
  pop    rbp
  pop    rbx
  ret

