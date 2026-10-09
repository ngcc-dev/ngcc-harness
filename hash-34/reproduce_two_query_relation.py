#!/usr/bin/env python3
"""Check WChain's short-message relation against its submitted C functions."""

import ctypes
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / (
    "WChain/Implementations and Test_Vectors/Implementations/Reference_Implementation/"
    "WChain-V1-512/wchain_c.c"
)
OTHER = ROOT / (
    "WChain/Implementations and Test_Vectors/Implementations/Reference_Implementation/"
    "WChain-V2-1024/wchain_c.c"
)
assert SOURCE.read_bytes() == OTHER.read_bytes(), "reference sources differ"


with tempfile.TemporaryDirectory(prefix="ngcc-wchain-relation-") as directory:
    library_path = Path(directory) / "libwchain.so"
    subprocess.run(
        ["cc", "-O2", "-fPIC", "-shared", "-DWCHAIN_NO_MAIN", "-DWCHAIN_DISABLE_SIMD",
         str(SOURCE), "-o", str(library_path)],
        check=True,
        capture_output=True,
    )
    library = ctypes.CDLL(str(library_path))

    for version, block_bytes, digest_bytes, state_words in (
        (1, 144, 64, 9),
        (2, 288, 128, 18),
    ):
        state_type = ctypes.c_uint64 * state_words
        block_type = ctypes.c_ubyte * block_bytes
        digest_type = ctypes.c_ubyte * digest_bytes
        compress = getattr(library, f"wchain_v{version}_compress_c")
        compress.argtypes = [state_type, state_type, block_type]
        compress.restype = None
        hash_bytes = getattr(library, f"wchain_v{version}_hash_c")
        hash_bytes.argtypes = [ctypes.c_void_p, ctypes.c_size_t, digest_type]
        hash_bytes.restype = None

        def digest(message: bytes) -> bytes:
            output = digest_type()
            hash_bytes(ctypes.create_string_buffer(message), len(message), output)
            return bytes(output)

        def cf(state: state_type, block: bytes) -> state_type:
            output = state_type()
            compress(output, state, block_type.from_buffer_copy(block))
            return output

        def published(state: state_type) -> bytes:
            return b"".join(word.to_bytes(8, "big") for word in state)[:digest_bytes]

        zero_state = state_type()
        zero_block = bytes(block_bytes)
        for message in (b"", b"abc", b"WChain short-message test"):
            padded = message + b"\x80" + bytes(block_bytes - len(message) - 1)
            first = cf(zero_state, padded)
            final = cf(first, padded)
            base_digest = digest(message)
            extended_digest = digest(padded + message)
            assert published(final) == base_digest
            assert published(cf(final, zero_block)) == extended_digest

            altered = state_type(*final)
            altered[-1] ^= 1
            assert published(cf(altered, zero_block)) != extended_digest
            changed = bytes([message[0] ^ 1]) + message[1:] if message else b"a"
            assert digest(padded + changed) != extended_digest

        print(f"RELATION hash-34-2 CONFIRMED: WChain-V{version}, "
              f"{8 * (state_words * 8 - digest_bytes)} hidden bits")

print("FULL ENUMERATION hash-34-2 NOT RUN")
