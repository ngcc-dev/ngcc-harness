#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "indcpa.h"
#include "polyvec.h"
#include "poly.h"
#include "ntt.h"
#include "symmetric.h"

/*************************************************
* Name:        pack_pk
*
* Description: Serialize the public key as concatenation of the
*              serialized vector of polynomials pk
*              and the public seed used to generate the matrix A.
*
* Arguments:   uint8_t *r: pointer to the output serialized public key
*              polyvec *pk: pointer to the input public-key polyvec
*              const uint8_t *seed: pointer to the input public seed
**************************************************/
static void pack_pk(uint8_t r[COMPASS_KEM_INDCPA_PUBLICKEYBYTES],
                    polyvec *pk,
                    const uint8_t seed[COMPASS_KEM_SYMBYTES])
{
  polyvec_compress(r, pk, COMPASS_KEM_D); 
  memcpy(r+COMPASS_KEM_POLYVECCOMPRESSEDBYTES, seed, COMPASS_KEM_SYMBYTES);
}

/*************************************************
* Name:        unpack_pk
*
* Description: De-serialize public key from a byte array;
*              approximate inverse of pack_pk
*
* Arguments:   - polyvec *pk: pointer to output public-key polynomial vector
*              - uint8_t *seed: pointer to output seed to generate matrix A
*              - const uint8_t *packedpk: pointer to input serialized public key
**************************************************/
static void unpack_pk(polyvec *pk,
                      uint8_t seed[COMPASS_KEM_SYMBYTES],
                      const uint8_t packedpk[COMPASS_KEM_INDCPA_PUBLICKEYBYTES])
{
  polyvec_decompress(pk, packedpk, COMPASS_KEM_D);
  memcpy(seed, packedpk+COMPASS_KEM_POLYVECCOMPRESSEDBYTES, COMPASS_KEM_SYMBYTES);
}
/*************************************************
* Name:        pack_sk
*
* Description: Serialize the secret key
*
* Arguments:   - uint8_t *r: pointer to output serialized secret key
*              - polyvec *sk: pointer to input vector of polynomials (secret key)
**************************************************/
static void pack_sk(uint8_t r[COMPASS_KEM_INDCPA_SECRETKEYBYTES], polyvec *sk)
{
  polyvec_tobytes(r, sk);
}

/*************************************************
* Name:        unpack_sk
*
* Description: De-serialize the secret key; inverse of pack_sk
*
* Arguments:   - polyvec *sk: pointer to output vector of polynomials (secret key)
*              - const uint8_t *packedsk: pointer to input serialized secret key
**************************************************/
static void unpack_sk(polyvec *sk, const uint8_t packedsk[COMPASS_KEM_INDCPA_SECRETKEYBYTES])
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
*              poly *pk: pointer to the input vector of polynomials b
*              poly *v: pointer to the input polynomial v
**************************************************/
static void pack_ciphertext(uint8_t r[COMPASS_KEM_INDCPA_BYTES], polyvec *b, poly *v)
{
  polyvec_compress(r, b, COMPASS_KEM_D1);
  poly_compress(r+COMPASS_KEM_POLYVECCOMPRESSEDBYTES, v, COMPASS_KEM_D2);
}

/*************************************************
* Name:        unpack_ciphertext
*
* Description: De-serialize and decompress ciphertext from a byte array;
*              approximate inverse of pack_ciphertext
*
* Arguments:   - polyvec *b: pointer to the output vector of polynomials b
*              - poly *v: pointer to the output polynomial v
*              - const uint8_t *c: pointer to the input serialized ciphertext
**************************************************/
static void unpack_ciphertext(polyvec *b, poly *v, const uint8_t c[COMPASS_KEM_INDCPA_BYTES])
{
  polyvec_decompress(b, c, COMPASS_KEM_D1);
  poly_decompress(v, c+COMPASS_KEM_POLYVECCOMPRESSEDBYTES, COMPASS_KEM_D2);
}

/*************************************************
* Name:        rej_uniform
*
* Description: Run rejection sampling on uniform random bytes to generate
*              uniform random integers mod q
*
* Arguments:   - int16_t *r: pointer to output buffer
*              - unsigned int len: requested number of 16-bit integers (uniform mod q)
*              - const uint8_t *buf: pointer to input buffer (assumed to be uniformly random bytes)
*              - unsigned int buflen: length of input buffer in bytes
*
* Returns number of sampled 16-bit integers (at most len)
**************************************************/
static unsigned int rej_uniform(int16_t *r,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen)
{
  unsigned int ctr = 0;
  unsigned int pos = 0;

#if (COMPASS_KEM_Q == 3329)
  // --- 128/256-bit security (q=3329): 12-bit extraction (2 values per 3 bytes) ---
  uint16_t val0, val1;
  while(ctr < len && pos + 3 <= buflen) {
    val0 = ((buf[pos+0] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0xFFF;
    val1 = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2] << 4)) & 0xFFF;
    pos += 3;

    if(val0 < COMPASS_KEM_Q)
      r[ctr++] = val0;
    if(ctr < len && val1 < COMPASS_KEM_Q)
      r[ctr++] = val1;
  }

#elif (COMPASS_KEM_Q == 7681)
  // --- 384/512-bit security (q=7681): 13-bit zero-waste extraction (sliding window) ---
  uint32_t bit_buf = 0;        // Bit reservoir
  unsigned int bit_pos = 0;    // Number of valid bits in reservoir

  while(ctr < len) {
    // 1. Fill: while reservoir has < 13 bits and buffer has data, feed one byte at a time
    while (bit_pos < 13 && pos < buflen) {
      bit_buf |= ((uint32_t)buf[pos++]) << bit_pos;
      bit_pos += 8;
    }

    // 2. If buffer exhausted and reservoir still lacks 13 bits, exit
    if (bit_pos < 13) {
      break;
    }

    // 3. Drain: extract 13 bits from bottom of reservoir
    uint16_t val = bit_buf & 0x1FFF;
    
    // 4. Consume: drop water level by 13 bits
    bit_buf >>= 13;
    bit_pos -= 13;

    // 5. Rejection sampling check (valid range 0 ~ 7680)
    if (val < COMPASS_KEM_Q) {
      r[ctr++] = val;
    }
  }
#else
  #error "Unsupported COMPASS_KEM_Q in rej_uniform"
#endif

  return ctr;
}

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
* Arguments:   - polyvec *a: pointer to ouptput matrix A
*              - const uint8_t *seed: pointer to input seed
*              - int transposed: boolean deciding whether A or A^T is generated
**************************************************/
#if(XOF_BLOCKBYTES % 3)
#error "Implementation of gen_matrix assumes that XOF_BLOCKBYTES is a multiple of 3"
#endif

#if (COMPASS_KEM_Q == 3329)
  // 12 bits per coefficient, sample space 2^12
  #define GEN_MATRIX_NBLOCKS ((12*COMPASS_KEM_N/8*(1 << 12)/COMPASS_KEM_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)

#elif (COMPASS_KEM_Q == 7681)
  // 13 bits per coefficient, sample space 2^13 (8192)
  #define GEN_MATRIX_NBLOCKS ((13*COMPASS_KEM_N/8*(1 << 13)/COMPASS_KEM_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)

#endif
// Not static for benchmarking
void gen_matrix(polyvec *a, const uint8_t seed[COMPASS_KEM_SYMBYTES], int transposed)
{
  unsigned int ctr, i, j;
  unsigned int buflen;
  uint8_t buf[GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES];
  xof_state state;

  for(i=0;i<COMPASS_KEM_K;i++) {
    for(j=0;j<COMPASS_KEM_K;j++) {
      if(transposed)
        xof_absorb(&state, seed, i, j);
      else
        xof_absorb(&state, seed, j, i);

      xof_squeezeblocks(buf, GEN_MATRIX_NBLOCKS, &state);
      buflen = GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES;
      ctr = rej_uniform(a[i].vec[j].coeffs, COMPASS_KEM_N, buf, buflen);

      while(ctr < COMPASS_KEM_N) {
        xof_squeezeblocks(buf, 1, &state);
        buflen = XOF_BLOCKBYTES;
        ctr += rej_uniform(a[i].vec[j].coeffs + ctr, COMPASS_KEM_N - ctr, buf, buflen);
      }
    }
  }
}

/*************************************************
* Name:        indcpa_keypair_derand
*
* Description: Generates public and private key for the CPA-secure
*              public-key encryption scheme underlying COMPASS_KEM
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                             (of length COMPASS_KEM_INDCPA_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
*                             (of length COMPASS_KEM_INDCPA_SECRETKEYBYTES bytes)
*              - const uint8_t *coins: pointer to input randomness
*                             (of length COMPASS_KEM_SYMBYTES bytes)
**************************************************/
void indcpa_keypair_derand(uint8_t pk[COMPASS_KEM_INDCPA_PUBLICKEYBYTES],
                           uint8_t sk[COMPASS_KEM_INDCPA_SECRETKEYBYTES],
                           const uint8_t coins[COMPASS_KEM_SYMBYTES])
{
  unsigned int i;
  uint8_t buf[2*COMPASS_KEM_SYMBYTES];
  const uint8_t *publicseed = buf;
  const uint8_t *noiseseed = buf+COMPASS_KEM_SYMBYTES;
  uint8_t nonce = 0;
  polyvec a[COMPASS_KEM_K], pkpv, skpv;

  memcpy(buf, coins, COMPASS_KEM_SYMBYTES);
  buf[COMPASS_KEM_SYMBYTES] = COMPASS_KEM_K;
  hash_g(buf, buf, COMPASS_KEM_SYMBYTES+1);

  gen_a(a, publicseed);
  
  for(i=0;i<COMPASS_KEM_K;i++)
    poly_getnoise_eta1(&skpv.vec[i], noiseseed, nonce++);
  // for(i=0;i<COMPASS_KEM_K;i++)
  //   poly_getnoise_eta1(&e.vec[i], noiseseed, nonce++);

  polyvec_ntt(&skpv);
  // polyvec_ntt(&e);

  // matrix-vector multiplication
  for(i=0;i<COMPASS_KEM_K;i++) {
    polyvec_basemul_acc_montgomery(&pkpv.vec[i], &a[i], &skpv);
    // poly_tomont(&pkpv.vec[i]);
  }

  // polyvec_add(&pkpv, &pkpv, &e);
  // polyvec_reduce(&pkpv);

  // [Mod]: Compute t' = INTT(A_hat * s_hat)
  polyvec_invntt_tomont(&pkpv);
  polyvec_reduce(&pkpv);

  // Pack secret key sk = (seed, s_hat), where s_hat remains in NTT domain
  pack_sk(sk, &skpv);
  
  // pack_pk applies t = Comp_d(t') compression internally
  pack_pk(pk, &pkpv, publicseed);

  // pack_sk(sk, &skpv);
  // pack_pk(pk, &pkpv, publicseed);
}


/*************************************************
* Name:        indcpa_enc
*
* Description: Encryption function of the CPA-secure
*              public-key encryption scheme underlying COMPASS_KEM.
*
* Arguments:   - uint8_t *c: pointer to output ciphertext
*                            (of length COMPASS_KEM_INDCPA_BYTES bytes)
*              - const uint8_t *m: pointer to input message
*                                  (of length COMPASS_KEM_INDCPA_MSGBYTES bytes)
*              - const uint8_t *pk: pointer to input public key
*                                   (of length COMPASS_KEM_INDCPA_PUBLICKEYBYTES)
*              - const uint8_t *coins: pointer to input random coins used as seed
*                                      (of length COMPASS_KEM_SYMBYTES) to deterministically
*                                      generate all randomness
**************************************************/
void indcpa_enc(uint8_t c[COMPASS_KEM_INDCPA_BYTES],
                const uint8_t m[COMPASS_KEM_INDCPA_MSGBYTES],
                const uint8_t pk[COMPASS_KEM_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[COMPASS_KEM_SYMBYTES])
{
  unsigned int i;
  uint8_t seed[COMPASS_KEM_SYMBYTES];
  uint8_t nonce = 0;
  polyvec sp, pkpv, at[COMPASS_KEM_K], b;
  poly v, k;
  
  // unpack_pk applies t' = Decomp_d(t) decompression internally
  unpack_pk(&pkpv, seed, pk);

  // [Mod]: Decompressed t' is in normal domain; must convert to NTT domain for multiplication
  polyvec_ntt(&pkpv);

  poly_frommsg(&k, m);
  gen_at(at, seed);
  
  // Checkpoint 1: Verify generated matrix A and public key correctness
  // debug_print_polyvec("ENC - Matrix A^T", &at[0]);
  // debug_print_polyvec("ENC - Public Key t'", &pkpv);
  // debug_print_poly("ENC - Message m (encoded)", &k);

  for(i=0;i<COMPASS_KEM_K;i++)
    poly_getnoise_eta1(sp.vec+i, coins, nonce++);
  // for(i=0;i<COMPASS_KEM_K;i++)
  //   poly_getnoise_eta2(ep.vec+i, coins, nonce++);
  // poly_getnoise_eta2(&epp, coins, nonce++);

  polyvec_ntt(&sp);

  // matrix-vector multiplication
  for(i=0;i<COMPASS_KEM_K;i++)
    polyvec_basemul_acc_montgomery(&b.vec[i], &at[i], &sp);

  polyvec_basemul_acc_montgomery(&v, &pkpv, &sp);

  polyvec_invntt_tomont(&b);
  poly_invntt_tomont(&v);

  // polyvec_add(&b, &b, &ep);
  // poly_add(&v, &v, &epp);
  poly_add(&v, &v, &k);
  polyvec_reduce(&b);
  poly_reduce(&v);
  
  // Checkpoint 2: Verify ciphertext state before compression (critical!)
  // debug_print_polyvec("ENC - Ciphertext c1 (before compress)", &b);
  // debug_print_poly("ENC - Ciphertext c2 (before compress)", &v);

  // pack_ciphertext applies Comp_{d1} and Comp_{d2} compression internally
  pack_ciphertext(c, &b, &v);
}

/*************************************************
* Name:        indcpa_dec
*
* Description: Decryption function of the CPA-secure
*              public-key encryption scheme underlying COMPASS_KEM.
*
* Arguments:   - uint8_t *m: pointer to output decrypted message
*                            (of length COMPASS_KEM_INDCPA_MSGBYTES)
*              - const uint8_t *c: pointer to input ciphertext
*                                  (of length COMPASS_KEM_INDCPA_BYTES)
*              - const uint8_t *sk: pointer to input secret key
*                                   (of length COMPASS_KEM_INDCPA_SECRETKEYBYTES)
**************************************************/
void indcpa_dec(uint8_t m[COMPASS_KEM_INDCPA_MSGBYTES],
                const uint8_t c[COMPASS_KEM_INDCPA_BYTES],
                const uint8_t sk[COMPASS_KEM_INDCPA_SECRETKEYBYTES])
{
  polyvec b, skpv;
  poly v, mp;

  unpack_ciphertext(&b, &v, c);
  unpack_sk(&skpv, sk);

  // Checkpoint 3: Verify decompressed ciphertext and secret key
  // debug_print_polyvec("DEC - Secret Key s^", &skpv);
  // debug_print_polyvec("DEC - Ciphertext c1 (after decompress)", &b);
  // debug_print_poly("DEC - Ciphertext c2 (after decompress)", &v);


  polyvec_ntt(&b);
  polyvec_basemul_acc_montgomery(&mp, &skpv, &b);
  poly_invntt_tomont(&mp);

  poly_sub(&mp, &v, &mp);
  poly_reduce(&mp);
  
  // // Checkpoint 4: Verify recovered noisy plaintext (before threshold decision)
  // debug_print_poly("DEC - Recovered m (before threshold)", &mp);

  // Threshold decision delegated to poly_tomsg(m, &mp) at end of decryption:
  // In poly.c, modify the threshold condition: if (q-1)/4 <= u_i < 3(q-1)/4,
  // Set m_i = 1; otherwise m_i = 0. No changes needed in indcpa.c callers.
  poly_tomsg(m, &mp);
}
