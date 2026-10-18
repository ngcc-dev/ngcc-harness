# SDitH-1 — Low: portable reference backend has secret-dependent tables and branches

**Severity:** Low
**Status:** Confirmed
**Layer:** Side-channel
**Affected:** Round-3 portable reference key generation and signing; all 12 parameter sets, including repository KAT builds
**Discovery:** Trivial
**Exploitation:** source/object-only; no measured channel or key recovery
**Remediation:** Local
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Date:** 2026-10-18

## Affected operations

The public harness deliberately builds the portable submitted reference backend
with `ONLY_REF_IMPLEMENTATION` for all 12 libraries. Secret-dependent traces
occur in both key generation and signing.

During key generation, `sdith_prng.c:1002-1024` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/sdith_prng.c#L1002`)
uses the secret-key seed directly as the AES-128 or Rijndael-256 key for the
key-generation PRG. The reference cipher's
`aes_ansi_littleendian_ref.c:338-385` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/lib/aes/aes_ansi_littleendian_ref.c#L338`)
indexes `Te0` through `Te3` with cipher state and also uses `Te4` during the
key schedule. Those memory addresses depend on the secret seed on a cacheful
machine.

Witness positions are derived from the same secret PRG in
`rsd.c:46-64` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/rsd.c#L46`).
The accepted 32-bit draw is passed to
`div_rem_u32_ct_ref` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/ct_utils.c#L27`),
whose fixed 32-round loop branches on the secret partial remainder at lines
33–36. In the current GCC-built `cat1_fast.so`, `objdump` confirms a conditional
`jb` at the corresponding comparison. This object check shows that the source
branch survived compilation; it is not a timing measurement.

Signing reaches the same reference table cipher with ephemeral secret state.
`sdith_signature.c:348-358` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/sdith_signature.c#L348`)
copies fresh entropy into the secret GGM root seed and invokes the prover tree
expansion. The expansion derives every internal node and leaf, then schedules
each leaf seed as an AES/Rijndael key
(`vole_expansion.c:146-218` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/vole_expansion.c#L146`),
`sdith_prng.c:19-70` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/sdith_prng.c#L19`)).
The reference key schedule and encryption therefore expose table addresses
derived from secret GGM seeds on every signature. In particular, recovering a
hidden leaf seed would expose its VOLE mask; the public opening and witness
correction then give a route to the long-term witness. This report does not
demonstrate that cache-key recovery step.

All 12 reference directories have identical relevant source hashes. The
submitted optimized directories contain AES-instruction and AVX2 helper paths;
`ct_utils.c:7-14` itself warns that the readable C helpers are not hardened and
directs constant-time builds to the AVX function table.

## Impact and limits

A local observer of cache addresses or sufficiently detailed branch behavior
can observe functions of secret key-generation and signing state. No trace was
collected, and this report does not reconstruct witness positions or GGM seeds,
recover a key, or forge a signature. Switching to the submitted native
optimized backend on supported x86 systems removes these particular
portable-reference operations. The source-only evidence and supplied
replacement support Low severity.

The rejection loop at `rsd.c:57-63` also has a secret-derived iteration count,
but each rejected PRG draw is discarded and independent of the accepted draw.
No candidate-specific consequence from that rare count was identified; it is
not the basis for a higher rating.

## Reproduction

```sh
python3 SDitH/reproduce_sdith_1.py
objdump -d --disassemble=div_rem_u32_ct_ref SDitH/lib/cat1_fast.so
```

The Python witness checks all 12 exact source copies, long-term and ephemeral
secret flow into the table cipher, key-generation division, harness backend
selection, and the supplied hardened helper alternative. The optional
`objdump` command requires a local build and shows generated control flow only.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SDitH-1
```
