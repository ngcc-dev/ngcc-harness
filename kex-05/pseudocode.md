# kex-05 Loom — algorithm summary

Four-pass **mutually authenticated** key exchange of the classical *sign-and-MAC*
(SIGMA-style) shape: one ephemeral KEM exchange for confidentiality, a long-term
signature over the transcript for authentication, and a MAC over the identity for
identity binding. The ephemeral KEM is **Loom-KEM**, a pure Module-LWR (rounding-only,
no explicit error vectors) PKE with *randomized lifting* of the compressed public key,
wrapped not in an FO transform but in a lighter **IND-CPAF ("rigid") KEM**: the
ciphertext carries a tag `t = F_m(c)` and decapsulation **explicitly rejects** (returns
⊥) when the tag does not match. The signature is **Shuttle** (an external NGCC
submission, hash-and-sign over lattices). Hardness: Shifted-MLWR for confidentiality,
MSIS (via Shuttle) for authentication.

Specification: `kex-05-spec.pdf` (58 pages). §2.2 AKE.KeyGen/Init/Respond/AuthInit/
AuthResp/Confirm (Algorithms 1–6), §2.2.7 KEM (Algorithms 7–15), §2.3 symmetric
components + KDF+ (Algorithm 16, Table 3), §2.4 Tables 2, 4, 5. English.

## Parameters

| parameter | LoomKEX-128 | LoomKEX-256 | LoomKEX-512 | meaning |
|---|---|---|---|---|
| KEM instance | Loom-KEM-128 | Loom-KEM-256 | Loom-KEM-512 | Table 2 |
| SIG instance | SHUTTLE-128 | SHUTTLE-256 | SHUTTLE-512 | Table 2 |
| \|µ\| = 8·len_ss | 128 | 256 | 512 | KEM message / shared-secret bits |
| n | 128 | 256 | 512 | ring degree |
| k | 5 | 4 | 4 | module rank |
| q | 3329 | 7681 | 7681 | modulus |
| ⌈log2 q⌉ | 12 | 13 | 13 | sk coefficient width |
| η1, η2 | 3, 2 | 8, 4 | 10, 8 | CBD widths for s and r |
| (d_t, d_u, d_v) | (9, 8, 4) | (10, 9, 4) | (11, 10, 4) | compression widths |
| DFR | 2^-19.7 | 2^-18.9 | 2^-24.8 | Table 5 — **very high**, see note |
| \|ek\|, \|ct\| | 752, 736 | 1312, 1344 | 2880, 2944 | Table 4 |
| \|σ\| | 1183 | 2417 | 5001 | Table 4 |
| MAC \|τ\| = \|mk\| | 16 | 32 | 64 | Table 4 / Table 3 |
| len_F (rigidity tag) | 32 | 64 | 128 | Table 3 (256/512/1024 bits) |
| len_seed, \|N_I\|=\|N_R\| | 32, 32 | 32, 32 | 64, 64 | Table 3 |
| SPI halves s_I, s_R | 4 + 4 | 4 + 4 | 4 + 4 | 32 bits each (Alg. 2/3) |

Sizes (bytes), specification vs the built reference library:

| instance | passes spec | passes impl | pk spec | pk impl | sk spec | sk impl | st_A / st_B | total msg | ss spec | ss impl |
|---|---|---|---|---|---|---|---|---|---|---|
| LoomKEX-128 | 4 | 4 | (Shuttle) | 1264 | (Shuttle) | 2288 | 2657 / 2657 | 4038 | 16 | 16 |
| LoomKEX-256 | 4 | 4 | (Shuttle) | 1952 | (Shuttle) | 3680 | 4609 / 4609 | 7706 | 32 | 32 |
| LoomKEX-512 | 4 | 4 | (Shuttle) | 3648 | (Shuttle) | 7104 | 9665 / 9665 | 16170 | 64 | 64 |

`pk`/`sk` are the Shuttle verification/signing keys; this spec does not state them (it
defers to the Shuttle submission [16]), so there is nothing to compare against. Every
size that *is* derivable from the spec checks out:
`|ek| = d_t·k·n/8 + len_seed` = 752/1312/2880 ✓ (Table 4) and
`|ct| = (d_u·k + d_v)·n/8 + len_F` = 736/1344/2944 ✓ (Table 4).
The reported `total_msg` is the sum of the four wire messages as this implementation
lays them out:
`|m1| = 8 + len_seed_nonce + |ek|`, `|m2| = 8 + |N| + |ct|`,
`|m3| = |m4| = 2 + 32 + 2 + |σ| + |τ|` → 792+776+1235+1235 = 4038,
1352+1384+2485+2485 = 7706, 2952+3016+5101+5101 = 16170 ✓ (all three reproduce exactly).
`st` is a serialized context (see below): its packed length reproduces 2657/4609/9665 ✓.

**Table 2's "Total Bandwidth" column does not follow its own footnote.** The footnote
says `|ek| + |ct| + 2|σ|`, which gives 3854 / 7490 / 15826, but the table prints
3870 / 7522 / 15986 — an excess of 16 / 32 / **160**. The first two excesses equal |τ|;
the third does not equal anything in Table 4, so at least the 512 row is wrong.

**DFR note.** Table 5's decryption failure rates are only 2^-19.7 … 2^-24.8, and
Algorithm 4 line 6 turns a decapsulation failure into an explicit session abort. So
roughly 1 in 10^6 (level 1) honest handshakes aborts and must be retried. The spec is
open about this (§1, §3.3) and calls it a deliberate trade for aggressive compression,
but it is far outside the usual ≤2^-128 KEM convention and is a property an evaluator
should weigh separately from the security claims.

**Reproduced (`kex-05-1`).** A deterministic search using the submitted optimized
whole-AKE implementation found an honest LoomKEX-256 exchange that aborts pass 3 at
trial 136129. The separately submitted scalar reference implementation produces the
identical keys, states, pass-1/pass-2 messages and `-3` rigid-decapsulation failure.
See [`../security/LOOM_FAILURE_SEARCH.md`](../security/LOOM_FAILURE_SEARCH.md) and the
saved complete witness.

**State-rollback key recovery (`kex-05-2`).** The proof's one-observation-per-ephemeral-key
condition is not intrinsic to the serialized-state API. Restoring the same pass-1 state around
chosen pass-2 calls gives a reusable decoder-boundary oracle. A complete exploit recovers all
1,024 LoomKEX-256 ephemeral-secret coefficients in 4,532 pass-3 queries, decapsulates an
honest responder ciphertext, and predicts the final AKE shared secret. This result is explicitly
conditional on snapshot rollback, state cloning, or concurrent evaluation; a strictly linear
deployment that irrevocably commits the first accepted query is not broken by this witness.
The public witness is built by the [`exploit` Makefile target](Makefile).

## Pseudocode

### Loom-KEM: IND-CPAF ("rigid") KEM over the MLWR PKE (Algorithms 7–9)
```
KGen():  (ek, dk) = Loom.CPAPKE.KeyGen()                 # dk = Encode_{⌈log2 q⌉}(ŝ)
Encap(ek):
    m, r ←$ {0,1}^{8·len_ss}
    c = Loom.CPAPKE.Enc(ek, m; r)
    t = F_m(c)                                           # rigidity tag, m used as the key
    return (ct = c ‖ t,  K = G(m))
Decap(dk, ct = c ‖ t):
    m' = Loom.CPAPKE.Dec(dk, c)
    if m' = ⊥ or F_{m'}(c) ≠ t:  return ⊥                # EXPLICIT rejection, no FO
    return K = G(m')
```
The underlying PKE is the same rounding-only construction as kem-39 Weaver: KeyGen
computes `t = NTT^-1(Â ∘ ŝ)` with **no error vector** and publishes
`Compress_q(t, d_t) ‖ seedA`; Enc lifts the compressed public key with the *randomized*
`Inv(t', d_t; PRF(seed1, ctr))` (Algorithm 12) rather than a canonical Decompress, samples
r ← CBD_η2, and emits `Compress(NTT^-1(Â^T∘r̂), d_u)` and `Compress(v0 + MsgEncode(µ), d_v)`;
Dec uses the deterministic Decompress and MsgDecode (Algorithms 10, 11, 13, 14, 15).

### KDF+ (Algorithm 16)
```
SS  = KDF(K, salt ‖ 0x01)          with salt = N_I ‖ N_R
mkI = KDF(K, SS  ‖ salt ‖ 0x02)
mkR = KDF(K, mkI ‖ salt ‖ 0x03)
```

### Loom-AKE — pass structure (Algorithms 1–6, 4 passes)

The specification defines **4 passes**; OBSERVED reports `passes=4`. Agreement.

```
Init_A / Init_B  (AKE.KeyGen, Algorithm 1)
    (vk_U, sk_U) ← SIG.KGen(1^λ)                  # Shuttle long-term identity key
    st_U ← ε                                      # no session state yet

pass 1  I → R   (AKE.Init, Algorithm 2)
    s_I ←$ {0,1}^32 ;  N_I ←$ {0,1}^256
    (ek, dk) ← KEM.KGen(1^λ)                      # EPHEMERAL KEM key pair
    s' = s_I ‖ 0                                  # "half-open" SPI
    msg_{I,1} = (s', ek, N_I)
    st_I = (msg_{I,1}, dk)

pass 2  R → I   (AKE.Respond, Algorithm 3)
    s_R ←$ {0,1}^32 ;  N_R ←$ {0,1}^256
    (s', ek, N_I) ← msg_{I,1}
    (K, ct_kem) ← KEM.Encap(ek)
    s = s_I ‖ s_R                                 # SPI completed
    msg_{R,1} = (s, ct_kem, N_R)
    st_R = (msg_{I,1}, msg_{R,1}, K)

pass 3  I → R   (AKE.AuthInit, Algorithm 4)
    ensure s'(s_I) == s(s_I)
    K' ← KEM.Decap(dk, ct_kem)
    if K' = ⊥: return ⊥                           # rigid failure -> abort the session
    (SS, mk_I, mk_R) ← KDF+(K', N_I ‖ N_R)
    DTBS_I = "0" ‖ (s_I, s_R) ‖ ek ‖ ct_kem
    σ_I ← SIG.Sign(sk_I, DTBS_I)
    τ_I ← MAC(mk_I, "0" ‖ (s_I, s_R) ‖ ID_I)
    msg_{I,2} = (ID_I, σ_I, τ_I)
    st'_I = (msg_{R,1}, SS, mk_R)

pass 4  R → I   (AKE.AuthResp, Algorithm 5)     # final pass
    (SS, mk_I, mk_R) ← KDF+(K, N_I ‖ N_R)
    DTBS_I = "0" ‖ (s_I, s_R) ‖ ek ‖ ct_kem
    if SIG.Verify(vk_I, DTBS_I, σ_I) ≠ 1
       or MAC(mk_I, "0" ‖ (s_I,s_R) ‖ ID_I) ≠ τ_I:  return ⊥
    DTBS_R = "1" ‖ (s_I, s_R) ‖ ct_kem ‖ ek        # note the swapped ek/ct order
    σ_R ← SIG.Sign(sk_R, DTBS_R)
    τ_R ← MAC(mk_R, "1" ‖ (s_I, s_R) ‖ ID_R)
    msg_{R,2} = (ID_R, σ_R, τ_R)                   # R accepts SS here

DeriveSS_A  (AKE.Confirm, Algorithm 6)
    DTBS_R = "1" ‖ (s_I, s_R) ‖ ct_kem ‖ ek
    if SIG.Verify(vk_R, DTBS_R, σ_R) ≠ 1
       or MAC(mk_R, "1" ‖ (s_I,s_R) ‖ ID_R) ≠ τ_R:  return ⊥
    return SS

DeriveSS_B
    return SS                                      # already accepted at the end of pass 4
```

## Implementation vs specification

Checked: `loom/params.h`, `loom/kemparams.h`, `loom/sigparams.h`, `loom/loom.c`
(Algorithms 1–6), `loom/prf_mac.c` (KDF+, MAC), `loom/state_serialize.c`,
`loom/KEX_LoomKEX-128.c` (ICCS mapping), `kem/kem_cpaf.c` (Algorithms 7–9),
`kem/indcpa.c` + `kem/poly_invq.c` + `kem/msgenc.c` (Algorithms 10–15),
`loom/symmetric.h` + `loom/symmetric-iccs.c`. The build selects `-DLOOM_MODE=N`, the
ICCS SM3 symmetric path (`symmetric-iccs.c`, with `fips202.c`/`symmetric-shake.c` and
the OpenSSL `rng.c` excluded), and the scalar reference KEM/SIG cores.

Agreements:
- **Every Table-5 parameter matches** `kemparams.h` for all three rows: n = 128/256/512,
  k = 5/4/4, q = 3329/7681/7681, η1 = 3/8/10, η2 = 2/4/8,
  (d_t,d_u,d_v) = (9,8,4)/(10,9,4)/(11,10,4), |µ| = 128/256/512 bits. `WEAVER_TAGBYTES`
  = 32/64/128 equals Table 3's len_F, `LOOM_MACBYTES` = 16/32/64 equals Table 4's |τ|,
  `LOOM_NONCEBYTES` = 32/32/64 and the seed lengths match Table 3, and
  `LOOM_SPIHALFBYTES` = 4 matches the 32-bit SPI halves of Algorithms 2–3.
  Derived |ek| and |ct| reproduce Table 4 exactly.
- The rigid KEM is Algorithms 7–9 verbatim: `crypto_kem_enc_derand` writes
  `ct = c ‖ F_m(c)` with `F` keyed by prefixing m, sets `K = G(m)`, and
  `crypto_kem_dec_rigid` recomputes the tag, compares it with the constant-time
  `verify()` and **returns −1 on mismatch** — explicit, not implicit, rejection ✓.
  `LOOM_KEM_SKBYTES` = ⌈log2 q⌉·k·n/8 = 960/1664/3328.
- **Both signature verifications and both MAC verifications are present, correctly
  ordered, and not inverted** (`loom_auth_ver` in `loom.c`: parse → peer-ID compare →
  `crypto_sign_verify` → (responder only) derive the PRF keys → `loom_mac_verify`).
  `loom_mac_verify` is a constant-time XOR-accumulate compare.
- **The state machine actually enforces the protocol order.** Each pass packs the
  context into a versioned blob (`"LSTA"`, version, suite, role, stage) and each pass
  unpacks with an *expected* role and stage; `loom_kex_ctx_unpack` rejects a wrong
  magic, version, suite, role or stage, and `crypto_loom_finalize` refuses unless
  `ctx->state == 4`. So `kex_derive_ss_b` cannot emit a session key unless pass 4's
  signature and MAC checks succeeded — the failure mode that kex-02 has is absent here.
  The context is securely wiped (`loom_kex_ctx_secure_wipe`) on every path.
- The role/label bytes are right in every one of the four uses: signing uses
  `1 − initiator`, verifying uses `initiator`, so I always signs under "0" and R under
  "1", and `ek ‖ ct` vs `ct ‖ ek` is swapped exactly as Algorithms 4 and 5 require.
- All randomness comes from the seeded DRNG: `loom.c:16` defines
  `randombytes()` over `get_random_number(&drng_algorithm, …)`; the OpenSSL AES-CTR-DRBG
  in `loom/rng.c` is not compiled.

Discrepancies:
1. **(a) non-canonical encoding — DTBS puts the SPI before the role label.**
   Algorithms 4 and 5 define `DTBS_I = "0" ‖ (s_I,s_R) ‖ ek ‖ ct` and
   `DTBS_R = "1" ‖ (s_I,s_R) ‖ ct ‖ ek`. `loom.c` `build_DTBS` (lines 33–64) writes
   **`SPI ‖ role ‖ …`** — the SPI first, then the one-byte label. The function's own
   comment three lines below says `sig_msg = 0 | SPI | (ek, ect)`, i.e. the code
   contradicts its own documentation. The MAC input (`mac_compute_loc`) *does* follow the
   spec order (`role ‖ SPI ‖ ID`), so the two are inconsistent with each other as well.
   Self-consistent across signer and verifier, so the protocol runs, but a spec-faithful
   peer will fail every signature check.
2. **(a) non-canonical encoding — KDF+ swaps the chaining value and the key.**
   Algorithm 16 is `mkI = KDF(K, SS ‖ salt ‖ 0x02)`, `mkR = KDF(K, mkI ‖ salt ‖ 0x03)`.
   With KDF instantiated as `XOF(key ‖ msg)`, that means `XOF(K ‖ SS ‖ salt ‖ 0x02)`.
   `prf_mac.c` `loom_prf_derive` instead computes `XOF(SS ‖ K ‖ N_I ‖ N_R ‖ 0x02)` and
   `XOF(mkI ‖ K ‖ N_I ‖ N_R ‖ 0x03)` — K and the chaining value are transposed. (The
   first output, `SS = XOF(K ‖ salt ‖ 0x01)`, does match.) Note that **§2.3's prose
   contradicts Algorithm 16 in the first place** — it states the unchained
   `mkI = KDF(K, salt‖0x02)`, `mkR = KDF(K, salt‖0x03)` — so the spec has to be fixed
   before conformance can even be defined.
3. **(a) hard-wired constant — the identities are the literal strings "initiator" and
   "responder".** `KEX_LoomKEX-128.c:19-22` defines `ID_INITIATOR "initiator"` /
   `ID_RESPONDER "responder"` and passes them into every `crypto_loom_*` call as both the
   self-ID and the expected peer-ID. The identity binding that τ_I/τ_R is supposed to
   provide (Algorithms 4, 5, 6) therefore authenticates a compile-time constant, not a
   party. The ICCS KEX interface has no identity parameter, so some placeholder is
   forced; the defect is that `loom.c` fully supports caller-supplied IDs
   (`crypto_loom_initialize_state(..., idself, idlen)`, `LOOM_MAX_IDBYTES`) and the
   wrapper throws that away. Anyone copying the wrapper gets no identity binding.
4. **(a) wire-format deviation, self-declared.** `params.h` defines
   `LOOM_SIGLENBYTES 2` with the comment "LOOM_DISPUTE: 2-byte sig length prefix; not in
   AKE PDF wire format", and `prf_mac.c` carries "LOOM_DISPUTE: exact PRF not named in
   AKE PDF; SHAKE256 chosen". So msg3/msg4 are
   `idlen(2) ‖ ID(32, zero-padded) ‖ siglen(2) ‖ σ(|σ|_max, zero-padded) ‖ τ`, where the
   spec's msg is just `(ID, σ, τ)`. `LOOM_MAX_IDBYTES` is additionally capped at 32 (an
   8191 bound is commented out). None of this is in the specification.
5. **(a) PRF counter reuse in `indcpa_enc` — inherited from the shared Weaver core.**
   Algorithm 11 lines 3–5 spend exactly one counter value on the lift (`ctr = 0`, then
   `ctr++`) so that r[i] uses counters 1..k. `kem/indcpa.c:351` calls
   `polyvec_invq(&pkpv, coins, nonce++)`; the callee increments its *own copy* once per
   module component, consuming nonces 0..k−1 (more on rejection restarts), while the
   caller's nonce has only reached 1. The CBD loop at `indcpa.c:365` then draws
   r[0..k−1] from PRF(coins, 1..k). PRF substreams 1..k−1 are therefore used **twice** —
   once as randomized-lifting randomness for t_lift components 1..k−1 and once as the
   secret CBD noise r[0..k−2]. Both stay secret in an honest run, so this is not an
   immediate break, but it voids the independence that Algorithm 12's uniformity argument
   assumes and makes the code incompatible with any spec-faithful re-implementation.
   (This is the identical defect found in kem-39 Weaver, whose `indcpa.c` is the same
   file.)
6. **(b) spec typo — the "12" in Algorithms 7 and 10.** Both write the secret key as
   `B^{12·k·n/8}` / `Encode_12(ŝ)`, but q = 7681 at levels 3 and 5 needs 13 bits. The
   implementation correctly uses `WEAVER_QBITS` = 12/13/13, giving
   `LOOM_KEM_SKBYTES` = 960/1664/3328. The spec must say ⌈log2 q⌉.
7. **(a) minor size deviation at level 1 — the encryption coins are 32 bytes, not
   len_ss.** Algorithm 8 line 1 says `m, r ←$ {0,1}^{8·len_ss}`, i.e. 16 bytes each at
   level 1. `kemparams.h:87` sets
   `WEAVER_KEM_DERAND_COINBYTES = WEAVER_INDCPA_MSGBYTES + WEAVER_SYMBYTES` = 16 + **32**,
   so r is 32 bytes. Levels 3 and 5 coincide because there len_ss = len_seed. Not a
   weakening (more randomness), but the stated encapsulation randomness is wrong.
8. **(b) no domain separation between G and F.** Table 3 lists them as distinct
   primitives, but `kem_cpaf.c` instantiates both as the same XOF with no label:
   `K = XOF(m, len_ss)` and `t = XOF(m ‖ c, len_F)`. They are collision-free only because
   their inputs have different lengths and the underlying SM3-counter XOF length-pads.
   A one-byte domain tag would make this robust rather than incidental.
9. **(informational) latent length confusion in `loom_prf_derive`.** The third
   derivation copies `mki` with `LOOM_KEM_SSBYTES` as its length although `mki` is a
   `LOOM_MACBYTES` buffer (`prf_mac.c`). The two are equal in all three shipped
   instances (16/16, 32/32, 64/64), so nothing overflows today, but any future suite with
   |mk| < |K| would read out of bounds.
10. **(informational) debug output in the library.** The ICCS wrapper prints
    `**Error: …` to stdout on every failure path (`KEX_LoomKEX-128.c`), including
    authentication failures, which both leaks failure-reason detail and corrupts the
    KAT driver's stdout in a production build.

Not verified: the Shuttle signature core under `sig/` (rejection sampling, `irs.c`,
`rans.c`, `approx_exp.c`/`approx_log.c`) was not reviewed at all — this spec defers it to
the separate Shuttle submission; the Barnes-of-Weaver MsgEncode/MsgDecode and the
`invq` tables were only spot-checked against the shared kem-39 analysis; the DFR figures
of Table 5 were not recomputed; HMAC-over-pseudoXOF with a 64-byte block was checked for
structural correctness (the `0x36`→`0x6a` re-XOR does yield the 0x5c opad) but not for
indifferentiability. Only the LoomKEX-128 tree was read line by line.
