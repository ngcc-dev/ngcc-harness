# This assembly file is automatically generated. Do not modify it directly.
# The code generator is available at https://github.com/mupq/polymul-z2mx-m4
# - Matthias Kannwischer, Joost Rijneveld, and Peter Schwabe, 2018, Public Domain

.syntax unified
.cpu cortex-m4
.global schoolbook_16x16
.type schoolbook_16x16, %function
.align 2
schoolbook_16x16:
push {lr}
ldr r6, [r1, #0]
ldr.w ip, [r1, #4]
ldr.w r3, [r1, #8]
ldr.w sl, [r1, #12]
ldr.w r7, [r2, #0]
ldr.w r8, [r2, #4]
ldr.w r4, [r2, #8]
ldr.w lr, [r2, #12]
smulbb r9, r7, r6
smuadx fp, r7, r6
pkhbt r9, r9, fp, lsl #16
str.w r9, [r0]
smuadx fp, r7, ip
smulbb r5, r7, ip
pkhbt r9, r8, r7
smladx fp, r8, r6, fp
smlad r5, r9, r6, r5
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #4]
smulbb r5, r3, r7
smuadx fp, r3, r7
smlad r5, r9, ip, r5
pkhbt r9, r4, r8
smlad r5, r9, r6, r5
smladx fp, r8, ip, fp
smladx fp, r4, r6, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #8]
smulbb r5, sl, r7
smuadx fp, sl, r7
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, r3, r9, r5
pkhbt r9, lr, r4
smlad r5, r6, r9, r5
smladx fp, r3, r8, fp
smladx fp, ip, r4, fp
smladx fp, r6, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #12]
smultt r5, r6, lr
smuadx fp, sl, r8
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, sl, r9, r5
pkhbt r9, r4, r8
smlad r5, r3, r9, r5
smladx fp, r3, r4, fp
smladx fp, ip, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #16]
smultt r5, ip, lr
smuadx fp, sl, r4
smlad r5, sl, r9, r5
pkhbt r9, lr, r4
smlad r5, r3, r9, r5
smladx fp, r3, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #20]
smuad r5, sl, r9
smuadx fp, sl, lr
smlatt r5, r3, lr, r5
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #24]
smultt fp, sl, lr
movt fp, #0
str.w fp, [r0, #28]
ldr.w r7, [r2, #16]
ldr.w r8, [r2, #20]
ldr.w r4, [r2, #24]
ldr.w lr, [r2, #28]
ldr.w r9, [r0, #16]
ldr.w r5, [r0, #20]
mov.w fp, r9, lsr #16
smlabb r9, r7, r6, r9
smladx fp, r7, r6, fp
pkhbt r9, r9, fp, lsl #16
str.w r9, [r0, #16]
mov.w fp, r5, lsr #16
smladx fp, r7, ip, fp
smlabb r5, r7, ip, r5
pkhbt r9, r8, r7
smladx fp, r8, r6, fp
smlad r5, r9, r6, r5
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #24]
str.w fp, [r0, #20]
mov.w fp, r5, lsr #16
smlabb r5, r3, r7, r5
smladx fp, r3, r7, fp
smlad r5, r9, ip, r5
pkhbt r9, r4, r8
smlad r5, r9, r6, r5
smladx fp, r8, ip, fp
smladx fp, r4, r6, fp
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #28]
str.w fp, [r0, #24]
mov.w fp, r5, lsr #16
smlabb r5, sl, r7, r5
smladx fp, sl, r7, fp
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, r3, r9, r5
pkhbt r9, lr, r4
smlad r5, r6, r9, r5
smladx fp, r3, r8, fp
smladx fp, ip, r4, fp
smladx fp, r6, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #28]
smultt r5, r6, lr
smuadx fp, sl, r8
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, sl, r9, r5
pkhbt r9, r4, r8
smlad r5, r3, r9, r5
smladx fp, r3, r4, fp
smladx fp, ip, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #32]
smultt r5, ip, lr
smuadx fp, sl, r4
smlad r5, sl, r9, r5
pkhbt r9, lr, r4
smlad r5, r3, r9, r5
smladx fp, r3, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #36]
smuad r5, sl, r9
smuadx fp, sl, lr
smlatt r5, r3, lr, r5
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #40]
smultt fp, sl, lr
movt fp, #0
str.w fp, [r0, #44]
ldr.w r6, [r1, #16]
ldr.w ip, [r1, #20]
ldr.w r3, [r1, #24]
ldr.w sl, [r1, #28]
ldr.w r9, [r0, #32]
ldr.w r5, [r0, #36]
mov.w fp, r9, lsr #16
smlabb r9, r7, r6, r9
smladx fp, r7, r6, fp
pkhbt r9, r9, fp, lsl #16
str.w r9, [r0, #32]
mov.w fp, r5, lsr #16
smladx fp, r7, ip, fp
smlabb r5, r7, ip, r5
pkhbt r9, r8, r7
smladx fp, r8, r6, fp
smlad r5, r9, r6, r5
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #40]
str.w fp, [r0, #36]
mov.w fp, r5, lsr #16
smlabb r5, r3, r7, r5
smladx fp, r3, r7, fp
smlad r5, r9, ip, r5
pkhbt r9, r4, r8
smlad r5, r9, r6, r5
smladx fp, r8, ip, fp
smladx fp, r4, r6, fp
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #44]
str.w fp, [r0, #40]
mov.w fp, r5, lsr #16
smlabb r5, sl, r7, r5
smladx fp, sl, r7, fp
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, r3, r9, r5
pkhbt r9, lr, r4
smlad r5, r6, r9, r5
smladx fp, r3, r8, fp
smladx fp, ip, r4, fp
smladx fp, r6, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #44]
smultt r5, r6, lr
smuadx fp, sl, r8
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, sl, r9, r5
pkhbt r9, r4, r8
smlad r5, r3, r9, r5
smladx fp, r3, r4, fp
smladx fp, ip, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #48]
smultt r5, ip, lr
smuadx fp, sl, r4
smlad r5, sl, r9, r5
pkhbt r9, lr, r4
smlad r5, r3, r9, r5
smladx fp, r3, lr, fp
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #52]
smuad r5, sl, r9
smuadx fp, sl, lr
smlatt r5, r3, lr, r5
pkhbt fp, r5, fp, lsl #16
str.w fp, [r0, #56]
smultt fp, sl, lr
strh.w fp, [r0, #60]
ldr.w r7, [r2, #0]
ldr.w r8, [r2, #4]
ldr.w r4, [r2, #8]
ldr.w lr, [r2, #12]
ldr.w r9, [r0, #16]
ldr.w r5, [r0, #20]
mov.w fp, r9, lsr #16
smlabb r9, r7, r6, r9
smladx fp, r7, r6, fp
pkhbt r9, r9, fp, lsl #16
str.w r9, [r0, #16]
mov.w fp, r5, lsr #16
smladx fp, r7, ip, fp
smlabb r5, r7, ip, r5
pkhbt r9, r8, r7
smladx fp, r8, r6, fp
smlad r5, r9, r6, r5
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #24]
str.w fp, [r0, #20]
mov.w fp, r5, lsr #16
smlabb r5, r3, r7, r5
smladx fp, r3, r7, fp
smlad r5, r9, ip, r5
pkhbt r9, r4, r8
smlad r5, r9, r6, r5
smladx fp, r8, ip, fp
smladx fp, r4, r6, fp
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #28]
str.w fp, [r0, #24]
mov.w fp, r5, lsr #16
smlabb r5, sl, r7, r5
smladx fp, sl, r7, fp
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, r3, r9, r5
pkhbt r9, lr, r4
smlad r5, r6, r9, r5
smladx fp, r3, r8, fp
smladx fp, ip, r4, fp
smladx fp, r6, lr, fp
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #32]
str.w fp, [r0, #28]
mov.w fp, r5, lsr #16
smlatt r5, r6, lr, r5
smladx fp, sl, r8, fp
smlad r5, ip, r9, r5
pkhbt r9, r8, r7
smlad r5, sl, r9, r5
pkhbt r9, r4, r8
smlad r5, r3, r9, r5
smladx fp, r3, r4, fp
smladx fp, ip, lr, fp
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #36]
str.w fp, [r0, #32]
mov.w fp, r5, lsr #16
smlatt r5, ip, lr, r5
smladx fp, sl, r4, fp
smlad r5, sl, r9, r5
pkhbt r9, lr, r4
smlad r5, r3, r9, r5
smladx fp, r3, lr, fp
pkhbt fp, r5, fp, lsl #16
ldr.w r5, [r0, #40]
str.w fp, [r0, #36]
mov.w fp, r5, lsr #16
smlad r5, sl, r9, r5
smladx fp, sl, lr, fp
smlatt r5, r3, lr, r5
pkhbt fp, r5, fp, lsl #16
ldrh.w r5, [r0, #44]
str.w fp, [r0, #40]
smlatt fp, sl, lr, r5
strh.w fp, [r0, #44]
ldr.w lr, [sp], #4
bx lr
.global karatsuba_32x32
.type karatsuba_32x32, %function
.align 2
karatsuba_32x32:
push.w {lr}
bl schoolbook_16x16
push {r1}
push {r2}
push.w {r0}
add.w r1, r1, #32
add.w r2, r2, #32
add.w r0, r0, #64
bl schoolbook_16x16
sub.w r1, r1, #32
sub.w r2, r2, #32
sub sp, #128
ldr r3, [r1, #0]
ldr r4, [r1, #32]
ldr r5, [r1, #4]
ldr r6, [r1, #36]
ldr r7, [r1, #8]
ldr.w r8, [r1, #40]
ldr.w r9, [r1, #12]
ldr.w sl, [r1, #44]
ldr.w fp, [r1, #16]
ldr.w ip, [r1, #48]
ldr.w r0, [r1, #20]
ldr.w lr, [r1, #52]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
uadd16 r7, r7, r8
uadd16 r9, r9, sl
uadd16 fp, fp, ip
uadd16 r0, r0, lr
str r3, [sp, #0]
str r5, [sp, #4]
str.w r7, [sp, #8]
str.w r9, [sp, #12]
str.w fp, [sp, #16]
str r0, [sp, #20]
ldr r3, [r1, #24]
ldr r4, [r1, #56]
ldr r5, [r1, #28]
ldr.w r6, [r1, #60]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
str r3, [sp, #24]
str r5, [sp, #28]
mov r1, sp
ldr r3, [r2, #0]
ldr r4, [r2, #32]
ldr r5, [r2, #4]
ldr r6, [r2, #36]
ldr r7, [r2, #8]
ldr.w r8, [r2, #40]
ldr.w r9, [r2, #12]
ldr.w sl, [r2, #44]
ldr.w fp, [r2, #16]
ldr.w ip, [r2, #48]
ldr.w r0, [r2, #20]
ldr.w lr, [r2, #52]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
uadd16 r7, r7, r8
uadd16 r9, r9, sl
uadd16 fp, fp, ip
uadd16 r0, r0, lr
str r3, [sp, #32]
str r5, [sp, #36]
str.w r7, [sp, #40]
str.w r9, [sp, #44]
str.w fp, [sp, #48]
str r0, [sp, #52]
ldr r3, [r2, #24]
ldr r4, [r2, #56]
ldr r5, [r2, #28]
ldr.w r6, [r2, #60]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
str r3, [sp, #56]
str r5, [sp, #60]
add r2, sp, #32
add r0, sp, #64
bl schoolbook_16x16
mov r3, r0
ldr r0, [sp, #128]
ldr r5, [r3, #0]
ldr r6, [r3, #32]
ldr.w r7, [r0, #0]
ldr.w r8, [r0, #32]
ldr.w r9, [r0, #64]
ldr.w sl, [r0, #96]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #32]
usub16 r6, r6, r8
usub16 r6, r6, sl
str r6, [r0, #64]
ldr r5, [r3, #4]
ldr r6, [r3, #36]
ldr r7, [r0, #4]
ldr.w r8, [r0, #36]
ldr.w r9, [r0, #68]
ldr.w sl, [r0, #100]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #36]
usub16 r6, r6, r8
usub16 r6, r6, sl
str r6, [r0, #68]
ldr r5, [r3, #8]
ldr r6, [r3, #40]
ldr r7, [r0, #8]
ldr.w r8, [r0, #40]
ldr.w r9, [r0, #72]
ldr.w sl, [r0, #104]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #40]
usub16 r6, r6, r8
usub16 r6, r6, sl
str r6, [r0, #72]
ldr r5, [r3, #12]
ldr r6, [r3, #44]
ldr r7, [r0, #12]
ldr.w r8, [r0, #44]
ldr.w r9, [r0, #76]
ldr.w sl, [r0, #108]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #44]
usub16 r6, r6, r8
usub16 r6, r6, sl
str r6, [r0, #76]
ldr r5, [r3, #16]
ldr r6, [r3, #48]
ldr r7, [r0, #16]
ldr.w r8, [r0, #48]
ldr.w r9, [r0, #80]
ldr.w sl, [r0, #112]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #48]
usub16 r6, r6, r8
usub16 r6, r6, sl
str r6, [r0, #80]
ldr r5, [r3, #20]
ldr r6, [r3, #52]
ldr r7, [r0, #20]
ldr.w r8, [r0, #52]
ldr.w r9, [r0, #84]
ldr.w sl, [r0, #116]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #52]
usub16 r6, r6, r8
usub16 r6, r6, sl
str r6, [r0, #84]
ldr r5, [r3, #24]
ldr r6, [r3, #56]
ldr r7, [r0, #24]
ldr.w r8, [r0, #56]
ldr.w r9, [r0, #88]
ldr.w sl, [r0, #120]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #56]
usub16 r6, r6, r8
usub16 r6, r6, sl
str r6, [r0, #88]
ldrh r5, [r3, #28]
ldrh r6, [r3, #60]
ldrh r7, [r0, #28]
ldrh.w r8, [r0, #60]
ldrh.w r9, [r0, #92]
ldrh.w sl, [r0, #124]
sub.w fp, r5, r7
sub.w r8, r8, r9
add.w fp, r8
strh.w fp, [r0, #60]
sub.w r6, r6, r8
sub.w r6, r6, sl
strh.w r6, [r0, #92]
ldrh r5, [r3, #30]
ldrh r7, [r0, #30]
ldrh.w r9, [r0, #94]
sub.w fp, r5, r7
sub.w fp, fp, r9
strh.w fp, [r0, #62]
add sp, #132
pop {r2}
pop.w {r1}
ldr.w lr, [sp], #4
bx lr
.global polymul_asm
.type polymul_asm, %function
.align 2
polymul_asm:
stmdb sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
sub.w sp, sp, #1664
add r3, sp, #384
ldr r4, [r1, #0]
ldr.w r5, [r1, #64]
ldr.w r6, [r1, #128]
ldr.w r7, [r1, #192]
str.w r7, [sp, #320]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp]
str.w ip, [sp, #64]
str.w r9, [sp, #128]
str.w sl, [sp, #192]
str r7, [sp, #256]
ldr r4, [r2, #0]
ldr.w r5, [r2, #64]
ldr.w r6, [r2, #128]
ldr.w r7, [r2, #192]
str.w r7, [r3, #320]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3]
str.w ip, [r3, #64]
str.w r9, [r3, #128]
str.w sl, [r3, #192]
str.w r7, [r3, #256]
ldr r4, [r1, #4]
ldr r5, [r1, #68]
ldr.w r6, [r1, #132]
ldr.w r7, [r1, #196]
str.w r7, [sp, #324]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #4]
str.w ip, [sp, #68]
str.w r9, [sp, #132]
str.w sl, [sp, #196]
str r7, [sp, #260]
ldr r4, [r2, #4]
ldr.w r5, [r2, #68]
ldr.w r6, [r2, #132]
ldr.w r7, [r2, #196]
str.w r7, [r3, #324]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #4]
str.w ip, [r3, #68]
str.w r9, [r3, #132]
str.w sl, [r3, #196]
str.w r7, [r3, #260]
ldr r4, [r1, #8]
ldr r5, [r1, #72]
ldr.w r6, [r1, #136]
ldr.w r7, [r1, #200]
str.w r7, [sp, #328]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #8]
str.w ip, [sp, #72]
str.w r9, [sp, #136]
str.w sl, [sp, #200]
str r7, [sp, #264]
ldr r4, [r2, #8]
ldr.w r5, [r2, #72]
ldr.w r6, [r2, #136]
ldr.w r7, [r2, #200]
str.w r7, [r3, #328]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #8]
str.w ip, [r3, #72]
str.w r9, [r3, #136]
str.w sl, [r3, #200]
str.w r7, [r3, #264]
ldr r4, [r1, #12]
ldr r5, [r1, #76]
ldr.w r6, [r1, #140]
ldr.w r7, [r1, #204]
str.w r7, [sp, #332]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #12]
str.w ip, [sp, #76]
str.w r9, [sp, #140]
str.w sl, [sp, #204]
str r7, [sp, #268]
ldr r4, [r2, #12]
ldr.w r5, [r2, #76]
ldr.w r6, [r2, #140]
ldr.w r7, [r2, #204]
str.w r7, [r3, #332]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #12]
str.w ip, [r3, #76]
str.w r9, [r3, #140]
str.w sl, [r3, #204]
str.w r7, [r3, #268]
ldr r4, [r1, #16]
ldr r5, [r1, #80]
ldr.w r6, [r1, #144]
ldr.w r7, [r1, #208]
str.w r7, [sp, #336]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #16]
str.w ip, [sp, #80]
str.w r9, [sp, #144]
str.w sl, [sp, #208]
str r7, [sp, #272]
ldr r4, [r2, #16]
ldr.w r5, [r2, #80]
ldr.w r6, [r2, #144]
ldr.w r7, [r2, #208]
str.w r7, [r3, #336]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #16]
str.w ip, [r3, #80]
str.w r9, [r3, #144]
str.w sl, [r3, #208]
str.w r7, [r3, #272]
ldr r4, [r1, #20]
ldr r5, [r1, #84]
ldr.w r6, [r1, #148]
ldr.w r7, [r1, #212]
str.w r7, [sp, #340]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #20]
str.w ip, [sp, #84]
str.w r9, [sp, #148]
str.w sl, [sp, #212]
str r7, [sp, #276]
ldr r4, [r2, #20]
ldr.w r5, [r2, #84]
ldr.w r6, [r2, #148]
ldr.w r7, [r2, #212]
str.w r7, [r3, #340]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #20]
str.w ip, [r3, #84]
str.w r9, [r3, #148]
str.w sl, [r3, #212]
str.w r7, [r3, #276]
ldr r4, [r1, #24]
ldr r5, [r1, #88]
ldr.w r6, [r1, #152]
ldr.w r7, [r1, #216]
str.w r7, [sp, #344]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #24]
str.w ip, [sp, #88]
str.w r9, [sp, #152]
str.w sl, [sp, #216]
str r7, [sp, #280]
ldr r4, [r2, #24]
ldr.w r5, [r2, #88]
ldr.w r6, [r2, #152]
ldr.w r7, [r2, #216]
str.w r7, [r3, #344]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #24]
str.w ip, [r3, #88]
str.w r9, [r3, #152]
str.w sl, [r3, #216]
str.w r7, [r3, #280]
ldr r4, [r1, #28]
ldr r5, [r1, #92]
ldr.w r6, [r1, #156]
ldr.w r7, [r1, #220]
str.w r7, [sp, #348]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #28]
str.w ip, [sp, #92]
str.w r9, [sp, #156]
str.w sl, [sp, #220]
str r7, [sp, #284]
ldr r4, [r2, #28]
ldr.w r5, [r2, #92]
ldr.w r6, [r2, #156]
ldr.w r7, [r2, #220]
str.w r7, [r3, #348]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #28]
str.w ip, [r3, #92]
str.w r9, [r3, #156]
str.w sl, [r3, #220]
str.w r7, [r3, #284]
ldr r4, [r1, #32]
ldr r5, [r1, #96]
ldr.w r6, [r1, #160]
ldr.w r7, [r1, #224]
str.w r7, [sp, #352]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #32]
str.w ip, [sp, #96]
str.w r9, [sp, #160]
str.w sl, [sp, #224]
str r7, [sp, #288]
ldr r4, [r2, #32]
ldr.w r5, [r2, #96]
ldr.w r6, [r2, #160]
ldr.w r7, [r2, #224]
str.w r7, [r3, #352]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #32]
str.w ip, [r3, #96]
str.w r9, [r3, #160]
str.w sl, [r3, #224]
str.w r7, [r3, #288]
ldr r4, [r1, #36]
ldr r5, [r1, #100]
ldr.w r6, [r1, #164]
ldr.w r7, [r1, #228]
str.w r7, [sp, #356]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #36]
str.w ip, [sp, #100]
str.w r9, [sp, #164]
str.w sl, [sp, #228]
str r7, [sp, #292]
ldr r4, [r2, #36]
ldr.w r5, [r2, #100]
ldr.w r6, [r2, #164]
ldr.w r7, [r2, #228]
str.w r7, [r3, #356]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #36]
str.w ip, [r3, #100]
str.w r9, [r3, #164]
str.w sl, [r3, #228]
str.w r7, [r3, #292]
ldr r4, [r1, #40]
ldr r5, [r1, #104]
ldr.w r6, [r1, #168]
ldr.w r7, [r1, #232]
str.w r7, [sp, #360]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #40]
str.w ip, [sp, #104]
str.w r9, [sp, #168]
str.w sl, [sp, #232]
str r7, [sp, #296]
ldr r4, [r2, #40]
ldr.w r5, [r2, #104]
ldr.w r6, [r2, #168]
ldr.w r7, [r2, #232]
str.w r7, [r3, #360]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #40]
str.w ip, [r3, #104]
str.w r9, [r3, #168]
str.w sl, [r3, #232]
str.w r7, [r3, #296]
ldr r4, [r1, #44]
ldr r5, [r1, #108]
ldr.w r6, [r1, #172]
ldr.w r7, [r1, #236]
str.w r7, [sp, #364]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #44]
str.w ip, [sp, #108]
str.w r9, [sp, #172]
str.w sl, [sp, #236]
str r7, [sp, #300]
ldr r4, [r2, #44]
ldr.w r5, [r2, #108]
ldr.w r6, [r2, #172]
ldr.w r7, [r2, #236]
str.w r7, [r3, #364]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #44]
str.w ip, [r3, #108]
str.w r9, [r3, #172]
str.w sl, [r3, #236]
str.w r7, [r3, #300]
ldr r4, [r1, #48]
ldr r5, [r1, #112]
ldr.w r6, [r1, #176]
ldr.w r7, [r1, #240]
str.w r7, [sp, #368]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #48]
str.w ip, [sp, #112]
str.w r9, [sp, #176]
str.w sl, [sp, #240]
str r7, [sp, #304]
ldr r4, [r2, #48]
ldr.w r5, [r2, #112]
ldr.w r6, [r2, #176]
ldr.w r7, [r2, #240]
str.w r7, [r3, #368]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #48]
str.w ip, [r3, #112]
str.w r9, [r3, #176]
str.w sl, [r3, #240]
str.w r7, [r3, #304]
ldr r4, [r1, #52]
ldr r5, [r1, #116]
ldr.w r6, [r1, #180]
ldr.w r7, [r1, #244]
str.w r7, [sp, #372]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #52]
str.w ip, [sp, #116]
str.w r9, [sp, #180]
str.w sl, [sp, #244]
str r7, [sp, #308]
ldr r4, [r2, #52]
ldr.w r5, [r2, #116]
ldr.w r6, [r2, #180]
ldr.w r7, [r2, #244]
str.w r7, [r3, #372]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #52]
str.w ip, [r3, #116]
str.w r9, [r3, #180]
str.w sl, [r3, #244]
str.w r7, [r3, #308]
ldr r4, [r1, #56]
ldr r5, [r1, #120]
ldr.w r6, [r1, #184]
ldr.w r7, [r1, #248]
str.w r7, [sp, #376]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #56]
str.w ip, [sp, #120]
str.w r9, [sp, #184]
str.w sl, [sp, #248]
str r7, [sp, #312]
ldr r4, [r2, #56]
ldr.w r5, [r2, #120]
ldr.w r6, [r2, #184]
ldr.w r7, [r2, #248]
str.w r7, [r3, #376]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #56]
str.w ip, [r3, #120]
str.w r9, [r3, #184]
str.w sl, [r3, #248]
str.w r7, [r3, #312]
ldr r4, [r1, #60]
ldr r5, [r1, #124]
ldr.w r6, [r1, #188]
ldr.w r7, [r1, #252]
str.w r7, [sp, #380]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [sp, #60]
str.w ip, [sp, #124]
str.w r9, [sp, #188]
str.w sl, [sp, #252]
str r7, [sp, #316]
ldr r4, [r2, #60]
ldr.w r5, [r2, #124]
ldr.w r6, [r2, #188]
ldr.w r7, [r2, #252]
str.w r7, [r3, #380]
uadd16 lr, r4, r6
uadd16 ip, r5, r7
uadd16 fp, lr, ip
usub16 ip, lr, ip
uadd16 lr, r6, r6
uadd16 lr, lr, lr
uadd16 lr, r4, lr
uadd16 sl, r7, r7
uadd16 sl, sl, sl
uadd16 sl, r5, sl
uadd16 sl, sl, sl
uadd16 r9, lr, sl
usub16 sl, lr, sl
uadd16 r8, r7, r7
uadd16 r7, r8, r7
uadd16 r7, r7, r6
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r5
uadd16 r8, r7, r7
uadd16 r7, r7, r8
uadd16 r7, r7, r4
str.w fp, [r3, #60]
str.w ip, [r3, #124]
str.w r9, [r3, #188]
str.w sl, [r3, #252]
str.w r7, [r3, #316]
push {r0}
push {r3}
movw fp, #776
add.w r0, sp, fp
bl karatsuba_32x32
pop.w {r3}
add.w r0, r0, #128
add.w r1, sp, #4
add.w r2, r3, #0
bl karatsuba_32x32
add.w r0, r0, #128
add.w r1, r1, #64
add.w r2, r2, #64
bl karatsuba_32x32
add.w r0, r0, #128
add.w r1, r1, #64
add.w r2, r2, #64
bl karatsuba_32x32
add.w r0, r0, #128
add.w r1, r1, #64
add.w r2, r2, #64
bl karatsuba_32x32
add.w r0, r0, #128
add.w r1, r1, #64
add.w r2, r2, #64
bl karatsuba_32x32
add.w r0, r0, #128
add.w r1, r1, #64
add.w r2, r2, #64
bl karatsuba_32x32
pop.w {r0}
add.w sp, sp, #768
add.w fp, sp, #512
movw lr, #43691
movw ip, #52429
ldrh.w r1, [sp]
ldrh.w r2, [sp, #128]
ldrh.w r3, [sp, #256]
ldrh.w r4, [sp, #384]
ldrh.w r5, [fp]
ldrh.w r6, [fp, #128]
ldrh.w r7, [fp, #256]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #0]
strh.w r3, [r0, #64]
strh.w r8, [r0, #128]
strh.w r6, [r0, #192]
strh.w r9, [r0, #256]
strh.w r5, [r0, #320]
strh.w r7, [r0, #384]
ldrh.w r1, [sp, #2]
ldrh.w r2, [sp, #130]
ldrh.w r3, [sp, #258]
ldrh.w r4, [sp, #386]
ldrh.w r5, [fp, #2]
ldrh.w r6, [fp, #130]
ldrh.w r7, [fp, #258]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #2]
strh.w r3, [r0, #66]
strh.w r8, [r0, #130]
strh.w r6, [r0, #194]
strh.w r9, [r0, #258]
strh.w r5, [r0, #322]
strh.w r7, [r0, #386]
ldrh.w r1, [sp, #4]
ldrh.w r2, [sp, #132]
ldrh.w r3, [sp, #260]
ldrh.w r4, [sp, #388]
ldrh.w r5, [fp, #4]
ldrh.w r6, [fp, #132]
ldrh.w r7, [fp, #260]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #4]
strh.w r3, [r0, #68]
strh.w r8, [r0, #132]
strh.w r6, [r0, #196]
strh.w r9, [r0, #260]
strh.w r5, [r0, #324]
strh.w r7, [r0, #388]
ldrh.w r1, [sp, #6]
ldrh.w r2, [sp, #134]
ldrh.w r3, [sp, #262]
ldrh.w r4, [sp, #390]
ldrh.w r5, [fp, #6]
ldrh.w r6, [fp, #134]
ldrh.w r7, [fp, #262]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #6]
strh.w r3, [r0, #70]
strh.w r8, [r0, #134]
strh.w r6, [r0, #198]
strh.w r9, [r0, #262]
strh.w r5, [r0, #326]
strh.w r7, [r0, #390]
ldrh.w r1, [sp, #8]
ldrh.w r2, [sp, #136]
ldrh.w r3, [sp, #264]
ldrh.w r4, [sp, #392]
ldrh.w r5, [fp, #8]
ldrh.w r6, [fp, #136]
ldrh.w r7, [fp, #264]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #8]
strh.w r3, [r0, #72]
strh.w r8, [r0, #136]
strh.w r6, [r0, #200]
strh.w r9, [r0, #264]
strh.w r5, [r0, #328]
strh.w r7, [r0, #392]
ldrh.w r1, [sp, #10]
ldrh.w r2, [sp, #138]
ldrh.w r3, [sp, #266]
ldrh.w r4, [sp, #394]
ldrh.w r5, [fp, #10]
ldrh.w r6, [fp, #138]
ldrh.w r7, [fp, #266]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #10]
strh.w r3, [r0, #74]
strh.w r8, [r0, #138]
strh.w r6, [r0, #202]
strh.w r9, [r0, #266]
strh.w r5, [r0, #330]
strh.w r7, [r0, #394]
ldrh.w r1, [sp, #12]
ldrh.w r2, [sp, #140]
ldrh.w r3, [sp, #268]
ldrh.w r4, [sp, #396]
ldrh.w r5, [fp, #12]
ldrh.w r6, [fp, #140]
ldrh.w r7, [fp, #268]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #12]
strh.w r3, [r0, #76]
strh.w r8, [r0, #140]
strh.w r6, [r0, #204]
strh.w r9, [r0, #268]
strh.w r5, [r0, #332]
strh.w r7, [r0, #396]
ldrh.w r1, [sp, #14]
ldrh.w r2, [sp, #142]
ldrh.w r3, [sp, #270]
ldrh.w r4, [sp, #398]
ldrh.w r5, [fp, #14]
ldrh.w r6, [fp, #142]
ldrh.w r7, [fp, #270]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #14]
strh.w r3, [r0, #78]
strh.w r8, [r0, #142]
strh.w r6, [r0, #206]
strh.w r9, [r0, #270]
strh.w r5, [r0, #334]
strh.w r7, [r0, #398]
ldrh.w r1, [sp, #16]
ldrh.w r2, [sp, #144]
ldrh.w r3, [sp, #272]
ldrh.w r4, [sp, #400]
ldrh.w r5, [fp, #16]
ldrh.w r6, [fp, #144]
ldrh.w r7, [fp, #272]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #16]
strh.w r3, [r0, #80]
strh.w r8, [r0, #144]
strh.w r6, [r0, #208]
strh.w r9, [r0, #272]
strh.w r5, [r0, #336]
strh.w r7, [r0, #400]
ldrh.w r1, [sp, #18]
ldrh.w r2, [sp, #146]
ldrh.w r3, [sp, #274]
ldrh.w r4, [sp, #402]
ldrh.w r5, [fp, #18]
ldrh.w r6, [fp, #146]
ldrh.w r7, [fp, #274]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #18]
strh.w r3, [r0, #82]
strh.w r8, [r0, #146]
strh.w r6, [r0, #210]
strh.w r9, [r0, #274]
strh.w r5, [r0, #338]
strh.w r7, [r0, #402]
ldrh.w r1, [sp, #20]
ldrh.w r2, [sp, #148]
ldrh.w r3, [sp, #276]
ldrh.w r4, [sp, #404]
ldrh.w r5, [fp, #20]
ldrh.w r6, [fp, #148]
ldrh.w r7, [fp, #276]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #20]
strh.w r3, [r0, #84]
strh.w r8, [r0, #148]
strh.w r6, [r0, #212]
strh.w r9, [r0, #276]
strh.w r5, [r0, #340]
strh.w r7, [r0, #404]
ldrh.w r1, [sp, #22]
ldrh.w r2, [sp, #150]
ldrh.w r3, [sp, #278]
ldrh.w r4, [sp, #406]
ldrh.w r5, [fp, #22]
ldrh.w r6, [fp, #150]
ldrh.w r7, [fp, #278]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #22]
strh.w r3, [r0, #86]
strh.w r8, [r0, #150]
strh.w r6, [r0, #214]
strh.w r9, [r0, #278]
strh.w r5, [r0, #342]
strh.w r7, [r0, #406]
ldrh.w r1, [sp, #24]
ldrh.w r2, [sp, #152]
ldrh.w r3, [sp, #280]
ldrh.w r4, [sp, #408]
ldrh.w r5, [fp, #24]
ldrh.w r6, [fp, #152]
ldrh.w r7, [fp, #280]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #24]
strh.w r3, [r0, #88]
strh.w r8, [r0, #152]
strh.w r6, [r0, #216]
strh.w r9, [r0, #280]
strh.w r5, [r0, #344]
strh.w r7, [r0, #408]
ldrh.w r1, [sp, #26]
ldrh.w r2, [sp, #154]
ldrh.w r3, [sp, #282]
ldrh.w r4, [sp, #410]
ldrh.w r5, [fp, #26]
ldrh.w r6, [fp, #154]
ldrh.w r7, [fp, #282]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #26]
strh.w r3, [r0, #90]
strh.w r8, [r0, #154]
strh.w r6, [r0, #218]
strh.w r9, [r0, #282]
strh.w r5, [r0, #346]
strh.w r7, [r0, #410]
ldrh.w r1, [sp, #28]
ldrh.w r2, [sp, #156]
ldrh.w r3, [sp, #284]
ldrh.w r4, [sp, #412]
ldrh.w r5, [fp, #28]
ldrh.w r6, [fp, #156]
ldrh.w r7, [fp, #284]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #28]
strh.w r3, [r0, #92]
strh.w r8, [r0, #156]
strh.w r6, [r0, #220]
strh.w r9, [r0, #284]
strh.w r5, [r0, #348]
strh.w r7, [r0, #412]
ldrh.w r1, [sp, #30]
ldrh.w r2, [sp, #158]
ldrh.w r3, [sp, #286]
ldrh.w r4, [sp, #414]
ldrh.w r5, [fp, #30]
ldrh.w r6, [fp, #158]
ldrh.w r7, [fp, #286]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #30]
strh.w r3, [r0, #94]
strh.w r8, [r0, #158]
strh.w r6, [r0, #222]
strh.w r9, [r0, #286]
strh.w r5, [r0, #350]
strh.w r7, [r0, #414]
ldrh.w r1, [sp, #32]
ldrh.w r2, [sp, #160]
ldrh.w r3, [sp, #288]
ldrh.w r4, [sp, #416]
ldrh.w r5, [fp, #32]
ldrh.w r6, [fp, #160]
ldrh.w r7, [fp, #288]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #32]
strh.w r3, [r0, #96]
strh.w r8, [r0, #160]
strh.w r6, [r0, #224]
strh.w r9, [r0, #288]
strh.w r5, [r0, #352]
strh.w r7, [r0, #416]
ldrh.w r1, [sp, #34]
ldrh.w r2, [sp, #162]
ldrh.w r3, [sp, #290]
ldrh.w r4, [sp, #418]
ldrh.w r5, [fp, #34]
ldrh.w r6, [fp, #162]
ldrh.w r7, [fp, #290]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #34]
strh.w r3, [r0, #98]
strh.w r8, [r0, #162]
strh.w r6, [r0, #226]
strh.w r9, [r0, #290]
strh.w r5, [r0, #354]
strh.w r7, [r0, #418]
ldrh.w r1, [sp, #36]
ldrh.w r2, [sp, #164]
ldrh.w r3, [sp, #292]
ldrh.w r4, [sp, #420]
ldrh.w r5, [fp, #36]
ldrh.w r6, [fp, #164]
ldrh.w r7, [fp, #292]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #36]
strh.w r3, [r0, #100]
strh.w r8, [r0, #164]
strh.w r6, [r0, #228]
strh.w r9, [r0, #292]
strh.w r5, [r0, #356]
strh.w r7, [r0, #420]
ldrh.w r1, [sp, #38]
ldrh.w r2, [sp, #166]
ldrh.w r3, [sp, #294]
ldrh.w r4, [sp, #422]
ldrh.w r5, [fp, #38]
ldrh.w r6, [fp, #166]
ldrh.w r7, [fp, #294]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #38]
strh.w r3, [r0, #102]
strh.w r8, [r0, #166]
strh.w r6, [r0, #230]
strh.w r9, [r0, #294]
strh.w r5, [r0, #358]
strh.w r7, [r0, #422]
ldrh.w r1, [sp, #40]
ldrh.w r2, [sp, #168]
ldrh.w r3, [sp, #296]
ldrh.w r4, [sp, #424]
ldrh.w r5, [fp, #40]
ldrh.w r6, [fp, #168]
ldrh.w r7, [fp, #296]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #40]
strh.w r3, [r0, #104]
strh.w r8, [r0, #168]
strh.w r6, [r0, #232]
strh.w r9, [r0, #296]
strh.w r5, [r0, #360]
strh.w r7, [r0, #424]
ldrh.w r1, [sp, #42]
ldrh.w r2, [sp, #170]
ldrh.w r3, [sp, #298]
ldrh.w r4, [sp, #426]
ldrh.w r5, [fp, #42]
ldrh.w r6, [fp, #170]
ldrh.w r7, [fp, #298]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #42]
strh.w r3, [r0, #106]
strh.w r8, [r0, #170]
strh.w r6, [r0, #234]
strh.w r9, [r0, #298]
strh.w r5, [r0, #362]
strh.w r7, [r0, #426]
ldrh.w r1, [sp, #44]
ldrh.w r2, [sp, #172]
ldrh.w r3, [sp, #300]
ldrh.w r4, [sp, #428]
ldrh.w r5, [fp, #44]
ldrh.w r6, [fp, #172]
ldrh.w r7, [fp, #300]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #44]
strh.w r3, [r0, #108]
strh.w r8, [r0, #172]
strh.w r6, [r0, #236]
strh.w r9, [r0, #300]
strh.w r5, [r0, #364]
strh.w r7, [r0, #428]
ldrh.w r1, [sp, #46]
ldrh.w r2, [sp, #174]
ldrh.w r3, [sp, #302]
ldrh.w r4, [sp, #430]
ldrh.w r5, [fp, #46]
ldrh.w r6, [fp, #174]
ldrh.w r7, [fp, #302]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #46]
strh.w r3, [r0, #110]
strh.w r8, [r0, #174]
strh.w r6, [r0, #238]
strh.w r9, [r0, #302]
strh.w r5, [r0, #366]
strh.w r7, [r0, #430]
ldrh.w r1, [sp, #48]
ldrh.w r2, [sp, #176]
ldrh.w r3, [sp, #304]
ldrh.w r4, [sp, #432]
ldrh.w r5, [fp, #48]
ldrh.w r6, [fp, #176]
ldrh.w r7, [fp, #304]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #48]
strh.w r3, [r0, #112]
strh.w r8, [r0, #176]
strh.w r6, [r0, #240]
strh.w r9, [r0, #304]
strh.w r5, [r0, #368]
strh.w r7, [r0, #432]
ldrh.w r1, [sp, #50]
ldrh.w r2, [sp, #178]
ldrh.w r3, [sp, #306]
ldrh.w r4, [sp, #434]
ldrh.w r5, [fp, #50]
ldrh.w r6, [fp, #178]
ldrh.w r7, [fp, #306]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #50]
strh.w r3, [r0, #114]
strh.w r8, [r0, #178]
strh.w r6, [r0, #242]
strh.w r9, [r0, #306]
strh.w r5, [r0, #370]
strh.w r7, [r0, #434]
ldrh.w r1, [sp, #52]
ldrh.w r2, [sp, #180]
ldrh.w r3, [sp, #308]
ldrh.w r4, [sp, #436]
ldrh.w r5, [fp, #52]
ldrh.w r6, [fp, #180]
ldrh.w r7, [fp, #308]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #52]
strh.w r3, [r0, #116]
strh.w r8, [r0, #180]
strh.w r6, [r0, #244]
strh.w r9, [r0, #308]
strh.w r5, [r0, #372]
strh.w r7, [r0, #436]
ldrh.w r1, [sp, #54]
ldrh.w r2, [sp, #182]
ldrh.w r3, [sp, #310]
ldrh.w r4, [sp, #438]
ldrh.w r5, [fp, #54]
ldrh.w r6, [fp, #182]
ldrh.w r7, [fp, #310]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #54]
strh.w r3, [r0, #118]
strh.w r8, [r0, #182]
strh.w r6, [r0, #246]
strh.w r9, [r0, #310]
strh.w r5, [r0, #374]
strh.w r7, [r0, #438]
ldrh.w r1, [sp, #56]
ldrh.w r2, [sp, #184]
ldrh.w r3, [sp, #312]
ldrh.w r4, [sp, #440]
ldrh.w r5, [fp, #56]
ldrh.w r6, [fp, #184]
ldrh.w r7, [fp, #312]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #56]
strh.w r3, [r0, #120]
strh.w r8, [r0, #184]
strh.w r6, [r0, #248]
strh.w r9, [r0, #312]
strh.w r5, [r0, #376]
strh.w r7, [r0, #440]
ldrh.w r1, [sp, #58]
ldrh.w r2, [sp, #186]
ldrh.w r3, [sp, #314]
ldrh.w r4, [sp, #442]
ldrh.w r5, [fp, #58]
ldrh.w r6, [fp, #186]
ldrh.w r7, [fp, #314]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #58]
strh.w r3, [r0, #122]
strh.w r8, [r0, #186]
strh.w r6, [r0, #250]
strh.w r9, [r0, #314]
strh.w r5, [r0, #378]
strh.w r7, [r0, #442]
ldrh.w r1, [sp, #60]
ldrh.w r2, [sp, #188]
ldrh.w r3, [sp, #316]
ldrh.w r4, [sp, #444]
ldrh.w r5, [fp, #60]
ldrh.w r6, [fp, #188]
ldrh.w r7, [fp, #316]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #60]
strh.w r3, [r0, #124]
strh.w r8, [r0, #188]
strh.w r6, [r0, #252]
strh.w r9, [r0, #316]
strh.w r5, [r0, #380]
strh.w r7, [r0, #444]
ldrh.w r1, [sp, #62]
ldrh.w r2, [sp, #190]
ldrh.w r3, [sp, #318]
ldrh.w r4, [sp, #446]
ldrh.w r5, [fp, #62]
ldrh.w r6, [fp, #190]
ldrh.w r7, [fp, #318]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
strh.w r1, [r0, #62]
strh.w r3, [r0, #126]
strh.w r8, [r0, #190]
strh.w r6, [r0, #254]
strh.w r9, [r0, #318]
strh.w r5, [r0, #382]
strh.w r7, [r0, #446]
ldrh.w r1, [sp, #64]
ldrh.w r2, [sp, #192]
ldrh.w r3, [sp, #320]
ldrh.w r4, [sp, #448]
ldrh.w r5, [fp, #64]
ldrh.w r6, [fp, #192]
ldrh.w r7, [fp, #320]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #64]
add.w r1, sl
strh.w r1, [r0, #64]
ldrh.w sl, [r0, #128]
add.w r3, sl
strh.w r3, [r0, #128]
ldrh.w sl, [r0, #192]
add.w r8, sl
strh.w r8, [r0, #192]
ldrh.w sl, [r0, #256]
add.w r6, sl
strh.w r6, [r0, #256]
ldrh.w sl, [r0, #320]
add.w r9, sl
strh.w r9, [r0, #320]
ldrh.w sl, [r0, #384]
add.w r5, sl
strh.w r5, [r0, #384]
strh.w r7, [r0, #448]
ldrh.w r1, [sp, #66]
ldrh.w r2, [sp, #194]
ldrh.w r3, [sp, #322]
ldrh.w r4, [sp, #450]
ldrh.w r5, [fp, #66]
ldrh.w r6, [fp, #194]
ldrh.w r7, [fp, #322]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #66]
add.w r1, sl
strh.w r1, [r0, #66]
ldrh.w sl, [r0, #130]
add.w r3, sl
strh.w r3, [r0, #130]
ldrh.w sl, [r0, #194]
add.w r8, sl
strh.w r8, [r0, #194]
ldrh.w sl, [r0, #258]
add.w r6, sl
strh.w r6, [r0, #258]
ldrh.w sl, [r0, #322]
add.w r9, sl
strh.w r9, [r0, #322]
ldrh.w sl, [r0, #386]
add.w r5, sl
strh.w r5, [r0, #386]
strh.w r7, [r0, #450]
ldrh.w r1, [sp, #68]
ldrh.w r2, [sp, #196]
ldrh.w r3, [sp, #324]
ldrh.w r4, [sp, #452]
ldrh.w r5, [fp, #68]
ldrh.w r6, [fp, #196]
ldrh.w r7, [fp, #324]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #68]
add.w r1, sl
strh.w r1, [r0, #68]
ldrh.w sl, [r0, #132]
add.w r3, sl
strh.w r3, [r0, #132]
ldrh.w sl, [r0, #196]
add.w r8, sl
strh.w r8, [r0, #196]
ldrh.w sl, [r0, #260]
add.w r6, sl
strh.w r6, [r0, #260]
ldrh.w sl, [r0, #324]
add.w r9, sl
strh.w r9, [r0, #324]
ldrh.w sl, [r0, #388]
add.w r5, sl
strh.w r5, [r0, #388]
strh.w r7, [r0, #452]
ldrh.w r1, [sp, #70]
ldrh.w r2, [sp, #198]
ldrh.w r3, [sp, #326]
ldrh.w r4, [sp, #454]
ldrh.w r5, [fp, #70]
ldrh.w r6, [fp, #198]
ldrh.w r7, [fp, #326]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #70]
add.w r1, sl
strh.w r1, [r0, #70]
ldrh.w sl, [r0, #134]
add.w r3, sl
strh.w r3, [r0, #134]
ldrh.w sl, [r0, #198]
add.w r8, sl
strh.w r8, [r0, #198]
ldrh.w sl, [r0, #262]
add.w r6, sl
strh.w r6, [r0, #262]
ldrh.w sl, [r0, #326]
add.w r9, sl
strh.w r9, [r0, #326]
ldrh.w sl, [r0, #390]
add.w r5, sl
strh.w r5, [r0, #390]
strh.w r7, [r0, #454]
ldrh.w r1, [sp, #72]
ldrh.w r2, [sp, #200]
ldrh.w r3, [sp, #328]
ldrh.w r4, [sp, #456]
ldrh.w r5, [fp, #72]
ldrh.w r6, [fp, #200]
ldrh.w r7, [fp, #328]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #72]
add.w r1, sl
strh.w r1, [r0, #72]
ldrh.w sl, [r0, #136]
add.w r3, sl
strh.w r3, [r0, #136]
ldrh.w sl, [r0, #200]
add.w r8, sl
strh.w r8, [r0, #200]
ldrh.w sl, [r0, #264]
add.w r6, sl
strh.w r6, [r0, #264]
ldrh.w sl, [r0, #328]
add.w r9, sl
strh.w r9, [r0, #328]
ldrh.w sl, [r0, #392]
add.w r5, sl
strh.w r5, [r0, #392]
strh.w r7, [r0, #456]
ldrh.w r1, [sp, #74]
ldrh.w r2, [sp, #202]
ldrh.w r3, [sp, #330]
ldrh.w r4, [sp, #458]
ldrh.w r5, [fp, #74]
ldrh.w r6, [fp, #202]
ldrh.w r7, [fp, #330]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #74]
add.w r1, sl
strh.w r1, [r0, #74]
ldrh.w sl, [r0, #138]
add.w r3, sl
strh.w r3, [r0, #138]
ldrh.w sl, [r0, #202]
add.w r8, sl
strh.w r8, [r0, #202]
ldrh.w sl, [r0, #266]
add.w r6, sl
strh.w r6, [r0, #266]
ldrh.w sl, [r0, #330]
add.w r9, sl
strh.w r9, [r0, #330]
ldrh.w sl, [r0, #394]
add.w r5, sl
strh.w r5, [r0, #394]
strh.w r7, [r0, #458]
ldrh.w r1, [sp, #76]
ldrh.w r2, [sp, #204]
ldrh.w r3, [sp, #332]
ldrh.w r4, [sp, #460]
ldrh.w r5, [fp, #76]
ldrh.w r6, [fp, #204]
ldrh.w r7, [fp, #332]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #76]
add.w r1, sl
strh.w r1, [r0, #76]
ldrh.w sl, [r0, #140]
add.w r3, sl
strh.w r3, [r0, #140]
ldrh.w sl, [r0, #204]
add.w r8, sl
strh.w r8, [r0, #204]
ldrh.w sl, [r0, #268]
add.w r6, sl
strh.w r6, [r0, #268]
ldrh.w sl, [r0, #332]
add.w r9, sl
strh.w r9, [r0, #332]
ldrh.w sl, [r0, #396]
add.w r5, sl
strh.w r5, [r0, #396]
strh.w r7, [r0, #460]
ldrh.w r1, [sp, #78]
ldrh.w r2, [sp, #206]
ldrh.w r3, [sp, #334]
ldrh.w r4, [sp, #462]
ldrh.w r5, [fp, #78]
ldrh.w r6, [fp, #206]
ldrh.w r7, [fp, #334]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #78]
add.w r1, sl
strh.w r1, [r0, #78]
ldrh.w sl, [r0, #142]
add.w r3, sl
strh.w r3, [r0, #142]
ldrh.w sl, [r0, #206]
add.w r8, sl
strh.w r8, [r0, #206]
ldrh.w sl, [r0, #270]
add.w r6, sl
strh.w r6, [r0, #270]
ldrh.w sl, [r0, #334]
add.w r9, sl
strh.w r9, [r0, #334]
ldrh.w sl, [r0, #398]
add.w r5, sl
strh.w r5, [r0, #398]
strh.w r7, [r0, #462]
ldrh.w r1, [sp, #80]
ldrh.w r2, [sp, #208]
ldrh.w r3, [sp, #336]
ldrh.w r4, [sp, #464]
ldrh.w r5, [fp, #80]
ldrh.w r6, [fp, #208]
ldrh.w r7, [fp, #336]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #80]
add.w r1, sl
strh.w r1, [r0, #80]
ldrh.w sl, [r0, #144]
add.w r3, sl
strh.w r3, [r0, #144]
ldrh.w sl, [r0, #208]
add.w r8, sl
strh.w r8, [r0, #208]
ldrh.w sl, [r0, #272]
add.w r6, sl
strh.w r6, [r0, #272]
ldrh.w sl, [r0, #336]
add.w r9, sl
strh.w r9, [r0, #336]
ldrh.w sl, [r0, #400]
add.w r5, sl
strh.w r5, [r0, #400]
strh.w r7, [r0, #464]
ldrh.w r1, [sp, #82]
ldrh.w r2, [sp, #210]
ldrh.w r3, [sp, #338]
ldrh.w r4, [sp, #466]
ldrh.w r5, [fp, #82]
ldrh.w r6, [fp, #210]
ldrh.w r7, [fp, #338]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #82]
add.w r1, sl
strh.w r1, [r0, #82]
ldrh.w sl, [r0, #146]
add.w r3, sl
strh.w r3, [r0, #146]
ldrh.w sl, [r0, #210]
add.w r8, sl
strh.w r8, [r0, #210]
ldrh.w sl, [r0, #274]
add.w r6, sl
strh.w r6, [r0, #274]
ldrh.w sl, [r0, #338]
add.w r9, sl
strh.w r9, [r0, #338]
ldrh.w sl, [r0, #402]
add.w r5, sl
strh.w r5, [r0, #402]
strh.w r7, [r0, #466]
ldrh.w r1, [sp, #84]
ldrh.w r2, [sp, #212]
ldrh.w r3, [sp, #340]
ldrh.w r4, [sp, #468]
ldrh.w r5, [fp, #84]
ldrh.w r6, [fp, #212]
ldrh.w r7, [fp, #340]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #84]
add.w r1, sl
strh.w r1, [r0, #84]
ldrh.w sl, [r0, #148]
add.w r3, sl
strh.w r3, [r0, #148]
ldrh.w sl, [r0, #212]
add.w r8, sl
strh.w r8, [r0, #212]
ldrh.w sl, [r0, #276]
add.w r6, sl
strh.w r6, [r0, #276]
ldrh.w sl, [r0, #340]
add.w r9, sl
strh.w r9, [r0, #340]
ldrh.w sl, [r0, #404]
add.w r5, sl
strh.w r5, [r0, #404]
strh.w r7, [r0, #468]
ldrh.w r1, [sp, #86]
ldrh.w r2, [sp, #214]
ldrh.w r3, [sp, #342]
ldrh.w r4, [sp, #470]
ldrh.w r5, [fp, #86]
ldrh.w r6, [fp, #214]
ldrh.w r7, [fp, #342]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #86]
add.w r1, sl
strh.w r1, [r0, #86]
ldrh.w sl, [r0, #150]
add.w r3, sl
strh.w r3, [r0, #150]
ldrh.w sl, [r0, #214]
add.w r8, sl
strh.w r8, [r0, #214]
ldrh.w sl, [r0, #278]
add.w r6, sl
strh.w r6, [r0, #278]
ldrh.w sl, [r0, #342]
add.w r9, sl
strh.w r9, [r0, #342]
ldrh.w sl, [r0, #406]
add.w r5, sl
strh.w r5, [r0, #406]
strh.w r7, [r0, #470]
ldrh.w r1, [sp, #88]
ldrh.w r2, [sp, #216]
ldrh.w r3, [sp, #344]
ldrh.w r4, [sp, #472]
ldrh.w r5, [fp, #88]
ldrh.w r6, [fp, #216]
ldrh.w r7, [fp, #344]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #88]
add.w r1, sl
strh.w r1, [r0, #88]
ldrh.w sl, [r0, #152]
add.w r3, sl
strh.w r3, [r0, #152]
ldrh.w sl, [r0, #216]
add.w r8, sl
strh.w r8, [r0, #216]
ldrh.w sl, [r0, #280]
add.w r6, sl
strh.w r6, [r0, #280]
ldrh.w sl, [r0, #344]
add.w r9, sl
strh.w r9, [r0, #344]
ldrh.w sl, [r0, #408]
add.w r5, sl
strh.w r5, [r0, #408]
strh.w r7, [r0, #472]
ldrh.w r1, [sp, #90]
ldrh.w r2, [sp, #218]
ldrh.w r3, [sp, #346]
ldrh.w r4, [sp, #474]
ldrh.w r5, [fp, #90]
ldrh.w r6, [fp, #218]
ldrh.w r7, [fp, #346]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #90]
add.w r1, sl
strh.w r1, [r0, #90]
ldrh.w sl, [r0, #154]
add.w r3, sl
strh.w r3, [r0, #154]
ldrh.w sl, [r0, #218]
add.w r8, sl
strh.w r8, [r0, #218]
ldrh.w sl, [r0, #282]
add.w r6, sl
strh.w r6, [r0, #282]
ldrh.w sl, [r0, #346]
add.w r9, sl
strh.w r9, [r0, #346]
ldrh.w sl, [r0, #410]
add.w r5, sl
strh.w r5, [r0, #410]
strh.w r7, [r0, #474]
ldrh.w r1, [sp, #92]
ldrh.w r2, [sp, #220]
ldrh.w r3, [sp, #348]
ldrh.w r4, [sp, #476]
ldrh.w r5, [fp, #92]
ldrh.w r6, [fp, #220]
ldrh.w r7, [fp, #348]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #92]
add.w r1, sl
strh.w r1, [r0, #92]
ldrh.w sl, [r0, #156]
add.w r3, sl
strh.w r3, [r0, #156]
ldrh.w sl, [r0, #220]
add.w r8, sl
strh.w r8, [r0, #220]
ldrh.w sl, [r0, #284]
add.w r6, sl
strh.w r6, [r0, #284]
ldrh.w sl, [r0, #348]
add.w r9, sl
strh.w r9, [r0, #348]
ldrh.w sl, [r0, #412]
add.w r5, sl
strh.w r5, [r0, #412]
strh.w r7, [r0, #476]
ldrh.w r1, [sp, #94]
ldrh.w r2, [sp, #222]
ldrh.w r3, [sp, #350]
ldrh.w r4, [sp, #478]
ldrh.w r5, [fp, #94]
ldrh.w r6, [fp, #222]
ldrh.w r7, [fp, #350]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #94]
add.w r1, sl
strh.w r1, [r0, #94]
ldrh.w sl, [r0, #158]
add.w r3, sl
strh.w r3, [r0, #158]
ldrh.w sl, [r0, #222]
add.w r8, sl
strh.w r8, [r0, #222]
ldrh.w sl, [r0, #286]
add.w r6, sl
strh.w r6, [r0, #286]
ldrh.w sl, [r0, #350]
add.w r9, sl
strh.w r9, [r0, #350]
ldrh.w sl, [r0, #414]
add.w r5, sl
strh.w r5, [r0, #414]
strh.w r7, [r0, #478]
ldrh.w r1, [sp, #96]
ldrh.w r2, [sp, #224]
ldrh.w r3, [sp, #352]
ldrh.w r4, [sp, #480]
ldrh.w r5, [fp, #96]
ldrh.w r6, [fp, #224]
ldrh.w r7, [fp, #352]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #96]
add.w r1, sl
strh.w r1, [r0, #96]
ldrh.w sl, [r0, #160]
add.w r3, sl
strh.w r3, [r0, #160]
ldrh.w sl, [r0, #224]
add.w r8, sl
strh.w r8, [r0, #224]
ldrh.w sl, [r0, #288]
add.w r6, sl
strh.w r6, [r0, #288]
ldrh.w sl, [r0, #352]
add.w r9, sl
strh.w r9, [r0, #352]
ldrh.w sl, [r0, #416]
add.w r5, sl
strh.w r5, [r0, #416]
strh.w r7, [r0, #480]
ldrh.w r1, [sp, #98]
ldrh.w r2, [sp, #226]
ldrh.w r3, [sp, #354]
ldrh.w r4, [sp, #482]
ldrh.w r5, [fp, #98]
ldrh.w r6, [fp, #226]
ldrh.w r7, [fp, #354]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #98]
add.w r1, sl
strh.w r1, [r0, #98]
ldrh.w sl, [r0, #162]
add.w r3, sl
strh.w r3, [r0, #162]
ldrh.w sl, [r0, #226]
add.w r8, sl
strh.w r8, [r0, #226]
ldrh.w sl, [r0, #290]
add.w r6, sl
strh.w r6, [r0, #290]
ldrh.w sl, [r0, #354]
add.w r9, sl
strh.w r9, [r0, #354]
ldrh.w sl, [r0, #418]
add.w r5, sl
strh.w r5, [r0, #418]
strh.w r7, [r0, #482]
ldrh.w r1, [sp, #100]
ldrh.w r2, [sp, #228]
ldrh.w r3, [sp, #356]
ldrh.w r4, [sp, #484]
ldrh.w r5, [fp, #100]
ldrh.w r6, [fp, #228]
ldrh.w r7, [fp, #356]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #100]
add.w r1, sl
strh.w r1, [r0, #100]
ldrh.w sl, [r0, #164]
add.w r3, sl
strh.w r3, [r0, #164]
ldrh.w sl, [r0, #228]
add.w r8, sl
strh.w r8, [r0, #228]
ldrh.w sl, [r0, #292]
add.w r6, sl
strh.w r6, [r0, #292]
ldrh.w sl, [r0, #356]
add.w r9, sl
strh.w r9, [r0, #356]
ldrh.w sl, [r0, #420]
add.w r5, sl
strh.w r5, [r0, #420]
strh.w r7, [r0, #484]
ldrh.w r1, [sp, #102]
ldrh.w r2, [sp, #230]
ldrh.w r3, [sp, #358]
ldrh.w r4, [sp, #486]
ldrh.w r5, [fp, #102]
ldrh.w r6, [fp, #230]
ldrh.w r7, [fp, #358]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #102]
add.w r1, sl
strh.w r1, [r0, #102]
ldrh.w sl, [r0, #166]
add.w r3, sl
strh.w r3, [r0, #166]
ldrh.w sl, [r0, #230]
add.w r8, sl
strh.w r8, [r0, #230]
ldrh.w sl, [r0, #294]
add.w r6, sl
strh.w r6, [r0, #294]
ldrh.w sl, [r0, #358]
add.w r9, sl
strh.w r9, [r0, #358]
ldrh.w sl, [r0, #422]
add.w r5, sl
strh.w r5, [r0, #422]
strh.w r7, [r0, #486]
ldrh.w r1, [sp, #104]
ldrh.w r2, [sp, #232]
ldrh.w r3, [sp, #360]
ldrh.w r4, [sp, #488]
ldrh.w r5, [fp, #104]
ldrh.w r6, [fp, #232]
ldrh.w r7, [fp, #360]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #104]
add.w r1, sl
strh.w r1, [r0, #104]
ldrh.w sl, [r0, #168]
add.w r3, sl
strh.w r3, [r0, #168]
ldrh.w sl, [r0, #232]
add.w r8, sl
strh.w r8, [r0, #232]
ldrh.w sl, [r0, #296]
add.w r6, sl
strh.w r6, [r0, #296]
ldrh.w sl, [r0, #360]
add.w r9, sl
strh.w r9, [r0, #360]
ldrh.w sl, [r0, #424]
add.w r5, sl
strh.w r5, [r0, #424]
strh.w r7, [r0, #488]
ldrh.w r1, [sp, #106]
ldrh.w r2, [sp, #234]
ldrh.w r3, [sp, #362]
ldrh.w r4, [sp, #490]
ldrh.w r5, [fp, #106]
ldrh.w r6, [fp, #234]
ldrh.w r7, [fp, #362]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #106]
add.w r1, sl
strh.w r1, [r0, #106]
ldrh.w sl, [r0, #170]
add.w r3, sl
strh.w r3, [r0, #170]
ldrh.w sl, [r0, #234]
add.w r8, sl
strh.w r8, [r0, #234]
ldrh.w sl, [r0, #298]
add.w r6, sl
strh.w r6, [r0, #298]
ldrh.w sl, [r0, #362]
add.w r9, sl
strh.w r9, [r0, #362]
ldrh.w sl, [r0, #426]
add.w r5, sl
strh.w r5, [r0, #426]
strh.w r7, [r0, #490]
ldrh.w r1, [sp, #108]
ldrh.w r2, [sp, #236]
ldrh.w r3, [sp, #364]
ldrh.w r4, [sp, #492]
ldrh.w r5, [fp, #108]
ldrh.w r6, [fp, #236]
ldrh.w r7, [fp, #364]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #108]
add.w r1, sl
strh.w r1, [r0, #108]
ldrh.w sl, [r0, #172]
add.w r3, sl
strh.w r3, [r0, #172]
ldrh.w sl, [r0, #236]
add.w r8, sl
strh.w r8, [r0, #236]
ldrh.w sl, [r0, #300]
add.w r6, sl
strh.w r6, [r0, #300]
ldrh.w sl, [r0, #364]
add.w r9, sl
strh.w r9, [r0, #364]
ldrh.w sl, [r0, #428]
add.w r5, sl
strh.w r5, [r0, #428]
strh.w r7, [r0, #492]
ldrh.w r1, [sp, #110]
ldrh.w r2, [sp, #238]
ldrh.w r3, [sp, #366]
ldrh.w r4, [sp, #494]
ldrh.w r5, [fp, #110]
ldrh.w r6, [fp, #238]
ldrh.w r7, [fp, #366]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #110]
add.w r1, sl
strh.w r1, [r0, #110]
ldrh.w sl, [r0, #174]
add.w r3, sl
strh.w r3, [r0, #174]
ldrh.w sl, [r0, #238]
add.w r8, sl
strh.w r8, [r0, #238]
ldrh.w sl, [r0, #302]
add.w r6, sl
strh.w r6, [r0, #302]
ldrh.w sl, [r0, #366]
add.w r9, sl
strh.w r9, [r0, #366]
ldrh.w sl, [r0, #430]
add.w r5, sl
strh.w r5, [r0, #430]
strh.w r7, [r0, #494]
ldrh.w r1, [sp, #112]
ldrh.w r2, [sp, #240]
ldrh.w r3, [sp, #368]
ldrh.w r4, [sp, #496]
ldrh.w r5, [fp, #112]
ldrh.w r6, [fp, #240]
ldrh.w r7, [fp, #368]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #112]
add.w r1, sl
strh.w r1, [r0, #112]
ldrh.w sl, [r0, #176]
add.w r3, sl
strh.w r3, [r0, #176]
ldrh.w sl, [r0, #240]
add.w r8, sl
strh.w r8, [r0, #240]
ldrh.w sl, [r0, #304]
add.w r6, sl
strh.w r6, [r0, #304]
ldrh.w sl, [r0, #368]
add.w r9, sl
strh.w r9, [r0, #368]
ldrh.w sl, [r0, #432]
add.w r5, sl
strh.w r5, [r0, #432]
strh.w r7, [r0, #496]
ldrh.w r1, [sp, #114]
ldrh.w r2, [sp, #242]
ldrh.w r3, [sp, #370]
ldrh.w r4, [sp, #498]
ldrh.w r5, [fp, #114]
ldrh.w r6, [fp, #242]
ldrh.w r7, [fp, #370]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #114]
add.w r1, sl
strh.w r1, [r0, #114]
ldrh.w sl, [r0, #178]
add.w r3, sl
strh.w r3, [r0, #178]
ldrh.w sl, [r0, #242]
add.w r8, sl
strh.w r8, [r0, #242]
ldrh.w sl, [r0, #306]
add.w r6, sl
strh.w r6, [r0, #306]
ldrh.w sl, [r0, #370]
add.w r9, sl
strh.w r9, [r0, #370]
ldrh.w sl, [r0, #434]
add.w r5, sl
strh.w r5, [r0, #434]
strh.w r7, [r0, #498]
ldrh.w r1, [sp, #116]
ldrh.w r2, [sp, #244]
ldrh.w r3, [sp, #372]
ldrh.w r4, [sp, #500]
ldrh.w r5, [fp, #116]
ldrh.w r6, [fp, #244]
ldrh.w r7, [fp, #372]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #116]
add.w r1, sl
strh.w r1, [r0, #116]
ldrh.w sl, [r0, #180]
add.w r3, sl
strh.w r3, [r0, #180]
ldrh.w sl, [r0, #244]
add.w r8, sl
strh.w r8, [r0, #244]
ldrh.w sl, [r0, #308]
add.w r6, sl
strh.w r6, [r0, #308]
ldrh.w sl, [r0, #372]
add.w r9, sl
strh.w r9, [r0, #372]
ldrh.w sl, [r0, #436]
add.w r5, sl
strh.w r5, [r0, #436]
strh.w r7, [r0, #500]
ldrh.w r1, [sp, #118]
ldrh.w r2, [sp, #246]
ldrh.w r3, [sp, #374]
ldrh.w r4, [sp, #502]
ldrh.w r5, [fp, #118]
ldrh.w r6, [fp, #246]
ldrh.w r7, [fp, #374]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #118]
add.w r1, sl
strh.w r1, [r0, #118]
ldrh.w sl, [r0, #182]
add.w r3, sl
strh.w r3, [r0, #182]
ldrh.w sl, [r0, #246]
add.w r8, sl
strh.w r8, [r0, #246]
ldrh.w sl, [r0, #310]
add.w r6, sl
strh.w r6, [r0, #310]
ldrh.w sl, [r0, #374]
add.w r9, sl
strh.w r9, [r0, #374]
ldrh.w sl, [r0, #438]
add.w r5, sl
strh.w r5, [r0, #438]
strh.w r7, [r0, #502]
ldrh.w r1, [sp, #120]
ldrh.w r2, [sp, #248]
ldrh.w r3, [sp, #376]
ldrh.w r4, [sp, #504]
ldrh.w r5, [fp, #120]
ldrh.w r6, [fp, #248]
ldrh.w r7, [fp, #376]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #120]
add.w r1, sl
strh.w r1, [r0, #120]
ldrh.w sl, [r0, #184]
add.w r3, sl
strh.w r3, [r0, #184]
ldrh.w sl, [r0, #248]
add.w r8, sl
strh.w r8, [r0, #248]
ldrh.w sl, [r0, #312]
add.w r6, sl
strh.w r6, [r0, #312]
ldrh.w sl, [r0, #376]
add.w r9, sl
strh.w r9, [r0, #376]
ldrh.w sl, [r0, #440]
add.w r5, sl
strh.w r5, [r0, #440]
strh.w r7, [r0, #504]
ldrh.w r1, [sp, #122]
ldrh.w r2, [sp, #250]
ldrh.w r3, [sp, #378]
ldrh.w r4, [sp, #506]
ldrh.w r5, [fp, #122]
ldrh.w r6, [fp, #250]
ldrh.w r7, [fp, #378]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #122]
add.w r1, sl
strh.w r1, [r0, #122]
ldrh.w sl, [r0, #186]
add.w r3, sl
strh.w r3, [r0, #186]
ldrh.w sl, [r0, #250]
add.w r8, sl
strh.w r8, [r0, #250]
ldrh.w sl, [r0, #314]
add.w r6, sl
strh.w r6, [r0, #314]
ldrh.w sl, [r0, #378]
add.w r9, sl
strh.w r9, [r0, #378]
ldrh.w sl, [r0, #442]
add.w r5, sl
strh.w r5, [r0, #442]
strh.w r7, [r0, #506]
ldrh.w r1, [sp, #124]
ldrh.w r2, [sp, #252]
ldrh.w r3, [sp, #380]
ldrh.w r4, [sp, #508]
ldrh.w r5, [fp, #124]
ldrh.w r6, [fp, #252]
ldrh.w r7, [fp, #380]
add.w r8, r2, r3
mov.w r8, r8, lsr #1
sub.w r8, r8, r1
sub.w r8, r8, r7
add.w r9, r4, r5
sub.w r9, r9, r1, lsl #1
sub.w r9, r9, r7, lsl #7
mov.w r9, r9, lsr #3
sub.w r9, r9, r8
mul.w r9, r9, lr
sub.w r8, r8, r9
sub.w r2, r2, r3
mov.w r2, r2, lsr #1
sub.w r3, r4, r5
mov.w r3, r3, lsr #2
sub.w r3, r3, r2
mul.w r3, r3, lr
add.w r4, r7, r7, lsl #3
add.w r4, r9
add.w r4, r4, r4, lsl #3
add.w r4, r8
add.w r4, r4, r4, lsl #3
add.w r4, r1
sub.w r4, r6, r4
mul.w r4, r4, lr
sub.w r4, r4, r2
mov.w r4, r4, lsr #3
sub.w r4, r4, r3
mul.w r5, r4, ip
sub.w r6, r3, r4
sub.w r3, r2, r6
sub.w r3, r3, r5
ldrh.w sl, [r0, #124]
add.w r1, sl
strh.w r1, [r0, #124]
ldrh.w sl, [r0, #188]
add.w r3, sl
strh.w r3, [r0, #188]
ldrh.w sl, [r0, #252]
add.w r8, sl
strh.w r8, [r0, #252]
ldrh.w sl, [r0, #316]
add.w r6, sl
strh.w r6, [r0, #316]
ldrh.w sl, [r0, #380]
add.w r9, sl
strh.w r9, [r0, #380]
ldrh.w sl, [r0, #444]
add.w r5, sl
strh.w r5, [r0, #444]
strh.w r7, [r0, #508]
add.w sp, sp, #896
ldmia.w sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
bx lr
