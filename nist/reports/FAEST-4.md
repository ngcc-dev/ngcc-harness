# FAEST-4 — Low: AVX2 and AArch64 crypto_sign_open read before a short input and crash

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: All twelve parameter sets of the AVX2 tree (run) and the AArch64 tree (identical api.hpp, not run); the reference tree is not affected
Discovery: Trivial
Exploitation: A short signed message chosen by the sender terminates the verifier with SIGSEGV; no memory disclosure was shown
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

The AVX2 `crypto_sign_open` computes the message length as
`smlen - FAEST_SIGNATURE_BYTES<P>` with no prior check
(`api.hpp:67` (`FAEST/submission/nist_submission_round3/Additional_Implementations/avx2/faest_128f/api.hpp#L67`)).
For a shorter input the subtraction wraps, and the verifier reads the
signature from before the caller's buffer and hashes a message of about
`2^64` bytes. The reference tree checks the length first
(`crypto_sign.c:34` (`FAEST/submission/nist_submission_round3/Reference_Implementation/faest_128f/crypto_sign.c#L34`)).
The AArch64 tree's `api.hpp` is byte-identical to the AVX2 file.

## Evidence

The reproducer builds faest_128f from the AVX2 tree with the submitted
Makefile's flags and from the reference tree, and calls `crypto_sign_open` on
all-zero inputs in separate child processes with a timeout.

- AVX2, `smlen` 5169 and 100: the process is killed by SIGSEGV.
- Control: AVX2 with a full-length invalid input (5170 bytes) returns −1, and
  the reference build returns −1 for 5169, 100, 0 and 5170 bytes. Both builds
  sign and verify a valid message.

The AVX2 verifier first tests the grinding bits of the claimed signature,
which for `smlen >= 21` lie in the supplied input. The crash needs those bits
to be zero, which the sender chooses; with them set, a 100-byte input returns
−1. The `smlen = 0` case depends on neighbouring heap contents and is recorded
only.

## Classification and limits

Low / Confirmed / Implementation: a crash reachable through the submitted
API, with no memory disclosure or control-flow effect shown. The AArch64 tree
and the other eleven parameter sets were not run.

Reject `smlen < FAEST_SIGNATURE_BYTES<P>` before the subtraction, as the
reference tree does.

## Reproduction

```sh
python3 FAEST/reproduce_faest_4.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py FAEST-4
```
