#!/usr/bin/env python3
"""Step 1 of the retained SQIsign hint/-A alias derivation.
Signs one message with the library, then uses the library's exported ec_curve_init_from_A and
ec_curve_to_basis_2f_from_hint to decode, for every hint byte 0..255, the x-only basis (P,Q,P-Q) of
E[2^E] on E_aux (A) and on the isomorphic model E_{-A}. Writes JSON for the Sage step.
Usage: python3 -I hintdump.py <lib.so> <set> <tag> <out.json>"""
import ctypes, hashlib, sys, json
SETS = {"p324_3": (3, 324, 128, 196, 41), "p500_27": (27, 500, 192, 302, 64), "p664_17": (17, 664, 256, 400, 84)}
lib, name, tag, out = sys.argv[1:5]
c, f, lam, ersp, FPB = SETS[name]; p = c * 2**f - 1; E = ersp + 2; RB = (E + 7) // 8; CB = lam // 8
PKB, SIGB = 2 * FPB + 1, 2 * FPB + 4 * RB + CB + 2
L = ctypes.CDLL(lib); pre = f"sqisign_{name}_ref_"
kp, sg = getattr(L, pre + "crypto_sign_keypair"), getattr(L, pre + "crypto_sign_signature")
dec, enc = getattr(L, pre + "fp2_decode"), getattr(L, pre + "fp2_encode")
cinit, fromhint = getattr(L, pre + "ec_curve_init_from_A"), getattr(L, pre + "ec_curve_to_basis_2f_from_hint")
fromhint.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_uint8]
L.randombytes_init.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
# sizeof(fp2_t): count bytes written by fp2_decode into a patterned buffer (value with all limbs nonzero)
buf = ctypes.create_string_buffer(b"\xa5" * 2048, 2048)
dec(buf, (p - 12345).to_bytes(FPB, "little") * 2)
FP2 = max(i for i in range(2048) if buf.raw[i] != 0xa5) + 1
FP2 = (FP2 + 7) // 8 * 8
assert FP2 % 16 == 0, FP2
def fp2_bytes_at(base, off):
    o = ctypes.create_string_buffer(2 * FPB); enc(o, ctypes.c_void_p(ctypes.addressof(base) + off)); return o.raw
def toint(b): return [int.from_bytes(b[:FPB], "little"), int.from_bytes(b[FPB:], "little")]
L.randombytes_init(hashlib.sha384(tag.encode()).digest(), None, 256)
pk = ctypes.create_string_buffer(PKB); sk = ctypes.create_string_buffer(2000); assert kp(pk, sk) == 0
msg = ("hint probe " + tag).encode()
sb = ctypes.create_string_buffer(SIGB + 64); sl = ctypes.c_ulonglong(); assert sg(sb, ctypes.byref(sl), msg, len(msg), sk) == 0
sig = sb.raw[:SIGB]
A = toint(sig[:2 * FPB]); Aneg = [(-A[0]) % p, (-A[1]) % p]
res = {"set": name, "p": p, "E": E, "pk": pk.raw[:PKB].hex(), "sig": sig.hex(), "msg": msg.hex(), "A": A,
       "hint_aux": sig[2 * FPB + 4 * RB + CB], "sizeof_fp2": FP2, "models": {}}
for label, Av in (("A", A), ("negA", Aneg)):
    Ab = Av[0].to_bytes(FPB, "little") + Av[1].to_bytes(FPB, "little")
    fA = ctypes.create_string_buffer(FP2); dec(fA, Ab)
    curve = ctypes.create_string_buffer(4096)
    if cinit(curve, fA) == 0: res["models"][label] = None; continue
    bases = {}
    for h in range(256):
        B = ctypes.create_string_buffer(6 * FP2 + 64)
        r = fromhint(B, curve, E, h)
        pts = [toint(fp2_bytes_at(B, i * FP2)) for i in range(6)]  # Px Pz Qx Qz PmQx PmQz
        bases[h] = {"ret": r, "pts": pts}
    res["models"][label] = bases
json.dump(res, open(out, "w"))
print("sizeof(fp2_t) =", FP2, "honest hint_aux =", res["hint_aux"])
