#!/usr/bin/env python3
"""Show that QIMEN-PIKE ciphertexts with non-canonical field encodings decapsulate to the honest key."""

import argparse
import os
import pickle
import sys
import ctypes, hashlib

U8 = ctypes.c_ubyte
ULL = ctypes.c_ulonglong


def array(data, extra=0):
    buf = (U8 * (len(data) + extra))()
    ctypes.memmove(buf, data, len(data))
    return buf


class Kem:
    def __init__(self, path, seed):
        self.lib = lib = ctypes.CDLL(path)
        for field in ("pk", "sk", "ct", "ss"):
            getattr(lib, f"kem_get_{field}_len_bytes").restype = ULL
        self.sizes = {f: int(getattr(lib, f"kem_get_{f}_len_bytes")())
                      for f in ("pk", "sk", "ct", "ss")}
        lib.ngcc_seed.argtypes = [ctypes.POINTER(U8), ULL]
        digest = hashlib.sha512(seed).digest()
        assert lib.ngcc_seed(array(digest), len(digest)) == 0

    def keygen(self):
        s = self.sizes
        pk, sk = (U8 * s["pk"])(), (U8 * s["sk"])()
        pk_len, sk_len = ULL(s["pk"]), ULL(s["sk"])
        assert self.lib.kem_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len)) == 0
        return bytes(pk)[:pk_len.value], bytes(sk)[:sk_len.value]

    def enc(self, pk):
        s = self.sizes
        ct, ss = (U8 * s["ct"])(), (U8 * s["ss"])()
        ct_len, ss_len = ULL(s["ct"]), ULL(s["ss"])
        rc = self.lib.kem_enc(array(pk), len(pk), ss, ctypes.byref(ss_len), ct, ctypes.byref(ct_len))
        return rc, bytes(ct)[:ct_len.value], bytes(ss)[:ss_len.value]

    def dec(self, sk, ct):
        s = self.sizes
        out, out_len = (U8 * s["ss"])(), ULL(s["ss"])
        rc = self.lib.kem_dec(array(sk), len(sk), array(ct), len(ct), out, ctypes.byref(out_len))
        return rc, bytes(out)[:out_len.value]


SETS = {  # name: (p, value bytes, encoded bytes per Fp element)
    "NGCC-1": (0x588ee91ac23ef552f7bea02286b19c0bf3bb9a90588643dadd9997fbe0daea16974b919e94cbe0b3ffffffffffffffffffffffffffffffffffffffff, 60, 64),
    "NGCC-2": (0x1bfa60de9c0419b7bf54fba3dbb47692301e3eeaebbf1ce8a1434ee88e42e1e6140243d6c8bdb2744acf5820369b15b0e7bb8e9933551de530eb7aa16fc48c0fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff, 96, 96),
    "NGCC-3": (0x22606deb050ed1445554f51ce0e783f6a886397f71da19e514541bc051219e91bdfbff3ede30e07607a2821a8511c268f4c3a493432f085476f58123de8fab63ef81c420b2dfbb3fb3e2fa38f5419baf8cd6ed92b85bd0cb3a5035c534b5f3afdae93fda6645a281e3cd1244524530814e9c3615eab1df2a74f745f71f7b456bffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff, 192, 192),
}


def isolated_dec(kem, sk, ct):
    """Decapsulate in a forked child: malformed inputs may hit assertions."""
    r, w = os.pipe()
    pid = os.fork()
    if pid == 0:
        os.close(r)
        os.write(w, pickle.dumps(kem.dec(sk, ct)))
        os._exit(0)
    os.close(w)
    data = b""
    while chunk := os.read(r, 65536):
        data += chunk
    os.close(r)
    os.waitpid(pid, 0)
    return pickle.loads(data) if data else ("crash", None)


def check(name, p, nbytes, ebytes, trials):
    kem = Kem(f"kem-31/lib/lib{name}.so", b"QIMEN-PIKE ciphertext alias witness")
    pk, sk = kem.keygen()
    plus_p = [0, 0]; slot = [0, 0]; control = [0, 0]
    for _ in range(trials):
        rc, ct, key = kem.enc(pk)
        assert rc == 0 and isolated_dec(kem, sk, ct) == (0, key)
        for j in range(8):                          # the eight Fp values of the four Fp2 fields
            off = j * ebytes
            x = int.from_bytes(ct[off:off + nbytes], "little")
            if x + p < 1 << (8 * nbytes):
                c = bytearray(ct); c[off:off + nbytes] = (x + p).to_bytes(nbytes, "little")
                rc, k = isolated_dec(kem, sk, bytes(c))
                plus_p[1] += 1; plus_p[0] += rc == 0 and k == key
            if ebytes > nbytes:                     # bytes of the slot that are never read
                c = bytearray(ct); c[off + nbytes:off + ebytes] = b"\xa5" * (ebytes - nbytes)
                rc, k = isolated_dec(kem, sk, bytes(c))
                slot[1] += 1; slot[0] += rc == 0 and k == key
        c = bytearray(ct); c[-1] ^= 1              # control: changed payload bit
        rc, k = isolated_dec(kem, sk, bytes(c))
        control[1] += 1; control[0] += rc == 0 and k == key
    print(f"{name}: CONFIRMED: {plus_p[0]}/{plus_p[1]} x+p field re-encodings return the honest key"
          + (f"; {slot[0]}/{slot[1]} changed unread slot bytes return the honest key" if slot[1] else ""))
    print(f"{name}: CONTROL: {control[0]}/{control[1]} payload-bit changes return the honest key")
    assert plus_p[0] == plus_p[1] > 0 and slot[0] == slot[1] and control[0] == 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trials", type=int, default=2)
    args = parser.parse_args()
    for name, params in SETS.items():
        check(name, *params, args.trials)


if __name__ == "__main__":
    main()
