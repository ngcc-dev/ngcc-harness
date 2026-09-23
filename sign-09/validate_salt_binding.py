#!/usr/bin/env python3
"""Static validation that DOVE signs a salt the verifier never reads."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent
IMPL = ROOT / "Implementations"


def compact(value: str) -> str:
    return re.sub(r"\s+", "", value)


def main() -> int:
    signers = sorted(IMPL.glob("**/DOVE_*_ref/SIG_AlgorithmInstance.c"))
    assert len(signers) == 4, len(signers)

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
        assert "memcpy(sig_ptr,salt,DOVE_SALT_BYTES);" in text, path
        verify = text.split("intsig_verify(", 1)[1]
        assert "DOVE_SALT_BYTES" not in verify, path

    print("CONFIRMED sign-09-1: 4/4 source variants append a salt but verify without reading it")
    print("LIMITATION: this does not establish an EUF-CMA break with a secure replacement hash")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
