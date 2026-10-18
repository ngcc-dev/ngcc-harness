# Step 2 (Sage): for each hint h on E_A and E_{-A} with a valid basis of E[2^E], compute N with
# B_aux(honest) = B'_h * N (columns), and the candidate M' = Normalize(M * N^-1 mod 2^E).
# For E_{-A}, the honest basis is transported by the isomorphism (x,y) -> (-x, i*y).
import json, sys
d = json.load(open(sys.argv[1])); outp = sys.argv[2]
p = Integer(d["p"]); E = int(d["E"]); mod = 2**E
Rt.<t> = GF(p)[]
F.<i> = GF(p^2, modulus=t^2+1)
FPB = (p.nbits() + 7) // 8
sig = bytes.fromhex(d["sig"]); RB = (E + 7) // 8; OFF_M = 2 * FPB
M = [int.from_bytes(sig[OFF_M + k*RB: OFF_M + (k+1)*RB], "little") for k in range(4)]  # a, c, b, d
Mm = matrix(Zmod(mod), [[M[0], M[1]], [M[2], M[3]]])
fe = lambda v: F(v[0]) + F(v[1]) * i
def lift_basis(Ec, pts):
    xs = []
    for k in range(3):
        X, Z = fe(pts[2*k]), fe(pts[2*k+1])
        if Z == 0: return None
        xs.append(X / Z)
    try:
        P = Ec.lift_x(xs[0]); Q = Ec.lift_x(xs[1])
    except Exception:
        return None
    if (P - Q).x() != xs[2]:
        Q = -Q
        if (P - Q).x() != xs[2]: return None
    return P, Q
def is_basis(P, Q):
    if (2**(E-1))*P == 0 or (2**(E-1))*Q == 0 or (2**E)*P != 0 or (2**E)*Q != 0: return False
    w = P.weil_pairing(Q, 2**E)
    return w**(2**(E-1)) != 1
def coords(T, P, Q, w):
    # T = alpha P + beta Q ; e(T,Q) = w^alpha, e(P,T) = w^beta
    al = discrete_log(T.weil_pairing(Q, 2**E), w, ord=2**E, operation='*')
    be = discrete_log(P.weil_pairing(T, 2**E), w, ord=2**E, operation='*')
    assert al*P + be*Q == T
    return al, be
def normalize(v):
    mid = 2**(E-1)
    for x in v:
        if x > mid: return [(-y) % mod for y in v]
        if 0 < x < mid: return v
    return v
A = fe(d["A"]); EA = EllipticCurve(F, [0, A, 0, 1, 0]); EnA = EllipticCurve(F, [0, -A, 0, 1, 0])
h0 = d["hint_aux"]
hon = lift_basis(EA, d["models"]["A"][str(h0)]["pts"]); assert hon and is_basis(*hon)
Ph, Qh = hon
res = {"set": d["set"], "hint_aux": h0, "cands": []}
for label, Ec, (P0, Q0) in (("A", EA, (Ph, Qh)), ("negA", EnA, (EnA(-Ph.x(), i*Ph.y()), EnA(-Qh.x(), i*Qh.y())))):
    if d["models"].get(label) is None: continue
    nvalid = 0
    for h in range(256):
        b = lift_basis(Ec, d["models"][label][str(h)]["pts"])
        if b is None or not is_basis(*b): continue
        nvalid += 1
        P1, Q1 = b; w = P1.weil_pairing(Q1, 2**E)
        a1, b1 = coords(P0, P1, Q1, w); a2, b2 = coords(Q0, P1, Q1, w)
        N = matrix(Zmod(mod), [[a1, a2], [b1, b2]])   # columns = coords of P0, Q0
        Mp = Mm * N.inverse()
        v = normalize([int(Mp[0,0]), int(Mp[0,1]), int(Mp[1,0]), int(Mp[1,1])])
        res["cands"].append({"model": label, "hint": int(h), "M": [int(z) for z in v]})
    res["valid_" + label] = int(nvalid)
json.dump(res, open(outp, "w"))
print(d["set"], "valid hints: A", res.get("valid_A"), "negA", res.get("valid_negA"), "honest", h0)
