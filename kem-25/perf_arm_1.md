# kem-25 NEV — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-25` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560863040819200.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: NEV
- Implementation versions measured: reference
- Parameter sets: `NEV_512_769_C_ICCS`, `NEV_512_769_ICCS`, `NEV_512_1409_ICCS`, `NEV_512_3329_ICCS`, `NEV_1024_769_C_ICCS`, `NEV_1024_769_ICCS`, `NEV_1024_1409_ICCS`, `NEV_1024_3329_ICCS`, `NEV_2048_769_C_ICCS`, `NEV_2048_769_ICCS`, `NEV_2048_1409_ICCS`, `NEV_2048_3329_ICCS`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-25/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NEV_512_769_C_ICCS` | guide | PASS |
| `NEV_512_769_ICCS` | guide | PASS |
| `NEV_512_1409_ICCS` | guide | PASS |
| `NEV_512_3329_ICCS` | guide | PASS |
| `NEV_1024_769_C_ICCS` | guide | PASS |
| `NEV_1024_769_ICCS` | guide | PASS |
| `NEV_1024_1409_ICCS` | guide | PASS |
| `NEV_1024_3329_ICCS` | guide | PASS |
| `NEV_2048_769_C_ICCS` | guide | PASS |
| `NEV_2048_769_ICCS` | guide | PASS |
| `NEV_2048_1409_ICCS` | guide | PASS |
| `NEV_2048_3329_ICCS` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NEV_512_769_C_ICCS` | keygen | 68.3 k | 25.3 µs | 3.95e+04 | 25.3 µs | 96195 (5 × 19239) |
| `NEV_512_769_C_ICCS` | enc | 51.0 k | 18.9 µs | 5.28e+04 | 18.9 µs | 100000 (5 × 20000) |
| `NEV_512_769_C_ICCS` | dec | 55.3 k | 20.5 µs | 4.88e+04 | 20.4 µs | 100000 (5 × 20000) |
| `NEV_512_769_ICCS` | keygen | 68.0 k | 25.2 µs | 3.97e+04 | 25.2 µs | 100000 (5 × 20000) |
| `NEV_512_769_ICCS` | enc | 57.1 k | 21.2 µs | 4.72e+04 | 21.2 µs | 100000 (5 × 20000) |
| `NEV_512_769_ICCS` | dec | 58.0 k | 21.5 µs | 4.64e+04 | 21.5 µs | 100000 (5 × 20000) |
| `NEV_512_1409_ICCS` | keygen | 95.8 k | 35.5 µs | 2.81e+04 | 35.5 µs | 61940 (5 × 12388) |
| `NEV_512_1409_ICCS` | enc | 71.5 k | 26.6 µs | 3.77e+04 | 26.5 µs | 100000 (5 × 20000) |
| `NEV_512_1409_ICCS` | dec | 67.5 k | 25 µs | 4e+04 | 25 µs | 100000 (5 × 20000) |
| `NEV_512_3329_ICCS` | keygen | 136.6 k | 50.7 µs | 1.97e+04 | 50.6 µs | 53275 (5 × 10655) |
| `NEV_512_3329_ICCS` | enc | 121.4 k | 45 µs | 2.22e+04 | 45 µs | 67230 (5 × 13446) |
| `NEV_512_3329_ICCS` | dec | 114.3 k | 42.5 µs | 2.35e+04 | 42.3 µs | 71115 (5 × 14223) |
| `NEV_1024_769_C_ICCS` | keygen | 147.5 k | 54.8 µs | 1.83e+04 | 54.8 µs | 49235 (5 × 9847) |
| `NEV_1024_769_C_ICCS` | enc | 109.1 k | 40.5 µs | 2.47e+04 | 40.5 µs | 62995 (5 × 12599) |
| `NEV_1024_769_C_ICCS` | dec | 125.3 k | 46.5 µs | 2.15e+04 | 46.5 µs | 65575 (5 × 13115) |
| `NEV_1024_769_ICCS` | keygen | 148.1 k | 54.9 µs | 1.82e+04 | 54.8 µs | 43560 (5 × 8712) |
| `NEV_1024_769_ICCS` | enc | 118.6 k | 44 µs | 2.27e+04 | 44 µs | 68970 (5 × 13794) |
| `NEV_1024_769_ICCS` | dec | 128.1 k | 47.5 µs | 2.1e+04 | 47.5 µs | 64005 (5 × 12801) |
| `NEV_1024_1409_ICCS` | keygen | 189.6 k | 70.4 µs | 1.42e+04 | 70.4 µs | 37070 (5 × 7414) |
| `NEV_1024_1409_ICCS` | enc | 130.1 k | 48.3 µs | 2.07e+04 | 48.1 µs | 63325 (5 × 12665) |
| `NEV_1024_1409_ICCS` | dec | 133.5 k | 49.5 µs | 2.02e+04 | 49.3 µs | 61070 (5 × 12214) |
| `NEV_1024_3329_ICCS` | keygen | 220.0 k | 81.6 µs | 1.23e+04 | 81.5 µs | 34095 (5 × 6819) |
| `NEV_1024_3329_ICCS` | enc | 178.7 k | 66.3 µs | 1.51e+04 | 66.1 µs | 45330 (5 × 9066) |
| `NEV_1024_3329_ICCS` | dec | 174.2 k | 64.6 µs | 1.55e+04 | 64.7 µs | 47200 (5 × 9440) |
| `NEV_2048_769_C_ICCS` | keygen | 486.2 k | 180 µs | 5.54e+03 | 180 µs | 15145 (5 × 3029) |
| `NEV_2048_769_C_ICCS` | enc | 293.0 k | 109 µs | 9.2e+03 | 108 µs | 28240 (5 × 5648) |
| `NEV_2048_769_C_ICCS` | dec | 287.7 k | 107 µs | 9.37e+03 | 107 µs | 28780 (5 × 5756) |
| `NEV_2048_769_ICCS` | keygen | 487.9 k | 181 µs | 5.52e+03 | 181 µs | 15740 (5 × 3148) |
| `NEV_2048_769_ICCS` | enc | 326.1 k | 121 µs | 8.26e+03 | 121 µs | 23600 (5 × 4720) |
| `NEV_2048_769_ICCS` | dec | 306.4 k | 114 µs | 8.79e+03 | 114 µs | 26760 (5 × 5352) |
| `NEV_2048_1409_ICCS` | keygen | 526.7 k | 195 µs | 5.12e+03 | 195 µs | 14280 (5 × 2856) |
| `NEV_2048_1409_ICCS` | enc | 387.4 k | 144 µs | 6.95e+03 | 144 µs | 20465 (5 × 4093) |
| `NEV_2048_1409_ICCS` | dec | 357.8 k | 133 µs | 7.53e+03 | 133 µs | 23465 (5 × 4693) |
| `NEV_2048_3329_ICCS` | keygen | 597.8 k | 222 µs | 4.51e+03 | 222 µs | 12945 (5 × 2589) |
| `NEV_2048_3329_ICCS` | enc | 476.6 k | 177 µs | 5.65e+03 | 177 µs | 17605 (5 × 3521) |
| `NEV_2048_3329_ICCS` | dec | 415.5 k | 154 µs | 6.49e+03 | 154 µs | 19255 (5 × 3851) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NEV_512_769_C_ICCS` | keygen | 36456 | 1428 KiB | 1492 KiB |
| `NEV_512_769_C_ICCS` | enc | 36456 | 1428 KiB | 1492 KiB |
| `NEV_512_769_C_ICCS` | dec | 36456 | 1432 KiB | 1496 KiB |
| `NEV_512_769_ICCS` | keygen | 36472 | 1428 KiB | 1492 KiB |
| `NEV_512_769_ICCS` | enc | 36472 | 1432 KiB | 1496 KiB |
| `NEV_512_769_ICCS` | dec | 36472 | 1432 KiB | 1496 KiB |
| `NEV_512_1409_ICCS` | keygen | 38720 | 1432 KiB | 1500 KiB |
| `NEV_512_1409_ICCS` | enc | 38720 | 1436 KiB | 1500 KiB |
| `NEV_512_1409_ICCS` | dec | 38720 | 1436 KiB | 1500 KiB |
| `NEV_512_3329_ICCS` | keygen | 37328 | 1428 KiB | 1500 KiB |
| `NEV_512_3329_ICCS` | enc | 37328 | 1436 KiB | 1500 KiB |
| `NEV_512_3329_ICCS` | dec | 37328 | 1436 KiB | 1500 KiB |
| `NEV_1024_769_C_ICCS` | keygen | 39224 | 1436 KiB | 1512 KiB |
| `NEV_1024_769_C_ICCS` | enc | 39224 | 1444 KiB | 1508 KiB |
| `NEV_1024_769_C_ICCS` | dec | 39224 | 1448 KiB | 1512 KiB |
| `NEV_1024_769_ICCS` | keygen | 39240 | 1436 KiB | 1512 KiB |
| `NEV_1024_769_ICCS` | enc | 39240 | 1448 KiB | 1512 KiB |
| `NEV_1024_769_ICCS` | dec | 39240 | 1448 KiB | 1512 KiB |
| `NEV_1024_1409_ICCS` | keygen | 42232 | 1436 KiB | 1516 KiB |
| `NEV_1024_1409_ICCS` | enc | 42232 | 1452 KiB | 1516 KiB |
| `NEV_1024_1409_ICCS` | dec | 42232 | 1452 KiB | 1516 KiB |
| `NEV_1024_3329_ICCS` | keygen | 39280 | 1436 KiB | 1512 KiB |
| `NEV_1024_3329_ICCS` | enc | 39280 | 1448 KiB | 1512 KiB |
| `NEV_1024_3329_ICCS` | dec | 39280 | 1448 KiB | 1512 KiB |
| `NEV_2048_769_C_ICCS` | keygen | 55088 | 1452 KiB | 1552 KiB |
| `NEV_2048_769_C_ICCS` | enc | 55088 | 1484 KiB | 1552 KiB |
| `NEV_2048_769_C_ICCS` | dec | 55088 | 1488 KiB | 1552 KiB |
| `NEV_2048_769_ICCS` | keygen | 42816 | 1444 KiB | 1548 KiB |
| `NEV_2048_769_ICCS` | enc | 42816 | 1476 KiB | 1544 KiB |
| `NEV_2048_769_ICCS` | dec | 42816 | 1476 KiB | 1540 KiB |
| `NEV_2048_1409_ICCS` | keygen | 50688 | 1448 KiB | 1560 KiB |
| `NEV_2048_1409_ICCS` | enc | 50688 | 1488 KiB | 1560 KiB |
| `NEV_2048_1409_ICCS` | dec | 50688 | 1492 KiB | 1556 KiB |
| `NEV_2048_3329_ICCS` | keygen | 42952 | 1444 KiB | 1548 KiB |
| `NEV_2048_3329_ICCS` | enc | 42952 | 1480 KiB | 1548 KiB |
| `NEV_2048_3329_ICCS` | dec | 42952 | 1480 KiB | 1544 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `NEV_512_769_C_ICCS` | 615 | 1246 | 512 | 16 |
| `NEV_512_769_ICCS` | 615 | 1246 | 615 | 16 |
| `NEV_512_1409_ICCS` | 672 | 1360 | 672 | 16 |
| `NEV_512_3329_ICCS` | 768 | 1552 | 768 | 16 |
| `NEV_1024_769_C_ICCS` | 1229 | 2490 | 1024 | 32 |
| `NEV_1024_769_ICCS` | 1229 | 2490 | 1229 | 32 |
| `NEV_1024_1409_ICCS` | 1344 | 2720 | 1344 | 32 |
| `NEV_1024_3329_ICCS` | 1536 | 3104 | 1536 | 32 |
| `NEV_2048_769_C_ICCS` | 2458 | 4980 | 2048 | 64 |
| `NEV_2048_769_ICCS` | 2458 | 4980 | 2458 | 64 |
| `NEV_2048_1409_ICCS` | 2688 | 5440 | 2688 | 64 |
| `NEV_2048_3329_ICCS` | 3072 | 6208 | 3072 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — with -DUSE_ICCS (SHA3 build selectable)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `NEV_512_769_C_ICCS` | keygen | 45% | 6.2% | drng 1, pseudoXOF 4 |
| `NEV_512_769_C_ICCS` | enc | 49% | 8.3% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NEV_512_769_C_ICCS` | dec | 23% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `NEV_512_769_ICCS` | keygen | 45% | 6.2% | drng 1, pseudoXOF 4 |
| `NEV_512_769_ICCS` | enc | 56% | 7.2% | drng 1, pseudoXOF 2, sm3hash 6 |
| `NEV_512_769_ICCS` | dec | 33% | 0.0% | pseudoXOF 1, sm3hash 6 |
| `NEV_512_1409_ICCS` | keygen | 51% | 4.7% | drng 1, pseudoXOF 4 |
| `NEV_512_1409_ICCS` | enc | 68% | 5.9% | drng 1, pseudoXOF 3, sm3hash 1 |
| `NEV_512_1409_ICCS` | dec | 51% | 0.0% | pseudoXOF 2, sm3hash 1 |
| `NEV_512_3329_ICCS` | keygen | 70% | 3.1% | drng 1, pseudoXOF 4 |
| `NEV_512_3329_ICCS` | enc | 78% | 3.6% | drng 1, pseudoXOF 3, sm3hash 1 |
| `NEV_512_3329_ICCS` | dec | 69% | 0.0% | pseudoXOF 2, sm3hash 1 |
| `NEV_1024_769_C_ICCS` | keygen | 38% | 2.9% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_769_C_ICCS` | enc | 54% | 3.9% | drng 1, pseudoXOF 1, pseudohash 1, sm3hash 1 |
| `NEV_1024_769_C_ICCS` | dec | 26% | 0.0% | pseudoXOF 1, pseudohash 1 |
| `NEV_1024_769_ICCS` | keygen | 37% | 2.9% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_769_ICCS` | enc | 58% | 3.6% | drng 1, pseudoXOF 1, pseudohash 1, sm3hash 9 |
| `NEV_1024_769_ICCS` | dec | 34% | 0.0% | pseudoXOF 1, pseudohash 1, sm3hash 8 |
| `NEV_1024_1409_ICCS` | keygen | 40% | 2.2% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_1409_ICCS` | enc | 64% | 3.3% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `NEV_1024_1409_ICCS` | dec | 42% | 0.0% | pseudoXOF 2, pseudohash 1 |
| `NEV_1024_3329_ICCS` | keygen | 56% | 1.9% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_3329_ICCS` | enc | 74% | 2.4% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `NEV_1024_3329_ICCS` | dec | 58% | 0.0% | pseudoXOF 2, pseudohash 1 |
| `NEV_2048_769_C_ICCS` | keygen | 51% | 1.2% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_769_C_ICCS` | enc | 62% | 1.9% | drng 1, pseudoXOF 1, pseudohash 2 |
| `NEV_2048_769_C_ICCS` | dec | 26% | 0.0% | pseudoXOF 1, pseudohash 1 |
| `NEV_2048_769_ICCS` | keygen | 51% | 1.2% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_769_ICCS` | enc | 66% | 1.7% | drng 1, pseudoXOF 1, pseudohash 2, sm3hash 14 |
| `NEV_2048_769_ICCS` | dec | 36% | 0.0% | pseudoXOF 1, pseudohash 1, sm3hash 14 |
| `NEV_2048_1409_ICCS` | keygen | 39% | 1.1% | drng 1, pseudohash 2, sm3hash 28 |
| `NEV_2048_1409_ICCS` | enc | 70% | 1.4% | drng 1, pseudoXOF 1, pseudohash 2, sm3hash 14 |
| `NEV_2048_1409_ICCS` | dec | 43% | 0.0% | pseudoXOF 1, pseudohash 1, sm3hash 14 |
| `NEV_2048_3329_ICCS` | keygen | 60% | 0.9% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_3329_ICCS` | enc | 80% | 1.2% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_3329_ICCS` | dec | 59% | 0.0% | pseudoXOF 2, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NEV_512_769_C_ICCS` | KAT log (sha256 `785bd8bff0670b89…`) | `kat/kem-25/NEV_512_769_C_ICCS.log` |
| `NEV_512_769_C_ICCS` | timing dec | `records/kem-25/NEV_512_769_C_ICCS__dec.json` |
| `NEV_512_769_C_ICCS` | timing enc | `records/kem-25/NEV_512_769_C_ICCS__enc.json` |
| `NEV_512_769_C_ICCS` | timing keygen | `records/kem-25/NEV_512_769_C_ICCS__keygen.json` |
| `NEV_512_769_C_ICCS` | hash profile dec | `profile/kem-25/NEV_512_769_C_ICCS__dec.json` |
| `NEV_512_769_C_ICCS` | hash profile enc | `profile/kem-25/NEV_512_769_C_ICCS__enc.json` |
| `NEV_512_769_C_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_769_C_ICCS__keygen.json` |
| `NEV_512_769_ICCS` | KAT log (sha256 `99a7175ecc914b75…`) | `kat/kem-25/NEV_512_769_ICCS.log` |
| `NEV_512_769_ICCS` | timing dec | `records/kem-25/NEV_512_769_ICCS__dec.json` |
| `NEV_512_769_ICCS` | timing enc | `records/kem-25/NEV_512_769_ICCS__enc.json` |
| `NEV_512_769_ICCS` | timing keygen | `records/kem-25/NEV_512_769_ICCS__keygen.json` |
| `NEV_512_769_ICCS` | hash profile dec | `profile/kem-25/NEV_512_769_ICCS__dec.json` |
| `NEV_512_769_ICCS` | hash profile enc | `profile/kem-25/NEV_512_769_ICCS__enc.json` |
| `NEV_512_769_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_769_ICCS__keygen.json` |
| `NEV_512_1409_ICCS` | KAT log (sha256 `de4d6d6a72313749…`) | `kat/kem-25/NEV_512_1409_ICCS.log` |
| `NEV_512_1409_ICCS` | timing dec | `records/kem-25/NEV_512_1409_ICCS__dec.json` |
| `NEV_512_1409_ICCS` | timing enc | `records/kem-25/NEV_512_1409_ICCS__enc.json` |
| `NEV_512_1409_ICCS` | timing keygen | `records/kem-25/NEV_512_1409_ICCS__keygen.json` |
| `NEV_512_1409_ICCS` | hash profile dec | `profile/kem-25/NEV_512_1409_ICCS__dec.json` |
| `NEV_512_1409_ICCS` | hash profile enc | `profile/kem-25/NEV_512_1409_ICCS__enc.json` |
| `NEV_512_1409_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_1409_ICCS__keygen.json` |
| `NEV_512_3329_ICCS` | KAT log (sha256 `281d5370cee472d4…`) | `kat/kem-25/NEV_512_3329_ICCS.log` |
| `NEV_512_3329_ICCS` | timing dec | `records/kem-25/NEV_512_3329_ICCS__dec.json` |
| `NEV_512_3329_ICCS` | timing enc | `records/kem-25/NEV_512_3329_ICCS__enc.json` |
| `NEV_512_3329_ICCS` | timing keygen | `records/kem-25/NEV_512_3329_ICCS__keygen.json` |
| `NEV_512_3329_ICCS` | hash profile dec | `profile/kem-25/NEV_512_3329_ICCS__dec.json` |
| `NEV_512_3329_ICCS` | hash profile enc | `profile/kem-25/NEV_512_3329_ICCS__enc.json` |
| `NEV_512_3329_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_3329_ICCS__keygen.json` |
| `NEV_1024_769_C_ICCS` | KAT log (sha256 `c1106c8c1cf618b3…`) | `kat/kem-25/NEV_1024_769_C_ICCS.log` |
| `NEV_1024_769_C_ICCS` | timing dec | `records/kem-25/NEV_1024_769_C_ICCS__dec.json` |
| `NEV_1024_769_C_ICCS` | timing enc | `records/kem-25/NEV_1024_769_C_ICCS__enc.json` |
| `NEV_1024_769_C_ICCS` | timing keygen | `records/kem-25/NEV_1024_769_C_ICCS__keygen.json` |
| `NEV_1024_769_C_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_769_C_ICCS__dec.json` |
| `NEV_1024_769_C_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_769_C_ICCS__enc.json` |
| `NEV_1024_769_C_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_769_C_ICCS__keygen.json` |
| `NEV_1024_769_ICCS` | KAT log (sha256 `566173a4f1ab8fde…`) | `kat/kem-25/NEV_1024_769_ICCS.log` |
| `NEV_1024_769_ICCS` | timing dec | `records/kem-25/NEV_1024_769_ICCS__dec.json` |
| `NEV_1024_769_ICCS` | timing enc | `records/kem-25/NEV_1024_769_ICCS__enc.json` |
| `NEV_1024_769_ICCS` | timing keygen | `records/kem-25/NEV_1024_769_ICCS__keygen.json` |
| `NEV_1024_769_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_769_ICCS__dec.json` |
| `NEV_1024_769_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_769_ICCS__enc.json` |
| `NEV_1024_769_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_769_ICCS__keygen.json` |
| `NEV_1024_1409_ICCS` | KAT log (sha256 `b97a8aaf469cd6ea…`) | `kat/kem-25/NEV_1024_1409_ICCS.log` |
| `NEV_1024_1409_ICCS` | timing dec | `records/kem-25/NEV_1024_1409_ICCS__dec.json` |
| `NEV_1024_1409_ICCS` | timing enc | `records/kem-25/NEV_1024_1409_ICCS__enc.json` |
| `NEV_1024_1409_ICCS` | timing keygen | `records/kem-25/NEV_1024_1409_ICCS__keygen.json` |
| `NEV_1024_1409_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_1409_ICCS__dec.json` |
| `NEV_1024_1409_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_1409_ICCS__enc.json` |
| `NEV_1024_1409_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_1409_ICCS__keygen.json` |
| `NEV_1024_3329_ICCS` | KAT log (sha256 `0b1d88badb802379…`) | `kat/kem-25/NEV_1024_3329_ICCS.log` |
| `NEV_1024_3329_ICCS` | timing dec | `records/kem-25/NEV_1024_3329_ICCS__dec.json` |
| `NEV_1024_3329_ICCS` | timing enc | `records/kem-25/NEV_1024_3329_ICCS__enc.json` |
| `NEV_1024_3329_ICCS` | timing keygen | `records/kem-25/NEV_1024_3329_ICCS__keygen.json` |
| `NEV_1024_3329_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_3329_ICCS__dec.json` |
| `NEV_1024_3329_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_3329_ICCS__enc.json` |
| `NEV_1024_3329_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_3329_ICCS__keygen.json` |
| `NEV_2048_769_C_ICCS` | KAT log (sha256 `73d84b68911abd87…`) | `kat/kem-25/NEV_2048_769_C_ICCS.log` |
| `NEV_2048_769_C_ICCS` | timing dec | `records/kem-25/NEV_2048_769_C_ICCS__dec.json` |
| `NEV_2048_769_C_ICCS` | timing enc | `records/kem-25/NEV_2048_769_C_ICCS__enc.json` |
| `NEV_2048_769_C_ICCS` | timing keygen | `records/kem-25/NEV_2048_769_C_ICCS__keygen.json` |
| `NEV_2048_769_C_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_769_C_ICCS__dec.json` |
| `NEV_2048_769_C_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_769_C_ICCS__enc.json` |
| `NEV_2048_769_C_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_769_C_ICCS__keygen.json` |
| `NEV_2048_769_ICCS` | KAT log (sha256 `aaaf48628445594b…`) | `kat/kem-25/NEV_2048_769_ICCS.log` |
| `NEV_2048_769_ICCS` | timing dec | `records/kem-25/NEV_2048_769_ICCS__dec.json` |
| `NEV_2048_769_ICCS` | timing enc | `records/kem-25/NEV_2048_769_ICCS__enc.json` |
| `NEV_2048_769_ICCS` | timing keygen | `records/kem-25/NEV_2048_769_ICCS__keygen.json` |
| `NEV_2048_769_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_769_ICCS__dec.json` |
| `NEV_2048_769_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_769_ICCS__enc.json` |
| `NEV_2048_769_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_769_ICCS__keygen.json` |
| `NEV_2048_1409_ICCS` | KAT log (sha256 `bd97e677c147e022…`) | `kat/kem-25/NEV_2048_1409_ICCS.log` |
| `NEV_2048_1409_ICCS` | timing dec | `records/kem-25/NEV_2048_1409_ICCS__dec.json` |
| `NEV_2048_1409_ICCS` | timing enc | `records/kem-25/NEV_2048_1409_ICCS__enc.json` |
| `NEV_2048_1409_ICCS` | timing keygen | `records/kem-25/NEV_2048_1409_ICCS__keygen.json` |
| `NEV_2048_1409_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_1409_ICCS__dec.json` |
| `NEV_2048_1409_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_1409_ICCS__enc.json` |
| `NEV_2048_1409_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_1409_ICCS__keygen.json` |
| `NEV_2048_3329_ICCS` | KAT log (sha256 `b1d2f0970795169e…`) | `kat/kem-25/NEV_2048_3329_ICCS.log` |
| `NEV_2048_3329_ICCS` | timing dec | `records/kem-25/NEV_2048_3329_ICCS__dec.json` |
| `NEV_2048_3329_ICCS` | timing enc | `records/kem-25/NEV_2048_3329_ICCS__enc.json` |
| `NEV_2048_3329_ICCS` | timing keygen | `records/kem-25/NEV_2048_3329_ICCS__keygen.json` |
| `NEV_2048_3329_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_3329_ICCS__dec.json` |
| `NEV_2048_3329_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_3329_ICCS__enc.json` |
| `NEV_2048_3329_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_3329_ICCS__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

