#!/usr/bin/env python3
"""Step 3: assemble candidate signatures (A or -A, hint_aux, M') and verify with the library.
Usage: python3 -I hintverify.py <lib.so> <dump.json> <cands.json>"""
import ctypes, json, sys
lib, dj, cj = sys.argv[1:4]
d = json.load(open(dj)); c = json.load(open(cj))
p, E, name = d["p"], d["E"], d["set"]; FPB = (p.bit_length() + 7) // 8; RB = (E + 7) // 8
sig = bytes.fromhex(d["sig"]); pk = bytes.fromhex(d["pk"]); msg = bytes.fromhex(d["msg"])
CB = len(sig) - 2 * FPB - 4 * RB - 2; OFF_HA = 2 * FPB + 4 * RB + CB
L = ctypes.CDLL(lib); vf = getattr(L, f"sqisign_{name}_ref_crypto_sign_verify")
V = lambda s, m: vf(s, len(s), m, len(m), pk) == 0
assert V(sig, msg) and not V(sig, msg + b"x")
A = d["A"]; seen = {sig}; stats = {}
for cd in c["cands"]:
    Av = A if cd["model"] == "A" else [(-A[0]) % p, (-A[1]) % p]
    b = bytearray(sig)
    b[:2 * FPB] = Av[0].to_bytes(FPB, "little") + Av[1].to_bytes(FPB, "little")
    for k, x in enumerate(cd["M"]): b[2 * FPB + k * RB: 2 * FPB + (k + 1) * RB] = x.to_bytes(RB, "little")
    b[OFF_HA] = cd["hint"]; s = bytes(b)
    ok = V(s, msg); oth = V(s, msg + b"x")
    st = stats.setdefault(cd["model"], {"valid_bases": 0, "accepted": 0, "accepted_other_msg": 0, "accepted_hints": []})
    st["valid_bases"] += 1
    if ok:
        st["accepted"] += 1; st["accepted_hints"].append(cd["hint"]); seen.add(s)
    st["accepted_other_msg"] += oth
# control: -A with unchanged M and hint
b = bytearray(sig); Av = [(-A[0]) % p, (-A[1]) % p]
b[:2 * FPB] = Av[0].to_bytes(FPB, "little") + Av[1].to_bytes(FPB, "little")
print(json.dumps({"set": name, "honest_hint": d["hint_aux"], "negA_unchanged_M_accepted": V(bytes(b), msg),
                  "distinct_new_encodings": len(seen) - 1,
                  **{k: {kk: (vv if kk != "accepted_hints" else len(vv)) for kk, vv in v.items()} for k, v in stats.items()},
                  "A_non_honest_hints_accepted": len([h for h in stats.get("A", {}).get("accepted_hints", []) if h != d["hint_aux"]])}))
