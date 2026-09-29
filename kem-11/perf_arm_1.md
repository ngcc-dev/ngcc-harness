# kem-11 COMPASS-KEM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-11` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844283891712.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: COMPASS-KEM
- Implementation versions measured: reference
- Parameter sets: `COMPASS-KEM-128`, `COMPASS-KEM-256`, `COMPASS-KEM-384`, `COMPASS-KEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-11/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `COMPASS-KEM-128` | guide | PASS |
| `COMPASS-KEM-256` | guide | PASS |
| `COMPASS-KEM-384` | guide | PASS |
| `COMPASS-KEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `COMPASS-KEM-128` | keygen | 1.51 M | 561 µs | 1.78e+03 | 561 µs | 5455 (5 × 1091) |
| `COMPASS-KEM-128` | enc | 1.53 M | 568 µs | 1.76e+03 | 569 µs | 5495 (5 × 1099) |
| `COMPASS-KEM-128` | dec | 1.55 M | 577 µs | 1.73e+03 | 577 µs | 5395 (5 × 1079) |
| `COMPASS-KEM-256` | keygen | 5.88 M | 2.18 ms | 459 | 2.18 ms | 1420 (5 × 284) |
| `COMPASS-KEM-256` | enc | 5.90 M | 2.19 ms | 457 | 2.19 ms | 1445 (5 × 289) |
| `COMPASS-KEM-256` | dec | 5.94 M | 2.2 ms | 454 | 2.2 ms | 1435 (5 × 287) |
| `COMPASS-KEM-384` | keygen | 3.53 M | 1.31 ms | 763 | 1.31 ms | 2315 (5 × 463) |
| `COMPASS-KEM-384` | enc | 3.67 M | 1.36 ms | 735 | 1.35 ms | 2325 (5 × 465) |
| `COMPASS-KEM-384` | dec | 3.75 M | 1.39 ms | 719 | 1.39 ms | 2140 (5 × 428) |
| `COMPASS-KEM-512` | keygen | 6.21 M | 2.3 ms | 434 | 2.29 ms | 1365 (5 × 273) |
| `COMPASS-KEM-512` | enc | 6.37 M | 2.36 ms | 423 | 2.34 ms | 1275 (5 × 255) |
| `COMPASS-KEM-512` | dec | 6.46 M | 2.4 ms | 418 | 2.4 ms | 1315 (5 × 263) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `COMPASS-KEM-128` | keygen | 24544 | 1416 KiB | 1496 KiB |
| `COMPASS-KEM-128` | enc | 24544 | 1432 KiB | 1500 KiB |
| `COMPASS-KEM-128` | dec | 24544 | 1432 KiB | 1500 KiB |
| `COMPASS-KEM-256` | keygen | 24976 | 1420 KiB | 1508 KiB |
| `COMPASS-KEM-256` | enc | 24976 | 1444 KiB | 1512 KiB |
| `COMPASS-KEM-256` | dec | 24976 | 1444 KiB | 1512 KiB |
| `COMPASS-KEM-384` | keygen | 26128 | 1424 KiB | 1516 KiB |
| `COMPASS-KEM-384` | enc | 26128 | 1448 KiB | 1520 KiB |
| `COMPASS-KEM-384` | dec | 26128 | 1452 KiB | 1524 KiB |
| `COMPASS-KEM-512` | keygen | 26208 | 1428 KiB | 1536 KiB |
| `COMPASS-KEM-512` | enc | 26208 | 3500 KiB | 3564 KiB |
| `COMPASS-KEM-512` | dec | 26208 | 1472 KiB | 1544 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `COMPASS-KEM-128` | 672 | 1504 | 768 | 32 |
| `COMPASS-KEM-256` | 1312 | 2912 | 1472 | 32 |
| `COMPASS-KEM-384` | 2144 | 4704 | 2432 | 32 |
| `COMPASS-KEM-512` | 2848 | 6240 | 3264 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `COMPASS-KEM-128` | keygen | 97% | 0.4% | drng 1, pseudoXOF 6, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-128` | enc | 96% | 0.3% | drng 1, pseudoXOF 6, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-128` | dec | 94% | 0.0% | pseudoXOF 7, pseudohash 1 |
| `COMPASS-KEM-256` | keygen | 98% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-256` | enc | 97% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-256` | dec | 97% | 0.0% | pseudoXOF 21, pseudohash 1 |
| `COMPASS-KEM-384` | keygen | 93% | 0.2% | drng 1, pseudoXOF 12, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-384` | enc | 91% | 0.1% | drng 1, pseudoXOF 12, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-384` | dec | 88% | 0.0% | pseudoXOF 13, pseudohash 1 |
| `COMPASS-KEM-512` | keygen | 94% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-512` | enc | 93% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-512` | dec | 91% | 0.0% | pseudoXOF 21, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `COMPASS-KEM-128` | KAT log (sha256 `dbc283a62e47ee76…`) | `kat/kem-11/COMPASS-KEM-128.log` |
| `COMPASS-KEM-128` | timing dec | `records/kem-11/COMPASS-KEM-128__dec.json` |
| `COMPASS-KEM-128` | timing enc | `records/kem-11/COMPASS-KEM-128__enc.json` |
| `COMPASS-KEM-128` | timing keygen | `records/kem-11/COMPASS-KEM-128__keygen.json` |
| `COMPASS-KEM-128` | hash profile dec | `profile/kem-11/COMPASS-KEM-128__dec.json` |
| `COMPASS-KEM-128` | hash profile enc | `profile/kem-11/COMPASS-KEM-128__enc.json` |
| `COMPASS-KEM-128` | hash profile keygen | `profile/kem-11/COMPASS-KEM-128__keygen.json` |
| `COMPASS-KEM-256` | KAT log (sha256 `c52543665853268a…`) | `kat/kem-11/COMPASS-KEM-256.log` |
| `COMPASS-KEM-256` | timing dec | `records/kem-11/COMPASS-KEM-256__dec.json` |
| `COMPASS-KEM-256` | timing enc | `records/kem-11/COMPASS-KEM-256__enc.json` |
| `COMPASS-KEM-256` | timing keygen | `records/kem-11/COMPASS-KEM-256__keygen.json` |
| `COMPASS-KEM-256` | hash profile dec | `profile/kem-11/COMPASS-KEM-256__dec.json` |
| `COMPASS-KEM-256` | hash profile enc | `profile/kem-11/COMPASS-KEM-256__enc.json` |
| `COMPASS-KEM-256` | hash profile keygen | `profile/kem-11/COMPASS-KEM-256__keygen.json` |
| `COMPASS-KEM-384` | KAT log (sha256 `b9a5b47f1a5c37f2…`) | `kat/kem-11/COMPASS-KEM-384.log` |
| `COMPASS-KEM-384` | timing dec | `records/kem-11/COMPASS-KEM-384__dec.json` |
| `COMPASS-KEM-384` | timing enc | `records/kem-11/COMPASS-KEM-384__enc.json` |
| `COMPASS-KEM-384` | timing keygen | `records/kem-11/COMPASS-KEM-384__keygen.json` |
| `COMPASS-KEM-384` | hash profile dec | `profile/kem-11/COMPASS-KEM-384__dec.json` |
| `COMPASS-KEM-384` | hash profile enc | `profile/kem-11/COMPASS-KEM-384__enc.json` |
| `COMPASS-KEM-384` | hash profile keygen | `profile/kem-11/COMPASS-KEM-384__keygen.json` |
| `COMPASS-KEM-512` | KAT log (sha256 `eb46faa326ec90ae…`) | `kat/kem-11/COMPASS-KEM-512.log` |
| `COMPASS-KEM-512` | timing dec | `records/kem-11/COMPASS-KEM-512__dec.json` |
| `COMPASS-KEM-512` | timing enc | `records/kem-11/COMPASS-KEM-512__enc.json` |
| `COMPASS-KEM-512` | timing keygen | `records/kem-11/COMPASS-KEM-512__keygen.json` |
| `COMPASS-KEM-512` | hash profile dec | `profile/kem-11/COMPASS-KEM-512__dec.json` |
| `COMPASS-KEM-512` | hash profile enc | `profile/kem-11/COMPASS-KEM-512__enc.json` |
| `COMPASS-KEM-512` | hash profile keygen | `profile/kem-11/COMPASS-KEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

