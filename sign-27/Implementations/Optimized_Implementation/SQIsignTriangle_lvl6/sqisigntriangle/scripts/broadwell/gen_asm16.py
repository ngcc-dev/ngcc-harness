#!/usr/bin/env python3
# Memory-accumulator mulx/adcx/adox Montgomery (p+1 trick) for large N (>8),
# where the N+1-limb accumulator doesn't fit in registers. CIOS with shift.
# Emits fp_mul/fp_sqr.  Validated structure vs the C CIOS (gf<tag>_mul).
import sys
N=int(sys.argv[1]); c=int(sys.argv[2]); f=int(sys.argv[3])
BITS=64*N; p=c*(1<<f)-1; nbits=p.bit_length(); tsh=f%64
ptop=c<<tsh; ptop_off=8*(N-1)
ACC=N+2                      # t[0..N+1]
L=[]; e=L.append
def off(j): return '' if j==0 else '+%d'%(8*j)

e('#include <sqisign_namespace.h>')
e('.intel_syntax noprefix'); e('')
e('#ifdef __APPLE__'); e('.section __TEXT,__const'); e('#else'); e('.section .rodata'); e('#endif')
quads=['0x%016X'%0]*(N-1)+['0x%016X'%ptop]
e('p_plus_1: .quad '+', '.join(quads))
e('#if defined(__linux__) && defined(__ELF__)'); e('.section .note.GNU-stack,"",@progbits'); e('#endif')
e('#include <asm_preamble.h>'); e(''); e('.text'); e('.p2align 4,,15'); e('')


# ---------------- fp_add / fp_sub (headroom=1, memory-cycled single chain) ----------------
def red_sub_q(load_from):  # 2 rounds: subtract q when top bit set (>=2^nbits)
    for _r in range(2):
        e('  mov    rax, [%s+%d]'%(load_from,8*(N-1))); e('  sar    rax, 63')
        e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
        for j in range(N):
            src='rdx' if j==N-1 else 'rax'; opc='sub' if j==0 else 'sbb'
            e('  mov    r8, [%s%s]'%(load_from,off(j))); e('  %s    r8, %s'%(opc,src)); e('  mov    [%s%s], r8'%(load_from,off(j)))

e('.global fp_add'); e('fp_add:')
e('  mov    r8, [rsi]'); e('  add    r8, [rdx]'); e('  mov    [rdi], r8')
for j in range(1,N):
    e('  mov    r8, [rsi%s]'%off(j)); e('  adc    r8, [rdx%s]'%off(j)); e('  mov    [rdi%s], r8'%off(j))
red_sub_q('rdi')
e('  ret'); e('')

e('.global fp_sub'); e('fp_sub:')
e('  mov    r8, [rsi]'); e('  sub    r8, [rdx]'); e('  mov    [rdi], r8')
for j in range(1,N):
    e('  mov    r8, [rsi%s]'%off(j)); e('  sbb    r8, [rdx%s]'%off(j)); e('  mov    [rdi%s], r8'%off(j))
e('  sbb    rax, rax')                     # m = -borrow
# round1: add q&m
e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='add' if j==0 else 'adc'
    e('  mov    r8, [rdi%s]'%off(j)); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%off(j))
# round2: add q&sar
e('  mov    rax, [rdi+%d]'%(8*(N-1))); e('  sar    rax, 63')
e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='add' if j==0 else 'adc'
    e('  mov    r8, [rdi%s]'%off(j)); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%off(j))
e('  ret'); e('')

# ---------------- fp_mul ----------------
e('.global fp_mul'); e('fp_mul:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  mov    rcx, rdx')                 # rcx = b
e('  sub    rsp, %d'%(8*ACC))
# zero t
e('  xor    eax, eax')
for j in range(ACC): e('  mov    [rsp%s], rax'%off(j))
e('  xor    r14d, r14d')               # i (byte offset into b)
e('.Lmul_outer:')
e('  mov    rdx, [rcx + r14]')
# multiply-add: t[0..N-1]+=a*b[i] (dual chain), carry -> t[N], t[N+1]
e('  xor    r12d, r12d')               # ze=0, clears CF/OF
for j in range(N):
    if j==0:
        e('  mulx   r9, r8, [rsi]')
        e('  adcx   r8, [rsp]')
        e('  mov    [rsp], r8')
    else:
        lo,hi = ('r8','r9') if j%2==0 else ('r10','r11')
        prevhi = 'r9' if j%2==1 else 'r11'
        e('  mulx   %s, %s, [rsi%s]'%(hi,lo,off(j)))
        e('  adox   %s, %s'%(lo,prevhi))
        e('  adcx   %s, [rsp%s]'%(lo,off(j)))
        e('  mov    [rsp%s], %s'%(off(j),lo))
lasthi = 'r9' if (N-1)%2==0 else 'r11'
# flush: t[N] += lasthi + OF + CF ; t[N+1] += carry
e('  mov    r13, [rsp+%d]'%(8*N))
e('  adox   %s, r12'%lasthi)
e('  adcx   %s, r13'%lasthi)
e('  mov    [rsp+%d], %s'%(8*N,lasthi))
e('  mov    r13, [rsp+%d]'%(8*(N+1)))
e('  adc    r13, 0')
e('  mov    [rsp+%d], r13'%(8*(N+1)))
# reduce: m=t[0]; t += m*ptop at t[N-1],t[N],carry t[N+1]
e('  mov    rdx, [rsp]')
e('  mulx   r9, r8, [rip+p_plus_1+%d]'%ptop_off)
e('  mov    r13, [rsp+%d]'%(8*(N-1))); e('  add    r13, r8'); e('  mov    [rsp+%d], r13'%(8*(N-1)))
e('  mov    r13, [rsp+%d]'%(8*N));     e('  adc    r13, r9'); e('  mov    [rsp+%d], r13'%(8*N))
e('  mov    r13, [rsp+%d]'%(8*(N+1))); e('  adc    r13, 0');  e('  mov    [rsp+%d], r13'%(8*(N+1)))
# shift down: t[j]=t[j+1], j=0..N; t[N+1]=0
for j in range(N+1):
    e('  mov    r8, [rsp+%d]'%(8*(j+1))); e('  mov    [rsp%s], r8'%off(j))
e('  xor    r8d, r8d'); e('  mov    [rsp+%d], r8'%(8*(N+1)))
e('  add    r14, 8')
e('  cmp    r14, %d'%(8*N))
e('  jne    .Lmul_outer')
# final: t[0..N-1] < 2q; conditional subtract q (headroom=1) ; store to [rdi]
e('  mov    rax, [rsp+%d]'%(8*(N-1)))  # top limb
e('  sar    rax, 63')                  # mask
e('  mov    rdx, [rip+p+%d]'%ptop_off) # q top
e('  and    rdx, rax')
for j in range(N):
    src = 'rdx' if j==N-1 else 'rax'
    opc = 'sub' if j==0 else 'sbb'
    e('  mov    r8, [rsp%s]'%off(j))
    e('  %s    r8, %s'%(opc,src))
    e('  mov    [rdi%s], r8'%off(j))
e('  add    rsp, %d'%(8*ACC))
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')
e('.global fp_sqr'); e('fp_sqr:'); e('  mov    rdx, rsi'); e('  jmp    fp_mul')


# ---------------- fp2 mul kernels (call-based: reuse validated asm fp_mul/fp_sub/fp_add) ----------------
for c0 in (True, False):
    nm = 'fp2_mul_c0' if c0 else 'fp2_mul_c1'
    e(''); e('.global %s'%nm); e('%s:'%nm)
    for r in ['rbx','rbp','r12']: e('  push   %s'%r)
    e('  mov    rbx, rsi'); e('  mov    rbp, rdx'); e('  mov    r12, rdi')
    e('  sub    rsp, %d'%(8*2*N))
    if c0:
        e('  lea    rdi, [rsp]'); e('  mov    rsi, rbx'); e('  mov    rdx, rbp'); e('  call   fp_mul')
        e('  lea    rdi, [rsp+%d]'%(8*N)); e('  lea    rsi, [rbx+%d]'%(8*N)); e('  lea    rdx, [rbp+%d]'%(8*N)); e('  call   fp_mul')
        e('  mov    rdi, r12'); e('  lea    rsi, [rsp]'); e('  lea    rdx, [rsp+%d]'%(8*N)); e('  call   fp_sub')
    else:
        e('  lea    rdi, [rsp]'); e('  mov    rsi, rbx'); e('  lea    rdx, [rbp+%d]'%(8*N)); e('  call   fp_mul')
        e('  lea    rdi, [rsp+%d]'%(8*N)); e('  lea    rsi, [rbx+%d]'%(8*N)); e('  mov    rdx, rbp'); e('  call   fp_mul')
        e('  mov    rdi, r12'); e('  lea    rsi, [rsp]'); e('  lea    rdx, [rsp+%d]'%(8*N)); e('  call   fp_add')
    e('  add    rsp, %d'%(8*2*N))
    for r in ['r12','rbp','rbx']: e('  pop    %s'%r)
    e('  ret')

# fp2_sq_c0 = (a0+a1)(a0-a1) ; fp2_sq_c1 = 2*a0*a1   (reuse asm fp_add/sub/mul)
e(''); e('.global fp2_sq_c0'); e('fp2_sq_c0:')
for r in ['rbx','rbp','r12']: e('  push   %s'%r)
e('  mov    rbx, rsi')                       # in
e('  mov    r12, rdi')                        # out
e('  sub    rsp, %d'%(8*2*N))                 # s at [rsp], d at [rsp+8N]
e('  lea    rdi, [rsp]'); e('  mov    rsi, rbx'); e('  lea    rdx, [rbx+%d]'%(8*N)); e('  call   fp_add')   # s=a0+a1
e('  lea    rdi, [rsp+%d]'%(8*N)); e('  mov    rsi, rbx'); e('  lea    rdx, [rbx+%d]'%(8*N)); e('  call   fp_sub')  # d=a0-a1
e('  mov    rdi, r12'); e('  lea    rsi, [rsp]'); e('  lea    rdx, [rsp+%d]'%(8*N)); e('  call   fp_mul')   # out=s*d
e('  add    rsp, %d'%(8*2*N))
for r in ['r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret')

e(''); e('.global fp2_sq_c1'); e('fp2_sq_c1:')
for r in ['rbx','rbp','r12']: e('  push   %s'%r)
e('  mov    rbx, rsi'); e('  mov    r12, rdi')
e('  sub    rsp, %d'%(8*N))                    # 2a0 at [rsp]
e('  lea    rdi, [rsp]'); e('  mov    rsi, rbx'); e('  mov    rdx, rbx'); e('  call   fp_add')   # 2a0=a0+a0
e('  mov    rdi, r12'); e('  lea    rsi, [rsp]'); e('  lea    rdx, [rbx+%d]'%(8*N)); e('  call   fp_mul')  # out=2a0*a1
e('  add    rsp, %d'%(8*N))
for r in ['r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret')


open('/tmp/gen/fp_asm16_mul.S','w').write('\n'.join(L)+'\n')
print('wrote fp_asm16_mul.S (%d lines)'%len(L))
