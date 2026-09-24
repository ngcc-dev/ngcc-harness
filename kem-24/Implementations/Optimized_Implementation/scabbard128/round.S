.macro round_iter i

vmovdqu (\i*64)*2(%rdi), %ymm0
vmovdqu (\i*64 + 16)*2(%rdi), %ymm1
vmovdqu (\i*64 + 32)*2(%rdi), %ymm2
vmovdqu (\i*64 + 48)*2(%rdi), %ymm3

vpaddw %ymm4, %ymm0, %ymm0
vpaddw %ymm4, %ymm1, %ymm1
vpaddw %ymm4, %ymm2, %ymm2
vpaddw %ymm4, %ymm3, %ymm3

vpand %ymm5, %ymm0, %ymm0
vpand %ymm5, %ymm1, %ymm1
vpand %ymm5, %ymm2, %ymm2
vpand %ymm5, %ymm3, %ymm3

vpsrlw $4, %ymm0, %ymm0
vpsrlw $4, %ymm1, %ymm1
vpsrlw $4, %ymm2, %ymm2
vpsrlw $4, %ymm3, %ymm3

vmovdqu  %ymm0, (\i*64)*2(%rdi)
vmovdqu  %ymm1, (\i*64 + 16)*2(%rdi)
vmovdqu  %ymm2, (\i*64 + 32)*2(%rdi)
vmovdqu  %ymm3, (\i*64 + 48)*2(%rdi)

.endm

.text
.global round_keygen_avx
round_keygen_avx:

vpbroadcastd (%rsi), %ymm4
vpbroadcastd 4*2(%rsi), %ymm5

round_iter 0
round_iter 1
round_iter 2
round_iter 3
round_iter 4
round_iter 5
round_iter 6
round_iter 7
round_iter 8


ret