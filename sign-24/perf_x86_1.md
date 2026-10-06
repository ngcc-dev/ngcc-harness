# sign-24 Sigurd — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-24` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096235732992.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Sigurd
- Implementation versions measured: reference
- Parameter sets: `Sigurd-128`, `Sigurd-256`, `Sigurd-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-24/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Sigurd-128` | guide | PASS |
| `Sigurd-256` | guide | PASS |
| `Sigurd-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Sigurd-128` | keygen | 21.21 M | 10.2 ms | 98.3 | 10.2 ms | 300 (5 × 60) |
| `Sigurd-128` | sign | 37.45 M | 18 ms | 55.4 | 18 ms | 280 (5 × 56) |
| `Sigurd-128` | verify | 15.26 M | 7.29 ms | 137 | 7.29 ms | 550 (5 × 110) |
| `Sigurd-256` | keygen | 113.63 M | 54.4 ms | 18.4 | 54.4 ms | 100 (5 × 20) |
| `Sigurd-256` | sign | 325.62 M | 156 ms | 6.42 | 156 ms | 100 (5 × 20) |
| `Sigurd-256` | verify | 110.82 M | 53.3 ms | 18.8 | 53.3 ms | 100 (5 × 20) |
| `Sigurd-512` | keygen | 564.66 M | 271 ms | 3.69 | 270 ms | 100 (5 × 20) |
| `Sigurd-512` | sign | 2.97 G | 1.43 s | 0.702 | 1.43 s | 100 (5 × 20) |
| `Sigurd-512` | verify | 931.31 M | 462 ms | 2.17 | 462 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Sigurd-128` | keygen | 777857 | 1776 KiB | 2944 KiB |
| `Sigurd-128` | sign | 777857 | 2692 KiB | 3252 KiB |
| `Sigurd-128` | verify | 777857 | 2888 KiB | 9052 KiB |
| `Sigurd-256` | keygen | 845041 | 1728 KiB | 4672 KiB |
| `Sigurd-256` | sign | 845041 | 2724 KiB | 6404 KiB |
| `Sigurd-256` | verify | 845041 | 4772 KiB | 30700 KiB |
| `Sigurd-512` | keygen | 1771257 | 1720 KiB | 15376 KiB |
| `Sigurd-512` | sign | 1771257 | 2840 KiB | 29580 KiB |
| `Sigurd-512` | verify | 1771257 | 16580 KiB | 129688 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Sigurd-128` | 112 | 80 | 25108 ≈ nominal |
| `Sigurd-256` | 212 | 128 | 74756 ≈ nominal |
| `Sigurd-512` | 435 | 256 | 282692 ≈ nominal |

The submitted signature API reports a buffer capacity, not a fixed transmitted length; the nominal figure above is the specification's representative size, and actual signatures vary.

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Sigurd-128` | keygen | 21% | 0.1% | drng 2, pseudoXOF 2 |
| `Sigurd-128` | sign | 31% | 1.4% | drng 2, pseudoXOF 7, sm3hash 3.15e+03 |
| `Sigurd-128` | verify | 43% | 0.0% | pseudoXOF 6, sm3hash 743 |
| `Sigurd-256` | keygen | 32% | 0.0% | drng 2, pseudoXOF 2 |
| `Sigurd-256` | sign | 57% | 0.4% | drng 2, pseudoXOF 7, pseudohash 1.04e+04 |
| `Sigurd-256` | verify | 49% | 0.0% | pseudoXOF 6, pseudohash 1.05e+03 |
| `Sigurd-512` | keygen | 40% | 0.0% | drng 2, pseudoXOF 2 |
| `Sigurd-512` | sign | 62% | 0.2% | drng 2, pseudoXOF 7, pseudohash 3.97e+04 |
| `Sigurd-512` | verify | 34% | 0.0% | pseudoXOF 6, pseudohash 1.96e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Sigurd-128` | KAT log (sha256 `4f7d471565795e18…`) | `kat/sign-24/Sigurd-128.log` |
| `Sigurd-128` | timing keygen | `records/sign-24/Sigurd-128__keygen.json` |
| `Sigurd-128` | timing sign | `records/sign-24/Sigurd-128__sign.json` |
| `Sigurd-128` | timing verify | `records/sign-24/Sigurd-128__verify.json` |
| `Sigurd-128` | hash profile keygen | `profile/sign-24/Sigurd-128__keygen.json` |
| `Sigurd-128` | hash profile sign | `profile/sign-24/Sigurd-128__sign.json` |
| `Sigurd-128` | hash profile verify | `profile/sign-24/Sigurd-128__verify.json` |
| `Sigurd-256` | KAT log (sha256 `77a1dbae1634047b…`) | `kat/sign-24/Sigurd-256.log` |
| `Sigurd-256` | timing keygen | `records/sign-24/Sigurd-256__keygen.json` |
| `Sigurd-256` | timing sign | `records/sign-24/Sigurd-256__sign.json` |
| `Sigurd-256` | timing verify | `records/sign-24/Sigurd-256__verify.json` |
| `Sigurd-256` | hash profile keygen | `profile/sign-24/Sigurd-256__keygen.json` |
| `Sigurd-256` | hash profile sign | `profile/sign-24/Sigurd-256__sign.json` |
| `Sigurd-256` | hash profile verify | `profile/sign-24/Sigurd-256__verify.json` |
| `Sigurd-512` | KAT log (sha256 `f41196b4557885ce…`) | `kat/sign-24/Sigurd-512.log` |
| `Sigurd-512` | timing keygen | `records/sign-24/Sigurd-512__keygen.json` |
| `Sigurd-512` | timing sign | `records/sign-24/Sigurd-512__sign.json` |
| `Sigurd-512` | timing verify | `records/sign-24/Sigurd-512__verify.json` |
| `Sigurd-512` | hash profile keygen | `profile/sign-24/Sigurd-512__keygen.json` |
| `Sigurd-512` | hash profile sign | `profile/sign-24/Sigurd-512__sign.json` |
| `Sigurd-512` | hash profile verify | `profile/sign-24/Sigurd-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

