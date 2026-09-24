#!/usr/bin/env python3
# No-shift fp_mul (advancing-base CIOS, p+1 trick) for N>8, headroom=1.
# 2N+2 stack buffer; window slides via r14 base; result lands in t[N..2N-1].
import sys
N=int(sys.argv[1]); c=int(sys.argv[2]); f=int(sys.argv[3])
BITS=64*N; p=c*(1<<f)-1; nbits=p.bit_length(); tsh=f%64
ptop=c<<tsh; ptop_off=8*(N-1); BUF=2*N+2
L=[]; e=L.append
def o(j): return '' if j==0 else '+%d'%(8*j)
e('#include <sqisign_namespace.h>'); e('.intel_syntax noprefix'); e('')
e('#ifdef __APPLE__'); e('.section __TEXT,__const'); e('#else'); e('.section .rodata'); e('#endif')
e('p_plus_1: .quad '+', '.join(['0x%016X'%0]*(N-1)+['0x%016X'%ptop]))
e('#if defined(__linux__) && defined(__ELF__)'); e('.section .note.GNU-stack,"",@progbits'); e('#endif')
e('#include <asm_preamble.h>'); e(''); e('.text'); e('.p2align 4,,15'); e('')

e('.global fp_mul'); e('fp_mul:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  mov    rcx, rdx')                       # rcx = b
e('  sub    rsp, %d'%(8*BUF))
e('  xor    eax, eax')
for j in range(BUF): e('  mov    [rsp%s], rax'%o(j))
e('  xor    r14d, r14d')                      # base = 8*i
e('.Lmo:')
e('  mov    rdx, [rcx + r14]')                # b[i]
# ---- mac: t[i+0..i+N-1] += a*b[i]; flush -> t[i+N], t[i+N+1] ----
e('  xor    r12d, r12d')
e('  mulx   r9, r8, [rsi]')
e('  adcx   r8, [rsp + r14]')
e('  mov    [rsp + r14], r8')
for j in range(1,N):
    lo,hi=('r8','r9') if j%2==0 else ('r10','r11'); prevhi='r9' if j%2==1 else 'r11'
    e('  mulx   %s, %s, [rsi%s]'%(hi,lo,o(j)))
    e('  adox   %s, %s'%(lo,prevhi))
    e('  adcx   %s, [rsp + r14 + %d]'%(lo,8*j))
    e('  mov    [rsp + r14 + %d], %s'%(8*j,lo))
lasthi='r9' if (N-1)%2==0 else 'r11'
e('  mov    r13, [rsp + r14 + %d]'%(8*N))
e('  adox   %s, r12'%lasthi)
e('  adcx   %s, r13'%lasthi)
e('  mov    [rsp + r14 + %d], %s'%(8*N,lasthi))
e('  mov    r13, [rsp + r14 + %d]'%(8*(N+1)))
e('  adc    r13, 0')
e('  mov    [rsp + r14 + %d], r13'%(8*(N+1)))
# ---- reduce: m=t[i]; t[i+N-1]+=m*ptop; (NO shift) ----
e('  mov    rdx, [rsp + r14]')
e('  mulx   r9, r8, [rip+p_plus_1+%d]'%ptop_off)
e('  mov    r13, [rsp + r14 + %d]'%(8*(N-1))); e('  add    r13, r8'); e('  mov    [rsp + r14 + %d], r13'%(8*(N-1)))
e('  mov    r13, [rsp + r14 + %d]'%(8*N));     e('  adc    r13, r9'); e('  mov    [rsp + r14 + %d], r13'%(8*N))
e('  mov    r13, [rsp + r14 + %d]'%(8*(N+1))); e('  adc    r13, 0');  e('  mov    [rsp + r14 + %d], r13'%(8*(N+1)))
e('  add    r14, 8')
e('  cmp    r14, %d'%(8*N))
e('  jne    .Lmo')
# ---- final: result at t[N..2N-1]; 1 conditional subtract -> [rdi] ----
e('  mov    rax, [rsp + %d]'%(8*(2*N-1))); e('  sar    rax, 63')
e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='sub' if j==0 else 'sbb'
    e('  mov    r8, [rsp + %d]'%(8*(N+j))); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%o(j))
e('  add    rsp, %d'%(8*BUF))
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')
e('.global fp_sqr'); e('fp_sqr:'); e('  mov    rdx, rsi'); e('  jmp    fp_mul')
open('/tmp/gen/fp_mul_noshift.S','w').write('\n'.join(L)+'\n')
print('wrote fp_mul_noshift.S (%d lines), BUF=%d limbs'%(len(L),BUF))
