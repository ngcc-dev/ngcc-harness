Candidate: Origami
Family: Multivariate (layered MQ)
Archive: [Origami.zip](https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/Origami.zip) (SHA-256: `e34f18832e968681dd0c51ce0d4b29d80e01ad29daa83dfb805718b76fdffa80`)

## sign-18-1: A fixed 512-bit message prehash caps forgery security at 256 bits

Severity: Critical
Status: Confirmed
Layer: Design
Affected: Origami-384 and Origami-512 specifications and reference implementations
Discovery: Trivial
Exploitation: Approximately 2^256 hash evaluations
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-09-21

Origami-384 and Origami-512 claim 384- and 512-bit classical security but first compress every message with the same fixed 64-byte `H_msg`. They only then derive the randomized target from `target || H_msg(message) || salt`.

A generic collision in `H_msg` costs about `2^256` evaluations. Once two messages share that prehash, every later salt produces the same signing target for both, so a signature requested on one message transfers to the other. Adding the salt after the short prehash does not repair the collision.

The construction and length are explicit in both the PDF and source, making this a specification-level design break of both advertised classical EUF-CMA levels.

### Reproducing

```sh
python3 security/design_parameter_audit.py
```

The check verifies physical PDF pages 14–16 and 50–52 and the submitted 64-byte digest constant.

## sign-18-2: Signatures expose the hidden-algebra constraint subspace

Severity: High
Status: Confirmed
Layer: Design
Affected: All four parameter sets
Discovery: Moderate
Exploitation: Polynomial-time structure recovery; no complete forgery demonstrated
Credit: Peigen Li (archive sender `PeigenLi`)
Date: 2026-09-22
Original source: [NGCC PKC Forum report](https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/7LD2222NOYVSL3KXW4UQCLPHMU7JFKQW/)

The specification places every vinegar and oil element in a degree-`l_j` subalgebra of `l_j`-by-`l_j` matrices, but counts and solves all `l_j^2` matrix coordinates independently. These requirements are incompatible: a subalgebra element has only `l_j` degrees of freedom. The implementation makes the signer work by sampling only vinegar elements in the hidden subalgebra while treating oil coordinates as unrestricted field elements, contradicting the specified variable space.

Peigen Li's [mailing-list analysis](https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/7LD2222NOYVSL3KXW4UQCLPHMU7JFKQW/) identifies this mismatch and observes that repeated signatures reveal the small hidden subalgebras. Further extension to Peigen Li's analysis: the source gives Origami-128 exactly 18 sampled degree-2 vinegar elements; each occupies four matrix coordinates but has dimension two. Every signature therefore lies in a subspace of dimension at most `200 - 18*(4-2) = 164`. A run of 180 accepted signatures reaches rank 164, saturating that bound, while 180 uniform 200-coordinate vectors reach the sample-count maximum rank 180. The analogous source count predicts 180, 336, and 432 exposed constraints for Origami-256, -384, and -512.

This is direct, reproducible leakage of the purportedly hidden structure, but no end-to-end key recovery or forgery is yet established. The post's separate estimate of at most 93 bits for Origami-128 has no published derivation and is therefore not adopted here.

### Reproducing

```sh
make -C sign-18 lib/libOrigami-128.so
python3 sign-18/reproduce_signature_subspace.py
```

The witness verifies every collected signature with the official verifier before measuring its rank over `GF(16)`.

## sign-18-3: Variable-time hidden-zone linear solving

Severity: Medium
Status: Probable
Layer: Side-channel
Affected: Origami reference signer, all four parameter sets
Discovery: Trivial
Exploitation: Local timing/cache side channel; no key recovery demonstrated
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-09-23

Signing builds a linear system from the expanded secret zone structure and sampled hidden variables, then solves it with first-nonzero pivot search and data-dependent row skips. The reference `origami_ref.c` branches on matrix entries at lines 636 and 659, and on the solve result at line 798. Thus a local trace can distinguish properties of intermediate private systems. This does not by itself establish recovery of the master seed or a forgery; see `constant_time.md` for the secret/public classification.

### Reproducing

In each reference instance, follow `sign` → `build_zone_system` → `solve_rect_random` in `origami_ref.c`; inspect the pivot test and early break at lines 633–640, row skip at 659, and attempt retry at 794–811. This is a source/dataflow witness, not a measured remote timing exploit.

## sign-18-4: Signing indexes field tables and signature state with private values

Severity: Medium
Status: Probable
Layer: Side-channel
Affected: Origami reference signer, all four parameter sets
Discovery: Trivial
Exploitation: Local cache/address trace of secret intermediates; forgery not demonstrated
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-09-23

Origami's `gf_mult` and `gf_inv` index 256-byte and 16-byte tables by field operands (`origami_gf.h:25,32-37`), including private central-map and solver values during signing (`origami_ref.c:400,599-601,653-661`). The signer also derives `w_vars` from private permutation `rho` and writes `secret_y[w_vars[i]]` (`origami_ref.c:421-426,798-800`). These addresses can depend on private values even for the same public message. No measured cache channel or EUF-CMA forgery is established; see `constant_time.md`.

### Reproducing

Inspect the cited lookup definitions and signer call sites in `Implementations and Test_Vectors/Implementations/Reference_Implementation/Origami-256/`; the same pattern is present in the other reference parameter sets. This is a source/dataflow witness, not a measured extraction.

## sign-18-5: Public variable permutation permits signature forgery

Severity: Critical
Status: Confirmed
Layer: Design
Affected: Origami-128 (full forgery reproduced); the same public-permutation and triangular-zone construction occurs in Origami-256/-384/-512, not runtime-tested here
Discovery: Moderate
Exploitation: Public-key-only forgery for a chosen message, with no signing queries
Credit: Pierre Pébereau
Date: 2026-09-24
Original source: [NGCC PKC Forum post](https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/HTZTMJ42AUXCWEDA3AURP4ZOEGSGSV4Z/), [UnfoldOrigami code](https://github.com/pi-r2/UnfoldOrigami/tree/defa3405d66e763580729b14d6f81c1300fbb219)

Origami §2.5.6 explicitly derives the change of variables `Π_pub` from the *public* expansion seed and defines `P_pub = G ∘ Π_pub^-1`. Undoing that permutation exposes the zone order. The public-key expansion supplies each zone's affine coefficients, so an attacker can choose its vinegar coordinates and solve the resulting linear system for oil coordinates, proceeding zone by zone. This reproduces the signer's easy inversion without its secret seed or a signing oracle. It is distinct from `sign-18-2`'s signature-derived subspace observation.

Pébereau's Origami-128 implementation forges a signature that the archived `sig_verify` accepts; a changed-message control rejects. The four submitted sets have byte-identical `origami_ref.c` evaluators and the same public-permutation construction, but full-size forging runtimes for the higher sets have not been independently measured here. This attack survives ideal replacement of the contest hash/XOF placeholders: those determine the public target, not the invertibility of the public map.

### Reproducing

```sh
make -C sign-18 lib/libOrigami-128.so
python3 sign-18/reproduce_public_forgery.py
```

The checker clones and checks out Pébereau's pinned attack source into a temporary directory; an existing checkout at that commit can instead be supplied as its positional argument. It loads only the library built from the archived Origami source, not the attack repository's bundled binary. It seeds key generation afresh, erases the resulting secret before calling `forge`, verifies the new signature, and rejects the same signature on a changed message. Three local runs passed with roughly 0.6-second inversion each; this is not an all-level benchmark. Python must provide `hashlib` SM3 support for this submitted evaluation variant.
