# sign-23 Shuttle — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-23` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096101515264.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Shuttle
- Implementation versions measured: reference
- Parameter sets: `SHUTTLE-128`, `SHUTTLE-256`, `SHUTTLE-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-23/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `SHUTTLE-128` | guide | PASS |
| `SHUTTLE-256` | guide | PASS |
| `SHUTTLE-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `SHUTTLE-128` | keygen | 8.29 M | 3.08 ms | 325 | 3.07 ms | 2590 (5 × 518) |
| `SHUTTLE-128` | sign | 4.31 M | 1.6 ms | 625 | 1.59 ms | 1890 (5 × 378) |
| `SHUTTLE-128` | verify | 1.05 M | 389 µs | 2.57e+03 | 389 µs | 7835 (5 × 1567) |
| `SHUTTLE-256` | keygen | 8.84 M | 3.28 ms | 305 | 3.28 ms | 645 (5 × 129) |
| `SHUTTLE-256` | sign | 6.85 M | 2.54 ms | 394 | 2.54 ms | 1195 (5 × 239) |
| `SHUTTLE-256` | verify | 1.43 M | 531 µs | 1.88e+03 | 531 µs | 5750 (5 × 1150) |
| `SHUTTLE-512` | keygen | 18.80 M | 6.98 ms | 143 | 6.96 ms | 1010 (5 × 202) |
| `SHUTTLE-512` | sign | 14.10 M | 5.23 ms | 191 | 5.23 ms | 595 (5 × 119) |
| `SHUTTLE-512` | verify | 2.60 M | 964 µs | 1.04e+03 | 964 µs | 3170 (5 × 634) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SHUTTLE-128` | keygen | 77120 | 3488 KiB | 3592 KiB |
| `SHUTTLE-128` | sign | 77120 | 1488 KiB | 1628 KiB |
| `SHUTTLE-128` | verify | 77120 | 1564 KiB | 1648 KiB |
| `SHUTTLE-256` | keygen | 79348 | 1448 KiB | 1580 KiB |
| `SHUTTLE-256` | sign | 79348 | 3544 KiB | 3716 KiB |
| `SHUTTLE-256` | verify | 79348 | 1624 KiB | 1712 KiB |
| `SHUTTLE-512` | keygen | 80096 | 1460 KiB | 1656 KiB |
| `SHUTTLE-512` | sign | 80096 | 1588 KiB | 1860 KiB |
| `SHUTTLE-512` | verify | 80096 | 1796 KiB | 1884 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `SHUTTLE-128` | 1264 | 2288 | 1183 |
| `SHUTTLE-256` | 1952 | 3680 | 2417 |
| `SHUTTLE-512` | 3648 | 7104 | 5001 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — uses the ICCS SM3 DRBG (init/get_random_number) as its XOF; AVX2/AVX-512 SM3 DRBG in optimized

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `SHUTTLE-128` | keygen | 0.0% | 79% | drng 298 |
| `SHUTTLE-128` | sign | 0.0% | 67% | drng 89.9 |
| `SHUTTLE-128` | verify | 0.0% | 56% | drng 20 |
| `SHUTTLE-256` | keygen | 0.0% | 79% | drng 293 |
| `SHUTTLE-256` | sign | 0.0% | 66% | drng 135 |
| `SHUTTLE-256` | verify | 0.0% | 55% | drng 20 |
| `SHUTTLE-512` | keygen | 0.0% | 81% | drng 672 |
| `SHUTTLE-512` | sign | 0.0% | 62% | drng 230 |
| `SHUTTLE-512` | verify | 0.0% | 58% | drng 20 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SHUTTLE-128` | KAT log (sha256 `8214034b6b4a3972…`) | `kat/sign-23/SHUTTLE-128.log` |
| `SHUTTLE-128` | timing keygen | `records/sign-23/SHUTTLE-128__keygen.json` |
| `SHUTTLE-128` | timing sign | `records/sign-23/SHUTTLE-128__sign.json` |
| `SHUTTLE-128` | timing verify | `records/sign-23/SHUTTLE-128__verify.json` |
| `SHUTTLE-128` | hash profile keygen | `profile/sign-23/SHUTTLE-128__keygen.json` |
| `SHUTTLE-128` | hash profile sign | `profile/sign-23/SHUTTLE-128__sign.json` |
| `SHUTTLE-128` | hash profile verify | `profile/sign-23/SHUTTLE-128__verify.json` |
| `SHUTTLE-256` | KAT log (sha256 `d0fd7283f3fd5396…`) | `kat/sign-23/SHUTTLE-256.log` |
| `SHUTTLE-256` | timing keygen | `records/sign-23/SHUTTLE-256__keygen.json` |
| `SHUTTLE-256` | timing sign | `records/sign-23/SHUTTLE-256__sign.json` |
| `SHUTTLE-256` | timing verify | `records/sign-23/SHUTTLE-256__verify.json` |
| `SHUTTLE-256` | hash profile keygen | `profile/sign-23/SHUTTLE-256__keygen.json` |
| `SHUTTLE-256` | hash profile sign | `profile/sign-23/SHUTTLE-256__sign.json` |
| `SHUTTLE-256` | hash profile verify | `profile/sign-23/SHUTTLE-256__verify.json` |
| `SHUTTLE-512` | KAT log (sha256 `175a53a5d59b424d…`) | `kat/sign-23/SHUTTLE-512.log` |
| `SHUTTLE-512` | timing keygen | `records/sign-23/SHUTTLE-512__keygen.json` |
| `SHUTTLE-512` | timing sign | `records/sign-23/SHUTTLE-512__sign.json` |
| `SHUTTLE-512` | timing verify | `records/sign-23/SHUTTLE-512__verify.json` |
| `SHUTTLE-512` | hash profile keygen | `profile/sign-23/SHUTTLE-512__keygen.json` |
| `SHUTTLE-512` | hash profile sign | `profile/sign-23/SHUTTLE-512__sign.json` |
| `SHUTTLE-512` | hash profile verify | `profile/sign-23/SHUTTLE-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

