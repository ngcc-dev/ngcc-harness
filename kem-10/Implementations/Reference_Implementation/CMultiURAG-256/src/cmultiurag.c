/** 
 * \file cmultiurag.c
 * \brief Implementation of the CMultiURAG.PKE public key encryption scheme
 *
 * The CMultiURAG.PKE scheme is constructed from the (decisional) Rank Support
 * Learning (RSL) problem. Main components:
 *   - AG code C_{EG}(g, k): a public [N1*N2, k] Augmented Gabidulin code determined by generator g
 *   - Random linear code C_{rand}(H): a random [2n, n] linear code with parity-check [I_n, H]
 *   - Random linear code C_{rand}(H, S): a random [2n+N1, n] linear code with parity-check
 *     [I_n, 0, H^T; 0, I_{N1}, S^T]
 *
 * See algorithm specification Section 3.3, Figure 2.
 */

 #include "cmultiurag.h"
 #include "parameters.h"
 #include "rbc_vspace.h"
 #include "rbc_mat.h"
 #include "augmented_gabidulin.h"
 #include "parsing.h"
 
 /** 
  * \fn void cmultiurag_pke_keygen(uint8_t* pk, uint8_t* sk)
  * \brief PKE key generation (PKE.KGen)
  *
  * Corresponds to Figure 2, lines 04-09:
  *   1. Sample public random matrix H from seed θ1
  *   2. Sample public EG code generator g of weight t from seed θ1
  *   3. Sample secret key matrices (X, Y) with wt_R(X, Y) = w1, 1 ∈ Supp(X, Y)
  *      from seed θ2
  *   4. Compute public matrix S = X + H·Y
 *   5. pk is serialized as (S, pk_seed), while the secret key stores sk_seed
 *      so that (X, Y) can be deterministically regenerated during parsing
  *
  * \param[out] pk Public key byte string
  * \param[out] sk Secret key byte string
  */
void cmultiurag_pke_keygen(uint8_t* pk, uint8_t* sk) {
  random_source sk_seedexpander;
  random_source pk_seedexpander;
  random_source_init(&sk_seedexpander, RANDOM_SOURCE_SEEDEXP);
  random_source_init(&pk_seedexpander, RANDOM_SOURCE_SEEDEXP);
  uint8_t sk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
  uint8_t pk_seed[SEEDEXPANDER_SEED_BYTES] = {0};
  rbc_mat X, Y, S, H;
  rbc_vec g;
  rbc_vspace support_1;

  rbc_vspace_init(&support_1, CMULTIURAG_PARAM_W1);

  rbc_mat_init(&X, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_init(&Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_vec_init(&g, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_mat_init(&H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N);
  rbc_mat_init(&S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);


  random_source prng;
  random_source_init(&prng, RANDOM_SOURCE_PRNG);

  /* Generate seeds for secret key and public key */
  random_source_get_bytes(&prng, sk_seed, SEEDEXPANDER_SEED_BYTES);
  random_source_get_bytes(&prng, pk_seed, SEEDEXPANDER_SEED_BYTES);
  random_source_seed_with_len(&sk_seedexpander, sk_seed, SEEDEXPANDER_SEED_BYTES);
  random_source_seed_with_len(&pk_seedexpander, pk_seed, SEEDEXPANDER_SEED_BYTES);

  /* Generate secret key (X, Y) */
  rbc_vspace_set_random_full_rank_with_one(&sk_seedexpander, support_1, CMULTIURAG_PARAM_W1);
  
  rbc_mat_set_random_from_support(&sk_seedexpander, X, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1, support_1, CMULTIURAG_PARAM_W1, 1);
  rbc_mat_set_random_from_support(&sk_seedexpander, Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1, support_1, CMULTIURAG_PARAM_W1, 1);

  /* Generate public key vectors g (full rank) and matrix H (random) */
  rbc_vec_set_random_full_rank(&pk_seedexpander, g, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_mat_set_random(&pk_seedexpander, H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N); 
  
  // Compute matrix S = X + H.Y
  rbc_mat_mul(S, H, Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_add(S, S, X, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);

  // Parse keys to string
  cmultiurag_public_key_to_string(pk, S, pk_seed);
  cmultiurag_secret_key_to_string(sk, sk_seed, pk);

  #ifdef VERBOSE
    printf("\n\nsk_seed: "); for(int i = 0 ; i < SEEDEXPANDER_SEED_BYTES ; ++i) printf("%02x", sk_seed[i]);
    printf("\n\npk_seed: "); for(int i = 0 ; i < SEEDEXPANDER_SEED_BYTES ; ++i) printf("%02x", pk_seed[i]);
    printf("\n\nsupport_1: "); rbc_vspace_print(support_1, CMULTIURAG_PARAM_W1);
    printf("\n\nX: "); rbc_mat_print(X, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
    printf("\n\nY: "); rbc_mat_print(Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
    printf("\n\ng: "); rbc_vec_print(g, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
    printf("\n\nH: "); rbc_mat_print(H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N);
    printf("\n\nS: "); rbc_mat_print(S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
    printf("\n\nsk: "); for(int i = 0 ; i < CMULTIURAG_SECRET_KEY_BYTES ; ++i) printf("%02x", sk[i]);
    printf("\n\npk: "); for(int i = 0 ; i < CMULTIURAG_PUBLIC_KEY_BYTES ; ++i) printf("%02x", pk[i]);
  #endif

  rbc_vspace_clear(support_1);
  rbc_mat_clear(X);
  rbc_mat_clear(Y);
  rbc_vec_clear(g);
  rbc_mat_clear(H);
  rbc_mat_clear(S);
  random_source_clear(&sk_seedexpander);
  random_source_clear(&pk_seedexpander);
}
 
 

/** 
* \fn void cmultiurag_pke_encrypt(rbc_mat u, rbc_mat v, const rbc_vec m, uint8_t* theta, const uint8_t* pk)
* \brief PKE encryption (PKE.Enc)
*
* Corresponds to Figure 2, lines 10-14:
*   1. Recover public key (H, S) and EG code generator g from pk
*   2. Sample error matrices R1, R2, E with small rank weights
*   3. Compute U = R1 + H^T * R2
*   4. Compute V = S^T * R2 + E + Fold(mG)
*
* \param[out] U Matrix U (first part of the ciphertext)
* \param[out] V Matrix V (second part of the ciphertext)
* \param[in] m Plaintext vector m to encrypt
* \param[in] theta Seed used to derive randomness required for encryption
* \param[in] pk Public key byte string
*/
void cmultiurag_pke_encrypt(rbc_mat U, rbc_mat V, const rbc_vec m, uint8_t* theta, const uint8_t* pk) {
  random_source seedexpander;
  random_source_init(&seedexpander, RANDOM_SOURCE_SEEDEXP);

  rbc_vec g, mG;
  rbc_mat H, S, H_T, S_T;
  rbc_vspace support_2;
  rbc_mat R1, R2, E;
  rbc_mat tmp;
  rbc_augmented_gabidulin code;

  rbc_vec_init(&g, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_vec_init(&mG, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_mat_init(&H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N);
  rbc_mat_init(&H_T, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N);
  rbc_mat_init(&S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_init(&S_T, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N);

  rbc_vspace_init(&support_2, CMULTIURAG_PARAM_W2);

  rbc_mat_init(&R1, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
  rbc_mat_init(&R2, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
  rbc_mat_init(&E, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
  rbc_mat_init(&tmp, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);

  /* Initialize seed expander with theta (deterministic random) */
  random_source_seed_with_len(&seedexpander, theta, SHA512_BYTES);

  /* Recover g, H, S     from public key byte string */
  cmultiurag_public_key_from_string(g, H, S, pk);

  /* Generate [R1; E; R2] from S_{w2}: the encryption support has rank w2 and
   * does not require 1 to belong to the support. */
  rbc_vspace_set_random_full_rank(&seedexpander, support_2, CMULTIURAG_PARAM_W2);
  rbc_mat_set_random_from_support(&seedexpander, E, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2, support_2, CMULTIURAG_PARAM_W2, 1);
  rbc_mat_set_random_from_support(&seedexpander, R1, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2, support_2, CMULTIURAG_PARAM_W2, 1);
  rbc_mat_set_random_from_support(&seedexpander, R2, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2, support_2, CMULTIURAG_PARAM_W2, 1);

  // Compute U = R1 + Trans(H) · R2
  rbc_mat_trans(H_T, H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N);
  rbc_mat_mul(U, H_T, R2, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
  rbc_mat_add(U, U, R1, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);

  // Compute V = Fold(m.G) by encoding the message
  rbc_augmented_gabidulin_init(&code, g, CMULTIURAG_PARAM_K, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_augmented_gabidulin_encode(mG, code, m);
  rbc_mat_fold(V, mG, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);

  // Compute V = Fold(mG) + E + Trans(S) · R2
  rbc_mat_trans(S_T, S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_mul(tmp, S_T, R2, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
  rbc_mat_add(tmp, tmp, E, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
  rbc_mat_add(V, V, tmp, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);

  #ifdef VERBOSE
    printf("\n\ng: "); rbc_vec_print(g, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
    printf("\n\nH: "); rbc_mat_print(H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N);
    printf("\n\nS: "); rbc_mat_print(S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
    printf("\n\nsupport_2: "); rbc_vspace_print(support_2, CMULTIURAG_PARAM_W2);
    printf("\n\nR1: "); rbc_mat_print(R1, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
    printf("\n\nR2: "); rbc_mat_print(R2, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
    printf("\n\nE: "); rbc_mat_print(E, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
    printf("\n\nU: "); rbc_mat_print(U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
    printf("\n\nV: "); rbc_mat_print(V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
  #endif

  rbc_vec_clear(g);
  rbc_vec_clear(mG);
  rbc_mat_clear(H);
  rbc_mat_clear(H_T);
  rbc_mat_clear(S);
  rbc_mat_clear(S_T);
  rbc_vspace_clear(support_2);
  rbc_mat_clear(R1);
  rbc_mat_clear(R2);
  rbc_mat_clear(E);
  rbc_mat_clear(tmp);
  random_source_clear(&seedexpander);
}



/** 
* \fn void cmultiurag_pke_decrypt(rbc_vec m, const rbc_mat U, const rbc_mat V, const uint8_t* sk)
* \brief PKE decryption (PKE.Dec)

Corresponds to Figure 2, lines 15-16:
 *   1. Compute V - Y^T · U
 *      V - Y^T U = Fold(mG) + S^T · R2 + E - Y^T · (R1 + H^T · R2)
 *                = mG + (X + HY)^T · R2 + E - Y^T · R1 - Y^T · H^T · R2
 *                = mG + X^T · R2 + E - Y^T · R1
 *   2. UnFold(V - Y^T U) = mG + UnFold(X^T R2 - Y^T R1 + E)
 *      Since wt_R(UnFold(X^T R2 - Y^T R1 + E)) ≤ w1 * w2, the EG code can
 *      correct the error
 *   3. m = EG.Decode(UnFold(V - Y^T U)), recover plaintext using EG decoding
*
* \param[out] m Recovered plaintext vector m
* \param[in] U Matrix U (first part of the ciphertext)
* \param[in] V Matrix V (second part of the ciphertext)
* \param[in] sk Secret key byte string
*/
void cmultiurag_pke_decrypt(rbc_vec m, const rbc_mat U, const rbc_mat V, const uint8_t* sk) {
  uint8_t pk[CMULTIURAG_PUBLIC_KEY_BYTES] = {0};
  rbc_mat X, Y, Y_T, S, H;
  rbc_vec g, mG;
  rbc_mat tmp;
  rbc_augmented_gabidulin code;

  rbc_mat_init(&X, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_init(&Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_init(&Y_T, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N);
  rbc_vec_init(&g, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_vec_init(&mG, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_mat_init(&H, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N);
  rbc_mat_init(&S, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_init(&tmp, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);

  // Recover X, Y, g, H and S from secret key
  cmultiurag_secret_key_from_string(X, Y, pk, sk);

  // Recover g, H and S from public key
  cmultiurag_public_key_from_string(g, H, S, pk);

  // Compute UnFold(V- Trans(Y) · U)
  rbc_mat_trans(Y_T, Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
  rbc_mat_mul(tmp, Y_T, U, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
  rbc_mat_add(tmp, V, tmp, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
  rbc_mat_unfold(mG, tmp, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);

  // Compute m by decoding UnFold(V- Trans(Y) · U)
  rbc_augmented_gabidulin_init(&code, g, CMULTIURAG_PARAM_K, CMULTIURAG_PARAM_N1 * CMULTIURAG_PARAM_N2);
  rbc_augmented_gabidulin_decode(m, code, mG);

  #ifdef VERBOSE
    printf("\n\nU: "); rbc_mat_print(U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2);
    printf("\n\nV: "); rbc_mat_print(V, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
    printf("\n\nY: "); rbc_mat_print(Y, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N1);
    printf("\n\nV- Trans(Y) · U: "); rbc_mat_print(tmp, CMULTIURAG_PARAM_N1, CMULTIURAG_PARAM_N2);
  #endif

  rbc_mat_clear(X);
  rbc_mat_clear(Y);
  rbc_mat_clear(Y_T);
  rbc_vec_clear(g);
  rbc_vec_clear(mG);
  rbc_mat_clear(H);
  rbc_mat_clear(S);
  rbc_mat_clear(tmp);
}
