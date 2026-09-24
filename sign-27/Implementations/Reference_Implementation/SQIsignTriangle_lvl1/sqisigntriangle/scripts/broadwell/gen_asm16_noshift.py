#!/usr/bin/env python3
# No-shift (advancing-base CIOS, p+1 trick) fp_add/sub/mul/sqr + FUSED fp2, N>8 headroom=1.
# 2N+2 product buffer; window slides via r14; result lands in t[N..2N-1] (+t[2N] for fp2).
import sys
N=int(sys.argv[1]); c=int(sys.argv[2]); f=int(sys.argv[3])
BITS=64*N; p=c*(1<<f)-1; nbits=p.bit_length(); tsh=f%64
assert BITS-nbits==1
ptop=c<<tsh; ptop_off=8*(N-1); BUF=2*N+2
L=[]; e=L.append
def o(j): return '' if j==0 else '+%d'%(8*j)
e('#include <sqisign_namespace.h>'); e('.intel_syntax noprefix'); e('')
e('#ifdef __APPLE__'); e('.section __TEXT,__const'); e('#else'); e('.section .rodata'); e('#endif')
e('p_plus_1: .quad '+', '.join(['0x%016X'%0]*(N-1)+['0x%016X'%ptop]))
e('#if defined(__linux__) && defined(__ELF__)'); e('.section .note.GNU-stack,"",@progbits'); e('#endif')
e('#include <asm_preamble.h>'); e(''); e('.text'); e('.p2align 4,,15'); e('')

# ---------- fp_add / fp_sub (unchanged, headroom=1) ----------
def red_sub_q(lf):
    for _r in range(2):
        e('  mov    rax, [%s+%d]'%(lf,8*(N-1))); e('  sar    rax, 63')
        e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
        for j in range(N):
            src='rdx' if j==N-1 else 'rax'; opc='sub' if j==0 else 'sbb'
            e('  mov    r8, [%s%s]'%(lf,o(j))); e('  %s    r8, %s'%(opc,src)); e('  mov    [%s%s], r8'%(lf,o(j)))
e('.global fp_add'); e('fp_add:')
e('  mov    r8, [rsi]'); e('  add    r8, [rdx]'); e('  mov    [rdi], r8')
for j in range(1,N): e('  mov    r8, [rsi%s]'%o(j)); e('  adc    r8, [rdx%s]'%o(j)); e('  mov    [rdi%s], r8'%o(j))
red_sub_q('rdi'); e('  ret'); e('')
e('.global fp_sub'); e('fp_sub:')
e('  mov    r8, [rsi]'); e('  sub    r8, [rdx]'); e('  mov    [rdi], r8')
for j in range(1,N): e('  mov    r8, [rsi%s]'%o(j)); e('  sbb    r8, [rdx%s]'%o(j)); e('  mov    [rdi%s], r8'%o(j))
e('  sbb    rax, rax'); e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='add' if j==0 else 'adc'
    e('  mov    r8, [rdi%s]'%o(j)); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%o(j))
e('  mov    rax, [rdi+%d]'%(8*(N-1))); e('  sar    rax, 63'); e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='add' if j==0 else 'adc'
    e('  mov    r8, [rdi%s]'%o(j)); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%o(j))
e('  ret'); e('')

# ---------- advancing-base helpers (window t[i..i+N+1] at [rsp+r14+8j]) ----------
def mac_ns(mb, mo):   # t[..] += (mem[mb+mo+8j]) * rdx, accumulate at advancing base r14
    def M(j):
        x=mo+8*j; return '[%s]'%mb if x==0 else '[%s+%d]'%(mb,x)
    e('  xor    r12d, r12d')
    e('  mulx   r9, r8, %s'%M(0)); e('  adcx   r8, [rsp + r14]'); e('  mov    [rsp + r14], r8')
    for j in range(1,N):
        lo,hi=('r8','r9') if j%2==0 else ('r10','r11'); prevhi='r9' if j%2==1 else 'r11'
        e('  mulx   %s, %s, %s'%(hi,lo,M(j))); e('  adox   %s, %s'%(lo,prevhi))
        e('  adcx   %s, [rsp + r14 + %d]'%(lo,8*j)); e('  mov    [rsp + r14 + %d], %s'%(8*j,lo))
    lasthi='r9' if (N-1)%2==0 else 'r11'
    e('  mov    r13, [rsp + r14 + %d]'%(8*N)); e('  adox   %s, r12'%lasthi); e('  adcx   %s, r13'%lasthi); e('  mov    [rsp + r14 + %d], %s'%(8*N,lasthi))
    e('  mov    r13, [rsp + r14 + %d]'%(8*(N+1))); e('  adc    r13, 0'); e('  mov    [rsp + r14 + %d], r13'%(8*(N+1)))
def reduce_ns():
    e('  mov    rdx, [rsp + r14]')
    e('  mulx   r9, r8, [rip+p_plus_1+%d]'%ptop_off)
    e('  mov    r13, [rsp + r14 + %d]'%(8*(N-1))); e('  add    r13, r8'); e('  mov    [rsp + r14 + %d], r13'%(8*(N-1)))
    e('  mov    r13, [rsp + r14 + %d]'%(8*N));     e('  adc    r13, r9'); e('  mov    [rsp + r14 + %d], r13'%(8*N))
    e('  mov    r13, [rsp + r14 + %d]'%(8*(N+1))); e('  adc    r13, 0');  e('  mov    [rsp + r14 + %d], r13'%(8*(N+1)))
def zero_buf():
    e('  xor    eax, eax')
    for j in range(BUF): e('  mov    [rsp%s], rax'%o(j))

# ---------- fp_mul / fp_sqr (no-shift) ----------
e('.global fp_mul'); e('fp_mul:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  mov    rcx, rdx'); e('  sub    rsp, %d'%(8*BUF)); zero_buf(); e('  xor    r14d, r14d')
e('.Lmul:')
e('  mov    rdx, [rcx + r14]'); mac_ns('rsi',0); reduce_ns()
e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .Lmul')
e('  mov    rax, [rsp + %d]'%(8*(2*N-1))); e('  sar    rax, 63'); e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='sub' if j==0 else 'sbb'
    e('  mov    r8, [rsp + %d]'%(8*(N+j))); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%o(j))
e('  add    rsp, %d'%(8*BUF))
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')
e('.global fp_sqr'); e('fp_sqr:'); e('  mov    rdx, rsi'); e('  jmp    fp_mul'); e('')

# ---------- FUSED fp2 (no-shift) ----------
A0N=8*BUF; A1N=A0N+8*N; BB=A1N+8*N; FRAME=BB+8*N
S=A0N; D=A1N
if FRAME%16: FRAME+=8
def norm(src_off,dst_off):
    e('  mov    rbx, [rip+p]'); e('  mov    r15, [rip+p+%d]'%ptop_off); e('  xor    eax, eax')
    e('  mov    r8, [rsi+%d]'%src_off); e('  sub    r8, rbx'); e('  mov    [rsp+%d], r8'%dst_off)
    for j in range(1,N-1): e('  mov    r8, [rsi+%d]'%(src_off+8*j)); e('  sbb    r8, rbx'); e('  mov    [rsp+%d], r8'%(dst_off+8*j))
    e('  mov    r8, [rsi+%d]'%(src_off+8*(N-1))); e('  sbb    r8, r15'); e('  mov    [rsp+%d], r8'%(dst_off+8*(N-1)))
    e('  sbb    rax, 0'); e('  and    r15, rax')
    e('  mov    r8, [rsp+%d]'%dst_off); e('  add    r8, rax'); e('  mov    [rsp+%d], r8'%dst_off)
    for j in range(1,N-1): e('  mov    r8, [rsp+%d]'%(dst_off+8*j)); e('  adc    r8, rax'); e('  mov    [rsp+%d], r8'%(dst_off+8*j))
    e('  mov    r8, [rsp+%d]'%(dst_off+8*(N-1))); e('  adc    r8, r15'); e('  mov    [rsp+%d], r8'%(dst_off+8*(N-1)))
def final2_ns():    # result at t[N..2N] (N+1 limbs); 2x "subtract p if >=p"; write t[N..2N-1]->[rdi]
    for _r in range(2):
        e('  mov    rbx, [rip+p]'); e('  mov    r15, [rip+p+%d]'%ptop_off)
        e('  mov    r8, [rsp+%d]'%(8*N)); e('  sub    r8, rbx'); e('  mov    [rsp+%d], r8'%(8*N))
        for j in range(1,N-1): e('  mov    r8, [rsp+%d]'%(8*(N+j))); e('  sbb    r8, rbx'); e('  mov    [rsp+%d], r8'%(8*(N+j)))
        e('  mov    r8, [rsp+%d]'%(8*(2*N-1))); e('  sbb    r8, r15'); e('  mov    [rsp+%d], r8'%(8*(2*N-1)))
        e('  mov    r8, [rsp+%d]'%(8*(2*N))); e('  sbb    r8, 0'); e('  mov    [rsp+%d], r8'%(8*(2*N)))
        e('  sbb    rax, rax'); e('  and    r15, rax')
        e('  mov    r8, [rsp+%d]'%(8*N)); e('  add    r8, rax'); e('  mov    [rsp+%d], r8'%(8*N))
        for j in range(1,N-1): e('  mov    r8, [rsp+%d]'%(8*(N+j))); e('  adc    r8, rax'); e('  mov    [rsp+%d], r8'%(8*(N+j)))
        e('  mov    r8, [rsp+%d]'%(8*(2*N-1))); e('  adc    r8, r15'); e('  mov    [rsp+%d], r8'%(8*(2*N-1)))
        e('  mov    r8, [rsp+%d]'%(8*(2*N))); e('  adc    r8, 0'); e('  mov    [rsp+%d], r8'%(8*(2*N)))
    for j in range(N): e('  mov    r8, [rsp+%d]'%(8*(N+j))); e('  mov    [rdi%s], r8'%o(j))

for c0 in (True, False):
    nm='fp2_mul_c0' if c0 else 'fp2_mul_c1'
    e('.global %s'%nm); e('%s:'%nm)
    for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
    e('  mov    rcx, rdx'); e('  sub    rsp, %d'%FRAME)
    norm(0,A0N); norm(8*N,A1N)
    if c0:
        e('  mov    r8, [rip+p2]'); e('  sub    r8, [rcx+%d]'%(8*N)); e('  mov    [rsp+%d], r8'%BB)
        for j in range(1,N): e('  mov    r8, [rip+p2+%d]'%(8*j)); e('  sbb    r8, [rcx+%d]'%(8*N+8*j)); e('  mov    [rsp+%d], r8'%(BB+8*j))
    zero_buf(); e('  xor    r14d, r14d')
    e('.L%s:'%nm)
    if c0:
        e('  mov    rdx, [rcx + r14]'); mac_ns('rsp',A0N)
        e('  lea    rax, [rsp + r14]'); e('  mov    rdx, [rax + %d]'%BB); mac_ns('rsp',A1N)
    else:
        e('  mov    rdx, [rcx + r14 + %d]'%(8*N)); mac_ns('rsp',A0N)
        e('  mov    rdx, [rcx + r14]'); mac_ns('rsp',A1N)
    reduce_ns()
    e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .L%s'%nm)
    final2_ns(); e('  add    rsp, %d'%FRAME)
    for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
    e('  ret'); e('')

# fp2_sq_c0 = (a0+a1)*(a0-a1)
e('.global fp2_sq_c0'); e('fp2_sq_c0:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  sub    rsp, %d'%FRAME)
e('  mov    r8, [rsi]'); e('  add    r8, [rsi+%d]'%(8*N)); e('  mov    [rsp+%d], r8'%S)
for j in range(1,N): e('  mov    r8, [rsi+%d]'%(8*j)); e('  adc    r8, [rsi+%d]'%(8*N+8*j)); e('  mov    [rsp+%d], r8'%(S+8*j))
e('  mov    r8, [rsi]'); e('  sub    r8, [rsi+%d]'%(8*N)); e('  mov    [rsp+%d], r8'%D)
for j in range(1,N): e('  mov    r8, [rsi+%d]'%(8*j)); e('  sbb    r8, [rsi+%d]'%(8*N+8*j)); e('  mov    [rsp+%d], r8'%(D+8*j))
e('  sbb    rax, rax'); e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
e('  mov    r8, [rsp+%d]'%D); e('  add    r8, rax'); e('  mov    [rsp+%d], r8'%D)
for j in range(1,N-1): e('  mov    r8, [rsp+%d]'%(D+8*j)); e('  adc    r8, rax'); e('  mov    [rsp+%d], r8'%(D+8*j))
e('  mov    r8, [rsp+%d]'%(D+8*(N-1))); e('  adc    r8, rdx'); e('  mov    [rsp+%d], r8'%(D+8*(N-1)))
e('  mov    rax, [rsp+%d]'%(D+8*(N-1))); e('  sar    rax, 63'); e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
e('  mov    r8, [rsp+%d]'%D); e('  add    r8, rax'); e('  mov    [rsp+%d], r8'%D)
for j in range(1,N-1): e('  mov    r8, [rsp+%d]'%(D+8*j)); e('  adc    r8, rax'); e('  mov    [rsp+%d], r8'%(D+8*j))
e('  mov    r8, [rsp+%d]'%(D+8*(N-1))); e('  adc    r8, rdx'); e('  mov    [rsp+%d], r8'%(D+8*(N-1)))
zero_buf(); e('  xor    r14d, r14d')
e('.Lsqc0:')
e('  lea    rax, [rsp + r14]'); e('  mov    rdx, [rax + %d]'%S); mac_ns('rsp',D); reduce_ns()
e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .Lsqc0')
final2_ns(); e('  add    rsp, %d'%FRAME)
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')

# fp2_sq_c1 = 2*a0*a1
e('.global fp2_sq_c1'); e('fp2_sq_c1:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  sub    rsp, %d'%FRAME)
e('  mov    r8, [rsi]'); e('  add    r8, [rsi]'); e('  mov    [rsp+%d], r8'%S)
for j in range(1,N): e('  mov    r8, [rsi+%d]'%(8*j)); e('  adc    r8, [rsi+%d]'%(8*j)); e('  mov    [rsp+%d], r8'%(S+8*j))
zero_buf(); e('  xor    r14d, r14d')
e('.Lsqc1:')
e('  lea    rax, [rsp + r14]'); e('  mov    rdx, [rax + %d]'%S); mac_ns('rsi',8*N); reduce_ns()
e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .Lsqc1')
final2_ns(); e('  add    rsp, %d'%FRAME)
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')

open('/tmp/gen/fp_asm16_noshift.S','w').write('\n'.join(L)+'\n')
print('wrote fp_asm16_noshift.S (%d lines), BUF=%d FRAME=%d'%(len(L),BUF,FRAME))
