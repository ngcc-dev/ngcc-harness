.global poly_double
.type poly_double, @function
poly_double:
# rdi = r, rsi = a
# N=1296 = 2592 bytes = 81 ymm (32-byte) batches

xor %rcx, %rcx
.p2align 5
_looptop_double:
vmovdqu   (%rsi,%rcx), %ymm0
vpaddw %ymm0, %ymm0, %ymm0
vmovdqu %ymm0,    (%rdi,%rcx)

add $32, %rcx
cmp $2592, %rcx
jb  _looptop_double

ret
.size poly_double, .-poly_double

.global poly_sub
.type poly_sub, @function
poly_sub:
# rdi = r, rsi = a, rdx = b
# N=1296 = 2592 bytes = 81 ymm (32-byte) batches

xor %rcx, %rcx
.p2align 5
_looptop_sub:
vmovdqu   (%rsi,%rcx), %ymm0
vpsubw    (%rdx,%rcx), %ymm0, %ymm0
vmovdqu %ymm0,    (%rdi,%rcx)

add $32, %rcx
cmp $2592, %rcx
jb  _looptop_sub

ret
.size poly_sub, .-poly_sub

.section .note.GNU-stack,"",@progbits
