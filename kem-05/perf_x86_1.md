# kem-05 BIKE-MLThre — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-05` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843495362560.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BIKE-MLThre
- Implementation versions measured: reference
- Parameter sets: `BIKE_v2_128`, `BIKE_v2_256`, `BIKE_v2_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-05/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BIKE_v2_128` | guide | PASS |
| `BIKE_v2_256` | guide | PASS |
| `BIKE_v2_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BIKE_v2_128` | keygen | 1.45 G | 691 ms | 1.45 | 691 ms | 100 (5 × 20) |
| `BIKE_v2_128` | enc | 13.48 M | 6.47 ms | 155 | 6.46 ms | 950 (5 × 190) |
| `BIKE_v2_128` | dec | 88.85 M | 42.8 ms | 23.3 | 42.8 ms | 120 (5 × 24) |
| `BIKE_v2_256` | keygen | 15.62 G | 7.5 s | 0.133 | 7.5 s | 100 (5 × 20) |
| `BIKE_v2_256` | enc | 88.31 M | 42.2 ms | 23.7 | 42.4 ms | 130 (5 × 26) |
| `BIKE_v2_256` | dec | 454.96 M | 218 ms | 4.59 | 218 ms | 100 (5 × 20) |
| `BIKE_v2_512` | keygen | 211.25 G | 101 s | 0.00986 | 101 s | 5 (5 × 1) |
| `BIKE_v2_512` | enc | 660.20 M | 317 ms | 3.16 | 315 ms | 100 (5 × 20) |
| `BIKE_v2_512` | dec | 3.27 G | 1.57 s | 0.636 | 1.57 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BIKE_v2_128` | keygen | 71313 | 3996 KiB | 6708 KiB |
| `BIKE_v2_128` | enc | 71313 | 6632 KiB | 6696 KiB |
| `BIKE_v2_128` | dec | 71313 | 6780 KiB | 6872 KiB |
| `BIKE_v2_256` | keygen | 128497 | 4056 KiB | 6788 KiB |
| `BIKE_v2_256` | enc | 128497 | 6696 KiB | 6764 KiB |
| `BIKE_v2_256` | dec | 128497 | 7148 KiB | 7224 KiB |
| `BIKE_v2_512` | keygen | 346521 | 4104 KiB | 6920 KiB |
| `BIKE_v2_512` | enc | 346521 | 6820 KiB | 6892 KiB |
| `BIKE_v2_512` | dec | 346521 | 8688 KiB | 8824 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `BIKE_v2_128` | 1541 | 3114 | 1573 | 32 |
| `BIKE_v2_256` | 5122 | 10276 | 5154 | 32 |
| `BIKE_v2_512` | 18751 | 37566 | 18815 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — hash/XOF via ICCS; seeds from NIST AES-256-CTR-DRBG (OpenSSL) only with -DNIST_RAND, otherwise libc rand()

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BIKE_v2_128` | keygen | 0.0% | 0.0% | drng 1, pseudoXOF 1 |
| `BIKE_v2_128` | enc | 1.2% | 0.1% | drng 1, pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_128` | dec | 0.2% | 0.0% | pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_256` | keygen | 0.0% | 0.0% | drng 1, pseudoXOF 1 |
| `BIKE_v2_256` | enc | 0.5% | 0.0% | drng 1, pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_256` | dec | 0.1% | 0.0% | pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_512` | keygen | 0.0% | 0.0% | drng 1, pseudoXOF 1 |
| `BIKE_v2_512` | enc | 0.4% | 0.0% | drng 1, pseudoXOF 1, pseudohash 2 |
| `BIKE_v2_512` | dec | 0.1% | 0.0% | pseudoXOF 1, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BIKE_v2_128` | KAT log (sha256 `eaf955d2d6739b44…`) | `kat/kem-05/BIKE_v2_128.log` |
| `BIKE_v2_128` | timing dec | `records/kem-05/BIKE_v2_128__dec.json` |
| `BIKE_v2_128` | timing enc | `records/kem-05/BIKE_v2_128__enc.json` |
| `BIKE_v2_128` | timing keygen | `records/kem-05/BIKE_v2_128__keygen.json` |
| `BIKE_v2_128` | hash profile dec | `profile/kem-05/BIKE_v2_128__dec.json` |
| `BIKE_v2_128` | hash profile enc | `profile/kem-05/BIKE_v2_128__enc.json` |
| `BIKE_v2_128` | hash profile keygen | `profile/kem-05/BIKE_v2_128__keygen.json` |
| `BIKE_v2_256` | KAT log (sha256 `e9a8fb8ee88cbd6f…`) | `kat/kem-05/BIKE_v2_256.log` |
| `BIKE_v2_256` | timing dec | `records/kem-05/BIKE_v2_256__dec.json` |
| `BIKE_v2_256` | timing enc | `records/kem-05/BIKE_v2_256__enc.json` |
| `BIKE_v2_256` | timing keygen | `records/kem-05/BIKE_v2_256__keygen.json` |
| `BIKE_v2_256` | hash profile dec | `profile/kem-05/BIKE_v2_256__dec.json` |
| `BIKE_v2_256` | hash profile enc | `profile/kem-05/BIKE_v2_256__enc.json` |
| `BIKE_v2_256` | hash profile keygen | `profile/kem-05/BIKE_v2_256__keygen.json` |
| `BIKE_v2_512` | KAT log (sha256 `16e22ef8d7848fd6…`) | `kat/kem-05/BIKE_v2_512.log` |
| `BIKE_v2_512` | timing dec | `records/kem-05/BIKE_v2_512__dec.json` |
| `BIKE_v2_512` | timing enc | `records/kem-05/BIKE_v2_512__enc.json` |
| `BIKE_v2_512` | timing keygen | `records/kem-05/BIKE_v2_512__keygen.json` |
| `BIKE_v2_512` | hash profile dec | `profile/kem-05/BIKE_v2_512__dec.json` |
| `BIKE_v2_512` | hash profile enc | `profile/kem-05/BIKE_v2_512__enc.json` |
| `BIKE_v2_512` | hash profile keygen | `profile/kem-05/BIKE_v2_512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

