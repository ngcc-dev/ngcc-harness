/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * User-space Hygon CIS SM4 backend, adapted from the Hygon Linux kernel
 * sm4-cis x86 implementation. Only ECB primitives are exposed here; PRG
 * counter construction remains in prg.c.
 */

#define rRIP (%rip)

#define RX0 %xmm0
#define RX1 %xmm1
#define RX2 %xmm2
#define RX3 %xmm3

#define SM4_ROUND_INST_SM4RK \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc2, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc2, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc2, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc2, 0x03

#define SM4_ROUND_INST_SM4RND \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xc1, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xc1, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xc1, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xc1, 0x03

#define SM4RK_0_1 \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc1, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc1, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc1, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xc1, 0x03

#define SM4RK_2_1 \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xd1, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xd1, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xd1, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xd1, 0x03

#define SM4RK_4_1 \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xe1, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xe1, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xe1, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xe1, 0x03

#define SM4RK_6_1 \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xf1, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xf1, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xf1, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8b, 0xf1, 0x03

#define SM4RND_0_1 SM4_ROUND_INST_SM4RND

#define SM4RND_2_3 \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xd3, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xd3, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xd3, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xd3, 0x03

#define SM4RND_4_5 \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xe5, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xe5, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xe5, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xe5, 0x03

#define SM4RND_6_7 \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xf7, 0x00; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xf7, 0x01; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xf7, 0x02; \
  .byte 0x66, 0x0f, 0x3a, 0x8a, 0xf7, 0x03

.macro HYGON_CIS_BUILD_RK_FROM_KEY key_reg
  movdqu .Lbswap32_mask rRIP, RX3
  movdqu (\key_reg), RX0
  movdqu .L_sm4_fk rRIP, RX1
  pshufb RX3, RX0
  pxor RX1, RX0
  movq $8, %r11
  leaq .L_sm4_ck rRIP, %rax
  movq %r10, %rdi

.Lhygon_prg_key_loop_\@:
  movdqu (%rax), RX2
  SM4_ROUND_INST_SM4RK
  movdqu RX0, (%rdi)
  addq $16, %rdi
  addq $16, %rax
  decq %r11
  jne .Lhygon_prg_key_loop_\@
.endm

.macro HYGON_CIS_ENC_BLOCK src_reg dst_reg
  movq %r10, %r11
  leaq 128(%r11), %rax
  movdqu (\src_reg), RX0
  pshufb RX3, RX0

.Lhygon_prg_blk_loop_\@:
  movdqu (%r11), RX1
  SM4_ROUND_INST_SM4RND
  addq $16, %r11
  cmpq %rax, %r11
  jne .Lhygon_prg_blk_loop_\@

  pshufd $0x1b, RX0, RX0
  pshufb RX3, RX0
  movdqu RX0, (\dst_reg)
.endm

.macro HYGON_CIS_BUILD_RK4_FROM_KEYS
  movdqu (%rdi), %xmm0
  movq %rsi, %rax
  movdqu (%rdi,%rax), %xmm2
  addq %rsi, %rax
  movdqu (%rdi,%rax), %xmm4
  addq %rsi, %rax
  movdqu (%rdi,%rax), %xmm6

  pshufb .Lbswap32_mask rRIP, %xmm0
  pshufb .Lbswap32_mask rRIP, %xmm2
  pshufb .Lbswap32_mask rRIP, %xmm4
  pshufb .Lbswap32_mask rRIP, %xmm6

  movdqu .L_sm4_fk rRIP, %xmm1
  pxor %xmm1, %xmm0
  pxor %xmm1, %xmm2
  pxor %xmm1, %xmm4
  pxor %xmm1, %xmm6

  movq $8, %r11
  leaq .L_sm4_ck rRIP, %rax
  movq %r12, %r10

.Lhygon_prg_key4_loop_\@:
  movdqu (%rax), %xmm1
  SM4RK_0_1
  SM4RK_2_1
  SM4RK_4_1
  SM4RK_6_1
  movdqu %xmm0, (%r10)
  movdqu %xmm2, 128(%r10)
  movdqu %xmm4, 256(%r10)
  movdqu %xmm6, 384(%r10)
  addq $16, %r10
  addq $16, %rax
  decq %r11
  jne .Lhygon_prg_key4_loop_\@
.endm

.macro HYGON_CIS_ENC4_SAME_SRC src_reg
  movdqu (\src_reg), %xmm0
  movdqa %xmm0, %xmm2
  movdqa %xmm0, %xmm4
  movdqa %xmm0, %xmm6

  pshufb .Lbswap32_mask rRIP, %xmm0
  pshufb .Lbswap32_mask rRIP, %xmm2
  pshufb .Lbswap32_mask rRIP, %xmm4
  pshufb .Lbswap32_mask rRIP, %xmm6

  movq $8, %r11
  movq %r12, %r10

.Lhygon_prg_blk4_loop_\@:
  movdqu (%r10), %xmm1
  movdqu 128(%r10), %xmm3
  movdqu 256(%r10), %xmm5
  movdqu 384(%r10), %xmm7
  SM4RND_0_1
  SM4RND_2_3
  SM4RND_4_5
  SM4RND_6_7
  addq $16, %r10
  decq %r11
  jne .Lhygon_prg_blk4_loop_\@

  pshufd $0x1b, %xmm0, %xmm0
  pshufd $0x1b, %xmm2, %xmm2
  pshufd $0x1b, %xmm4, %xmm4
  pshufd $0x1b, %xmm6, %xmm6
  pshufb .Lbswap32_mask rRIP, %xmm0
  pshufb .Lbswap32_mask rRIP, %xmm2
  pshufb .Lbswap32_mask rRIP, %xmm4
  pshufb .Lbswap32_mask rRIP, %xmm6
.endm

.macro HYGON_CIS_ENC4_LANE_SRCS src_reg offset
  movdqu \offset(\src_reg), %xmm0
  movdqu \offset+32(\src_reg), %xmm2
  movdqu \offset+64(\src_reg), %xmm4
  movdqu \offset+96(\src_reg), %xmm6

  pshufb .Lbswap32_mask rRIP, %xmm0
  pshufb .Lbswap32_mask rRIP, %xmm2
  pshufb .Lbswap32_mask rRIP, %xmm4
  pshufb .Lbswap32_mask rRIP, %xmm6

  movq $8, %r11
  movq %r12, %r10

.Lhygon_prg_blk4_lane_loop_\@:
  movdqu (%r10), %xmm1
  movdqu 128(%r10), %xmm3
  movdqu 256(%r10), %xmm5
  movdqu 384(%r10), %xmm7
  SM4RND_0_1
  SM4RND_2_3
  SM4RND_4_5
  SM4RND_6_7
  addq $16, %r10
  decq %r11
  jne .Lhygon_prg_blk4_lane_loop_\@

  pshufd $0x1b, %xmm0, %xmm0
  pshufd $0x1b, %xmm2, %xmm2
  pshufd $0x1b, %xmm4, %xmm4
  pshufd $0x1b, %xmm6, %xmm6
  pshufb .Lbswap32_mask rRIP, %xmm0
  pshufb .Lbswap32_mask rRIP, %xmm2
  pshufb .Lbswap32_mask rRIP, %xmm4
  pshufb .Lbswap32_mask rRIP, %xmm6
.endm

.macro HYGON_CIS_STORE4_STRIDED base_reg stride_reg offset
  movdqu %xmm0, \offset(\base_reg)
  movq \stride_reg, %r10
  movdqu %xmm2, \offset(\base_reg,%r10)
  leaq (\stride_reg,\stride_reg), %r10
  movdqu %xmm4, \offset(\base_reg,%r10)
  addq \stride_reg, %r10
  movdqu %xmm6, \offset(\base_reg,%r10)
.endm

.macro HYGON_CIS_STORE4_PTRS_OFFSET ptrs_reg offset_reg
  movq 0(\ptrs_reg), %r10
  addq \offset_reg, %r10
  movdqu %xmm0, (%r10)
  movq 8(\ptrs_reg), %r10
  addq \offset_reg, %r10
  movdqu %xmm2, (%r10)
  movq 16(\ptrs_reg), %r10
  addq \offset_reg, %r10
  movdqu %xmm4, (%r10)
  movq 24(\ptrs_reg), %r10
  addq \offset_reg, %r10
  movdqu %xmm6, (%r10)
.endm

.macro HYGON_CIS_STORE4_PTRS_4B ptrs_reg offset
  movq 0(\ptrs_reg), %r10
  movd %xmm0, \offset(%r10)
  movq 8(\ptrs_reg), %r10
  movd %xmm2, \offset(%r10)
  movq 16(\ptrs_reg), %r10
  movd %xmm4, \offset(%r10)
  movq 24(\ptrs_reg), %r10
  movd %xmm6, \offset(%r10)
.endm

.section .rodata
.align 16

.L_sm4_fk:
  .long 0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc

.L_sm4_ck:
  .long 0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269
  .long 0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9
  .long 0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249
  .long 0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9
  .long 0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229
  .long 0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299
  .long 0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209
  .long 0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279

.Lbswap32_mask:
  .byte 0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04
  .byte 0x0b, 0x0a, 0x09, 0x08, 0x0f, 0x0e, 0x0d, 0x0c

.text

.globl hygon_cis_sm4_set_key
.type hygon_cis_sm4_set_key, @function
.align 16
hygon_cis_sm4_set_key:
  movdqu .Lbswap32_mask rRIP, RX3
  movdqu (%rdi), RX0
  movdqu .L_sm4_fk rRIP, RX1
  pshufb RX3, RX0
  pxor RX1, RX0
  movq $8, %r8
  leaq .L_sm4_ck rRIP, %rdx

.Lhygon_key_loop:
  movdqu (%rdx), RX2
  SM4_ROUND_INST_SM4RK
  movdqu RX0, (%rsi)
  addq $16, %rsi
  addq $16, %rdx
  decq %r8
  jne .Lhygon_key_loop

  ret
.size hygon_cis_sm4_set_key, .-hygon_cis_sm4_set_key

.globl hygon_cis_sm4_encrypt_block
.type hygon_cis_sm4_encrypt_block, @function
.align 16
hygon_cis_sm4_encrypt_block:
  leaq 128(%rdi), %rax
  movdqa .Lbswap32_mask rRIP, RX3
  movdqu (%rsi), RX0
  pshufb RX3, RX0

.Lhygon_blk_loop:
  movdqu (%rdi), RX1
  SM4_ROUND_INST_SM4RND
  addq $16, %rdi
  cmpq %rax, %rdi
  jne .Lhygon_blk_loop

  pshufd $0x1b, RX0, RX0
  pshufb RX3, RX0
  movdqu RX0, (%rdx)
  ret
.size hygon_cis_sm4_encrypt_block, .-hygon_cis_sm4_encrypt_block

.globl hygon_cis_sm4_encrypt_blocks
.type hygon_cis_sm4_encrypt_blocks, @function
.align 16
hygon_cis_sm4_encrypt_blocks:
  testq %rcx, %rcx
  je .Lhygon_ecb_done

  movq %rdi, %r9
  movdqa .Lbswap32_mask rRIP, RX3

.Lhygon_ecb_loop:
  movq %r9, %rdi
  leaq 128(%rdi), %rax
  movdqu (%rsi), RX0
  pshufb RX3, RX0

.Lhygon_ecb_round_loop:
  movdqu (%rdi), RX1
  SM4_ROUND_INST_SM4RND
  addq $16, %rdi
  cmpq %rax, %rdi
  jne .Lhygon_ecb_round_loop

  pshufd $0x1b, RX0, RX0
  pshufb RX3, RX0
  movdqu RX0, (%rdx)

  addq $16, %rsi
  addq $16, %rdx
  decq %rcx
  jne .Lhygon_ecb_loop

.Lhygon_ecb_done:
  ret
.size hygon_cis_sm4_encrypt_blocks, .-hygon_cis_sm4_encrypt_blocks

.globl hygon_cis_sm4_prg_2lambda_from_key
.type hygon_cis_sm4_prg_2lambda_from_key, @function
.align 16
hygon_cis_sm4_prg_2lambda_from_key:
  subq $128, %rsp
  movq %rsp, %r10

  HYGON_CIS_BUILD_RK_FROM_KEY %rdi

  HYGON_CIS_ENC_BLOCK %rsi, %rdx
  addq $16, %rsi
  addq $16, %rdx
  HYGON_CIS_ENC_BLOCK %rsi, %rdx

  addq $128, %rsp
  ret
.size hygon_cis_sm4_prg_2lambda_from_key, .-hygon_cis_sm4_prg_2lambda_from_key

.globl hygon_cis_sm4_prg_2lambda_276_from_key
.type hygon_cis_sm4_prg_2lambda_276_from_key, @function
.align 16
hygon_cis_sm4_prg_2lambda_276_from_key:
  subq $160, %rsp
  movq %rsp, %r10

  HYGON_CIS_BUILD_RK_FROM_KEY %rdi

  HYGON_CIS_ENC_BLOCK %rsi, %rdx
  addq $16, %rsi
  addq $16, %rdx
  HYGON_CIS_ENC_BLOCK %rsi, %rdx

  movq $16, %rdi
.Lhygon_prg_276_full_loop:
  HYGON_CIS_ENC_BLOCK %rcx, %r9
  addq $16, %rcx
  addq $16, %r9
  decq %rdi
  jne .Lhygon_prg_276_full_loop

  HYGON_CIS_ENC_BLOCK %r8, %r9
  addq $16, %r8
  leaq 128(%r10), %rdi
  HYGON_CIS_ENC_BLOCK %r8, %rdi
  movl 128(%r10), %eax
  movl %eax, 16(%r9)

  addq $160, %rsp
  ret
.size hygon_cis_sm4_prg_2lambda_276_from_key, .-hygon_cis_sm4_prg_2lambda_276_from_key

.globl hygon_cis_sm4_prg_2lambda_4way_from_key
.type hygon_cis_sm4_prg_2lambda_4way_from_key, @function
.align 16
hygon_cis_sm4_prg_2lambda_4way_from_key:
  pushq %r12
  subq $512, %rsp
  movq %rsp, %r12

  HYGON_CIS_BUILD_RK4_FROM_KEYS

  HYGON_CIS_ENC4_LANE_SRCS %rdx, 0
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 0
  HYGON_CIS_ENC4_LANE_SRCS %rdx, 16
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 16

  addq $512, %rsp
  popq %r12
  ret
.size hygon_cis_sm4_prg_2lambda_4way_from_key, .-hygon_cis_sm4_prg_2lambda_4way_from_key

.globl hygon_cis_sm4_prg_2lambda_276_4way_from_key
.type hygon_cis_sm4_prg_2lambda_276_4way_from_key, @function
.align 16
hygon_cis_sm4_prg_2lambda_276_4way_from_key:
  pushq %rbx
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  movq 48(%rsp), %r13
  movq 56(%rsp), %r14
  subq $512, %rsp
  movq %rsp, %r12

  HYGON_CIS_BUILD_RK4_FROM_KEYS

  HYGON_CIS_ENC4_SAME_SRC %rdx
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 0
  addq $16, %rdx
  HYGON_CIS_ENC4_SAME_SRC %rdx
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 16

  xorq %r15, %r15
  movq $16, %rbx
.Lhygon_prg_276_4way_full_loop:
  HYGON_CIS_ENC4_SAME_SRC %r9
  HYGON_CIS_STORE4_PTRS_OFFSET %r14, %r15
  addq $16, %r9
  addq $16, %r15
  decq %rbx
  jne .Lhygon_prg_276_4way_full_loop

  HYGON_CIS_ENC4_SAME_SRC %r13
  HYGON_CIS_STORE4_PTRS_OFFSET %r14, %r15
  addq $16, %r13
  HYGON_CIS_ENC4_SAME_SRC %r13
  HYGON_CIS_STORE4_PTRS_4B %r14, 272

  addq $512, %rsp
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  ret
.size hygon_cis_sm4_prg_2lambda_276_4way_from_key, .-hygon_cis_sm4_prg_2lambda_276_4way_from_key

.globl hygon_cis_sm4_prg_2lambda_276_8way_from_key
.type hygon_cis_sm4_prg_2lambda_276_8way_from_key, @function
.align 16
hygon_cis_sm4_prg_2lambda_276_8way_from_key:
  pushq %rbx
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  movq 48(%rsp), %r13
  movq 56(%rsp), %r14
  subq $512, %rsp
  movq %rsp, %r12

  HYGON_CIS_BUILD_RK4_FROM_KEYS

  HYGON_CIS_ENC4_SAME_SRC %rdx
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 0
  addq $16, %rdx
  HYGON_CIS_ENC4_SAME_SRC %rdx
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 16

  xorq %r15, %r15
  movq $16, %rbx
.Lhygon_prg_276_8way_full_loop_a:
  HYGON_CIS_ENC4_SAME_SRC %r9
  HYGON_CIS_STORE4_PTRS_OFFSET %r14, %r15
  addq $16, %r9
  addq $16, %r15
  decq %rbx
  jne .Lhygon_prg_276_8way_full_loop_a

  HYGON_CIS_ENC4_SAME_SRC %r13
  HYGON_CIS_STORE4_PTRS_OFFSET %r14, %r15
  addq $16, %r13
  HYGON_CIS_ENC4_SAME_SRC %r13
  HYGON_CIS_STORE4_PTRS_4B %r14, 272

  subq $16, %rdx
  subq $256, %r9
  subq $16, %r13
  movq %rsi, %rax
  shlq $2, %rax
  addq %rax, %rdi
  movq %r8, %rax
  shlq $2, %rax
  addq %rax, %rcx
  addq $32, %r14

  HYGON_CIS_BUILD_RK4_FROM_KEYS

  HYGON_CIS_ENC4_SAME_SRC %rdx
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 0
  addq $16, %rdx
  HYGON_CIS_ENC4_SAME_SRC %rdx
  HYGON_CIS_STORE4_STRIDED %rcx, %r8, 16

  xorq %r15, %r15
  movq $16, %rbx
.Lhygon_prg_276_8way_full_loop_b:
  HYGON_CIS_ENC4_SAME_SRC %r9
  HYGON_CIS_STORE4_PTRS_OFFSET %r14, %r15
  addq $16, %r9
  addq $16, %r15
  decq %rbx
  jne .Lhygon_prg_276_8way_full_loop_b

  HYGON_CIS_ENC4_SAME_SRC %r13
  HYGON_CIS_STORE4_PTRS_OFFSET %r14, %r15
  addq $16, %r13
  HYGON_CIS_ENC4_SAME_SRC %r13
  HYGON_CIS_STORE4_PTRS_4B %r14, 272

  addq $512, %rsp
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  ret
.size hygon_cis_sm4_prg_2lambda_276_8way_from_key, .-hygon_cis_sm4_prg_2lambda_276_8way_from_key

.section .note.GNU-stack, "", @progbits
