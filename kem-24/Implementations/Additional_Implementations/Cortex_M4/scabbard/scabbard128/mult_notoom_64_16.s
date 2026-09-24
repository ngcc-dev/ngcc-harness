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
.global polymul_asm
.type polymul_asm, %function
.align 2
polymul_asm:
stmdb sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
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
pop {r1}
push {r1}
push {r2}
push {r0}
add.w r1, r1, #64
add.w r2, r2, #64
add.w r0, r0, #128
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
sub.w r1, r1, #64
sub.w r2, r2, #64
sub sp, #256
ldr r3, [r1, #0]
ldr r4, [r1, #64]
ldr r5, [r1, #4]
ldr r6, [r1, #68]
ldr r7, [r1, #8]
ldr.w r8, [r1, #72]
ldr.w r9, [r1, #12]
ldr.w sl, [r1, #76]
ldr.w fp, [r1, #16]
ldr.w ip, [r1, #80]
ldr.w r0, [r1, #20]
ldr.w lr, [r1, #84]
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
ldr r4, [r1, #88]
ldr r5, [r1, #28]
ldr r6, [r1, #92]
ldr r7, [r1, #32]
ldr.w r8, [r1, #96]
ldr.w r9, [r1, #36]
ldr.w sl, [r1, #100]
ldr.w fp, [r1, #40]
ldr.w ip, [r1, #104]
ldr.w r0, [r1, #44]
ldr.w lr, [r1, #108]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
uadd16 r7, r7, r8
uadd16 r9, r9, sl
uadd16 fp, fp, ip
uadd16 r0, r0, lr
str r3, [sp, #24]
str r5, [sp, #28]
str.w r7, [sp, #32]
str.w r9, [sp, #36]
str.w fp, [sp, #40]
str r0, [sp, #44]
ldr r3, [r1, #48]
ldr r4, [r1, #112]
ldr r5, [r1, #52]
ldr r6, [r1, #116]
ldr r7, [r1, #56]
ldr.w r8, [r1, #120]
ldr.w r9, [r1, #60]
ldr.w sl, [r1, #124]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
uadd16 r7, r7, r8
uadd16 r9, r9, sl
str r3, [sp, #48]
str r5, [sp, #52]
str.w r7, [sp, #56]
str.w r9, [sp, #60]
mov r1, sp
ldr r3, [r2, #0]
ldr r4, [r2, #64]
ldr r5, [r2, #4]
ldr r6, [r2, #68]
ldr r7, [r2, #8]
ldr.w r8, [r2, #72]
ldr.w r9, [r2, #12]
ldr.w sl, [r2, #76]
ldr.w fp, [r2, #16]
ldr.w ip, [r2, #80]
ldr.w r0, [r2, #20]
ldr.w lr, [r2, #84]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
uadd16 r7, r7, r8
uadd16 r9, r9, sl
uadd16 fp, fp, ip
uadd16 r0, r0, lr
str r3, [sp, #64]
str r5, [sp, #68]
str.w r7, [sp, #72]
str.w r9, [sp, #76]
str.w fp, [sp, #80]
str r0, [sp, #84]
ldr r3, [r2, #24]
ldr r4, [r2, #88]
ldr r5, [r2, #28]
ldr r6, [r2, #92]
ldr r7, [r2, #32]
ldr.w r8, [r2, #96]
ldr.w r9, [r2, #36]
ldr.w sl, [r2, #100]
ldr.w fp, [r2, #40]
ldr.w ip, [r2, #104]
ldr.w r0, [r2, #44]
ldr.w lr, [r2, #108]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
uadd16 r7, r7, r8
uadd16 r9, r9, sl
uadd16 fp, fp, ip
uadd16 r0, r0, lr
str r3, [sp, #88]
str r5, [sp, #92]
str.w r7, [sp, #96]
str.w r9, [sp, #100]
str.w fp, [sp, #104]
str r0, [sp, #108]
ldr r3, [r2, #48]
ldr r4, [r2, #112]
ldr r5, [r2, #52]
ldr r6, [r2, #116]
ldr r7, [r2, #56]
ldr.w r8, [r2, #120]
ldr.w r9, [r2, #60]
ldr.w sl, [r2, #124]
uadd16 r3, r3, r4
uadd16 r5, r5, r6
uadd16 r7, r7, r8
uadd16 r9, r9, sl
str r3, [sp, #112]
str r5, [sp, #116]
str.w r7, [sp, #120]
str.w r9, [sp, #124]
add r2, sp, #64
add r0, sp, #128
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
pop {r1}
mov r3, r0
ldr r0, [sp, #256]
ldr r5, [r3, #0]
ldr r6, [r3, #64]
ldr r7, [r0, #0]
ldr.w r8, [r0, #64]
ldr.w r9, [r0, #128]
ldr.w sl, [r0, #192]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #64]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #128]
ldr r5, [r3, #4]
ldr r6, [r3, #68]
ldr.w r7, [r0, #4]
ldr.w r8, [r0, #68]
ldr.w r9, [r0, #132]
ldr.w sl, [r0, #196]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #68]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #132]
ldr r5, [r3, #8]
ldr r6, [r3, #72]
ldr.w r7, [r0, #8]
ldr.w r8, [r0, #72]
ldr.w r9, [r0, #136]
ldr.w sl, [r0, #200]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #72]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #136]
ldr r5, [r3, #12]
ldr r6, [r3, #76]
ldr.w r7, [r0, #12]
ldr.w r8, [r0, #76]
ldr.w r9, [r0, #140]
ldr.w sl, [r0, #204]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #76]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #140]
ldr r5, [r3, #16]
ldr r6, [r3, #80]
ldr.w r7, [r0, #16]
ldr.w r8, [r0, #80]
ldr.w r9, [r0, #144]
ldr.w sl, [r0, #208]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #80]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #144]
ldr r5, [r3, #20]
ldr r6, [r3, #84]
ldr.w r7, [r0, #20]
ldr.w r8, [r0, #84]
ldr.w r9, [r0, #148]
ldr.w sl, [r0, #212]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #84]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #148]
ldr r5, [r3, #24]
ldr r6, [r3, #88]
ldr.w r7, [r0, #24]
ldr.w r8, [r0, #88]
ldr.w r9, [r0, #152]
ldr.w sl, [r0, #216]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #88]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #152]
ldr r5, [r3, #28]
ldr r6, [r3, #92]
ldr.w r7, [r0, #28]
ldr.w r8, [r0, #92]
ldr.w r9, [r0, #156]
ldr.w sl, [r0, #220]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #92]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #156]
ldr r5, [r3, #32]
ldr r6, [r3, #96]
ldr.w r7, [r0, #32]
ldr.w r8, [r0, #96]
ldr.w r9, [r0, #160]
ldr.w sl, [r0, #224]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #96]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #160]
ldr r5, [r3, #36]
ldr r6, [r3, #100]
ldr.w r7, [r0, #36]
ldr.w r8, [r0, #100]
ldr.w r9, [r0, #164]
ldr.w sl, [r0, #228]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #100]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #164]
ldr r5, [r3, #40]
ldr r6, [r3, #104]
ldr.w r7, [r0, #40]
ldr.w r8, [r0, #104]
ldr.w r9, [r0, #168]
ldr.w sl, [r0, #232]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #104]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #168]
ldr r5, [r3, #44]
ldr r6, [r3, #108]
ldr.w r7, [r0, #44]
ldr.w r8, [r0, #108]
ldr.w r9, [r0, #172]
ldr.w sl, [r0, #236]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #108]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #172]
ldr r5, [r3, #48]
ldr r6, [r3, #112]
ldr.w r7, [r0, #48]
ldr.w r8, [r0, #112]
ldr.w r9, [r0, #176]
ldr.w sl, [r0, #240]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #112]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #176]
ldr r5, [r3, #52]
ldr r6, [r3, #116]
ldr.w r7, [r0, #52]
ldr.w r8, [r0, #116]
ldr.w r9, [r0, #180]
ldr.w sl, [r0, #244]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #116]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #180]
ldr r5, [r3, #56]
ldr r6, [r3, #120]
ldr.w r7, [r0, #56]
ldr.w r8, [r0, #120]
ldr.w r9, [r0, #184]
ldr.w sl, [r0, #248]
usub16 fp, r5, r7
usub16 r8, r8, r9
uadd16 fp, fp, r8
str.w fp, [r0, #120]
usub16 r6, r6, r8
usub16 r6, r6, sl
str.w r6, [r0, #184]
ldrh.w r5, [r3, #60]
ldrh.w r6, [r3, #124]
ldrh.w r7, [r0, #60]
ldrh.w r8, [r0, #124]
ldrh.w r9, [r0, #188]
ldrh.w sl, [r0, #252]
sub.w fp, r5, r7
sub.w r8, r8, r9
add.w fp, r8
strh.w fp, [r0, #124]
sub.w r6, r6, r8
sub.w r6, r6, sl
strh.w r6, [r0, #188]
ldrh r5, [r3, #62]
ldrh r7, [r0, #62]
ldrh.w r9, [r0, #190]
sub.w fp, r5, r7
sub.w fp, fp, r9
strh.w fp, [r0, #126]
add sp, #260
pop {r2}
pop.w {r1}
ldmia.w sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
bx lr
