# FAEST-5 — Low: A getrandom failure is ignored, so key generation returns a predictable key

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: All twelve parameter sets of the reference and AVX2 trees (AArch64 by identical source) in builds that use the operating-system randomness path; KAT builds are not affected
Discovery: Trivial
Exploitation: Environment-fault witness: with getrandom failing, key generation returns success and the secret key is the caller's buffer contents
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

`rand_bytes` calls `getrandom(..., GRND_NONBLOCK)` and returns −1 when it
fails (`randomness.c:38-44` (`FAEST/submission/nist_submission_round3/Reference_Implementation/faest_128f/randomness.c#L38`);
the AVX2 `randomness_os.c` is the same). Every caller ignores the result: key
generation and signing in
`faest_128f.c:40,44,112,123` (`FAEST/submission/nist_submission_round3/Reference_Implementation/faest_128f/faest_128f.c#L40`)
and the AVX2 `api.hpp:32,34,46,59`. With `GRND_NONBLOCK`, `getrandom` fails
with `EAGAIN` before the kernel entropy pool is initialised, for example early
in boot.

## Evidence

The reproducer builds reference faest_128f and faest_em_256s and AVX2
faest_128f with their operating-system randomness files, and runs them with a
preloaded `getrandom` that fails with `EAGAIN`.

- `rand_bytes` returns −1, and `crypto_sign_keypair` still returns 0.
- The secret OWF key is whatever the caller's buffer held, for a zero and a
  non-zero prefill. With a zeroed buffer, the faest_128f public key is the
  AES-128 encryption of the zero block under the zero key.
- The reference tree leaves the whole secret key as the caller's buffer; the
  AVX2 tree first zeroes the public OWF-input half.
- Control: without the failing `getrandom`, two runs give different, fresh
  keys and the signatures verify.
- Recorded, not required: under the fault, two signatures of one message were
  identical (the signing randomness `rho` is an uninitialised stack array),
  and a prefill with both low key bits set made key generation loop without
  end.

## Classification and limits

Low / Confirmed / Implementation: an ignored randomness failure with a
demonstrated consequence through the submitted API in the default build. The
trigger is an operating-system condition, not attacker input. The AArch64 tree
and the other parameter sets were not run; KAT builds use a deterministic
`randombytes` and do not take this path.

Propagate the `rand_bytes` result and fail key generation and signing, or use
a blocking `getrandom` call.

## Reproduction

```sh
python3 FAEST/reproduce_faest_5.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py FAEST-5
```
