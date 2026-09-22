/*
 * Falcon-style negacyclic NTT for multiplication in Z_q[X]/(X^d + 1).
 *
 * Yuanyang's native q does not have enough 2-adicity for a full NTT at the
 * largest target degree.  We therefore lift coefficients to the auxiliary NTT
 * prime:
 *
 *   QNTT = 2147352577 = 1 mod 2^17
 *
 * This single auxiliary prime supports yuanyang-512, yuanyang-1024 and
 * yuanyang-2048.  The table values below are parameter-set specific, because
 * the required negacyclic root has order 2*d.
 *
 * This file was generated for yuanyang-512:
 *
 *   d       = 512
 *   logd    = 9
 *   zeta    = 1543607367  (primitive 4096-th root modulo QNTT)
 *   psi     = 898500465  (primitive 2*d-th root, psi^d = -1)
 *   psi^-1  = 487056385
 *
 * The transform follows Falcon's direct layout, instead of a cyclic
 * NTT surrounded by explicit psi^i / psi^-i twists.  Thus only two root tables
 * are needed:
 *
 *   qntt_GMb[x]  = R * psi^bitrev_logd(x) mod QNTT
 *   qntt_iGMb[x] = R * psi^-bitrev_logd(x) mod QNTT
 *
 * Index 0 is unused, as in Falcon's GMb/iGMb tables.  The forward and inverse
 * butterflies can therefore use the same m+i / hm+i indexing pattern as
 * Falcon's verifier.  Coefficients are reduced back to Yuanyang's q after the
 * inverse transform.
 */

#include <stdint.h>
#include <stdalign.h>
#include <assert.h>

#include "yuanyang_inner.h"
#include "ntttable.h"

/*
 * Addition modulo q. Operands must be in the 0..q-1 range.
 */
static inline uint32_t
qntt_add(uint32_t a, uint32_t b)
{
	/*
	 * We compute x + y - q. If the result is negative, then the
	 * high bit will be set, and 'd >> 31' will be equal to 1;
	 * thus '-(d >> 31)' will be an all-one pattern. Otherwise,
	 * it will be an all-zero pattern. In other words, this
	 * implements a conditional addition of q.
	 */
	uint32_t d;

	d = a + b - QNTT;
	d += QNTT & -(d >> 31);
	return d;
}

/*
 * Subtraction modulo q. Operands must be in the 0..q-1 range.
 */
static inline uint32_t
qntt_sub(uint32_t a, uint32_t b)
{
	/*
	 * As in qntt_add(), we use a conditional addition to ensure the
	 * result is in the 0..QNTT-1 range.
	 */
	uint32_t d;

	d = a - b;
	d += QNTT & -(d >> 31);
	return d;
}

/*
 * Montgomery multiplication modulo QNTT. With R = 2^32 mod QNTT, this function
 * computes a*b/R mod QNTT. Operands must be in the 0..QNTT-1 range.
 */
static inline uint32_t
qntt_montymul(uint32_t a, uint32_t b)
{
	/*
	 * This is the same reduction pattern as Falcon's mq_montymul(), but
	 * with a 32-bit Montgomery radix instead of Falcon's 16-bit radix. We
	 * compute a*b + m*QNTT with m chosen so that the low 32 bits are zero;
	 * after the shift, one conditional subtraction puts the result back in
	 * the expected range.
	 */
	uint64_t z;
	uint32_t m;
	uint32_t d;

	z = (uint64_t)a * (uint64_t)b;
	m = (uint32_t)z * QNTT_Q0I;
	z = (z + (uint64_t)m * QNTT) >> 32;
	d = (uint32_t)z - QNTT;
	d += QNTT & -(d >> 31);
	return d;
}

static inline uint32_t
qntt_norm_i32(int32_t x)
{
	/*
	 * Same idea as Falcon's mq_conv_small(): if x < 0, add the modulus to
	 * obtain its representative in 0..QNTT-1.
	 */
	if (x >= 0) {
		uint32_t ux;

		ux = (uint32_t)x;
		return ux < QNTT ? ux : ux - QNTT;
	}
	if (x > -(int32_t)QNTT) {
		return QNTT + (uint32_t)x;
	}
	{
		int64_t r;

		r = (int64_t)x % (int64_t)QNTT;
		if (r < 0) {
			r += QNTT;
		}
		return (uint32_t)r;
	}
}

static inline uint32_t
qntt_norm_i64(int64_t x)
{
	uint64_t ux, sign, mag, hi;
	uint32_t r, d, neg_r;

	/*
	 * Reduce the magnitude with 2^31 = 2^17 - 1 (mod QNTT). Three
	 * fixed folds cover the full signed 64-bit input range and leave a value
	 * below 2*QNTT. This avoids integer division without narrowing callers.
	 * Plus, it's constant time.
	 */
	ux = (uint64_t)x;
	sign = (uint64_t)0 - (uint64_t)(x < 0);
	mag = (ux ^ sign) - sign;
	hi = mag >> 31;
	mag = (mag & UINT64_C(0x7FFFFFFF)) + (hi << 17) - hi;
	hi = mag >> 31;
	mag = (mag & UINT64_C(0x7FFFFFFF)) + (hi << 17) - hi;
	hi = mag >> 31;
	mag = (mag & UINT64_C(0x7FFFFFFF)) + (hi << 17) - hi;
	r = (uint32_t)mag;
	d = r - QNTT;
	r = d + (QNTT & (uint32_t)-(d >> 31));
	neg_r = (QNTT - r) & (uint32_t)-(r != 0u);
	return (r & (uint32_t)~sign) | (neg_r & (uint32_t)sign);
}

static inline int64_t
qntt_center_u32(uint32_t x)
{
	/* Interpret a QNTT residue as the centered representative. */
	if (x > QNTT_HALF) {
		return (int64_t)x - (int64_t)QNTT;
	}
	return (int64_t)x;
}

static inline uint16_t
qntt_to_q(uint32_t x)
{
	/*
	 * The product was computed modulo QNTT, but the target ring is modulo
	 * YUANYANG_Q. First interpret x as a balanced integer modulo QNTT, then
	 * reduce that integer modulo YUANYANG_Q.
	 */
	int32_t sx;

	sx = x > QNTT_HALF ? -(int32_t)(QNTT - x) : (int32_t)x;
	sx %= (int32_t)YUANYANG_Q;
	if (sx < 0) {
		sx += YUANYANG_Q;
	}
	return (uint16_t)sx;
}

/*
 * Compute the direct negacyclic NTT on a ring element.
 */
void
qntt_forward_partial(uint32_t *a, int logn)
{
    unsigned n;

    assert((unsigned)logn <= YUANYANG_LOGD && logn > 0);
    n = 1u << logn;

#define QNTT_STAGE(M) do {                                               \
        unsigned ht = n / (2u * (M));                                    \
        for (unsigned i = 0, j1 = 0; i < (M); i++, j1 += 2u * ht) {      \
            uint32_t s = qntt_GMb[(M) + i];                              \
            unsigned j2 = j1 + ht;                                       \
            for (unsigned j = j1; j < j2; j++) {                         \
                uint32_t u = a[j];                                       \
                uint32_t v = qntt_montymul(a[j + ht], s);                \
                a[j]      = qntt_add(u, v);                              \
                a[j + ht] = qntt_sub(u, v);                              \
            }                                                            \
        }                                                                \
    } while (0)

    QNTT_STAGE(1u << 0);
    if (logn == 1) return;

    QNTT_STAGE(1u << 1);
    if (logn == 2) return;

    QNTT_STAGE(1u << 2);
    if (logn == 3) return;

    QNTT_STAGE(1u << 3);
    if (logn == 4) return;

    QNTT_STAGE(1u << 4);
    if (logn == 5) return;

    QNTT_STAGE(1u << 5);
    if (logn == 6) return;

    QNTT_STAGE(1u << 6);
    if (logn == 7) return;

    QNTT_STAGE(1u << 7);
    if (logn == 8) return;

    QNTT_STAGE(1u << 8);
    if (logn == 9) return;

    QNTT_STAGE(1u << 9);
    if (logn == 10) return;

    QNTT_STAGE(1u << 10);

#undef QNTT_STAGE
}

void
qntt_forward(uint32_t a[YUANYANG_D])
{
	qntt_forward_partial(a,YUANYANG_LOGD);
}

/*
 * Compute the inverse direct negacyclic NTT on a ring element.
 */
void
qntt_inverse_partial(uint32_t *a, int modq, int logn)
{
    unsigned n;

    assert(logn >= 0 && (unsigned)logn <= YUANYANG_LOGD);
    n = 1u << logn;

#define QNTT_ISTAGE(HM) do {                                             \
        unsigned t = n / (2u * (HM));                                    \
        unsigned dt = t << 1;                                            \
        for (unsigned i = 0, j1 = 0; i < (HM); i++, j1 += dt) {           \
            uint32_t s = qntt_iGMb[(HM) + i];                            \
            unsigned j2 = j1 + t;                                        \
            for (unsigned j = j1; j < j2; j++) {                         \
                uint32_t u = a[j];                                       \
                uint32_t v = a[j + t];                                   \
                a[j]     = qntt_add(u, v);                               \
                a[j + t] = qntt_montymul(qntt_sub(u, v), s);             \
            }                                                            \
        }                                                                \
    } while (0)

    switch (logn) {
    case 11: QNTT_ISTAGE(1u << 10); /* fall through */
    case 10: QNTT_ISTAGE(1u << 9);  /* fall through */
    case 9:  QNTT_ISTAGE(1u << 8);  /* fall through */
    case 8:  QNTT_ISTAGE(1u << 7);  /* fall through */
    case 7:  QNTT_ISTAGE(1u << 6);  /* fall through */
    case 6:  QNTT_ISTAGE(1u << 5);  /* fall through */
    case 5:  QNTT_ISTAGE(1u << 4);  /* fall through */
    case 4:  QNTT_ISTAGE(1u << 3);  /* fall through */
    case 3:  QNTT_ISTAGE(1u << 2);  /* fall through */
    case 2:  QNTT_ISTAGE(1u << 1);  /* fall through */
    case 1:  QNTT_ISTAGE(1u << 0);  /* fall through */
    case 0:
    default:
        ;
    }

#undef QNTT_ISTAGE

    unsigned scale = (((uint64_t)QNTT_INV_N_MONTY)
        << (YUANYANG_LOGD - logn)) % QNTT;

    if (modq) {
        for (unsigned i = 0; i < n; i++) {
            a[i] = qntt_to_q(qntt_montymul(a[i], scale));
        }
    } else {
        for (unsigned i = 0; i < n; i++) {
            uint32_t sx = qntt_montymul(a[i], scale) % QNTT;
            a[i] = sx > QNTT_HALF ? sx - QNTT : sx;
        }
    }
}

void
qntt_inverse(uint32_t a[YUANYANG_D], int modq)
{
	qntt_inverse_partial(a,modq,YUANYANG_LOGD);
}

void
qntt_inverse_residue(uint32_t a[YUANYANG_D])
{
#define QNTT_RESIDUE_INVERSE_STAGE(HM) do {                     \
		const unsigned st = YUANYANG_D / (2u * (HM));           \
		for (unsigned i = 0, j1 = 0; i < (HM);                  \
			i++, j1 += 2u * st)                                 \
		{                                                       \
			const uint32_t s = qntt_iGMb[(HM) + i];             \
			const unsigned j2 = j1 + st;                        \
			for (unsigned j = j1; j < j2; j++) {                \
				const uint32_t u = a[j];                        \
				const uint32_t v = a[j + st];                   \
				a[j] = qntt_add(u, v);                          \
				a[j + st] = qntt_montymul(qntt_sub(u, v), s);   \
			}                                                   \
		}                                                       \
	} while (0)

	QNTT_RESIDUE_INVERSE_STAGE(1u << 10);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 9);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 8);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 7);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 6);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 5);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 4);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 3);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 2);
	QNTT_RESIDUE_INVERSE_STAGE(1u << 1);

#undef QNTT_RESIDUE_INVERSE_STAGE

	/*
	 * Fuse the final inverse butterfly with multiplication by d^(-1).
	 * Combining its Montgomery twiddle with the Montgomery normalization
	 * constant saves one reduction for every coefficient in the upper half.
	 */
	{
		const unsigned t = YUANYANG_D >> 1;
		const uint32_t scaled_twiddle = qntt_montymul(qntt_iGMb[1],
			QNTT_INV_N_MONTY);
		for (unsigned j = 0; j < t; j++) {
			const uint32_t u = a[j];
			const uint32_t v = a[j + t];
			a[j] = qntt_montymul(qntt_add(u, v), QNTT_INV_N_MONTY);
			a[j + t] = qntt_montymul(qntt_sub(u, v), scaled_twiddle);
		}
	}
}

void
qntt_encode_i64(
	uint32_t out[YUANYANG_D],
	const int64_t a[YUANYANG_D])
{
	/* Convert signed 64-bit coefficients to QNTT residues. */
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = qntt_norm_i64(a[i]);
	}
}

void
qntt_encode_i32(
	uint32_t out[YUANYANG_D],
	const int32_t a[YUANYANG_D])
{
	/* Convert signed 32-bit coefficients to QNTT residues. */
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = qntt_norm_i32(a[i]);
	}
}

void
qntt_center_i64(
	int64_t out[YUANYANG_D],
	const uint32_t a[YUANYANG_D])
{
	/* Interpret QNTT residues as centered signed representatives. */
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = qntt_center_u32(a[i]);
	}
}

void
qntt_to_montgomery(uint32_t a[YUANYANG_D])
{
	/* Import normal residues with a*R^2/R = a*R (mod QNTT). */
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		a[i] = qntt_montymul(a[i], QNTT_R2);
	}
}

void
qntt_pointwise_mul_to_partial(
	uint32_t *out,
	const uint32_t *a,
	const uint32_t *b,
	int logn)
{
	/*
	 * qntt_forward() outputs normal residues. Convert one operand to
	 * Montgomery representation, then perform the pointwise product; this
	 * is the same convention as Falcon's mq_poly_montymul_ntt().
	 */
	for (unsigned i = 0; i < (1u << (unsigned)logn); i++) {
		out[i] = qntt_montymul(qntt_montymul(a[i], QNTT_R2), b[i]);
	}
}

void
qntt_pointwise_mul_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a[YUANYANG_D],
	const uint32_t b[YUANYANG_D])
{
	qntt_pointwise_mul_to_partial(out,a,b,YUANYANG_LOGD);
}

void
qntt_pointwise_muladd_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a0[YUANYANG_D],
	const uint32_t b0[YUANYANG_D],
	const uint32_t a1[YUANYANG_D],
	const uint32_t b1[YUANYANG_D])
{
	/* Compute a0*b0 + a1*b1 in the NTT domain before one inverse NTT. */
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		uint32_t p0;
		uint32_t p1;

		p0 = qntt_montymul(qntt_montymul(a0[i], QNTT_R2), b0[i]);
		p1 = qntt_montymul(qntt_montymul(a1[i], QNTT_R2), b1[i]);
		out[i] = qntt_add(p0, p1);
	}
}

void
qntt_pointwise_montgomery_mul_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a_montgomery[YUANYANG_D],
	const uint32_t b[YUANYANG_D])
{
	/* a is already a*R, hence Montgomery reduction directly yields a*b. */
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = qntt_montymul(a_montgomery[i], b[i]);
	}
}

void
qntt_pointwise_montgomery_muladd_to(
	uint32_t out[YUANYANG_D],
	const uint32_t a0_montgomery[YUANYANG_D],
	const uint32_t b0[YUANYANG_D],
	const uint32_t a1_montgomery[YUANYANG_D],
	const uint32_t b1[YUANYANG_D])
{
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		const uint32_t p0 = qntt_montymul(a0_montgomery[i], b0[i]);
		const uint32_t p1 = qntt_montymul(a1_montgomery[i], b1[i]);
		out[i] = qntt_add(p0, p1);
	}
}

void
qntt_pointwise_mul(
	uint32_t a[YUANYANG_D],
	const uint32_t b[YUANYANG_D])
{
	qntt_pointwise_mul_to_partial(a, a, b,YUANYANG_LOGD);
}


void
qntt_mul(uint32_t a[YUANYANG_D], uint32_t b[YUANYANG_D])
{
	/*
	 * Pointwise QNTT product followed by inverse reduction to
	 * Yuanyang's public modulus q.
	 */
	qntt_pointwise_mul(a, b);
	qntt_inverse(a, 1);
}

void
qntt_mul_int_partial(uint32_t *a, uint32_t *b,int logn)
{
	/*
	 * Same pointwise product as qntt_mul(), but the inverse returns centered
	 * representatives modulo QNTT.  Kept for callers that need the auxiliary
	 * integer ring rather than reduction modulo Yuanyang q.
	 */
	qntt_pointwise_mul_to_partial(a,a, b,logn);
	qntt_inverse_partial(a, 0,logn);
}

void
qntt_mul_int(uint32_t a[YUANYANG_D], uint32_t b[YUANYANG_D])
{
	qntt_mul_int_partial(a,b,YUANYANG_LOGD);
}

void
yuanyang_mul_mod_xn_plus_1_ntt_big(
	uint16_t out[YUANYANG_D],
	const uint16_t h[YUANYANG_D],
	const int16_t s1[YUANYANG_D])
{
	alignas(64) uint32_t a[YUANYANG_D];
	alignas(64) uint32_t b[YUANYANG_D];

	for (unsigned i = 0; i < YUANYANG_D; i++) {
		a[i] = qntt_norm_i32(yuanyang_center_lift_q(h[i]));
		b[i] = qntt_norm_i32((int32_t)s1[i]);
	}

	qntt_forward(a);
	qntt_forward(b);
	qntt_mul(a, b);
	for (unsigned i = 0; i < YUANYANG_D; i++) {
		out[i] = a[i];
	}
}
