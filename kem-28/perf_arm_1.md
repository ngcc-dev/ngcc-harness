# kem-28 OAEP-NTRU — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-28` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560871911772160.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: OAEP-NTRU
- Implementation versions measured: reference
- Parameter sets: `OAEP-NTRU-648`, `OAEP-NTRU-1296`, `OAEP-NTRU-2592`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T02:47:54 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-28/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `OAEP-NTRU-648` | guide | PASS |
| `OAEP-NTRU-1296` | guide | PASS |
| `OAEP-NTRU-2592` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `OAEP-NTRU-648` | keygen | 138.5 k | 51.4 µs | 1.95e+04 | 51.4 µs | 54795 (5 × 10959) |
| `OAEP-NTRU-648` | enc | 96.2 k | 35.7 µs | 2.8e+04 | 35.6 µs | 83335 (5 × 16667) |
| `OAEP-NTRU-648` | dec | 74.9 k | 27.8 µs | 3.6e+04 | 27.8 µs | 100000 (5 × 20000) |
| `OAEP-NTRU-1296` | keygen | 359.4 k | 133 µs | 7.5e+03 | 133 µs | 22400 (5 × 4480) |
| `OAEP-NTRU-1296` | enc | 321.7 k | 119 µs | 8.37e+03 | 119 µs | 25935 (5 × 5187) |
| `OAEP-NTRU-1296` | dec | 216.0 k | 80.2 µs | 1.25e+04 | 80.2 µs | 38370 (5 × 7674) |
| `OAEP-NTRU-2592` | keygen | 872.7 k | 324 µs | 3.09e+03 | 323 µs | 9130 (5 × 1826) |
| `OAEP-NTRU-2592` | enc | 873.4 k | 324 µs | 3.09e+03 | 323 µs | 9725 (5 × 1945) |
| `OAEP-NTRU-2592` | dec | 661.7 k | 246 µs | 4.07e+03 | 246 µs | 12780 (5 × 2556) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `OAEP-NTRU-648` | keygen | 27736 | 1420 KiB | 1488 KiB |
| `OAEP-NTRU-648` | enc | 27736 | 1428 KiB | 1492 KiB |
| `OAEP-NTRU-648` | dec | 27736 | 1428 KiB | 1492 KiB |
| `OAEP-NTRU-1296` | keygen | 27328 | 1428 KiB | 1508 KiB |
| `OAEP-NTRU-1296` | enc | 27328 | 1440 KiB | 1508 KiB |
| `OAEP-NTRU-1296` | dec | 27328 | 1444 KiB | 1508 KiB |
| `OAEP-NTRU-2592` | keygen | 28980 | 1432 KiB | 1544 KiB |
| `OAEP-NTRU-2592` | enc | 28980 | 1472 KiB | 1544 KiB |
| `OAEP-NTRU-2592` | dec | 28980 | 1476 KiB | 1540 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `OAEP-NTRU-648` | 1053 | 2138 | 1085 | 32 |
| `OAEP-NTRU-1296` | 2430 | 4924 | 2494 | 32 |
| `OAEP-NTRU-2592` | 4860 | 9848 | 4988 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `OAEP-NTRU-648` | keygen | 16% | 16% | drng 2, sm3hash 1 |
| `OAEP-NTRU-648` | enc | 63% | 12% | drng 1, pseudoXOF 2, sm3hash 1 |
| `OAEP-NTRU-648` | dec | 52% | 0.0% | pseudoXOF 2 |
| `OAEP-NTRU-1296` | keygen | 30% | 10% | drng 2, pseudohash 1 |
| `OAEP-NTRU-1296` | enc | 74% | 5.6% | drng 1, pseudoXOF 2, pseudohash 1 |
| `OAEP-NTRU-1296` | dec | 61% | 0.0% | pseudoXOF 2 |
| `OAEP-NTRU-2592` | keygen | 26% | 7.3% | drng 2, pseudohash 1 |
| `OAEP-NTRU-2592` | enc | 80% | 3.7% | drng 1, pseudoXOF 2, pseudohash 1 |
| `OAEP-NTRU-2592` | dec | 71% | 0.0% | pseudoXOF 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `OAEP-NTRU-648` | KAT log (sha256 `9fe3f4bb99d56fe5…`) | `kat/kem-28/OAEP-NTRU-648.log` |
| `OAEP-NTRU-648` | timing dec | `records/kem-28/OAEP-NTRU-648__dec.json` |
| `OAEP-NTRU-648` | timing enc | `records/kem-28/OAEP-NTRU-648__enc.json` |
| `OAEP-NTRU-648` | timing keygen | `records/kem-28/OAEP-NTRU-648__keygen.json` |
| `OAEP-NTRU-648` | hash profile dec | `profile/kem-28/OAEP-NTRU-648__dec.json` |
| `OAEP-NTRU-648` | hash profile enc | `profile/kem-28/OAEP-NTRU-648__enc.json` |
| `OAEP-NTRU-648` | hash profile keygen | `profile/kem-28/OAEP-NTRU-648__keygen.json` |
| `OAEP-NTRU-1296` | KAT log (sha256 `7116950926d4b7bb…`) | `kat/kem-28/OAEP-NTRU-1296.log` |
| `OAEP-NTRU-1296` | timing dec | `records/kem-28/OAEP-NTRU-1296__dec.json` |
| `OAEP-NTRU-1296` | timing enc | `records/kem-28/OAEP-NTRU-1296__enc.json` |
| `OAEP-NTRU-1296` | timing keygen | `records/kem-28/OAEP-NTRU-1296__keygen.json` |
| `OAEP-NTRU-1296` | hash profile dec | `profile/kem-28/OAEP-NTRU-1296__dec.json` |
| `OAEP-NTRU-1296` | hash profile enc | `profile/kem-28/OAEP-NTRU-1296__enc.json` |
| `OAEP-NTRU-1296` | hash profile keygen | `profile/kem-28/OAEP-NTRU-1296__keygen.json` |
| `OAEP-NTRU-2592` | KAT log (sha256 `e9d8de109288ad8a…`) | `kat/kem-28/OAEP-NTRU-2592.log` |
| `OAEP-NTRU-2592` | timing dec | `records/kem-28/OAEP-NTRU-2592__dec.json` |
| `OAEP-NTRU-2592` | timing enc | `records/kem-28/OAEP-NTRU-2592__enc.json` |
| `OAEP-NTRU-2592` | timing keygen | `records/kem-28/OAEP-NTRU-2592__keygen.json` |
| `OAEP-NTRU-2592` | hash profile dec | `profile/kem-28/OAEP-NTRU-2592__dec.json` |
| `OAEP-NTRU-2592` | hash profile enc | `profile/kem-28/OAEP-NTRU-2592__enc.json` |
| `OAEP-NTRU-2592` | hash profile keygen | `profile/kem-28/OAEP-NTRU-2592__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

