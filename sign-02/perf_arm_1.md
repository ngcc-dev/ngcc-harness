# sign-02 BIT: Bimodal Triangular distribution based lattice signatures — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-02` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076358926336.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: BIT: Bimodal Triangular distribution based lattice signatures
- Implementation versions measured: reference
- Parameter sets: `BiT-128`, `BiT-256`, `BiT-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-02/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BiT-128` | guide | PASS |
| `BiT-256` | guide | PASS |
| `BiT-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BiT-128` | keygen | 405.1 k | 150 µs | 6.65e+03 | 150 µs | 19210 (5 × 3842) |
| `BiT-128` | sign | 1.76 M | 654 µs | 1.53e+03 | 655 µs | 9335 (5 × 1867) |
| `BiT-128` | verify | 395.0 k | 147 µs | 6.82e+03 | 147 µs | 21010 (5 × 4202) |
| `BiT-256` | keygen | 1.12 M | 415 µs | 2.41e+03 | 415 µs | 7135 (5 × 1427) |
| `BiT-256` | sign | 2.47 M | 918 µs | 1.09e+03 | 918 µs | 2615 (5 × 523) |
| `BiT-256` | verify | 1.15 M | 428 µs | 2.33e+03 | 428 µs | 6825 (5 × 1365) |
| `BiT-512` | keygen | 3.34 M | 1.24 ms | 808 | 1.24 ms | 2480 (5 × 496) |
| `BiT-512` | sign | 10.51 M | 3.9 ms | 256 | 3.9 ms | 950 (5 × 190) |
| `BiT-512` | verify | 3.27 M | 1.21 ms | 824 | 1.21 ms | 2510 (5 × 502) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BiT-128` | keygen | 47868 | 1444 KiB | 1524 KiB |
| `BiT-128` | sign | 47868 | 1460 KiB | 1548 KiB |
| `BiT-128` | verify | 47868 | 1484 KiB | 1548 KiB |
| `BiT-256` | keygen | 49188 | 1452 KiB | 1580 KiB |
| `BiT-256` | sign | 49188 | 1512 KiB | 1664 KiB |
| `BiT-256` | verify | 49188 | 1600 KiB | 1668 KiB |
| `BiT-512` | keygen | 59588 | 1472 KiB | 1668 KiB |
| `BiT-512` | sign | 59588 | 1600 KiB | 1836 KiB |
| `BiT-512` | verify | 59588 | 1772 KiB | 1840 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `BiT-128` | 1048 | 1864 | 1504 |
| `BiT-256` | 2144 | 4160 | 3456 |
| `BiT-512` | 5056 | 9024 | 6695 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable (BIT_USE_SHAKE=0)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BiT-128` | keygen | 78% | 2.1% | drng 2, pseudoXOF 15, sm3hash 1 |
| `BiT-128` | sign | 69% | 0.5% | drng 2, pseudoXOF 69.8, sm3hash 6.95 |
| `BiT-128` | verify | 75% | 0.0% | pseudoXOF 10, sm3hash 3 |
| `BiT-256` | keygen | 84% | 0.8% | drng 2, pseudoXOF 33, pseudohash 1 |
| `BiT-256` | sign | 65% | 0.3% | drng 2, pseudoXOF 56.2, pseudohash 2.88 |
| `BiT-256` | verify | 83% | 0.0% | pseudoXOF 31, pseudohash 3 |
| `BiT-512` | keygen | 89% | 0.3% | drng 2, pseudoXOF 51, pseudohash 1 |
| `BiT-512` | sign | 69% | 0.1% | drng 2, pseudoXOF 96.5, pseudohash 3.91 |
| `BiT-512` | verify | 87% | 0.0% | pseudoXOF 53, pseudohash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BiT-128` | KAT log (sha256 `5ee07f4d92b643c4…`) | `kat/sign-02/BiT-128.log` |
| `BiT-128` | timing keygen | `records/sign-02/BiT-128__keygen.json` |
| `BiT-128` | timing sign | `records/sign-02/BiT-128__sign.json` |
| `BiT-128` | timing verify | `records/sign-02/BiT-128__verify.json` |
| `BiT-128` | hash profile keygen | `profile/sign-02/BiT-128__keygen.json` |
| `BiT-128` | hash profile sign | `profile/sign-02/BiT-128__sign.json` |
| `BiT-128` | hash profile verify | `profile/sign-02/BiT-128__verify.json` |
| `BiT-256` | KAT log (sha256 `c2e728e482808935…`) | `kat/sign-02/BiT-256.log` |
| `BiT-256` | timing keygen | `records/sign-02/BiT-256__keygen.json` |
| `BiT-256` | timing sign | `records/sign-02/BiT-256__sign.json` |
| `BiT-256` | timing verify | `records/sign-02/BiT-256__verify.json` |
| `BiT-256` | hash profile keygen | `profile/sign-02/BiT-256__keygen.json` |
| `BiT-256` | hash profile sign | `profile/sign-02/BiT-256__sign.json` |
| `BiT-256` | hash profile verify | `profile/sign-02/BiT-256__verify.json` |
| `BiT-512` | KAT log (sha256 `49753f44e59d3073…`) | `kat/sign-02/BiT-512.log` |
| `BiT-512` | timing keygen | `records/sign-02/BiT-512__keygen.json` |
| `BiT-512` | timing sign | `records/sign-02/BiT-512__sign.json` |
| `BiT-512` | timing verify | `records/sign-02/BiT-512__verify.json` |
| `BiT-512` | hash profile keygen | `profile/sign-02/BiT-512__keygen.json` |
| `BiT-512` | hash profile sign | `profile/sign-02/BiT-512__sign.json` |
| `BiT-512` | hash profile verify | `profile/sign-02/BiT-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

