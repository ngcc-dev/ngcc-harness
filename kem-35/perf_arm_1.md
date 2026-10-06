# kem-35 Scloud+ — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-35` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881269264384.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Scloud+
- Implementation versions measured: reference
- Parameter sets: `Scloudplus-128-AES-packed10`, `Scloudplus-128-SHAKE-packed10`, `Scloudplus-128-SM3-packed10`, `Scloudplus-192-AES-packed10`, `Scloudplus-192-SHAKE-packed10`, `Scloudplus-192-SM3-packed10`, `Scloudplus-256-AES-packed10`, `Scloudplus-256-SHAKE-packed10`, `Scloudplus-256-SM3-packed10`, `Scloudplus-384-AES-packed10`, `Scloudplus-384-SHAKE-packed10`, `Scloudplus-384-SM3-packed10`, `Scloudplus-512-AES-packed10`, `Scloudplus-512-SHAKE-packed10`, `Scloudplus-512-SM3-packed10`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-35/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Scloudplus-128-AES-packed10` | guide | PASS |
| `Scloudplus-128-SHAKE-packed10` | guide | PASS |
| `Scloudplus-128-SM3-packed10` | guide | PASS |
| `Scloudplus-192-AES-packed10` | guide | PASS |
| `Scloudplus-192-SHAKE-packed10` | guide | PASS |
| `Scloudplus-192-SM3-packed10` | guide | PASS |
| `Scloudplus-256-AES-packed10` | guide | PASS |
| `Scloudplus-256-SHAKE-packed10` | guide | PASS |
| `Scloudplus-256-SM3-packed10` | guide | PASS |
| `Scloudplus-384-AES-packed10` | guide | PASS |
| `Scloudplus-384-SHAKE-packed10` | guide | PASS |
| `Scloudplus-384-SM3-packed10` | guide | PASS |
| `Scloudplus-512-AES-packed10` | guide | PASS |
| `Scloudplus-512-SHAKE-packed10` | guide | PASS |
| `Scloudplus-512-SM3-packed10` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | keygen | 24.30 M | 9.02 ms | 111 | 9.02 ms | 350 (5 × 70) |
| `Scloudplus-128-AES-packed10` | enc | 24.20 M | 8.98 ms | 111 | 8.98 ms | 355 (5 × 71) |
| `Scloudplus-128-AES-packed10` | dec | 24.17 M | 8.97 ms | 112 | 8.97 ms | 355 (5 × 71) |
| `Scloudplus-128-SHAKE-packed10` | keygen | 4.65 M | 1.73 ms | 580 | 1.73 ms | 1815 (5 × 363) |
| `Scloudplus-128-SHAKE-packed10` | enc | 4.61 M | 1.71 ms | 584 | 1.71 ms | 1845 (5 × 369) |
| `Scloudplus-128-SHAKE-packed10` | dec | 4.56 M | 1.69 ms | 591 | 1.69 ms | 1830 (5 × 366) |
| `Scloudplus-128-SM3-packed10` | keygen | 21.87 M | 8.11 ms | 123 | 8.11 ms | 390 (5 × 78) |
| `Scloudplus-128-SM3-packed10` | enc | 22.02 M | 8.17 ms | 122 | 8.1 ms | 390 (5 × 78) |
| `Scloudplus-128-SM3-packed10` | dec | 21.80 M | 8.09 ms | 124 | 8.04 ms | 395 (5 × 79) |
| `Scloudplus-192-AES-packed10` | keygen | 45.62 M | 16.9 ms | 59.1 | 16.9 ms | 190 (5 × 38) |
| `Scloudplus-192-AES-packed10` | enc | 45.38 M | 16.8 ms | 59.4 | 16.8 ms | 190 (5 × 38) |
| `Scloudplus-192-AES-packed10` | dec | 45.48 M | 16.9 ms | 59.3 | 16.9 ms | 190 (5 × 38) |
| `Scloudplus-192-SHAKE-packed10` | keygen | 9.28 M | 3.44 ms | 290 | 3.44 ms | 920 (5 × 184) |
| `Scloudplus-192-SHAKE-packed10` | enc | 9.18 M | 3.41 ms | 294 | 3.39 ms | 935 (5 × 187) |
| `Scloudplus-192-SHAKE-packed10` | dec | 9.20 M | 3.42 ms | 293 | 3.42 ms | 925 (5 × 185) |
| `Scloudplus-192-SM3-packed10` | keygen | 41.42 M | 15.4 ms | 65.1 | 15.4 ms | 205 (5 × 41) |
| `Scloudplus-192-SM3-packed10` | enc | 41.56 M | 15.4 ms | 64.9 | 15.3 ms | 210 (5 × 42) |
| `Scloudplus-192-SM3-packed10` | dec | 41.12 M | 15.3 ms | 65.6 | 15.2 ms | 210 (5 × 42) |
| `Scloudplus-256-AES-packed10` | keygen | 93.10 M | 34.5 ms | 29 | 34.5 ms | 100 (5 × 20) |
| `Scloudplus-256-AES-packed10` | enc | 92.63 M | 34.4 ms | 29.1 | 34.4 ms | 100 (5 × 20) |
| `Scloudplus-256-AES-packed10` | dec | 92.61 M | 34.4 ms | 29.1 | 34.4 ms | 100 (5 × 20) |
| `Scloudplus-256-SHAKE-packed10` | keygen | 18.20 M | 6.75 ms | 148 | 6.75 ms | 465 (5 × 93) |
| `Scloudplus-256-SHAKE-packed10` | enc | 17.77 M | 6.59 ms | 152 | 6.58 ms | 480 (5 × 96) |
| `Scloudplus-256-SHAKE-packed10` | dec | 17.75 M | 6.59 ms | 152 | 6.59 ms | 480 (5 × 96) |
| `Scloudplus-256-SM3-packed10` | keygen | 85.24 M | 31.6 ms | 31.6 | 31.5 ms | 100 (5 × 20) |
| `Scloudplus-256-SM3-packed10` | enc | 84.50 M | 31.3 ms | 31.9 | 31.3 ms | 105 (5 × 21) |
| `Scloudplus-256-SM3-packed10` | dec | 84.26 M | 31.3 ms | 32 | 31.3 ms | 105 (5 × 21) |
| `Scloudplus-384-AES-packed10` | keygen | 186.86 M | 69.3 ms | 14.4 | 69.3 ms | 100 (5 × 20) |
| `Scloudplus-384-AES-packed10` | enc | 183.63 M | 68.1 ms | 14.7 | 68.1 ms | 100 (5 × 20) |
| `Scloudplus-384-AES-packed10` | dec | 184.00 M | 68.3 ms | 14.6 | 68.3 ms | 100 (5 × 20) |
| `Scloudplus-384-SHAKE-packed10` | keygen | 39.73 M | 14.7 ms | 67.8 | 14.7 ms | 215 (5 × 43) |
| `Scloudplus-384-SHAKE-packed10` | enc | 36.63 M | 13.6 ms | 73.6 | 13.6 ms | 235 (5 × 47) |
| `Scloudplus-384-SHAKE-packed10` | dec | 37.02 M | 13.7 ms | 72.8 | 13.7 ms | 230 (5 × 46) |
| `Scloudplus-384-SM3-packed10` | keygen | 169.39 M | 62.8 ms | 15.9 | 62.8 ms | 100 (5 × 20) |
| `Scloudplus-384-SM3-packed10` | enc | 167.48 M | 62.1 ms | 16.1 | 61.9 ms | 100 (5 × 20) |
| `Scloudplus-384-SM3-packed10` | dec | 166.27 M | 61.7 ms | 16.2 | 61.7 ms | 100 (5 × 20) |
| `Scloudplus-512-AES-packed10` | keygen | 392.60 M | 146 ms | 6.86 | 146 ms | 100 (5 × 20) |
| `Scloudplus-512-AES-packed10` | enc | 380.54 M | 141 ms | 7.08 | 141 ms | 100 (5 × 20) |
| `Scloudplus-512-AES-packed10` | dec | 380.97 M | 141 ms | 7.07 | 141 ms | 100 (5 × 20) |
| `Scloudplus-512-SHAKE-packed10` | keygen | 83.76 M | 31.1 ms | 32.2 | 31.1 ms | 105 (5 × 21) |
| `Scloudplus-512-SHAKE-packed10` | enc | 71.85 M | 26.7 ms | 37.5 | 26.7 ms | 120 (5 × 24) |
| `Scloudplus-512-SHAKE-packed10` | dec | 72.33 M | 26.8 ms | 37.3 | 26.8 ms | 120 (5 × 24) |
| `Scloudplus-512-SM3-packed10` | keygen | 358.53 M | 133 ms | 7.52 | 133 ms | 100 (5 × 20) |
| `Scloudplus-512-SM3-packed10` | enc | 347.69 M | 129 ms | 7.75 | 129 ms | 100 (5 × 20) |
| `Scloudplus-512-SM3-packed10` | dec | 346.63 M | 129 ms | 7.78 | 129 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | keygen | 27080 | 1436 KiB | 1516 KiB |
| `Scloudplus-128-AES-packed10` | enc | 27080 | 1460 KiB | 1524 KiB |
| `Scloudplus-128-AES-packed10` | dec | 27080 | 1460 KiB | 1524 KiB |
| `Scloudplus-128-SHAKE-packed10` | keygen | 25696 | 1428 KiB | 1512 KiB |
| `Scloudplus-128-SHAKE-packed10` | enc | 25696 | 1456 KiB | 1520 KiB |
| `Scloudplus-128-SHAKE-packed10` | dec | 25696 | 1456 KiB | 1520 KiB |
| `Scloudplus-128-SM3-packed10` | keygen | 29344 | 3452 KiB | 3536 KiB |
| `Scloudplus-128-SM3-packed10` | enc | 29344 | 1472 KiB | 1544 KiB |
| `Scloudplus-128-SM3-packed10` | dec | 29344 | 1472 KiB | 1536 KiB |
| `Scloudplus-192-AES-packed10` | keygen | 27032 | 1456 KiB | 1556 KiB |
| `Scloudplus-192-AES-packed10` | enc | 27032 | 1508 KiB | 1572 KiB |
| `Scloudplus-192-AES-packed10` | dec | 27032 | 3520 KiB | 3584 KiB |
| `Scloudplus-192-SHAKE-packed10` | keygen | 25648 | 1452 KiB | 1552 KiB |
| `Scloudplus-192-SHAKE-packed10` | enc | 25648 | 1504 KiB | 1568 KiB |
| `Scloudplus-192-SHAKE-packed10` | dec | 25648 | 1508 KiB | 1572 KiB |
| `Scloudplus-192-SM3-packed10` | keygen | 29272 | 1456 KiB | 1588 KiB |
| `Scloudplus-192-SM3-packed10` | enc | 29272 | 1536 KiB | 1604 KiB |
| `Scloudplus-192-SM3-packed10` | dec | 29272 | 1540 KiB | 1604 KiB |
| `Scloudplus-256-AES-packed10` | keygen | 27656 | 1472 KiB | 1588 KiB |
| `Scloudplus-256-AES-packed10` | enc | 27656 | 3572 KiB | 3636 KiB |
| `Scloudplus-256-AES-packed10` | dec | 27656 | 3536 KiB | 3600 KiB |
| `Scloudplus-256-SHAKE-packed10` | keygen | 26272 | 3508 KiB | 3624 KiB |
| `Scloudplus-256-SHAKE-packed10` | enc | 26272 | 1548 KiB | 1612 KiB |
| `Scloudplus-256-SHAKE-packed10` | dec | 26272 | 1548 KiB | 1612 KiB |
| `Scloudplus-256-SM3-packed10` | keygen | 29896 | 1472 KiB | 1632 KiB |
| `Scloudplus-256-SM3-packed10` | enc | 29896 | 1584 KiB | 1660 KiB |
| `Scloudplus-256-SM3-packed10` | dec | 29896 | 1588 KiB | 1652 KiB |
| `Scloudplus-384-AES-packed10` | keygen | 27512 | 1520 KiB | 1692 KiB |
| `Scloudplus-384-AES-packed10` | enc | 27512 | 1660 KiB | 1724 KiB |
| `Scloudplus-384-AES-packed10` | dec | 27512 | 3600 KiB | 3664 KiB |
| `Scloudplus-384-SHAKE-packed10` | keygen | 26128 | 1516 KiB | 1684 KiB |
| `Scloudplus-384-SHAKE-packed10` | enc | 26128 | 1656 KiB | 1720 KiB |
| `Scloudplus-384-SHAKE-packed10` | dec | 26128 | 3640 KiB | 3704 KiB |
| `Scloudplus-384-SM3-packed10` | keygen | 29912 | 1520 KiB | 1792 KiB |
| `Scloudplus-384-SM3-packed10` | enc | 29912 | 3592 KiB | 3656 KiB |
| `Scloudplus-384-SM3-packed10` | dec | 29912 | 3728 KiB | 3792 KiB |
| `Scloudplus-512-AES-packed10` | keygen | 27760 | 1564 KiB | 1784 KiB |
| `Scloudplus-512-AES-packed10` | enc | 27760 | 3768 KiB | 3832 KiB |
| `Scloudplus-512-AES-packed10` | dec | 27760 | 1772 KiB | 1836 KiB |
| `Scloudplus-512-SHAKE-packed10` | keygen | 26408 | 1568 KiB | 1788 KiB |
| `Scloudplus-512-SHAKE-packed10` | enc | 26408 | 1768 KiB | 1832 KiB |
| `Scloudplus-512-SHAKE-packed10` | dec | 26408 | 1772 KiB | 1836 KiB |
| `Scloudplus-512-SM3-packed10` | keygen | 30192 | 3476 KiB | 3692 KiB |
| `Scloudplus-512-SM3-packed10` | enc | 30192 | 3728 KiB | 3792 KiB |
| `Scloudplus-512-SM3-packed10` | dec | 30192 | 1956 KiB | 2020 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | 6096 | 7440 | 6160 | 16 |
| `Scloudplus-128-SHAKE-packed10` | 6096 | 7440 | 6160 | 16 |
| `Scloudplus-128-SM3-packed10` | 6096 | 7440 | 6160 | 16 |
| `Scloudplus-192-AES-packed10` | 11456 | 13872 | 12645 | 24 |
| `Scloudplus-192-SHAKE-packed10` | 11456 | 13872 | 12645 | 24 |
| `Scloudplus-192-SM3-packed10` | 11456 | 13872 | 12645 | 24 |
| `Scloudplus-256-AES-packed10` | 16296 | 19680 | 17925 | 32 |
| `Scloudplus-256-SHAKE-packed10` | 16296 | 19680 | 17925 | 32 |
| `Scloudplus-256-SM3-packed10` | 16296 | 19680 | 17925 | 32 |
| `Scloudplus-384-AES-packed10` | 33296 | 40144 | 33600 | 48 |
| `Scloudplus-384-SHAKE-packed10` | 33296 | 40144 | 33600 | 48 |
| `Scloudplus-384-SM3-packed10` | 33296 | 40144 | 33600 | 48 |
| `Scloudplus-512-AES-packed10` | 48016 | 57872 | 48320 | 64 |
| `Scloudplus-512-SHAKE-packed10` | 48016 | 57872 | 48320 | 64 |
| `Scloudplus-512-SM3-packed10` | 48016 | 57872 | 48320 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **instance-dependent** — SM3 sets: private modified copy of auxfunc (official not linked); AES/SHAKE sets: own AES/Keccak (bypass); benchmark pilot uses SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-128-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-128-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-128-SHAKE-packed10` | keygen | 0.0% | 0.3% | drng 2 |
| `Scloudplus-128-SHAKE-packed10` | enc | 0.0% | 0.1% | drng 1 |
| `Scloudplus-128-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-128-SM3-packed10` | keygen | 93% | 0.0% | drng 2, pseudoXOF 609, pseudohash 1 |
| `Scloudplus-128-SM3-packed10` | enc | 94% | 0.0% | drng 1, pseudoXOF 611, pseudohash 1 |
| `Scloudplus-128-SM3-packed10` | dec | 93% | 0.0% | pseudoXOF 611 |
| `Scloudplus-192-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-192-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-192-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-192-SHAKE-packed10` | keygen | 0.0% | 0.1% | drng 2 |
| `Scloudplus-192-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-192-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-192-SM3-packed10` | keygen | 92% | 0.0% | drng 2, pseudoXOF 833, pseudohash 1 |
| `Scloudplus-192-SM3-packed10` | enc | 93% | 0.0% | drng 1, pseudoXOF 835, pseudohash 1 |
| `Scloudplus-192-SM3-packed10` | dec | 93% | 0.0% | pseudoXOF 835 |
| `Scloudplus-256-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-256-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-256-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-256-SHAKE-packed10` | keygen | 0.0% | 0.1% | drng 2 |
| `Scloudplus-256-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-256-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-256-SM3-packed10` | keygen | 91% | 0.0% | drng 2, pseudoXOF 1.18e+03, pseudohash 1 |
| `Scloudplus-256-SM3-packed10` | enc | 92% | 0.0% | drng 1, pseudoXOF 1.19e+03, pseudohash 1 |
| `Scloudplus-256-SM3-packed10` | dec | 92% | 0.0% | pseudoXOF 1.19e+03 |
| `Scloudplus-384-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-384-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-384-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-384-SHAKE-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-384-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-384-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-384-SM3-packed10` | keygen | 89% | 0.0% | drng 2, pseudoXOF 1.66e+03, pseudohash 1 |
| `Scloudplus-384-SM3-packed10` | enc | 91% | 0.0% | drng 1, pseudoXOF 1.67e+03, pseudohash 1 |
| `Scloudplus-384-SM3-packed10` | dec | 90% | 0.0% | pseudoXOF 1.67e+03 |
| `Scloudplus-512-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-512-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-512-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-512-SHAKE-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-512-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-512-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-512-SM3-packed10` | keygen | 87% | 0.0% | drng 2, pseudoXOF 2.4e+03, pseudohash 1 |
| `Scloudplus-512-SM3-packed10` | enc | 91% | 0.0% | drng 1, pseudoXOF 2.4e+03, pseudohash 1 |
| `Scloudplus-512-SM3-packed10` | dec | 90% | 0.0% | pseudoXOF 2.4e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Scloudplus-128-AES-packed10` | KAT log (sha256 `79d7efc926a1a891…`) | `kat/kem-35/Scloudplus-128-AES-packed10.log` |
| `Scloudplus-128-AES-packed10` | timing dec | `records/kem-35/Scloudplus-128-AES-packed10__dec.json` |
| `Scloudplus-128-AES-packed10` | timing enc | `records/kem-35/Scloudplus-128-AES-packed10__enc.json` |
| `Scloudplus-128-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-128-AES-packed10__keygen.json` |
| `Scloudplus-128-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-128-AES-packed10__dec.json` |
| `Scloudplus-128-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-128-AES-packed10__enc.json` |
| `Scloudplus-128-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-128-AES-packed10__keygen.json` |
| `Scloudplus-128-SHAKE-packed10` | KAT log (sha256 `4cc1d04dc98bd07e…`) | `kat/kem-35/Scloudplus-128-SHAKE-packed10.log` |
| `Scloudplus-128-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-128-SHAKE-packed10__dec.json` |
| `Scloudplus-128-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-128-SHAKE-packed10__enc.json` |
| `Scloudplus-128-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-128-SHAKE-packed10__keygen.json` |
| `Scloudplus-128-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-128-SHAKE-packed10__dec.json` |
| `Scloudplus-128-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-128-SHAKE-packed10__enc.json` |
| `Scloudplus-128-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-128-SHAKE-packed10__keygen.json` |
| `Scloudplus-128-SM3-packed10` | KAT log (sha256 `430d1ff06832aa1d…`) | `kat/kem-35/Scloudplus-128-SM3-packed10.log` |
| `Scloudplus-128-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-128-SM3-packed10__dec.json` |
| `Scloudplus-128-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-128-SM3-packed10__enc.json` |
| `Scloudplus-128-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-128-SM3-packed10__keygen.json` |
| `Scloudplus-128-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-128-SM3-packed10__dec.json` |
| `Scloudplus-128-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-128-SM3-packed10__enc.json` |
| `Scloudplus-128-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-128-SM3-packed10__keygen.json` |
| `Scloudplus-192-AES-packed10` | KAT log (sha256 `e7a68dbcaf2eae83…`) | `kat/kem-35/Scloudplus-192-AES-packed10.log` |
| `Scloudplus-192-AES-packed10` | timing dec | `records/kem-35/Scloudplus-192-AES-packed10__dec.json` |
| `Scloudplus-192-AES-packed10` | timing enc | `records/kem-35/Scloudplus-192-AES-packed10__enc.json` |
| `Scloudplus-192-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-192-AES-packed10__keygen.json` |
| `Scloudplus-192-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-192-AES-packed10__dec.json` |
| `Scloudplus-192-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-192-AES-packed10__enc.json` |
| `Scloudplus-192-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-192-AES-packed10__keygen.json` |
| `Scloudplus-192-SHAKE-packed10` | KAT log (sha256 `751e930ecf0b3e8b…`) | `kat/kem-35/Scloudplus-192-SHAKE-packed10.log` |
| `Scloudplus-192-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-192-SHAKE-packed10__dec.json` |
| `Scloudplus-192-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-192-SHAKE-packed10__enc.json` |
| `Scloudplus-192-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-192-SHAKE-packed10__keygen.json` |
| `Scloudplus-192-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-192-SHAKE-packed10__dec.json` |
| `Scloudplus-192-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-192-SHAKE-packed10__enc.json` |
| `Scloudplus-192-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-192-SHAKE-packed10__keygen.json` |
| `Scloudplus-192-SM3-packed10` | KAT log (sha256 `11eaa6d39383c54f…`) | `kat/kem-35/Scloudplus-192-SM3-packed10.log` |
| `Scloudplus-192-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-192-SM3-packed10__dec.json` |
| `Scloudplus-192-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-192-SM3-packed10__enc.json` |
| `Scloudplus-192-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-192-SM3-packed10__keygen.json` |
| `Scloudplus-192-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-192-SM3-packed10__dec.json` |
| `Scloudplus-192-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-192-SM3-packed10__enc.json` |
| `Scloudplus-192-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-192-SM3-packed10__keygen.json` |
| `Scloudplus-256-AES-packed10` | KAT log (sha256 `a707abd4c8921610…`) | `kat/kem-35/Scloudplus-256-AES-packed10.log` |
| `Scloudplus-256-AES-packed10` | timing dec | `records/kem-35/Scloudplus-256-AES-packed10__dec.json` |
| `Scloudplus-256-AES-packed10` | timing enc | `records/kem-35/Scloudplus-256-AES-packed10__enc.json` |
| `Scloudplus-256-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-256-AES-packed10__keygen.json` |
| `Scloudplus-256-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-256-AES-packed10__dec.json` |
| `Scloudplus-256-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-256-AES-packed10__enc.json` |
| `Scloudplus-256-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-256-AES-packed10__keygen.json` |
| `Scloudplus-256-SHAKE-packed10` | KAT log (sha256 `0c6e32792d818470…`) | `kat/kem-35/Scloudplus-256-SHAKE-packed10.log` |
| `Scloudplus-256-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-256-SHAKE-packed10__dec.json` |
| `Scloudplus-256-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-256-SHAKE-packed10__enc.json` |
| `Scloudplus-256-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-256-SHAKE-packed10__keygen.json` |
| `Scloudplus-256-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-256-SHAKE-packed10__dec.json` |
| `Scloudplus-256-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-256-SHAKE-packed10__enc.json` |
| `Scloudplus-256-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-256-SHAKE-packed10__keygen.json` |
| `Scloudplus-256-SM3-packed10` | KAT log (sha256 `1b17e5b892645258…`) | `kat/kem-35/Scloudplus-256-SM3-packed10.log` |
| `Scloudplus-256-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-256-SM3-packed10__dec.json` |
| `Scloudplus-256-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-256-SM3-packed10__enc.json` |
| `Scloudplus-256-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-256-SM3-packed10__keygen.json` |
| `Scloudplus-256-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-256-SM3-packed10__dec.json` |
| `Scloudplus-256-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-256-SM3-packed10__enc.json` |
| `Scloudplus-256-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-256-SM3-packed10__keygen.json` |
| `Scloudplus-384-AES-packed10` | KAT log (sha256 `ac3da0d5a99b1c18…`) | `kat/kem-35/Scloudplus-384-AES-packed10.log` |
| `Scloudplus-384-AES-packed10` | timing dec | `records/kem-35/Scloudplus-384-AES-packed10__dec.json` |
| `Scloudplus-384-AES-packed10` | timing enc | `records/kem-35/Scloudplus-384-AES-packed10__enc.json` |
| `Scloudplus-384-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-384-AES-packed10__keygen.json` |
| `Scloudplus-384-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-384-AES-packed10__dec.json` |
| `Scloudplus-384-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-384-AES-packed10__enc.json` |
| `Scloudplus-384-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-384-AES-packed10__keygen.json` |
| `Scloudplus-384-SHAKE-packed10` | KAT log (sha256 `49132410831489b8…`) | `kat/kem-35/Scloudplus-384-SHAKE-packed10.log` |
| `Scloudplus-384-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-384-SHAKE-packed10__dec.json` |
| `Scloudplus-384-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-384-SHAKE-packed10__enc.json` |
| `Scloudplus-384-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-384-SHAKE-packed10__keygen.json` |
| `Scloudplus-384-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-384-SHAKE-packed10__dec.json` |
| `Scloudplus-384-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-384-SHAKE-packed10__enc.json` |
| `Scloudplus-384-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-384-SHAKE-packed10__keygen.json` |
| `Scloudplus-384-SM3-packed10` | KAT log (sha256 `5d5080c11b90c736…`) | `kat/kem-35/Scloudplus-384-SM3-packed10.log` |
| `Scloudplus-384-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-384-SM3-packed10__dec.json` |
| `Scloudplus-384-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-384-SM3-packed10__enc.json` |
| `Scloudplus-384-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-384-SM3-packed10__keygen.json` |
| `Scloudplus-384-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-384-SM3-packed10__dec.json` |
| `Scloudplus-384-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-384-SM3-packed10__enc.json` |
| `Scloudplus-384-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-384-SM3-packed10__keygen.json` |
| `Scloudplus-512-AES-packed10` | KAT log (sha256 `e6a6e3fd3db68aa4…`) | `kat/kem-35/Scloudplus-512-AES-packed10.log` |
| `Scloudplus-512-AES-packed10` | timing dec | `records/kem-35/Scloudplus-512-AES-packed10__dec.json` |
| `Scloudplus-512-AES-packed10` | timing enc | `records/kem-35/Scloudplus-512-AES-packed10__enc.json` |
| `Scloudplus-512-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-512-AES-packed10__keygen.json` |
| `Scloudplus-512-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-512-AES-packed10__dec.json` |
| `Scloudplus-512-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-512-AES-packed10__enc.json` |
| `Scloudplus-512-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-512-AES-packed10__keygen.json` |
| `Scloudplus-512-SHAKE-packed10` | KAT log (sha256 `ce0051c0d18361e9…`) | `kat/kem-35/Scloudplus-512-SHAKE-packed10.log` |
| `Scloudplus-512-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-512-SHAKE-packed10__dec.json` |
| `Scloudplus-512-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-512-SHAKE-packed10__enc.json` |
| `Scloudplus-512-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-512-SHAKE-packed10__keygen.json` |
| `Scloudplus-512-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-512-SHAKE-packed10__dec.json` |
| `Scloudplus-512-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-512-SHAKE-packed10__enc.json` |
| `Scloudplus-512-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-512-SHAKE-packed10__keygen.json` |
| `Scloudplus-512-SM3-packed10` | KAT log (sha256 `838a6304d6dc09f9…`) | `kat/kem-35/Scloudplus-512-SM3-packed10.log` |
| `Scloudplus-512-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-512-SM3-packed10__dec.json` |
| `Scloudplus-512-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-512-SM3-packed10__enc.json` |
| `Scloudplus-512-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-512-SM3-packed10__keygen.json` |
| `Scloudplus-512-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-512-SM3-packed10__dec.json` |
| `Scloudplus-512-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-512-SM3-packed10__enc.json` |
| `Scloudplus-512-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-512-SM3-packed10__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

