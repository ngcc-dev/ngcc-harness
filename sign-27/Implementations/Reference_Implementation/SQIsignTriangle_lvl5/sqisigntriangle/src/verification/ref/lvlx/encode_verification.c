#include <verification.h>
#include <string.h>
#include <tutil.h>
#include <fp2.h>
#include <encoded_sizes.h>
#include <torsion_constants.h>
#include <biextension.h>
#include <mp.h>
#include <assert.h>

typedef unsigned char byte_t;

// digits

static void
encode_digits(byte_t *enc, const digit_t *x, size_t nbytes)
{
#ifdef TARGET_BIG_ENDIAN
    const size_t ndigits = nbytes / sizeof(digit_t);
    const size_t rem = nbytes % sizeof(digit_t);

    for (size_t i = 0; i < ndigits; i++)
        ((digit_t *)enc)[i] = BSWAP_DIGIT(x[i]);
    if (rem) {
        digit_t ld = BSWAP_DIGIT(x[ndigits]);
        memcpy(enc + ndigits * sizeof(digit_t), (byte_t *)&ld, rem);
    }
#else
    memcpy(enc, (const byte_t *)x, nbytes);
#endif
}

static void
decode_digits(digit_t *x, const byte_t *enc, size_t nbytes, size_t ndigits)
{
    assert(nbytes <= ndigits * sizeof(digit_t));
    memcpy((byte_t *)x, enc, nbytes);
    memset((byte_t *)x + nbytes, 0, ndigits * sizeof(digit_t) - nbytes);

#ifdef TARGET_BIG_ENDIAN
    for (size_t i = 0; i < ndigits; i++)
        x[i] = BSWAP_DIGIT(x[i]);
#endif
}

// fp2_t

static byte_t *
fp2_to_bytes(byte_t *enc, const fp2_t *x)
{
    fp2_encode(enc, x);
    return enc + FP2_ENCODED_BYTES;
}

static const byte_t *
fp2_from_bytes(fp2_t *x, const byte_t *enc)
{
    fp2_decode(x, enc);
    return enc + FP2_ENCODED_BYTES;
}

// curves and points

static byte_t *
proj_to_bytes(byte_t *enc, const fp2_t *x, const fp2_t *z)
{
    assert(!fp2_is_zero(z));
    fp2_t tmp = *z;
    fp2_inv(&tmp);
#ifndef NDEBUG
    {
        fp2_t chk;
        fp2_mul(&chk, z, &tmp);
        fp2_t one;
        fp2_set_one(&one);
        assert(fp2_is_equal(&chk, &one));
    }
#endif
    fp2_mul(&tmp, x, &tmp);
    enc = fp2_to_bytes(enc, &tmp);
    return enc;
}

static const byte_t *
proj_from_bytes(fp2_t *x, fp2_t *z, const byte_t *enc)
{
    enc = fp2_from_bytes(x, enc);
    fp2_set_one(z);
    return enc;
}

static byte_t *
ec_curve_to_bytes(byte_t *enc, const ec_curve_t *curve)
{
    return proj_to_bytes(enc, &curve->A, &curve->C);
}

static const byte_t *
ec_curve_from_bytes(ec_curve_t *curve, const byte_t *enc)
{
    memset(curve, 0, sizeof(*curve));
    return proj_from_bytes(&curve->A, &curve->C, enc);
}

static byte_t *
ec_point_to_bytes(byte_t *enc, const ec_point_t *point)
{
    return proj_to_bytes(enc, &point->x, &point->z);
}

static const byte_t *
ec_point_from_bytes(ec_point_t *point, const byte_t *enc)
{
    return proj_from_bytes(&point->x, &point->z, enc);
}

static byte_t *
ec_basis_to_bytes(byte_t *enc, const ec_basis_t *basis)
{
    enc = ec_point_to_bytes(enc, &basis->P);
    enc = ec_point_to_bytes(enc, &basis->Q);
    enc = ec_point_to_bytes(enc, &basis->PmQ);
    return enc;
}

static const byte_t *
ec_basis_from_bytes(ec_basis_t *basis, const byte_t *enc)
{
    enc = ec_point_from_bytes(&basis->P, enc);
    enc = ec_point_from_bytes(&basis->Q, enc);
    enc = ec_point_from_bytes(&basis->PmQ, enc);
    return enc;
}

static byte_t *
scalar_to_bytes(byte_t *enc, const digit_t *x, size_t nbytes)
{
    encode_digits(enc, x, nbytes);
    return enc + nbytes;
}

static const byte_t *
scalar_from_bytes(digit_t *x, const byte_t *enc, size_t nbytes)
{
    decode_digits(x, enc, nbytes, NWORDS_ORDER);
    return enc + nbytes;
}

static byte_t *
q_to_bytes(byte_t *enc, const digit_t *q)
{
#ifndef NDEBUG
    ibz_t q_int;
    ibz_init(&q_int);
    // q is a pointer to a NWORDS_ORDER-digit array; pass the length explicitly.
    // (ibz_copy_digit_array() uses sizeof(q)/sizeof(*q), which is 1 for a pointer
    //  and trips -Werror=sizeof-pointer-div on newer compilers, e.g. GCC 15.)
    ibz_copy_digits(&q_int, q, NWORDS_ORDER);
    assert(ibz_bitsize(&q_int) <= 8 * SQISIGNTRIANGLE_Q_BYTES);
    ibz_finalize(&q_int);
#endif

    return scalar_to_bytes(enc, q, SQISIGNTRIANGLE_Q_BYTES);
}

static const byte_t *
q_from_bytes(digit_t *q, const byte_t *enc)
{
    decode_digits(q, enc, SQISIGNTRIANGLE_Q_BYTES, NWORDS_ORDER);
    return enc + SQISIGNTRIANGLE_Q_BYTES;
}

static byte_t *
compressed_aux_basis_to_bytes(byte_t *enc, const new_signature_t *sig)
{
    ibz_t q;
    ibz_init(&q);
    ibz_copy_digit_array(&q, sig->q);
    const int e_rsp = ibz_bitsize(&q);
    assert(e_rsp >= 3 && e_rsp <= TORSION_EVEN_POWER);

    ec_curve_t E_aux;
    int valid_curve UNUSED = ec_curve_init_from_A(&E_aux, &sig->E_aux_A);
    assert(valid_curve);

    ec_basis_t full_basis;
    (void)ec_curve_to_basis_2f_to_hint(&full_basis, &E_aux, TORSION_EVEN_POWER);

    digit_t r1[NWORDS_ORDER], r2[NWORDS_ORDER], s1[NWORDS_ORDER], s2[NWORDS_ORDER];
    ec_dlog_2_tate(r1, r2, s1, s2, &full_basis, &sig->B_aux, &E_aux, e_rsp);

    enc = scalar_to_bytes(enc, r1, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);
    enc = scalar_to_bytes(enc, r2, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);
    enc = scalar_to_bytes(enc, s1, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);
    enc = scalar_to_bytes(enc, s2, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);

    ibz_finalize(&q);
    return enc;
}

static const byte_t *
compressed_aux_basis_from_bytes(new_signature_t *sig, const byte_t *enc)
{
    digit_t r1[NWORDS_ORDER], r2[NWORDS_ORDER], s1[NWORDS_ORDER], s2[NWORDS_ORDER];
    digit_t d1[NWORDS_ORDER], d2[NWORDS_ORDER];

    enc = scalar_from_bytes(r1, enc, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);
    enc = scalar_from_bytes(r2, enc, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);
    enc = scalar_from_bytes(s1, enc, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);
    enc = scalar_from_bytes(s2, enc, SQISIGNTRIANGLE_BASIS_COEFF_BYTES);

    ibz_t q;
    ibz_init(&q);
    ibz_copy_digit_array(&q, sig->q);
    const int e_rsp = ibz_bitsize(&q);
    assert(e_rsp >= 3 && e_rsp <= TORSION_EVEN_POWER);

    ec_curve_t E_aux;
    int valid_curve UNUSED = ec_curve_init_from_A(&E_aux, &sig->E_aux_A);
    assert(valid_curve);

    ec_basis_t full_basis;
    (void)ec_curve_to_basis_2f_to_hint(&full_basis, &E_aux, TORSION_EVEN_POWER);

    int ok UNUSED = ec_biscalar_mul(&sig->B_aux.P, r1, r2, e_rsp, &full_basis, &E_aux);
    assert(ok);
    ok = ec_biscalar_mul(&sig->B_aux.Q, s1, s2, e_rsp, &full_basis, &E_aux);
    assert(ok);

    mp_sub(d1, r1, s1, NWORDS_ORDER);
    mp_sub(d2, r2, s2, NWORDS_ORDER);
    mp_mod_2exp(d1, e_rsp, NWORDS_ORDER);
    mp_mod_2exp(d2, e_rsp, NWORDS_ORDER);
    ok = ec_biscalar_mul(&sig->B_aux.PmQ, d1, d2, e_rsp, &full_basis, &E_aux);
    assert(ok);

    ec_dbl_iter_basis(&sig->B_aux, TORSION_EVEN_POWER - e_rsp, &sig->B_aux, &E_aux);

    ibz_finalize(&q);
    return enc;
}

// public API

byte_t *
public_key_to_bytes(byte_t *enc, const public_key_t *pk)
{
#ifndef NDEBUG
    const byte_t *const start = enc;
#endif
    enc = ec_curve_to_bytes(enc, &pk->curve);
    *enc++ = pk->hint_pk;
    assert(enc - start == PUBLICKEY_BYTES);
    return enc;
}

const byte_t *
public_key_from_bytes(public_key_t *pk, const byte_t *enc)
{
#ifndef NDEBUG
    const byte_t *const start = enc;
#endif
    enc = ec_curve_from_bytes(&pk->curve, enc);
    pk->hint_pk = *enc++;
    assert(enc - start == PUBLICKEY_BYTES);
    return enc;
}

void
new_signature_to_bytes(byte_t *enc, const new_signature_t *sig)
{
#ifndef NDEBUG
    byte_t *const start = enc;
#endif

    enc = fp2_to_bytes(enc, &sig->E_aux_A);
    enc = compressed_aux_basis_to_bytes(enc, sig);
    enc = q_to_bytes(enc, sig->q);

    assert(enc - start == NEW_SIGNATURE_BYTES);
}

void
new_signature_from_bytes(new_signature_t *sig, const byte_t *enc)
{
#ifndef NDEBUG
    const byte_t *const start = enc;
#endif

    enc = fp2_from_bytes(&sig->E_aux_A, enc);
    const byte_t *basis_enc = enc;
    enc += SQISIGNTRIANGLE_COMPRESSED_BASIS_BYTES;
    enc = q_from_bytes(sig->q, enc);
    compressed_aux_basis_from_bytes(sig, basis_enc);

    assert(enc - start == NEW_SIGNATURE_BYTES);
}
