# sign-32 UVW signature — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-32` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105735831552.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: UVW signature
- Implementation versions measured: reference
- Parameter sets: `UVW-128`, `UVW-256`, `UVW-512`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T08:43:52 |

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
| `UVW-128` | keygen | 61.26 G | 22.7 s | 0.044 | 22.6 s | 15 (5 × 3) |
| `UVW-128` | sign | 21.01 G | 7.8 s | 0.128 | 7.79 s | 65 (5 × 13) |
| `UVW-128` | verify | 225.85 M | 83.8 ms | 11.9 | 83.8 ms | 100 (5 × 20) |
| `UVW-256` | keygen | 837.91 G | 311 s | 0.00322 | 311 s | 1 (1 × 1) |
| `UVW-256` | sign | 185.89 G | 69 s | 0.0145 | 59.2 s | 3 (3 × 1) |
| `UVW-256` | verify | 927.11 M | 344 ms | 2.91 | 344 ms | 100 (5 × 20) |
| `UVW-512` | keygen | 10404.84 G | 3.86e+03 s | 0.000259 | 3.86e+03 s | 1 (1 × 1) |
| `UVW-512` | sign | 1564.34 G | 581 s | 0.00172 | 581 s | 1 (1 × 1) |
| `UVW-512` | verify | 4.17 G | 1.55 s | 0.646 | 1.53 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `UVW-128` | keygen | 53892 | 1452 KiB | 96404 KiB |
| `UVW-128` | sign | 53892 | 64644 KiB | 385704 KiB |
| `UVW-128` | verify | 53892 | 63844 KiB | 64868 KiB |
| `UVW-256` | keygen | 53892 | 1448 KiB | 217272 KiB |
| `UVW-256` | sign | 53892 | 147076 KiB | 505540 KiB |
| `UVW-256` | verify | 53892 | 286204 KiB | 286268 KiB |
| `UVW-512` | keygen | 49924 | 1448 KiB | 875444 KiB |
| `UVW-512` | sign | 49924 | 594280 KiB | 1157544 KiB |
| `UVW-512` | verify | 49924 | 1158216 KiB | 1158280 KiB |

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
| `UVW-128` | keygen | 0.0% | 1.5% | drng 3.34e+04 |
| `UVW-128` | sign | 0.0% | 3.1% | drng 9.74e+04, pseudoXOF 1 |
| `UVW-128` | verify | 2.1% | 0.0% | pseudoXOF 1 |
| `UVW-256` | keygen | 0.0% | 0.6% | drng 1.09e+05 |
| `UVW-256` | sign | 0.0% | 3.4% | drng 1.59e+06, pseudoXOF 1 |
| `UVW-256` | verify | 0.6% | 0.0% | pseudoXOF 1 |
| `UVW-512` | keygen | 0.0% | 0.3% | drng 3.51e+05 |
| `UVW-512` | sign | 0.0% | 0.4% | drng 2.89e+05, pseudoXOF 1 |
| `UVW-512` | verify | 0.2% | 0.0% | pseudoXOF 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `UVW-128` | KAT log (sha256 `02fbc3d4b32b6a15…`) | `kat/sign-32/UVW-128.log` |
| `UVW-128` | timing keygen | `records/sign-32/UVW-128__keygen.json` |
| `UVW-128` | timing sign | `records/sign-32/UVW-128__sign.json` |
| `UVW-128` | timing verify | `records/sign-32/UVW-128__verify.json` |
| `UVW-128` | hash profile keygen | `profile/sign-32/UVW-128__keygen.json` |
| `UVW-128` | hash profile sign | `profile/sign-32/UVW-128__sign.json` |
| `UVW-128` | hash profile verify | `profile/sign-32/UVW-128__verify.json` |
| `UVW-256` | KAT log (sha256 `d34d6e65965da1aa…`) | `kat/sign-32/UVW-256.log` |
| `UVW-256` | timing keygen | `records/sign-32/UVW-256__keygen.json` |
| `UVW-256` | timing sign | `records/sign-32/UVW-256__sign.json` |
| `UVW-256` | timing verify | `records/sign-32/UVW-256__verify.json` |
| `UVW-256` | hash profile keygen | `profile/sign-32/UVW-256__keygen.json` |
| `UVW-256` | hash profile sign | `profile/sign-32/UVW-256__sign.json` |
| `UVW-256` | hash profile verify | `profile/sign-32/UVW-256__verify.json` |
| `UVW-512` | KAT log (sha256 `af097fb460a8dd43…`) | `kat/sign-32/UVW-512.log` |
| `UVW-512` | timing keygen | `records/sign-32/UVW-512__keygen.json` |
| `UVW-512` | timing sign | `records/sign-32/UVW-512__sign.json` |
| `UVW-512` | timing verify | `records/sign-32/UVW-512__verify.json` |
| `UVW-512` | hash profile keygen | `profile/sign-32/UVW-512__keygen.json` |
| `UVW-512` | hash profile sign | `profile/sign-32/UVW-512__sign.json` |
| `UVW-512` | hash profile verify | `profile/sign-32/UVW-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

