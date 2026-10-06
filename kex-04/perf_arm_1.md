# kex-04 DKEX (Ding Key Exchange) — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-04` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625651601408.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: DKEX (Ding Key Exchange)
- Implementation versions measured: reference
- Parameter sets: `DKEX-128`, `DKEX-256`, `DKEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-04/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `DKEX-128` | guide | PASS |
| `DKEX-256` | guide | PASS |
| `DKEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `DKEX-128` | exchange | 3.17 M | 1.18 ms | 849 | 1.18 ms | 3040 (5 × 608) |
| `DKEX-128` | init_a | 215.5 k | 79.9 µs | 1.25e+04 | 80 µs | 3040 (5 × 608) |
| `DKEX-128` | init_b | 216.1 k | 80.1 µs | 1.25e+04 | 80 µs | 3040 (5 × 608) |
| `DKEX-128` | pass1 | 173.4 k | 64.3 µs | 1.56e+04 | 64.1 µs | 3040 (5 × 608) |
| `DKEX-128` | pass2 | 1.13 M | 420 µs | 2.38e+03 | 419 µs | 3040 (5 × 608) |
| `DKEX-128` | pass3 | 1.20 M | 447 µs | 2.24e+03 | 447 µs | 3040 (5 × 608) |
| `DKEX-128` | derive_a | 2854 | 1.01 µs | 9.91e+05 | 1.01 µs | 3040 (5 × 608) |
| `DKEX-128` | derive_b | 234.6 k | 87 µs | 1.15e+04 | 87 µs | 3040 (5 × 608) |
| `DKEX-256` | exchange | 7.27 M | 2.7 ms | 371 | 2.7 ms | 1015 (5 × 203) |
| `DKEX-256` | init_a | 600.4 k | 223 µs | 4.49e+03 | 223 µs | 1015 (5 × 203) |
| `DKEX-256` | init_b | 599.8 k | 223 µs | 4.49e+03 | 223 µs | 1015 (5 × 203) |
| `DKEX-256` | pass1 | 532.4 k | 197 µs | 5.06e+03 | 197 µs | 1015 (5 × 203) |
| `DKEX-256` | pass2 | 2.40 M | 892 µs | 1.12e+03 | 891 µs | 1015 (5 × 203) |
| `DKEX-256` | pass3 | 2.51 M | 930 µs | 1.07e+03 | 930 µs | 1015 (5 × 203) |
| `DKEX-256` | derive_a | 2856 | 1.01 µs | 9.92e+05 | 1.01 µs | 1015 (5 × 203) |
| `DKEX-256` | derive_b | 625.3 k | 232 µs | 4.31e+03 | 232 µs | 1015 (5 × 203) |
| `DKEX-512` | exchange | 10.57 M | 3.92 ms | 255 | 3.92 ms | 840 (5 × 168) |
| `DKEX-512` | init_a | 600.9 k | 223 µs | 4.49e+03 | 223 µs | 840 (5 × 168) |
| `DKEX-512` | init_b | 600.4 k | 223 µs | 4.49e+03 | 223 µs | 840 (5 × 168) |
| `DKEX-512` | pass1 | 1.86 M | 691 µs | 1.45e+03 | 691 µs | 840 (5 × 168) |
| `DKEX-512` | pass2 | 4.09 M | 1.52 ms | 659 | 1.52 ms | 840 (5 × 168) |
| `DKEX-512` | pass3 | 2.77 M | 1.03 ms | 972 | 1.03 ms | 840 (5 × 168) |
| `DKEX-512` | derive_a | 8226 | 3.02 µs | 3.31e+05 | 3.02 µs | 840 (5 × 168) |
| `DKEX-512` | derive_b | 630.8 k | 234 µs | 4.27e+03 | 234 µs | 840 (5 × 168) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DKEX-128` | exchange | 51892 | 3620 KiB | 3684 KiB |
| `DKEX-128` | init_a | 51892 | 1604 KiB | 1668 KiB |
| `DKEX-128` | init_b | 51892 | 1604 KiB | 1668 KiB |
| `DKEX-128` | pass1 | 51892 | 1604 KiB | 1668 KiB |
| `DKEX-128` | pass2 | 51892 | 3544 KiB | 3608 KiB |
| `DKEX-128` | pass3 | 51892 | 1604 KiB | 1668 KiB |
| `DKEX-128` | derive_a | 51892 | 3620 KiB | 3684 KiB |
| `DKEX-128` | derive_b | 51892 | 1604 KiB | 1668 KiB |
| `DKEX-256` | exchange | 52748 | 1780 KiB | 1844 KiB |
| `DKEX-256` | init_a | 52748 | 1780 KiB | 1844 KiB |
| `DKEX-256` | init_b | 52748 | 1776 KiB | 1840 KiB |
| `DKEX-256` | pass1 | 52748 | 1780 KiB | 1844 KiB |
| `DKEX-256` | pass2 | 52748 | 1776 KiB | 1840 KiB |
| `DKEX-256` | pass3 | 52748 | 3692 KiB | 3756 KiB |
| `DKEX-256` | derive_a | 52748 | 1780 KiB | 1844 KiB |
| `DKEX-256` | derive_b | 52748 | 1780 KiB | 1844 KiB |
| `DKEX-512` | exchange | 53740 | 3864 KiB | 3928 KiB |
| `DKEX-512` | init_a | 53740 | 3616 KiB | 3680 KiB |
| `DKEX-512` | init_b | 53740 | 1836 KiB | 1912 KiB |
| `DKEX-512` | pass1 | 53740 | 1840 KiB | 1916 KiB |
| `DKEX-512` | pass2 | 53740 | 1840 KiB | 1916 KiB |
| `DKEX-512` | pass3 | 53740 | 3712 KiB | 3776 KiB |
| `DKEX-512` | derive_a | 53740 | 1840 KiB | 1916 KiB |
| `DKEX-512` | derive_b | 53740 | 3716 KiB | 3780 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `DKEX-128` | 3 | 800 / 3188 / 2420 | 6408 | 1312 / 1312 | 9032 | 2560 | 32 |
| `DKEX-256` | 3 | 1568 / 6195 / 4627 | 12390 | 2592 / 2592 | 17574 | 4896 | 32 |
| `DKEX-512` | 3 | 3392 / 7699 / 4627 | 15718 | 2592 / 2592 | 20902 | 4896 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — KEX core via ICCS; embedded ML-DSA uses its own SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `DKEX-128` | exchange | 14% | 0.6% | drng 4, pseudoXOF 1, sm3hash 186 |
| `DKEX-256` | exchange | 16% | 0.3% | drng 4, pseudoXOF 1, sm3hash 585 |
| `DKEX-512` | exchange | 40% | 0.2% | drng 4, pseudoXOF 5, sm3hash 1.23e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DKEX-128` | KAT log (sha256 `6de87a5477330449…`) | `kat/kex-04/DKEX-128.log` |
| `DKEX-128` | timing derive_a | `records/kex-04/DKEX-128__derive_a.json` |
| `DKEX-128` | timing derive_b | `records/kex-04/DKEX-128__derive_b.json` |
| `DKEX-128` | timing exchange | `records/kex-04/DKEX-128__exchange.json` |
| `DKEX-128` | timing init_a | `records/kex-04/DKEX-128__init_a.json` |
| `DKEX-128` | timing init_b | `records/kex-04/DKEX-128__init_b.json` |
| `DKEX-128` | timing pass1 | `records/kex-04/DKEX-128__pass1.json` |
| `DKEX-128` | timing pass2 | `records/kex-04/DKEX-128__pass2.json` |
| `DKEX-128` | timing pass3 | `records/kex-04/DKEX-128__pass3.json` |
| `DKEX-128` | hash profile exchange | `profile/kex-04/DKEX-128__exchange.json` |
| `DKEX-256` | KAT log (sha256 `46c525733f5ec01c…`) | `kat/kex-04/DKEX-256.log` |
| `DKEX-256` | timing derive_a | `records/kex-04/DKEX-256__derive_a.json` |
| `DKEX-256` | timing derive_b | `records/kex-04/DKEX-256__derive_b.json` |
| `DKEX-256` | timing exchange | `records/kex-04/DKEX-256__exchange.json` |
| `DKEX-256` | timing init_a | `records/kex-04/DKEX-256__init_a.json` |
| `DKEX-256` | timing init_b | `records/kex-04/DKEX-256__init_b.json` |
| `DKEX-256` | timing pass1 | `records/kex-04/DKEX-256__pass1.json` |
| `DKEX-256` | timing pass2 | `records/kex-04/DKEX-256__pass2.json` |
| `DKEX-256` | timing pass3 | `records/kex-04/DKEX-256__pass3.json` |
| `DKEX-256` | hash profile exchange | `profile/kex-04/DKEX-256__exchange.json` |
| `DKEX-512` | KAT log (sha256 `b1fcb3f7d7440d71…`) | `kat/kex-04/DKEX-512.log` |
| `DKEX-512` | timing derive_a | `records/kex-04/DKEX-512__derive_a.json` |
| `DKEX-512` | timing derive_b | `records/kex-04/DKEX-512__derive_b.json` |
| `DKEX-512` | timing exchange | `records/kex-04/DKEX-512__exchange.json` |
| `DKEX-512` | timing init_a | `records/kex-04/DKEX-512__init_a.json` |
| `DKEX-512` | timing init_b | `records/kex-04/DKEX-512__init_b.json` |
| `DKEX-512` | timing pass1 | `records/kex-04/DKEX-512__pass1.json` |
| `DKEX-512` | timing pass2 | `records/kex-04/DKEX-512__pass2.json` |
| `DKEX-512` | timing pass3 | `records/kex-04/DKEX-512__pass3.json` |
| `DKEX-512` | hash profile exchange | `profile/kex-04/DKEX-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

