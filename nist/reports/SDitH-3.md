# SDitH-3 — Informational: one signing-entropy repeat reveals the witness

Severity: Informational
Status: Confirmed
Layer: Design
Affected: All 12 Round-3 parameter sets when two signatures on different messages reuse the root seed and the salt bits retained by TweakSalt; runtime witness uses cat1_fast
Discovery: Non-trivial
Exploitation: End-to-end key reconstruction and fresh-message forgery under one repeated signing-RNG output
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

SDitH signing draws `salt` and `rseed` directly from fresh randomness, without
deriving or hedging them with the secret key and message. If two signatures
reuse the root seed and the salt bits that Algorithm 5's `TweakSalt` retains,
they use the same GGM trees and VOLE values. At category I, `TweakSalt` clears
24 low bits and overwrites one high domain bit, so equality of the other 103
salt bits is sufficient; a complete RNG-output repeat is a simple way to meet
the condition. The signatures also carry the same auxiliary and witness
corrections. Their different Fiat–Shamir challenges normally hide different
leaves. For any repetition with different hidden leaves, the union of the two
openings exposes every leaf of that repetition.
The attacker reconstructs the corresponding VOLE value and removes it from the
public witness correction to obtain the complete signing witness.
The remaining secret-key prefix is the public key, so the recovered witness
reconstructs a working signing key.

The retained runtime witness resets the submitted 48-byte harness DRBG before
two `cat1_fast` signing calls, checks both signatures with the submitted
verifier, combines only their public openings, compares the recovered witness
with the secret key as a final control, and signs a fresh message with the
reconstructed key. A distinct-DRBG-seed control does not recover the witness:

```sh
python3 SDitH/reproduce_sdith_3.py
```

This is outside EUF-CMA because the attacker cannot normally reset or clone the
signer's RNG. It models a stuck RNG, process fork, or VM snapshot restore. The
mechanism is common to all 12 parameter sets; the retained end-to-end run covers
the category-1 fast reference library. Because this requires severe entropy
failure and SDitH does not claim hedged-signing robustness, it does not break
the required EUF-CMA property. It is rated Informational because NIST call
§4.B.4, physical p.18, identifies resilience to RNG malfunction and nonce
reuse as desirable rather than mandatory, and SDitH makes no hedged-signing or
reuse-resistance claim. The report still records the demonstrated full key
recovery and fresh-message forgery. Deriving signing
coins from the secret key, message, and fresh randomness would make the signer
robust to this exact entropy repetition.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SDitH-3
```
