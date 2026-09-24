.global polydouble_avx
polydouble_avx:

xor   %rax,%rax
_loopdouble:
vmovdqa    (%rsi),%ymm0
vmovdqa    32(%rsi),%ymm1
vmovdqa    64(%rsi),%ymm2
vmovdqa    96(%rsi),%ymm3
vmovdqa    128(%rsi),%ymm4
vmovdqa    160(%rsi),%ymm5
vmovdqa    192(%rsi),%ymm6
vmovdqa    224(%rsi),%ymm7

vpaddw     %ymm0,%ymm0,%ymm8
vpaddw     %ymm1,%ymm1,%ymm9
vpaddw     %ymm2,%ymm2,%ymm10
vpaddw     %ymm3,%ymm3,%ymm11
vpaddw     %ymm4,%ymm4,%ymm12
vpaddw     %ymm5,%ymm5,%ymm13
vpaddw     %ymm6,%ymm6,%ymm14
vpaddw     %ymm7,%ymm7,%ymm15


vmovdqa    %ymm8,(%rdi)
vmovdqa    %ymm9,32(%rdi)
vmovdqa    %ymm10,64(%rdi)
vmovdqa    %ymm11,96(%rdi)
vmovdqa    %ymm12,128(%rdi)
vmovdqa    %ymm13,160(%rdi)
vmovdqa    %ymm14,192(%rdi)
vmovdqa    %ymm15,224(%rdi)

add   $256,%rdi
add   $256,%rsi
add   $1,%rax
cmp   $6,%rax
jb _loopdouble
ret