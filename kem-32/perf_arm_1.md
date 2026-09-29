# kem-32 Quasi-Cyclic Twisted McEliece Key Encapsulation Mechanism — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-32` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872431865856.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Quasi-Cyclic Twisted McEliece Key Encapsulation Mechanism
- Implementation versions measured: reference
- Parameter sets: `QCTM128`, `QCTM256`, `QCTM512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-32/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `QCTM128` | guide | PASS |
| `QCTM256` | guide | PASS |
| `QCTM512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `QCTM128` | keygen | 55.69 G | 20.7 s | 0.0484 | 20.8 s | 40 (5 × 8) |
| `QCTM128` | enc | 13.43 M | 4.98 ms | 201 | 4.97 ms | 850 (5 × 170) |
| `QCTM128` | dec | 4.98 G | 1.85 s | 0.542 | 1.85 s | 100 (5 × 20) |
| `QCTM256` | keygen | 119.50 G | 44.4 s | 0.0225 | 44.7 s | 5 (5 × 1) |
| `QCTM256` | enc | 43.27 M | 16.1 ms | 62.3 | 16.1 ms | 205 (5 × 41) |
| `QCTM256` | dec | 29.85 G | 11.1 s | 0.0902 | 11.1 s | 30 (5 × 6) |
| `QCTM512` | keygen | 697.86 G | 259 s | 0.00386 | 259 s | 1 (1 × 1) |
| `QCTM512` | enc | 163.01 M | 60.5 ms | 16.5 | 60.3 ms | 100 (5 × 20) |
| `QCTM512` | dec | 201.48 G | 74.8 s | 0.0134 | 75.2 s | 4 (4 × 1) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `QCTM128` | keygen | 92252 | 3612 KiB | 148700 KiB |
| `QCTM128` | enc | 92252 | 11392 KiB | 148560 KiB |
| `QCTM128` | dec | 92252 | 14620 KiB | 148848 KiB |
| `QCTM256` | keygen | 90508 | 5352 KiB | 474076 KiB |
| `QCTM256` | enc | 90508 | 13580 KiB | 474248 KiB |
| `QCTM256` | dec | 90508 | 23852 KiB | 474344 KiB |
| `QCTM512` | keygen | 91716 | 3272 KiB | 1827828 KiB |
| `QCTM512` | enc | 91716 | 15324 KiB | 1827592 KiB |
| `QCTM512` | dec | 91716 | 15896 KiB | 1826112 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `QCTM128` | 167987 | 192545 | 640 | 32 |
| `QCTM256` | 595662 | 641942 | 1153 | 32 |
| `QCTM512` | 2374727 | 2467243 | 2264 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — own SHAKE256 for session key/keygen expansion; OpenSSL AES-256-CTR-DRBG seeded by the ICCS DRNG; auxfunc not linked

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `QCTM128` | keygen | 0.0% | 0.0% | drng 1 |
| `QCTM128` | enc | 0.0% | 0.0% | – |
| `QCTM128` | dec | 0.0% | 0.0% | – |
| `QCTM256` | keygen | 0.0% | 0.0% | drng 1 |
| `QCTM256` | enc | 0.0% | 0.0% | – |
| `QCTM256` | dec | 0.0% | 0.0% | – |
| `QCTM512` | keygen | 0.0% | 0.0% | drng 1 |
| `QCTM512` | enc | 0.0% | 0.0% | – |
| `QCTM512` | dec | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `QCTM128` | KAT log (sha256 `2a8169ca8ae22343…`) | `kat/kem-32/QCTM128.log` |
| `QCTM128` | timing dec | `records/kem-32/QCTM128__dec.json` |
| `QCTM128` | timing enc | `records/kem-32/QCTM128__enc.json` |
| `QCTM128` | timing keygen | `records/kem-32/QCTM128__keygen.json` |
| `QCTM128` | hash profile dec | `profile/kem-32/QCTM128__dec.json` |
| `QCTM128` | hash profile enc | `profile/kem-32/QCTM128__enc.json` |
| `QCTM128` | hash profile keygen | `profile/kem-32/QCTM128__keygen.json` |
| `QCTM256` | KAT log (sha256 `aa2c8443858ae876…`) | `kat/kem-32/QCTM256.log` |
| `QCTM256` | timing dec | `records/kem-32/QCTM256__dec.json` |
| `QCTM256` | timing enc | `records/kem-32/QCTM256__enc.json` |
| `QCTM256` | timing keygen | `records/kem-32/QCTM256__keygen.json` |
| `QCTM256` | hash profile dec | `profile/kem-32/QCTM256__dec.json` |
| `QCTM256` | hash profile enc | `profile/kem-32/QCTM256__enc.json` |
| `QCTM256` | hash profile keygen | `profile/kem-32/QCTM256__keygen.json` |
| `QCTM512` | KAT log (sha256 `8162573769a321d8…`) | `kat/kem-32/QCTM512.log` |
| `QCTM512` | timing dec | `records/kem-32/QCTM512__dec.json` |
| `QCTM512` | timing enc | `records/kem-32/QCTM512__enc.json` |
| `QCTM512` | timing keygen | `records/kem-32/QCTM512__keygen.json` |
| `QCTM512` | hash profile dec | `profile/kem-32/QCTM512__dec.json` |
| `QCTM512` | hash profile enc | `profile/kem-32/QCTM512__enc.json` |
| `QCTM512` | hash profile keygen | `profile/kem-32/QCTM512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

