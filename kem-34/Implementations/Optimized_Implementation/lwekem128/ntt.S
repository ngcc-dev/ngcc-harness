#define _16xQ 0
#define _16x2Qinv 4
#define _ZETAS_EXP 8

.include "shuffle.inc"
.include "arith.inc"

.macro update rln,rl0,rl1,rh0,rh1
vpaddw		%ymm\rh0,%ymm\rl0,%ymm\rln
vpsubw		%ymm\rh0,%ymm\rl0,%ymm\rh0

vpaddw		%ymm\rh1,%ymm\rl1,%ymm\rl0
vpsubw		%ymm\rh1,%ymm\rl1,%ymm\rh1

caddq       \rh0, 12
caddq       \rh1, 12

csubq       \rln, 12
csubq       \rl0, 12

.endm

.text
.global ntt_avx
ntt_avx:

vpbroadcastq	(_16xQ)*2(%rsi),%ymm0

vmovdqa	    (%rdi),%ymm8
vmovdqa		(16)*2(%rdi),%ymm9
vmovdqa		(32)*2(%rdi),%ymm10
vmovdqa		(48)*2(%rdi),%ymm11

/* level 0 */

vpbroadcastq	(_ZETAS_EXP+0)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_EXP+4)*2(%rsi),%ymm2


mul		10,11

update		7,8,9,10,11

/* level 1 */
vpbroadcastq	(_ZETAS_EXP+8)*2(%rsi),%ymm15
vpbroadcastq	(_ZETAS_EXP+12)*2(%rsi),%ymm2
vpbroadcastq	(_ZETAS_EXP+16)*2(%rsi),%ymm14
vpbroadcastq	(_ZETAS_EXP+20)*2(%rsi),%ymm3


mul     8, 11, 15, 14, 2, 3

update       6, 7, 10, 8, 11

/* level 2  */

shuffle8	6,8,5,8
shuffle8	7,11,10,11

vmovdqa		(_ZETAS_EXP+24)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_EXP+24+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_EXP+24+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_EXP+24+48)*2(%rsi),%ymm3

mul		8, 11, 15, 14, 2, 3

update       6, 5, 10, 8, 11

/* level 3 */

shuffle4	6,8,7,8
shuffle4	5,11,10,11

vmovdqa		(_ZETAS_EXP+88)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_EXP+88+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_EXP+88+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_EXP+88+48)*2(%rsi),%ymm3

mul        8, 11, 15, 14, 2, 3

update      6, 7, 10, 8, 11

/* level 4 */

shuffle2	6,8,5,8
shuffle2	7,11,9,11

vmovdqa		(_ZETAS_EXP+152)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_EXP+152+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_EXP+152+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_EXP+152+48)*2(%rsi),%ymm3

mul        8, 11, 15, 14, 2, 3

update      6, 5, 9, 8, 11

/* level 5 */

shuffle1	6,8,7,8
shuffle1	5,11,10,11

vmovdqa		(_ZETAS_EXP+216)*2(%rsi),%ymm15
vmovdqa		(_ZETAS_EXP+216+16)*2(%rsi),%ymm2
vmovdqa		(_ZETAS_EXP+216+32)*2(%rsi),%ymm14
vmovdqa		(_ZETAS_EXP+216+48)*2(%rsi),%ymm3

mul        8, 11, 15, 14, 2, 3

update      6, 7, 10, 8, 11

shuffle1       6,  8, 5,  8 
shuffle1       7, 11, 6, 11

shuffle2       5,  8, 7,  8
shuffle2       6, 11, 5, 11

shuffle4       7,  8, 6,  8
shuffle4       5, 11, 7, 11

shuffle8       6,  8, 5,  8
shuffle8       7, 11, 6, 11

vmovdqa		%ymm5,(%rdi)
vmovdqa		%ymm8,(16)*2(%rdi)
vmovdqa		%ymm6,(32)*2(%rdi)
vmovdqa		%ymm11,(48)*2(%rdi)

/*
vmovdqa		%ymm6,(%rdi)
vmovdqa		%ymm8,(16)*2(%rdi)
vmovdqa		%ymm7,(32)*2(%rdi)
vmovdqa		%ymm11,(48)*2(%rdi)
*/

ret