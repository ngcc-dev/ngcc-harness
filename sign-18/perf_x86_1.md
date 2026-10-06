# sign-18 Origami — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-18` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561086978904064.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Origami
- Implementation versions measured: reference
- Parameter sets: `Origami-128`, `Origami-256`, `Origami-384`, `Origami-512`

## 2. Assessment environment

| item | value |
|---|---|
| processor | 12th Gen Intel(R) Core(TM) i7-12700 (CPU 2, one core) |
| clock | max 2.10 GHz, governor performance, turbo off, SMT off |
| memory | 31788 MiB |
| OS / kernel | Debian GNU/Linux 13 (trixie) / 6.12.107+deb13-amd64 |
| compiler / build tool | gcc (Debian 14.2.0-19) 14.2.0 / cmake version 3.31.6 |
| campaign start / end (UTC) | 2026-09-25T10:02:21 / 2026-09-28T10:51:21 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-18/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Origami-128` | guide | PASS [1] |
| `Origami-256` | guide | PASS [1] |
| `Origami-384` | guide | PASS [1] |
| `Origami-512` | guide | PASS [1] |

[1] Not a KAT problem (KATs pass): sig_sign fails with return code -4 for about 8% of (message, salt) pairs — for some targets every one of the MAX_SIGN_ATTEMPTS = 8192 zone solves in origami_ref.c fails — and the API does not retry. The benchmark retries signing with a fresh salt, as an application would; retries are included in the signing time and counted in each record (sign_failures_retried). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Origami-128` | keygen | 2.53 M | 1.21 ms | 829 | 1.21 ms | 4005 (5 × 801) |
| `Origami-128` | sign | 405.85 M | 194 ms | 5.16 | 194 ms | 360 (5 × 72); 15 failed signing attempts retried |
| `Origami-128` | verify | 25.87 M | 12.4 ms | 80.9 | 12.4 ms | 405 (5 × 81); 5 failed signing attempts retried |
| `Origami-256` | keygen | 88.77 M | 42.4 ms | 23.6 | 42.4 ms | 120 (5 × 24) |
| `Origami-256` | sign | 1.51 G | 722 ms | 1.39 | 719 ms | 100 (5 × 20) |
| `Origami-256` | verify | 1.38 G | 659 ms | 1.52 | 659 ms | 100 (5 × 20) |
| `Origami-384` | keygen | 306.15 M | 146 ms | 6.84 | 146 ms | 100 (5 × 20) |
| `Origami-384` | sign | 19.85 G | 9.48 s | 0.105 | 9.48 s | 25 (5 × 5); 5 failed signing attempts retried |
| `Origami-384` | verify | 6.48 G | 3.1 s | 0.323 | 3.09 s | 100 (5 × 20) |
| `Origami-512` | keygen | 499.54 M | 240 ms | 4.17 | 239 ms | 100 (5 × 20) |
| `Origami-512` | sign | 13.08 G | 6.28 s | 0.159 | 6.27 s | 100 (5 × 20) |
| `Origami-512` | verify | 11.92 G | 5.71 s | 0.175 | 5.71 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Origami-128` | keygen | 41989 | 1728 KiB | 1832 KiB |
| `Origami-128` | sign | 41989 | 1724 KiB | 1800 KiB |
| `Origami-128` | verify | 41989 | 1732 KiB | 1840 KiB |
| `Origami-256` | keygen | 42437 | 1740 KiB | 1924 KiB |
| `Origami-256` | sign | 42437 | 1824 KiB | 1960 KiB |
| `Origami-256` | verify | 42437 | 1844 KiB | 1948 KiB |
| `Origami-384` | keygen | 42501 | 1692 KiB | 1992 KiB |
| `Origami-384` | sign | 42501 | 1900 KiB | 2056 KiB |
| `Origami-384` | verify | 42501 | 1940 KiB | 2004 KiB |
| `Origami-512` | keygen | 42717 | 1748 KiB | 2020 KiB |
| `Origami-512` | sign | 42717 | 1892 KiB | 2080 KiB |
| `Origami-512` | verify | 42717 | 1964 KiB | 2076 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Origami-128` | 2996 | 16 | 116 |
| `Origami-256` | 14968 | 32 | 516 |
| `Origami-384` | 27940 | 48 | 948 |
| `Origami-512` | 35924 | 64 | 1220 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Origami-128` | keygen | 98% | 0.2% | drng 1, pseudoXOF 349 |
| `Origami-128` | sign | 91% | 0.0% | drng 1.03, pseudoXOF 2.29e+03, pseudohash 1.03 |
| `Origami-128` | verify | 83% | 0.0% | pseudoXOF 304, pseudohash 1 |
| `Origami-256` | keygen | 100% | 0.0% | drng 1, pseudoXOF 1.68e+03 |
| `Origami-256` | sign | 81% | 0.0% | drng 1, pseudoXOF 4.78e+03, pseudohash 1 |
| `Origami-256` | verify | 82% | 0.0% | pseudoXOF 3.86e+03, pseudohash 1 |
| `Origami-384` | keygen | 100% | 0.0% | drng 1, pseudoXOF 3.11e+03 |
| `Origami-384` | sign | 81% | 0.0% | drng 1.33, pseudoXOF 6.69e+04, pseudohash 1.33 |
| `Origami-384` | verify | 81% | 0.0% | pseudoXOF 1.56e+04, pseudohash 1 |
| `Origami-512` | keygen | 100% | 0.0% | drng 1, pseudoXOF 4e+03 |
| `Origami-512` | sign | 79% | 0.0% | drng 1, pseudoXOF 2.95e+04, pseudohash 1 |
| `Origami-512` | verify | 81% | 0.0% | pseudoXOF 2.77e+04, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Origami-128` | KAT log (sha256 `5d4395c0a8cd394d…`) | `kat/sign-18/Origami-128.log` |
| `Origami-128` | timing keygen | `records/sign-18/Origami-128__keygen.json` |
| `Origami-128` | timing sign | `records/sign-18/Origami-128__sign.json` |
| `Origami-128` | timing verify | `records/sign-18/Origami-128__verify.json` |
| `Origami-128` | hash profile keygen | `profile/sign-18/Origami-128__keygen.json` |
| `Origami-128` | hash profile sign | `profile/sign-18/Origami-128__sign.json` |
| `Origami-128` | hash profile verify | `profile/sign-18/Origami-128__verify.json` |
| `Origami-256` | KAT log (sha256 `5591616a49cbc2f2…`) | `kat/sign-18/Origami-256.log` |
| `Origami-256` | timing keygen | `records/sign-18/Origami-256__keygen.json` |
| `Origami-256` | timing sign | `records/sign-18/Origami-256__sign.json` |
| `Origami-256` | timing verify | `records/sign-18/Origami-256__verify.json` |
| `Origami-256` | hash profile keygen | `profile/sign-18/Origami-256__keygen.json` |
| `Origami-256` | hash profile sign | `profile/sign-18/Origami-256__sign.json` |
| `Origami-256` | hash profile verify | `profile/sign-18/Origami-256__verify.json` |
| `Origami-384` | KAT log (sha256 `daff883390fdc4fd…`) | `kat/sign-18/Origami-384.log` |
| `Origami-384` | timing keygen | `records/sign-18/Origami-384__keygen.json` |
| `Origami-384` | timing sign | `records/sign-18/Origami-384__sign.json` |
| `Origami-384` | timing verify | `records/sign-18/Origami-384__verify.json` |
| `Origami-384` | hash profile keygen | `profile/sign-18/Origami-384__keygen.json` |
| `Origami-384` | hash profile sign | `profile/sign-18/Origami-384__sign.json` |
| `Origami-384` | hash profile verify | `profile/sign-18/Origami-384__verify.json` |
| `Origami-512` | KAT log (sha256 `f7e6265d0c1d6da4…`) | `kat/sign-18/Origami-512.log` |
| `Origami-512` | timing keygen | `records/sign-18/Origami-512__keygen.json` |
| `Origami-512` | timing sign | `records/sign-18/Origami-512__sign.json` |
| `Origami-512` | timing verify | `records/sign-18/Origami-512__verify.json` |
| `Origami-512` | hash profile keygen | `profile/sign-18/Origami-512__keygen.json` |
| `Origami-512` | hash profile sign | `profile/sign-18/Origami-512__sign.json` |
| `Origami-512` | hash profile verify | `profile/sign-18/Origami-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

