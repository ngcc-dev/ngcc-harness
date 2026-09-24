

#ifndef CTL_H__
#define CTL_H__

#include <stddef.h>
#include <stdint.h>

/*
 * For modulus qqq and degree nnn, the following types and functions are
 * defined:
 *
 *   ctl_qqq_nnn_private_key
 *
 *      Private key structure; contains all private key elements, including
 *      a copy of the public key.
 *
 *   ctl_qqq_nnn_public_key
 *
 *      Public key structure. Contains only the public key.
 *
 *   ctl_qqq_nnn_ciphertext
 *
 *      Ciphertext structure. Contains the ciphertext polynomial and
 *      the FO tag.
 *
 *   int ctl_qqq_nnn_keygen(
 *           ctl_qqq_nnn_private_key *sk, void *tmp, size_t tmp_len);
 *
 *      Generate a new key pair. Returned value is 0 on success, a negative
 *      value on error. Buffer tmp (tmp_len bytes) should be large enough
 *      (see the CTL_qqq_nnn_TMP_KEYGEN macro).
 *
 *   void ctl_qqq_nnn_get_public_key(
 *           ctl_qqq_nnn_public_key *pk, const ctl_qqq_nnn_private_key *sk);
 *
 *      Get a copy of the public key from the private key.
 *
 *   size_t ctl_qqq_nnn_encode_private_key(
 *           void *out, size_t max_out_len,
 *           const ctl_qqq_nnn_private_key *sk, int short_format);
 *
 *      Encode the private key into bytes. If short_format is zero, then
 *      the "long format" is used (encoding contains f, g, F, G, w, and the
 *      generation seed). If short_format is non-zero, then the "short
 *      format" is used (encoding contains only F and the seed). The short
 *      format is much smaller, but requires more CPU and temporary RAM
 *      when decoding.
 *
 *      If out is NULL, then max_out_len is ignored, and the function
 *      returns the size (in bytes) that the encoded private key would have.
 *      Otherwise, if the encoded private key would be longer than
 *      max_out_len, then the function returns 0 and encodes nothing.
 *      Otherwise, the encoded private key is written into out, and its
 *      size (in bytes) is returned.
 *
 *   size_t ctl_qqq_nnn_decode_private_key(
 *           ctl_qqq_nnn_private_key *sk,
 *           const void *in, size_t max_in_len,
 *           void *tmp, size_t tmp_len);
 *
 *      Decode the private key from bytes. If the incoming bytes are
 *      invalid, or relate to different set of parameters, or max_in_len
 *      is shorter than the private key size (i.e. it was truncated),
 *      then this function returns 0. Otherwise, it returns the actual
 *      size (in bytes) of the encoded private key (which is not greater
 *      than max_in_len, but may be lower than max_in_len).
 *
 *      If the encoded key uses the long format, then tmp and tmp_len
 *      are ignored. If the encoded key uses the short format, then
 *      tmp (of size tmp_len bytes) is used for temporary storage; in
 *      that case, if the buffer is too short, then the function fails
 *      and returns 0. The CTL_qqq_nnn_TMP_DECODE_PRIV macro evaluates to
 *      the required minimal size.
 *
 *   size_t ctl_qqq_nnn_encode_public_key(
 *           void *out, size_t max_out_len,
 *           const ctl_qqq_nnn_public_key *pk);
 *
 *      Encode the public key into bytes.
 *
 *      If out is NULL, then max_out_len is ignored, and the function
 *      returns the size (in bytes) that the encoded public key would have.
 *      Otherwise, if the encoded public key would be longer than
 *      max_out_len, then the function returns 0 and encodes nothing.
 *      Otherwise, the encoded public key is written into out, and its
 *      size (in bytes) is returned.
 *
 *   size_t ctl_qqq_nnn_decode_public_key(
 *           ctl_qqq_nnn_public_key *pk,
 *           const void *in, size_t max_in_len);
 *
 *      Decode the public key from bytes. If the incoming bytes are
 *      invalid, or relate to different set of parameters, or max_in_len
 *      is shorter than the public key size (i.e. it was truncated),
 *      then this function returns 0. Otherwise, it returns the actual
 *      size (in bytes) of the encoded public key (which is not greater
 *      than max_in_len, but may be lower than max_in_len).
 *
 *   size_t ctl_qqq_nnn_encode_ciphertext(
 *           void *out, size_t max_out_len,
 *           const ctl_qqq_nnn_ciphertext *ct);
 *
 *      Encode the ciphertext into bytes.
 *
 *      If out is NULL, then max_out_len is ignored, and the function
 *      returns the size (in bytes) that the encoded ciphertext would have.
 *      Otherwise, if the encoded ciphertext would be longer than
 *      max_out_len, then the function returns 0 and encodes nothing.
 *      Otherwise, the encoded ciphertext is written into out, and its
 *      size (in bytes) is returned.
 *
 *   size_t ctl_qqq_nnn_decode_ciphertext(
 *           ctl_qqq_nnn_ciphertext *ct,
 *           const void *in, size_t max_in_len);
 *
 *      Decode the ciphertext from bytes. If the incoming bytes are
 *      invalid, or relate to different set of parameters, or max_in_len
 *      is shorter than the ciphertext size (i.e. it was truncated),
 *      then this function returns 0. Otherwise, it returns the actual
 *      size (in bytes) of the encoded ciphertext (which is not greater
 *      than max_in_len, but may be lower than max_in_len).
 *
 *   int ctl_qqq_nnn_encapsulate(
 *           void *secret, size_t secret_len,
 *           ctl_qqq_nnn_ciphertext *ct,
 *           const ctl_qqq_nnn_public_key *pk,
 *           void *tmp, size_t tmp_len);
 *
 *      Perform a key encpasulation with the provided public key. The
 *      resulting shared secret is written into secret[], while the
 *      ciphertext is written into *ct. The shared secret length is
 *      arbitrary (it internally comes from a BLAKE2-based KDF) but
 *      of course the sender and receiver should agree on the length to
 *      use, depending on what the secret is for.
 *
 *      On success, 0 is returned; on error, a negative error code is
 *      returned and the secret value is not produced. If provided
 *      temporary buffer (tmp, of size tmp_len bytes) is too small, then
 *      CTL_ERR_NOSPACE is returned (see CTL_qqq_nnn_TMP_ENCAPS).
 *
 *   int ctl_qqq_nnn_encapsulate_explicit_seed(
 *           void *secret, size_t secret_len,
 *           ctl_qqq_nnn_ciphertext *ct,
 *           const ctl_qqq_nnn_public_key *pk,
 *           const uint8_t *m, void *tmp, size_t tmp_len);
 *
 *      This is a variant of ctl_qqq_nnn_encapsulate(), in which the
 *      random seed (m[] value) is provided explicitly. This function
 *      is meant mostly for benchmarks and reproducible test vectors,
 *      to avoid the overhead and unpredictability of the OS-provided
 *      random generator; in general, ctl_qqq_nnn_encapsulate() SHOULD
 *      be used instead. If m is NULL, then the OS RNG is used to create
 *      the seed. When m is not NULL, then it MUST be generated as a
 *      uniform unpredictable sequence of bytes of the right length for
 *      the target CTL version (16, 32 or 48 bytes, for CTL-257-512,
 *      CTL-769-1024, and CTL-3329-2048 respectively).
 *
 *   int ctl_qqq_nnn_decapsulate(
 *           void *secret, size_t secret_len,
 *           const ctl_qqq_nnn_ciphertext *ct,
 *           const ctl_qqq_nnn_private_key *sk,
 *           void *tmp, size_t tmp_len);
 *
 *      Perform a key decpasulation with the provided ciphertext and
 *      private key. The resulting shared secret is written into
 *      secret[]. The shared secret length is arbitrary (it internally
 *      comes from a BLAKE2-based KDF) but of course the sender and
 *      receiver should agree on the length to use, depending on what
 *      the secret is for.
 *
 *      On success, 0 is returned; on error, a negative error code is
 *      returned and the secret value is not produced. Such errors are
 *      reported only for local technical reasons unrelated to the
 *      received ciphertext; e.g. CTL_ERR_NOSPACE is returned if the
 *      tmp[] buffer (of size tmp_len bytes) is returned. By
 *      construction of the algorithm, invalid ciphertext values lead to
 *      a recovered shared secret which is deterministic from the
 *      ciphertext and private key, but unpredictable by third parties;
 *      in such cases, this function reports a success (0).
 */

#define CTL_MK(q, n, lvl_bytes, htype, ctype, fgtype) \
typedef struct { \
	int8_t f[n]; \
	int8_t g[n]; \
	fgtype F[n]; \
	fgtype G[n]; \
	int32_t w[n]; \
	htype h[n]; \
	uint8_t rr[32]; \
	uint8_t seed[32]; \
} ctl_ ## q ## _ ## n ## _private_key; \
typedef struct { \
	htype h[n]; \
} ctl_ ## q ## _ ## n ## _public_key; \
typedef struct { \
	ctype c[n]; \
	uint8_t c2[lvl_bytes]; \
} ctl_ ## q ## _ ## n ## _ciphertext; \
int ctl_ ## q ## _ ## n ## _keygen(ctl_ ## q ## _ ## n ## _private_key *sk, \
	void *tmp, size_t tmp_len); \
void ctl_ ## q ## _ ## n ## _get_public_key( \
	ctl_ ## q ## _ ## n ## _public_key *pk, \
	const ctl_ ## q ## _ ## n ## _private_key *sk); \
size_t ctl_ ## q ## _ ## n ## _encode_private_key( \
	void *out, size_t max_out_len, \
	const ctl_ ## q ## _ ## n ## _private_key *sk, int short_format); \
size_t ctl_ ## q ## _ ## n ## _decode_private_key( \
	ctl_ ## q ## _ ## n ## _private_key *sk, \
	const void *in, size_t max_in_len, \
	void *tmp, size_t tmp_len); \
size_t ctl_ ## q ## _ ## n ## _encode_public_key( \
	void *out, size_t max_out_len, \
	const ctl_ ## q ## _ ## n ## _public_key *pk); \
size_t ctl_ ## q ## _ ## n ## _decode_public_key( \
	ctl_ ## q ## _ ## n ## _public_key *pk, \
	const void *in, size_t max_in_len); \
size_t ctl_ ## q ## _ ## n ## _encode_ciphertext( \
	void *out, size_t max_out_len, \
	const ctl_ ## q ## _ ## n ## _ciphertext *ct); \
size_t ctl_ ## q ## _ ## n ## _decode_ciphertext( \
	ctl_ ## q ## _ ## n ## _ciphertext *ct, \
	const void *in, size_t max_in_len); \
int ctl_ ## q ## _ ## n ## _encapsulate( \
	void *secret, size_t secret_len, \
	ctl_ ## q ## _ ## n ## _ciphertext *ct, \
	const ctl_ ## q ## _ ## n ## _public_key *pk, \
	void *tmp, size_t tmp_len); \
int ctl_ ## q ## _ ## n ## _encapsulate_explicit_seed( \
	void *secret, size_t secret_len, \
	ctl_ ## q ## _ ## n ## _ciphertext *ct, \
	const ctl_ ## q ## _ ## n ## _public_key *pk, \
	const void *m, void *tmp, size_t tmp_len); \
int ctl_ ## q ## _ ## n ## _decapsulate( \
	void *secret, size_t secret_len, \
	const ctl_ ## q ## _ ## n ## _ciphertext *ct, \
	const ctl_ ## q ## _ ## n ## _private_key *sk, \
	void *tmp, size_t tmp_len);

/*
 * NIST security levels: the library implements three parameter sets
 * corresponding roughly to the three categories required by the NIST
 * post-quantum competition, plus an additional high security set.
 */

CTL_MK(257, 512, 16, uint16_t, int8_t, int8_t)
CTL_MK(769, 1024, 32, uint16_t, int8_t, int8_t)  /* third security level (NIST category 3) */
CTL_MK(3329, 2048, 48, uint16_t, int16_t, int16_t) /* high security level */

#undef CTL_MK

/*
 * Macros for temporary buffer sizes.
 *
 * Each length is in bytes and accounts for an extra 31 bytes for internal
 * alignment adjustment.
 */
#define CTL_257_512_TMP_KEYGEN         12319
#define CTL_257_512_TMP_DECODE_PRIV    12319
#define CTL_257_512_TMP_ENCAPS          2079
#define CTL_257_512_TMP_DECAPS          4127

#define CTL_769_1024_TMP_KEYGEN        24607
#define CTL_769_1024_TMP_DECODE_PRIV   24607
#define CTL_769_1024_TMP_ENCAPS         4127
#define CTL_769_1024_TMP_DECAPS         8223

#define CTL_3329_2048_TMP_KEYGEN     1572863
#define CTL_3329_2048_TMP_DECODE_PRIV 262143
#define CTL_3329_2048_TMP_ENCAPS        8223
#define CTL_3329_2048_TMP_DECAPS       16415

/*
 * Error codes.
 */

/* Decapsulation failed. */
#define CTL_ERR_DECAPS_FAILED   -1

/* Provided object (key or ciphertext) uses a different set of parameters
   (modulus and/or degree) than expected by the called function. */
#define CTL_ERR_WRONG_PARAMS    -2

/* Provided object (key or ciphertext) is invalidly encoded. */
#define CTL_ERR_BAD_ENCODING    -3

/* Provided temporary space has insufficient length for the requested
   operation. */
#define CTL_ERR_NOSPACE         -4

/* Random seeding from operating system failed. */
#define CTL_ERR_RANDOM          -5

/*
 * Tag bytes. Each encoded public key, private key or ciphertext starts
 * with a tag byte that identifies the object type and parameters.
 * General format is (most-to-least significant order):
 *
 *    t t q q n n n n
 *
 * with:
 *
 *  - tt = 00 for a private key (long format), 01 for a private key (short
 *    format), 10 for a public key, 11 for a ciphertext.
 *  - qq = 01 for q = 257, 10 for q = 769, 11 for q = 3329.
 *  - nnnn = log2(n) where n is the degree (power of 2, up to 2048).
 */
#define CTL_257_512_TAG_PRIVKEY_LONG     0x19
#define CTL_257_512_TAG_PRIVKEY_SHORT    0x59
#define CTL_257_512_TAG_PUBKEY           0x99
#define CTL_257_512_TAG_CIPHERTEXT       0xD9

#define CTL_769_1024_TAG_PRIVKEY_LONG    0x2A
#define CTL_769_1024_TAG_PRIVKEY_SHORT   0x6A
#define CTL_769_1024_TAG_PUBKEY          0xAA
#define CTL_769_1024_TAG_CIPHERTEXT      0xEA

/*
 * For q = 3329 (qq = 11), n = 2048 (nnnn = 1011)
 */
#define CTL_3329_2048_TAG_PRIVKEY_LONG   0x3B
#define CTL_3329_2048_TAG_PRIVKEY_SHORT  0x7B
#define CTL_3329_2048_TAG_PUBKEY         0xBB
#define CTL_3329_2048_TAG_CIPHERTEXT     0xFB

#endif
