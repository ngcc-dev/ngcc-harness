"""Shared ctypes helpers for the NIIKE-lv128 witnesses."""
import ctypes

LIB = "kex-08/lib/libNIIKE-lv128.so"
FP2, TUPLES, TUPLE_ELEMS = 64, 13, 5      # pk = 13 tuples of (A24, Ps, Pt, Qs, Qt), each Fp2
U = ctypes.c_ulonglong


def load():
    lib = ctypes.CDLL(LIB)
    for f in ("kex_get_pk_len_bytes", "kex_get_sk_len_bytes", "kex_get_ss_len_bytes"):
        getattr(lib, f).restype = U
    return lib


def keygen(lib, seed):
    npk, nsk = lib.kex_get_pk_len_bytes(), lib.kex_get_sk_len_bytes()
    lib.ngcc_seed(seed, U(len(seed)))
    pk, sk, st = (ctypes.create_string_buffer(n + 64) for n in (npk, nsk, 64))
    a, b, c = U(0), U(0), U(0)
    assert lib.kex_init_a(pk, ctypes.byref(a), sk, ctypes.byref(b), st, ctypes.byref(c)) == 0
    return pk.raw[:npk], sk.raw[:nsk]


def derive(lib, sk, peer_pk):
    nss = lib.kex_get_ss_len_bytes()
    ss, ss_len = ctypes.create_string_buffer(nss + 64), U(nss)
    rc = lib.kex_derive_ss_a(ctypes.create_string_buffer(sk, len(sk) + 64), U(len(sk)),
                             ctypes.create_string_buffer(peer_pk, len(peer_pk) + 64), U(len(peer_pk)),
                             None, U(0), None, U(0), ss, ctypes.byref(ss_len))
    return rc, ss.raw[:ss_len.value]


def swap_pq(pk):
    """Swap the P and Q points inside every tuple: (A24,Ps,Pt,Qs,Qt) -> (A24,Qs,Qt,Ps,Pt)."""
    out = b""
    for k in range(TUPLES):
        t = pk[k * TUPLE_ELEMS * FP2:(k + 1) * TUPLE_ELEMS * FP2]
        e = [t[i * FP2:(i + 1) * FP2] for i in range(TUPLE_ELEMS)]
        out += e[0] + e[3] + e[4] + e[1] + e[2]
    return out


def conjugate(pk):
    """Public relabelling of a key: cycle reversed and P, Q swapped."""
    size = TUPLE_ELEMS * FP2
    tuples = [swap_pq(pk)[k * size:(k + 1) * size] for k in range(TUPLES)]
    return b"".join(tuples[(12 - k) % TUPLES] for k in range(TUPLES))
