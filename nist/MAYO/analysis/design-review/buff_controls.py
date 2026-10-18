#!/usr/bin/env python3
"""Verifier re-run of the retained BUFF witnesses (mayo_buff.py),
with negative controls. Usage: python3 -I buff_controls.py <repo> <mayo_X> [trials]"""
import sys, os, hashlib, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mayo_buff as B

root, name = sys.argv[1], sys.argv[2]
trials = int(sys.argv[3]) if len(sys.argv) > 3 else 5
P = B.load_params(root, name)
m, n, o, k = P['m'], P['n'], P['o'], P['k']
lib = B.Lib(os.path.join(root, 'MAYO/lib', name + '.so'))
c = dict(honest=0, ctrl_randomP3_rejects=0, ctrl_wrong_delta_rejects=0,
         NR_SCEO=0, ctrl_NR_otherMsg_rejects=0, SDEO=0, ctrl_SDEO_honestpk_rejects_M2=0,
         MBS=0, ctrl_MBS_M3_rejects=0, ctrl_MBS_honestpk_rejects=0, ctrl_MBS_noLambda_fails=0,
         seedpk_changed_NR_rejects=0)
for tr in range(trials):
    lib.seed(hashlib.sha384(b'verifier-buff-%d' % tr).digest())
    pk, sk = lib.keypair()
    M1, M2, M3 = b'v-one %d' % tr, b'v-two %d' % tr, b'v-three %d' % tr
    sig = lib.sign(M1, sk)
    sb = P['sig_bytes'] - P['salt_bytes']
    s = B.nib_decode(sig[:sb], k * n)
    blocks = [s[i * n:(i + 1) * n] for i in range(k)]
    salt = sig[sb:]
    c['honest'] += lib.verify(M1, sig, pk)
    # control 1: random perturbation of one P3 entry must break verification
    rnd = [b & 15 for b in os.urandom(m)]
    c['ctrl_randomP3_rejects'] += not lib.verify(M1, sig, B.apply_delta(P, pk, {(0, 0): rnd}))
    # NR / S-CEO witness
    W0, W1 = B.W_rc(P, blocks, 0, 0), B.W_rc(P, blocks, 1, 1)
    d0 = [0] * m; d0[0] = 1
    d1 = B.kdiv(P, W0, W1)
    pk2 = B.apply_delta(P, pk, {(0, 0): d0, (1, 1): d1})
    c['NR_SCEO'] += (pk2 != pk) and lib.verify(M1, sig, pk2)
    # control 2: perturb only (0,0) by d0 (no compensation) -> reject
    c['ctrl_wrong_delta_rejects'] += not lib.verify(M1, sig, B.apply_delta(P, pk, {(0, 0): d0}))
    c['ctrl_NR_otherMsg_rejects'] += not lib.verify(M2, sig, pk2)
    # control 3: same P3 kernel delta but different seed_pk -> reject (P1/P2 matter)
    pk2b = bytes([pk2[0] ^ 1]) + pk2[1:]
    c['seedpk_changed_NR_rejects'] += not lib.verify(M1, sig, pk2b)
    # S-DEO
    t1, L1 = B.t_lambda(P, M1, salt); t2, L2 = B.t_lambda(P, M2, salt)
    Ls1, Ls2 = B.L_eval(P, L1, blocks), B.L_eval(P, L2, blocks)
    T = [a ^ b ^ cc ^ d for a, b, cc, d in zip(t1, Ls1, t2, Ls2)]
    pk3 = B.apply_delta(P, pk, {(0, 0): B.kdiv(P, T, W0)})
    c['SDEO'] += lib.verify(M2, sig, pk3)
    c['ctrl_SDEO_honestpk_rejects_M2'] += not lib.verify(M2, sig, pk)
    # MBS (malicious key)
    salt4 = os.urandom(P['salt_bytes'])
    ta, La = B.t_lambda(P, M1, salt4); tb, Lb = B.t_lambda(P, M2, salt4)
    cols = []
    for i in range(k):
        for cc in range(o):
            blk = [[0] * n for _ in range(k)]; blk[i][n - o + cc] = 1
            va, vb = B.L_eval(P, La, blk), B.L_eval(P, Lb, blk)
            cols.append([x ^ y for x, y in zip(va, vb)])
    rows = [[cols[j][r] for j in range(k * o)] for r in range(m)]
    xs = B.solve(rows, [x ^ y for x, y in zip(ta, tb)], k * o)
    # Round-2 analogue: with Lambda absent the linear condition reads 0 = ta - tb (inconsistent)
    c['ctrl_MBS_noLambda_fails'] += (ta != tb)
    if xs is None:
        continue
    blk = [[0] * (n - o) + xs[i * o:(i + 1) * o] for i in range(k)]
    Tm = [x ^ y for x, y in zip(ta, B.L_eval(P, La, blk))]
    dd = B.kdiv(P, Tm, B.W_rc(P, blk, 0, 0))
    pk4 = B.apply_delta(P, os.urandom(P['pk_seed_bytes']) + bytes(P['P3_bytes']), {(0, 0): dd})
    sig4 = B.nib_encode(sum(blk, [])) + salt4
    c['MBS'] += lib.verify(M1, sig4, pk4) and lib.verify(M2, sig4, pk4)
    c['ctrl_MBS_M3_rejects'] += not lib.verify(M3, sig4, pk4)
    c['ctrl_MBS_honestpk_rejects'] += not lib.verify(M1, sig4, pk)
print(json.dumps({'param': name, 'trials': trials, 'counts': c}))
