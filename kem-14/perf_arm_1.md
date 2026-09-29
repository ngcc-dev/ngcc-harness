# kem-14 DTRU — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-14` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844682350592.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: DTRU
- Implementation versions measured: reference
- Parameter sets: `DTRU-648`, `DTRU-648-PACK_PK`, `DTRU-768`, `DTRU-768-PACK_PK`, `DTRU-1024`, `DTRU-1024-PACK_PK`, `DTRU-1536`, `DTRU-1536-PACK_PK`, `DTRU-2048`, `DTRU-2048-PACK_PK`, `DTRU-Light`, `DTRU-Prime`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-14/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `DTRU-648` | guide | PASS |
| `DTRU-648-PACK_PK` | guide | PASS |
| `DTRU-768` | guide | PASS |
| `DTRU-768-PACK_PK` | guide | PASS |
| `DTRU-1024` | guide | PASS |
| `DTRU-1024-PACK_PK` | guide | PASS |
| `DTRU-1536` | guide | PASS |
| `DTRU-1536-PACK_PK` | guide | PASS |
| `DTRU-2048` | guide | PASS |
| `DTRU-2048-PACK_PK` | guide | PASS |
| `DTRU-Light` | guide | PASS |
| `DTRU-Prime` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `DTRU-648` | keygen | 241.9 k | 89.7 µs | 1.11e+04 | 89.7 µs | 26940 (5 × 5388) |
| `DTRU-648` | enc | 129.9 k | 48.2 µs | 2.07e+04 | 48.1 µs | 55365 (5 × 11073) |
| `DTRU-648` | dec | 257.4 k | 95.5 µs | 1.05e+04 | 95.4 µs | 30655 (5 × 6131) |
| `DTRU-648-PACK_PK` | keygen | 245.3 k | 91 µs | 1.1e+04 | 91 µs | 31645 (5 × 6329) |
| `DTRU-648-PACK_PK` | enc | 140.5 k | 52.1 µs | 1.92e+04 | 52.1 µs | 57835 (5 × 11567) |
| `DTRU-648-PACK_PK` | dec | 268.2 k | 99.5 µs | 1e+04 | 99.5 µs | 31190 (5 × 6238) |
| `DTRU-768` | keygen | 211.9 k | 78.6 µs | 1.27e+04 | 78.6 µs | 37210 (5 × 7442) |
| `DTRU-768` | enc | 150.1 k | 55.7 µs | 1.8e+04 | 55.7 µs | 54055 (5 × 10811) |
| `DTRU-768` | dec | 282.4 k | 105 µs | 9.54e+03 | 104 µs | 29835 (5 × 5967) |
| `DTRU-768-PACK_PK` | keygen | 216.5 k | 80.4 µs | 1.24e+04 | 80.1 µs | 36530 (5 × 7306) |
| `DTRU-768-PACK_PK` | enc | 162.9 k | 60.4 µs | 1.65e+04 | 60.4 µs | 49385 (5 × 9877) |
| `DTRU-768-PACK_PK` | dec | 294.4 k | 109 µs | 9.16e+03 | 109 µs | 28305 (5 × 5661) |
| `DTRU-1024` | keygen | 256.9 k | 95.3 µs | 1.05e+04 | 94.8 µs | 30365 (5 × 6073) |
| `DTRU-1024` | enc | 175.8 k | 65.3 µs | 1.53e+04 | 65.2 µs | 47105 (5 × 9421) |
| `DTRU-1024` | dec | 341.4 k | 127 µs | 7.89e+03 | 127 µs | 24695 (5 × 4939) |
| `DTRU-1024-PACK_PK` | keygen | 261.5 k | 97 µs | 1.03e+04 | 97.1 µs | 29685 (5 × 5937) |
| `DTRU-1024-PACK_PK` | enc | 192.5 k | 71.4 µs | 1.4e+04 | 71.5 µs | 42405 (5 × 8481) |
| `DTRU-1024-PACK_PK` | dec | 358.2 k | 133 µs | 7.52e+03 | 133 µs | 23520 (5 × 4704) |
| `DTRU-1536` | keygen | 341.5 k | 127 µs | 7.89e+03 | 126 µs | 23580 (5 × 4716) |
| `DTRU-1536` | enc | 297.4 k | 110 µs | 9.06e+03 | 110 µs | 28005 (5 × 5601) |
| `DTRU-1536` | dec | 564.2 k | 209 µs | 4.78e+03 | 209 µs | 14930 (5 × 2986) |
| `DTRU-1536-PACK_PK` | keygen | 349.6 k | 130 µs | 7.71e+03 | 129 µs | 22515 (5 × 4503) |
| `DTRU-1536-PACK_PK` | enc | 322.1 k | 120 µs | 8.36e+03 | 120 µs | 24100 (5 × 4820) |
| `DTRU-1536-PACK_PK` | dec | 589.7 k | 219 µs | 4.57e+03 | 219 µs | 14305 (5 × 2861) |
| `DTRU-2048` | keygen | 583.0 k | 216 µs | 4.62e+03 | 216 µs | 13550 (5 × 2710) |
| `DTRU-2048` | enc | 405.7 k | 151 µs | 6.64e+03 | 151 µs | 20360 (5 × 4072) |
| `DTRU-2048` | dec | 775.2 k | 288 µs | 3.48e+03 | 287 µs | 10885 (5 × 2177) |
| `DTRU-2048-PACK_PK` | keygen | 595.0 k | 221 µs | 4.53e+03 | 220 µs | 13180 (5 × 2636) |
| `DTRU-2048-PACK_PK` | enc | 439.3 k | 163 µs | 6.13e+03 | 163 µs | 19165 (5 × 3833) |
| `DTRU-2048-PACK_PK` | dec | 806.2 k | 299 µs | 3.34e+03 | 299 µs | 10215 (5 × 2043) |
| `DTRU-Light` | keygen | 91.1 k | 33.8 µs | 2.96e+04 | 33.7 µs | 81635 (5 × 16327) |
| `DTRU-Light` | enc | 75.3 k | 28 µs | 3.58e+04 | 27.9 µs | 100000 (5 × 20000) |
| `DTRU-Light` | dec | 149.5 k | 55.5 µs | 1.8e+04 | 55.5 µs | 54985 (5 × 10997) |
| `DTRU-Prime` | keygen | 92.29 M | 34.2 ms | 29.2 | 34.2 ms | 100 (5 × 20) |
| `DTRU-Prime` | enc | 264.6 k | 98.2 µs | 1.02e+04 | 98.2 µs | 31455 (5 × 6291) |
| `DTRU-Prime` | dec | 546.7 k | 203 µs | 4.93e+03 | 202 µs | 15470 (5 × 3094) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DTRU-648` | keygen | 28620 | 1420 KiB | 1488 KiB |
| `DTRU-648` | enc | 28620 | 1424 KiB | 1488 KiB |
| `DTRU-648` | dec | 28620 | 1432 KiB | 1496 KiB |
| `DTRU-648-PACK_PK` | keygen | 30516 | 1420 KiB | 1496 KiB |
| `DTRU-648-PACK_PK` | enc | 30516 | 1436 KiB | 1500 KiB |
| `DTRU-648-PACK_PK` | dec | 30516 | 1440 KiB | 1504 KiB |
| `DTRU-768` | keygen | 27204 | 1416 KiB | 1484 KiB |
| `DTRU-768` | enc | 27204 | 1420 KiB | 1488 KiB |
| `DTRU-768` | dec | 27204 | 1428 KiB | 1492 KiB |
| `DTRU-768-PACK_PK` | keygen | 29116 | 1420 KiB | 1496 KiB |
| `DTRU-768-PACK_PK` | enc | 29116 | 1440 KiB | 1508 KiB |
| `DTRU-768-PACK_PK` | dec | 29116 | 1444 KiB | 1508 KiB |
| `DTRU-1024` | keygen | 29204 | 1424 KiB | 1496 KiB |
| `DTRU-1024` | enc | 29204 | 1432 KiB | 1496 KiB |
| `DTRU-1024` | dec | 29204 | 1436 KiB | 1500 KiB |
| `DTRU-1024-PACK_PK` | keygen | 31100 | 1428 KiB | 1508 KiB |
| `DTRU-1024-PACK_PK` | enc | 31100 | 1452 KiB | 1516 KiB |
| `DTRU-1024-PACK_PK` | dec | 31100 | 1456 KiB | 1520 KiB |
| `DTRU-1536` | keygen | 30092 | 1424 KiB | 1500 KiB |
| `DTRU-1536` | enc | 30092 | 1436 KiB | 1504 KiB |
| `DTRU-1536` | dec | 30092 | 1444 KiB | 1512 KiB |
| `DTRU-1536-PACK_PK` | keygen | 32020 | 1424 KiB | 1512 KiB |
| `DTRU-1536-PACK_PK` | enc | 32020 | 3500 KiB | 3564 KiB |
| `DTRU-1536-PACK_PK` | dec | 32020 | 1464 KiB | 1532 KiB |
| `DTRU-2048` | keygen | 36508 | 1436 KiB | 1520 KiB |
| `DTRU-2048` | enc | 36508 | 1460 KiB | 1524 KiB |
| `DTRU-2048` | dec | 36508 | 1460 KiB | 1528 KiB |
| `DTRU-2048-PACK_PK` | keygen | 38404 | 1436 KiB | 1536 KiB |
| `DTRU-2048-PACK_PK` | enc | 38404 | 1488 KiB | 1552 KiB |
| `DTRU-2048-PACK_PK` | dec | 38404 | 1492 KiB | 1560 KiB |
| `DTRU-Light` | keygen | 29404 | 1420 KiB | 1484 KiB |
| `DTRU-Light` | enc | 29404 | 1424 KiB | 1488 KiB |
| `DTRU-Light` | dec | 29404 | 1424 KiB | 1488 KiB |
| `DTRU-Prime` | keygen | 32932 | 1428 KiB | 1528 KiB |
| `DTRU-Prime` | enc | 32932 | 3484 KiB | 3548 KiB |
| `DTRU-Prime` | dec | 32932 | 1464 KiB | 1528 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `DTRU-648` | 972 | 1328 | 729 | 32 |
| `DTRU-648-PACK_PK` | 954 | 1310 | 729 | 32 |
| `DTRU-768` | 1152 | 1568 | 960 | 32 |
| `DTRU-768-PACK_PK` | 1130 | 1546 | 960 | 32 |
| `DTRU-1024` | 1536 | 2080 | 1280 | 32 |
| `DTRU-1024-PACK_PK` | 1506 | 2050 | 1280 | 32 |
| `DTRU-1536` | 2304 | 3136 | 1920 | 64 |
| `DTRU-1536-PACK_PK` | 2259 | 3091 | 1920 | 64 |
| `DTRU-2048` | 3072 | 3904 | 2560 | 64 |
| `DTRU-2048-PACK_PK` | 3012 | 3844 | 2560 | 64 |
| `DTRU-Light` | 640 | 864 | 512 | 32 |
| `DTRU-Prime` | 1495 | 1935 | 1359 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `DTRU-648` | keygen | 0.0% | 35% | drng 2 |
| `DTRU-648` | enc | 30% | 3.3% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-648` | dec | 31% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-648-PACK_PK` | keygen | 0.0% | 34% | drng 2 |
| `DTRU-648-PACK_PK` | enc | 28% | 3.2% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-648-PACK_PK` | dec | 29% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-768` | keygen | 0.0% | 30% | drng 2 |
| `DTRU-768` | enc | 41% | 2.8% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-768` | dec | 39% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-768-PACK_PK` | keygen | 0.0% | 30% | drng 2 |
| `DTRU-768-PACK_PK` | enc | 38% | 2.6% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-768-PACK_PK` | dec | 37% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-1024` | keygen | 0.0% | 33% | drng 2 |
| `DTRU-1024` | enc | 40% | 2.4% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-1024` | dec | 37% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-1024-PACK_PK` | keygen | 0.0% | 32% | drng 2 |
| `DTRU-1024-PACK_PK` | enc | 35% | 2.2% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-1024-PACK_PK` | dec | 36% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-1536` | keygen | 0.0% | 27% | drng 2 |
| `DTRU-1536` | enc | 40% | 1.9% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-1536` | dec | 37% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-1536-PACK_PK` | keygen | 0.0% | 26% | drng 2 |
| `DTRU-1536-PACK_PK` | enc | 37% | 1.8% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-1536-PACK_PK` | dec | 35% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-2048` | keygen | 0.0% | 24% | drng 2 |
| `DTRU-2048` | enc | 39% | 1.4% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-2048` | dec | 35% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-2048-PACK_PK` | keygen | 0.0% | 24% | drng 2 |
| `DTRU-2048-PACK_PK` | enc | 36% | 1.3% | drng 1, pseudoXOF 1, pseudohash 2 |
| `DTRU-2048-PACK_PK` | dec | 33% | 0.0% | pseudoXOF 1, pseudohash 3 |
| `DTRU-Light` | keygen | 0.0% | 26% | drng 2 |
| `DTRU-Light` | enc | 35% | 5.7% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-Light` | dec | 38% | 0.0% | pseudoXOF 1, pseudohash 2 |
| `DTRU-Prime` | keygen | 0.0% | 0.0% | drng 2 |
| `DTRU-Prime` | enc | 18% | 1.6% | drng 1, pseudoXOF 1, pseudohash 1 |
| `DTRU-Prime` | dec | 20% | 0.0% | pseudoXOF 1, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DTRU-648` | KAT log (sha256 `aeec76d8d2e84461…`) | `kat/kem-14/DTRU-648.log` |
| `DTRU-648` | timing dec | `records/kem-14/DTRU-648__dec.json` |
| `DTRU-648` | timing enc | `records/kem-14/DTRU-648__enc.json` |
| `DTRU-648` | timing keygen | `records/kem-14/DTRU-648__keygen.json` |
| `DTRU-648` | hash profile dec | `profile/kem-14/DTRU-648__dec.json` |
| `DTRU-648` | hash profile enc | `profile/kem-14/DTRU-648__enc.json` |
| `DTRU-648` | hash profile keygen | `profile/kem-14/DTRU-648__keygen.json` |
| `DTRU-648-PACK_PK` | KAT log (sha256 `d4d9fc462d803add…`) | `kat/kem-14/DTRU-648-PACK_PK.log` |
| `DTRU-648-PACK_PK` | timing dec | `records/kem-14/DTRU-648-PACK_PK__dec.json` |
| `DTRU-648-PACK_PK` | timing enc | `records/kem-14/DTRU-648-PACK_PK__enc.json` |
| `DTRU-648-PACK_PK` | timing keygen | `records/kem-14/DTRU-648-PACK_PK__keygen.json` |
| `DTRU-648-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-648-PACK_PK__dec.json` |
| `DTRU-648-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-648-PACK_PK__enc.json` |
| `DTRU-648-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-648-PACK_PK__keygen.json` |
| `DTRU-768` | KAT log (sha256 `3aa38e441ebe4ebd…`) | `kat/kem-14/DTRU-768.log` |
| `DTRU-768` | timing dec | `records/kem-14/DTRU-768__dec.json` |
| `DTRU-768` | timing enc | `records/kem-14/DTRU-768__enc.json` |
| `DTRU-768` | timing keygen | `records/kem-14/DTRU-768__keygen.json` |
| `DTRU-768` | hash profile dec | `profile/kem-14/DTRU-768__dec.json` |
| `DTRU-768` | hash profile enc | `profile/kem-14/DTRU-768__enc.json` |
| `DTRU-768` | hash profile keygen | `profile/kem-14/DTRU-768__keygen.json` |
| `DTRU-768-PACK_PK` | KAT log (sha256 `4bf33b245a17e05c…`) | `kat/kem-14/DTRU-768-PACK_PK.log` |
| `DTRU-768-PACK_PK` | timing dec | `records/kem-14/DTRU-768-PACK_PK__dec.json` |
| `DTRU-768-PACK_PK` | timing enc | `records/kem-14/DTRU-768-PACK_PK__enc.json` |
| `DTRU-768-PACK_PK` | timing keygen | `records/kem-14/DTRU-768-PACK_PK__keygen.json` |
| `DTRU-768-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-768-PACK_PK__dec.json` |
| `DTRU-768-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-768-PACK_PK__enc.json` |
| `DTRU-768-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-768-PACK_PK__keygen.json` |
| `DTRU-1024` | KAT log (sha256 `52d45fddf07db848…`) | `kat/kem-14/DTRU-1024.log` |
| `DTRU-1024` | timing dec | `records/kem-14/DTRU-1024__dec.json` |
| `DTRU-1024` | timing enc | `records/kem-14/DTRU-1024__enc.json` |
| `DTRU-1024` | timing keygen | `records/kem-14/DTRU-1024__keygen.json` |
| `DTRU-1024` | hash profile dec | `profile/kem-14/DTRU-1024__dec.json` |
| `DTRU-1024` | hash profile enc | `profile/kem-14/DTRU-1024__enc.json` |
| `DTRU-1024` | hash profile keygen | `profile/kem-14/DTRU-1024__keygen.json` |
| `DTRU-1024-PACK_PK` | KAT log (sha256 `d63ebc264ba63e3b…`) | `kat/kem-14/DTRU-1024-PACK_PK.log` |
| `DTRU-1024-PACK_PK` | timing dec | `records/kem-14/DTRU-1024-PACK_PK__dec.json` |
| `DTRU-1024-PACK_PK` | timing enc | `records/kem-14/DTRU-1024-PACK_PK__enc.json` |
| `DTRU-1024-PACK_PK` | timing keygen | `records/kem-14/DTRU-1024-PACK_PK__keygen.json` |
| `DTRU-1024-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-1024-PACK_PK__dec.json` |
| `DTRU-1024-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-1024-PACK_PK__enc.json` |
| `DTRU-1024-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-1024-PACK_PK__keygen.json` |
| `DTRU-1536` | KAT log (sha256 `30f3d16cfee917da…`) | `kat/kem-14/DTRU-1536.log` |
| `DTRU-1536` | timing dec | `records/kem-14/DTRU-1536__dec.json` |
| `DTRU-1536` | timing enc | `records/kem-14/DTRU-1536__enc.json` |
| `DTRU-1536` | timing keygen | `records/kem-14/DTRU-1536__keygen.json` |
| `DTRU-1536` | hash profile dec | `profile/kem-14/DTRU-1536__dec.json` |
| `DTRU-1536` | hash profile enc | `profile/kem-14/DTRU-1536__enc.json` |
| `DTRU-1536` | hash profile keygen | `profile/kem-14/DTRU-1536__keygen.json` |
| `DTRU-1536-PACK_PK` | KAT log (sha256 `bd51dde08cf71945…`) | `kat/kem-14/DTRU-1536-PACK_PK.log` |
| `DTRU-1536-PACK_PK` | timing dec | `records/kem-14/DTRU-1536-PACK_PK__dec.json` |
| `DTRU-1536-PACK_PK` | timing enc | `records/kem-14/DTRU-1536-PACK_PK__enc.json` |
| `DTRU-1536-PACK_PK` | timing keygen | `records/kem-14/DTRU-1536-PACK_PK__keygen.json` |
| `DTRU-1536-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-1536-PACK_PK__dec.json` |
| `DTRU-1536-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-1536-PACK_PK__enc.json` |
| `DTRU-1536-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-1536-PACK_PK__keygen.json` |
| `DTRU-2048` | KAT log (sha256 `71a81edc9fcd566a…`) | `kat/kem-14/DTRU-2048.log` |
| `DTRU-2048` | timing dec | `records/kem-14/DTRU-2048__dec.json` |
| `DTRU-2048` | timing enc | `records/kem-14/DTRU-2048__enc.json` |
| `DTRU-2048` | timing keygen | `records/kem-14/DTRU-2048__keygen.json` |
| `DTRU-2048` | hash profile dec | `profile/kem-14/DTRU-2048__dec.json` |
| `DTRU-2048` | hash profile enc | `profile/kem-14/DTRU-2048__enc.json` |
| `DTRU-2048` | hash profile keygen | `profile/kem-14/DTRU-2048__keygen.json` |
| `DTRU-2048-PACK_PK` | KAT log (sha256 `dfd43290bfab0cb1…`) | `kat/kem-14/DTRU-2048-PACK_PK.log` |
| `DTRU-2048-PACK_PK` | timing dec | `records/kem-14/DTRU-2048-PACK_PK__dec.json` |
| `DTRU-2048-PACK_PK` | timing enc | `records/kem-14/DTRU-2048-PACK_PK__enc.json` |
| `DTRU-2048-PACK_PK` | timing keygen | `records/kem-14/DTRU-2048-PACK_PK__keygen.json` |
| `DTRU-2048-PACK_PK` | hash profile dec | `profile/kem-14/DTRU-2048-PACK_PK__dec.json` |
| `DTRU-2048-PACK_PK` | hash profile enc | `profile/kem-14/DTRU-2048-PACK_PK__enc.json` |
| `DTRU-2048-PACK_PK` | hash profile keygen | `profile/kem-14/DTRU-2048-PACK_PK__keygen.json` |
| `DTRU-Light` | KAT log (sha256 `64a7aad61a11dbbd…`) | `kat/kem-14/DTRU-Light.log` |
| `DTRU-Light` | timing dec | `records/kem-14/DTRU-Light__dec.json` |
| `DTRU-Light` | timing enc | `records/kem-14/DTRU-Light__enc.json` |
| `DTRU-Light` | timing keygen | `records/kem-14/DTRU-Light__keygen.json` |
| `DTRU-Light` | hash profile dec | `profile/kem-14/DTRU-Light__dec.json` |
| `DTRU-Light` | hash profile enc | `profile/kem-14/DTRU-Light__enc.json` |
| `DTRU-Light` | hash profile keygen | `profile/kem-14/DTRU-Light__keygen.json` |
| `DTRU-Prime` | KAT log (sha256 `f4235f064790c004…`) | `kat/kem-14/DTRU-Prime.log` |
| `DTRU-Prime` | timing dec | `records/kem-14/DTRU-Prime__dec.json` |
| `DTRU-Prime` | timing enc | `records/kem-14/DTRU-Prime__enc.json` |
| `DTRU-Prime` | timing keygen | `records/kem-14/DTRU-Prime__keygen.json` |
| `DTRU-Prime` | hash profile dec | `profile/kem-14/DTRU-Prime__dec.json` |
| `DTRU-Prime` | hash profile enc | `profile/kem-14/DTRU-Prime__enc.json` |
| `DTRU-Prime` | hash profile keygen | `profile/kem-14/DTRU-Prime__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

