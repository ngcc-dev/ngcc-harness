#!/usr/bin/env python3
"""Generate fp_asm.S (broadwell, mulx/adcx/adox, "p+1" Montgomery) for SQIsign.
Parameterized by (N, c, f). Emits ONLY the core: p_plus_1, fp_add, fp_sub,
MULADD/FPMUL macros, fp_mul, fp_sqr.  (asm fp2_* omitted -> use generic C fp2.)

Validated structure against committed lvl1(N4)/lvl3(N6)/lvl5(N8); register
allocation reproduces lvl3/lvl5 exactly.  headroom = 64*N - bitlen(p).
"""
import sys

REGPOOL = ['r8','r9','r10','r11','r12','r13','r14','r15','rax','rbx','rbp']
CALLEE = ['rbx','rbp','r12','r13','r14','r15']   # System V AMD64 callee-saved

def pushes(used):
    cs = [r for r in ['r12','r13','r14','r15','rbx','rbp'] if r in used]
    return cs

def gen(N, c, f):
    BITS = 64*N
    p = c*(1<<f) - 1
    nbits = p.bit_length()
    headroom = BITS - nbits
    sh = 64 - headroom            # fp_add/fp_sub reduction shift
    tsh = f % 64
    ptop = c << tsh               # nonzero top limb of p_plus_1 = c*2^f in N limbs
    ptop_off = 8*(N-1)
    W = BITS                      # macro width tag
    L = []                        # output lines
    def e(s=''): L.append(s)

    # ---- header ----
    e('#include <sqisign_namespace.h>')
    e('.intel_syntax noprefix')
    e('')
    e('.set pbytes,32')
    e('.set plimbs,4')
    e('')
    e('#ifdef __APPLE__')
    e('.section __TEXT,__const')
    e('#else')
    e('.section .rodata')
    e('#endif')
    quads = ['0x%016X'%0]*(N-1) + ['0x%016X'%ptop]
    e('p_plus_1: .quad ' + ', '.join(quads))
    e('')
    e('#if defined(__linux__) && defined(__ELF__)')
    e('.section .note.GNU-stack,"",@progbits')
    e('#endif')
    e('')
    e('#include <asm_preamble.h>')
    e('')
    e('.text')
    e('.p2align 4,,15')
    e('')

    acc = REGPOOL[:N]             # fp_add/sub use N regs
    top = acc[-1]

    # ---- fp_add ----
    e('.global fp_add')
    e('fp_add:')
    for r in pushes(acc): e('  push   %s'%r)
    e('  xor    rax, rax')
    for i,r in enumerate(acc):
        e('  mov    %s, [rsi%s]'%(r, '' if i==0 else '+%d'%(8*i)))
    for i,r in enumerate(acc):
        op = 'add' if i==0 else 'adc'
        e('  %s    %s, [rdx%s]'%(op, r, '' if i==0 else '+%d'%(8*i)))
    for _round in range(2):
        e('  mov    rax, %s'%top)
        if headroom == 1:
            e('  sar    rax, %d'%sh)
        else:
            e('  shr    rax, %d'%sh)
            e('  neg    rax')
        e('  mov    rdx, [rip+p+%d]'%ptop_off)
        e('  and    rdx, rax')
        for i,r in enumerate(acc):
            op = 'sub' if i==0 else 'sbb'
            src = 'rdx' if i==N-1 else 'rax'
            e('  %s    %s, %s'%(op, r, src))
    for i,r in enumerate(acc):
        e('  mov    [rdi%s], %s'%('' if i==0 else '+%d'%(8*i), r))
    for r in reversed(pushes(acc)): e('  pop    %s'%r)
    e('  ret')
    e('')

    # ---- fp_sub ----
    e('.global fp_sub')
    e('fp_sub:')
    for r in pushes(acc): e('  push   %s'%r)
    e('  xor    rax, rax')
    for i,r in enumerate(acc):
        e('  mov    %s, [rsi%s]'%(r, '' if i==0 else '+%d'%(8*i)))
    for i,r in enumerate(acc):
        op = 'sub' if i==0 else 'sbb'
        e('  %s    %s, [rdx%s]'%(op, r, '' if i==0 else '+%d'%(8*i)))
    e('  sbb    rax, 0')
    # round 1: add (q & borrow-mask)
    e('  mov    rdx, [rip+p+%d]'%ptop_off)
    e('  and    rdx, rax')
    for i,r in enumerate(acc):
        op = 'add' if i==0 else 'adc'
        src = 'rdx' if i==N-1 else 'rax'
        e('  %s    %s, %s'%(op, r, src))
    # round 2: add (q & sar-mask)
    e('  mov    rax, %s'%top)
    e('  sar    rax, %d'%sh)
    e('  mov    rdx, [rip+p+%d]'%ptop_off)
    e('  and    rdx, rax')
    for i,r in enumerate(acc):
        op = 'add' if i==0 else 'adc'
        src = 'rdx' if i==N-1 else 'rax'
        e('  %s    %s, %s'%(op, r, src))
    for i,r in enumerate(acc):
        e('  mov    [rdi%s], %s'%('' if i==0 else '+%d'%(8*i), r))
    for r in reversed(pushes(acc)): e('  pop    %s'%r)
    e('  ret')
    e('')

    # ---- macros ----
    regs = REGPOOL[:N+1]          # N+1 accumulators for mul
    T0, T1 = REGPOOL[N+1], REGPOOL[N+2]
    Zs = ', '.join('Z%d'%i for i in range(N+1))
    # MULADD64xW : z += rdx * (M1[0..N-1]) into Z0..ZN
    e('.macro MULADD64x%d M1, %s, T0, T1, C' % (W, Zs))
    e('    xor    \\C, \\C')
    e('    mulx   \\T0, \\T1, \\M1')
    e('    adox   \\Z0, \\T1')
    e('    adox   \\Z1, \\T0')
    for k in range(1, N):
        e('    mulx   \\T0, \\T1, %d\\M1' % (8*k))
        e('    adcx   \\Z%d, \\T1' % k)
        e('    adox   \\Z%d, \\T0' % (k+1))
    e('    adc    \\Z%d, 0' % N)
    e('.endm')
    e('')
    # MULADD64x64 : z += rdx * M1 (single limb) into last two of N args
    Zs2 = ', '.join('Z%d'%i for i in range(N))
    e('.macro MULADD64x64 M1, %s, T0, T1' % Zs2)
    e('    xor    \\T0, \\T0')
    e('    mulx   \\T0, \\T1, \\M1')
    e('    adox   \\Z%d, \\T1' % (N-2))
    e('    adox   \\Z%d, \\T0' % (N-1))
    e('.endm')
    e('')
    # FPMULWxW : full Montgomery (reduce + remaining b_i muls), window rotation
    e('.macro FPMUL%dx%d M0, M1, %s, T0, T1' % (W, W, Zs))
    def Z(idx): return '\\Z%d' % (idx % (N+1))
    # reduce(0): window start=1 length=N
    e('    mov    rdx, \\Z0')
    win = ', '.join(Z(1+k) for k in range(N))
    e('    MULADD64x64 [rip+p_plus_1+%d], %s, \\T0, \\T1' % (ptop_off, win))
    for i in range(1, N):
        e('    mov    rdx, %d\\M0' % (8*i))
        win = ', '.join(Z(i+k) for k in range(N+1))
        e('    MULADD64x%d \\M1, %s, \\T0, \\T1, %s' % (W, win, Z(i-1)))
        e('    mov    rdx, %s' % Z(i))
        win = ', '.join(Z(i+1+k) for k in range(N))
        e('    MULADD64x64 [rip+p_plus_1+%d], %s, \\T0, \\T1' % (ptop_off, win))
    e('.endm')
    e('')

    # ---- fp_mul ----
    used = regs + [T0, T1]
    e('.global fp_mul')
    e('fp_mul:')
    for r in pushes(used): e('  push   %s'%r)
    e('  mov    rcx, rdx')
    e('  mov    rdx, [rcx]')
    # init z = a * b0   (operand scanning, single adcx chain)
    e('  mulx   %s, %s, [rsi]' % (regs[1], regs[0]))
    e('  xor    rax, rax')
    for j in range(1, N):
        # hi -> regs[j+1]; lo scratch consumed immediately by adcx into regs[j].
        # mirror lvl5: lo=regs[j+2] for j<=N-3, T1 for j==N-2, T0 for j==N-1.
        if j <= N-3:
            lo = regs[j+2]
        elif j == N-2:
            lo = T1
        else:  # j == N-1
            lo = T0
        e('  mulx   %s, %s, [rsi+%d]' % (regs[j+1], lo, 8*j))
        e('  adcx   %s, %s' % (regs[j], lo))
    e('  adc    %s, 0' % regs[N])
    e('  FPMUL%dx%d [rcx], [rsi], %s, %s, %s' % (W, W, ', '.join(regs), T0, T1))
    # store: limb0=regs[N], limb j=regs[j-1]
    order = [regs[N]] + [regs[j-1] for j in range(1, N)]
    for i,r in enumerate(order):
        e('  mov    [rdi%s], %s'%('' if i==0 else '+%d'%(8*i), r))
    for r in reversed(pushes(used)): e('  pop    %s'%r)
    e('  ret')
    e('')
    e('.global fp_sqr')
    e('fp_sqr:')
    e('    mov rdx, rsi')
    e('    jmp fp_mul')
    e('')

    # ===== GF(p^2) kernels (a=[a1,a0] at [rsi], a1 at [rsi+8N]) =====
    a1off = 8*N
    fp2used = regs + [T0, T1]
    def Zf(idx): return regs[idx % (N+1)]
    def init_prod(mult, base, boff):
        def m(j):
            o = boff + 8*j
            return '[%s]'%base if o==0 else '[%s+%d]'%(base, o)
        e('  mov    rdx, %s'%mult)
        e('  mulx   %s, %s, %s'%(regs[1], regs[0], m(0)))
        e('  xor    rax, rax')
        for j in range(1, N):
            lo = regs[j+2] if j <= N-3 else (T1 if j == N-2 else T0)
            e('  mulx   %s, %s, %s'%(regs[j+1], lo, m(j)))
            e('  adcx   %s, %s'%(regs[j], lo))
        e('  adc    %s, 0'%regs[N])
    def full(Maddr, ws, C):
        win = ', '.join(Zf(ws+k) for k in range(N+1))
        e('  MULADD64x%d %s, %s, %s, %s, %s'%(W, Maddr, win, T0, T1, C))
    def red(ws):
        win = ', '.join(Zf(ws+k) for k in range(N))
        e('  MULADD64x64 [rip+p_plus_1+%d], %s, %s, %s'%(ptop_off, win, T0, T1))
    def store_rot():
        order = [regs[N]] + [regs[j-1] for j in range(1, N)]
        for i, r in enumerate(order):
            e('  mov    [rdi%s], %s'%('' if i == 0 else '+%d'%(8*i), r))
    def prologue(name):
        e('.global %s'%name); e('%s:'%name)
        for r in pushes(fp2used): e('  push   %s'%r)
    def epilogue():
        for r in reversed(pushes(fp2used)): e('  pop    %s'%r)
        e('  ret'); e('')

    for c0 in (True, False):
        prologue('fp2_mul_c0' if c0 else 'fp2_mul_c1')
        e('  mov    rcx, rdx')
        if c0:
            e('  mov    %s, [rip+p2]'%regs[0])
            e('  mov    %s, [rip+p2+8]'%regs[1])
            for i in range(2, N-1): e('  mov    %s, %s'%(regs[i], regs[1]))
            e('  mov    %s, [rip+p2+%d]'%(regs[N-1], 8*(N-1)))
            for j in range(N):
                e('  mov    rax, [rcx+%d]'%(a1off+8*j))
                e('  %s    %s, rax'%('sub' if j == 0 else 'sbb', regs[j]))
            for j in range(N):
                e('  mov    [rdi%s], %s'%('' if j == 0 else '+%d'%(8*j), regs[j]))
            M0a = lambda i: '[rcx]' if i == 0 else '[rcx+%d]'%(8*i)
            M1a = lambda i: '[rdi]' if i == 0 else '[rdi+%d]'%(8*i)
        else:
            M0a = lambda i: '[rcx+%d]'%(a1off+8*i)
            M1a = lambda i: '[rcx]' if i == 0 else '[rcx+%d]'%(8*i)
        init_prod(M0a(0), 'rsi', 0)
        e('  mov    rdx, %s'%M1a(0)); full('[rsi+%d]'%a1off, 0, T0)
        e('  mov    rdx, %s'%regs[0]); red(1)
        for i in range(1, N):
            e('  mov    rdx, %s'%M0a(i)); full('[rsi]', i, Zf(i-1))
            e('  mov    rdx, %s'%M1a(i)); full('[rsi+%d]'%a1off, i, T0)
            e('  mov    rdx, %s'%Zf(i)); red(i+1)
        store_rot(); epilogue()

    prologue('fp2_sq_c0')
    e('  mov    rdx, [rsi]')
    for i in range(1, N): e('  mov    %s, [rsi+%d]'%(regs[i], 8*i))
    e('  add    rdx, [rsi+%d]'%a1off)
    for i in range(1, N): e('  adc    %s, [rsi+%d]'%(regs[i], a1off+8*i))
    e('  mov    [rdi], rdx')
    for i in range(1, N): e('  mov    [rdi+%d], %s'%(8*i, regs[i]))
    e('  mov    %s, [rsi]'%regs[0])
    for i in range(1, N): e('  mov    %s, [rsi+%d]'%(regs[i], 8*i))
    e('  sub    %s, [rsi+%d]'%(regs[0], a1off))
    for i in range(1, N): e('  sbb    %s, [rsi+%d]'%(regs[i], a1off+8*i))
    e('  mov    rax, [rip+p2]'); e('  add    %s, rax'%regs[0])
    e('  mov    rax, [rip+p2+8]')
    for i in range(1, N-1): e('  adc    %s, rax'%regs[i])
    e('  adc    %s, [rip+p2+%d]'%(regs[N-1], 8*(N-1)))
    for i in range(N): e('  mov    [rdi+%d], %s'%(a1off+8*i, regs[i]))
    init_prod('[rdi]', 'rdi', a1off)
    e('  FPMUL%dx%d [rdi], [rdi+%d], %s, %s, %s'%(W, W, a1off, ', '.join(regs), T0, T1))
    store_rot(); epilogue()

    prologue('fp2_sq_c1')
    e('  mov    rdx, [rsi]')
    for i in range(1, N): e('  mov    %s, [rsi+%d]'%(regs[i], 8*i))
    e('  add    rdx, rdx')
    for i in range(1, N): e('  adc    %s, %s'%(regs[i], regs[i]))
    e('  sub    rsp, %d'%(8*N))
    for i in range(1, N): e('  mov    [rsp+%d], %s'%(8*i, regs[i]))
    e('  mulx   %s, %s, [rsi+%d]'%(regs[1], regs[0], a1off))
    e('  xor    rax, rax')
    for j in range(1, N):
        lo = regs[j+2] if j <= N-3 else (T1 if j == N-2 else T0)
        e('  mulx   %s, %s, [rsi+%d]'%(regs[j+1], lo, a1off+8*j))
        e('  adcx   %s, %s'%(regs[j], lo))
    e('  adc    %s, 0'%regs[N])
    e('  FPMUL%dx%d [rsp], [rsi+%d], %s, %s, %s'%(W, W, a1off, ', '.join(regs), T0, T1))
    e('  add    rsp, %d'%(8*N))
    store_rot(); epilogue()

    return '\n'.join(L) + '\n'

if __name__ == '__main__':
    N=int(sys.argv[1]); c=int(sys.argv[2]); f=int(sys.argv[3])
    sys.stdout.write(gen(N,c,f))
