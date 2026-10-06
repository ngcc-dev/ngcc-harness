# kem-25 NEV — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-25` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560863040819200.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: NEV
- Implementation versions measured: reference
- Parameter sets: `NEV_512_769_C_ICCS`, `NEV_512_769_ICCS`, `NEV_512_1409_ICCS`, `NEV_512_3329_ICCS`, `NEV_1024_769_C_ICCS`, `NEV_1024_769_ICCS`, `NEV_1024_1409_ICCS`, `NEV_1024_3329_ICCS`, `NEV_2048_769_C_ICCS`, `NEV_2048_769_ICCS`, `NEV_2048_1409_ICCS`, `NEV_2048_3329_ICCS`

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
| `NEV_512_769_C_ICCS` | keygen | 95.3 k | 45.5 µs | 2.2e+04 | 45.6 µs | 87325 (5 × 17465) |
| `NEV_512_769_C_ICCS` | enc | 70.2 k | 33.5 µs | 2.98e+04 | 33.6 µs | 100000 (5 × 20000) |
| `NEV_512_769_C_ICCS` | dec | 85.0 k | 40.6 µs | 2.46e+04 | 40.6 µs | 100000 (5 × 20000) |
| `NEV_512_769_ICCS` | keygen | 94.8 k | 45.3 µs | 2.21e+04 | 45.3 µs | 87350 (5 × 17470) |
| `NEV_512_769_ICCS` | enc | 80.0 k | 38.2 µs | 2.62e+04 | 38.2 µs | 100000 (5 × 20000) |
| `NEV_512_769_ICCS` | dec | 85.9 k | 41 µs | 2.44e+04 | 41 µs | 100000 (5 × 20000) |
| `NEV_512_1409_ICCS` | keygen | 126.8 k | 60.6 µs | 1.65e+04 | 60.6 µs | 70380 (5 × 14076) |
| `NEV_512_1409_ICCS` | enc | 95.2 k | 45.5 µs | 2.2e+04 | 45.4 µs | 96400 (5 × 19280) |
| `NEV_512_1409_ICCS` | dec | 95.1 k | 45.4 µs | 2.2e+04 | 45.4 µs | 93485 (5 × 18697) |
| `NEV_512_3329_ICCS` | keygen | 166.9 k | 79.7 µs | 1.25e+04 | 79.7 µs | 55195 (5 × 11039) |
| `NEV_512_3329_ICCS` | enc | 152.8 k | 73 µs | 1.37e+04 | 72.9 µs | 61845 (5 × 12369) |
| `NEV_512_3329_ICCS` | dec | 149.8 k | 71.6 µs | 1.4e+04 | 71.6 µs | 62940 (5 × 12588) |
| `NEV_1024_769_C_ICCS` | keygen | 205.1 k | 98 µs | 1.02e+04 | 98 µs | 45265 (5 × 9053) |
| `NEV_1024_769_C_ICCS` | enc | 149.2 k | 71.3 µs | 1.4e+04 | 71.2 µs | 64745 (5 × 12949) |
| `NEV_1024_769_C_ICCS` | dec | 188.8 k | 90.2 µs | 1.11e+04 | 90.2 µs | 51730 (5 × 10346) |
| `NEV_1024_769_ICCS` | keygen | 206.1 k | 98.5 µs | 1.02e+04 | 98.5 µs | 45450 (5 × 9090) |
| `NEV_1024_769_ICCS` | enc | 165.7 k | 79.2 µs | 1.26e+04 | 79.2 µs | 59490 (5 × 11898) |
| `NEV_1024_769_ICCS` | dec | 186.6 k | 89.1 µs | 1.12e+04 | 89.2 µs | 51760 (5 × 10352) |
| `NEV_1024_1409_ICCS` | keygen | 253.5 k | 121 µs | 8.26e+03 | 121 µs | 37455 (5 × 7491) |
| `NEV_1024_1409_ICCS` | enc | 173.8 k | 83 µs | 1.2e+04 | 83 µs | 57380 (5 × 11476) |
| `NEV_1024_1409_ICCS` | dec | 188.2 k | 89.9 µs | 1.11e+04 | 89.7 µs | 52575 (5 × 10515) |
| `NEV_1024_3329_ICCS` | keygen | 263.4 k | 126 µs | 7.95e+03 | 126 µs | 35520 (5 × 7104) |
| `NEV_1024_3329_ICCS` | enc | 227.5 k | 109 µs | 9.2e+03 | 109 µs | 43625 (5 × 8725) |
| `NEV_1024_3329_ICCS` | dec | 229.3 k | 110 µs | 9.13e+03 | 110 µs | 43145 (5 × 8629) |
| `NEV_2048_769_C_ICCS` | keygen | 620.7 k | 296 µs | 3.37e+03 | 296 µs | 16290 (5 × 3258) |
| `NEV_2048_769_C_ICCS` | enc | 364.5 k | 174 µs | 5.74e+03 | 174 µs | 21280 (5 × 4256) |
| `NEV_2048_769_C_ICCS` | dec | 401.2 k | 192 µs | 5.22e+03 | 192 µs | 24705 (5 × 4941) |
| `NEV_2048_769_ICCS` | keygen | 622.6 k | 297 µs | 3.36e+03 | 297 µs | 15975 (5 × 3195) |
| `NEV_2048_769_ICCS` | enc | 409.9 k | 196 µs | 5.11e+03 | 196 µs | 24285 (5 × 4857) |
| `NEV_2048_769_ICCS` | dec | 413.3 k | 197 µs | 5.06e+03 | 198 µs | 24610 (5 × 4922) |
| `NEV_2048_1409_ICCS` | keygen | 672.5 k | 321 µs | 3.11e+03 | 321 µs | 15090 (5 × 3018) |
| `NEV_2048_1409_ICCS` | enc | 481.8 k | 230 µs | 4.34e+03 | 230 µs | 20820 (5 × 4164) |
| `NEV_2048_1409_ICCS` | dec | 466.8 k | 223 µs | 4.48e+03 | 223 µs | 21040 (5 × 4208) |
| `NEV_2048_3329_ICCS` | keygen | 720.2 k | 344 µs | 2.91e+03 | 344 µs | 13905 (5 × 2781) |
| `NEV_2048_3329_ICCS` | enc | 575.6 k | 275 µs | 3.64e+03 | 275 µs | 17700 (5 × 3540) |
| `NEV_2048_3329_ICCS` | dec | 525.9 k | 251 µs | 3.98e+03 | 251 µs | 18975 (5 × 3795) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NEV_512_769_C_ICCS` | keygen | 40097 | 1696 KiB | 1788 KiB |
| `NEV_512_769_C_ICCS` | enc | 40097 | 1716 KiB | 1808 KiB |
| `NEV_512_769_C_ICCS` | dec | 40097 | 1700 KiB | 1804 KiB |
| `NEV_512_769_ICCS` | keygen | 40105 | 1700 KiB | 1804 KiB |
| `NEV_512_769_ICCS` | enc | 40105 | 1684 KiB | 1808 KiB |
| `NEV_512_769_ICCS` | dec | 40105 | 1704 KiB | 1808 KiB |
| `NEV_512_1409_ICCS` | keygen | 44961 | 1676 KiB | 1808 KiB |
| `NEV_512_1409_ICCS` | enc | 44961 | 1720 KiB | 1808 KiB |
| `NEV_512_1409_ICCS` | dec | 44961 | 1724 KiB | 1812 KiB |
| `NEV_512_3329_ICCS` | keygen | 40545 | 1720 KiB | 1788 KiB |
| `NEV_512_3329_ICCS` | enc | 40545 | 1700 KiB | 1820 KiB |
| `NEV_512_3329_ICCS` | dec | 40545 | 1680 KiB | 1808 KiB |
| `NEV_1024_769_C_ICCS` | keygen | 46209 | 1696 KiB | 1828 KiB |
| `NEV_1024_769_C_ICCS` | enc | 46209 | 1724 KiB | 1820 KiB |
| `NEV_1024_769_C_ICCS` | dec | 46209 | 1724 KiB | 1820 KiB |
| `NEV_1024_769_ICCS` | keygen | 46209 | 1708 KiB | 1808 KiB |
| `NEV_1024_769_ICCS` | enc | 46209 | 1740 KiB | 1824 KiB |
| `NEV_1024_769_ICCS` | dec | 46209 | 1728 KiB | 1832 KiB |
| `NEV_1024_1409_ICCS` | keygen | 51785 | 1724 KiB | 1828 KiB |
| `NEV_1024_1409_ICCS` | enc | 51785 | 1748 KiB | 1812 KiB |
| `NEV_1024_1409_ICCS` | dec | 51785 | 1748 KiB | 1812 KiB |
| `NEV_1024_3329_ICCS` | keygen | 45433 | 1716 KiB | 1804 KiB |
| `NEV_1024_3329_ICCS` | enc | 45433 | 1740 KiB | 1828 KiB |
| `NEV_1024_3329_ICCS` | dec | 45433 | 1708 KiB | 1828 KiB |
| `NEV_2048_769_C_ICCS` | keygen | 53657 | 1720 KiB | 1824 KiB |
| `NEV_2048_769_C_ICCS` | enc | 53657 | 1748 KiB | 1816 KiB |
| `NEV_2048_769_C_ICCS` | dec | 53657 | 1760 KiB | 1860 KiB |
| `NEV_2048_769_ICCS` | keygen | 53657 | 1720 KiB | 1824 KiB |
| `NEV_2048_769_ICCS` | enc | 53657 | 1680 KiB | 1812 KiB |
| `NEV_2048_769_ICCS` | dec | 53657 | 1756 KiB | 1864 KiB |
| `NEV_2048_1409_ICCS` | keygen | 59009 | 1720 KiB | 1864 KiB |
| `NEV_2048_1409_ICCS` | enc | 59009 | 1792 KiB | 1888 KiB |
| `NEV_2048_1409_ICCS` | dec | 59009 | 1764 KiB | 1864 KiB |
| `NEV_2048_3329_ICCS` | keygen | 52817 | 1736 KiB | 1864 KiB |
| `NEV_2048_3329_ICCS` | enc | 52817 | 1748 KiB | 1872 KiB |
| `NEV_2048_3329_ICCS` | dec | 52817 | 1756 KiB | 1860 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

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
| `NEV_512_769_C_ICCS` | keygen | 35% | 4.8% | drng 1, pseudoXOF 4 |
| `NEV_512_769_C_ICCS` | enc | 39% | 6.5% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NEV_512_769_C_ICCS` | dec | 16% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `NEV_512_769_ICCS` | keygen | 34% | 4.8% | drng 1, pseudoXOF 4 |
| `NEV_512_769_ICCS` | enc | 43% | 5.7% | drng 1, pseudoXOF 2, sm3hash 6 |
| `NEV_512_769_ICCS` | dec | 24% | 0.0% | pseudoXOF 1, sm3hash 6 |
| `NEV_512_1409_ICCS` | keygen | 42% | 3.6% | drng 1, pseudoXOF 4 |
| `NEV_512_1409_ICCS` | enc | 54% | 4.7% | drng 1, pseudoXOF 3, sm3hash 1 |
| `NEV_512_1409_ICCS` | dec | 39% | 0.0% | pseudoXOF 2, sm3hash 1 |
| `NEV_512_3329_ICCS` | keygen | 61% | 2.8% | drng 1, pseudoXOF 4 |
| `NEV_512_3329_ICCS` | enc | 67% | 3.1% | drng 1, pseudoXOF 3, sm3hash 1 |
| `NEV_512_3329_ICCS` | dec | 56% | 0.0% | pseudoXOF 2, sm3hash 1 |
| `NEV_1024_769_C_ICCS` | keygen | 29% | 2.3% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_769_C_ICCS` | enc | 41% | 3.1% | drng 1, pseudoXOF 1, pseudohash 1, sm3hash 1 |
| `NEV_1024_769_C_ICCS` | dec | 18% | 0.0% | pseudoXOF 1, pseudohash 1 |
| `NEV_1024_769_ICCS` | keygen | 28% | 2.3% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_769_ICCS` | enc | 44% | 2.8% | drng 1, pseudoXOF 1, pseudohash 1, sm3hash 9 |
| `NEV_1024_769_ICCS` | dec | 25% | 0.0% | pseudoXOF 1, pseudohash 1, sm3hash 8 |
| `NEV_1024_1409_ICCS` | keygen | 32% | 1.9% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_1409_ICCS` | enc | 51% | 2.7% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `NEV_1024_1409_ICCS` | dec | 31% | 0.0% | pseudoXOF 2, pseudohash 1 |
| `NEV_1024_3329_ICCS` | keygen | 49% | 1.8% | drng 1, pseudoXOF 2, sm3hash 2 |
| `NEV_1024_3329_ICCS` | enc | 61% | 2.0% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `NEV_1024_3329_ICCS` | dec | 46% | 0.0% | pseudoXOF 2, pseudohash 1 |
| `NEV_2048_769_C_ICCS` | keygen | 43% | 1.0% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_769_C_ICCS` | enc | 52% | 1.7% | drng 1, pseudoXOF 1, pseudohash 2 |
| `NEV_2048_769_C_ICCS` | dec | 19% | 0.0% | pseudoXOF 1, pseudohash 1 |
| `NEV_2048_769_ICCS` | keygen | 43% | 1.0% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_769_ICCS` | enc | 56% | 1.5% | drng 1, pseudoXOF 1, pseudohash 2, sm3hash 14 |
| `NEV_2048_769_ICCS` | dec | 28% | 0.0% | pseudoXOF 1, pseudohash 1, sm3hash 14 |
| `NEV_2048_1409_ICCS` | keygen | 32% | 0.9% | drng 1, pseudohash 2, sm3hash 28 |
| `NEV_2048_1409_ICCS` | enc | 59% | 1.3% | drng 1, pseudoXOF 1, pseudohash 2, sm3hash 14 |
| `NEV_2048_1409_ICCS` | dec | 34% | 0.0% | pseudoXOF 1, pseudohash 1, sm3hash 14 |
| `NEV_2048_3329_ICCS` | keygen | 53% | 0.9% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_3329_ICCS` | enc | 69% | 1.1% | drng 1, pseudoXOF 2, pseudohash 2 |
| `NEV_2048_3329_ICCS` | dec | 49% | 0.0% | pseudoXOF 2, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NEV_512_769_C_ICCS` | KAT log (sha256 `ab4f882f97a2af69…`) | `kat/kem-25/NEV_512_769_C_ICCS.log` |
| `NEV_512_769_C_ICCS` | timing dec | `records/kem-25/NEV_512_769_C_ICCS__dec.json` |
| `NEV_512_769_C_ICCS` | timing enc | `records/kem-25/NEV_512_769_C_ICCS__enc.json` |
| `NEV_512_769_C_ICCS` | timing keygen | `records/kem-25/NEV_512_769_C_ICCS__keygen.json` |
| `NEV_512_769_C_ICCS` | hash profile dec | `profile/kem-25/NEV_512_769_C_ICCS__dec.json` |
| `NEV_512_769_C_ICCS` | hash profile enc | `profile/kem-25/NEV_512_769_C_ICCS__enc.json` |
| `NEV_512_769_C_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_769_C_ICCS__keygen.json` |
| `NEV_512_769_ICCS` | KAT log (sha256 `95e41144304f9a40…`) | `kat/kem-25/NEV_512_769_ICCS.log` |
| `NEV_512_769_ICCS` | timing dec | `records/kem-25/NEV_512_769_ICCS__dec.json` |
| `NEV_512_769_ICCS` | timing enc | `records/kem-25/NEV_512_769_ICCS__enc.json` |
| `NEV_512_769_ICCS` | timing keygen | `records/kem-25/NEV_512_769_ICCS__keygen.json` |
| `NEV_512_769_ICCS` | hash profile dec | `profile/kem-25/NEV_512_769_ICCS__dec.json` |
| `NEV_512_769_ICCS` | hash profile enc | `profile/kem-25/NEV_512_769_ICCS__enc.json` |
| `NEV_512_769_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_769_ICCS__keygen.json` |
| `NEV_512_1409_ICCS` | KAT log (sha256 `059936feb33aa813…`) | `kat/kem-25/NEV_512_1409_ICCS.log` |
| `NEV_512_1409_ICCS` | timing dec | `records/kem-25/NEV_512_1409_ICCS__dec.json` |
| `NEV_512_1409_ICCS` | timing enc | `records/kem-25/NEV_512_1409_ICCS__enc.json` |
| `NEV_512_1409_ICCS` | timing keygen | `records/kem-25/NEV_512_1409_ICCS__keygen.json` |
| `NEV_512_1409_ICCS` | hash profile dec | `profile/kem-25/NEV_512_1409_ICCS__dec.json` |
| `NEV_512_1409_ICCS` | hash profile enc | `profile/kem-25/NEV_512_1409_ICCS__enc.json` |
| `NEV_512_1409_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_1409_ICCS__keygen.json` |
| `NEV_512_3329_ICCS` | KAT log (sha256 `9d1488be623b6abc…`) | `kat/kem-25/NEV_512_3329_ICCS.log` |
| `NEV_512_3329_ICCS` | timing dec | `records/kem-25/NEV_512_3329_ICCS__dec.json` |
| `NEV_512_3329_ICCS` | timing enc | `records/kem-25/NEV_512_3329_ICCS__enc.json` |
| `NEV_512_3329_ICCS` | timing keygen | `records/kem-25/NEV_512_3329_ICCS__keygen.json` |
| `NEV_512_3329_ICCS` | hash profile dec | `profile/kem-25/NEV_512_3329_ICCS__dec.json` |
| `NEV_512_3329_ICCS` | hash profile enc | `profile/kem-25/NEV_512_3329_ICCS__enc.json` |
| `NEV_512_3329_ICCS` | hash profile keygen | `profile/kem-25/NEV_512_3329_ICCS__keygen.json` |
| `NEV_1024_769_C_ICCS` | KAT log (sha256 `55200868c20a0102…`) | `kat/kem-25/NEV_1024_769_C_ICCS.log` |
| `NEV_1024_769_C_ICCS` | timing dec | `records/kem-25/NEV_1024_769_C_ICCS__dec.json` |
| `NEV_1024_769_C_ICCS` | timing enc | `records/kem-25/NEV_1024_769_C_ICCS__enc.json` |
| `NEV_1024_769_C_ICCS` | timing keygen | `records/kem-25/NEV_1024_769_C_ICCS__keygen.json` |
| `NEV_1024_769_C_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_769_C_ICCS__dec.json` |
| `NEV_1024_769_C_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_769_C_ICCS__enc.json` |
| `NEV_1024_769_C_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_769_C_ICCS__keygen.json` |
| `NEV_1024_769_ICCS` | KAT log (sha256 `f31fcdc3ec3453c3…`) | `kat/kem-25/NEV_1024_769_ICCS.log` |
| `NEV_1024_769_ICCS` | timing dec | `records/kem-25/NEV_1024_769_ICCS__dec.json` |
| `NEV_1024_769_ICCS` | timing enc | `records/kem-25/NEV_1024_769_ICCS__enc.json` |
| `NEV_1024_769_ICCS` | timing keygen | `records/kem-25/NEV_1024_769_ICCS__keygen.json` |
| `NEV_1024_769_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_769_ICCS__dec.json` |
| `NEV_1024_769_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_769_ICCS__enc.json` |
| `NEV_1024_769_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_769_ICCS__keygen.json` |
| `NEV_1024_1409_ICCS` | KAT log (sha256 `b2943b97346e8bee…`) | `kat/kem-25/NEV_1024_1409_ICCS.log` |
| `NEV_1024_1409_ICCS` | timing dec | `records/kem-25/NEV_1024_1409_ICCS__dec.json` |
| `NEV_1024_1409_ICCS` | timing enc | `records/kem-25/NEV_1024_1409_ICCS__enc.json` |
| `NEV_1024_1409_ICCS` | timing keygen | `records/kem-25/NEV_1024_1409_ICCS__keygen.json` |
| `NEV_1024_1409_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_1409_ICCS__dec.json` |
| `NEV_1024_1409_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_1409_ICCS__enc.json` |
| `NEV_1024_1409_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_1409_ICCS__keygen.json` |
| `NEV_1024_3329_ICCS` | KAT log (sha256 `bc5dc1a22253f5d3…`) | `kat/kem-25/NEV_1024_3329_ICCS.log` |
| `NEV_1024_3329_ICCS` | timing dec | `records/kem-25/NEV_1024_3329_ICCS__dec.json` |
| `NEV_1024_3329_ICCS` | timing enc | `records/kem-25/NEV_1024_3329_ICCS__enc.json` |
| `NEV_1024_3329_ICCS` | timing keygen | `records/kem-25/NEV_1024_3329_ICCS__keygen.json` |
| `NEV_1024_3329_ICCS` | hash profile dec | `profile/kem-25/NEV_1024_3329_ICCS__dec.json` |
| `NEV_1024_3329_ICCS` | hash profile enc | `profile/kem-25/NEV_1024_3329_ICCS__enc.json` |
| `NEV_1024_3329_ICCS` | hash profile keygen | `profile/kem-25/NEV_1024_3329_ICCS__keygen.json` |
| `NEV_2048_769_C_ICCS` | KAT log (sha256 `9f76708ff15954a7…`) | `kat/kem-25/NEV_2048_769_C_ICCS.log` |
| `NEV_2048_769_C_ICCS` | timing dec | `records/kem-25/NEV_2048_769_C_ICCS__dec.json` |
| `NEV_2048_769_C_ICCS` | timing enc | `records/kem-25/NEV_2048_769_C_ICCS__enc.json` |
| `NEV_2048_769_C_ICCS` | timing keygen | `records/kem-25/NEV_2048_769_C_ICCS__keygen.json` |
| `NEV_2048_769_C_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_769_C_ICCS__dec.json` |
| `NEV_2048_769_C_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_769_C_ICCS__enc.json` |
| `NEV_2048_769_C_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_769_C_ICCS__keygen.json` |
| `NEV_2048_769_ICCS` | KAT log (sha256 `6b847d142ec61c2b…`) | `kat/kem-25/NEV_2048_769_ICCS.log` |
| `NEV_2048_769_ICCS` | timing dec | `records/kem-25/NEV_2048_769_ICCS__dec.json` |
| `NEV_2048_769_ICCS` | timing enc | `records/kem-25/NEV_2048_769_ICCS__enc.json` |
| `NEV_2048_769_ICCS` | timing keygen | `records/kem-25/NEV_2048_769_ICCS__keygen.json` |
| `NEV_2048_769_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_769_ICCS__dec.json` |
| `NEV_2048_769_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_769_ICCS__enc.json` |
| `NEV_2048_769_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_769_ICCS__keygen.json` |
| `NEV_2048_1409_ICCS` | KAT log (sha256 `ea8628c7ee234c6f…`) | `kat/kem-25/NEV_2048_1409_ICCS.log` |
| `NEV_2048_1409_ICCS` | timing dec | `records/kem-25/NEV_2048_1409_ICCS__dec.json` |
| `NEV_2048_1409_ICCS` | timing enc | `records/kem-25/NEV_2048_1409_ICCS__enc.json` |
| `NEV_2048_1409_ICCS` | timing keygen | `records/kem-25/NEV_2048_1409_ICCS__keygen.json` |
| `NEV_2048_1409_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_1409_ICCS__dec.json` |
| `NEV_2048_1409_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_1409_ICCS__enc.json` |
| `NEV_2048_1409_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_1409_ICCS__keygen.json` |
| `NEV_2048_3329_ICCS` | KAT log (sha256 `722e3d30ee304045…`) | `kat/kem-25/NEV_2048_3329_ICCS.log` |
| `NEV_2048_3329_ICCS` | timing dec | `records/kem-25/NEV_2048_3329_ICCS__dec.json` |
| `NEV_2048_3329_ICCS` | timing enc | `records/kem-25/NEV_2048_3329_ICCS__enc.json` |
| `NEV_2048_3329_ICCS` | timing keygen | `records/kem-25/NEV_2048_3329_ICCS__keygen.json` |
| `NEV_2048_3329_ICCS` | hash profile dec | `profile/kem-25/NEV_2048_3329_ICCS__dec.json` |
| `NEV_2048_3329_ICCS` | hash profile enc | `profile/kem-25/NEV_2048_3329_ICCS__enc.json` |
| `NEV_2048_3329_ICCS` | hash profile keygen | `profile/kem-25/NEV_2048_3329_ICCS__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

