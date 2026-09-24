.global poly_double
.type poly_double, @function
poly_double:
# rdi = r, rsi = a
# N=2304 = 4608 bytes = 144 ymm (32-byte) batches

xor %rcx, %rcx
.p2align 5
_looptop_double:
vmovdqu   (%rsi,%rcx), %ymm0
vpaddw %ymm0, %ymm0, %ymm0
vmovdqu %ymm0,    (%rdi,%rcx)

add $32, %rcx
cmp $4608, %rcx
jb  _looptop_double

ret
.size poly_double, .-poly_double

.section .note.GNU-stack,"",@progbits
