#!/usr/bin/env python3
"""kem-09-4 CheetahKEM: the implicit-rejection key hashes the re-encryption.

Algorithm 20 specifies Kbar = H(salt || c) over the received ciphertext c.
KEM_Cheetah.c instead hashes salt || c', where c' = Encrypt(pk, m', gamma') is
the re-encryption of the decrypted message m'.  Distinct invalid ciphertexts
that decrypt to the same message therefore receive the same rejection output.

Black-box test against the prebuilt kem-09/lib/*.so through ctypes:
  * honest control: Decaps(sk, c*) == K*;
  * single bit-0 flips c_j = c* xor (0x01 at byte j) at spread positions; a
    spec-conformant decapsulator returns H(salt || c_j), distinct for every j,
    whereas here several distinct c_j return one identical value != K*;
  * second-encapsulation control: bit-0 flips of an independent ciphertext
    c_b also collide, but on a different value (the value depends on Dec(c));
  * heavy-mutation control: rewriting the high nibbles of the last 64 bytes
    (Dec changes) does not return the challenge collision value.
Bit 0 is used because the OR of the byte differences between c_j and c* is
then exactly 0x01, whose negation is the all-ones mask, so the output is the
whole rejection key rather than a bitwise mixture with K*.

When Python's hashlib provides SM3, the script also recomputes the rejection
hash itself (pseudoXOF, the SM3 counter-mode XOF the code uses, keyed with the
salt stored as the last SHARED_KEY_BYTES+16 bytes of sk) and checks that the
collision value is H(salt || c*) and that no output is the specified
H(salt || c_j).  Without SM3 that cross-check is reported as skipped.
"""
import ctypes
import hashlib
import sys
import time
from collections import Counter
from pathlib import Path

ID = "kem-09-4"
ROOT = Path(__file__).resolve().parents[1]
LIBS = ["kem-09/lib/libCheetah128.so", "kem-09/lib/libCheetah256.so",
        "kem-09/lib/libCheetah384.so", "kem-09/lib/libCheetah512.so"]
NPOS = 16          # bit-0 flip positions per ciphertext
MIN_CLUSTER = 4    # distinct invalid ciphertexts required to share one output
try:
    hashlib.new("sm3")
    HAVE_SM3 = True
except ValueError:
    HAVE_SM3 = False

U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong


def buf(data=b"", size=None):
    n = len(data) if size is None else size
    b = (U8 * (n + 64))()
    ctypes.memmove(b, bytes(data), len(data))
    return b


class Kem:
    def __init__(self, path, seed):
        self.lib = lib = ctypes.CDLL(str(path))
        for f in ("pk", "sk", "ct", "ss"):
            getattr(lib, f"kem_get_{f}_len_bytes").restype = ULL
        self.n = {f: int(getattr(lib, f"kem_get_{f}_len_bytes")()) for f in ("pk", "sk", "ct", "ss")}
        lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
        d = hashlib.sha512(seed).digest()
        lib.ngcc_seed(buf(d), len(d))

    def keygen(self):
        pk, sk = buf(size=self.n["pk"]), buf(size=self.n["sk"])
        pl, sl = ULL(self.n["pk"]), ULL(self.n["sk"])
        rc = self.lib.kem_keygen(pk, ctypes.byref(pl), sk, ctypes.byref(sl))
        if rc != 0:
            raise RuntimeError(f"kem_keygen rc={rc}")
        return bytes(pk)[:pl.value], bytes(sk)[:sl.value]

    def enc(self, pk):
        ct, ss = buf(size=self.n["ct"]), buf(size=self.n["ss"])
        cl, sl = ULL(self.n["ct"]), ULL(self.n["ss"])
        rc = self.lib.kem_enc(buf(pk), ULL(len(pk)), ss, ctypes.byref(sl), ct, ctypes.byref(cl))
        if rc != 0:
            raise RuntimeError(f"kem_enc rc={rc}")
        return bytes(ct)[:cl.value], bytes(ss)[:sl.value]

    def dec(self, sk, ct):
        ss, sl = buf(size=self.n["ss"]), ULL(self.n["ss"])
        rc = self.lib.kem_dec(buf(sk), ULL(len(sk)), buf(ct), ULL(len(ct)), ss, ctypes.byref(sl))
        return rc, bytes(ss)[:sl.value]


def xof(nbytes, msg):
    """pseudoXOF for byte-aligned input: SM3(msg || be32(i)), i = 1, 2, ..."""
    out = b""
    i = 1
    while len(out) < nbytes:
        out += hashlib.new("sm3", msg + i.to_bytes(4, "big")).digest()
        i += 1
    return out[:nbytes]


def flips(kem, sk, c):
    """Decapsulate NPOS distinct bit-0 flips of c; return [(c_j, output)]."""
    step = max(1, len(c) // NPOS)
    out = []
    for j in range(0, step * NPOS, step):
        cj = bytearray(c)
        cj[j] ^= 0x01
        out.append((bytes(cj), kem.dec(sk, bytes(cj))[1]))
    return out


def main():
    t0 = time.time()
    ok_all = True
    for rel in LIBS:
        path = ROOT / rel
        if not path.is_file():
            print(f"SKIP {ID}: {rel} missing (run make -C {rel.split('/')[0]})")
            return 77
        t = time.time()
        kem = Kem(path, b"ngcc " + ID.encode() + b" " + rel.encode())
        pk, sk = kem.keygen()
        c, k = kem.enc(pk)
        honest = kem.dec(sk, c)[1] == k

        res = flips(kem, sk, c)
        outs = [o for _, o in res]
        val, size = Counter(outs).most_common(1)[0]
        not_key = val != k and k not in outs

        cb, kb = kem.enc(pk)
        outs_b = [o for _, o in flips(kem, sk, cb)]
        val_b, size_b = Counter(outs_b).most_common(1)[0]
        ctrl_b = size_b >= MIN_CLUSTER and val_b != val and val_b != kb

        c3 = bytearray(c)
        for j in range(len(c) - 64, len(c)):
            c3[j] ^= 0xF0
        o3 = kem.dec(sk, bytes(c3))[1]
        ctrl_h = o3 != val and o3 != k

        if HAVE_SM3:
            nss = len(k)
            salt = sk[-(nss + 16):]
            reenc = val == xof(nss, salt + c) and val_b == xof(nss, salt + cb)
            spec_hits = sum(o == xof(nss, salt + cj) for cj, o in res)
            hashed = f"value_is_H(salt||c*)={val == xof(nss, salt + c)} " \
                     f"control_value_is_H(salt||c_b)={val_b == xof(nss, salt + cb)} " \
                     f"outputs_equal_to_spec_H(salt||c_j)={spec_hits}/{len(res)}"
            hash_ok = reenc and spec_hits == 0
        else:
            hashed, hash_ok = "H(salt||c) cross-check skipped (hashlib has no SM3)", True

        ok = honest and size >= MIN_CLUSTER and not_key and ctrl_b and ctrl_h and hash_ok
        ok_all &= ok
        print(f"{Path(rel).name:18s} ct={len(c)}B  honest_dec_matches={honest}  "
              f"bit0_flips={len(outs)}  largest_identical_output={size}/{len(outs)}  "
              f"value_equals_K*={val == k}")
        print(f"{'':18s} control second_encaps: identical={size_b}/{len(outs_b)} "
              f"differs_from_challenge_value={val_b != val} differs_from_K_b={val_b != kb}  "
              f"control heavy_mutation_differs={ctrl_h}")
        print(f"{'':18s} {hashed}  {'ok' if ok else 'FAIL'} {time.time() - t:.1f}s")
    if ok_all:
        print(f"CONFIRMED {ID}: distinct invalid ciphertexts decrypting to the challenge message "
              f"get one identical rejection key (all four sets, {time.time() - t0:.1f}s)")
        return 0
    print(f"NOT CONFIRMED {ID}: see per-set lines above")
    return 1


if __name__ == "__main__":
    sys.exit(main())
