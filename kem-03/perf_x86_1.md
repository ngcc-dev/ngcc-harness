# kem-03 BAG-Loong — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-03` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843247898624.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BAG-Loong
- Implementation versions measured: reference
- Parameter sets: `BAG-Loong-128`, `BAG-Loong-256`, `BAG-Loong-384`, `BAG-Loong-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-03/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BAG-Loong-128` | guide | PASS |
| `BAG-Loong-256` | guide | PASS |
| `BAG-Loong-384` | guide | PASS |
| `BAG-Loong-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BAG-Loong-128` | keygen | 57.12 M | 27.4 ms | 36.5 | 27.4 ms | 185 (5 × 37) |
| `BAG-Loong-128` | enc | 54.70 M | 26.3 ms | 38.1 | 26.3 ms | 190 (5 × 38) |
| `BAG-Loong-128` | dec | 99.48 M | 47.8 ms | 20.9 | 47.8 ms | 105 (5 × 21) |
| `BAG-Loong-256` | keygen | 149.90 M | 72 ms | 13.9 | 71.9 ms | 100 (5 × 20) |
| `BAG-Loong-256` | enc | 128.42 M | 61.7 ms | 16.2 | 61.7 ms | 100 (5 × 20) |
| `BAG-Loong-256` | dec | 251.28 M | 121 ms | 8.28 | 121 ms | 100 (5 × 20) |
| `BAG-Loong-384` | keygen | 347.64 M | 167 ms | 6 | 167 ms | 100 (5 × 20) |
| `BAG-Loong-384` | enc | 264.60 M | 127 ms | 7.87 | 127 ms | 100 (5 × 20) |
| `BAG-Loong-384` | dec | 514.28 M | 247 ms | 4.05 | 247 ms | 100 (5 × 20) |
| `BAG-Loong-512` | keygen | 589.88 M | 284 ms | 3.52 | 282 ms | 100 (5 × 20) |
| `BAG-Loong-512` | enc | 391.51 M | 188 ms | 5.32 | 188 ms | 100 (5 × 20) |
| `BAG-Loong-512` | dec | 749.64 M | 363 ms | 2.76 | 360 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BAG-Loong-128` | keygen | 51185 | 1648 KiB | 2464 KiB |
| `BAG-Loong-128` | enc | 51185 | 2280 KiB | 2472 KiB |
| `BAG-Loong-128` | dec | 51185 | 2268 KiB | 2508 KiB |
| `BAG-Loong-256` | keygen | 51505 | 1748 KiB | 3364 KiB |
| `BAG-Loong-256` | enc | 51505 | 2452 KiB | 3356 KiB |
| `BAG-Loong-256` | dec | 51505 | 2440 KiB | 3360 KiB |
| `BAG-Loong-384` | keygen | 51473 | 1760 KiB | 4104 KiB |
| `BAG-Loong-384` | enc | 51473 | 2608 KiB | 4140 KiB |
| `BAG-Loong-384` | dec | 51473 | 2596 KiB | 4108 KiB |
| `BAG-Loong-512` | keygen | 51473 | 1804 KiB | 4876 KiB |
| `BAG-Loong-512` | enc | 51473 | 2580 KiB | 4864 KiB |
| `BAG-Loong-512` | dec | 51473 | 2600 KiB | 4924 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `BAG-Loong-128` | 2500 | 5064 | 3071 | 16 |
| `BAG-Loong-256` | 6597 | 13258 | 8400 | 32 |
| `BAG-Loong-384` | 12152 | 24368 | 15112 | 48 |
| `BAG-Loong-512` | 19043 | 38150 | 21660 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `loong_hash_tagged_sm3_256` wraps sm3hash

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BAG-Loong-128` | keygen | 69% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-128` | enc | 79% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-128` | dec | 69% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |
| `BAG-Loong-256` | keygen | 49% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-256` | enc | 61% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-256` | dec | 54% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |
| `BAG-Loong-384` | keygen | 45% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-384` | enc | 59% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-384` | dec | 56% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |
| `BAG-Loong-512` | keygen | 36% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-512` | enc | 54% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-512` | dec | 53% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BAG-Loong-128` | KAT log (sha256 `c98ec3233f7913ec…`) | `kat/kem-03/BAG-Loong-128.log` |
| `BAG-Loong-128` | timing dec | `records/kem-03/BAG-Loong-128__dec.json` |
| `BAG-Loong-128` | timing enc | `records/kem-03/BAG-Loong-128__enc.json` |
| `BAG-Loong-128` | timing keygen | `records/kem-03/BAG-Loong-128__keygen.json` |
| `BAG-Loong-128` | hash profile dec | `profile/kem-03/BAG-Loong-128__dec.json` |
| `BAG-Loong-128` | hash profile enc | `profile/kem-03/BAG-Loong-128__enc.json` |
| `BAG-Loong-128` | hash profile keygen | `profile/kem-03/BAG-Loong-128__keygen.json` |
| `BAG-Loong-256` | KAT log (sha256 `2ca0e09af3be45af…`) | `kat/kem-03/BAG-Loong-256.log` |
| `BAG-Loong-256` | timing dec | `records/kem-03/BAG-Loong-256__dec.json` |
| `BAG-Loong-256` | timing enc | `records/kem-03/BAG-Loong-256__enc.json` |
| `BAG-Loong-256` | timing keygen | `records/kem-03/BAG-Loong-256__keygen.json` |
| `BAG-Loong-256` | hash profile dec | `profile/kem-03/BAG-Loong-256__dec.json` |
| `BAG-Loong-256` | hash profile enc | `profile/kem-03/BAG-Loong-256__enc.json` |
| `BAG-Loong-256` | hash profile keygen | `profile/kem-03/BAG-Loong-256__keygen.json` |
| `BAG-Loong-384` | KAT log (sha256 `67bbb75e76ca31b1…`) | `kat/kem-03/BAG-Loong-384.log` |
| `BAG-Loong-384` | timing dec | `records/kem-03/BAG-Loong-384__dec.json` |
| `BAG-Loong-384` | timing enc | `records/kem-03/BAG-Loong-384__enc.json` |
| `BAG-Loong-384` | timing keygen | `records/kem-03/BAG-Loong-384__keygen.json` |
| `BAG-Loong-384` | hash profile dec | `profile/kem-03/BAG-Loong-384__dec.json` |
| `BAG-Loong-384` | hash profile enc | `profile/kem-03/BAG-Loong-384__enc.json` |
| `BAG-Loong-384` | hash profile keygen | `profile/kem-03/BAG-Loong-384__keygen.json` |
| `BAG-Loong-512` | KAT log (sha256 `71329a98dd7d4ffd…`) | `kat/kem-03/BAG-Loong-512.log` |
| `BAG-Loong-512` | timing dec | `records/kem-03/BAG-Loong-512__dec.json` |
| `BAG-Loong-512` | timing enc | `records/kem-03/BAG-Loong-512__enc.json` |
| `BAG-Loong-512` | timing keygen | `records/kem-03/BAG-Loong-512__keygen.json` |
| `BAG-Loong-512` | hash profile dec | `profile/kem-03/BAG-Loong-512__dec.json` |
| `BAG-Loong-512` | hash profile enc | `profile/kem-03/BAG-Loong-512__enc.json` |
| `BAG-Loong-512` | hash profile keygen | `profile/kem-03/BAG-Loong-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

