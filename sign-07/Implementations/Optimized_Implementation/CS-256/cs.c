#include "cs.h"

/*************************************************
* Name:        CS_KeyGen
*
* Description: Generates public and private key.
*
* Arguments:   - uint8_t *pk: pointer to output public key (allocated
*                             array of PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key (allocated
*                             array of SECRETKEYBYTES bytes)
*
**************************************************/
void CS_KeyGen(uint8_t pk[PUBLICKEYBYTES],
               uint8_t sk[SECRETKEYBYTES])
{
    int i;
    const uint8_t* rho0, * rho1, * K;
    poly A0[k][l], s[l], e[k], t[k], t0[k], t1[k];
    uint8_t zeta[2 * SEEDBYTES + HBYTES], tr[HBYTES];

    get_random_number(&drng_algorithm, zeta, HBYTES * 8);
    zeta[HBYTES] = k;
    zeta[HBYTES + 1] = l;
    /* Get randomness for rho, rhoprime and key */
    H(zeta, 2 * SEEDBYTES + HBYTES, zeta, HBYTES + 2);
    rho0 = zeta;
    rho1 = zeta + SEEDBYTES;
    K = zeta + SEEDBYTES + HBYTES;

    /* Expand matrix */
    ExpandA(A0, rho0);

    /* Sample short vectors s and e */
    ExpandS(s, e, rho1);

    /* Matrix-vector multiplication */
    for (i = 0; i < l; i++) { t0[i] = NTT(&s[i]); }
    MatrixVectorNTT(t, A0, t0);
    for (i = 0; i < k; i++) { INTT(&t[i]); }
    AddVector(t, t, e); /* Add error vector e */
    LazyReductionVector(t);
    Power2RoundVector(t1, t0, t);

    /* Extract b1 and write public key */
    pkEncode(pk, rho0, t1);

    H(tr, HBYTES, pk, PUBLICKEYBYTES);
    /* write secret key */
    skEncode(sk, rho0, K, tr, s, e, t0, t1);
}

/*************************************************
* Name:        CS_Sign
*
* Description: Computes signature.
*
* Arguments:   - uint8_t *sig:   pointer to output signature (of length SIGNATUREBYTES)
*              - uint8_t *sk:    pointer to bit-packed secret key
*              - uint8_t *M:     pointer to message to be signed
*              - size_t len:     length of message
*
* Returns 0 (success)
**************************************************/
int CS_Sign(uint8_t sig[SIGNATUREBYTES],
            const uint8_t sk[SECRETKEYBYTES],
            const uint8_t* M,
            int len)
{
    int i, fail;
    uint16_t kappa = 0;
    poly A[k][l], s[k + l];
    int32_t coordinates[tau];
    uint8_t* buf = malloc(len + 4 * HBYTES);
    poly c, b[k], t0[k], t1[k], w[k], wh[k], y[k + l + 1], z[l + 1];
    uint8_t rho[SEEDBYTES], K[SEEDBYTES], tr[HBYTES], rhoprime[HBYTES];
    uint8_t cwave[POLYC_PACKEDBYTES], mu[HBYTES + k * POLYW_PACKEDBYTES + n / 8];

    skDecode(rho, K, tr, s, t0, t1, sk);
    /* Expand matrix and transform vectors */
    ExpandA(A, rho);

    ShiftLeftVector(t1, beta);
    for (i = 0; i < k; i++) { t1[i] = NTT(&t1[i]); }

    /* Compute H(tr||M') */
    memcpy(buf, tr, HBYTES);
    memcpy(buf + HBYTES, M, len);
    H(mu, HBYTES, buf, HBYTES + len);

    /* Compute H(K||rnd||mu) */
    memcpy(buf, K, SEEDBYTES);
    get_random_number(&drng_algorithm, buf + SEEDBYTES, HBYTES * 8);
    memcpy(buf + SEEDBYTES + HBYTES, mu, HBYTES);
    H(rhoprime, HBYTES, buf, 2 * HBYTES + SEEDBYTES);

sign:
    /* Sample intermediate vector y */
    ExpandMask(y, rhoprime, kappa);
    kappa += (k + l + 1);
    for (i = 0; i <= l; i++) { z[i] = NTT(&y[i]); }

    /* Matrix-vector multiplication */
    MatrixVectorNTT(w, A, z + 1);
    ScalarVectorNTT(wh, z[0], t1);
    SubVector(w, w, wh);
    for (i = 0; i < k; i++) { INTT(&w[i]); }
    AddVector(w, w, y + l + 1);
    LazyReductionVector(w);
    ExchangeModulus(w, y[0]);
    HighBitsVector(wh, w);

    /* Decompose w and call the random oracle */
    w1Encode(mu + HBYTES, wh);
    CheckParityPoly(&z[0], y[0]);
    SimpleBitPack(mu + HBYTES + k * POLYW_PACKEDBYTES, &z[0], 1);
    H(cwave, HBYTES, mu, sizeof(mu));
    SampleInBall(&c, coordinates, cwave);

    fail = SampleSign(y, &c, coordinates, s, rhoprime, kappa++);
    if (fail == -1)
        return 1;
    else if (fail == 1)
        goto sign;

    ScalarMulVector(b, c, coordinates, t0);
    SubVector(y + l + 1, y + l + 1, b);
    HighBitsZ2Vector(y + l + 1, w, y + l + 1);
    hModpVector(y + l + 1, wh, y + l + 1, 1);

    /* Write signature */
    if (sigEncode(sig, cwave, y))
        goto sign;

    free(buf);
    return 0;
}

/*************************************************
* Name:        CS_Verify
*
* Description: Verifies signature.
*
* Arguments:   - const uint8_t *pk: pointer to bit-packed public key
*              - const uint8_t *M: pointer to message
*              - int len: length of message
*              - const uint8_t *sig: pointer to input signature
*
* Returns true if signature could be verified correctly and false otherwise
**************************************************/
bool CS_Verify(const uint8_t pk[PUBLICKEYBYTES],
               const uint8_t* M,
               int len,
               const uint8_t sig[SIGNATUREBYTES])
{
    int32_t i, coordinates[tau], fail = 0;
    uint8_t* buf = malloc(len + HBYTES);
    poly A[k][l], z[k + l + 1], t1[k], w1[k], w0[k], cp;
    uint8_t rho[SEEDBYTES], mu[HBYTES + k * POLYW_PACKEDBYTES + n / 8], cwave[POLYC_PACKEDBYTES];

    pkDecode(rho, t1, pk);
    if (sigDecode(cwave, z, sig))
        return false;

    if (CheckNormPoly(&z[0], B0 - N) || CheckNormVector(z + 1, l, B1 - tau))
        return false; 

    ExpandA(A, rho);
    SampleInBall(&cp, coordinates, cwave);
    SubPoly(&cp, &z[0], &cp);
    ShiftLeftVector(t1, beta);

    H(mu, HBYTES, pk, PUBLICKEYBYTES);
    /* Compute H(H(pk)||M') */
    memcpy(buf, mu, HBYTES);
    memcpy(buf + HBYTES, M, len);
    H(mu, HBYTES, buf, HBYTES + len);

    for (i = 0; i < k; i++) { t1[i] = NTT(&t1[i]); }
    for (i = 0; i <= l; i++) { z[i] = NTT(&z[i]); }

    /* Matrix-vector multiplication */
    MatrixVectorNTT(w1, A, z + 1);
    ScalarVectorNTT(w0, z[0], t1);
    SubVector(w0, w1, w0);
    for (i = 0; i < k; i++) { INTT(&w0[i]); }
    LazyReductionVector(w0);
    ExchangeModulus(w0, cp);
    HighBitsVector(w1, w0);

    hModpVector(w1, w1, z + l + 1, 0);
    CheckParityPoly(&cp, cp);

    /* Decompose w and call the random oracle */
    w1Encode(mu + HBYTES, w1);
    SimpleBitPack(mu + HBYTES + k * POLYW_PACKEDBYTES, &cp, 1);
    H(mu, HBYTES, mu, sizeof(mu));
    for (i = 0; i < HBYTES; i++) { fail |= mu[i] ^ cwave[i]; }
    if (fail) 
        return false; 

    ScalarVector(w1, alpha);
    SubVector(w1, w1, w0);
    AddPoly(&w1[0], &w1[0], &cp);
    HalfCenterModVector(w1);
    if (CheckNormVector(w1, k, B2 - tau + alpha / 4 + 1 + tau * (1 << (beta - 1))))
        return false;

    free(buf);
    return true;
}