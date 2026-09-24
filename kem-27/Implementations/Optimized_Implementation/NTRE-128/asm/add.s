.global poly_double
.type poly_double, @function
poly_double:
# rdi = r, rsi = a
# N=648 = 1296 bytes = 40 ymm (32-byte) batches + 16 bytes tail (xmm)

xor %rcx, %rcx
.p2align 5
_looptop_double:
vmovdqu   (%rsi,%rcx), %ymm0
vpaddw %ymm0, %ymm0, %ymm0
vmovdqu %ymm0,    (%rdi,%rcx)

add $32, %rcx
cmp $1280, %rcx
jb  _looptop_double

# tail: 16 bytes (8 int16_t)
vmovdqu   (%rsi,%rcx), %xmm0
vpaddw %xmm0, %xmm0, %xmm0
vmovdqu %xmm0,    (%rdi,%rcx)

ret
.size poly_double, .-poly_double

.global poly_sub
.type poly_sub, @function
poly_sub:
# rdi = r, rsi = a, rdx = b
# N=648 = 1296 bytes = 40 ymm (32-byte) batches + 16 bytes tail (xmm)

xor %rcx, %rcx
.p2align 5
_looptop_sub:
vmovdqu   (%rsi,%rcx), %ymm0
vpsubw    (%rdx,%rcx), %ymm0, %ymm0
vmovdqu %ymm0,    (%rdi,%rcx)

add $32, %rcx
cmp $1280, %rcx
jb  _looptop_sub

# tail: 16 bytes (8 int16_t)
vmovdqu   (%rsi,%rcx), %xmm0
vpsubw    (%rdx,%rcx), %xmm0, %xmm0
vmovdqu %xmm0,    (%rdi,%rcx)

ret
.size poly_sub, .-poly_sub

.section .note.GNU-stack,"",@progbits
