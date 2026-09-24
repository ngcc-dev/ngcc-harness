/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "hash_wrapper.h"
#include "ntl.h"
#include "decode_ml.h"
#include "sampling.h"
#include "kem.h"
#include "conversions.h"
#include "mlthre_runtime_sampling.h"
#include "xof_prng.h"

// Function H uses the API_PKC XOF to produce e from m.
_INLINE_ status_t functionH(
        OUT uint8_t * e,
        IN const uint8_t * m)
{
    status_t res = SUCCESS;

    // format m as the seed input:
    seed_t seed_for_hash;
    memcpy(seed_for_hash.raw, m, ELL_SIZE);

    // use the seed to generate sparse error vector e:
    DMSG("    Generating random error.\n");
    xof_prng_state_t prng_state = {0};
    res = xof_prng_init(seed_for_hash.raw, ELL_SIZE, &prng_state); CHECK_STATUS(res);
    res = generate_sparse_rep(e, T1, N_BITS, &prng_state); CHECK_STATUS(res);

    EXIT:
    DMSG("  Exit functionH.\n");
    return res;
}

// Function L. Computes L(e0 || e1)
_INLINE_ status_t functionL(
        OUT uint8_t * output,
        IN const uint8_t * e)
{
    status_t res = SUCCESS;
    uint8_t e_split[2 * R_SIZE] = {0};

    ntl_split_polynomial(e_split, &e_split[R_SIZE], e);

    res = bike_hash(output, ELL_SIZE, e_split, 2*R_SIZE);

    DMSG("  Exit functionL.\n");
    return res;
}

// Function K. Computes K(m || c0 || c1).
_INLINE_ status_t functionK(
        OUT uint8_t * output,
        IN const uint8_t * m,
        IN const uint8_t * c0,
        IN const uint8_t * c1)
{
    status_t res = SUCCESS;

    // preparing buffer with: [m || c0 || c1]
    uint8_t tmp1[ELL_SIZE + 2*R_SIZE] = {0};
    memcpy(tmp1, m, ELL_SIZE);
    memcpy(tmp1 + ELL_SIZE, c0, R_SIZE);
    memcpy(tmp1 + ELL_SIZE + R_SIZE, c1, ELL_SIZE);

    // shared secret = K(m || c0 || c1)
    res = bike_hash(output, ELL_SIZE, tmp1, 2*ELL_SIZE + R_SIZE);
  
    DMSG("  Exit functionK.\n");
    return res;
}

_INLINE_ status_t compute_syndrome(OUT syndrome_t* syndrome,
        IN const ct_t* ct,
        IN const sk_t* sk)
{
    status_t res = SUCCESS;
    uint8_t s_tmp_bytes[R_BITS] = {0};
    uint8_t s0[R_SIZE] = {0};

    // syndrome: s = c0*h0
    ntl_mod_mul(s0, sk->val0, ct->val0);

    // store the syndrome in a bit array
    convertByteToBinary(s_tmp_bytes, s0, R_BITS);
    transpose(syndrome->raw, s_tmp_bytes);

    DMSG("  Exit compute_syndrome.\n");

    return res;
}

////////////////////////////////////////////////////////////////
//The three APIs below (keypair, enc, dec) are defined by NIST:
////////////////////////////////////////////////////////////////
int crypto_kem_keypair(OUT unsigned char *pk, OUT unsigned char *sk)
{
    //Convert to these implementation types
    sk_t* l_sk = (sk_t*)sk;
    pk_t* l_pk = (pk_t*)pk;

    // return code
    status_t res = SUCCESS;

    //For NIST DRBG_CTR
    double_seed_t seeds = {0};
    xof_prng_state_t h_prng_state = {0};

    //Get the entropy seeds
    get_seeds(&seeds, KEYGEN_SEEDS);

    // sk = (h0, h1, sigma)
    uint8_t * h0 = l_sk->val0;
    uint8_t * h1 = l_sk->val1;
    uint8_t * sigma = l_sk->sigma;

    uint8_t inv_h0[R_SIZE] = {0};

    DMSG("  Enter crypto_kem_keypair.\n");
    DMSG("    Calculating the secret key.\n");

    res = xof_prng_init(seeds.s1.raw, ELL_SIZE, &h_prng_state); CHECK_STATUS(res);
    res = generate_sparse_rep(h0, DV, R_BITS, &h_prng_state); CHECK_STATUS(res);
    res = generate_sparse_rep(h1, DV, R_BITS, &h_prng_state); CHECK_STATUS(res);

    // use the second seed as sigma
    memcpy(sigma, seeds.s2.raw, ELL_SIZE);

    DMSG("    Calculating the public key.\n");

    // pk = (1, h1*h0^(-1)), the first pk component (1) is implicitly assumed
    ntl_mod_inv(inv_h0, h0);
    ntl_mod_mul(l_pk->val, h1, inv_h0);

    EDMSG("h0: "); print((uint64_t*)l_sk->val0, R_BITS);
    EDMSG("h1: "); print((uint64_t*)l_sk->val1, R_BITS);
    EDMSG("h: "); print((uint64_t*)l_pk->val, R_BITS);
    EDMSG("sigma: "); print((uint64_t*)l_sk->sigma, ELL_BITS);

    EXIT:
    DMSG("  Exit crypto_kem_keypair.\n");
    return res;
}

//Encapsulate - pk is the public key,
//              ct is a key encapsulation message (ciphertext),
//              ss is the shared secret.
int crypto_kem_enc(OUT unsigned char *ct,
        OUT unsigned char *ss,
        IN  const unsigned char *pk)
{
    DMSG("  Enter crypto_kem_enc.\n");

    status_t res = SUCCESS;

    //Convert to these implementation types
    const pk_t* l_pk = (pk_t*)pk;
    ct_t* l_ct = (ct_t*)ct;
    ss_t* l_ss = (ss_t*)ss;

    //For NIST DRBG_CTR.
    double_seed_t seeds = {0};

    //Get the entropy seeds.
    get_seeds(&seeds, ENCAPS_SEEDS);

    // quantity m:
    uint8_t m[ELL_SIZE] = {0};

    // error vector:
    uint8_t e[N_SIZE] = {0};
    uint8_t e0[R_SIZE] = {0};
    uint8_t e1[R_SIZE] = {0};

    // temporary buffer:
    uint8_t tmp[ELL_SIZE] = {0};

    //random data generator; Using seed s1
    memcpy(m, seeds.s1.raw, ELL_SIZE);

    // (e0, e1) = H(m)
    res = functionH(e, m); CHECK_STATUS(res);
    mlthre_runtime_sampling_capture_enc_error(e);
    ntl_split_polynomial(e0, e1, e);

    // ct = (c0, c1) = (e0 + e1*h, L(e0, e1) \XOR m)
    ntl_mod_mul(l_ct->val0, e1, l_pk->val);
    ntl_add(l_ct->val0, l_ct->val0, e0);
    res = functionL(tmp, e); CHECK_STATUS(res);
    for (uint32_t i = 0; i < ELL_SIZE; i++)
        l_ct->val1[i] = tmp[i] ^ m[i];

    // Function K:
    //shared secret =  K(m || c0 || c1)
    res = functionK(l_ss->raw, m, l_ct->val0, l_ct->val1); CHECK_STATUS(res);

    EDMSG("ss: "); print((uint64_t*)l_ss->raw, sizeof(*l_ss)*8);

    EXIT:

    DMSG("  Exit crypto_kem_enc.\n");
    return res;
}

//Decapsulate - ct is a key encapsulation message (ciphertext),
//              sk is the private key,
//              ss is the shared secret
int crypto_kem_dec(OUT unsigned char *ss,
        IN const unsigned char *ct,
        IN const unsigned char *sk)
{
    DMSG("  Enter crypto_kem_dec.\n");
    status_t res = SUCCESS;

    // convert to this implementation types
    const sk_t* l_sk = (sk_t*)sk;
    const ct_t* l_ct = (ct_t*)ct;
    ss_t* l_ss = (ss_t*)ss;

    int failed = 0;

    // for NIST DRBG_CTR
    double_seed_t seeds = {0};
  
    uint8_t e_recomputed[N_SIZE] = {0};

    uint8_t Le0e1[ELL_SIZE + 2*R_SIZE] = {0};
    uint8_t m_prime[ELL_SIZE] = {0};

    uint32_t h0_compact[DV] = {0};
    uint32_t h1_compact[DV] = {0};

    uint8_t e_prime[N_SIZE] = {0};
    uint8_t e_twoprime[R_BITS*2] = {0};
   
    uint8_t e_tmp1[R_BITS*2] = {0};
    uint8_t e_tmp2[N_SIZE] = {0};

    uint8_t e0rand[R_SIZE] = {0};
    uint8_t e1rand[R_SIZE] = {0};

    int rc;

    DMSG("  Converting to compact rep.\n");
    convert2compact(h0_compact, l_sk->val0);
    convert2compact(h1_compact, l_sk->val1);

    DMSG("  Computing s.\n");
    syndrome_t syndrome;

       // Step 1. computing syndrome:
    res = compute_syndrome(&syndrome, l_ct, l_sk); CHECK_STATUS(res);

    // Step 2. decoding:
    DMSG("  Decoding.\n");
    rc = BGF_decoder(e_tmp1, syndrome.raw, h0_compact, h1_compact);

    convertBinaryToByte(e_prime, e_tmp1, 2*R_BITS);

    // Step 3. compute L(e0 || e1)
    res = functionL(Le0e1, e_prime); CHECK_STATUS(res);

    // Step 4. retrieve m' = c1 \xor L(e0 || e1)
    for(uint32_t i = 0; i < ELL_SIZE; i++)
    {
        m_prime[i] = l_ct->val1[i] ^ Le0e1[i];
    }

    // Step 5. (e0, e1) = H(m)
    res = functionH(e_recomputed, m_prime); CHECK_STATUS(res);

    if (!safe_cmp(e_recomputed, e_prime, N_SIZE))
    {
        DMSG("recomputed error vector does not match decoded error vector\n");
        failed = 1;
    }

    // Step 6. compute shared secret k = K()
    if (failed) {
        // shared secret = K(sigma || c0 || c1)
        res = functionK(l_ss->raw, l_sk->sigma, l_ct->val0, l_ct->val1);
        CHECK_STATUS(res);
    }
    else
    {
       // shared secret = K(m' || c0 || c1)
       res = functionK(l_ss->raw, m_prime, l_ct->val0, l_ct->val1);
       CHECK_STATUS(res);
    }

    EXIT:

    DMSG("  Exit crypto_kem_dec.\n");
    return res;
}
