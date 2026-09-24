#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "indcpa.h"
#include "polyvec.h"
#include "poly.h"
#include "ntt.h"
#include "symmetric.h"
#include <immintrin.h>
#include "cbd.h"
#include "rejsample.h"


// #include <stdio.h>
// Calculate SHAKE256 blocks needed for noise generation
// poly_cbd_eta1 needs (ETA1 * N / 4) bytes
// SHAKE256 rate = 136 bytes, compute number of blocks needed (rounded up)
#define PRF_NBLOCKS (((COMPASS_KEM_ETA1 * COMPASS_KEM_N / 4) + 135) / 136)

static void polyvec_getnoise_eta1_avx(polyvec *r, const uint8_t seed[COMPASS_KEM_SYMBYTES], uint8_t *nonce) {
#if COMPASS_KEM_K == 2
    uint8_t buf0[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    uint8_t buf1[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    prf(buf0, sizeof(buf0), seed, (*nonce)++);
    prf(buf1, sizeof(buf1), seed, (*nonce)++);
    poly_cbd_eta1(&r->vec[0], buf0);
    poly_cbd_eta1(&r->vec[1], buf1);
#elif COMPASS_KEM_K == 3
    uint8_t buf0[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    uint8_t buf1[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    uint8_t buf2[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    prf(buf0, sizeof(buf0), seed, (*nonce)++);
    prf(buf1, sizeof(buf1), seed, (*nonce)++);
    prf(buf2, sizeof(buf2), seed, (*nonce)++);
    poly_cbd_eta1(&r->vec[0], buf0);
    poly_cbd_eta1(&r->vec[1], buf1);
    poly_cbd_eta1(&r->vec[2], buf2);
#elif COMPASS_KEM_K == 4
    uint8_t buf0[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    uint8_t buf1[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    uint8_t buf2[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    uint8_t buf3[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
    prf(buf0, sizeof(buf0), seed, (*nonce)++);
    prf(buf1, sizeof(buf1), seed, (*nonce)++);
    prf(buf2, sizeof(buf2), seed, (*nonce)++);
    prf(buf3, sizeof(buf3), seed, (*nonce)++);
    poly_cbd_eta1(&r->vec[0], buf0);
    poly_cbd_eta1(&r->vec[1], buf1);
    poly_cbd_eta1(&r->vec[2], buf2);
    poly_cbd_eta1(&r->vec[3], buf3);
#endif
}

__attribute__((unused))
static unsigned int rej_uniform(int16_t *r,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen);

#ifndef REJ_UNIFORM_AVX_NBLOCKS
  #if (COMPASS_KEM_Q == 3329)
    #define REJ_UNIFORM_AVX_NBLOCKS ((12*COMPASS_KEM_N/8*(1 << 12)/COMPASS_KEM_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)
  #elif (COMPASS_KEM_Q == 7681)
    #define REJ_UNIFORM_AVX_NBLOCKS ((13*COMPASS_KEM_N/8*(1 << 13)/COMPASS_KEM_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)
  #endif
#endif

#define GEN_MATRIX_BUF_BYTES (REJ_UNIFORM_AVX_NBLOCKS * XOF_BLOCKBYTES)

// [Fix]: Add 16-byte tail safety buffer to prevent out-of-bounds access
typedef union {
  uint8_t coeffs[GEN_MATRIX_BUF_BYTES + 16]; 
  __m256i vec[(GEN_MATRIX_BUF_BYTES + 31) / 32 + 1];
} __attribute__((aligned(32))) aligned_xof_buf;

// #define REJ_UNIFORM_AVX_NBLOCKS ((13*COMPASS_KEM_N/8*(1 << 13)/COMPASS_KEM_Q + XOF_BLOCKBYTES)/XOF_BLOCKBYTES)
// #define GEN_MATRIX_BUF_BYTES (REJ_UNIFORM_AVX_NBLOCKS * XOF_BLOCKBYTES)

// typedef union {
//   uint8_t coeffs[GEN_MATRIX_BUF_BYTES];
//   __m256i vec[(GEN_MATRIX_BUF_BYTES + 31) / 32];
// } aligned_xof_buf;

static inline unsigned int rej_uniform_13bit_avx(int16_t *r, unsigned int len, const uint8_t *buf, unsigned int buflen) {
    unsigned int ctr = 0;
    unsigned int pos = 0;

    const __m256i shuf_mask = _mm256_set_epi8(
        -1, -1, 12, 11,  -1, 11, 10,  9,  -1, -1,  9,  8,  -1,  8,  7,  6, 
        -1,  6,  5,  4,  -1, -1,  4,  3,  -1,  3,  2,  1,  -1, -1,  1,  0  
    );
    const __m256i shift_vec = _mm256_set_epi32(3, 6, 1, 4, 7, 2, 5, 0);
    const __m256i mask_13   = _mm256_set1_epi32(0x1FFF);

    int16_t temp[16] __attribute__((aligned(32)));

    // Replaced all COMPASS_KEM_N with len
    while (pos + 26 <= buflen && ctr < len) {
        __m128i inA = _mm_loadu_si128((const __m128i*)(buf + pos));
        __m128i inB = _mm_loadu_si128((const __m128i*)(buf + pos + 13));

        __m256i ymmA = _mm256_set_m128i(inA, inA);
        __m256i ymmB = _mm256_set_m128i(inB, inB);

        ymmA = _mm256_shuffle_epi8(ymmA, shuf_mask);
        ymmB = _mm256_shuffle_epi8(ymmB, shuf_mask);

        ymmA = _mm256_srlv_epi32(ymmA, shift_vec);
        ymmB = _mm256_srlv_epi32(ymmB, shift_vec);
        ymmA = _mm256_and_si256(ymmA, mask_13);
        ymmB = _mm256_and_si256(ymmB, mask_13);

        __m256i packed = _mm256_packus_epi32(ymmA, ymmB);
        __m256i permuted = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));

        _mm256_store_si256((__m256i*)temp, permuted);
        
        for(int i = 0; i < 16 && ctr < len; i++) {
            if (temp[i] < COMPASS_KEM_Q) {
                r[ctr++] = temp[i];
            }
        }
        pos += 26;
    }

    uint32_t bit_buf = 0;
    unsigned int bit_pos = 0;
    while(ctr < len && pos < buflen) {
        while (bit_pos < 13 && pos < buflen) {
            bit_buf |= ((uint32_t)buf[pos++]) << bit_pos;
            bit_pos += 8;
        }
        if (bit_pos < 13) break;
        uint16_t val = bit_buf & 0x1FFF;
        bit_buf >>= 13;
        bit_pos -= 13;
        if (val < COMPASS_KEM_Q) {
            r[ctr++] = val;
        }
    }
    return ctr;
}

static inline unsigned int rej_uniform_auto_avx(int16_t *r, unsigned int len, const uint8_t *buf, unsigned int buflen) {
#if (COMPASS_KEM_Q == 3329)
    return rej_uniform_avx(r, len, buf, buflen);
#elif (COMPASS_KEM_Q == 7681)
    return rej_uniform_13bit_avx(r, len, buf, buflen);
#endif
}

// #include <stdio.h>

// // Print first 8 coefficients of a single polynomial
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
// void gen_matrix(polyvec *a, const uint8_t seed[COMPASS_KEM_SYMBYTES], int transposed)
// {
//   unsigned int ctr, i, j;
//   unsigned int buflen;
//   uint8_t buf[GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES];
//   xof_state state;

//   for(i=0;i<COMPASS_KEM_K;i++) {
//     for(j=0;j<COMPASS_KEM_K;j++) {
//       if(transposed)
//         xof_absorb(&state, seed, i, j);
//       else
//         xof_absorb(&state, seed, j, i);

//       xof_squeezeblocks(buf, GEN_MATRIX_NBLOCKS, &state);
//       buflen = GEN_MATRIX_NBLOCKS*XOF_BLOCKBYTES;
//       ctr = rej_uniform(a[i].vec[j].coeffs, COMPASS_KEM_N, buf, buflen);

//       while(ctr < COMPASS_KEM_N) {
//         xof_squeezeblocks(buf, 1, &state);
//         buflen = XOF_BLOCKBYTES;
//         ctr += rej_uniform(a[i].vec[j].coeffs + ctr, COMPASS_KEM_N - ctr, buf, buflen);
//       }
//     }
//   }
// }

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

// static void debug_print_poly(const char *label, const poly *p) {
//     printf("[DEBUG] %s: ", label);
//     for(int i = 0; i < 8; i++) {
//         printf("%6d ", p->coeffs[i]);
//     }
//     printf("... %6d %6d\n", p->coeffs[COMPASS_KEM_N-2], p->coeffs[COMPASS_KEM_N-1]);
// }

// // Helper: print polynomial vector
// static void debug_print_polyvec(const char *label, const polyvec *pv) {
//     for(int i = 0; i < COMPASS_KEM_K; i++) {
//         char buf[64];
//         sprintf(buf, "%s (vec[%d])", label, i);
//         debug_print_poly(buf, &pv->vec[i]);
//     }
// }
// /*************************************************
// * Name:        indcpa_keypair_derand
// *
// * Description: Generates public and private key for the CPA-secure
// *              public-key encryption scheme underlying COMPASS_KEM
// *
// * Arguments:   - uint8_t *pk: pointer to output public key
// *                             (of length COMPASS_KEM_INDCPA_PUBLICKEYBYTES bytes)
// *              - uint8_t *sk: pointer to output private key
// *                             (of length COMPASS_KEM_INDCPA_SECRETKEYBYTES bytes)
// *              - const uint8_t *coins: pointer to input randomness
// *                             (of length COMPASS_KEM_SYMBYTES bytes)
// **************************************************/
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
  

  // for(i=0;i<COMPASS_KEM_K;i++)
  //   poly_getnoise_eta1(&skpv.vec[i], noiseseed, nonce++);
  // for(i=0;i<COMPASS_KEM_K;i++)
  //   poly_getnoise_eta1(&e.vec[i], noiseseed, nonce++);

  polyvec_getnoise_eta1_avx(&skpv, noiseseed, &nonce);

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
  polyvec_reduce(&skpv);
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

  // for(i=0;i<COMPASS_KEM_K;i++)
  //   poly_getnoise_eta1(sp.vec+i, coins, nonce++);
  // for(i=0;i<COMPASS_KEM_K;i++)
  //   poly_getnoise_eta2(ep.vec+i, coins, nonce++);
  // poly_getnoise_eta2(&epp, coins, nonce++);

  polyvec_getnoise_eta1_avx(&sp, coins, &nonce);

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

