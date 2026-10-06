# kex-08 NIIKE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-08` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560626184278016.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: NIIKE
- Implementation versions measured: reference
- Parameter sets: `NIIKE-lv128`, `NIIKE-lv256`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-08/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NIIKE-lv128` | guide | PASS |
| `NIIKE-lv256` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NIIKE-lv128` | exchange | 52.91 G | 25.5 s | 0.0393 | 25.5 s | 35 (5 × 7) |
| `NIIKE-lv128` | init_a | 14.44 G | 6.93 s | 0.144 | 6.93 s | 35 (5 × 7) |
| `NIIKE-lv128` | init_b | 14.44 G | 6.92 s | 0.144 | 6.93 s | 35 (5 × 7) |
| `NIIKE-lv128` | derive_a | 12.01 G | 5.76 s | 0.173 | 5.76 s | 35 (5 × 7) |
| `NIIKE-lv128` | derive_b | 12.01 G | 5.76 s | 0.173 | 5.76 s | 35 (5 × 7) |
| `NIIKE-lv256` | exchange | 935.98 G | 456 s | 0.00219 | 456 s | 1 (1 × 1) |
| `NIIKE-lv256` | init_a | 245.28 G | 119 s | 0.0084 | 119 s | 1 (1 × 1) |
| `NIIKE-lv256` | init_b | 245.41 G | 119 s | 0.0084 | 119 s | 1 (1 × 1) |
| `NIIKE-lv256` | derive_a | 222.88 G | 108 s | 0.00923 | 108 s | 1 (1 × 1) |
| `NIIKE-lv256` | derive_b | 222.76 G | 108 s | 0.00924 | 108 s | 1 (1 × 1) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NIIKE-lv128` | exchange | 646597 | 2748 KiB | 2820 KiB |
| `NIIKE-lv128` | init_a | 646597 | 2744 KiB | 2816 KiB |
| `NIIKE-lv128` | init_b | 646597 | 2744 KiB | 2816 KiB |
| `NIIKE-lv128` | derive_a | 646597 | 2724 KiB | 2820 KiB |
| `NIIKE-lv128` | derive_b | 646597 | 2744 KiB | 2816 KiB |
| `NIIKE-lv256` | exchange | 972573 | 4236 KiB | 5136 KiB |
| `NIIKE-lv256` | init_a | 972573 | 4096 KiB | 4932 KiB |
| `NIIKE-lv256` | init_b | 972573 | 4160 KiB | 4996 KiB |
| `NIIKE-lv256` | derive_a | 972573 | 4208 KiB | 5044 KiB |
| `NIIKE-lv256` | derive_b | 972573 | 4100 KiB | 4936 KiB |

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

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NIIKE-lv128` | KAT log (sha256 `db5247e7fcb3d1f1…`) | `kat/kex-08/NIIKE-lv128.log` |
| `NIIKE-lv128` | timing derive_a | `records/kex-08/NIIKE-lv128__derive_a.json` |
| `NIIKE-lv128` | timing derive_b | `records/kex-08/NIIKE-lv128__derive_b.json` |
| `NIIKE-lv128` | timing exchange | `records/kex-08/NIIKE-lv128__exchange.json` |
| `NIIKE-lv128` | timing init_a | `records/kex-08/NIIKE-lv128__init_a.json` |
| `NIIKE-lv128` | timing init_b | `records/kex-08/NIIKE-lv128__init_b.json` |
| `NIIKE-lv128` | hash profile exchange | `profile/kex-08/NIIKE-lv128__exchange.json` |
| `NIIKE-lv256` | KAT log (sha256 `7949b138d5b1919b…`) | `kat/kex-08/NIIKE-lv256.log` |
| `NIIKE-lv256` | timing derive_a | `records/kex-08/NIIKE-lv256__derive_a.json` |
| `NIIKE-lv256` | timing derive_b | `records/kex-08/NIIKE-lv256__derive_b.json` |
| `NIIKE-lv256` | timing exchange | `records/kex-08/NIIKE-lv256__exchange.json` |
| `NIIKE-lv256` | timing init_a | `records/kex-08/NIIKE-lv256__init_a.json` |
| `NIIKE-lv256` | timing init_b | `records/kex-08/NIIKE-lv256__init_b.json` |
| `NIIKE-lv256` | hash profile exchange | `profile/kex-08/NIIKE-lv256__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

