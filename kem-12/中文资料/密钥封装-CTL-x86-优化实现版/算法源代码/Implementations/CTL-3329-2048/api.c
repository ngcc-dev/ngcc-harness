

/*
 * This file is not meant to be compiled independently, but to be
 * included (with #include) by another C file. The caller shall
 * first define the Q, N and LOGN macros to relevant values (decimal
 * literal constants only).
 */

#if !defined Q || !defined N || !defined LOGN || !defined LVLBYTES
#error This module must not be compiled separately.
#endif

#include "ctl.h"
#include "inner.h"

/*
 * For Q = 3329, ciphertext elements require 16-bit integers.
 * For smaller Q, 8-bit integers are sufficient.
 */
#if Q == 3329
typedef int16_t ct_elem_t;
#else
typedef int8_t ct_elem_t;
#endif

/*
 * Use SM3-based hash and XOF from auxfunc.
 */
#include "auxfunc.h"

#define XCAT(x, y)    XCAT_(x, y)
#define XCAT_(x, y)   x ## y
#define XSTR(x)       XSTR_(x)
#define XSTR_(x)      #x

#define Zn(name)   XCAT(XCAT(XCAT(ctl_, Q), XCAT(_, N)), XCAT(_, name))
#define ZN(name)   XCAT(XCAT(XCAT(CTL_, Q), XCAT(_, N)), XCAT(_, name))

static inline void HASH(void *dst, size_t dst_len, const void *key, size_t key_len, const void *src, size_t src_len)
{
	(void)key;
	(void)key_len;
	if (dst_len == 32) {
		sm3hash(256, src, src_len * 8, dst);
	} else if (dst_len <= 32) {
		uint8_t tmp[8 + src_len];
		enc64le(tmp, 0);
		memcpy(tmp + 8, src, src_len);
		pseudoXOF(dst_len * 8, tmp, (8 + src_len) * 8, dst);
	} else {
		pseudoXOF(dst_len * 8, src, src_len * 8, dst);
	}
}

static inline void EXPAND(void *dst, size_t dst_len, const void *seed, size_t seed_len, uint64_t label)
{
	size_t tmp_len = 16 + seed_len;
	uint8_t *tmp = (uint8_t *)malloc(tmp_len);
	if (tmp == NULL) {
		return;
	}
	enc64le(tmp, label);
	memset(tmp + 8, 0, 8);
	memcpy(tmp + 16, seed, seed_len);
	pseudoXOF(dst_len * 8, tmp, tmp_len * 8, dst);
	free(tmp);
}

/*
 * Ensure good alignment of the provided pointer (8-byte alignment).
 * Returned value is the aligned pointer. If, after alignment, the size
 * is not at least equal to min_tmp_len, then NULL is returned.
 */
static void *
tmp_align(void *tmp, size_t tmp_len, size_t min_tmp_len)
{
	unsigned off;

	if (tmp == NULL) {
		return NULL;
	}
	off = (8u - (unsigned)(uintptr_t)tmp) & 7u;
	if (tmp_len < off || (tmp_len - off) < min_tmp_len) {
		return NULL;
	}
	return (void *)((uintptr_t)tmp + off);
}

/*
 * Recompute the additional secret seed (rr) from the private key seed.
 */
static void
make_rr(Zn(private_key) *sk)
{
	EXPAND(sk->rr, sizeof sk->rr, sk->seed, sizeof sk->seed,
		(uint32_t)Q | ((uint32_t)LOGN << 16) | 0x72000000);
}

/*
 * Compute the hash function Hash_m(), used over the plaintext polynomial
 * 's' to generate the encryption seed. Output size matches the security
 * level.
 */
static void
hash_m(void *dst, const void *sbuf, size_t sbuf_len)
{
	/*
	 * We use a raw hash here because in practice sbuf_len exactly
	 * matches the block length of the BLAKE2 function and we want
	 * to stick to a single invocation of the primitive.
	 *
	 * Note that the output size used here is at most 16 (with BLAKE2s,
	 * for degree N <= 512) or 32 (with BLAKE2b, for degree N >= 1024),
	 * i.e. strictly less than the natural hash output size. The output
	 * size is part of the personalization block of BLAKE2, so this
	 * already ensures domain separation from the BLAKE2 invocations
	 * in the expand() calls in other functions used in this file.
	 */
	HASH(dst, LVLBYTES, NULL, 0, sbuf, sbuf_len);
}

/*
 * Compute the combination of Hash_s() and Sample_s(): the provided input
 * is nominally hashed into a seed, which is extended into enough bytes
 * with a KDF. The seed is used for nothing else. Moreover, the input is
 * guaranteed to be small (at most 32 bytes), so we can just use the
 * hash expand function.
 */
static void
hash_and_sample_s(void *sbuf, size_t sbuf_len, const void *m, size_t m_len)
{
	EXPAND(sbuf, sbuf_len, m, m_len,
		(uint32_t)Q | ((uint32_t)LOGN << 16) | 0x73000000);
}

/*
 * Make an alternate seed for key derivation, to be used on decapsulation
 * failure. This function is called F() in the CTL specification.
 */
static void
make_kdf_seed_bad(void *m, size_t m_len,
	const Zn(private_key) *sk, const Zn(ciphertext) *ct)
{
	size_t tmp_len = 8 + sizeof(sk->rr) + (size_t)N * sizeof(ct_elem_t) + sizeof(ct->c2);
	uint8_t *tmp = (uint8_t *)malloc(tmp_len);
	size_t off;

	if (tmp == NULL) {
		return;
	}

	enc64le(tmp, (uint32_t)Q | ((uint32_t)LOGN << 16) | 0x62000000);
	off = 8;
	memcpy(tmp + off, sk->rr, sizeof sk->rr);
	off += sizeof sk->rr;
	memcpy(tmp + off, ct->c, N * sizeof(ct_elem_t));
	off += N * sizeof(ct_elem_t);
	memcpy(tmp + off, ct->c2, sizeof ct->c2);
	off += sizeof ct->c2;

	/* Use pseudoXOF for exact output length matching m_len */
	pseudoXOF(m_len * 8, tmp, off * 8, m);
	free(tmp);
}

/*
 * Make the secret value from the plaintext s.
 * 'good' should be 1 for normal secret derivation, or 0 when doing
 * fake derivation after decapsulation failure.
 */
static void
make_secret(void *secret, size_t secret_len,
	const void *m, size_t m_len, uint32_t good)
{
	EXPAND(secret, secret_len, m, m_len,
		(uint32_t)Q | ((uint32_t)LOGN << 16) | ((good + 0x66) << 24));
}

/* see ctl.h */
int
Zn(keygen)(Zn(private_key) *sk, void *tmp, size_t tmp_len)
{
	prng_context rng;
	uint8_t rng_seed[32];

	tmp = tmp_align(tmp, tmp_len, ZN(TMP_KEYGEN) - 31);
	if (tmp == NULL) {
		return CTL_ERR_NOSPACE;
	}
	if (!ctl_get_seed(rng_seed, sizeof rng_seed)) {
		return CTL_ERR_RANDOM;
	}
	prng_init(&rng, rng_seed, sizeof rng_seed, 0);
	for (;;) {
		prng_get_bytes(&rng, sk->seed, sizeof sk->seed);
		if (!ctl_keygen_make_fg(sk->f, sk->g,
			(uint16_t *)sk->h, Q, LOGN,
			sk->seed, sizeof sk->seed, tmp))
		{
			continue;
		}
#if Q == 3329
		if (!ctl_keygen_solve_FG_3329(sk->F, sk->G, sk->f, sk->g,
			LOGN, tmp))
#else
		if (!ctl_keygen_solve_FG(sk->F, sk->G, sk->f, sk->g,
			Q, LOGN, tmp))
#endif
		{
			continue;
		}
#if Q == 3329
		if (!ctl_keygen_compute_w_3329(sk->w,
			sk->f, sk->g, sk->F, sk->G, LOGN, tmp))
#else
		if (!ctl_keygen_compute_w(sk->w,
			sk->f, sk->g, sk->F, sk->G, Q, LOGN, tmp))
#endif
		{
			continue;
		}
		make_rr(sk);
		return 0;
	}
}

/* see ctl.h */
void
Zn(get_public_key)(Zn(public_key) *pk, const Zn(private_key) *sk)
{
	memmove(pk->h, sk->h, sizeof sk->h);
}

static size_t
get_privkey_length(const Zn(private_key) *sk, int short_format)
{
	if (short_format) {
		return 1 + sizeof(sk->seed) + ctl_trim_i8_encode(
			NULL, 0, NULL, LOGN, ctl_max_FG_bits[LOGN]);
	} else {
		return 1 + sizeof(sk->seed) + sizeof(sk->rr)
			+ ctl_trim_i8_encode(NULL, 0,
				sk->f, LOGN, ctl_max_fg_bits[LOGN])
			+ ctl_trim_i8_encode(NULL, 0,
				sk->g, LOGN, ctl_max_fg_bits[LOGN])
#if Q == 3329
			+ ctl_trim_i16_encode(NULL, 0,
				sk->F, LOGN, ctl_max_FG_bits[LOGN])
			+ ctl_trim_i16_encode(NULL, 0,
				sk->G, LOGN, ctl_max_FG_bits[LOGN])
#else
			+ ctl_trim_i8_encode(NULL, 0,
				sk->F, LOGN, ctl_max_FG_bits[LOGN])
			+ ctl_trim_i8_encode(NULL, 0,
				sk->G, LOGN, ctl_max_FG_bits[LOGN])
#endif
			+ ctl_trim_i32_encode(NULL, 0,
				sk->w, LOGN, ctl_max_w_bits[LOGN])
			+ XCAT(ctl_encode_, Q)(NULL, 0, sk->h, LOGN);
	}
}

/* see ctl.h */
size_t
Zn(encode_private_key)(void *out, size_t max_out_len,
	const Zn(private_key) *sk, int short_format)
{
	uint8_t *buf;
	size_t len, off, out_len;

	out_len = get_privkey_length(sk, short_format);
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}
	buf = out;
	if (short_format) {
		buf[0] = ZN(TAG_PRIVKEY_SHORT);
		memmove(&buf[1], sk->seed, sizeof sk->seed);
		off = 1 + sizeof sk->seed;
#if Q == 3329
		len = ctl_trim_i16_encode(buf + off, out_len - off,
			sk->F, LOGN, ctl_max_FG_bits[LOGN]);
#else
		len = ctl_trim_i8_encode(buf + off, out_len - off,
			sk->F, LOGN, ctl_max_FG_bits[LOGN]);
#endif
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		return off;
	} else {
		buf[0] = ZN(TAG_PRIVKEY_LONG);
		memmove(&buf[1], sk->seed, sizeof sk->seed);
		off = 1 + sizeof sk->seed;
		memmove(&buf[off], sk->rr, sizeof sk->rr);
		off += sizeof sk->rr;
		len = ctl_trim_i8_encode(buf + off, out_len - off,
			sk->f, LOGN, ctl_max_fg_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = ctl_trim_i8_encode(buf + off, out_len - off,
			sk->g, LOGN, ctl_max_fg_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
 #if Q == 3329
		len = ctl_trim_i16_encode(buf + off, out_len - off,
			sk->F, LOGN, ctl_max_FG_bits[LOGN]);
#else
		len = ctl_trim_i8_encode(buf + off, out_len - off,
			sk->F, LOGN, ctl_max_FG_bits[LOGN]);
#endif
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
#if Q == 3329
		len = ctl_trim_i16_encode(buf + off, out_len - off,
			sk->G, LOGN, ctl_max_FG_bits[LOGN]);
#else
		len = ctl_trim_i8_encode(buf + off, out_len - off,
			sk->G, LOGN, ctl_max_FG_bits[LOGN]);
#endif
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = ctl_trim_i32_encode(buf + off, out_len - off,
			sk->w, LOGN, ctl_max_w_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = XCAT(ctl_encode_, Q)(buf + off, out_len - off,
			sk->h, LOGN);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		return off;
	}
}

/* see ctl.h */
size_t
Zn(decode_private_key)(Zn(private_key) *sk, const void *in, size_t max_in_len,
	void *tmp, size_t tmp_len)
{
	const uint8_t *buf;
	size_t off, len;

	if (in == NULL || max_in_len == 0) {
		return 0;
	}
	buf = in;
	switch (buf[0]) {
	case ZN(TAG_PRIVKEY_SHORT):
		if (max_in_len < get_privkey_length(sk, 1)) {
			return 0;
		}
		memmove(sk->seed, buf + 1, sizeof sk->seed);
		off = 1 + sizeof sk->seed;
#if Q == 3329
		len = ctl_trim_i16_decode(sk->F, LOGN, ctl_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
#else
		len = ctl_trim_i8_decode(sk->F, LOGN, ctl_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
#endif
		if (len == 0) {
			return 0;
		}
		off += len;
		tmp = tmp_align(tmp, tmp_len, ZN(TMP_DECODE_PRIV) - 31);
		if (tmp == NULL) {
			return 0;
		}
		if (!ctl_keygen_make_fg(sk->f, sk->g,
			(uint16_t *)sk->h, Q, LOGN,
			sk->seed, sizeof sk->seed, tmp))
		{
			return 0;
		}
#if Q == 3329
		if (!ctl_keygen_rebuild_G_3329_i16(sk->G, sk->f, sk->g, sk->F,
			LOGN, tmp))
#else
		if (!ctl_keygen_rebuild_G(sk->G, sk->f, sk->g, sk->F,
			Q, LOGN, tmp))
#endif
		{
			return 0;
		}
#if Q == 3329
		if (!ctl_keygen_compute_w_3329(sk->w,
			sk->f, sk->g, sk->F, sk->G, LOGN, tmp))
#else
		if (!ctl_keygen_compute_w(sk->w,
			sk->f, sk->g, sk->F, sk->G, Q, LOGN, tmp))
#endif
		{
			return 0;
		}
		make_rr(sk);
		return off;
	case ZN(TAG_PRIVKEY_LONG):
		if (max_in_len < get_privkey_length(sk, 0)) {
			return 0;
		}
		memmove(sk->seed, buf + 1, sizeof sk->seed);
		off = 1 + sizeof sk->seed;
		memmove(sk->rr, buf + off, sizeof sk->rr);
		off += sizeof sk->rr;
		len = ctl_trim_i8_decode(sk->f, LOGN, ctl_max_fg_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		len = ctl_trim_i8_decode(sk->g, LOGN, ctl_max_fg_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
#if Q == 3329
		len = ctl_trim_i16_decode(sk->F, LOGN, ctl_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
#else
		len = ctl_trim_i8_decode(sk->F, LOGN, ctl_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
#endif
		if (len == 0) {
			return 0;
		}
		off += len;
#if Q == 3329
		len = ctl_trim_i16_decode(sk->G, LOGN, ctl_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
#else
		len = ctl_trim_i8_decode(sk->G, LOGN, ctl_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
#endif
		if (len == 0) {
			return 0;
		}
		off += len;
		len = ctl_trim_i32_decode(sk->w, LOGN, ctl_max_w_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		len = XCAT(ctl_decode_, Q)(sk->h, LOGN,
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		return off;
	default:
		return 0;
	}
}

/* see ctl.h */
size_t
Zn(encode_public_key)(void *out, size_t max_out_len, const Zn(public_key) *pk)
{
	uint8_t *buf;
	size_t out_len, len;

	out_len = 1 + XCAT(ctl_encode_, Q)(NULL, 0, pk->h, LOGN);
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}
	buf = out;
	buf[0] = ZN(TAG_PUBKEY);
	len = XCAT(ctl_encode_, Q)(buf + 1, max_out_len - 1, pk->h, LOGN);
	if (len == 0) {
		return 0;
	}
	return 1 + len;
}

/* see ctl.h */
size_t
Zn(decode_public_key)(Zn(public_key) *pk, const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	size_t len;

	if (max_in_len == 0) {
		return 0;
	}
	buf = in;
	if (buf[0] != ZN(TAG_PUBKEY)) {
		return 0;
	}
	len = XCAT(ctl_decode_, Q)(pk->h, LOGN, buf + 1, max_in_len - 1);
	if (len == 0) {
		return 0;
	}
	return 1 + len;
}

/* see ctl.h */
size_t
Zn(encode_ciphertext)(void *out, size_t max_out_len, const Zn(ciphertext) *ct)
{
	uint8_t *buf;
	size_t out_len, len, off;

	out_len = 1 + XCAT(ctl_encode_ciphertext_, Q)(NULL, 0, ct->c, LOGN)
		+ sizeof ct->c2;
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}
	buf = out;
	buf[0] = ZN(TAG_CIPHERTEXT);
	off = 1;
	len = XCAT(ctl_encode_ciphertext_, Q)(
		buf + off, max_out_len - off, ct->c, LOGN);
	if (len == 0) {
		return 0;
	}
	off += len;
	memcpy(buf + off, ct->c2, sizeof ct->c2);
	off += sizeof ct->c2;
	return off;
}

/* see ctl.h */
size_t
Zn(decode_ciphertext)(Zn(ciphertext) *ct, const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	size_t off, len;

	if (max_in_len < 1) {
		return 0;
	}
	buf = in;
	if (buf[0] != ZN(TAG_CIPHERTEXT)) {
		return 0;
	}
	off = 1;
	len = XCAT(ctl_decode_ciphertext_, Q)(
		ct->c, LOGN, buf + off, max_in_len - off);
	if (len == 0) {
		return 0;
	}
	off += len;
	if (max_in_len - off < sizeof ct->c2) {
		return 0;
	}
	memcpy(ct->c2, buf + off, sizeof ct->c2);
	off += sizeof ct->c2;
	return off;
}

/* see ctl.h */
int
Zn(encapsulate)(void *secret, size_t secret_len,
	Zn(ciphertext) *ct, const Zn(public_key) *pk,
	void *tmp, size_t tmp_len)
{
	tmp = tmp_align(tmp, tmp_len, ZN(TMP_ENCAPS) - 31);
	if (tmp == NULL) {
		return CTL_ERR_NOSPACE;
	}

	/*
	 * Encapsulation may theoretically fail if the resulting
	 * vector norm is higher than a specific bound. However, this
	 * is very rare (it cannot happen at all for q = 257). Thus,
	 * we expect not to have to loop. Correspondingly, it is more
	 * efficient to use the random seed from the OS directly.
	 */
	for (;;) {
		uint8_t m[LVLBYTES], sbuf[SBUF_LEN(LOGN)];
		size_t u;

		/*
		 * Get a random message m from the OS.
		 */
		if (!ctl_get_seed(m, sizeof m)) {
			return CTL_ERR_RANDOM;
		}

		/*
		 * Hash m to sample s.
		 */
		hash_and_sample_s(sbuf, sizeof sbuf, m, sizeof m);
#if N < 8
		/* For very reduced toy versions, we don't even have a
		   full byte, and we must clear the unused bits. */
		sbuf[0] &= (1u << N) - 1u;
#endif

		/*
		 * Compute c1. This may fail (rarely!) only for q = 769 and above.
		 */
		if (!XCAT(ctl_encrypt_, Q)(ct->c, sbuf, pk->h, LOGN, tmp)) {
			continue;
		}

		/*
		 * Make c2 = Hash_m(s) XOR m.
		 */
		hash_m(ct->c2, sbuf, sizeof sbuf);
		for (u = 0; u < sizeof m; u ++) {
			ct->c2[u] ^= m[u];
		}

		/*
		 * Produce the shared secret (output of a successful key
		 * exchange).
		 */
		make_secret(secret, secret_len, m, sizeof m, 1);

		return 0;
	}
}

/* see ctl.h */
int
Zn(encapsulate_explicit_seed)(void *secret, size_t secret_len,
	Zn(ciphertext) *ct, const Zn(public_key) *pk,
	const void *m, void *tmp, size_t tmp_len)
{
	uint8_t m2[LVLBYTES];

	tmp = tmp_align(tmp, tmp_len, ZN(TMP_ENCAPS) - 31);
	if (tmp == NULL) {
		return CTL_ERR_NOSPACE;
	}

	for (;;) {
		uint8_t sbuf[SBUF_LEN(LOGN)];
		size_t u;

		/*
		 * If no seed is provided, then generate one randomly.
		 */
		if (m == NULL) {
			if (!ctl_get_seed(m2, sizeof m2)) {
				return CTL_ERR_RANDOM;
			}
			m = m2;
		}

		/*
		 * Hash m to sample s.
		 */
		hash_and_sample_s(sbuf, sizeof sbuf, m, LVLBYTES);
#if N < 8
		/* For very reduced toy versions, we don't even have a
		   full byte, and we must clear the unused bits. */
		sbuf[0] &= (1u << N) - 1u;
#endif

		/*
		 * Compute c1. This may fail (very rarely!) only for q = 769;
		 * we just hash the current seed. Since this occurrence is
		 * very rare in practice, this process does not induce any
		 * non-negligible bias.
		 */
		if (!XCAT(ctl_encrypt_, Q)(ct->c, sbuf, pk->h, LOGN, tmp)) {
			HASH(m2, LVLBYTES, NULL, 0, m, LVLBYTES);
			m = m2;
			continue;
		}

		/*
		 * Make c2 = Hash_m(s) XOR m.
		 */
		hash_m(ct->c2, sbuf, sizeof sbuf);
		for (u = 0; u < LVLBYTES; u ++) {
			ct->c2[u] ^= ((const uint8_t *)m)[u];
		}

		/*
		 * Produce the shared secret (output of a successful key
		 * exchange).
		 */
		make_secret(secret, secret_len, m, LVLBYTES, 1);

		return 0;
	}
}

/* see ctl.h */
int
Zn(decapsulate)(void *secret, size_t secret_len,
	const Zn(ciphertext) *ct, const Zn(private_key) *sk,
	void *tmp, size_t tmp_len)
{
	uint8_t sbuf[SBUF_LEN(LOGN)], m[LVLBYTES], m_alt[LVLBYTES];
	uint8_t sbuf_alt[SBUF_LEN(LOGN)];
	ct_elem_t *c_alt;
	size_t u;
	uint32_t d;

	tmp = tmp_align(tmp, tmp_len, ZN(TMP_DECAPS) - 31);
	if (tmp == NULL) {
		return CTL_ERR_NOSPACE;
	}

	/*
	 * Inner decryption never fails (at least, it never reports
	 * a failure).
	 */
	XCAT(ctl_decrypt_, Q)(sbuf, ct->c,
		sk->f, sk->g, sk->F, sk->G, sk->w, LOGN, tmp);

	/*
	 * From sbuf, we derive the mask that allows recovery of m
	 * out of the second ciphertext half (c2).
	 */
	hash_m(m, sbuf, sizeof sbuf);
	for (u = 0; u < sizeof m; u ++) {
		m[u] ^= ct->c2[u];
	}

	/*
	 * Decryption is valid if and only if we can reencrypt the
	 * obtained message m and get the exact same polynomial s
	 * and ciphertext c1.
	 */
	hash_and_sample_s(sbuf_alt, sizeof sbuf_alt, m, sizeof m);
#if N < 8
	sbuf_alt[0] &= (1u << N) - 1u;
#endif
	c_alt = tmp;
	
	tmp = tmp_align((void *)(c_alt + N), ZN(TMP_DECAPS) - N * sizeof(ct_elem_t),
		ZN(TMP_ENCAPS) - 31);
	if (tmp == NULL) {
		/* This should never happen in practice. */
		return CTL_ERR_NOSPACE;
	}
	d = XCAT(ctl_encrypt_, Q)(c_alt, sbuf_alt, sk->h, LOGN, tmp);
	d --;
	for (u = 0; u < sizeof sbuf; u ++) {
		d |= sbuf[u] ^ sbuf_alt[u];
	}
	/* Fix: Use N for the loop bounds to prevent out-of-bounds access when ct_elem_t is int16_t */
	for (u = 0; u < N; u ++) {
		d |= (uint32_t)(ct->c[u] - c_alt[u]);
	}

	/*
	 * If encapsulation worked AND yielded the same ciphertext as
	 * received, then d == 0 at this point, and we want to produce
	 * the secret key as a hash of m. Otherwise, d != 0, and we
	 * must produce the secret as a hash of the received ciphertext
	 * and the secret value r (stored in sk->rr). We MUST NOT leak
	 * which was the case, and therefore we must always compute
	 * both hashes and perform constant-time conditional replacement.
	 */

	make_kdf_seed_bad(m_alt, sizeof m, sk, ct);
	d = -((uint32_t)(d | -d) >> 31);
	for (u = 0; u < sizeof m; u ++) {
		m[u] ^= d & (m[u] ^ m_alt[u]);
	}
	make_secret(secret, secret_len, m, sizeof m, d + 1);
	return 0;
}
