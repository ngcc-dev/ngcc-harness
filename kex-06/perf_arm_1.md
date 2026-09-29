# kex-06 MAMBA-NIKE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-06` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625907453952.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: MAMBA-NIKE
- Implementation versions measured: reference
- Parameter sets: `MAMBA-NIKE-128`, `MAMBA-NIKE-192`, `MAMBA-NIKE-256`, `MAMBA-NIKE-384`, `MAMBA-NIKE-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-06/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MAMBA-NIKE-128` | guide | PASS |
| `MAMBA-NIKE-192` | guide | PASS |
| `MAMBA-NIKE-256` | guide | PASS |
| `MAMBA-NIKE-384` | guide | PASS |
| `MAMBA-NIKE-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `MAMBA-NIKE-128` | exchange | 6.83 M | 2.53 ms | 394 | 2.53 ms | 1245 (5 × 249) |
| `MAMBA-NIKE-128` | init_a | 1.36 M | 503 µs | 1.99e+03 | 503 µs | 1245 (5 × 249) |
| `MAMBA-NIKE-128` | init_b | 1.35 M | 514 µs | 1.94e+03 | 503 µs | 1245 (5 × 249) |
| `MAMBA-NIKE-128` | pass1 | 2.74 M | 1.02 ms | 985 | 1.02 ms | 1245 (5 × 249) |
| `MAMBA-NIKE-128` | derive_a | 4340 | 1.55 µs | 6.45e+05 | 1.55 µs | 1245 (5 × 249) |
| `MAMBA-NIKE-128` | derive_b | 1.38 M | 510 µs | 1.96e+03 | 510 µs | 1245 (5 × 249) |
| `MAMBA-NIKE-192` | exchange | 6.85 M | 2.54 ms | 394 | 2.54 ms | 1240 (5 × 248) |
| `MAMBA-NIKE-192` | init_a | 1.36 M | 505 µs | 1.98e+03 | 505 µs | 1240 (5 × 248) |
| `MAMBA-NIKE-192` | init_b | 1.36 M | 505 µs | 1.98e+03 | 505 µs | 1240 (5 × 248) |
| `MAMBA-NIKE-192` | pass1 | 2.75 M | 1.02 ms | 981 | 1.02 ms | 1240 (5 × 248) |
| `MAMBA-NIKE-192` | derive_a | 4325 | 1.56 µs | 6.39e+05 | 1.57 µs | 1240 (5 × 248) |
| `MAMBA-NIKE-192` | derive_b | 1.38 M | 511 µs | 1.96e+03 | 510 µs | 1240 (5 × 248) |
| `MAMBA-NIKE-256` | exchange | 6.90 M | 2.56 ms | 390 | 2.56 ms | 1225 (5 × 245) |
| `MAMBA-NIKE-256` | init_a | 1.38 M | 512 µs | 1.95e+03 | 512 µs | 1225 (5 × 245) |
| `MAMBA-NIKE-256` | init_b | 1.38 M | 517 µs | 1.93e+03 | 512 µs | 1225 (5 × 245) |
| `MAMBA-NIKE-256` | pass1 | 2.77 M | 1.03 ms | 974 | 1.03 ms | 1225 (5 × 245) |
| `MAMBA-NIKE-256` | derive_a | 4335 | 1.56 µs | 6.4e+05 | 1.56 µs | 1225 (5 × 245) |
| `MAMBA-NIKE-256` | derive_b | 1.38 M | 511 µs | 1.96e+03 | 510 µs | 1225 (5 × 245) |
| `MAMBA-NIKE-384` | exchange | 14.48 M | 5.4 ms | 185 | 5.38 ms | 585 (5 × 117) |
| `MAMBA-NIKE-384` | init_a | 2.87 M | 1.07 ms | 934 | 1.07 ms | 585 (5 × 117) |
| `MAMBA-NIKE-384` | init_b | 2.87 M | 1.07 ms | 934 | 1.07 ms | 585 (5 × 117) |
| `MAMBA-NIKE-384` | pass1 | 5.81 M | 2.16 ms | 464 | 2.16 ms | 585 (5 × 117) |
| `MAMBA-NIKE-384` | derive_a | 8567 | 3.11 µs | 3.21e+05 | 3.12 µs | 585 (5 × 117) |
| `MAMBA-NIKE-384` | derive_b | 2.92 M | 1.1 ms | 913 | 1.09 ms | 585 (5 × 117) |
| `MAMBA-NIKE-512` | exchange | 14.56 M | 5.49 ms | 182 | 5.55 ms | 570 (5 × 114) |
| `MAMBA-NIKE-512` | init_a | 2.90 M | 1.08 ms | 930 | 1.08 ms | 570 (5 × 114) |
| `MAMBA-NIKE-512` | init_b | 2.90 M | 1.08 ms | 929 | 1.08 ms | 570 (5 × 114) |
| `MAMBA-NIKE-512` | pass1 | 5.84 M | 2.17 ms | 462 | 2.17 ms | 570 (5 × 114) |
| `MAMBA-NIKE-512` | derive_a | 8579 | 3.12 µs | 3.2e+05 | 3.12 µs | 570 (5 × 114) |
| `MAMBA-NIKE-512` | derive_b | 2.92 M | 1.09 ms | 917 | 1.09 ms | 570 (5 × 114) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MAMBA-NIKE-128` | exchange | 36856 | 3520 KiB | 3584 KiB |
| `MAMBA-NIKE-128` | init_a | 36856 | 1764 KiB | 1832 KiB |
| `MAMBA-NIKE-128` | init_b | 36856 | 1764 KiB | 3844 KiB |
| `MAMBA-NIKE-128` | pass1 | 36856 | 3528 KiB | 3592 KiB |
| `MAMBA-NIKE-128` | derive_a | 36856 | 1764 KiB | 1832 KiB |
| `MAMBA-NIKE-128` | derive_b | 36856 | 3520 KiB | 3584 KiB |
| `MAMBA-NIKE-192` | exchange | 36816 | 3620 KiB | 3684 KiB |
| `MAMBA-NIKE-192` | init_a | 36816 | 1764 KiB | 1832 KiB |
| `MAMBA-NIKE-192` | init_b | 36816 | 3668 KiB | 3732 KiB |
| `MAMBA-NIKE-192` | pass1 | 36816 | 3504 KiB | 3568 KiB |
| `MAMBA-NIKE-192` | derive_a | 36816 | 1764 KiB | 1832 KiB |
| `MAMBA-NIKE-192` | derive_b | 36816 | 3520 KiB | 3584 KiB |
| `MAMBA-NIKE-256` | exchange | 36816 | 1764 KiB | 1832 KiB |
| `MAMBA-NIKE-256` | init_a | 36816 | 3628 KiB | 3692 KiB |
| `MAMBA-NIKE-256` | init_b | 36816 | 1764 KiB | 3868 KiB |
| `MAMBA-NIKE-256` | pass1 | 36816 | 1764 KiB | 1832 KiB |
| `MAMBA-NIKE-256` | derive_a | 36816 | 1764 KiB | 1832 KiB |
| `MAMBA-NIKE-256` | derive_b | 36816 | 3656 KiB | 3720 KiB |
| `MAMBA-NIKE-384` | exchange | 40880 | 2116 KiB | 4096 KiB |
| `MAMBA-NIKE-384` | init_a | 40880 | 3644 KiB | 3796 KiB |
| `MAMBA-NIKE-384` | init_b | 40880 | 2112 KiB | 3912 KiB |
| `MAMBA-NIKE-384` | pass1 | 40880 | 2116 KiB | 2188 KiB |
| `MAMBA-NIKE-384` | derive_a | 40880 | 2116 KiB | 4076 KiB |
| `MAMBA-NIKE-384` | derive_b | 40880 | 2116 KiB | 4072 KiB |
| `MAMBA-NIKE-512` | exchange | 40880 | 2112 KiB | 4000 KiB |
| `MAMBA-NIKE-512` | init_a | 40880 | 3724 KiB | 3788 KiB |
| `MAMBA-NIKE-512` | init_b | 40880 | 2116 KiB | 2188 KiB |
| `MAMBA-NIKE-512` | pass1 | 40880 | 2116 KiB | 2188 KiB |
| `MAMBA-NIKE-512` | derive_a | 40880 | 2116 KiB | 4156 KiB |
| `MAMBA-NIKE-512` | derive_b | 40880 | 2112 KiB | 4168 KiB |

## 6. Transmission and storage overhead

| instance | passes | messages (bytes) | total | long-term pk / sk | shared secret |
|---|---|---|---|---|---|
| `MAMBA-NIKE-128` | 1 | 1568 | 1568 | 1184 / 3232 | 32 |
| `MAMBA-NIKE-192` | 1 | 1568 | 1568 | 1312 / 3360 | 32 |
| `MAMBA-NIKE-256` | 1 | 1568 | 1568 | 1312 / 3360 | 32 |
| `MAMBA-NIKE-384` | 1 | 3360 | 3360 | 2848 / 6944 | 48 |
| `MAMBA-NIKE-512` | 1 | 3360 | 3360 | 2848 / 6944 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — SHAKE128/256 + ChaCha20; DRNG only with -DKAT_BUILD, otherwise /dev/urandom

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `MAMBA-NIKE-128` | exchange | 0.0% | 0.4% | drng 6 |
| `MAMBA-NIKE-192` | exchange | 0.0% | 0.4% | drng 6 |
| `MAMBA-NIKE-256` | exchange | 0.0% | 0.4% | drng 6 |
| `MAMBA-NIKE-384` | exchange | 0.0% | 0.2% | drng 6 |
| `MAMBA-NIKE-512` | exchange | 0.0% | 0.2% | drng 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MAMBA-NIKE-128` | KAT log (sha256 `e8e1294b89f3dc9d…`) | `kat/kex-06/MAMBA-NIKE-128.log` |
| `MAMBA-NIKE-128` | timing derive_a | `records/kex-06/MAMBA-NIKE-128__derive_a.json` |
| `MAMBA-NIKE-128` | timing derive_b | `records/kex-06/MAMBA-NIKE-128__derive_b.json` |
| `MAMBA-NIKE-128` | timing exchange | `records/kex-06/MAMBA-NIKE-128__exchange.json` |
| `MAMBA-NIKE-128` | timing init_a | `records/kex-06/MAMBA-NIKE-128__init_a.json` |
| `MAMBA-NIKE-128` | timing init_b | `records/kex-06/MAMBA-NIKE-128__init_b.json` |
| `MAMBA-NIKE-128` | timing pass1 | `records/kex-06/MAMBA-NIKE-128__pass1.json` |
| `MAMBA-NIKE-128` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-128__exchange.json` |
| `MAMBA-NIKE-192` | KAT log (sha256 `4893aa52bd3d2c2e…`) | `kat/kex-06/MAMBA-NIKE-192.log` |
| `MAMBA-NIKE-192` | timing derive_a | `records/kex-06/MAMBA-NIKE-192__derive_a.json` |
| `MAMBA-NIKE-192` | timing derive_b | `records/kex-06/MAMBA-NIKE-192__derive_b.json` |
| `MAMBA-NIKE-192` | timing exchange | `records/kex-06/MAMBA-NIKE-192__exchange.json` |
| `MAMBA-NIKE-192` | timing init_a | `records/kex-06/MAMBA-NIKE-192__init_a.json` |
| `MAMBA-NIKE-192` | timing init_b | `records/kex-06/MAMBA-NIKE-192__init_b.json` |
| `MAMBA-NIKE-192` | timing pass1 | `records/kex-06/MAMBA-NIKE-192__pass1.json` |
| `MAMBA-NIKE-192` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-192__exchange.json` |
| `MAMBA-NIKE-256` | KAT log (sha256 `8c4070e20021de1c…`) | `kat/kex-06/MAMBA-NIKE-256.log` |
| `MAMBA-NIKE-256` | timing derive_a | `records/kex-06/MAMBA-NIKE-256__derive_a.json` |
| `MAMBA-NIKE-256` | timing derive_b | `records/kex-06/MAMBA-NIKE-256__derive_b.json` |
| `MAMBA-NIKE-256` | timing exchange | `records/kex-06/MAMBA-NIKE-256__exchange.json` |
| `MAMBA-NIKE-256` | timing init_a | `records/kex-06/MAMBA-NIKE-256__init_a.json` |
| `MAMBA-NIKE-256` | timing init_b | `records/kex-06/MAMBA-NIKE-256__init_b.json` |
| `MAMBA-NIKE-256` | timing pass1 | `records/kex-06/MAMBA-NIKE-256__pass1.json` |
| `MAMBA-NIKE-256` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-256__exchange.json` |
| `MAMBA-NIKE-384` | KAT log (sha256 `bf883e0a813b9569…`) | `kat/kex-06/MAMBA-NIKE-384.log` |
| `MAMBA-NIKE-384` | timing derive_a | `records/kex-06/MAMBA-NIKE-384__derive_a.json` |
| `MAMBA-NIKE-384` | timing derive_b | `records/kex-06/MAMBA-NIKE-384__derive_b.json` |
| `MAMBA-NIKE-384` | timing exchange | `records/kex-06/MAMBA-NIKE-384__exchange.json` |
| `MAMBA-NIKE-384` | timing init_a | `records/kex-06/MAMBA-NIKE-384__init_a.json` |
| `MAMBA-NIKE-384` | timing init_b | `records/kex-06/MAMBA-NIKE-384__init_b.json` |
| `MAMBA-NIKE-384` | timing pass1 | `records/kex-06/MAMBA-NIKE-384__pass1.json` |
| `MAMBA-NIKE-384` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-384__exchange.json` |
| `MAMBA-NIKE-512` | KAT log (sha256 `f6fd1a56cbfb5a0b…`) | `kat/kex-06/MAMBA-NIKE-512.log` |
| `MAMBA-NIKE-512` | timing derive_a | `records/kex-06/MAMBA-NIKE-512__derive_a.json` |
| `MAMBA-NIKE-512` | timing derive_b | `records/kex-06/MAMBA-NIKE-512__derive_b.json` |
| `MAMBA-NIKE-512` | timing exchange | `records/kex-06/MAMBA-NIKE-512__exchange.json` |
| `MAMBA-NIKE-512` | timing init_a | `records/kex-06/MAMBA-NIKE-512__init_a.json` |
| `MAMBA-NIKE-512` | timing init_b | `records/kex-06/MAMBA-NIKE-512__init_b.json` |
| `MAMBA-NIKE-512` | timing pass1 | `records/kex-06/MAMBA-NIKE-512__pass1.json` |
| `MAMBA-NIKE-512` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

