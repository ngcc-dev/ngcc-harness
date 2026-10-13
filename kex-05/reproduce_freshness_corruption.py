#!/usr/bin/env python3
"""Certify the no-matching-session Corrupt omission in Loom Definition 4.12.

This checks the frozen text and the resulting game predicate. It is not a
network exploit or a test of honest uncorrupted peers.
"""

from pathlib import Path
import subprocess


SPEC = Path(__file__).resolve().parent / "kex-05-spec.pdf"


def main() -> None:
    spec = subprocess.run(
        ["pdftotext", "-layout", str(SPEC), "-"],
        check=True, capture_output=True, text=True,
    ).stdout
    definition = spec.split("Definition 4.12 (Freshness).", 1)[1].split(
        "Forward Secrecy.", 1
    )[0]
    assert "If a matching session" in definition
    assert "no Corrupt has been issued to party Q" in definition
    assert definition.index("If a matching session") < definition.index(
        "no Corrupt has been issued to party Q"
    )
    assert "no Corrupt has been issued to party P" not in definition

    # P completes with a peer identity Q. The attacker has Q's signing key,
    # runs Q's side itself, and creates no honest matching session at Q.
    owner_revealed = False
    matching_session_exists = False
    peer_corrupted = True
    owner_clause = not owner_revealed
    matching_clause = not matching_session_exists or not peer_corrupted
    assert owner_clause and matching_clause
    print("CONFIRMED kex-05-6: Corrupt(Q) plus an impersonated no-match session remains fresh under Definition 4.12")


if __name__ == "__main__":
    main()
