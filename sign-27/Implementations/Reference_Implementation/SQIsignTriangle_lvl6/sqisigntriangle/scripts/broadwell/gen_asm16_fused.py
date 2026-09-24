#!/usr/bin/env python3
# N>8 memory-accumulator mulx/adcx/adox Montgomery (p+1 trick), headroom=1.
# Emits fp_add/fp_sub/fp_mul/fp_sqr (unchanged) + FUSED inline fp2 kernels
# (interleaved double-product, like lvl1/3/5; NO call to fp_mul).
import sys
N=int(sys.argv[1]); c=int(sys.argv[2]); f=int(sys.argv[3])
BITS=64*N; p=c*(1<<f)-1; nbits=p.bit_length(); tsh=f%64
assert BITS-nbits==1, "this generator is headroom=1 only (lvl6)"
ptop=c<<tsh; ptop_off=8*(N-1)
ACC=N+2
L=[]; e=L.append
def off(j): return '' if j==0 else '+%d'%(8*j)

e('#include <sqisign_namespace.h>')
e('.intel_syntax noprefix'); e('')
e('#ifdef __APPLE__'); e('.section __TEXT,__const'); e('#else'); e('.section .rodata'); e('#endif')
quads=['0x%016X'%0]*(N-1)+['0x%016X'%ptop]
e('p_plus_1: .quad '+', '.join(quads))
e('#if defined(__linux__) && defined(__ELF__)'); e('.section .note.GNU-stack,"",@progbits'); e('#endif')
e('#include <asm_preamble.h>'); e(''); e('.text'); e('.p2align 4,,15'); e('')

# ---------------- fp_add / fp_sub (headroom=1) ----------------
def red_sub_q(load_from):
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
e('  sbb    rax, rax')
e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='add' if j==0 else 'adc'
    e('  mov    r8, [rdi%s]'%off(j)); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%off(j))
e('  mov    rax, [rdi+%d]'%(8*(N-1))); e('  sar    rax, 63')
e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
for j in range(N):
    src='rdx' if j==N-1 else 'rax'; opc='add' if j==0 else 'adc'
    e('  mov    r8, [rdi%s]'%off(j)); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%off(j))
e('  ret'); e('')

# ---------------- shared mac / reduce / final ----------------
def emit_zero_t():
    e('  xor    eax, eax')
    for j in range(ACC): e('  mov    [rsp%s], rax'%off(j))

def emit_mac(br, bo):   # t[0..N-1] += (mem[br+bo + 8j]) * rdx ; flush -> t[N],t[N+1]
    def M(j):
        o=bo+8*j; return '[%s]'%br if o==0 else '[%s+%d]'%(br,o)
    e('  xor    r12d, r12d')
    e('  mulx   r9, r8, %s'%M(0)); e('  adcx   r8, [rsp]'); e('  mov    [rsp], r8')
    for j in range(1,N):
        lo,hi=('r8','r9') if j%2==0 else ('r10','r11'); prevhi='r9' if j%2==1 else 'r11'
        e('  mulx   %s, %s, %s'%(hi,lo,M(j))); e('  adox   %s, %s'%(lo,prevhi))
        e('  adcx   %s, [rsp%s]'%(lo,off(j))); e('  mov    [rsp%s], %s'%(off(j),lo))
    lasthi='r9' if (N-1)%2==0 else 'r11'
    e('  mov    r13, [rsp+%d]'%(8*N)); e('  adox   %s, r12'%lasthi); e('  adcx   %s, r13'%lasthi); e('  mov    [rsp+%d], %s'%(8*N,lasthi))
    e('  mov    r13, [rsp+%d]'%(8*(N+1))); e('  adc    r13, 0'); e('  mov    [rsp+%d], r13'%(8*(N+1)))

def emit_reduce():
    e('  mov    rdx, [rsp]')
    e('  mulx   r9, r8, [rip+p_plus_1+%d]'%ptop_off)
    e('  mov    r13, [rsp+%d]'%(8*(N-1))); e('  add    r13, r8'); e('  mov    [rsp+%d], r13'%(8*(N-1)))
    e('  mov    r13, [rsp+%d]'%(8*N));     e('  adc    r13, r9'); e('  mov    [rsp+%d], r13'%(8*N))
    e('  mov    r13, [rsp+%d]'%(8*(N+1))); e('  adc    r13, 0');  e('  mov    [rsp+%d], r13'%(8*(N+1)))
    for j in range(N+1):
        e('  mov    r8, [rsp+%d]'%(8*(j+1))); e('  mov    [rsp%s], r8'%off(j))
    e('  xor    r8d, r8d'); e('  mov    [rsp+%d], r8'%(8*(N+1)))

def emit_final_to_rdi():
    e('  mov    rax, [rsp+%d]'%(8*(N-1))); e('  sar    rax, 63')
    e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
    for j in range(N):
        src='rdx' if j==N-1 else 'rax'; opc='sub' if j==0 else 'sbb'
        e('  mov    r8, [rsp%s]'%off(j)); e('  %s    r8, %s'%(opc,src)); e('  mov    [rdi%s], r8'%off(j))

def emit_final2_to_rdi():
    # fused fp2 result < 3p and can need an (N+1)th limb t[N] (limb set when >= 2^BITS).
    # 2 rounds of constant-time "subtract p (over N+1 limbs) if value >= p" -> result < p.
    for _r in range(2):
        e('  mov    rbx, [rip+p]'); e('  mov    r15, [rip+p+%d]'%ptop_off)
        e('  mov    r8, [rsp]'); e('  sub    r8, rbx'); e('  mov    [rsp], r8')
        for j in range(1,N-1):
            e('  mov    r8, [rsp%s]'%off(j)); e('  sbb    r8, rbx'); e('  mov    [rsp%s], r8'%off(j))
        e('  mov    r8, [rsp+%d]'%(8*(N-1))); e('  sbb    r8, r15'); e('  mov    [rsp+%d], r8'%(8*(N-1)))
        e('  mov    r8, [rsp+%d]'%(8*N)); e('  sbb    r8, 0'); e('  mov    [rsp+%d], r8'%(8*N))
        e('  sbb    rax, rax')                       # mask = -1 if value < p (borrowed)
        e('  and    r15, rax')                        # p_top & mask
        e('  mov    r8, [rsp]'); e('  add    r8, rax'); e('  mov    [rsp], r8')   # p_low & mask = rax
        for j in range(1,N-1):
            e('  mov    r8, [rsp%s]'%off(j)); e('  adc    r8, rax'); e('  mov    [rsp%s], r8'%off(j))
        e('  mov    r8, [rsp+%d]'%(8*(N-1))); e('  adc    r8, r15'); e('  mov    [rsp+%d], r8'%(8*(N-1)))
        e('  mov    r8, [rsp+%d]'%(8*N)); e('  adc    r8, 0'); e('  mov    [rsp+%d], r8'%(8*N))
    for j in range(N):
        e('  mov    r8, [rsp%s]'%off(j)); e('  mov    [rdi%s], r8'%off(j))

# ---------------- fp_mul / fp_sqr (single product) ----------------
e('.global fp_mul'); e('fp_mul:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  mov    rcx, rdx'); e('  sub    rsp, %d'%(8*ACC))
emit_zero_t()
e('  xor    r14d, r14d')
e('.Lmul_outer:')
e('  mov    rdx, [rcx + r14]')
emit_mac('rsi', 0)
emit_reduce()
e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .Lmul_outer')
emit_final_to_rdi()
e('  add    rsp, %d'%(8*ACC))
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')
e('.global fp_sqr'); e('fp_sqr:'); e('  mov    rdx, rsi'); e('  jmp    fp_mul'); e('')

# ---------------- FUSED fp2 kernels ----------------
# stack frame:  T(N+2) | A0N(N) | A1N(N) | BB(N)
A0N=8*ACC; A1N=A0N+8*N; BB=A1N+8*N; FRAME=BB+8*N
S=A0N; D=A1N  # sq reuses these slots
if FRAME % 16 != 0: FRAME += 8

def emit_norm(src_off, dst_off):   # [rsi+src_off] -> [0,p) -> [rsp+dst_off]
    e('  mov    rbx, [rip+p]'); e('  mov    r15, [rip+p+%d]'%ptop_off); e('  xor    eax, eax')
    e('  mov    r8, [rsi+%d]'%src_off); e('  sub    r8, rbx'); e('  mov    [rsp+%d], r8'%dst_off)
    for j in range(1,N-1):
        e('  mov    r8, [rsi+%d]'%(src_off+8*j)); e('  sbb    r8, rbx'); e('  mov    [rsp+%d], r8'%(dst_off+8*j))
    e('  mov    r8, [rsi+%d]'%(src_off+8*(N-1))); e('  sbb    r8, r15'); e('  mov    [rsp+%d], r8'%(dst_off+8*(N-1)))
    e('  sbb    rax, 0'); e('  and    r15, rax')
    e('  mov    r8, [rsp+%d]'%dst_off); e('  add    r8, rax'); e('  mov    [rsp+%d], r8'%dst_off)
    for j in range(1,N-1):
        e('  mov    r8, [rsp+%d]'%(dst_off+8*j)); e('  adc    r8, rax'); e('  mov    [rsp+%d], r8'%(dst_off+8*j))
    e('  mov    r8, [rsp+%d]'%(dst_off+8*(N-1))); e('  adc    r8, r15'); e('  mov    [rsp+%d], r8'%(dst_off+8*(N-1)))

# fp2_mul_c0 = a0*b0 - a1*b1 ;  fp2_mul_c1 = a0*b1 + a1*b0
for c0 in (True, False):
    nm='fp2_mul_c0' if c0 else 'fp2_mul_c1'
    e('.global %s'%nm); e('%s:'%nm)
    for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
    e('  mov    rcx, rdx')                 # b
    e('  sub    rsp, %d'%FRAME)
    emit_norm(0, A0N)                      # a0n
    emit_norm(8*N, A1N)                    # a1n
    if c0:
        e('  mov    r8, [rip+p2]'); e('  sub    r8, [rcx+%d]'%(8*N)); e('  mov    [rsp+%d], r8'%BB)
        for j in range(1,N):
            e('  mov    r8, [rip+p2+%d]'%(8*j)); e('  sbb    r8, [rcx+%d]'%(8*N+8*j)); e('  mov    [rsp+%d], r8'%(BB+8*j))
    emit_zero_t()
    e('  xor    r14d, r14d')
    e('.L%s_loop:'%nm)
    if c0:
        e('  mov    rdx, [rcx + r14]')                 # b0[i]
        emit_mac('rsp', A0N)
        e('  lea    rax, [rsp + r14]'); e('  mov    rdx, [rax + %d]'%BB)   # bb[i]
        emit_mac('rsp', A1N)
    else:
        e('  mov    rdx, [rcx + r14 + %d]'%(8*N))      # b1[i]
        emit_mac('rsp', A0N)
        e('  mov    rdx, [rcx + r14]')                 # b0[i]
        emit_mac('rsp', A1N)
    emit_reduce()
    e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .L%s_loop'%nm)
    emit_final2_to_rdi()
    e('  add    rsp, %d'%FRAME)
    for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
    e('  ret'); e('')

# fp2_sq_c0 = (a0+a1)*(a0-a1)
e('.global fp2_sq_c0'); e('fp2_sq_c0:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  sub    rsp, %d'%FRAME)
# s = a0 + a1  (raw)
e('  mov    r8, [rsi]'); e('  add    r8, [rsi+%d]'%(8*N)); e('  mov    [rsp+%d], r8'%S)
for j in range(1,N):
    e('  mov    r8, [rsi+%d]'%(8*j)); e('  adc    r8, [rsi+%d]'%(8*N+8*j)); e('  mov    [rsp+%d], r8'%(S+8*j))
# d = a0 - a1  (2-round correct)
e('  mov    r8, [rsi]'); e('  sub    r8, [rsi+%d]'%(8*N)); e('  mov    [rsp+%d], r8'%D)
for j in range(1,N):
    e('  mov    r8, [rsi+%d]'%(8*j)); e('  sbb    r8, [rsi+%d]'%(8*N+8*j)); e('  mov    [rsp+%d], r8'%(D+8*j))
e('  sbb    rax, rax')
e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
e('  mov    r8, [rsp+%d]'%D); e('  add    r8, rax'); e('  mov    [rsp+%d], r8'%D)
for j in range(1,N-1):
    e('  mov    r8, [rsp+%d]'%(D+8*j)); e('  adc    r8, rax'); e('  mov    [rsp+%d], r8'%(D+8*j))
e('  mov    r8, [rsp+%d]'%(D+8*(N-1))); e('  adc    r8, rdx'); e('  mov    [rsp+%d], r8'%(D+8*(N-1)))
e('  mov    rax, [rsp+%d]'%(D+8*(N-1))); e('  sar    rax, 63')
e('  mov    rdx, [rip+p+%d]'%ptop_off); e('  and    rdx, rax')
e('  mov    r8, [rsp+%d]'%D); e('  add    r8, rax'); e('  mov    [rsp+%d], r8'%D)
for j in range(1,N-1):
    e('  mov    r8, [rsp+%d]'%(D+8*j)); e('  adc    r8, rax'); e('  mov    [rsp+%d], r8'%(D+8*j))
e('  mov    r8, [rsp+%d]'%(D+8*(N-1))); e('  adc    r8, rdx'); e('  mov    [rsp+%d], r8'%(D+8*(N-1)))
emit_zero_t()
e('  xor    r14d, r14d')
e('.Lsqc0_loop:')
e('  lea    rax, [rsp + r14]'); e('  mov    rdx, [rax + %d]'%S)   # s[i]
emit_mac('rsp', D)
emit_reduce()
e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .Lsqc0_loop')
emit_final2_to_rdi()
e('  add    rsp, %d'%FRAME)
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')

# fp2_sq_c1 = 2*a0*a1
e('.global fp2_sq_c1'); e('fp2_sq_c1:')
for r in ['rbx','rbp','r12','r13','r14','r15']: e('  push   %s'%r)
e('  sub    rsp, %d'%FRAME)
# twoa0 = a0 + a0  -> S
e('  mov    r8, [rsi]'); e('  add    r8, [rsi]'); e('  mov    [rsp+%d], r8'%S)
for j in range(1,N):
    e('  mov    r8, [rsi+%d]'%(8*j)); e('  adc    r8, [rsi+%d]'%(8*j)); e('  mov    [rsp+%d], r8'%(S+8*j))
emit_zero_t()
e('  xor    r14d, r14d')
e('.Lsqc1_loop:')
e('  lea    rax, [rsp + r14]'); e('  mov    rdx, [rax + %d]'%S)   # 2a0[i]
emit_mac('rsi', 8*N)                                              # multiplicand a1 (raw)
emit_reduce()
e('  add    r14, 8'); e('  cmp    r14, %d'%(8*N)); e('  jne    .Lsqc1_loop')
emit_final2_to_rdi()
e('  add    rsp, %d'%FRAME)
for r in ['r15','r14','r13','r12','rbp','rbx']: e('  pop    %s'%r)
e('  ret'); e('')

open('/tmp/gen/fp_asm16_fused.S','w').write('\n'.join(L)+'\n')
print('wrote fp_asm16_fused.S (%d lines), FRAME=%d bytes'%(len(L),FRAME))
