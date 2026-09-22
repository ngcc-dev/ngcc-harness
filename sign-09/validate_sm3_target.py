#!/usr/bin/env python3
# Finding: sign-09-1
"""Static/structural validation of the DOVE SM3 target-collision attack.

This does not attempt the generic 2^128 compression-function collision search.
It checks the exact source conditions that make such a collision extend through
every pseudoXOF output block and through verification.
"""

from pathlib import Path
import math
import re


ROOT = Path(__file__).resolve().parent
IMPL = ROOT / "Implementations"


def compact(value: str) -> str:
    return re.sub(r"\s+", "", value)


def main() -> int:
    signers = sorted(IMPL.glob("**/DOVE_*_ref/SIG_AlgorithmInstance.c"))
    auxfiles = sorted(IMPL.glob("**/DOVE_*_ref/auxfunc.c"))
    assert len(signers) == 4, len(signers)
    assert len(auxfiles) == 4, len(auxfiles)

    for path in signers:
        text = compact(path.read_text(errors="replace"))
        target = (
            "input_len=m_len_bytes+DOVE_PK_SEED_BYTES;"
            "uint8_t*input=malloc(input_len);"
            "memcpy(input,m,m_len_bytes);"
            "memcpy(input+m_len_bytes,seed_pk,DOVE_PK_SEED_BYTES);"
            "pseudoXOF(DOVE_M*8,input,input_len*8,t);"
        )
        # One occurrence in Sign and one in Verify.
        assert text.count(target) == 2, path
        verify = text.split("intsig_verify(", 1)[1]
        assert "DOVE_SALT_BYTES" not in verify, path

    for path in auxfiles:
        text = compact(path.read_text(errors="replace"))
        assert "unsignedintdigest[8];" in text, path
        assert "cascade_msg_ct[(msg_len_bits+7)/8]=ct>>24;" in text, path
        assert "sm3_bit(cascade_msg_ct,msg_len_bits+32,K+i*32);" in text, path

    # A 64-byte message is exactly one SM3 compression block.  If B and B'
    # collide after that block, seed_pk || BE32(counter) || SM3 padding is an
    # identical continuation, including its encoded total length.
    message_bytes = 64
    seed_bytes = 16
    counter_bytes = 4
    total_bytes = message_bytes + seed_bytes + counter_bytes
    assert total_bytes == 84

    for name, target_bytes, claimed_bits in (
        ("DOVE128", 44, 128),
        ("DOVE256", 96, 263),
        ("DOVE512", 216, 522),
    ):
        blocks = math.ceil(target_bytes / 32)
        affected = claimed_bits > 128
        print(
            f"{name}: target={target_bytes} bytes, SM3 calls={blocks}, "
            f"Table-5 minimum={claimed_bits} bits, "
            f"2^128 collision cap={'BREAK' if affected else 'not below claim'}"
        )

    print("source variants: 4/4 use msg || seed_pk and ignore signature salt")
    print("XOF variants: 4/4 use a 256-bit SM3 state and BE32 counters")
    print("result: DOVE_SM3_TARGET_EXTENSION_CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
