# Constant-time review: Amoeba

Secret inputs are the PKE key in `sk`, recovered plaintext `m`, fallback seed, and shared secret. The public key and ciphertext are public. This traces the Amoeba-576 reference source; the Hamming decoder and KEM wrapper are duplicated across reference sets. It is a source-level finding without a measured timing or key-recovery experiment.

`src/backend/ccakem.c:60-80` decrypts with `sk`, returns early on decoder failure, and branches on the re-encryption comparison. Within the PKE decoder, `src/backend/cpapke.c:438-447` passes the secret-derived plaintext bits to `decode_ECC`; `src/backend/hamming.c:131` branches per decoded codeword bit, and `hamming.c:157-183` branches on the syndrome and indexes `syndrome_map[s_h_key]` at line 173. The syndrome is derived from the result of PKE secret-key decryption. Thus the lookup address and control flow are secret-dependent even before the KEM's validity branch. Public length and parameter decisions are not the finding.

The existing `kem-02-1` report demonstrates a separate full-key-recovery bug in the faulty ciphertext comparison; this CT trace does not claim a second full extraction. See `report.md` for the bounded side-channel finding.

Division follow-up: after secret-key decryption, `src/backend/ccakem.c:74` re-encrypts recovered `m`; `cpapke.c:303-319` applies `mod_down` to this secret-derived path. GCC `-O3 -march=native` emits `idiv` instructions in `mod_down` (five in a standalone Amoeba-576 build here). The divisor `q` is a function parameter, though public. This is not an attack on a public-only compression path. `ccakem.c:61-62` also exposes explicit decoder failure. Neither the extra timing predicate nor that failure bit has been shown here to yield a second key-recovery attack beyond `kem-02-1`.

Recheck from the repository root: set `ref=kem-02/Implementations/Reference_Implementation/Amoeba-576/src`, then run `cc -O3 -march=native -fPIC -std=gnu11 -w -fwrapv -DSECURITY_LEVEL=128 -I"$ref/symmetric" -I"$ref/backend" -I"$ref/api" -Iapi -S "$ref/backend/cpapke.c" -o - | rg -c '\bidiv'`; this standalone build prints `5`. The review's count of 11 used a different compilation context; the presence of hardware division, not that count, is the security-relevant observation.

Size-build leads: GCC `-Os` also emits division in `decode_MSB` (`cpapke.c:178-181`) on the decryption path and in `PolyMul`/`PolyMod` (`poly.c:60,89-95`) on secret-key products. These require separate observable-channel and attack analysis.
