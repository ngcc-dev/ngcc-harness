.macro csubq r,x=12
vpsubw		%ymm0,%ymm\r,%ymm\r
vpsraw		$15,%ymm\r,%ymm\x
vpand		%ymm0,%ymm\x,%ymm\x
vpaddw		%ymm\x,%ymm\r,%ymm\r
.endm

.macro caddq r,x=12
vpsraw		$15,%ymm\r,%ymm\x
vpand		%ymm0,%ymm\x,%ymm\x
vpaddw		%ymm\x,%ymm\r,%ymm\r
.endm

.macro mul rh0,rh1,zl0=15,zl1=15,zh0=2,zh1=2
vpmullw		%ymm\zl0,%ymm\rh0,%ymm12
vpmullw		%ymm\zl1,%ymm\rh1,%ymm13

vpmulhw		%ymm\zh0,%ymm\rh0,%ymm\rh0
vpmulhw		%ymm\zh1,%ymm\rh1,%ymm\rh1

vpmulhw		%ymm0,%ymm12,%ymm12
vpmulhw		%ymm0,%ymm13,%ymm13

vpsubw		%ymm12,%ymm\rh0,%ymm\rh0
vpsubw		%ymm13,%ymm\rh1,%ymm\rh1

caddq       \rh0, 12
caddq       \rh1, 12

.endm

.macro barret   r, x=12
vpmulhw        %ymm1,%ymm\r,%ymm\x
vpmullw        %ymm0,%ymm\x,%ymm\x
vpsubw         %ymm\x,%ymm\r,%ymm\r
.endm
