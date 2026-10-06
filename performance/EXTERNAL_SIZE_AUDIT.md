# External-size accounting for the published performance tables

The timing records measure computation, not a network serialization format.
The tables therefore take KEM public-key/ciphertext and signature public-key/
signature sizes from the separate [`external_sizes.csv`](external_sizes.csv)
catalog (394 measured KEM/signature instances), and KEX message/public-key
sizes from [`kex_bandwidth.csv`](kex_bandwidth.csv) (57 measured instances).
These catalogs were reconciled with the frozen submissions' size tables,
serialization descriptions and the per-candidate `pseudocode.md` comparisons;
the benchmark API size fields are comparison checks, not the source of the
published external-size values.

`encoded` means the size of the submitted external encoding. For signatures
whose size varies, `maximum-variable` means the submitted encoding's maximum, while
`nominal-variable` means a representative size stated in the frozen
specification, not a guarantee for each signature. We use that basis only
where the specification supplies a concrete nominal length and the submitted
serializer returns variable lengths (Rhyme and Sigurd). Otherwise the
submitted encoding's maximum is the conservative available figure (Aigis,
Phoenix and Tins). These different bases are visible on the candidate pages;
the ordered pages mark nominal values with `≈`. Secret-key and shared-secret
sizes are not ordering metrics.

## Specification versus submitted external encoding

The catalog uses the submitted encoding when a size table describes a
different format. The per-candidate analyses provide the source details:

| candidate | externally relevant difference |
|---|---|
| [BAG-Loong](../kem-03/pseudocode.md) | The four ciphertext encodings append 16 bytes beyond the specification's listed ciphertext sizes. |
| [BAG-Piglet](../kem-04/pseudocode.md) | Each submitted ciphertext is 16 bytes longer than the specification's table. |
| [CTL](../kem-12/pseudocode.md) | The CTL-3329-2048 ciphertext is 2,353 rather than the table's 2,305 bytes. |
| [Lore](../kem-19/pseudocode.md) | Submitted public keys and ciphertexts are larger than the four listed specification sizes. The catalog uses the submitted sizes: these are zero-padded worst-case buffers, but decapsulation requires the full fixed-length ciphertext on the wire. |
| [QIMEN-PIKE](../kem-31/pseudocode.md) | The three submitted public keys are 16/3/3 bytes shorter than the specification's table. |
| [QCTM](../kem-32/pseudocode.md) | §3.2's stated public-key sizes disagree with the §3.3 formula and §8.2 serialization; the formula itself is consistent with the submitted encoding, which the catalog uses. |
| [Phoenix](../sign-19/pseudocode.md) | The 192s submitted signature is 13,332 bytes versus 13,356 in the specification's table. |
| [Rhyme](../sign-22/pseudocode.md) | The signature API advertises padded caps of 5,156/10,308/14,436/20,612 bytes. Actual signatures are variable and near the specification's nominal 1,483/3,258/4,743/7,002 bytes; those nominal values are used and marked approximate. |
| [Sigurd](../sign-24/pseudocode.md) | The API reports worst-case signature buffers of 62,868/137,412/494,532 bytes, but signatures are variable-length. The catalog uses the specification's ≈25,108/74,756/282,692-byte representative sizes, marked approximate. |
| [TRINE](../sign-30/pseudocode.md) | Submitted signatures add a salt of 32/64/128 bytes relative to the listed values. The 512-ShortSig public key is also two bytes larger. |
| [TSUOV](../sign-31/pseudocode.md) | The submitted 128-bit public key is one byte larger than the specification's table. |
| [UVW-Sign](../sign-32/pseudocode.md) | The public-key and signature figures in Table 1 are information-entropy lower bounds, not the submitted encoding. The catalog uses the actual encoded sizes. |

Three other cases matter when reading the table, without changing its external
size: [Aigis-Sig+](../sign-01/pseudocode.md),
[Phoenix](../sign-19/pseudocode.md) and [Tins](../sign-29/pseudocode.md) have
variable-length signatures represented by their submitted encoding's maxima. The
[ATLAS](../sign-15/pseudocode.md) signer reports an incorrect output length
after writing the specified signature bytes; the catalog counts the encoded
signature, not that erroneous returned length. [VDOO](../sign-33/pseudocode.md)
prints rounded public-key sizes in its specification; the catalog uses exact
encoded byte counts.

## Key exchange

Protocol-message bytes omit separately delivered public keys. Bandwidth counts
both the messages and the required public keys once each. Public keys are
transmitted bytes in this KEX model. KEM encapsulation instead assumes the
recipient's public key is available and lists its size separately.
Certificates, identities supplied out of band and transport framing are
excluded. ADKEX and MAMBA-NIKE use only the responder's long-term
public key; CreTAKE's KEM/signature combinations use different key lengths on
the two sides. These responder-only cases do not authenticate the initiator
and are not directly comparable to mutually authenticated exchanges. NIIKE
has no separate protocol message, but requires both public keys. Its
specification lists 4,030/8,943-byte public keys, whereas the submitted encoding uses
4,160/8,960 bytes; the catalog counts the latter.

[AFS-KEX](../kex-02/pseudocode.md) has two accounting differences. Figure 3
sends fresh composite public keys in the first two passes; its protocol-message
totals are 3,136/6,080/12,288 bytes. The submitted API instead pre-distributes
each composite key alongside its long-term base key and sends only
1,568/2,944/6,016 bytes as protocol messages. Counting the base public keys
once, bandwidth is identical under either arrangement: 4,704/9,216/18,560 bytes.
The catalog shows the specification's protocol-message totals. Separately, the
submitted pass-4 function emits no message but leaves its output-length
parameter unchanged. The benchmark initialized that parameter to its buffer
capacity, producing phantom traffic; the submitted KAT initializes it to
zero. The original raw timing and metadata files remain unchanged.
