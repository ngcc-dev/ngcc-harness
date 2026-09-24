#include <string.h>
#include <stdlib.h>
#include "fast_matrix_opt.h"
#include "Fql.h"
#include "linear_algebra.h"
#include "multiply_E.h"
#include "tsuov.h"

static inline void save16_be(uint16_t s, uint8_t dst[2]) {
  dst[0] = (uint8_t)(s >> 8);
  dst[1] = (uint8_t)(s & 0xFF);
}

/* ---------------------------------------------------------
   --------------------------------------------------------- */

typedef struct {
  const uint8_t *src;
  const uint8_t *end;
  uint32_t       bit_buf;
  uint8_t        bit_cnt;
} rejsamp_pack_reader;

static inline void rejsamp_pack_reader_init(
  rejsamp_pack_reader *rd,
  const uint8_t *src,
  unsigned int tau
){
  rd->src     = src ;
  rd->end     = src + tau ;
  rd->bit_buf = 0 ;
  rd->bit_cnt = 0 ;
}

static inline int rejsamp_pack_reader_refill(rejsamp_pack_reader *rd)
{
  if(rd->src >= rd->end) return -1 ;
  rd->bit_buf |= ((uint32_t)*rd->src++) << rd->bit_cnt ;
  rd->bit_cnt += 8 ;
  return 0 ;
}

static inline int rejsamp_pack_reader_next(rejsamp_pack_reader *rd, uint8_t *out)
{
  for(;;){
    while(rd->bit_cnt < 5){
      if(rejsamp_pack_reader_refill(rd) != 0) return -1 ;
    }
    uint8_t v = (uint8_t)(rd->bit_buf & 0x1Fu) ;
    rd->bit_buf >>= 5 ;
    rd->bit_cnt = (uint8_t)(rd->bit_cnt - 5) ;
    if(v != TSUOV_q){
      *out = v ;
      return 0 ;
    }
  }
}

static inline void RejSamp(
  const unsigned int length,
  const unsigned int tau,
        uint8_t *    dst
){
  uint8_t xof[tau] ;
  memcpy(xof, dst, tau) ;
  rejsamp_pack_reader rd ;
  rejsamp_pack_reader_init(&rd, xof, tau) ;
  for(unsigned int i = 0; i < length; i++){
    if(rejsamp_pack_reader_next(&rd, &dst[i]) != 0)
      dst[i] = 0 ;
  }
  memset(xof, 0, tau) ;
}


/* -----------------------------------------------------
  PRG (SM3 counter-mode / pseudoXOF via auxfunc)
   ----------------------------------------------------- */

static void secure_zero(void *ptr, size_t len)
{
  volatile unsigned char *p = (volatile unsigned char *)ptr ;
  while(len--) *p++ = 0 ;
}

static MGF_CTX_s * PRG_init_sm3(const uint8_t seed[TSUOV_SEED_LEN])
{
  MGF_CTX_s * ctx = (MGF_CTX_s *)malloc(sizeof(MGF_CTX)) ;
  MGF_init(seed, TSUOV_SEED_LEN, ctx) ;
  return ctx ;
}

static void PRG_yield_sm3(MGF_CTX_s * ctx, int length, uint8_t * dst)
{
  MGF_yield(ctx, dst, (size_t)length) ;
}

static void PRG_final_sm3(MGF_CTX_s * ctx)
{
  MGF_final(ctx) ;
  free(ctx) ;
}

static MGF_CTX_s * PRG_copy_sm3(MGF_CTX_s * src)
{
  MGF_CTX_s * dst = (MGF_CTX_s *)malloc(sizeof(MGF_CTX)) ;
  MGF_CTX_copy(src, dst) ;
  return dst ;
}

static inline void RejSampPRG_sm3(
  MGF_CTX_s *        ctx,
  const uint64_t     index,
  const unsigned int length,
  const unsigned int tau,
        uint8_t *    dst
){
  uint8_t msg[TSUOV_SEED_LEN + 2] ;
  memcpy(msg, ctx->in, TSUOV_SEED_LEN) ;
  save16_be((uint16_t)index, msg + TSUOV_SEED_LEN) ;
  pseudoXOF((unsigned long long)tau * 8, msg, (unsigned long long)(TSUOV_SEED_LEN + 2) * 8, dst) ;
  RejSamp(length, tau, dst) ;
  secure_zero(msg, sizeof(msg)) ;
}




/* -------------------------------------------
  Expand_mu(), Hash() -> SM3 / auxfunc
   ------------------------------------------- */

static void Expand_mu(
  const TSUOV_SEED seed_pk,         // input
  const uint8_t    message[],       // input
  const size_t     message_length,  // input (message)
  uint8_t          mu[TSUOV_MU_LEN] // output mu[64]
){
  uint8_t * buf = (uint8_t *)malloc(TSUOV_SEED_LEN + message_length) ;
  if(buf == NULL) return ;
  memcpy(buf, seed_pk, TSUOV_SEED_LEN) ;
  memcpy(buf + TSUOV_SEED_LEN, message, message_length) ;
  pseudohash(512, buf, (unsigned long long)(TSUOV_SEED_LEN + message_length) * 8, mu) ;
  free(buf) ;
}

static void Hash(
  const uint8_t    mu[TSUOV_MU_LEN], // input
  const TSUOV_SALT salt,             // input
  uint8_t          dst[TSUOV_m2]      // output
){
  uint8_t tmp[TSUOV_m2 > TSUOV_t_RS_LEN ? TSUOV_m2 : TSUOV_t_RS_LEN] ;
  uint8_t msg[TSUOV_MU_LEN + TSUOV_SALT_LEN] ;
  memcpy(msg, mu, TSUOV_MU_LEN) ;
  memcpy(msg + TSUOV_MU_LEN, salt, TSUOV_SALT_LEN) ;
  pseudoXOF((unsigned long long)TSUOV_t_RS_LEN * 8, msg,
            (unsigned long long)(TSUOV_MU_LEN + TSUOV_SALT_LEN) * 8, tmp) ;
  RejSamp(TSUOV_m2, TSUOV_t_RS_LEN, tmp) ;
  memcpy(dst, tmp, TSUOV_m2) ;
  secure_zero(tmp, sizeof(tmp)) ;
  secure_zero(msg, sizeof(msg)) ;
  return ;
}

/* -------------------------------------------

   ------------------------------------------- */


static void ExpandMatrixOxV(
  uint8_t *  src, // input src[L*O*V]
  MATRIX_OxV   A  // output
) {
  uint8_t * s = src ;
  for(int i=0;i<TSUOV_O;i++){
    for(int j=0;j<TSUOV_V;j++){
      for(int k=0;k<TSUOV_L;k++){
        A[i][k][j] = *s++ ;
      }
    }
  }
  for(int i = 0; i < TSUOV_O; i++) VECTOR_V_CLEAR_TAIL(A[i]) ;
}

static void ExpandMatrixkxV (
  uint8_t *  src, // input src[L*k*V]
  MATRIX_kxV   A  // output
) {
  uint8_t * s = src ;
  for(int i=0;i<TSUOV_k;i++){
    for(int j=0;j<TSUOV_V;j++){
      for(int k=0;k<TSUOV_L;k++){
        A[i][k][j] = *s++ ;
      }
    }
    VECTOR_V_CLEAR_TAIL(A[i]) ;
  }
}

static void ExpandSymmetricMatrixVxV (
  uint8_t *  src, // input src[L*V*(V+1)/2]
  MATRIX_VxV   A  // output
){
  uint8_t * s = src ;
  for(int i=0; i<TSUOV_V; i++){
    for(int j=i; j<TSUOV_V; j++){
      for(int k=0;k<TSUOV_L;k++){
        A[i][k][j] = *s++ ;
        A[j][k][i] = A[i][k][j];        
      }
    }
    VECTOR_V_CLEAR_TAIL(A[i]) ;
  }
}

/* -------------------------------------------

   ------------------------------------------- */


static void Expand_sk(
  const TSUOV_SEED seed_sk,  // input
  MATRIX_OxV SdT             // output
){
  const int n2 = TSUOV_Pi2_LEN ;
  uint8_t   r2[TSUOV_Pi2_LEN] ;
  TSUOV_PRG_CTX * ctx = PRG_init(seed_sk) ;
  RejSampPRG(ctx, 0, n2, TSUOV_Pi2_RS_LEN, r2) ;
  // ExpandMatrixMxV(r2, SdT) ;
  // MATRIX_TRANSPOSE_VxM(Sd, SdT) ;
  uint8_t *ptr_r2 = r2;
  for(int i=0; i<TSUOV_O;i++){
    for(int j=0; j<TSUOV_V;j++){
      for(int k=0;k<TSUOV_L;k++){
        SdT[i][k][j] = *ptr_r2++ ;
      }
    }
    VECTOR_V_CLEAR_TAIL(SdT[i]);
  }
  PRG_final(ctx) ;
  secure_zero(r2, TSUOV_Pi2_LEN) ;
}

static void Expand_S(
  const TSUOV_SEED seed_sk,  // input
  MATRIX_OxN SdT              // output
){
  const int n2 = TSUOV_Pi2_LEN ;
  uint8_t   r2[TSUOV_Pi2_LEN] ALIGN_256BIT ;
  TSUOV_PRG_CTX * ctx = PRG_init(seed_sk) ;
  RejSampPRG(ctx, 0, n2, TSUOV_Pi2_RS_LEN, r2) ;
  PRG_final(ctx) ;
  
  uint8_t *ptr_r2 = r2;
  for(int i=0; i<TSUOV_O;i++){
    for(int j=0; j<TSUOV_V;j++){
      for(int k=0;k<TSUOV_L;k++){
        SdT[i][k][j] = *ptr_r2++ ;
      }
    }
  }
  int N_PADDED = (TSUOV_N + 31) & ~31;
  for(int i = 0; i < TSUOV_O; i++) {
    for(int k = 0; k < TSUOV_L; k++) {
      memset(&SdT[i][k][TSUOV_V], 0, (N_PADDED - TSUOV_V) * sizeof(uint8_t));
    }
  }
  for(int i = 0; i < TSUOV_O; i++){
    SdT[i][0][TSUOV_V+i] = 1; // 对角线元素设置为 1
  }
  secure_zero(r2, TSUOV_Pi2_LEN) ;
}


static void Expand_pk(
  TSUOV_PRG_CTX * ctx0,  // input
  const uint64_t  index, // input
  MATRIX_VxV      Pi1,   // output
  MATRIX_OxV      Pi2T    // output
){
  const int n1 = TSUOV_Pi1_LEN;
  const int n2 = TSUOV_Pi2_LEN ;
  uint8_t r1[TSUOV_Pi1_LEN] ;
  uint8_t r2[TSUOV_Pi2_LEN] ;

  TSUOV_PRG_CTX * ctx1 = PRG_copy(ctx0) ;
  RejSampPRG(ctx1, 2*index, n1, TSUOV_Pi1_RS_LEN, r1) ;
  ExpandSymmetricMatrixVxV(r1, Pi1) ;
  PRG_final(ctx1) ;

  TSUOV_PRG_CTX * ctx2 = PRG_copy(ctx0) ;
  RejSampPRG(ctx2, 2*index+1, n2, TSUOV_Pi2_RS_LEN, r2) ;
  ExpandMatrixOxV(r2, Pi2T) ;
  PRG_final(ctx2) ;
}


static void Expand_PK(
  const Fq P3i [TSUOV_O][TSUOV_L][TSUOV_O],
  TSUOV_PRG_CTX * ctx0, 
  const uint64_t index, 
  MATRIX_NxN Pi
){
  const int n1 = TSUOV_Pi1_LEN;
  const int n2 = TSUOV_Pi2_LEN;
  
  uint8_t r1[TSUOV_Pi1_LEN] ALIGN_256BIT;
  uint8_t r2[TSUOV_Pi2_LEN] ALIGN_256BIT;


  TSUOV_PRG_CTX * ctx1 = PRG_copy(ctx0);
  RejSampPRG(ctx1, 2*index, n1, TSUOV_Pi1_RS_LEN, r1);
  PRG_final(ctx1); // 尽早释放

  TSUOV_PRG_CTX * ctx2 = PRG_copy(ctx0);
  RejSampPRG(ctx2, 2*index+1, n2, TSUOV_Pi2_RS_LEN, r2);
  PRG_final(ctx2);

  
  uint8_t *ptr_r1 = r1;
  uint8_t *ptr_r2 = r2;

  //  fill (V x V) - Vinegar x Vinegar = -(sub_Pi1)
  for(int i = 0; i < TSUOV_V; i++){
    for(int j = i; j < TSUOV_V; j++){
      for(int k = 0; k < TSUOV_L; k++){
        uint8_t val = TSUOV_q - *ptr_r1++;
        Pi[i][k][j] = val;
        Pi[j][k][i] = val; 
      }
    }
  }

  // fill (OxV) 
  // use symmetric property to fill (VxO)
  for(int i = TSUOV_V; i < TSUOV_N; i++){
    for(int j = 0; j < TSUOV_V; j++){
      for(int k = 0; k < TSUOV_L; k++){
        uint8_t val = *ptr_r2++;
        Pi[i][k][j] = val;
        Pi[j][k][i] = val; 
      }
    }
  }

  // 4. fill (O x O) = P3  
  for(int i = TSUOV_V; i < TSUOV_N; i++){
    for(int k = 0; k < TSUOV_L; k++){
        memcpy(&Pi[i][k][TSUOV_V], &P3i[i-TSUOV_V][k][0], TSUOV_O * sizeof(uint8_t));
    }
  }

  int N_PADDED = (TSUOV_N + 31) & ~31;
  if(N_PADDED == TSUOV_N) return;
  for(int i = 0; i < TSUOV_N; i++){
    for(int k = 0; k < TSUOV_L; k++){
      memset(&Pi[i][k][TSUOV_N], 0, (N_PADDED-TSUOV_N) * sizeof(uint8_t));
    }
  }
}

static void Expand_P_MATRIX(
  TSUOV_PRG_CTX * ctx0, 
  const uint64_t index, 
  MATRIX_NxN Pi
){
  const int n1 = TSUOV_Pi1_LEN;
  const int n2 = TSUOV_Pi2_LEN;

  uint8_t r1[TSUOV_Pi1_LEN] ALIGN_256BIT;
  uint8_t r2[TSUOV_Pi2_LEN] ALIGN_256BIT;


  TSUOV_PRG_CTX * ctx1 = PRG_copy(ctx0);
  RejSampPRG(ctx1, 2*index, n1, TSUOV_Pi1_RS_LEN, r1);
  PRG_final(ctx1); 

  TSUOV_PRG_CTX * ctx2 = PRG_copy(ctx0);
  RejSampPRG(ctx2, 2*index+1, n2, TSUOV_Pi2_RS_LEN, r2);
  PRG_final(ctx2);

  uint8_t *ptr_r1 = r1;
  uint8_t *ptr_r2 = r2;

  // fill (V x V) - Vinegar x Vinegar
  for(int i = 0; i < TSUOV_V; i++){
    for(int j = i; j < TSUOV_V; j++){
      for(int k = 0; k < TSUOV_L; k++){
        uint8_t val = *ptr_r1++;
        Pi[i][k][j] = val;
        Pi[j][k][i] = val; 
      }
    }
  }

  // fill (OxV) 
  // use symmetric property to fill (VxO)
  for(int i = TSUOV_V; i < TSUOV_N; i++){
    for(int j = 0; j < TSUOV_V; j++){
      for(int k = 0; k < TSUOV_L; k++){
        uint8_t val = *ptr_r2++;
        Pi[i][k][j] = val;
        Pi[j][k][i] = val; 
      }
    }
  }

  // set (O x O) = 0
  for(int i = TSUOV_V; i < TSUOV_N; i++){
    for(int k = 0; k < TSUOV_L; k++){
        memset(&Pi[i][k][TSUOV_V], 0, (TSUOV_N - TSUOV_V) * sizeof(uint8_t));
    }
  }

  int N_PADDED = (TSUOV_N + 31) & ~31;
  if(N_PADDED == TSUOV_N) return;
  for(int i = 0; i < TSUOV_N; i++){
    for(int k = 0; k < TSUOV_L; k++){
      memset(&Pi[i][k][TSUOV_N], 0, (N_PADDED-TSUOV_N) * sizeof(uint8_t));
    }
  }
}



static void Expand_V (
  const TSUOV_SEED seed_V, // input
  MATRIX_kxV       V  // output
){
  const int n3 = TSUOV_kv_LEN;
  uint8_t r3[TSUOV_kv_LEN] ;
  TSUOV_PRG_CTX * ctx = PRG_init(seed_V) ;
  RejSampPRG(ctx, 0, n3, TSUOV_kv_RS_LEN, r3) ;
  ExpandMatrixkxV(r3, V) ;
  PRG_final(ctx) ;
}

static void Expand_sol (
  const   TSUOV_SEED seed_sol,  // input
  uint8_t dst[TSUOV_m2]          // output
){
  const int n4 = TSUOV_t_LEN ;
  uint8_t r4[TSUOV_m2 > TSUOV_t_RS_LEN ? TSUOV_m2 : TSUOV_t_RS_LEN] ;
  TSUOV_PRG_CTX * ctx = PRG_init(seed_sol) ;
  RejSampPRG(ctx, 0, n4, TSUOV_t_RS_LEN, r4) ;
  memcpy(dst, r4, n4) ;
  PRG_final(ctx) ;
}

void tsuov_expand_sol(const TSUOV_SEED seed_sol, uint8_t dst[TSUOV_m2])
{
  Expand_sol(seed_sol, dst);
}

static void pack_0 (Fq oil_u[TSUOV_m2], MATRIX_kxO oil) {
  Fq * s = oil_u ;
  for(int k = 0; k < TSUOV_k; k++) {
    for(int i=0; i<TSUOV_O; i++) {
      uint8_t b0 = *s++;                          // todo: generalize for different Fql
      uint8_t b1 = *s++;
      oil[k][1][i] = Fq_mul(b1, 4);
      oil[k][0][i] = Fq_sub(b0, oil[k][1][i]);
    }
  }
}

/* ---------------------------------------------------------
   KeyGen
   --------------------------------------------------------- */




void TSUOV_KeyGen (
  const TSUOV_SEED seed_sk, // input
  const TSUOV_SEED seed_pk, // input
  TSUOV_P3         P3       // output
){
  MATRIX_OxN SdT ;
  MATRIX_NxN Pi ;
  MATRIX_OxN TMP ; 

  Expand_S(seed_sk, SdT) ;
  TSUOV_PRG_CTX * ctx = PRG_init(seed_pk) ;
  for(int i = 0; i < TSUOV_m1; i++){
    Expand_P_MATRIX(ctx, i, Pi) ;
    // Pi padded row-major NxN_padded matrix, TMP padded column-major N_paddedxO matrix, SdT column-major OxN_padded matrix
    MATRIX_mul_MATRIX((uint8_t *)Pi, (uint8_t *)SdT, (uint8_t *)TMP, TSUOV_N, TSUOV_N, TSUOV_O, (TSUOV_N + 31) & ~31) ;
    MATRIX_mul_MATRIX((uint8_t *)SdT, (uint8_t *)TMP, (uint8_t *)P3[i], TSUOV_O, TSUOV_N, TSUOV_O, TSUOV_O) ;
  }
  PRG_final(ctx) ; 

  secure_zero(TMP, sizeof(TMP)) ;
  secure_zero(SdT, sizeof(MATRIX_OxV)) ;

  return ;
}






/* ---------------------------------------------------------
   Sign
   --------------------------------------------------------- */

static void SIG_GEN(const MATRIX_kxO oil, const MATRIX_OxV SdT, const MATRIX_kxV vineger, TSUOV_SIGNATURE sig){
  MATRIX_kxV t;
  MATRIX_VxO_dot_VECTOR_O_whipk(SdT, oil, t);
  VECTOR_V_sub_VECTOR_V_whipk_save_in_sign(vineger, t, sig->s);
  for(int k = 0; k < TSUOV_k; k++) {
    for(int l=0; l<TSUOV_L; l++) 
      memcpy(&sig->s[k][l][TSUOV_V], &oil[k][l][0], TSUOV_O * sizeof(uint8_t));
  }
}







void TSUOV_Sign (
  // ---------------------------------------------------
  const TSUOV_SEED seed_sk,        // input (signing key)
  const TSUOV_SEED seed_pk,        // input (signing key)
  // ---------------------------------------------------
  const TSUOV_SEED seed_v,         // input (seed_v   ->   \bar{v}_i:(F_q)^v     )
  const TSUOV_SEED seed_r,         // input (seed_r   ->   r:{0,1}^lambda)
  const TSUOV_SEED seed_sol,       // input (seed_sol -> sol:(F_q)^m     )
  // ---------------------------------------------------
  const uint8_t    message[],      // input (message)
  const size_t     message_length, // input (message)
  // ---------------------------------------------------
  TSUOV_SIGNATURE  sig        // output
) {
  //  int i,j,k,o ;

  ECHELON_FORM echelon_form ;

  MATRIX_kxO oil ;

  Fq  oil_u   [TSUOV_m2] ; // unpacked oil.
  Fq  b       [TSUOV_m2] ; //
  Fq  b2      [TSUOV_m2] ; //
  vector_m1  U[(TSUOV_k*(TSUOV_k+1))/2] ;
  Fq  c       [TSUOV_m2] ; //

  Fq M   [TSUOV_m1][TSUOV_k][TSUOV_o] ;

  Fq eqn [TSUOV_m2][TSUOV_k*TSUOV_o] ;
  Fq R   [TSUOV_m2][TSUOV_m2] ;

  int cacheR        = 0 ;

  Fq msg [TSUOV_m2] ;

  MATRIX_OxV SdT             ;
  MATRIX_OxV Fi2T           ;
  MATRIX_VxV Pi1             ;
  MATRIX_kxV y               ; // vineger
  VECTOR_V *vT = y           ;
  MATRIX_kxV vT_Pi1          ;
  VECTOR_V *vT_Fi1 = vT_Pi1  ;
  MATRIX_kxV phi_vT          ;
  Fq vT_Fi2 [TSUOV_k][TSUOV_L][TSUOV_O];  
  Fq vTk_Pi1_Sd [TSUOV_L][TSUOV_O];
  
  // gen Sd, SdT

  Expand_sk(seed_sk, SdT);
  
  
  // gen v1, ..., vk for vinegar part
  Expand_V (seed_v, vT) ;
  for(int i = 0; i < TSUOV_k; i++) {
    phi_t2_q31_planar_avx2((uint8_t *)vT[i], (uint8_t *)phi_vT[i], BLOCK_ALIGNED_BYTES(TSUOV_V));
  }
  
  TSUOV_PRG_CTX * ctx_pk = PRG_init(seed_pk) ;
  
  for(int i = 0; i < TSUOV_m1; i++){
    
    // gen -Pi1, Pi2
    Expand_pk(ctx_pk, i, Pi1, Fi2T);
    
    // compute vj(-Pi1), vjPi2, then vjFi2 = vjPi2 + vj(-Pi1)Sd

    for(int k = 0; k < TSUOV_k; k++) {
      int TSUOV_Vpadded = (TSUOV_V + 31) & ~31;
      VECTOR_mul_MATRIX((uint8_t *)vT[k], (uint8_t *)Pi1, (uint8_t *)vT_Pi1[k], TSUOV_V, TSUOV_V, TSUOV_Vpadded) ;

      VECTOR_mul_MATRIX((uint8_t *)(vT_Pi1[k]), (uint8_t *)SdT, (uint8_t *)vTk_Pi1_Sd, TSUOV_V, TSUOV_O, TSUOV_O) ;

      VECTOR_mul_MATRIX((uint8_t *)vT[k], (uint8_t *)Fi2T, (uint8_t *)vT_Fi2[k], TSUOV_V, TSUOV_O, TSUOV_O) ;
 
      vector_add((uint8_t *)vT_Fi2[k], (uint8_t *)vTk_Pi1_Sd, TSUOV_O*TSUOV_L) ;

      
      
      // M_matrices_GEN; use phi_2 mapping; todo: generalize for any Phi
      for(int j=0; j<TSUOV_O; j++){
        Fq u0 = vT_Fi2[k][0][j] ;                 // todo: generlize
        Fq u1 = vT_Fi2[k][1][j] ;
        M[i][k][TSUOV_L*j] = Fq_add(u0, u1) ; 
        M[i][k][TSUOV_L*j+1] = Fq_mul(8, u1) ; 
      }

      // C_GEN c[i] <- vT_Fi1 . y
      // k = j is equal, but k != j is different
      size_t rowk_start_index = k*TSUOV_k - (k*(k+1))/2;
      phi_t2_q31_planar_avx2((uint8_t *)vT_Fi1[k], (uint8_t *)vT_Fi1[k], BLOCK_ALIGNED_BYTES(TSUOV_V));
      for(int j = k; j < TSUOV_k; j++) {
        uint64_t c = vector_v_dot_vector_v(vT_Fi1[k], phi_vT[j]);
        U[rowk_start_index+j][i] = (Fq)(c % TSUOV_q);     //todo: fast modq
      }
    }
  }
  
  PRG_final(ctx_pk) ;

  Fq MT[TSUOV_k*TSUOV_o][TSUOV_m2];
  MATRIX_TRANSPOSE_m2xLO(M, MT); 

  Fq eqnT [TSUOV_k*TSUOV_o][TSUOV_m2] ;

  memset(eqnT, 0, sizeof(eqnT));
  memset(c, 0, sizeof(c));


  init_mul_table_mod31();
  // whipping-up gen c and eqn
 int ctr = 0;
  for(int i = 0; i < TSUOV_k; i++) {
    int rowi_start_index = i*TSUOV_k - (i*(i+1))/2;
    for(int j = TSUOV_k-1; j >= i; j--) {
      multiply_E_add_m1_m2_avx2(U[rowi_start_index+j], c, ctr);
      ctr++;
    }
  } 


  int ctr1 = 0, ctr2 = TSUOV_k*(TSUOV_k+1)/2-3;
  Fq sum [TSUOV_o][TSUOV_m2];
  memset(sum, 0, sizeof(sum));
  for(int j = TSUOV_k-1; j >= 1; j--) {
      multiply_E_add_mat_m2_avx2((Fq *)MT[j*TSUOV_o], (Fq *)sum,ctr1, TSUOV_o);
      multiply_E_add_mat_m2_avx2((Fq *)sum, (Fq *)eqnT[(j-1)*TSUOV_o],ctr2, TSUOV_o);
      ctr1++;
      ctr2 -= TSUOV_k-j+2;
  }
  
  ctr1 = 0, ctr2 = TSUOV_k-1;
  memset(sum, 0, sizeof(sum));
  for(int j = 0; j < TSUOV_k; j++) {
    Fq tmp[TSUOV_o][TSUOV_m2];
    multiply_E_mat_m2_avx2((Fq *)MT[j*TSUOV_o], (Fq *)tmp, ctr1, TSUOV_o);
    vector_add((uint8_t*)sum, (uint8_t*)tmp,  TSUOV_o*TSUOV_m2);
    vector_add((uint8_t*)tmp, (uint8_t*)sum, TSUOV_o*TSUOV_m2);
    multiply_E_add_mat_m2_avx2((Fq *)tmp, (Fq *)eqnT[j*TSUOV_o], ctr2, TSUOV_o);
    ctr1 += TSUOV_k-j;
    ctr2--;
  }

  
 
 // transpose eqnT to eqn
 for(int i = 0; i < TSUOV_m2; i++) {
  for(int j = 0; j < TSUOV_o * TSUOV_k; j++) {
    eqn[i][j] = (Fq)(eqnT[j][i]);   
  } 
 }

  
  LU_decompose(eqn, echelon_form) ;


  uint8_t mu [TSUOV_MU_LEN] ;
  Expand_mu(seed_pk, message, message_length, mu) ;

  TSUOV_PRG2_CTX * ctx_r = PRG2_init(seed_r) ;
  do{
    PRG2_yield(ctx_r, TSUOV_SALT_LEN, sig->r) ;
    Hash(mu, sig->r, msg) ;
    for(int i=0; i<TSUOV_m2; i++) b[i] = Fq_add(msg[i], c[i]) ;
  }while(!consistent(echelon_form, b, &cacheR, R)) ;
  PRG2_final(ctx_r) ;

  sample_a_solution(seed_sol, echelon_form, b, oil_u, b2) ;

  pack_0(oil_u, oil) ;        // todo: pack oil and vineager

  SIG_GEN(oil, SdT, y, sig) ;

  secure_zero(SdT, sizeof(MATRIX_OxV)) ;

  return ;
}


int TSUOV_Verify(
  const TSUOV_SEED       seed_pk,        // input
  const TSUOV_P3         P3,             // input
  const uint8_t          message[],      // input (message)
  const size_t           message_length, // input (message)
  const TSUOV_SIGNATURE  sig             // input
) {

  uint8_t mu [TSUOV_MU_LEN] ;
  Expand_mu(seed_pk, message, message_length, mu) ;
  Fq msg [TSUOV_m2] ;
  Hash(mu, sig->r, msg) ;
  
  MATRIX_NxN Pi ;

  
  vector_m1  U[(TSUOV_k*(TSUOV_k+1))/2] ;
  
  MATRIX_kxN phi_s;


  
  for(int i = 0; i < TSUOV_k; i++) {
    phi_t2_q31_planar_avx2((uint8_t *)sig->s[i], (uint8_t *)phi_s[i], BLOCK_ALIGNED_BYTES(TSUOV_N));
  }

  int verify = 1 ;

  TSUOV_PRG_CTX * ctx_pk = PRG_init(seed_pk) ;
  


  for(int i = 0; i < TSUOV_m1; i++) {
    Expand_PK(P3[i], ctx_pk, i, Pi);
    for(int k1 = 0; k1 < TSUOV_k; k1++) {
      size_t rowk_start_index = k1*TSUOV_k - (k1*(k1+1))/2;
      VECTOR_N sk1_Pi; 
      VECTOR_N phi_sk1_Pi;
      VECTOR_mul_MATRIX((uint8_t *)sig->s[k1], (uint8_t *)Pi, (uint8_t *)sk1_Pi, TSUOV_N, TSUOV_N, BLOCK_ALIGNED_BYTES(TSUOV_N));
      phi_t2_q31_planar_avx2((uint8_t *)sk1_Pi, (uint8_t *)phi_sk1_Pi, BLOCK_ALIGNED_BYTES(TSUOV_N));
      for(int k2 = k1; k2 < TSUOV_k; k2++) {
        U[rowk_start_index+k2][i] = vector_dot((uint8_t *)phi_sk1_Pi, (uint8_t *)phi_s[k2], 2*BLOCK_ALIGNED_BYTES(TSUOV_N));
      }
    }
  }
  
  
  Fq acc[TSUOV_m2] ;
  
  memset(acc, 0, sizeof(acc));

  init_mul_table_mod31();
  
  int ctr = 0;
  for(int i = 0; i < TSUOV_k; i++) {
    size_t rowi_start_index = i*TSUOV_k - (i*(i+1))/2;
    for(int j = TSUOV_k-1; j >= i; j--) {

      multiply_E_add_m1_m2_avx2(U[rowi_start_index+j], (uint8_t *)acc, ctr);
      // save_matrices_test("cij_verify.txt", cij, TSUOV_m2, 1);
      ctr++;
    }
  } 
  
  for(int i=0;i<TSUOV_m2;i++){
    verify &= (acc[i] == msg[i]) ;
    if(! verify) break ;
  }
  PRG_final(ctx_pk) ;
  return verify ;
}

/* ---------------------------------------------------------
   memory I/O
   --------------------------------------------------------- */

void store_TSUOV_P3(
  const TSUOV_P3   P3,       // input
  uint8_t        * pool,     // output
  size_t         * pool_bits // input/output (current bit index)
){
  for(int i=0; i<TSUOV_m1; i++){
    for(int j=0; j<TSUOV_O; j++){
      for(int k=0; k<TSUOV_O; k++){
        if(k<j){
          // do nothing
          // if(P3[i][j][k] != P3[i][k][j]){ printf("error\n"); };
	}else{
          for(int n=0; n<TSUOV_L; n++){
            store_Fq(P3[i][j][n][k], pool, pool_bits) ;
	  }
        }
      }
    }
  }
}

void restore_TSUOV_P3(
  const uint8_t * pool,      // input
  size_t        * pool_bits, // input/output (current bit index)
  TSUOV_P3        P3         // output
){
  for(int i=0; i<TSUOV_m1; i++){
    for(int j=0; j<TSUOV_O; j++){
      for(int k=0; k<TSUOV_O; k++){
        if(k<j){
          for(int n=0; n<TSUOV_L; n++) P3[i][j][n][k] = P3[i][k][n][j] ;
        }else{
          for(int n=0; n<TSUOV_L; n++) P3[i][j][n][k] = restore_Fq(pool, pool_bits) ;
        }
      }
    }
  }
}
