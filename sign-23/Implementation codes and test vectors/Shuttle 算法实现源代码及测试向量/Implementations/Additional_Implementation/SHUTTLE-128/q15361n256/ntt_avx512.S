/* Auto-generated AVX-512BW full-ZMM NTT.  Edit gen_ntt.py. */
.text
.global s256_ntt_avx512
.global s256_invntt_tomont_avx512
.global s256_pointwise_avx512

.macro load_consts512 qptr
  vmovdqa64 0(\qptr),%zmm16
  vmovdqa64 64(\qptr),%zmm17
  vmovdqa64 128(\qptr),%zmm29
.endm
.macro montmul512 out,in
  vpmullw  %zmm18,\in,%zmm20
  vpmulhw  %zmm19,\in,\out
  vpmulhw  %zmm16,%zmm20,%zmm20
  vpsubw   %zmm20,\out,\out
.endm
.macro red16_512 a
  vpmulhw   %zmm17,\a,%zmm21
  vpmulhrsw %zmm29,%zmm21,%zmm21
  vpmullw   %zmm16,%zmm21,%zmm21
  vpsubw    %zmm21,\a,\a
.endm
.macro dmul512 out,x,y
  vpmullw  \x,\y,%zmm20
  vpmullw  %zmm28,%zmm20,%zmm20
  vpmulhw  \x,\y,\out
  vpmulhw  %zmm16,%zmm20,%zmm20
  vpsubw   %zmm20,\out,\out
.endm
.macro ctbf512_lazy a,b
  montmul512 %zmm22,\b
  vpsubw  %zmm22,\a,\b
  vpaddw  %zmm22,\a,\a
.endm
.macro ctbf512_red a,b
  red16_512 \a
  montmul512 %zmm22,\b
  vpsubw  %zmm22,\a,\b
  vpaddw  %zmm22,\a,\a
.endm
.macro gsbf512_lazy a,b
  vpsubw  \b,\a,%zmm22
  vpaddw  \b,\a,\a
  montmul512 \b,%zmm22
.endm
.macro gsbf512_red a,b
  red16_512 \a
  red16_512 \b
  vpsubw  \b,\a,%zmm22
  vpaddw  \b,\a,\a
  montmul512 \b,%zmm22
.endm
.macro shuf16_512 r0,r1
  vmovdqa64 \r0,%zmm31
  vmovdqa64 s256_shuf16_c(%rip),%zmm30
  vpermt2w \r1,%zmm30,%zmm31
  vmovdqa64 s256_shuf16_d(%rip),%zmm30
  vpermt2w \r1,%zmm30,\r0
  vmovdqa64 \r0,\r1
  vmovdqa64 %zmm31,\r0
.endm
.macro shuf8_512 r0,r1
  vmovdqa64 \r0,%zmm31
  vmovdqa64 s256_shuf8_c(%rip),%zmm30
  vpermt2w \r1,%zmm30,%zmm31
  vmovdqa64 s256_shuf8_d(%rip),%zmm30
  vpermt2w \r1,%zmm30,\r0
  vmovdqa64 \r0,\r1
  vmovdqa64 %zmm31,\r0
.endm
.macro shuf4_512 r0,r1
  vmovdqa64 \r0,%zmm31
  vmovdqa64 s256_shuf4_c(%rip),%zmm30
  vpermt2w \r1,%zmm30,%zmm31
  vmovdqa64 s256_shuf4_d(%rip),%zmm30
  vpermt2w \r1,%zmm30,\r0
  vmovdqa64 \r0,\r1
  vmovdqa64 %zmm31,\r0
.endm
.macro shuf2_512 r0,r1
  vmovdqa64 \r0,%zmm31
  vmovdqa64 s256_shuf2_c(%rip),%zmm30
  vpermt2w \r1,%zmm30,%zmm31
  vmovdqa64 s256_shuf2_d(%rip),%zmm30
  vpermt2w \r1,%zmm30,\r0
  vmovdqa64 \r0,\r1
  vmovdqa64 %zmm31,\r0
.endm
.macro shuf1_512 r0,r1
  vmovdqa64 \r0,%zmm31
  vmovdqa64 s256_shuf1_c(%rip),%zmm30
  vpermt2w \r1,%zmm30,%zmm31
  vmovdqa64 s256_shuf1_d(%rip),%zmm30
  vpermt2w \r1,%zmm30,\r0
  vmovdqa64 \r0,\r1
  vmovdqa64 %zmm31,\r0
.endm


s256_ntt_avx512:
  load_consts512 %rsi
  vmovdqu16 0(%rdi),%zmm0
  vmovdqu16 64(%rdi),%zmm1
  vmovdqu16 128(%rdi),%zmm2
  vmovdqu16 192(%rdi),%zmm3
  vmovdqu16 256(%rdi),%zmm4
  vmovdqu16 320(%rdi),%zmm5
  vmovdqu16 384(%rdi),%zmm6
  vmovdqu16 448(%rdi),%zmm7
  vmovdqa64 0(%rdx),%zmm18
  vmovdqa64 64(%rdx),%zmm19
  ctbf512_lazy %zmm0,%zmm4
  vmovdqa64 128(%rdx),%zmm18
  vmovdqa64 192(%rdx),%zmm19
  ctbf512_lazy %zmm1,%zmm5
  vmovdqa64 256(%rdx),%zmm18
  vmovdqa64 320(%rdx),%zmm19
  ctbf512_lazy %zmm2,%zmm6
  vmovdqa64 384(%rdx),%zmm18
  vmovdqa64 448(%rdx),%zmm19
  ctbf512_lazy %zmm3,%zmm7
  vmovdqa64 512(%rdx),%zmm18
  vmovdqa64 576(%rdx),%zmm19
  ctbf512_red %zmm0,%zmm2
  vmovdqa64 640(%rdx),%zmm18
  vmovdqa64 704(%rdx),%zmm19
  ctbf512_red %zmm1,%zmm3
  vmovdqa64 768(%rdx),%zmm18
  vmovdqa64 832(%rdx),%zmm19
  ctbf512_red %zmm4,%zmm6
  vmovdqa64 896(%rdx),%zmm18
  vmovdqa64 960(%rdx),%zmm19
  ctbf512_red %zmm5,%zmm7
  vmovdqa64 1024(%rdx),%zmm18
  vmovdqa64 1088(%rdx),%zmm19
  ctbf512_lazy %zmm0,%zmm1
  vmovdqa64 1152(%rdx),%zmm18
  vmovdqa64 1216(%rdx),%zmm19
  ctbf512_lazy %zmm2,%zmm3
  vmovdqa64 1280(%rdx),%zmm18
  vmovdqa64 1344(%rdx),%zmm19
  ctbf512_lazy %zmm4,%zmm5
  vmovdqa64 1408(%rdx),%zmm18
  vmovdqa64 1472(%rdx),%zmm19
  ctbf512_lazy %zmm6,%zmm7
  shuf16_512 %zmm0,%zmm1
  vmovdqa64 1536(%rdx),%zmm18
  vmovdqa64 1600(%rdx),%zmm19
  ctbf512_red %zmm0,%zmm1
  shuf16_512 %zmm2,%zmm3
  vmovdqa64 1664(%rdx),%zmm18
  vmovdqa64 1728(%rdx),%zmm19
  ctbf512_red %zmm2,%zmm3
  shuf16_512 %zmm4,%zmm5
  vmovdqa64 1792(%rdx),%zmm18
  vmovdqa64 1856(%rdx),%zmm19
  ctbf512_red %zmm4,%zmm5
  shuf16_512 %zmm6,%zmm7
  vmovdqa64 1920(%rdx),%zmm18
  vmovdqa64 1984(%rdx),%zmm19
  ctbf512_red %zmm6,%zmm7
  shuf8_512 %zmm0,%zmm1
  vmovdqa64 2048(%rdx),%zmm18
  vmovdqa64 2112(%rdx),%zmm19
  ctbf512_lazy %zmm0,%zmm1
  shuf8_512 %zmm2,%zmm3
  vmovdqa64 2176(%rdx),%zmm18
  vmovdqa64 2240(%rdx),%zmm19
  ctbf512_lazy %zmm2,%zmm3
  shuf8_512 %zmm4,%zmm5
  vmovdqa64 2304(%rdx),%zmm18
  vmovdqa64 2368(%rdx),%zmm19
  ctbf512_lazy %zmm4,%zmm5
  shuf8_512 %zmm6,%zmm7
  vmovdqa64 2432(%rdx),%zmm18
  vmovdqa64 2496(%rdx),%zmm19
  ctbf512_lazy %zmm6,%zmm7
  shuf4_512 %zmm0,%zmm1
  vmovdqa64 2560(%rdx),%zmm18
  vmovdqa64 2624(%rdx),%zmm19
  ctbf512_red %zmm0,%zmm1
  shuf4_512 %zmm2,%zmm3
  vmovdqa64 2688(%rdx),%zmm18
  vmovdqa64 2752(%rdx),%zmm19
  ctbf512_red %zmm2,%zmm3
  shuf4_512 %zmm4,%zmm5
  vmovdqa64 2816(%rdx),%zmm18
  vmovdqa64 2880(%rdx),%zmm19
  ctbf512_red %zmm4,%zmm5
  shuf4_512 %zmm6,%zmm7
  vmovdqa64 2944(%rdx),%zmm18
  vmovdqa64 3008(%rdx),%zmm19
  ctbf512_red %zmm6,%zmm7
  shuf2_512 %zmm0,%zmm1
  vmovdqa64 3072(%rdx),%zmm18
  vmovdqa64 3136(%rdx),%zmm19
  ctbf512_lazy %zmm0,%zmm1
  shuf2_512 %zmm2,%zmm3
  vmovdqa64 3200(%rdx),%zmm18
  vmovdqa64 3264(%rdx),%zmm19
  ctbf512_lazy %zmm2,%zmm3
  shuf2_512 %zmm4,%zmm5
  vmovdqa64 3328(%rdx),%zmm18
  vmovdqa64 3392(%rdx),%zmm19
  ctbf512_lazy %zmm4,%zmm5
  shuf2_512 %zmm6,%zmm7
  vmovdqa64 3456(%rdx),%zmm18
  vmovdqa64 3520(%rdx),%zmm19
  ctbf512_lazy %zmm6,%zmm7
  shuf1_512 %zmm0,%zmm1
  vmovdqa64 3584(%rdx),%zmm18
  vmovdqa64 3648(%rdx),%zmm19
  ctbf512_red %zmm0,%zmm1
  shuf1_512 %zmm2,%zmm3
  vmovdqa64 3712(%rdx),%zmm18
  vmovdqa64 3776(%rdx),%zmm19
  ctbf512_red %zmm2,%zmm3
  shuf1_512 %zmm4,%zmm5
  vmovdqa64 3840(%rdx),%zmm18
  vmovdqa64 3904(%rdx),%zmm19
  ctbf512_red %zmm4,%zmm5
  shuf1_512 %zmm6,%zmm7
  vmovdqa64 3968(%rdx),%zmm18
  vmovdqa64 4032(%rdx),%zmm19
  ctbf512_red %zmm6,%zmm7
  vmovdqu16 %zmm0,0(%rdi)
  vmovdqu16 %zmm1,64(%rdi)
  vmovdqu16 %zmm2,128(%rdi)
  vmovdqu16 %zmm3,192(%rdi)
  vmovdqu16 %zmm4,256(%rdi)
  vmovdqu16 %zmm5,320(%rdi)
  vmovdqu16 %zmm6,384(%rdi)
  vmovdqu16 %zmm7,448(%rdi)
  vzeroupper
  ret

s256_invntt_tomont_avx512:
  load_consts512 %rsi
  vmovdqu16 0(%rdi),%zmm0
  vmovdqu16 64(%rdi),%zmm1
  vmovdqu16 128(%rdi),%zmm2
  vmovdqu16 192(%rdi),%zmm3
  vmovdqu16 256(%rdi),%zmm4
  vmovdqu16 320(%rdi),%zmm5
  vmovdqu16 384(%rdi),%zmm6
  vmovdqu16 448(%rdi),%zmm7
  vmovdqa64 0(%rdx),%zmm18
  vmovdqa64 64(%rdx),%zmm19
  gsbf512_red %zmm6,%zmm7
  shuf1_512 %zmm6,%zmm7
  vmovdqa64 128(%rdx),%zmm18
  vmovdqa64 192(%rdx),%zmm19
  gsbf512_red %zmm4,%zmm5
  shuf1_512 %zmm4,%zmm5
  vmovdqa64 256(%rdx),%zmm18
  vmovdqa64 320(%rdx),%zmm19
  gsbf512_red %zmm2,%zmm3
  shuf1_512 %zmm2,%zmm3
  vmovdqa64 384(%rdx),%zmm18
  vmovdqa64 448(%rdx),%zmm19
  gsbf512_red %zmm0,%zmm1
  shuf1_512 %zmm0,%zmm1
  vmovdqa64 512(%rdx),%zmm18
  vmovdqa64 576(%rdx),%zmm19
  gsbf512_lazy %zmm6,%zmm7
  shuf2_512 %zmm6,%zmm7
  vmovdqa64 640(%rdx),%zmm18
  vmovdqa64 704(%rdx),%zmm19
  gsbf512_lazy %zmm4,%zmm5
  shuf2_512 %zmm4,%zmm5
  vmovdqa64 768(%rdx),%zmm18
  vmovdqa64 832(%rdx),%zmm19
  gsbf512_lazy %zmm2,%zmm3
  shuf2_512 %zmm2,%zmm3
  vmovdqa64 896(%rdx),%zmm18
  vmovdqa64 960(%rdx),%zmm19
  gsbf512_lazy %zmm0,%zmm1
  shuf2_512 %zmm0,%zmm1
  vmovdqa64 1024(%rdx),%zmm18
  vmovdqa64 1088(%rdx),%zmm19
  gsbf512_red %zmm6,%zmm7
  shuf4_512 %zmm6,%zmm7
  vmovdqa64 1152(%rdx),%zmm18
  vmovdqa64 1216(%rdx),%zmm19
  gsbf512_red %zmm4,%zmm5
  shuf4_512 %zmm4,%zmm5
  vmovdqa64 1280(%rdx),%zmm18
  vmovdqa64 1344(%rdx),%zmm19
  gsbf512_red %zmm2,%zmm3
  shuf4_512 %zmm2,%zmm3
  vmovdqa64 1408(%rdx),%zmm18
  vmovdqa64 1472(%rdx),%zmm19
  gsbf512_red %zmm0,%zmm1
  shuf4_512 %zmm0,%zmm1
  vmovdqa64 1536(%rdx),%zmm18
  vmovdqa64 1600(%rdx),%zmm19
  gsbf512_lazy %zmm6,%zmm7
  shuf8_512 %zmm6,%zmm7
  vmovdqa64 1664(%rdx),%zmm18
  vmovdqa64 1728(%rdx),%zmm19
  gsbf512_lazy %zmm4,%zmm5
  shuf8_512 %zmm4,%zmm5
  vmovdqa64 1792(%rdx),%zmm18
  vmovdqa64 1856(%rdx),%zmm19
  gsbf512_lazy %zmm2,%zmm3
  shuf8_512 %zmm2,%zmm3
  vmovdqa64 1920(%rdx),%zmm18
  vmovdqa64 1984(%rdx),%zmm19
  gsbf512_lazy %zmm0,%zmm1
  shuf8_512 %zmm0,%zmm1
  vmovdqa64 2048(%rdx),%zmm18
  vmovdqa64 2112(%rdx),%zmm19
  gsbf512_red %zmm6,%zmm7
  shuf16_512 %zmm6,%zmm7
  vmovdqa64 2176(%rdx),%zmm18
  vmovdqa64 2240(%rdx),%zmm19
  gsbf512_red %zmm4,%zmm5
  shuf16_512 %zmm4,%zmm5
  vmovdqa64 2304(%rdx),%zmm18
  vmovdqa64 2368(%rdx),%zmm19
  gsbf512_red %zmm2,%zmm3
  shuf16_512 %zmm2,%zmm3
  vmovdqa64 2432(%rdx),%zmm18
  vmovdqa64 2496(%rdx),%zmm19
  gsbf512_red %zmm0,%zmm1
  shuf16_512 %zmm0,%zmm1
  vmovdqa64 2560(%rdx),%zmm18
  vmovdqa64 2624(%rdx),%zmm19
  gsbf512_lazy %zmm6,%zmm7
  vmovdqa64 2688(%rdx),%zmm18
  vmovdqa64 2752(%rdx),%zmm19
  gsbf512_lazy %zmm4,%zmm5
  vmovdqa64 2816(%rdx),%zmm18
  vmovdqa64 2880(%rdx),%zmm19
  gsbf512_lazy %zmm2,%zmm3
  vmovdqa64 2944(%rdx),%zmm18
  vmovdqa64 3008(%rdx),%zmm19
  gsbf512_lazy %zmm0,%zmm1
  vmovdqa64 3072(%rdx),%zmm18
  vmovdqa64 3136(%rdx),%zmm19
  gsbf512_lazy %zmm5,%zmm7
  vmovdqa64 3200(%rdx),%zmm18
  vmovdqa64 3264(%rdx),%zmm19
  gsbf512_red %zmm4,%zmm6
  vmovdqa64 3328(%rdx),%zmm18
  vmovdqa64 3392(%rdx),%zmm19
  gsbf512_lazy %zmm1,%zmm3
  vmovdqa64 3456(%rdx),%zmm18
  vmovdqa64 3520(%rdx),%zmm19
  gsbf512_red %zmm0,%zmm2
  vmovdqa64 3584(%rdx),%zmm18
  vmovdqa64 3648(%rdx),%zmm19
  gsbf512_lazy %zmm3,%zmm7
  vmovdqa64 3712(%rdx),%zmm18
  vmovdqa64 3776(%rdx),%zmm19
  gsbf512_lazy %zmm2,%zmm6
  vmovdqa64 3840(%rdx),%zmm18
  vmovdqa64 3904(%rdx),%zmm19
  gsbf512_red %zmm1,%zmm5
  vmovdqa64 3968(%rdx),%zmm18
  vmovdqa64 4032(%rdx),%zmm19
  gsbf512_lazy %zmm0,%zmm4
  /* final 1/n scale: red16 then montmul(NINV_TOMONT) */
  vmovdqa64 0(%rcx),%zmm18
  vmovdqa64 64(%rcx),%zmm19
  red16_512 %zmm0
  montmul512 %zmm0,%zmm0
  red16_512 %zmm1
  montmul512 %zmm1,%zmm1
  red16_512 %zmm2
  montmul512 %zmm2,%zmm2
  red16_512 %zmm3
  montmul512 %zmm3,%zmm3
  red16_512 %zmm4
  montmul512 %zmm4,%zmm4
  red16_512 %zmm5
  montmul512 %zmm5,%zmm5
  red16_512 %zmm6
  montmul512 %zmm6,%zmm6
  red16_512 %zmm7
  montmul512 %zmm7,%zmm7
  vmovdqu16 %zmm0,0(%rdi)
  vmovdqu16 %zmm1,64(%rdi)
  vmovdqu16 %zmm2,128(%rdi)
  vmovdqu16 %zmm3,192(%rdi)
  vmovdqu16 %zmm4,256(%rdi)
  vmovdqu16 %zmm5,320(%rdi)
  vmovdqu16 %zmm6,384(%rdi)
  vmovdqu16 %zmm7,448(%rdi)
  vzeroupper
  ret

s256_pointwise_avx512:
  load_consts512 %rcx
  vmovdqa64 s256_ntt512_qinv(%rip),%zmm28
  vmovdqu16 0(%rsi),%zmm0
  vmovdqu16 0(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,0(%rdi)
  vmovdqu16 64(%rsi),%zmm0
  vmovdqu16 64(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,64(%rdi)
  vmovdqu16 128(%rsi),%zmm0
  vmovdqu16 128(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,128(%rdi)
  vmovdqu16 192(%rsi),%zmm0
  vmovdqu16 192(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,192(%rdi)
  vmovdqu16 256(%rsi),%zmm0
  vmovdqu16 256(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,256(%rdi)
  vmovdqu16 320(%rsi),%zmm0
  vmovdqu16 320(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,320(%rdi)
  vmovdqu16 384(%rsi),%zmm0
  vmovdqu16 384(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,384(%rdi)
  vmovdqu16 448(%rsi),%zmm0
  vmovdqu16 448(%rdx),%zmm1
  red16_512 %zmm0
  red16_512 %zmm1
  dmul512 %zmm21,%zmm0,%zmm1
  vmovdqu16 %zmm21,448(%rdi)
  vzeroupper
  ret


/* nttunpack: standard-order int32 samples -> avx512 NTT layout.
 * Kyber-style forward shuffle ladder, no butterflies; narrows int32 [0,q) -> int16.
 *   rdi = dst (int16[n]), rsi = src (int32[n]). */
.global s256_nttunpack_avx512
s256_nttunpack_avx512:
  vmovdqu32 0(%rsi),%zmm20
  vpmovdw %zmm20,%ymm0
  vmovdqu32 64(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm0,%zmm0
  vmovdqu32 128(%rsi),%zmm20
  vpmovdw %zmm20,%ymm1
  vmovdqu32 192(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm1,%zmm1
  vmovdqu32 256(%rsi),%zmm20
  vpmovdw %zmm20,%ymm2
  vmovdqu32 320(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm2,%zmm2
  vmovdqu32 384(%rsi),%zmm20
  vpmovdw %zmm20,%ymm3
  vmovdqu32 448(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm3,%zmm3
  vmovdqu32 512(%rsi),%zmm20
  vpmovdw %zmm20,%ymm4
  vmovdqu32 576(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm4,%zmm4
  vmovdqu32 640(%rsi),%zmm20
  vpmovdw %zmm20,%ymm5
  vmovdqu32 704(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm5,%zmm5
  vmovdqu32 768(%rsi),%zmm20
  vpmovdw %zmm20,%ymm6
  vmovdqu32 832(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm6,%zmm6
  vmovdqu32 896(%rsi),%zmm20
  vpmovdw %zmm20,%ymm7
  vmovdqu32 960(%rsi),%zmm20
  vpmovdw %zmm20,%ymm21
  vinserti64x4 $1,%ymm21,%zmm7,%zmm7
  shuf16_512 %zmm0,%zmm1
  shuf16_512 %zmm2,%zmm3
  shuf16_512 %zmm4,%zmm5
  shuf16_512 %zmm6,%zmm7
  shuf8_512 %zmm0,%zmm1
  shuf8_512 %zmm2,%zmm3
  shuf8_512 %zmm4,%zmm5
  shuf8_512 %zmm6,%zmm7
  shuf4_512 %zmm0,%zmm1
  shuf4_512 %zmm2,%zmm3
  shuf4_512 %zmm4,%zmm5
  shuf4_512 %zmm6,%zmm7
  shuf2_512 %zmm0,%zmm1
  shuf2_512 %zmm2,%zmm3
  shuf2_512 %zmm4,%zmm5
  shuf2_512 %zmm6,%zmm7
  shuf1_512 %zmm0,%zmm1
  shuf1_512 %zmm2,%zmm3
  shuf1_512 %zmm4,%zmm5
  shuf1_512 %zmm6,%zmm7
  vmovdqu16 %zmm0,0(%rdi)
  vmovdqu16 %zmm1,64(%rdi)
  vmovdqu16 %zmm2,128(%rdi)
  vmovdqu16 %zmm3,192(%rdi)
  vmovdqu16 %zmm4,256(%rdi)
  vmovdqu16 %zmm5,320(%rdi)
  vmovdqu16 %zmm6,384(%rdi)
  vmovdqu16 %zmm7,448(%rdi)
  vzeroupper
  ret

.section .note.GNU-stack,"",@progbits
