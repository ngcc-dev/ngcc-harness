# SNOVA-10 — Low: Optional OpenSSL build leaks every digest context and crashes on allocation failure

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: Optimized tree, all 18 parameter sets by shared source, when built with -DUSE_OPENSSL (not the submitted default; SNOVA_I_K run)
Discovery: Trivial
Exploitation: Resource leak of 4 digest contexts per signature and a fail-stop crash under an injected allocation failure; no wrong output or secret exposure
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

With `-DUSE_OPENSSL`, the optimized tree's SHAKE wrapper allocates an OpenSSL
digest context with `EVP_MD_CTX_new` and passes it on without a NULL check
(`symmetric.c:24-32` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/symmetric.c#L24`)). Only `shake_release`
(`symmetric.c:50-54` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/symmetric.c#L50`)) frees a context, and no file in
the 18 optimized directories calls it: not `shake256`
(`symmetric.c:87-95` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/symmetric.c#L87`)), and not the two signer hashes
at `snova_opt_16.c:364-374` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/snova_opt_16.c#L364`) and
`snova_opt_16.c:780-788` (`SNOVA/submission/SNOVA_Round3/Optimized_Implementation/SNOVA_I_K/snova_opt_16.c#L780`). The submitted Makefile
does not enable this build.

## Evidence

The reproducer builds SNOVA_I_K with and without `-DUSE_OPENSSL`, wraps the
OpenSSL context functions with counters, and signs and verifies 20 messages.

- The OpenSSL build allocates 4 contexts per signature (5 when signing
  retries) and frees none: 145 of 145 contexts leak over the run, about
  2.1 kB per signature. Key generation and `crypto_sign_open` leak 3 each.
- With `EVP_MD_CTX_new` forced to return NULL at any of a signature's four
  allocations, the NULL context reaches `EVP_DigestInit_ex` and the process is
  killed by SIGSEGV; `crypto_sign` does not return.
- Controls: the default build never calls `EVP_MD_CTX_new` and signs normally
  under the same forced failure; a temporary copy that calls `shake_release`
  at all three hashing sites frees every context, while one that patches
  `shake256` alone still leaks two per signature. All builds give the same
  keys and signatures.

## Classification and limits

Low / Confirmed / Implementation: a resource leak and an unchecked allocation
in an optional build, with the consequences demonstrated. The allocation
failure is injected, no wrong signature is produced, and no secret is exposed.
Only SNOVA_I_K was built; the crash behaviour depends on the OpenSSL version
recorded by the reproducer.

Check the result of `EVP_MD_CTX_new` and call `shake_release` after each of
the three hashing sites.

## Reproduction

```sh
python3 SNOVA/reproduce_snova_10.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-10
```
