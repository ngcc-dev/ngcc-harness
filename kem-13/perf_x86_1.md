# kem-13 DKEM (Ding Key Encapsulation) — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-13` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844552327168.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: DKEM (Ding Key Encapsulation)
- Implementation versions measured: reference
- Parameter sets: `DKEM-128`, `DKEM-256`, `DKEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-13/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `DKEM-128` | guide | PASS |
| `DKEM-256` | guide | PASS |
| `DKEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `DKEM-128` | keygen | 199.1 k | 95.1 µs | 1.05e+04 | 95.1 µs | 46600 (5 × 9320) |
| `DKEM-128` | enc | 226.9 k | 108 µs | 9.23e+03 | 108 µs | 42680 (5 × 8536) |
| `DKEM-128` | dec | 254.3 k | 121 µs | 8.23e+03 | 121 µs | 38045 (5 × 7609) |
| `DKEM-256` | keygen | 594.4 k | 284 µs | 3.52e+03 | 284 µs | 16560 (5 × 3312) |
| `DKEM-256` | enc | 606.5 k | 290 µs | 3.45e+03 | 290 µs | 16495 (5 × 3299) |
| `DKEM-256` | dec | 653.3 k | 312 µs | 3.2e+03 | 312 µs | 15280 (5 × 3056) |
| `DKEM-512` | keygen | 2.04 M | 974 µs | 1.03e+03 | 973 µs | 4950 (5 × 990) |
| `DKEM-512` | enc | 2.12 M | 1.01 ms | 988 | 1.01 ms | 4795 (5 × 959) |
| `DKEM-512` | dec | 2.23 M | 1.07 ms | 938 | 1.07 ms | 4585 (5 × 917) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DKEM-128` | keygen | 32573 | 1712 KiB | 1800 KiB |
| `DKEM-128` | enc | 32573 | 1708 KiB | 1808 KiB |
| `DKEM-128` | dec | 32573 | 1720 KiB | 1812 KiB |
| `DKEM-256` | keygen | 33637 | 1692 KiB | 1816 KiB |
| `DKEM-256` | enc | 33637 | 1720 KiB | 1808 KiB |
| `DKEM-256` | dec | 33637 | 1736 KiB | 1820 KiB |
| `DKEM-512` | keygen | 34957 | 1716 KiB | 1836 KiB |
| `DKEM-512` | enc | 34957 | 1764 KiB | 1852 KiB |
| `DKEM-512` | dec | 34957 | 1720 KiB | 1844 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `DKEM-128` | 800 | 1600 | 800 | 32 |
| `DKEM-256` | 1568 | 3136 | 1600 | 32 |
| `DKEM-512` | 3392 | 6784 | 3136 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — default DKE_HASH=0; own SM3/HMAC compiled but unreachable; DKE_HASH=2 would switch to SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `DKEM-128` | keygen | 65% | 3.0% | drng 1, pseudoXOF 1, sm3hash 88.2 |
| `DKEM-128` | enc | 62% | 2.0% | drng 1, pseudoXOF 1, sm3hash 95 |
| `DKEM-128` | dec | 56% | 0.0% | pseudoXOF 1, sm3hash 96 |
| `DKEM-256` | keygen | 70% | 1.0% | drng 1, pseudoXOF 1, sm3hash 289 |
| `DKEM-256` | enc | 69% | 0.8% | drng 1, pseudoXOF 1, sm3hash 293 |
| `DKEM-256` | dec | 64% | 0.0% | pseudoXOF 1, sm3hash 294 |
| `DKEM-512` | keygen | 82% | 0.4% | drng 1, pseudoXOF 1, sm3hash 608 |
| `DKEM-512` | enc | 82% | 0.3% | drng 1, pseudoXOF 1, pseudohash 1, sm3hash 620 |
| `DKEM-512` | dec | 77% | 0.0% | pseudoXOF 1, pseudohash 2, sm3hash 620 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DKEM-128` | KAT log (sha256 `5d562c75d5e0e73f…`) | `kat/kem-13/DKEM-128.log` |
| `DKEM-128` | timing dec | `records/kem-13/DKEM-128__dec.json` |
| `DKEM-128` | timing enc | `records/kem-13/DKEM-128__enc.json` |
| `DKEM-128` | timing keygen | `records/kem-13/DKEM-128__keygen.json` |
| `DKEM-128` | hash profile dec | `profile/kem-13/DKEM-128__dec.json` |
| `DKEM-128` | hash profile enc | `profile/kem-13/DKEM-128__enc.json` |
| `DKEM-128` | hash profile keygen | `profile/kem-13/DKEM-128__keygen.json` |
| `DKEM-256` | KAT log (sha256 `4d40d7612360bd24…`) | `kat/kem-13/DKEM-256.log` |
| `DKEM-256` | timing dec | `records/kem-13/DKEM-256__dec.json` |
| `DKEM-256` | timing enc | `records/kem-13/DKEM-256__enc.json` |
| `DKEM-256` | timing keygen | `records/kem-13/DKEM-256__keygen.json` |
| `DKEM-256` | hash profile dec | `profile/kem-13/DKEM-256__dec.json` |
| `DKEM-256` | hash profile enc | `profile/kem-13/DKEM-256__enc.json` |
| `DKEM-256` | hash profile keygen | `profile/kem-13/DKEM-256__keygen.json` |
| `DKEM-512` | KAT log (sha256 `d5f1e14ee51a703e…`) | `kat/kem-13/DKEM-512.log` |
| `DKEM-512` | timing dec | `records/kem-13/DKEM-512__dec.json` |
| `DKEM-512` | timing enc | `records/kem-13/DKEM-512__enc.json` |
| `DKEM-512` | timing keygen | `records/kem-13/DKEM-512__keygen.json` |
| `DKEM-512` | hash profile dec | `profile/kem-13/DKEM-512__dec.json` |
| `DKEM-512` | hash profile enc | `profile/kem-13/DKEM-512__enc.json` |
| `DKEM-512` | hash profile keygen | `profile/kem-13/DKEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

