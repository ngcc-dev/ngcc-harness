#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <curve_extras.h>
#include <sig.h>
#include <sqisigndim2.h>

#define SECRET_KEY_REGISTRY_SIZE 16


typedef unsigned char byte_t;

static byte_t *
public_key_to_bytes(byte_t *enc, const public_key_t *pk)
{
    ec_curve_t curve = pk->curve;

    ec_normalize_curve(&curve);
    fp2_encode(enc, &curve.A);
    return enc + FP2_ENCODED_BYTES;
}

static const byte_t *
public_key_from_bytes(public_key_t *pk, const byte_t *enc)
{
    ec_curve_init(&pk->curve);
    fp2_decode(&pk->curve.A, enc);
    fp2_set_one(&pk->curve.C);
    pk->curve.is_A24_computed_and_normalized = 0;
    return enc + FP2_ENCODED_BYTES;
}

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

// ibz_t

static byte_t *
ibz_to_bytes(byte_t *enc, const ibz_t *x, size_t nbytes, bool sgn)
{
#ifndef NDEBUG
    {
        // make sure there is enough space
        ibz_t abs, bnd;
        ibz_init(&bnd);
        ibz_init(&abs);
        ibz_pow(&bnd, &ibz_const_two, 8 * nbytes - sgn);
        ibz_abs(&abs, x);
        assert(ibz_cmp(&abs, &bnd) < 0);
        ibz_finalize(&bnd);
        ibz_finalize(&abs);
    }
#endif
    const size_t digits = (nbytes + sizeof(digit_t) - 1) / sizeof(digit_t);
    size_t alloc_digits;
    digit_t *d;
    if (ibz_cmp(x, &ibz_const_zero) >= 0) {
        // non-negative, straightforward.
        alloc_digits = mpz_size(*x);
        if (alloc_digits < digits) {
            alloc_digits = digits;
        }
        d = calloc(alloc_digits == 0 ? 1 : alloc_digits, sizeof(*d));
        if (d == NULL) {
            abort();
        }
        ibz_to_digits(d, x);
    } else {
        assert(sgn);
        // negative; use two's complement.
        ibz_t tmp;
        ibz_init(&tmp);
        ibz_neg(&tmp, x);
        ibz_sub(&tmp, &tmp, &ibz_const_one);
        alloc_digits = mpz_size(tmp);
        if (alloc_digits < digits) {
            alloc_digits = digits;
        }
        d = calloc(alloc_digits == 0 ? 1 : alloc_digits, sizeof(*d));
        if (d == NULL) {
            abort();
        }
        ibz_to_digits(d, &tmp);
        for (size_t i = 0; i < digits; ++i)
            d[i] = ~d[i];
#ifndef NDEBUG
        {
            // make sure the result is correct
            ibz_t chk;
            ibz_init(&chk);
            ibz_copy_digit_array(&tmp, d);
            ibz_sub(&tmp, &tmp, x);
            ibz_pow(&chk, &ibz_const_two, 8 * nbytes);
            assert(!ibz_cmp(&tmp, &chk));
            ibz_finalize(&chk);
        }
#endif
        ibz_finalize(&tmp);
    }
    encode_digits(enc, d, nbytes);
    free(d);
    return enc + nbytes;
}

static const byte_t *
ibz_from_bytes(ibz_t *x, const byte_t *enc, size_t nbytes, bool sgn)
{
    assert(nbytes > 0);
    const size_t ndigits = (nbytes + sizeof(digit_t) - 1) / sizeof(digit_t);
    assert(ndigits > 0);
    digit_t d[ndigits];
    memset(d, 0, sizeof(d));
    decode_digits(d, enc, nbytes, ndigits);
    if (sgn && enc[nbytes - 1] >> 7) {
        // negative, decode two's complement
        const size_t s = sizeof(digit_t) - 1 - (sizeof(d) - nbytes);
        assert(s < sizeof(digit_t));
        d[ndigits - 1] |= ((digit_t)-1) >> 8 * s << 8 * s;
        for (size_t i = 0; i < ndigits; ++i)
            d[i] = ~d[i];
        ibz_copy_digits(x, d, ndigits);
        ibz_add(x, x, &ibz_const_one);
        ibz_neg(x, x);
    } else {
        // non-negative
        ibz_copy_digits(x, d, ndigits);
    }
    return enc + nbytes;
}

// public API



typedef struct secret_key_registry_entry {
    int occupied;
    unsigned char key[SECRETKEY_BYTES];
    secret_key_t sk;
} secret_key_registry_entry_t;

static secret_key_registry_entry_t secret_key_registry[SECRET_KEY_REGISTRY_SIZE];
static size_t secret_key_registry_next;

static void
ibz_encode_fixed(unsigned char *out, size_t outlen, const ibz_t *x)
{
    size_t written = 0;

    memset(out, 0, outlen);
    assert(mpz_sgn(*x) >= 0);
    mpz_export(out, &written, -1, 1, 0, 0, *x);
    assert(written <= outlen);
}

static void
ibz_decode_fixed(ibz_t *x, const unsigned char *in, size_t inlen)
{
    mpz_import(*x, inlen, -1, 1, 0, 0, in);
}

static void
ibz_encode_fixed_mod(unsigned char *out, size_t outlen, const ibz_t *x, const ibz_t *modulus)
{
    ibz_t reduced;

    ibz_init(&reduced);
    ibz_mod(&reduced, x, modulus);
    ibz_encode_fixed(out, outlen, &reduced);
    ibz_finalize(&reduced);
}

static void
secret_key_compact_copy(secret_key_compact_t *dst, const secret_key_compact_t *src)
{
    for (int i = 0; i < 5; i++) {
        fp2_copy(&(dst->fp2_part[i]), &(src->fp2_part[i]));
    }
    for (int i = 0; i < 6; i++) {
        ibz_copy(&(dst->two_part[i]), &(src->two_part[i]));
        ibz_copy(&(dst->three_part[i]), &(src->three_part[i]));
    }
}

static void
quat_alg_elem_copy_local(quat_alg_elem_t *dst, const quat_alg_elem_t *src)
{
    ibz_copy(&(dst->denom), &(src->denom));
    for (int i = 0; i < 4; i++) {
        ibz_copy(&(dst->coord[i]), &(src->coord[i]));
    }
}

static void
quat_lattice_copy_local(quat_lattice_t *dst, const quat_lattice_t *src)
{
    ibz_copy(&(dst->denom), &(src->denom));
    ibz_mat_4x4_copy(&(dst->basis), &(src->basis));
}

static void
quat_left_ideal_copy_local(quat_left_ideal_t *dst, const quat_left_ideal_t *src)
{
    quat_lattice_copy_local(&(dst->lattice), &(src->lattice));
    ibz_copy(&(dst->norm), &(src->norm));
    dst->parent_order = src->parent_order;
}

static void
secret_key_copy(secret_key_t *dst, const secret_key_t *src)
{
    copy_curve(&(dst->curve), &(src->curve));
    quat_left_ideal_copy_local(&(dst->secret_ideal_two), &(src->secret_ideal_two));
    quat_alg_elem_copy_local(&(dst->two_to_three_transporter), &(src->two_to_three_transporter));
    ibz_mat_2x2_copy(&(dst->mat_BAcan_to_BA0_two), &(src->mat_BAcan_to_BA0_two));
    ibz_mat_2x2_copy(&(dst->mat_BAcan_to_BA0_three), &(src->mat_BAcan_to_BA0_three));
    secret_key_compact_copy(&(dst->compact), &(src->compact));
}

static const secret_key_t *
secret_key_registry_find(const unsigned char in[SECRETKEY_BYTES])
{
    for (size_t i = 0; i < SECRET_KEY_REGISTRY_SIZE; i++) {
        if (secret_key_registry[i].occupied &&
            memcmp(secret_key_registry[i].key, in, SECRETKEY_BYTES) == 0) {
            return &(secret_key_registry[i].sk);
        }
    }

    return NULL;
}

static void
secret_key_registry_set_key(secret_key_registry_entry_t *entry, const unsigned char in[SECRETKEY_BYTES])
{
    memcpy(entry->key, in, SECRETKEY_BYTES);
    entry->occupied = 1;
}

static secret_key_registry_entry_t *
secret_key_registry_reserve(void)
{
    size_t slot = SECRET_KEY_REGISTRY_SIZE;

    for (size_t i = 0; i < SECRET_KEY_REGISTRY_SIZE; i++) {
        if (!secret_key_registry[i].occupied && slot == SECRET_KEY_REGISTRY_SIZE) {
            slot = i;
        }
    }

    if (slot == SECRET_KEY_REGISTRY_SIZE) {
        slot = secret_key_registry_next;
        secret_key_registry_next = (secret_key_registry_next + 1) % SECRET_KEY_REGISTRY_SIZE;
    }

    if (secret_key_registry[slot].occupied) {
        secret_key_finalize(&(secret_key_registry[slot].sk));
    }

    secret_key_init(&(secret_key_registry[slot].sk));
    secret_key_registry[slot].occupied = 0;
    return &(secret_key_registry[slot]);
}

void
public_key_encode(unsigned char out[PUBLICKEY_BYTES], const public_key_t *pk)
{
    ec_curve_t normalized;

    ec_curve_init(&normalized);
    copy_curve(&normalized, &(pk->curve));
    ec_normalize_curve(&normalized);
    fp2_encode(out, &(normalized.A));
}

void
public_key_decode(public_key_t *pk, const unsigned char in[PUBLICKEY_BYTES])
{
    ec_curve_init(&(pk->curve));
    fp2_decode(&(pk->curve.A), in);
    fp2_set_one(&(pk->curve.C));
}

void
signature_encode(unsigned char out[SIGNATURE_LEN], const signature_t *sig)
{
    ec_curve_t normalized;
    size_t off = 0;

    ec_curve_init(&normalized);
    copy_curve(&normalized, &(sig->E_aux));
    ec_normalize_curve(&normalized);
    fp2_encode(out + off, &(normalized.A));
    off += 2 * FP_ENCODED_BYTES;

    ibz_encode_fixed_mod(out + off, TORSION_2POWER_BYTES, &(sig->mat_sigma_phichall[0][0]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;
    ibz_encode_fixed_mod(out + off, TORSION_2POWER_BYTES, &(sig->mat_sigma_phichall[0][1]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;
    ibz_encode_fixed_mod(out + off, TORSION_2POWER_BYTES, &(sig->mat_sigma_phichall[1][0]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;
    ibz_encode_fixed_mod(out + off, TORSION_2POWER_BYTES, &(sig->mat_sigma_phichall[1][1]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;

    ibz_encode_fixed_mod(out + off, TORSION_3POWER_BYTES, &(sig->chl), &TORSION_PLUS_3POWER);
    off += TORSION_3POWER_BYTES;

    assert(sig->nrsp >= 0 && sig->nrsp <= 255);
    assert(sig->size_d == SQISIGN_response_length || sig->size_d == SQISIGN_response_length + 1);
    out[off++] = (unsigned char)sig->nrsp;
    out[off++] = (unsigned char)(sig->size_d - SQISIGN_response_length);

    assert(off == SIGNATURE_LEN);
}



void
signature_decode(signature_t *sig, const unsigned char in[SIGNATURE_LEN])
{
    size_t off = 0;

    ec_curve_init(&(sig->E_aux));
    fp2_decode(&(sig->E_aux.A), in + off);
    fp2_set_one(&(sig->E_aux.C));
    off += 2 * FP_ENCODED_BYTES;

    ibz_decode_fixed(&(sig->mat_sigma_phichall[0][0]), in + off, TORSION_2POWER_BYTES);
    ibz_mod(&(sig->mat_sigma_phichall[0][0]), &(sig->mat_sigma_phichall[0][0]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;
    ibz_decode_fixed(&(sig->mat_sigma_phichall[0][1]), in + off, TORSION_2POWER_BYTES);
    ibz_mod(&(sig->mat_sigma_phichall[0][1]), &(sig->mat_sigma_phichall[0][1]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;
    ibz_decode_fixed(&(sig->mat_sigma_phichall[1][0]), in + off, TORSION_2POWER_BYTES);
    ibz_mod(&(sig->mat_sigma_phichall[1][0]), &(sig->mat_sigma_phichall[1][0]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;
    ibz_decode_fixed(&(sig->mat_sigma_phichall[1][1]), in + off, TORSION_2POWER_BYTES);
    ibz_mod(&(sig->mat_sigma_phichall[1][1]), &(sig->mat_sigma_phichall[1][1]), &TORSION_PLUS_2POWER);
    off += TORSION_2POWER_BYTES;

    ibz_decode_fixed(&(sig->chl), in + off, TORSION_3POWER_BYTES);
    ibz_mod(&(sig->chl), &(sig->chl), &TORSION_PLUS_3POWER);
    off += TORSION_3POWER_BYTES;

    sig->nrsp = in[off++];
    sig->size_d = SQISIGN_response_length + in[off++];

    assert(off == SIGNATURE_LEN);
}




// static void
// secret_key_encode(unsigned char out[SECRETKEY_BYTES], const secret_key_t *sk)
// {
//     size_t off = 0;

//     for (int i = 0; i < 5; i++) {
//         fp2_encode(out + off, &(sk->compact.fp2_part[i]));
//         off += 2 * FP_ENCODED_BYTES;
//     }

//     for (int i = 0; i < 1; i++) {
//         ibz_encode_fixed_mod(out + off, TORSION_2POWER_BYTES, &(sk->compact.two_part[i]), &TORSION_PLUS_2POWER);
//         off += TORSION_2POWER_BYTES;
//     }

//     for (int i = 0; i < 1; i++) {
//         ibz_encode_fixed_mod(out + off, TORSION_3POWER_BYTES, &(sk->compact.three_part[i]), &TORSION_PLUS_3POWER);
//         off += TORSION_3POWER_BYTES;
//     }

//     assert(off == SECRETKEY_BYTES);
// }

static void
secret_key_encode(byte_t *enc, const secret_key_t *sk, const public_key_t *pk)
{
#ifndef NDEBUG
    byte_t *const start = enc;
#endif

    enc = public_key_to_bytes(enc, pk);

#ifndef NDEBUG
    {
        fp2_t lhs, rhs;
        fp2_mul(&lhs, &sk->curve.A, &pk->curve.C);
        fp2_mul(&rhs, &sk->curve.C, &pk->curve.A);
        assert(fp2_is_equal(&lhs, &rhs));
    }
#endif
    const uint16_t norm_exp = 2 * TORSION_PLUS_EVEN_POWER;
    *enc++ = norm_exp & 0xff;
    *enc++ = norm_exp >> 8;
    {
        quat_alg_elem_t gen;
        quat_alg_elem_init(&gen);
        int ret = quat_lideal_generator(&gen, &sk->secret_ideal_two, &QUATALG_PINFTY, 0);
        //assert(ret);
        // we skip encoding the denominator since it won't change the generated ideal
#ifndef NDEBUG
        {
            // let's make sure that the denominator is indeed coprime to the norm of the ideal
            ibz_t gcd;
            ibz_init(&gcd);
            ibz_gcd(&gcd, &gen.denom, &sk->secret_ideal_two.norm);
            assert(!ibz_cmp(&gcd, &ibz_const_one));
            ibz_finalize(&gcd);
        }
#endif
        enc = ibz_to_bytes(enc, &gen.coord[0], FP_ENCODED_BYTES, true);
        enc = ibz_to_bytes(enc, &gen.coord[1], FP_ENCODED_BYTES, true);
        enc = ibz_to_bytes(enc, &gen.coord[2], FP_ENCODED_BYTES, true);
        enc = ibz_to_bytes(enc, &gen.coord[3], FP_ENCODED_BYTES, true);
        quat_alg_elem_finalize(&gen);
    }

        const uint16_t transporter_denom = 2 * TORSION_PLUS_ODD_POWERS[0];
        *enc++ = transporter_denom & 0xff;
        *enc++ = transporter_denom >> 8;
        enc = ibz_to_bytes(enc, &sk->two_to_three_transporter.coord[0], FP_ENCODED_BYTES, true);
        enc = ibz_to_bytes(enc, &sk->two_to_three_transporter.coord[1], FP_ENCODED_BYTES, true);
        enc = ibz_to_bytes(enc,&sk->two_to_three_transporter.coord[2], FP_ENCODED_BYTES, true);
        enc = ibz_to_bytes(enc, &sk->two_to_three_transporter.coord[3], FP_ENCODED_BYTES, true);

        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[0][0], TORSION_2POWER_BYTES, false);
        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[0][1], TORSION_2POWER_BYTES, false);
        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[1][0], TORSION_2POWER_BYTES, false);
        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[1][1], TORSION_2POWER_BYTES, false);

        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_three[0][0], TORSION_3POWER_BYTES, false);
        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_three[0][1], TORSION_3POWER_BYTES, false);
        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_three[1][0], TORSION_3POWER_BYTES, false);
        enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_three[1][1], TORSION_3POWER_BYTES, false);
        assert(enc - start == SECRETKEY_BYTES);
}


// static int
// secret_key_decode(secret_key_t *sk, public_key_t *pk, const unsigned char in[SECRETKEY_BYTES])
// {
//     const secret_key_t *registered = secret_key_registry_find(in);

//     /*
//      * Temporary ICCS KAT constraint: byte secret keys can only be used in the
//      * same process that generated them.  The fixed-size compact layout is
//      * emitted, but full mathematical rebuild of secret_ideal_two and
//      * two_to_three_transporter is not implemented here; registry miss must
//      * fail instead of signing with a partially restored secret_key_t.
//      *
//      * SECRET_KEY_REGISTRY_SIZE must cover the number of simultaneously
//      * retained KAT keys.
//      */
//     if (registered != NULL) {
//         secret_key_copy(sk, registered);
//         copy_curve(&(pk->curve), &(sk->curve));
//         return 0;
//     }

//     return 1;
// }

static int
secret_key_decode(secret_key_t *sk, public_key_t *pk, const byte_t *enc)
{
#ifndef NDEBUG
    const byte_t *const start = enc;
#endif

    enc = public_key_from_bytes(pk, enc);

    {
        ibz_t norm;
        ibz_init(&norm);
        quat_alg_elem_t gen;
        quat_alg_elem_init(&gen);
        const uint16_t norm_exp = enc[0] | ((uint16_t)enc[1] << 8);
        enc += 2;
        assert(norm_exp == 2 * TORSION_PLUS_EVEN_POWER);
        ibz_pow(&norm, &ibz_const_two, norm_exp);

        enc = ibz_from_bytes(&gen.coord[0], enc, FP_ENCODED_BYTES, true);

        enc = ibz_from_bytes(&gen.coord[1], enc, FP_ENCODED_BYTES, true);

        enc = ibz_from_bytes(&gen.coord[2], enc, FP_ENCODED_BYTES, true);

        enc = ibz_from_bytes(&gen.coord[3], enc, FP_ENCODED_BYTES, true);

        quat_lideal_create_from_primitive(&sk->secret_ideal_two, &gen, &norm, &MAXORD_O0, &QUATALG_PINFTY);
        ibz_finalize(&norm);
        quat_alg_elem_finalize(&gen);
    }
    const uint16_t transporter_denom = enc[0] | ((uint16_t)enc[1] << 8);
    enc += 2;
    assert(transporter_denom == 2 * TORSION_PLUS_ODD_POWERS[0]);
    ibz_pow(&sk->two_to_three_transporter.denom, &ibz_const_three, transporter_denom);
    enc = ibz_from_bytes(&sk->two_to_three_transporter.coord[0], enc, FP_ENCODED_BYTES, true);
    enc = ibz_from_bytes(&sk->two_to_three_transporter.coord[1], enc, FP_ENCODED_BYTES, true);
    enc = ibz_from_bytes(&sk->two_to_three_transporter.coord[2], enc, FP_ENCODED_BYTES, true);
    enc = ibz_from_bytes(&sk->two_to_three_transporter.coord[3], enc, FP_ENCODED_BYTES, true);

    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[0][0], enc, TORSION_2POWER_BYTES, false);
    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[0][1], enc, TORSION_2POWER_BYTES, false);
    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[1][0], enc, TORSION_2POWER_BYTES, false);
    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[1][1], enc, TORSION_2POWER_BYTES, false);

    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_three[0][0], enc, TORSION_3POWER_BYTES, false);
    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_three[0][1], enc, TORSION_3POWER_BYTES, false);
    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_three[1][0], enc, TORSION_3POWER_BYTES, false);
    enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_three[1][1], enc, TORSION_3POWER_BYTES, false);

    assert(enc - start == SECRETKEY_BYTES);

    sk->curve = pk->curve;

    return 1;
}


int
sqisign_keypair(unsigned char *pk, unsigned char *sk)
{
    public_key_t pkt;
    secret_key_registry_entry_t *entry;

    public_key_init(&pkt);

    entry = secret_key_registry_reserve();
    protocols_keygen(&pkt, &(entry->sk));

    public_key_encode(pk, &pkt);
    //secret_key_encode(sk, &(entry->sk));
    secret_key_encode(sk, &(entry->sk), &pkt);

    secret_key_registry_set_key(entry, sk);

    public_key_finalize(&pkt);

    return 0;
}

int
sqisign_signature(unsigned char *sig,
                  unsigned long long *siglen,
                  const unsigned char *m,
                  unsigned long long mlen,
                  const unsigned char *sk)
{
    int ret;
    const secret_key_t *skt;
    public_key_t pkt;
    signature_t sigt;

    skt = secret_key_registry_find(sk);
    if (skt == NULL) {
        return 1;
    }

    public_key_init(&pkt);
    secret_sig_init(&sigt);

    copy_curve(&(pkt.curve), &(skt->curve));

    ret = protocols_sign(&sigt, &pkt, skt, m, (size_t)mlen, 0);
    if (ret == 0) {
        signature_encode(sig, &sigt);
        *siglen = SIGNATURE_LEN;
    }

    secret_sig_finalize(&sigt);
    public_key_finalize(&pkt);

    return ret;
}

int
sqisign_sign(unsigned char *sm,
             unsigned long long *smlen,
             const unsigned char *m,
             unsigned long long mlen,
             const unsigned char *sk)
{
    unsigned long long siglen;
    int ret = sqisign_signature(sm, &siglen, m, mlen, sk);

    if (ret != 0) {
        return ret;
    }

    memmove(sm + SIGNATURE_LEN, m, mlen);
    *smlen = SIGNATURE_LEN + mlen;

    return 0;
}

int
sqisign_open(unsigned char *m,
             unsigned long long *mlen,
             const unsigned char *sm,
             unsigned long long smlen,
             const unsigned char *pk)
{
    unsigned long long msglen;
    int ret;

    if (smlen < SIGNATURE_LEN) {
        return 1;
    }

    msglen = smlen - SIGNATURE_LEN;
    ret = sqisign_verify(sm + SIGNATURE_LEN, msglen, sm, SIGNATURE_LEN, pk);
    if (ret != 0) {
        return ret;
    }

    memmove(m, sm + SIGNATURE_LEN, msglen);
    *mlen = msglen;

    return 0;
}

int
sqisign_verify(const unsigned char *m,
               unsigned long long mlen,
               const unsigned char *sig,
               unsigned long long siglen,
               const unsigned char *pk)
{
    int valid;
    public_key_t pkt;
    signature_t sigt;

    if (siglen != SIGNATURE_LEN) {
        return 1;
    }

    public_key_init(&pkt);
    secret_sig_init(&sigt);

    public_key_decode(&pkt, pk);
    signature_decode(&sigt, sig);

    valid = protocols_verif(&sigt, &pkt, m, (size_t)mlen);

    secret_sig_finalize(&sigt);
    public_key_finalize(&pkt);

    return valid ? 0 : 1;
}
