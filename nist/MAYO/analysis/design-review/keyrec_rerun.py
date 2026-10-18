#!/usr/bin/env python3
"""Misuse witness (N17): the same seed_sk used for MAYO1 and MAYO2 (both Category 1, both 24-byte
seeds, identical SHAKE256 input layouts, no parameter-set domain tag). If the same message is signed
under both keys with the same randomizer R (always the case in the spec's deterministic mode R=0),
the salts and the vinegar streams coincide, and because the oil coordinates x_i are published in
the clear (s_i = (v_i + O x_i, x_i)), each signature pair gives 73 F16-LINEAR equations in the
shared O keystream. Solving recovers O for BOTH keys.

Run with Sage's python: sage -python keyrec_rerun.py <repo-root> <pairs> <seed-label>
"""
import hashlib, os, sys, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mayo_buff import Lib, load_params, nib_decode
from sage.all import GF, PolynomialRing, matrix, vector

root = sys.argv[1]; npairs = int(sys.argv[2])
PA, PB = load_params(root, 'mayo_1'), load_params(root, 'mayo_2')
la = Lib(os.path.join(root, 'MAYO/lib/mayo_1.so')); lb = Lib(os.path.join(root, 'MAYO/lib/mayo_2.so'))
X = PolynomialRing(GF(2), 'X').gen()
F = GF(16, 'x', modulus=X**4 + X + 1)
fe = F.from_integer

seed0 = hashlib.sha384(sys.argv[3].encode()).digest()
la.seed(seed0); pka, ska = la.keypair()
lb.seed(seed0); pkb, skb = lb.keypair()
assert ska == skb
na, oa, nb, ob = PA['n'], PA['o'], PB['n'], PB['o']
va, vb = na - oa, nb - ob          # 80, 73
NU = vb * ob                        # 949 unknown keystream nibbles (O_B row-major); O_A = first va*oa of them
pairs = []
for j in range(npairs):
    msg = b'common message %d' % j
    sj = hashlib.sha384((sys.argv[3]+'-R-%d' % j).encode()).digest()
    la.seed(sj); siga = la.sign(msg, ska)
    lb.seed(sj); sigb = lb.sign(msg, skb)
    sa = nib_decode(siga[:PA['sig_bytes'] - PA['salt_bytes']], PA['k'] * na)
    sb = nib_decode(sigb[:PB['sig_bytes'] - PB['salt_bytes']], PB['k'] * nb)
    xa = sa[va:na]; xb = sb[vb:nb]            # oil coordinates of block 0, public
    new_rows, new_rhs = [], []
    for r in range(vb):                        # vinegar nibbles r < 73 coincide in block 0
        row = [F(0)] * NU
        for c in range(oa):
            row[oa * r + c] += fe(xa[c])
        for c in range(ob):
            row[ob * r + c] += fe(xb[c])
        new_rows.append(row); new_rhs.append(fe(sa[r]) + fe(sb[r]))
    pairs.append((new_rows, new_rhs))
# Attacker-side filtering: a signing retry (ctr>0) in either scheme breaks a pair's relation and
# is not visible. Pick random subsets of 15 pairs (1095 equations > 949 unknowns) until the
# overdetermined system is consistent, then solve.
import random
rnd = random.Random(sys.argv[3])
tries = 0
while True:
    tries += 1
    sub = rnd.sample(range(len(pairs)), 15)
    A = matrix(F, [r for i in sub for r in pairs[i][0]]); b = vector(F, [v for i in sub for v in pairs[i][1]])
    try:
        sol = A.solve_right(b)
    except ValueError:
        continue
    break
rows = list(A.rows()); used = len(sub)
rank = A.rank()
S = hashlib.shake_256(skb).digest(16 + PB['O_bytes'])
true = [fe(v) for v in nib_decode(S[16:], NU)]
print(json.dumps({'pairs_requested': npairs, 'pairs_used': used, 'subset_tries': tries, 'equations': len(rows),
                  'unknowns': NU, 'rank': int(rank),
                  'O_MAYO2_recovered': list(sol) == true if rank == NU else 'underdetermined',
                  'O_MAYO1_recovered': list(sol[:va * oa]) == true[:va * oa] if rank == NU else 'underdetermined'}))
