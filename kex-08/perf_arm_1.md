# kex-08 NIIKE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-08` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560626184278016.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: NIIKE
- Implementation versions measured: reference
- Parameter sets: `NIIKE-lv128`, `NIIKE-lv256`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-08/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NIIKE-lv128` | guide | PASS |
| `NIIKE-lv256` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NIIKE-lv128` | exchange | 53.18 G | 19.7 s | 0.0507 | 19.7 s | 15 (5 × 3) |
| `NIIKE-lv128` | init_a | 14.53 G | 5.39 s | 0.185 | 5.38 s | 15 (5 × 3) |
| `NIIKE-lv128` | init_b | 14.68 G | 5.45 s | 0.184 | 5.41 s | 15 (5 × 3) |
| `NIIKE-lv128` | derive_a | 12.06 G | 4.47 s | 0.223 | 4.47 s | 15 (5 × 3) |
| `NIIKE-lv128` | derive_b | 12.15 G | 4.51 s | 0.222 | 4.49 s | 15 (5 × 3) |
| `NIIKE-lv256` | exchange | 848.72 G | 315 s | 0.00317 | 315 s | 1 (1 × 1) |
| `NIIKE-lv256` | init_a | 222.60 G | 83 s | 0.012 | 83 s | 1 (1 × 1) |
| `NIIKE-lv256` | init_b | 222.69 G | 83.1 s | 0.012 | 83.1 s | 1 (1 × 1) |
| `NIIKE-lv256` | derive_a | 201.98 G | 75 s | 0.0133 | 75 s | 1 (1 × 1) |
| `NIIKE-lv256` | derive_b | 202.12 G | 75 s | 0.0133 | 75 s | 1 (1 × 1) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NIIKE-lv128` | exchange | 692528 | 2580 KiB | 2652 KiB |
| `NIIKE-lv128` | init_a | 692528 | 2580 KiB | 2652 KiB |
| `NIIKE-lv128` | init_b | 692528 | 2580 KiB | 4580 KiB |
| `NIIKE-lv128` | derive_a | 692528 | 2576 KiB | 2648 KiB |
| `NIIKE-lv128` | derive_b | 692528 | 2580 KiB | 2652 KiB |
| `NIIKE-lv256` | exchange | 997992 | 4820 KiB | 4888 KiB |
| `NIIKE-lv256` | init_a | 997992 | 4308 KiB | 6356 KiB |
| `NIIKE-lv256` | init_b | 997992 | 4248 KiB | 6360 KiB |
| `NIIKE-lv256` | derive_a | 997992 | 4820 KiB | 4888 KiB |
| `NIIKE-lv256` | derive_b | 997992 | 4632 KiB | 6680 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

NIIKE sends no separate protocol message but requires both public keys. The specification lists 4,030/8,943-byte keys, while the submitted external encodings are 4,160/8,960 bytes; this table uses the latter.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `NIIKE-lv128` | 0 | 0 / 0 / 0 | 0 | 4160 / 4160 | 8320 | 196 | 64 |
| `NIIKE-lv256` | 0 | 0 / 0 / 0 | 0 | 8960 / 8960 | 17920 | 336 | 128 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — no hash or KDF at all: raw j-invariant (known kex-08-1)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `NIIKE-lv128` | exchange | 0.0% | 0.0% | drng 174 |
| `NIIKE-lv256` | exchange | 0.0% | 0.0% | drng 296 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NIIKE-lv128` | KAT log (sha256 `9f077a258eb6d903…`) | `kat/kex-08/NIIKE-lv128.log` |
| `NIIKE-lv128` | timing derive_a | `records/kex-08/NIIKE-lv128__derive_a.json` |
| `NIIKE-lv128` | timing derive_b | `records/kex-08/NIIKE-lv128__derive_b.json` |
| `NIIKE-lv128` | timing exchange | `records/kex-08/NIIKE-lv128__exchange.json` |
| `NIIKE-lv128` | timing init_a | `records/kex-08/NIIKE-lv128__init_a.json` |
| `NIIKE-lv128` | timing init_b | `records/kex-08/NIIKE-lv128__init_b.json` |
| `NIIKE-lv128` | hash profile exchange | `profile/kex-08/NIIKE-lv128__exchange.json` |
| `NIIKE-lv256` | KAT log (sha256 `de0f3d574c7621e6…`) | `kat/kex-08/NIIKE-lv256.log` |
| `NIIKE-lv256` | timing derive_a | `records/kex-08/NIIKE-lv256__derive_a.json` |
| `NIIKE-lv256` | timing derive_b | `records/kex-08/NIIKE-lv256__derive_b.json` |
| `NIIKE-lv256` | timing exchange | `records/kex-08/NIIKE-lv256__exchange.json` |
| `NIIKE-lv256` | timing init_a | `records/kex-08/NIIKE-lv256__init_a.json` |
| `NIIKE-lv256` | timing init_b | `records/kex-08/NIIKE-lv256__init_b.json` |
| `NIIKE-lv256` | hash profile exchange | `profile/kex-08/NIIKE-lv256__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

