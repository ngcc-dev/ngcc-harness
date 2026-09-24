#define _16xQ 0
#define _16x2Qinv 4
#define _ZETAS_EXP 8
#define _REV_B 288
#define _REV_W 304
#define _INV_N 320

.include "shuffle.inc"
.include "arith.inc"

.macro butterfly rl0,rl1,rh0,rh1,zl0=15,zl1=15,zh0=2,zh1=2
vpsubw		%ymm\rl0,%ymm\rh0,%ymm12
vpaddw		%ymm\rh0,%ymm\rl0,%ymm\rl0
vpsubw		%ymm\rl1,%ymm\rh1,%ymm13
vpaddw		%ymm\rh1,%ymm\rl1,%ymm\rl1

csubq       \rl0, \rh0
csubq       \rl1, \rh1
caddq       12, \rh0
caddq       13, \rh1

vpmullw		%ymm\zl0,%ymm12,%ymm\rh0
vpmullw		%ymm\zl1,%ymm13,%ymm\rh1

vpmulhw		%ymm\zh0,%ymm12,%ymm12
vpmulhw		%ymm\zh1,%ymm13,%ymm13

vpmulhw		%ymm0,%ymm\rh0,%ymm\rh0
vpmulhw		%ymm0,%ymm\rh1,%ymm\rh1

vpsubw		%ymm\rh0,%ymm12,%ymm\rh0
vpsubw		%ymm\rh1,%ymm13,%ymm\rh1

caddq       \rh0, 12
caddq       \rh1, 12

.endm


.text
.global invntt_avx
invntt_avx:
vpbroadcastq    _16xQ*2(%rsi),%ymm0
vpbroadcastq    _16x2Qinv*2(%rsi),%ymm1

vmovdqa	    (%rdi),%ymm10
vmovdqa		(16)*2(%rdi),%ymm9
vmovdqa		(32)*2(%rdi),%ymm7
vmovdqa		(48)*2(%rdi),%ymm11

shuffle8         10, 9, 8, 9
shuffle8        7,11, 10,11

shuffle4         8, 9, 7, 9
shuffle4         10,11, 8,11

shuffle2         7, 9, 10, 9
shuffle2         8,11, 7, 11

shuffle1         10, 9, 8, 9
shuffle1         7,11, 10,11

/* level 5 */

/* reversing the halfword order of packed twiddle factors */
vpermq		$0x4E,(_ZETAS_EXP+216)*2(%rsi),%ymm14
vpermq		$0x4E,(_ZETAS_EXP+216+16)*2(%rsi),%ymm3
vpermq		$0x4E,(_ZETAS_EXP+216+32)*2(%rsi),%ymm15
vpermq		$0x4E,(_ZETAS_EXP+216+48)*2(%rsi),%ymm2

vmovdqa		(_REV_B)*2(%rsi),%ymm13

vpshufb     %ymm13,%ymm14,%ymm14
vpshufb     %ymm13,%ymm3,%ymm3
vpshufb     %ymm13,%ymm15,%ymm15
vpshufb     %ymm13,%ymm2,%ymm2
/* end reverse */

butterfly	8,10,9,11,15,14,2,3


shuffle1	8,9,6,9
shuffle1	10,11,7,11

/* level 4 */

# reversing the word order of packed twiddle factors 
vmovdqa		(_REV_W)*2(%rsi),%ymm1

vpermd		(_ZETAS_EXP+152)*2(%rsi),%ymm1,%ymm14
vpermd		(_ZETAS_EXP+152+16)*2(%rsi),%ymm1,%ymm3
vpermd		(_ZETAS_EXP+152+32)*2(%rsi),%ymm1,%ymm15
vpermd		(_ZETAS_EXP+152+48)*2(%rsi),%ymm1,%ymm2
# end reverse 

butterfly	6,7,9,11,15,14,2,3

shuffle2	6,9,5,9
shuffle2	7,11,6,10

/* level 3 */ 

# reversing the doubleword order of packed twiddle factors 
vpermq		$0x1B,(_ZETAS_EXP+88)*2(%rsi),%ymm14
vpermq		$0x1B,(_ZETAS_EXP+88+16)*2(%rsi),%ymm3
vpermq		$0x1B,(_ZETAS_EXP+88+32)*2(%rsi),%ymm15
vpermq		$0x1B,(_ZETAS_EXP+88+48)*2(%rsi),%ymm2
# end reverse 

butterfly	5,6,9,10,15,14,2,3

shuffle4	5,9,7,9
shuffle4	6,10,8,10

/* level 2 */ 

# reversing the quadword order of packed twiddle factors 
vpermq		$0x4E,(_ZETAS_EXP+24)*2(%rsi),%ymm14
vpermq		$0x4E,(_ZETAS_EXP+24+16)*2(%rsi),%ymm3
vpermq		$0x4E,(_ZETAS_EXP+24+32)*2(%rsi),%ymm15
vpermq		$0x4E,(_ZETAS_EXP+24+48)*2(%rsi),%ymm2
# end reverse 

butterfly	7,8,9,10,15,14,2,3

shuffle8	7,9,6,9
shuffle8	8,10,7,10

/* level 1 */ 

vpbroadcastq	(_ZETAS_EXP+8)*2(%rsi),%ymm14
vpbroadcastq	(_ZETAS_EXP+12)*2(%rsi),%ymm3
vpbroadcastq	(_ZETAS_EXP+16)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_EXP+20)*2(%rsi),%ymm2

butterfly	6,7,9,10,15,14,2,3

/* level 0 */ 

vpbroadcastq	(_ZETAS_EXP+0)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_EXP+4)*2(%rsi),%ymm2

butterfly	6,9,7,10,15,15,2,2

vpbroadcastq	(_INV_N)*2(%rsi),%ymm15
vpbroadcastq	(_INV_N+4)*2(%rsi),%ymm2

mul        6,9,15,15,2,2
mul        7,10,15,15,2,2

vmovdqa	    %ymm6, (%rdi)
vmovdqa		%ymm9, (16)*2(%rdi)
vmovdqa		%ymm7, (32)*2(%rdi)
vmovdqa		%ymm10, (48)*2(%rdi)


ret

