# sign-10 Facto-DSA — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-10` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077457833984.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Facto-DSA
- Implementation versions measured: reference
- Parameter sets: `Facto-DSA-128`, `Facto-DSA-256`, `Facto-DSA-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-10/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Facto-DSA-128` | guide | PASS |
| `Facto-DSA-256` | guide | PASS |
| `Facto-DSA-512` | harness-default | NOKAT [1] |

[1] NOKAT: the submission contains no KAT file for Facto-DSA-512, so its output cannot be checked against submitted vectors; the other Facto-DSA instances pass. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Facto-DSA-128` | keygen | 7.34 M | 3.52 ms | 284 | 3.52 ms | 1355 (5 × 271) |
| `Facto-DSA-128` | sign | 2.10 M | 1 ms | 996 | 1 ms | 2595 (5 × 519) |
| `Facto-DSA-128` | verify | 983.9 k | 470 µs | 2.13e+03 | 470 µs | 10495 (5 × 2099) |
| `Facto-DSA-256` | keygen | 107.58 M | 51.4 ms | 19.4 | 51.3 ms | 100 (5 × 20) |
| `Facto-DSA-256` | sign | 11.72 M | 5.6 ms | 179 | 5.6 ms | 1260 (5 × 252) |
| `Facto-DSA-256` | verify | 11.19 M | 5.34 ms | 187 | 5.34 ms | 935 (5 × 187) |
| `Facto-DSA-512` | keygen | 2.24 G | 1.08 s | 0.928 | 1.08 s | 100 (5 × 20) |
| `Facto-DSA-512` | sign | 124.35 M | 59.5 ms | 16.8 | 59.4 ms | 900 (5 × 180) |
| `Facto-DSA-512` | verify | 140.34 M | 67.1 ms | 14.9 | 67.1 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Facto-DSA-128` | keygen | 43449 | 1764 KiB | 1916 KiB |
| `Facto-DSA-128` | sign | 43449 | 1844 KiB | 1912 KiB |
| `Facto-DSA-128` | verify | 43449 | 1844 KiB | 1908 KiB |
| `Facto-DSA-256` | keygen | 44533 | 1712 KiB | 2860 KiB |
| `Facto-DSA-256` | sign | 44533 | 2304 KiB | 2564 KiB |
| `Facto-DSA-256` | verify | 44533 | 2328 KiB | 2572 KiB |
| `Facto-DSA-512` | keygen | – | 1772 KiB | 13848 KiB |
| `Facto-DSA-512` | sign | – | 7488 KiB | 13540 KiB |
| `Facto-DSA-512` | verify | – | 7576 KiB | 13476 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Facto-DSA-128` | 40040 | 3094 | 40 |
| `Facto-DSA-256` | 456960 | 11662 | 68 |
| `Facto-DSA-512` | 5674240 | 61922 | 128 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Facto-DSA-128` | keygen | 11% | 2.6% | drng 1, sm3hash 1 |
| `Facto-DSA-128` | sign | 0.5% | 9.0% | drng 1, pseudoXOF 1 |
| `Facto-DSA-128` | verify | 87% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `Facto-DSA-256` | keygen | 9.2% | 0.5% | drng 3, sm3hash 1 |
| `Facto-DSA-256` | sign | 0.1% | 1.6% | drng 1, pseudoXOF 1 |
| `Facto-DSA-256` | verify | 87% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `Facto-DSA-512` | keygen | 5.5% | 0.1% | drng 16, sm3hash 1 |
| `Facto-DSA-512` | sign | 0.0% | 0.2% | drng 1, pseudoXOF 1 |
| `Facto-DSA-512` | verify | 88% | 0.0% | pseudoXOF 1, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Facto-DSA-128` | KAT log (sha256 `8885ba74036818dd…`) | `kat/sign-10/Facto-DSA-128.log` |
| `Facto-DSA-128` | timing keygen | `records/sign-10/Facto-DSA-128__keygen.json` |
| `Facto-DSA-128` | timing sign | `records/sign-10/Facto-DSA-128__sign.json` |
| `Facto-DSA-128` | timing verify | `records/sign-10/Facto-DSA-128__verify.json` |
| `Facto-DSA-128` | hash profile keygen | `profile/sign-10/Facto-DSA-128__keygen.json` |
| `Facto-DSA-128` | hash profile sign | `profile/sign-10/Facto-DSA-128__sign.json` |
| `Facto-DSA-128` | hash profile verify | `profile/sign-10/Facto-DSA-128__verify.json` |
| `Facto-DSA-256` | KAT log (sha256 `b26cf631cc16129a…`) | `kat/sign-10/Facto-DSA-256.log` |
| `Facto-DSA-256` | timing keygen | `records/sign-10/Facto-DSA-256__keygen.json` |
| `Facto-DSA-256` | timing sign | `records/sign-10/Facto-DSA-256__sign.json` |
| `Facto-DSA-256` | timing verify | `records/sign-10/Facto-DSA-256__verify.json` |
| `Facto-DSA-256` | hash profile keygen | `profile/sign-10/Facto-DSA-256__keygen.json` |
| `Facto-DSA-256` | hash profile sign | `profile/sign-10/Facto-DSA-256__sign.json` |
| `Facto-DSA-256` | hash profile verify | `profile/sign-10/Facto-DSA-256__verify.json` |
| `Facto-DSA-512` | KAT log (sha256 `46d8a75c2710998c…`) | `kat/sign-10/Facto-DSA-512.log` |
| `Facto-DSA-512` | timing keygen | `records/sign-10/Facto-DSA-512__keygen.json` |
| `Facto-DSA-512` | timing sign | `records/sign-10/Facto-DSA-512__sign.json` |
| `Facto-DSA-512` | timing verify | `records/sign-10/Facto-DSA-512__verify.json` |
| `Facto-DSA-512` | hash profile keygen | `profile/sign-10/Facto-DSA-512__keygen.json` |
| `Facto-DSA-512` | hash profile sign | `profile/sign-10/Facto-DSA-512__sign.json` |
| `Facto-DSA-512` | hash profile verify | `profile/sign-10/Facto-DSA-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

