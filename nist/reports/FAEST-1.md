# FAEST-1 — Low: cross-profile signing with repeated randomness reveals the secret key

Severity: Low
Status: Confirmed
Layer: Design
Affected: Round-3 FAEST and FAEST-EM short/fast pairs at 128, 192, and 256 bits when one key signs the same message in both profiles with the same rho
Discovery: Non-trivial
Exploitation: End-to-end key recovery and fresh-message forgery under same-key, same-message, cross-profile signing with repeated rho
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

FAEST signing samples a `lambda`-bit `rho`, with the all-zero string specified
for deterministic signing (Section 8.2, Figure 8.2, p. 65).  The short and fast
profiles at a given security level use the same key format, and no profile
identifier enters `(r, iv_pre) = H3(k || mu || rho)`.  Consequently, the same
key, message, and `rho` give both profiles the same root seed and IV.  This
includes deterministic signing and any replay of the randomized signer's RNG
state.  The public BAVC tree openings (Figure 5.4, p. 29) then overlap so that
they supply every hidden leaf of one signature.  Expanding those openings
recovers its VOLE mask and the secret OWF key `k`.

The retained witness resets the submitted KAT DRBG to the same state before the
ordinary randomized signing call in each profile.  It confirms that the first
generated `rho` is the same and nonzero, publicly selects the recovered key
candidate, and produces a valid signature on a fresh message.  It never uses
the original secret key to choose the candidate or create the forgery.  A
different-seed control has different `iv_pre` values, zero compatible hidden
leaves, zero candidates, and no forgery:

```sh
python3 FAEST/reproduce_faest_1.py
```

The retained witness passes for all six FAEST and FAEST-EM short/fast pairs at
128, 192, and 256 bits.  The default run is the inexpensive category-1
representative; another pair can be selected with two arguments, for example
`faest_em_192s faest_em_192f`.  Their common mechanism follows directly from
the shared key format and profile-free root/tree derivation.

This is outside the NIST single-parameter-set EUF-CMA experiment. It requires
an application to share one key between two profiles, sign the same message in
both, and repeat `rho`.  Correct fresh signing randomness does not produce the
common tree. The Low rating rests on the missing cross-profile domain separation
and the specified deterministic-signing mode, which supplies the same all-zero
`rho` without an RNG malfunction. RNG-state replay is an additional trigger,
but is not the basis for raising this finding above Informational. The
cross-profile key reuse and same-message preconditions remain outside the
normal per-parameter-set experiment. Bind an unambiguous
parameter-set identifier into `H3` or `H4` (or the message representative), or
explicitly prohibit key reuse across parameter profiles.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py FAEST-1
```
