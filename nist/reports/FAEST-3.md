# FAEST-3 — Low: Reference PRG ignores an OpenSSL AES failure and signs with unwritten buffers

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: All twelve parameter sets of the reference tree when built with OpenSSL support and run without the AES-NI path; the AVX2 and AArch64 trees are not affected
Discovery: Moderate
Exploitation: Environment-fault witness: crypto_sign returns success and emits a non-verifying signature; key bytes appeared in it only because unwritten buffers were zero-filled
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

The reference PRG takes its OpenSSL path when the build defines
`HAVE_OPENSSL` and the AES-NI path is unavailable: a build without AES-NI
intrinsics, or an x86 CPU without AES-NI at run time. On that path the result
of `EVP_EncryptInit_ex` is discarded
(`aes.c:1027` (`FAEST/submission/nist_submission_round3/Reference_Implementation/faest_128f/aes.c#L1027`)), and
`generic_prg` checks the cipher calls only with `assert`
(`aes.c:726-749` (`FAEST/submission/nist_submission_round3/Reference_Implementation/faest_128f/aes.c#L726`)).
The submitted
`Makefile:4` (`FAEST/submission/nist_submission_round3/Reference_Implementation/faest_128f/Makefile#L4`) sets
`-DNDEBUG`, which removes those asserts. When OpenSSL cannot provide AES, the
PRG output buffers are never written, and signing carries on.

## Evidence

The reproducer builds faest_128f and faest_em_256s with the submitted
Makefile's preprocessor flags and a build-time `config.h` that leaves
`HAVE_AESNI` undefined. It generates a key pair with working OpenSSL, then
signs once with working OpenSSL and once under an OpenSSL configuration that
activates only the null provider. Both outputs are checked by the same binary
with working OpenSSL.

- With the AES backend failing, `crypto_sign` returns 0 with the full
  signed-message length, and `crypto_sign_open` rejects the output (−1).
- Control: the same binary with working OpenSSL signs, and its output
  verifies. The default AES-NI build under the same failing configuration
  also signs and verifies.
- Recorded, not required: in every run, the first λ bits of the signature's
  `d` field equalled the secret OWF key, and every `c_i` byte was zero. This
  happens because the unwritten PRG buffers came from zero-filled heap pages;
  with other memory contents the key need not appear in this form.

## Classification and limits

Low / Confirmed / Implementation. The trigger is an environment fault (an
OpenSSL build or configuration without AES), not attacker input, and the
emitted signature does not verify. A caller that trusts the success return
may nonetheless publish secret-derived bytes. The no-AES-NI configuration was
emulated on x86 with `config.h`; no non-x86 build was run. Only two of the
twelve parameter sets were run; the others share the source.

Check every OpenSSL return value and propagate PRG failure out of
`crypto_sign`, rather than relying on `assert` in an `-DNDEBUG` build.

## Reproduction

```sh
python3 FAEST/reproduce_faest_3.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py FAEST-3
```
