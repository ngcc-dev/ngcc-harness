# sign-09 DOVE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-09` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077311033344.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: DOVE
- Implementation versions measured: reference
- Parameter sets: `dove_classic_128`, `dove_classic_256`, `dove_classic_512`, `dove_pkc_skc_128`, `dove_pkc_skc_256`, `dove_pkc_skc_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-09/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `dove_classic_128` | guide | PASS |
| `dove_classic_256` | guide | PASS |
| `dove_classic_512` | guide | PASS |
| `dove_pkc_skc_128` | guide | PASS |
| `dove_pkc_skc_256` | guide | PASS |
| `dove_pkc_skc_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `dove_classic_128` | keygen | 101.87 M | 48.7 ms | 20.5 | 48.7 ms | 105 (5 × 21) |
| `dove_classic_128` | sign | 4.72 M | 2.26 ms | 442 | 2.26 ms | 2180 (5 × 436) |
| `dove_classic_128` | verify | 4.36 M | 2.08 ms | 480 | 2.07 ms | 2350 (5 × 470) |
| `dove_classic_256` | keygen | 2.16 G | 1.04 s | 0.965 | 1.04 s | 100 (5 × 20) |
| `dove_classic_256` | sign | 49.56 M | 23.8 ms | 41.9 | 23.8 ms | 210 (5 × 42) |
| `dove_classic_256` | verify | 43.98 M | 21.1 ms | 47.5 | 21 ms | 235 (5 × 47) |
| `dove_classic_512` | keygen | 48.53 G | 23.3 s | 0.0429 | 23.3 s | 35 (5 × 7) |
| `dove_classic_512` | sign | 529.87 M | 255 ms | 3.92 | 255 ms | 100 (5 × 20) |
| `dove_classic_512` | verify | 478.61 M | 232 ms | 4.32 | 230 ms | 100 (5 × 20) |
| `dove_pkc_skc_128` | keygen | 89.58 M | 42.8 ms | 23.4 | 42.8 ms | 120 (5 × 24) |
| `dove_pkc_skc_128` | sign | 27.89 M | 13.3 ms | 75.1 | 13.3 ms | 375 (5 × 75) |
| `dove_pkc_skc_128` | verify | 11.21 M | 5.35 ms | 187 | 5.35 ms | 930 (5 × 186) |
| `dove_pkc_skc_256` | keygen | 2.02 G | 970 ms | 1.03 | 972 ms | 100 (5 × 20) |
| `dove_pkc_skc_256` | sign | 409.22 M | 197 ms | 5.08 | 195 ms | 100 (5 × 20) |
| `dove_pkc_skc_256` | verify | 118.08 M | 56.5 ms | 17.7 | 56.4 ms | 100 (5 × 20) |
| `dove_pkc_skc_512` | keygen | 46.96 G | 22.5 s | 0.0444 | 22.5 s | 35 (5 × 7) |
| `dove_pkc_skc_512` | sign | 6.12 G | 2.94 s | 0.34 | 2.94 s | 100 (5 × 20) |
| `dove_pkc_skc_512` | verify | 1.30 G | 629 ms | 1.59 | 630 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `dove_classic_128` | keygen | 37149 | 1724 KiB | 2276 KiB |
| `dove_classic_128` | sign | 37149 | 2188 KiB | 2260 KiB |
| `dove_classic_128` | verify | 37149 | 2196 KiB | 2260 KiB |
| `dove_classic_256` | keygen | 37373 | 1708 KiB | 6244 KiB |
| `dove_classic_256` | sign | 37373 | 5788 KiB | 6244 KiB |
| `dove_classic_256` | verify | 37373 | 5792 KiB | 5948 KiB |
| `dove_classic_512` | keygen | 37981 | 1724 KiB | 48468 KiB |
| `dove_classic_512` | sign | 37981 | 44000 KiB | 48344 KiB |
| `dove_classic_512` | verify | 37981 | 44032 KiB | 48148 KiB |
| `dove_pkc_skc_128` | keygen | 36861 | 1768 KiB | 1924 KiB |
| `dove_pkc_skc_128` | sign | 36861 | 1840 KiB | 1952 KiB |
| `dove_pkc_skc_128` | verify | 36861 | 1856 KiB | 1920 KiB |
| `dove_pkc_skc_256` | keygen | 37117 | 1724 KiB | 2792 KiB |
| `dove_pkc_skc_256` | sign | 37117 | 2348 KiB | 2916 KiB |
| `dove_pkc_skc_256` | verify | 37117 | 2320 KiB | 2576 KiB |
| `dove_pkc_skc_512` | keygen | 37629 | 1708 KiB | 11364 KiB |
| `dove_pkc_skc_512` | sign | 37629 | 6932 KiB | 11384 KiB |
| `dove_pkc_skc_512` | verify | 37629 | 6968 KiB | 11052 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `dove_classic_128` | 191646 | 192990 | 136 |
| `dove_classic_256` | 2050352 | 1925976 | 296 |
| `dove_classic_512` | 22833052 | 20181508 | 648 |
| `dove_pkc_skc_128` | 43576 | 24 | 136 |
| `dove_pkc_skc_256` | 446992 | 40 | 296 |
| `dove_pkc_skc_512` | 5062192 | 72 | 648 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `dove_classic_128` | keygen | 19% | 0.0% | drng 1, pseudoXOF 140 |
| `dove_classic_128` | sign | 0.4% | 0.0% | pseudoXOF 3 |
| `dove_classic_128` | verify | 0.1% | 0.0% | pseudoXOF 1 |
| `dove_classic_256` | keygen | 10% | 0.0% | drng 1, pseudoXOF 299 |
| `dove_classic_256` | sign | 0.1% | 0.0% | pseudoXOF 3 |
| `dove_classic_256` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `dove_classic_512` | keygen | 5.0% | 0.0% | drng 1, pseudoXOF 664 |
| `dove_classic_512` | sign | 0.0% | 0.0% | pseudoXOF 3 |
| `dove_classic_512` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `dove_pkc_skc_128` | keygen | 7.8% | 0.0% | drng 1, pseudoXOF 52 |
| `dove_pkc_skc_128` | sign | 25% | 0.0% | pseudoXOF 55 |
| `dove_pkc_skc_128` | verify | 61% | 0.0% | pseudoXOF 52 |
| `dove_pkc_skc_256` | keygen | 3.7% | 0.0% | drng 1, pseudoXOF 107 |
| `dove_pkc_skc_256` | sign | 18% | 0.0% | pseudoXOF 110 |
| `dove_pkc_skc_256` | verify | 63% | 0.0% | pseudoXOF 107 |
| `dove_pkc_skc_512` | keygen | 1.8% | 0.0% | drng 1, pseudoXOF 232 |
| `dove_pkc_skc_512` | sign | 14% | 0.0% | pseudoXOF 235 |
| `dove_pkc_skc_512` | verify | 63% | 0.0% | pseudoXOF 232 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `dove_classic_128` | KAT log (sha256 `89d908ec521c5449…`) | `kat/sign-09/dove_classic_128.log` |
| `dove_classic_128` | timing keygen | `records/sign-09/dove_classic_128__keygen.json` |
| `dove_classic_128` | timing sign | `records/sign-09/dove_classic_128__sign.json` |
| `dove_classic_128` | timing verify | `records/sign-09/dove_classic_128__verify.json` |
| `dove_classic_128` | hash profile keygen | `profile/sign-09/dove_classic_128__keygen.json` |
| `dove_classic_128` | hash profile sign | `profile/sign-09/dove_classic_128__sign.json` |
| `dove_classic_128` | hash profile verify | `profile/sign-09/dove_classic_128__verify.json` |
| `dove_classic_256` | KAT log (sha256 `c6190207227a8975…`) | `kat/sign-09/dove_classic_256.log` |
| `dove_classic_256` | timing keygen | `records/sign-09/dove_classic_256__keygen.json` |
| `dove_classic_256` | timing sign | `records/sign-09/dove_classic_256__sign.json` |
| `dove_classic_256` | timing verify | `records/sign-09/dove_classic_256__verify.json` |
| `dove_classic_256` | hash profile keygen | `profile/sign-09/dove_classic_256__keygen.json` |
| `dove_classic_256` | hash profile sign | `profile/sign-09/dove_classic_256__sign.json` |
| `dove_classic_256` | hash profile verify | `profile/sign-09/dove_classic_256__verify.json` |
| `dove_classic_512` | KAT log (sha256 `648b6d6e96f5ee20…`) | `kat/sign-09/dove_classic_512.log` |
| `dove_classic_512` | timing keygen | `records/sign-09/dove_classic_512__keygen.json` |
| `dove_classic_512` | timing sign | `records/sign-09/dove_classic_512__sign.json` |
| `dove_classic_512` | timing verify | `records/sign-09/dove_classic_512__verify.json` |
| `dove_classic_512` | hash profile keygen | `profile/sign-09/dove_classic_512__keygen.json` |
| `dove_classic_512` | hash profile sign | `profile/sign-09/dove_classic_512__sign.json` |
| `dove_classic_512` | hash profile verify | `profile/sign-09/dove_classic_512__verify.json` |
| `dove_pkc_skc_128` | KAT log (sha256 `0bb503ffe73aacc5…`) | `kat/sign-09/dove_pkc_skc_128.log` |
| `dove_pkc_skc_128` | timing keygen | `records/sign-09/dove_pkc_skc_128__keygen.json` |
| `dove_pkc_skc_128` | timing sign | `records/sign-09/dove_pkc_skc_128__sign.json` |
| `dove_pkc_skc_128` | timing verify | `records/sign-09/dove_pkc_skc_128__verify.json` |
| `dove_pkc_skc_128` | hash profile keygen | `profile/sign-09/dove_pkc_skc_128__keygen.json` |
| `dove_pkc_skc_128` | hash profile sign | `profile/sign-09/dove_pkc_skc_128__sign.json` |
| `dove_pkc_skc_128` | hash profile verify | `profile/sign-09/dove_pkc_skc_128__verify.json` |
| `dove_pkc_skc_256` | KAT log (sha256 `74fdb823d2946684…`) | `kat/sign-09/dove_pkc_skc_256.log` |
| `dove_pkc_skc_256` | timing keygen | `records/sign-09/dove_pkc_skc_256__keygen.json` |
| `dove_pkc_skc_256` | timing sign | `records/sign-09/dove_pkc_skc_256__sign.json` |
| `dove_pkc_skc_256` | timing verify | `records/sign-09/dove_pkc_skc_256__verify.json` |
| `dove_pkc_skc_256` | hash profile keygen | `profile/sign-09/dove_pkc_skc_256__keygen.json` |
| `dove_pkc_skc_256` | hash profile sign | `profile/sign-09/dove_pkc_skc_256__sign.json` |
| `dove_pkc_skc_256` | hash profile verify | `profile/sign-09/dove_pkc_skc_256__verify.json` |
| `dove_pkc_skc_512` | KAT log (sha256 `fe782d759aa37f89…`) | `kat/sign-09/dove_pkc_skc_512.log` |
| `dove_pkc_skc_512` | timing keygen | `records/sign-09/dove_pkc_skc_512__keygen.json` |
| `dove_pkc_skc_512` | timing sign | `records/sign-09/dove_pkc_skc_512__sign.json` |
| `dove_pkc_skc_512` | timing verify | `records/sign-09/dove_pkc_skc_512__verify.json` |
| `dove_pkc_skc_512` | hash profile keygen | `profile/sign-09/dove_pkc_skc_512__keygen.json` |
| `dove_pkc_skc_512` | hash profile sign | `profile/sign-09/dove_pkc_skc_512__sign.json` |
| `dove_pkc_skc_512` | hash profile verify | `profile/sign-09/dove_pkc_skc_512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

