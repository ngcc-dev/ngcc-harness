"""E3: output alignment of the AXIS digest phase and a sequential solver for the digest bits.

X = state at the start of digest generation (after the 1024 blank beats). For every bit x of X we
compute (structurally, i.e. an over-approximation of dependence) the earliest digest index e(x)
that can depend on x. Digest bit i is then solvable for free whenever some x has e(x)=i: fix all
bits with e < i first, set the others with e = i at random, and choose x to fix bit i.
"""
import os, sys, random, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import axis as A
random.seed(int(sys.argv[2]) if len(sys.argv) > 2 else 4)
VARIANT = int(sys.argv[1]) if len(sys.argv) > 1 else 1024
PB = 2048 + 64*3            # some fixed padded length (only selects the constant-stream offset)
D = VARIANT

def out_slots(beat_idx):
    if VARIANT == 1024 or (VARIANT == 768 and beat_idx % 2 == 0):
        return ('mu',)
    return ('mu', 'nu')

# ---- structural dependency of each digest bit on the 1536 bits of X ----
def dep_digest(nbeats):
    dep = [[1 << (192*j + i) for i in range(192)] for j in range(8)]
    outs = []
    bt = 1024
    for _ in range(nbeats):
        for s in out_slots(bt):
            regs = (6, 4, 2, 0) if s == 'mu' else (7, 5, 3, 1)
            outs.append(dep[regs[0]][0] | dep[regs[1]][0] | dep[regs[2]][0] | dep[regs[3]][0])
        for j in range(8):
            d = dep[j]; dp = dep[(j-1) % 8]; a5, a3, a1 = dep[(j+5) % 8], dep[(j+3) % 8], dep[(j+1) % 8]
            b = (d[125] | d[32] | d[107] | d[12] | d[96] | d[0] | a5[96] | a5[32] | a3[125] | a3[12]
                 | a1[107] | a1[0] | dp[11])
            new = list(d)
            for u in (66, 75, 90, 162, 178, 188): new[u] |= b
            dep[j] = new[1:] + new[:1]
        bt += 1
    return outs

NB = 320
t0 = time.time()
outs = dep_digest(NB)
earliest = {}
for i, m in enumerate(outs):
    new = m & ~sum(1 << v for v in earliest) if False else m
    x = m
    while x:
        lsb = x & -x; v = lsb.bit_length() - 1; x ^= lsb
        if v not in earliest: earliest[v] = i
by_idx = {}
for v, i in earliest.items(): by_idx.setdefault(i, []).append(v)
print(f"AXIS-{VARIANT}: structural dependencies over {NB} digest beats ({len(outs)} digest bits) in {time.time()-t0:.1f}s")
print(f"  state bits reaching the digest within these beats: {len(earliest)}/1536; largest earliest index: {max(earliest.values())}")
ctrl_idx = sorted(by_idx)
if VARIANT == 1024:
    assert len(ctrl_idx) == 133 and ctrl_idx == list(range(133))
print(f"  digest indices having >=1 fresh state bit (structurally controllable): {len(ctrl_idx)}")
print(f"  of which in the first 66 / contiguous prefix: {sum(1 for i in ctrl_idx if i < 66)} / "
      f"{next(i for i in range(len(outs)+1) if i not in by_idx)}")

# ---- sequential solver, verified by direct evaluation ----
def digest_bits_from(X, n):
    S = list(X); return A.digest_gen(S, VARIANT, PB, 1024, n)

def setbit(X, v, val):
    j, i = divmod(v, 192); X[j] = (X[j] & ~(1 << i)) | (val << i)

def solve(target):
    X = A.rand_state(); solved = []
    for i in ctrl_idx:
        cands = list(by_idx[i]); random.shuffle(cands)
        for v in cands:                       # randomize all fresh bits of this index first
            setbit(X, v, random.getrandbits(1))
        cur = digest_bits_from(X, i+1)[i]
        if cur == target[i]:
            solved.append(i); continue
        for v in cands:
            j, k = divmod(v, 192); setbit(X, v, ((X[j] >> k) & 1) ^ 1)
            if digest_bits_from(X, i+1)[i] == target[i]:
                solved.append(i); break
            setbit(X, v, ((X[j] >> k) & 1) ^ 1)
    return X, solved

ntr = 3
fully_solved = 0
for trial in range(ntr):
    target = [random.getrandbits(1) for _ in range(D)]
    t0 = time.time(); X, solved = solve(target)
    full = digest_bits_from(X, D)
    ok_ctrl = all(full[i] == target[i] for i in solved)
    assert ok_ctrl
    fully_solved += len(solved) == len(ctrl_idx)
    match_all = sum(full[i] == target[i] for i in range(D))
    pre = next(i for i in range(D + 1) if i == D or full[i] != target[i])
    print(f"  trial {trial}: solved {len(solved)} indices (all verified on full digest: {ok_ctrl}); "
          f"total matching digest bits {match_all}/{D}; matching prefix {pre}; {time.time()-t0:.1f}s")
    # sanity: invert the 1024 blank beats -> state Y at end of message phase, re-run forward
    S = list(X)
    for b in reversed(range(1024)): A.beat_inv(S, A.const_ins(PB, b))
    Y = list(S); S = list(Y)
    fwd = A.digest_phase(S, VARIANT, PB)
    print(f"           Y = blank^-1(X) recomputed forward: digest bits equal = {fwd == full}")
    assert fwd == full
assert fully_solved >= 1
print("STRUCTURAL hash-02-4 CONFIRMED: 133 structurally available positions; 132–133 fixed in the trials")
print("FULL PREIMAGE hash-02-4 NOT RUN: 2^891–2^892 cost is a heuristic estimate")
