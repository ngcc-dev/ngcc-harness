# sign-32 UVW signature — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-32` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105735831552.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: UVW signature
- Implementation versions measured: reference
- Parameter sets: `UVW-128`, `UVW-256`, `UVW-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-32/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `UVW-128` | harness-default | CRYPTOFAIL [1] |
| `UVW-256` | harness-default | CRYPTOFAIL [1] |
| `UVW-512` | harness-default | CRYPTOFAIL [1] |

[1] CRYPTOFAIL: sig_verify computes the verification result but returns 0 unconditionally, so every signature (including a modified message) is accepted (confirmed finding sign-32-1); the generated KAT text itself matches the submitted vectors (sign-32/Makefile). Timed anyway: verification time is that of the full check whose result is discarded. UVW-512 key generation alone takes more than 30 minutes, so its operations have few timed calls. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `UVW-128` | keygen | 78.68 G | 37.8 s | 0.0265 | 37.6 s | 20 (5 × 4) |
| `UVW-128` | sign | 30.29 G | 14.5 s | 0.0689 | 14.5 s | 95 (5 × 19) |
| `UVW-128` | verify | 340.31 M | 163 ms | 6.15 | 162 ms | 100 (5 × 20) |
| `UVW-256` | keygen | 1048.15 G | 503 s | 0.00199 | 503 s | 1 (1 × 1) |
| `UVW-256` | sign | 253.22 G | 121 s | 0.00824 | 117 s | 5 (5 × 1) |
| `UVW-256` | verify | 1.96 G | 940 ms | 1.06 | 921 ms | 100 (5 × 20) |
| `UVW-512` | keygen | 11676.24 G | 5.59e+03 s | 0.000179 | 5.59e+03 s | 1 (1 × 1) |
| `UVW-512` | sign | 1935.27 G | 926 s | 0.00108 | 926 s | 1 (1 × 1) |
| `UVW-512` | verify | 23.49 G | 11.2 s | 0.0889 | 11.3 s | 80 (5 × 16) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `UVW-128` | keygen | – | 1728 KiB | 110536 KiB |
| `UVW-128` | sign | – | 63288 KiB | 471184 KiB |
| `UVW-128` | verify | – | 62424 KiB | 63124 KiB |
| `UVW-256` | keygen | – | 1560 KiB | 215440 KiB |
| `UVW-256` | sign | – | 146624 KiB | 699544 KiB |
| `UVW-256` | verify | – | 285280 KiB | 285408 KiB |
| `UVW-512` | keygen | – | 1696 KiB | 874208 KiB |
| `UVW-512` | sign | – | 592976 KiB | 1157688 KiB |
| `UVW-512` | verify | – | 1157560 KiB | 1157684 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `UVW-128` | 5897612 | 40 | 1244 |
| `UVW-256` | 23040012 | 40 | 2444 |
| `UVW-512` | 95160012 | 72 | 4956 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — grep hits for AES/Keccak/OpenSSL are not reachable in the built library

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `UVW-128` | keygen | 0.0% | 1.2% | drng 3.34e+04 |
| `UVW-128` | sign | 0.0% | 2.1% | drng 8.84e+04, pseudoXOF 1 |
| `UVW-128` | verify | 1.0% | 0.0% | pseudoXOF 1 |
| `UVW-256` | keygen | 0.0% | 0.6% | drng 1.09e+05 |
| `UVW-256` | sign | 0.0% | 2.5% | drng 1.59e+06, pseudoXOF 1 |
| `UVW-256` | verify | 0.2% | 0.0% | pseudoXOF 1 |
| `UVW-512` | keygen | 0.0% | 0.3% | drng 3.51e+05 |
| `UVW-512` | sign | 0.0% | 0.4% | drng 2.89e+05, pseudoXOF 1 |
| `UVW-512` | verify | 0.0% | 0.0% | pseudoXOF 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `UVW-128` | KAT log (sha256 `0d572a73d7da5b16…`) | `kat/sign-32/UVW-128.log` |
| `UVW-128` | timing keygen | `records/sign-32/UVW-128__keygen.json` |
| `UVW-128` | timing sign | `records/sign-32/UVW-128__sign.json` |
| `UVW-128` | timing verify | `records/sign-32/UVW-128__verify.json` |
| `UVW-128` | hash profile keygen | `profile/sign-32/UVW-128__keygen.json` |
| `UVW-128` | hash profile sign | `profile/sign-32/UVW-128__sign.json` |
| `UVW-128` | hash profile verify | `profile/sign-32/UVW-128__verify.json` |
| `UVW-256` | KAT log (sha256 `c40a519459c2e435…`) | `kat/sign-32/UVW-256.log` |
| `UVW-256` | timing keygen | `records/sign-32/UVW-256__keygen.json` |
| `UVW-256` | timing sign | `records/sign-32/UVW-256__sign.json` |
| `UVW-256` | timing verify | `records/sign-32/UVW-256__verify.json` |
| `UVW-256` | hash profile keygen | `profile/sign-32/UVW-256__keygen.json` |
| `UVW-256` | hash profile sign | `profile/sign-32/UVW-256__sign.json` |
| `UVW-256` | hash profile verify | `profile/sign-32/UVW-256__verify.json` |
| `UVW-512` | KAT log (sha256 `05a5474b7e03d402…`) | `kat/sign-32/UVW-512.log` |
| `UVW-512` | timing keygen | `records/sign-32/UVW-512__keygen.json` |
| `UVW-512` | timing sign | `records/sign-32/UVW-512__sign.json` |
| `UVW-512` | timing verify | `records/sign-32/UVW-512__verify.json` |
| `UVW-512` | hash profile keygen | `profile/sign-32/UVW-512__keygen.json` |
| `UVW-512` | hash profile sign | `profile/sign-32/UVW-512__sign.json` |
| `UVW-512` | hash profile verify | `profile/sign-32/UVW-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

