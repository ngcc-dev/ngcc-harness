#!/usr/bin/env python3
"""Reproduce CreTAKE's signature- and second-key-binding defects."""

from __future__ import annotations

import ctypes
import hashlib
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations/Reference_Implementation"


def u8(data: bytes):
    return (ctypes.c_ubyte * len(data)).from_buffer_copy(data)


def check_signature_binding() -> None:
    sources = sorted(REF.glob("CreTAKE*/CreTAKE-*/KEX_AlgorithmInstance.c"))
    signed = [p for p in sources if "sig_sign" in p.read_text(errors="replace")]
    assert len(signed) == 18
    for path in signed:
        text = path.read_text(errors="replace")
        start = text.index("static void derive_session_key")
        end = text.index("\n}", start)
        kdf = text[start:end]
        assert not re.search(r"\bsig(?:nature)?\b", kdf, re.I), path

    # The submitted BiT backend is randomized: the same exposed signing key
    # produces distinct, valid encodings of the same authenticated string.
    so = ROOT / "lib/libCreTAKE-K2S-PLAC128-BiT128.so"
    if not so.exists():
        raise SystemExit(f"missing {so}; run: make -C {ROOT} lib/libCreTAKE-K2S-PLAC128-BiT128.so")
    lib = ctypes.CDLL(str(so), mode=ctypes.RTLD_LOCAL)
    lib.kex_get_pk_len_bytes.restype = ctypes.c_ulonglong
    lib.kex_get_sk_len_bytes.restype = ctypes.c_ulonglong
    pk_n, sk_n = lib.kex_get_pk_len_bytes(), lib.kex_get_sk_len_bytes()

    assert lib.ngcc_seed(u8(hashlib.sha512(b"CreTAKE key").digest()), 64) == 0
    pk, sk = (ctypes.c_ubyte * pk_n)(), (ctypes.c_ubyte * sk_n)()
    pk_len, sk_len = ctypes.c_ulonglong(), ctypes.c_ulonglong()
    dummy, dummy_len = (ctypes.c_ubyte * 1)(), ctypes.c_ulonglong()
    assert lib.kex_init_b(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len),
                          dummy, ctypes.byref(dummy_len)) == 0

    message_bytes = b"CreTAKE transcript-binding witness"
    message = u8(message_bytes)
    signatures: list[bytes] = []
    for label in (b"signature one", b"signature two"):
        assert lib.ngcc_seed(u8(hashlib.sha512(label).digest()), 64) == 0
        sig, sig_len = (ctypes.c_ubyte * 2048)(), ctypes.c_ulonglong()
        assert lib.sig_sign(sk, sk_len.value, message, len(message_bytes),
                            sig, ctypes.byref(sig_len)) == 0
        assert lib.sig_verify(pk, pk_len.value, sig, sig_len.value,
                              message, len(message_bytes)) == 0
        signatures.append(bytes(sig[:sig_len.value]))
    assert signatures[0] != signatures[1]

    # Figures 1, 2 and 4 define the complete wire messages as including these
    # signature bytes.  The two encodings therefore name non-matching
    # transcripts, while all 18 KDFs above receive identical inputs.
    transcript_1 = message_bytes + signatures[0]
    transcript_2 = message_bytes + signatures[1]
    assert transcript_1 != transcript_2
    common_secret = b"same hidden KEM contribution"
    key_1 = hashlib.sha256(message_bytes + common_secret).digest()
    key_2 = hashlib.sha256(message_bytes + common_secret).digest()
    assert key_1 == key_2
    print("CONFIRMED kex-03-2: 18 signature-bearing instances omit the signature from the KDF")
    print("CONTROL: two distinct submitted-BiT signatures verify on the same authenticated string")


def check_double_key_binding() -> None:
    generic = [
        "CreTAKE128/CreTAKE-K2K-PLAC128/twokem.c",
        "CreTAKE128/CreTAKE-K2K-ZEN128/twokem.c",
        "CreTAKE256/CreTAKE-K2K-PLAC256/twokem.c",
        "CreTAKE256/CreTAKE-K2K-ZEN256/twokem.c",
        "CreTAKE512/CreTAKE-K2K-ZEN512/twokem.c",
    ]
    for rel in generic:
        text = (REF / rel).read_text(errors="replace")
        # Both encapsulation and decapsulation hash only pk1, the first KEM
        # key, and the PKE plaintext.  Neither pk2 nor either ciphertext is in
        # the final-key buffer.
        regions = re.findall(r"memcpy\(buf3,[\s\S]*?pseudoXOF\([^;]+;", text)
        assert len(regions) == 2, rel
        for region in regions:
            assert "pk2" not in region and "c1" not in region and "c2" not in region, rel

    # Exact algebraic model of the Definition 8 attack.  Correctness of the
    # second PKE is all that is used: decrypt c2* under a leaked sk2*, then
    # re-encrypt the same m under another leaked pk2'.  The CCA oracle accepts
    # that different public key and returns the unchanged final key.
    def h(*parts: bytes) -> bytes:
        return hashlib.sha256(b"".join(parts)).digest()

    pk1, k1, message = h(b"pk1"), h(b"k1"), h(b"message")
    sk2_star, sk2_prime = h(b"sk2-star"), h(b"sk2-prime")
    pk2_star, pk2_prime = h(sk2_star), h(sk2_prime)

    def encrypt(pk: bytes, msg: bytes) -> bytes:
        mask = hashlib.shake_256(pk).digest(len(msg))
        return bytes(a ^ b for a, b in zip(msg, mask))

    def decrypt(sk: bytes, ct: bytes) -> bytes:
        return encrypt(h(sk), ct)

    c1_star = h(b"challenge c1")
    c2_star = encrypt(pk2_star, message)
    challenge_key = h(pk1, k1, message)
    recovered_message = decrypt(sk2_star, c2_star)
    assert recovered_message == message
    c2_prime = encrypt(pk2_prime, recovered_message)
    oracle_message = decrypt(sk2_prime, c2_prime)
    oracle_key = h(pk1, k1, oracle_message)
    assert (pk2_prime, c1_star + c2_prime) != (pk2_star, c1_star + c2_star)
    assert oracle_key == challenge_key
    assert h(pk1, k1, bytes([message[0] ^ 1]) + message[1:]) != challenge_key
    print("CONFIRMED kex-03-3: the permitted cross-key CCA query returns the challenge key")
    print("CONTROL: changing the recovered PKE plaintext changes the derived key")


if __name__ == "__main__":
    check_signature_binding()
    check_double_key_binding()
