.global polyadd_avx
polyadd_avx:

xor   %rax,%rax
_loopadd:
vmovdqa    (%rsi),%ymm0
vmovdqa    32(%rsi),%ymm1
vmovdqa    64(%rsi),%ymm2
vmovdqa    96(%rsi),%ymm3
vmovdqa    128(%rsi),%ymm4
vmovdqa    160(%rsi),%ymm5
vmovdqa    192(%rsi),%ymm6
vmovdqa    224(%rsi),%ymm7

vmovdqa    (%rdx),%ymm8
vmovdqa    32(%rdx),%ymm9
vmovdqa    64(%rdx),%ymm10
vmovdqa    96(%rdx),%ymm11
vmovdqa    128(%rdx),%ymm12
vmovdqa    160(%rdx),%ymm13
vmovdqa    192(%rdx),%ymm14
vmovdqa    224(%rdx),%ymm15


vpaddw     %ymm0,%ymm8,%ymm8
vpaddw     %ymm1,%ymm9,%ymm9
vpaddw     %ymm2,%ymm10,%ymm10
vpaddw     %ymm3,%ymm11,%ymm11
vpaddw     %ymm4,%ymm12,%ymm12
vpaddw     %ymm5,%ymm13,%ymm13
vpaddw     %ymm6,%ymm14,%ymm14
vpaddw     %ymm7,%ymm15,%ymm15

vmovdqa    %ymm8,(%rdi)
vmovdqa    %ymm9,32(%rdi)
vmovdqa    %ymm10,64(%rdi)
vmovdqa    %ymm11,96(%rdi)
vmovdqa    %ymm12,128(%rdi)
vmovdqa    %ymm13,160(%rdi)
vmovdqa    %ymm14,192(%rdi)
vmovdqa    %ymm15,224(%rdi)

add   $256,%rdi
add   $256,%rdx
add   $256,%rsi
add   $1,%rax
cmp   $6,%rax
jb _loopadd
ret