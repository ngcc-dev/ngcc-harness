#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "indcpa.h"
#include "polyvec.h"
#include "poly.h"
#include "msgenc.h"
#include "ntt.h"
#include "symmetric.h"

#ifdef PK_COMPRESS
#include "invq.h"
#if !defined(NO_INV_Q_LIFTING)
#define INV_Q_LIFTING
#endif
#endif

/*************************************************
* Name:        pack_pk
*
* Description: Serialize the public key as concatenation of the
*              serialized vector of polynomials pk
*              and the public seed used to generate the matrix A.
*
* Arguments:   uint8_t *r:          pointer to the output serialized public key
*              polyvec *pk:         pointer to the input public-key polyvec
*              const uint8_t *seed: pointer to the input public seed
**************************************************/
static void pack_pk(uint8_t r[WEAVER_INDCPA_PUBLICKEYBYTES],
                    polyvec *pk,
                    const uint8_t seed[WEAVER_SYMBYTES])
{
#ifdef PK_COMPRESS
  polyvec_compress_pk(r, pk);
  memcpy(r+ WEAVER_PK_POLYVECBYTES, seed, WEAVER_SYMBYTES);
#else
  polyvec_tobytes(r, pk);
  memcpy(r+WEAVER_POLYVECBYTES, seed, WEAVER_SYMBYTES);
#endif
}

/*************************************************
* Name:        unpack_pk
*
* Description: De-serialize public key from a byte array;
*              approximate inverse of pack_pk
*
* Arguments:   - polyvec *pk:             pointer to output public-key
*                                         polynomial vector
*              - uint8_t *seed:           pointer to output seed to generate
*                                         matrix A
*              - const uint8_t *packedpk: pointer to input serialized public key
**************************************************/
void unpack_pk(polyvec *pk,
                      uint8_t seed[WEAVER_SYMBYTES],
                      const uint8_t packedpk[WEAVER_INDCPA_PUBLICKEYBYTES])
{
#ifdef PK_COMPRESS
  polyvec_decompress_pk(pk, packedpk);
  memcpy(seed, packedpk+WEAVER_PK_POLYVECBYTES, WEAVER_SYMBYTES);
#else
  polyvec_frombytes(pk, packedpk);
  memcpy(seed, packedpk+WEAVER_POLYVECBYTES, WEAVER_SYMBYTES);
#endif
}

/*************************************************
* Name:        pack_sk
*
* Description: Serialize the secret key
*
* Arguments:   - uint8_t *r:  pointer to output serialized secret key
*              - polyvec *sk: pointer to input vector of polynomials (secret key)
**************************************************/
static void pack_sk(uint8_t r[WEAVER_INDCPA_SECRETKEYBYTES], polyvec *sk)
{
  polyvec_tobytes(r, sk);
}

/*************************************************
* Name:        unpack_sk
*
* Description: De-serialize the secret key;
*              inverse of pack_sk
*
* Arguments:   - polyvec *sk:             pointer to output vector of
*                                         polynomials (secret key)
*              - const uint8_t *packedsk: pointer to input serialized secret key
**************************************************/
static void unpack_sk(polyvec *sk,
                      const uint8_t packedsk[WEAVER_INDCPA_SECRETKEYBYTES])
{
  polyvec_frombytes(sk, packedsk);
}

/*************************************************
* Name:        pack_ciphertext
*
* Description: Serialize the ciphertext as concatenation of the
*              compressed and serialized vector of polynomials b
*              and the compressed and serialized polynomial v
*
* Arguments:   uint8_t *r: pointer to the output serialized ciphertext
*              poly *pk:   pointer to the input vector of polynomials b
*              poly *v:    pointer to the input polynomial v
**************************************************/
void pack_ciphertext(uint8_t r[WEAVER_INDCPA_BYTES],
                            polyvec *b,
                            poly *v)
{
  polyvec_compress(r, b);
  poly_compress(r+WEAVER_POLYVECCOMPRESSEDBYTES, v);
}

/*************************************************
* Name:        unpack_ciphertext
*
* Description: De-serialize and decompress ciphertext from a byte array;
*              approximate inverse of pack_ciphertext
*
* Arguments:   - polyvec *b:       pointer to the output vector of polynomials b
*              - poly *v:          pointer to the output polynomial v
*              - const uint8_t *c: pointer to the input serialized ciphertext
**************************************************/
static void unpack_ciphertext(polyvec *b,
                              poly *v,
                              const uint8_t c[WEAVER_INDCPA_BYTES])
{
  polyvec_decompress(b, c);
  poly_decompress(v, c+WEAVER_POLYVECCOMPRESSEDBYTES);
}

/*************************************************
* Name:        rej_uniform
*
* Description: Run rejection sampling on uniform random bytes to generate
*              uniform random integers mod q
*
* Arguments:   - int16_t *r:          pointer to output buffer
*              - unsigned int len:    requested number of 16-bit integers
*                                     (uniform mod q)
*              - const uint8_t *buf:  pointer to input buffer
*                                     (assumed to be uniform random bytes)
*              - unsigned int buflen: length of input buffer in bytes
*
* Returns number of sampled 16-bit integers (at most len)
**************************************************/

#if WEAVER_Q == 3329
#define REJ_UNIFORM_BITS 12
#define REJ_UNIFORM_MASK 0xFFF
#define GEN_MATRIX_NBLOCKS ((REJ_UNIFORM_BITS*WEAVER_N/8*(1 << REJ_UNIFORM_BITS)/WEAVER_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)
#elif WEAVER_Q == 7681
#define LEMIRE_REJ_THRESHOLD ((uint32_t)((1ULL << 16) % WEAVER_Q))
#define GEN_MATRIX_NBLOCKS ((((uint32_t)2 * WEAVER_N * 65536u + (65536u - LEMIRE_REJ_THRESHOLD - 1)) / (65536u - LEMIRE_REJ_THRESHOLD) + XOF_BLOCKBYTES - 1) / XOF_BLOCKBYTES)
#else
#error "Unsupported WEAVER_Q for gen_matrix rejection sampling"
#endif

#if WEAVER_Q == 3329
static unsigned int rej_uniform(int16_t *r,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen)
{
  unsigned int ctr, pos;
  uint16_t val0, val1;

  ctr = pos = 0;
  while(ctr < len && pos + 3 <= buflen) {
    val0 = ((buf[pos+0] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0xFFF;
    val1 = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2] << 4)) & 0xFFF;
    pos += 3;

    if(val0 < WEAVER_Q)
      r[ctr++] = val0;
    if(ctr < len && val1 < WEAVER_Q)
      r[ctr++] = val1;
  }

  return ctr;
}
#elif WEAVER_Q == 7681 && !defined(WEAVER_AVX_GEN_MATRIX7681)
static unsigned int rej_uniform(int16_t *r,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen)
{
  const uint32_t threshold = LEMIRE_REJ_THRESHOLD;
  unsigned int ctr = 0;
  unsigned int pos = 0;

  while (ctr < len && pos + 1 < buflen) {
    uint32_t val = (uint32_t)buf[pos] | ((uint32_t)buf[pos + 1] << 8);
    pos += 2;
    uint32_t prod = val * (uint32_t)WEAVER_Q;
    uint16_t low = (uint16_t)prod;

    if (low < threshold)
      continue;

    r[ctr++] = (int16_t)(prod >> 16);
  }

  return ctr;
}
#endif

#define gen_a(A,B)  gen_matrix(A,B,0)
#define gen_at(A,B) gen_matrix(A,B,1)

/*************************************************
* Name:        gen_matrix
*
* Description: Deterministically generate matrix A (or the transpose of A)
*              from a seed. Entries of the matrix are polynomials that look
*              uniformly random. Performs rejection sampling on output of
*              a XOF
*
* Arguments:   - polyvec *a:          pointer to ouptput matrix A
*              - const uint8_t *seed: pointer to input seed
*              - int transposed:      boolean deciding whether A or A^T
*                                     is generated
**************************************************/
#if (WEAVER_Q == 3329) && (XOF_BLOCKBYTES % 3)
#error "Implementation of gen_matrix for q=3329 assumes XOF_BLOCKBYTES is a multiple of 3"
#endif

#if !defined(WEAVER_AVX_GEN_MATRIX7681_ON) && !defined(WEAVER_AVX_GEN_MATRIX128_ON)
// Not static for benchmarking
void gen_matrix(polyvec *a, const uint8_t seed[WEAVER_SYMBYTES], int transposed)
{
  unsigned int ctr, i, j;
  unsigned int buflen;
  uint8_t buf[GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES];
  xof_state state;

  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_K;j++) {
      if(transposed)
        xof_absorb(&state, seed, i, j);
      else
        xof_absorb(&state, seed, j, i);

      xof_squeezeblocks(buf, GEN_MATRIX_NBLOCKS, &state);
      buflen = GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES;
      ctr = rej_uniform(a[i].vec[j].coeffs, WEAVER_N, buf, buflen);

      while(ctr < WEAVER_N) {
        xof_squeezeblocks(buf, 1, &state);
        buflen = XOF_BLOCKBYTES;
        ctr += rej_uniform(a[i].vec[j].coeffs + ctr, WEAVER_N - ctr, buf, buflen);
      }
    }
  }
}
#endif /* !WEAVER_AVX_GEN_MATRIX7681_ON && !WEAVER_AVX_GEN_MATRIX128_ON */

/*************************************************
* Name:        indcpa_keypair
*
* Description: Generates public and private key for the CPA-secure
*              public-key encryption scheme underlying Kyber
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                             (of length WEAVER_INDCPA_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
                              (of length WEAVER_INDCPA_SECRETKEYBYTES bytes)
**************************************************/
void indcpa_keypair_derand(uint8_t pk[WEAVER_INDCPA_PUBLICKEYBYTES],
                           uint8_t sk[WEAVER_INDCPA_SECRETKEYBYTES],
                           const uint8_t coins[WEAVER_SYMBYTES])
{
  unsigned int i;
  uint8_t buf[2 * WEAVER_SYMBYTES];
  const uint8_t *publicseed = buf;
  const uint8_t *noiseseed = buf + WEAVER_SYMBYTES;
  uint8_t nonce = 0;
  polyvec a[WEAVER_K] = {0}, pkpv = {0}, skpv = {0};

  expand_keypair_seeds(buf, coins, WEAVER_SYMBYTES);

  gen_a(a, publicseed);

  for(i=0;i<WEAVER_K;i++)
    poly_getnoise_eta1(&skpv.vec[i], noiseseed, nonce++);

  polyvec_ntt(&skpv);
  
#ifndef PK_COMPRESS

  // matrix-vector multiplication
  for(i=0;i<WEAVER_K;i++) {
    polyvec_basemul_acc_montgomery(&pkpv.vec[i], &a[i], &skpv);
    poly_tomont(&pkpv.vec[i]);
  }

  polyvec_reduce(&pkpv); // save in NTT domain.

#else
  for (i = 0; i < WEAVER_K; i++) {
      polyvec_basemul_acc_montgomery(&pkpv.vec[i], &a[i], &skpv);
      //poly_tomont(&pkpv.vec[i]);
  }
  polyvec_invntt_tomont(&pkpv);  // from NTT to plain.
  polyvec_reduce(&pkpv);

#endif
  pack_sk(sk, &skpv);
  pack_pk(pk, &pkpv, publicseed);
}

/*************************************************
* Name:        indcpa_enc
*
* Description: Encryption function of the CPA-secure
*              public-key encryption scheme underlying Kyber.
*
* Arguments:   - uint8_t *c:           pointer to output ciphertext
*                                      (of length WEAVER_INDCPA_BYTES bytes)
*              - const uint8_t *m:     pointer to input message
*                                      (of length WEAVER_INDCPA_MSGBYTES bytes)
*              - const uint8_t *pk:    pointer to input public key
*                                      (of length WEAVER_INDCPA_PUBLICKEYBYTES)
*              - const uint8_t *coins: pointer to input random coins
*                                      used as seed (of length WEAVER_SYMBYTES)
*                                      to deterministically generate all
*                                      randomness
**************************************************/
void indcpa_enc(uint8_t c[WEAVER_INDCPA_BYTES],
                const uint8_t m[WEAVER_INDCPA_MSGBYTES],
                const uint8_t pk[WEAVER_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[WEAVER_SYMBYTES])
{
  unsigned int i;
  uint8_t seed[WEAVER_SYMBYTES];
  uint8_t nonce = 0;
  polyvec sp = {0}, pkpv = {0}, at[WEAVER_K] = {0}, b = {0};
  poly v = {0}, k = {0};

#ifdef INV_Q_LIFTING
  /*
   * WEAVER-Inv (Algorithm 2):
   *   1. 从 pk 中提取压缩后的桶编号（不做 Decompress）
   *   2. 用 Inv_q 随机提升到 Z_q（消耗 nonce=0 的 PRF 输出）
   *   3. NTT 变换
   */
  polyvec_fromcompressed_pk(&pkpv, pk);
  memcpy(seed, pk + WEAVER_PK_POLYVECBYTES, WEAVER_SYMBYTES);

  polyvec_invq(&pkpv, coins, nonce++);
  polyvec_ntt(&pkpv);
#else
  unpack_pk(&pkpv, seed, pk);
#ifdef PK_COMPRESS
  polyvec_ntt(&pkpv);
#endif
#endif

  poly_frommsg(&k, m);
  gen_at(at, seed);

#ifdef INV_Q_LIFTING
  for(i=0;i<WEAVER_K;i++)
    poly_getnoise_eta2(sp.vec+i, coins, nonce++);
#else
  for(i=0;i<WEAVER_K;i++)
    poly_getnoise_eta2(sp.vec+i, coins, nonce++);
#endif

  polyvec_ntt(&sp);

  // matrix-vector multiplication
  for(i=0;i<WEAVER_K;i++)
    polyvec_basemul_acc_montgomery(&b.vec[i], &at[i], &sp);

  polyvec_basemul_acc_montgomery(&v, &pkpv, &sp);

  polyvec_invntt_tomont(&b);
  poly_invntt_tomont(&v);

  poly_add(&v, &v, &k);
  polyvec_reduce(&b);
  poly_reduce(&v);

  pack_ciphertext(c, &b, &v);
}

/*************************************************
* Name:        indcpa_dec
*
* Description: Decryption function of the CPA-secure
*              public-key encryption scheme underlying Kyber.
*
* Arguments:   - uint8_t *m:        pointer to output decrypted message
*                                   (of length WEAVER_INDCPA_MSGBYTES)
*              - const uint8_t *c:  pointer to input ciphertext
*                                   (of length WEAVER_INDCPA_BYTES)
*              - const uint8_t *sk: pointer to input secret key
*                                   (of length WEAVER_INDCPA_SECRETKEYBYTES)
**************************************************/
void indcpa_dec(uint8_t m[WEAVER_INDCPA_MSGBYTES],
                const uint8_t c[WEAVER_INDCPA_BYTES],
                const uint8_t sk[WEAVER_INDCPA_SECRETKEYBYTES])
{
  polyvec b = {0}, skpv = {0};
  poly v = {0}, mp = {0};

  unpack_ciphertext(&b, &v, c);
  unpack_sk(&skpv, sk);

  polyvec_ntt(&b);
  polyvec_basemul_acc_montgomery(&mp, &skpv, &b);
  poly_invntt_tomont(&mp);

  poly_sub(&mp, &v, &mp);
  poly_reduce(&mp);

  poly_tomsg(m, &mp);
}
