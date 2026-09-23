/**
 * \file gabidulin.c
 * \brief Implementation of gabidulin.h
 *
 * The decoding algorithm provided is based on q_polynomials reconstruction, see \cite gabidulin:welch and \cite gabidulin:generalized for details.
 *
 */

#include "ffi_elt.h"
#include "ffi_vec.h"
#include "gabidulin.h"
#include "parameters.h"
#include "q_polynomial.h"

static void gabidulin_elt_set(ffi_elt* o, ffi_elt e) { ffi_elt_set(o, &e); }
static void gabidulin_elt_set_zero(ffi_elt* o) { ffi_elt_set_zero(o); }
static int gabidulin_elt_is_zero(ffi_elt e) { return ffi_elt_is_zero(&e); }
static void gabidulin_elt_inv(ffi_elt* o, ffi_elt e) { ffi_elt_inv(o, &e); }
static void gabidulin_elt_sqr(ffi_elt* o, ffi_elt e) { ffi_elt_sqr(o, &e); }
static void gabidulin_elt_add(ffi_elt* o, ffi_elt e1, ffi_elt e2) { ffi_elt_add(o, &e1, &e2); }
static void gabidulin_elt_mul(ffi_elt* o, ffi_elt e1, ffi_elt e2) { ffi_elt_mul(o, &e1, &e2); }
static void gabidulin_vec_set_coeff(ffi_vec* o, ffi_elt e, unsigned int position) { ffi_vec_set_coeff(o, &e, position); }
static void gabidulin_q_polynomial_evaluate(ffi_elt* o, const q_polynomial* p, ffi_elt e) {
  q_polynomial_evaluate(o, p, &e);
}
static void gabidulin_q_polynomial_init(q_polynomial* p, unsigned int max_degree) {
  if(q_polynomial_init(p, max_degree) != 0) {
    abort();
  }
}

#define ffi_elt_set(o, e) gabidulin_elt_set(&(o), (e))
#define ffi_elt_set_zero(o) gabidulin_elt_set_zero(&(o))
#define ffi_elt_is_zero(e) gabidulin_elt_is_zero((e))
#define ffi_elt_inv(o, e) gabidulin_elt_inv(&(o), (e))
#define ffi_elt_sqr(o, e) gabidulin_elt_sqr(&(o), (e))
#define ffi_elt_add(o, e1, e2) gabidulin_elt_add(&(o), (e1), (e2))
#define ffi_elt_mul(o, e1, e2) gabidulin_elt_mul(&(o), (e1), (e2))
#define ffi_elt_print(e) ffi_elt_print(&(e))
#define ffi_vec_get_coeff(v, position) ffi_vec_get_coeff(&(v), (position))
#define ffi_vec_set_coeff(o, e, position) gabidulin_vec_set_coeff(&(o), (e), (position))
#define ffi_vec_set_length(o, size) ffi_vec_set_length(&(o), (size))
#define ffi_vec_set_zero(o, size) ffi_vec_set_zero(&(o), (size))
#define ffi_vec_set(o, v, size) ffi_vec_set(&(o), &(v), (size))
#define q_polynomial_set(o, p) q_polynomial_set(&(o), &(p))
#define q_polynomial_set_zero(o) q_polynomial_set_zero(&(o))
#define q_polynomial_set_one(o) q_polynomial_set_one(&(o))
#define q_polynomial_set_interpolate_vect_and_zero(o1, o2, v1, v2, size) \
  q_polynomial_set_interpolate_vect_and_zero(&(o1), &(o2), &(v1), &(v2), (size))
#define q_polynomial_evaluate(o, p, e) gabidulin_q_polynomial_evaluate(&(o), &(p), (e))
#define q_polynomial_scalar_mul(o, p, e) q_polynomial_scalar_mul(&(o), &(p), &(e))
#define q_polynomial_qexp(o, p) q_polynomial_qexp(&(o), &(p))
#define q_polynomial_add(o, p, q) q_polynomial_add(&(o), &(p), &(q))
#define q_polynomial_mul(o, p, q) q_polynomial_mul(&(o), &(p), &(q))
#define q_polynomial_left_div(q, r, a, b) q_polynomial_left_div(&(q), &(r), &(a), &(b))
/** 
 * \fn gabidulin_code gabidulin_code_init(const ffi_vec& g, unsigned int k, unsigned int n)
 * \brief This function initializes a gabidulin code
 *
 * \param[in] g Generator vector defining the code
 * \param[in] k Size of vectors representing messages
 * \param[in] n Size of vetors representing codewords
 * \return Gabidulin code
 */
int gabidulin_code_init(gabidulin_code* code, const ffi_vec* g, unsigned int k, unsigned int n) {
  ffi_vec_init(&code->g);
  if(ffi_vec_copy(&code->g, g) != 0) {
    return -1;
  }
  code->k = k;
  code->n = n;
  return 0;
}

void gabidulin_code_clear(gabidulin_code* code) {
  ffi_vec_clear(&code->g);
  code->k = 0;
  code->n = 0;
}



/** 
 * \fn void gabidulin_code_encode(ffi_vec& c, gabidulin_code gc, const ffi_vec& m)
 * \brief This function encodes a message into a codeword
 *
 * This function assumes <b>FIELD_Q</b> = 2
 *
 * \param[out] c Vector of size <b>n</b> representing a codeword
 * \param[in] gc Gabidulin code
 * \param[in] m Vector of size <b>k</b> representing a message
 */
void gabidulin_code_encode(ffi_vec* c_ptr, const gabidulin_code* gc_ptr, const ffi_vec* m_ptr) {
#define c (*c_ptr)
#define gc (*gc_ptr)
#define m (*m_ptr)
  // Compute generator matrix
  ffi_elt* matrix = (ffi_elt*) calloc((size_t) gc.k * gc.n, sizeof(ffi_elt));
  if(matrix == NULL) {
    abort();
  }
  for(unsigned int j = 0 ; j < gc.n ; ++j) {
    ffi_elt_set(matrix[j], gc.g.data[j]);
    for(unsigned int i = 1 ; i < gc.k ; ++i) {
      ffi_elt_sqr(matrix[i * gc.n + j], matrix[(i - 1) * gc.n + j]);
    }
  }

  // Encode message
  ffi_elt tmp = {0};
  ffi_elt_set_zero(tmp);
  ffi_vec_set_zero(c, gc.n);
  for(unsigned int i = 0 ; i < gc.k ; ++i) {
    for(unsigned int j = 0 ; j < gc.n ; ++j) {
      ffi_elt_mul(tmp, m.data[i], matrix[i * gc.n + j]);
      ffi_elt_add(tmp, c.data[j], tmp);
      ffi_vec_set_coeff(c, tmp, j);
    }
  }
  #ifdef VERBOSE
    printf("\n\n\n# Gabidulin Encoding - Begin #");
    printf("\n\ng: "); //ffi_vec_print(gc.g, PARAM_N);

    printf("\nmatrix:[ ");
    for(unsigned int i = 0 ; i < gc.k ; ++i) {
      printf("[ ");
      for(unsigned int j = 0 ; j < gc.n ; ++j) {
        ffi_elt_print(matrix[i * gc.n + j]);
      }
      printf("] ");
    }
    printf("]\n");

    printf("\ncodeword: "); //ffi_vec_print(c, PARAM_N);
    printf("\n# Gabidulin Encoding - End #\n");
  #endif
  free(matrix);
#undef m
#undef gc
#undef c
}



/** 
 * \fn void gabidulin_code_decode(ffi_vec& m, gabidulin_code gc, const ffi_vec& y)
 * \brief This function decodes a word
 *
 * As explained in the supporting documentation, the provided decoding algorithm works as follows (see \cite gabidulin:welch and \cite gabidulin:generalized for details):
 *   1. Find a solution (<b>V</b>, <b>N</b>) of the q-polynomial Reconstruction2(<b>y</b>, <b>gc.g</b>, <b>gc.k</b>, (<b>gc.n</b> - <b>gc.k</b>)/2) problem using \cite gabidulin:generalized (section 4, algorithm 5) ;
 *   2. Find <b>f</b> by computing <b>V \ (N.A) + I</b> (see "Polynomials with lower degree" improvement from \cite gabidulin:generalized, section 4.4.2) ;
 *   3. Retrieve the message <b>m</b> as the k first coordinates of <b>f</b>.
 *
 *  This function assumes <b>FIELD_Q</b> = 2
 *
 * \param[out] m Vector of size <b>k</b> representing a message
 * \param[in] gc Gabidulin code
 * \param[in] y Vector of size <b>n</b> representing a word to decode
 */


void gabidulin_code_decode_3(ffi_vec* m_ptr, const gabidulin_code* gc_ptr, const ffi_vec* y_ptr, q_polynomial* V2_ptr) {
#define m (*m_ptr)
#define gc (*gc_ptr)
#define y (*y_ptr)
#define V2 (*V2_ptr)
  //cout<<"开始打印各项数据"<<endl;
  //cout<<"n: "<<gc.n<<endl;
  //cout<<"k: "<<gc.k<<endl;
  //cout<<"g: "<<endl;
  //ffi_vec_print(gc.g,gc.n);
  /*  
   *  Step 1: Solving the q-polynomial reconstruction2 problem 
   */

  int t = (gc.n - gc.k) / 2;
  int max_degree_N = (gc.n - gc.k) % 2 == 0 ? gc.k + t - 1 : gc.k + t;
  //cout<<"t: "<<t<<endl;
  //cout<<"max_degree_N: "<<max_degree_N<<endl;

  q_polynomial A = {0}, I = {0};
  q_polynomial N0 = {0}, N1 = {0}, V0 = {0}, V1 = {0};
  q_polynomial qtmp1 = {0}, qtmp2 = {0}, qtmp3 = {0}, qtmp4 = {0};
  ffi_vec u0 = {0}, u1 = {0};
  ffi_elt e1 = {0}, e2 = {0}, tmp1 = {0}, tmp2 = {0};

  gabidulin_q_polynomial_init(&A, gc.k);
  gabidulin_q_polynomial_init(&I, gc.k - 1);
  gabidulin_q_polynomial_init(&N0, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&N1, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&V0, (unsigned int) t);
  gabidulin_q_polynomial_init(&V1, (unsigned int) t);
  gabidulin_q_polynomial_init(&qtmp1, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&qtmp2, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&qtmp3, (unsigned int) t);
  gabidulin_q_polynomial_init(&qtmp4, (unsigned int) t);
  ffi_vec_init(&u0);
  ffi_vec_init(&u1);

  //cout<<"初始化..."<<endl;
  
  // Initialization step
  
  // A(g[i]) = 0 for 0 <= i <= k - 1
  // I(g[i]) = y[i] for 0 <= i <= k - 1
  
  q_polynomial_set_interpolate_vect_and_zero(A, I, gc.g, y, gc.k);

  //cout<<"A: (应满足A(g[i]) = 0 for 0 <= i <= k - 1)"<<endl;
  //q_polynomial_print(A);
  //cout<<"I: (应满足I(g[i]) = y[i] for 0 <= i <= k - 1)"<<endl;
  //q_polynomial_print(I);

  q_polynomial_set_one(N0);
  q_polynomial_set_zero(N1);
  q_polynomial_set_zero(V0);
  q_polynomial_set_one(V1);

  q_polynomial_set_zero(qtmp1);
  q_polynomial_set_zero(qtmp2);
  q_polynomial_set_zero(qtmp3);
  q_polynomial_set_zero(qtmp4);

  //cout<<"N0: "<<endl;
  //q_polynomial_print(N0);
  //cout<<"N1: "<<endl;
  //q_polynomial_print(N1);
  //cout<<"V0: "<<endl;
  //q_polynomial_print(V0);
  //cout<<"V1: "<<endl;
  //q_polynomial_print(V1);

  // u0[i] = A(g[i]) - V0(y[i])
  // u1[i] = I(g[i]) - V1(y[i])

  for(unsigned int i = 0 ; i < gc.n ; ++i) {
    q_polynomial_evaluate(tmp1, A, ffi_vec_get_coeff(gc.g, i));
    q_polynomial_evaluate(tmp2, V0, ffi_vec_get_coeff(y, i));
    ffi_elt_add(tmp1, tmp1, tmp2);
    ffi_vec_set_coeff(u0, tmp1, i);

    q_polynomial_evaluate(tmp1, I, ffi_vec_get_coeff(gc.g, i));
    q_polynomial_evaluate(tmp2, V1, ffi_vec_get_coeff(y, i));
    ffi_elt_add(tmp1, tmp1, tmp2);
    ffi_vec_set_coeff(u1, tmp1, i);
  }

  //cout<<"u0: (应满足A(g[i]) - V0(y[i])) "<<endl;
  //ffi_vec_print(u0,gc.n);
  //cout<<"u1: (应满足I(g[i]) - V1(y[i]))"<<endl;
  //ffi_vec_print(u1,gc.n);
  //cout<<"初始化完成！"<<endl;

  #ifdef VERBOSE
    printf("\n\n# Gabidulin Decoding - Begin #");
    printf("\n\ng: "); //ffi_vec_print(gc.g, PARAM_N);
    printf("\nA: "); //q_polynomial_print(A);
    printf("\nI: "); //q_polynomial_print(I);
    printf("\nN0 (init): "); //q_polynomial_print(N0);
    printf("\nN1 (init): "); //q_polynomial_print(N1);
    printf("\nV0 (init): "); //q_polynomial_print(V0);
    printf("\nV1 (init): "); //q_polynomial_print(V1);
  #endif


  // Interpolation step 
  //cout<<"插值开始..."<<endl;
  int updateType = -1;
  for(unsigned int i = gc.k ; i < gc.n ; ++i) {
    //cout<<"第"<<i-gc.k+1<<"次循环"<<endl;
    unsigned int j = i;
    //cout<<"j: "<<j<<endl;
    while(ffi_elt_is_zero(ffi_vec_get_coeff(u0, j)) == 0 && 
          ffi_elt_is_zero(ffi_vec_get_coeff(u1, j)) == 1 &&
          j < gc.n) j++;
    //cout<<"j更新到: "<<j<<endl;
    if(j == gc.n) {
      break;
    } else {
      if(i != j) {
        // Permutation of the coordinates of positions i and j
        ffi_elt_set(tmp1, ffi_vec_get_coeff(u0, i));
        ffi_vec_set_coeff(u0, ffi_vec_get_coeff(u0, j), i);
        ffi_vec_set_coeff(u0, tmp1, j);

        ffi_elt_set(tmp1, ffi_vec_get_coeff(u1, i));
        ffi_vec_set_coeff(u1, ffi_vec_get_coeff(u1, j), i);
        ffi_vec_set_coeff(u1, tmp1, j);

        //cout<<"u0: "<<endl;
        //ffi_vec_print(u0,gc.n);
        //cout<<"u1: "<<endl;
        //ffi_vec_print(u1,gc.n);
      }
    }

    //cout<<"Update q_polynomials according to discrepancies"<<endl;
    // Update q_polynomials according to discrepancies
    if(ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) != 1) {
      updateType = 1;
      //cout<<"Updatetype"<<updateType<<endl;
      // e1 = - u1[i]^q / u1[i] 
      // e2 = - u0[i] / u1[i]
      // N0' = N1^q - e1.N1
      // V0' = V1^q - e1.V1
      // N1' = N0 - e2.N1 
      // V1' = V0 - e2.V1
      
      ffi_elt_inv(tmp1, ffi_vec_get_coeff(u1, i));
      ffi_elt_sqr(e1, ffi_vec_get_coeff(u1, i));
      ffi_elt_mul(e1, e1, tmp1);
      ffi_elt_mul(e2, ffi_vec_get_coeff(u0, i), tmp1);
      
      q_polynomial_scalar_mul(qtmp1, N1, e1);
      q_polynomial_qexp(qtmp2, N1);
      q_polynomial_scalar_mul(qtmp3, V1, e1);
      q_polynomial_qexp(qtmp4, V1);

      q_polynomial_scalar_mul(N1, N1, e2);
      q_polynomial_add(N1, N0, N1);

      q_polynomial_scalar_mul(V1, V1, e2);
      q_polynomial_add(V1, V0, V1);
      
      q_polynomial_add(N0, qtmp1, qtmp2);
      q_polynomial_add(V0, qtmp3, qtmp4);

      //cout<<"N0: "<<endl;
      //q_polynomial_print(N0);
      //cout<<"N1: "<<endl;
      //q_polynomial_print(N1);
      //cout<<"V0: "<<endl;
      //q_polynomial_print(V0);
      //cout<<"V1: "<<endl;
      //q_polynomial_print(V1);

    } 

    if(ffi_elt_is_zero(ffi_vec_get_coeff(u0, i)) == 1 && 
       ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) == 1) {
      updateType = 2;
      //cout<<"Updatetype"<<updateType<<endl;
      // N0' = N1^q 
      // V0' = V1^q
      // N1' = N0 
      // V1' = V0 
      
      q_polynomial_qexp(qtmp1, N1);
      q_polynomial_qexp(qtmp2, V1);

      q_polynomial_set(N1, N0);
      q_polynomial_set(V1, V0);
      q_polynomial_set(N0, qtmp1);
      q_polynomial_set(V0, qtmp2);

      //cout<<"N0: "<<endl;
      //q_polynomial_print(N0);
      //cout<<"N1: "<<endl;
      //q_polynomial_print(N1);
      //cout<<"V0: "<<endl;
      //q_polynomial_print(V0);
      //cout<<"V1: "<<endl;
      //q_polynomial_print(V1);
    } 

    //cout<<"Update discrepancies"<<endl;
    // Update discrepancies
    for(unsigned int k = i + 1 ; k < gc.n ; ++k) {
      if(updateType == 1) {
        //cout<<"Updatetype"<<updateType<<endl;
        // u0[k]' = u1[k]^q - e1.u1[k]
        // u1[k]' = u0[k] - e2.u1[k] 
      
        ffi_elt_mul(tmp1, e1, ffi_vec_get_coeff(u1, k));
        ffi_elt_sqr(tmp2, ffi_vec_get_coeff(u1, k));
        ffi_elt_add(tmp1, tmp1, tmp2);

        ffi_elt_mul(tmp2, e2, ffi_vec_get_coeff(u1, k));
        ffi_elt_add(tmp2, tmp2, ffi_vec_get_coeff(u0, k));
        ffi_vec_set_coeff(u1, tmp2, k);

        ffi_vec_set_coeff(u0, tmp1, k);

        //cout<<"u0: "<<endl;
        //ffi_vec_print(u0,gc.n);
        //cout<<"u1: "<<endl;
        //ffi_vec_print(u1,gc.n);
      } 
      
      if(updateType == 2) {
        //cout<<"Updatetype"<<updateType<<endl;
        // u0[k]' = u0[k]
        // u1[k]' = u1[k]^q
        
        ffi_elt_sqr(tmp1, u1.data[k]);
        ffi_vec_set_coeff(u1, tmp1, k);

        //cout<<"u0: "<<endl;
        //ffi_vec_print(u0,gc.n);
        //cout<<"u1: "<<endl;
        //ffi_vec_print(u1,gc.n);
      }
    }

    #ifdef VERBOSE
      printf("\nN0 (%i): ", i); //q_polynomial_print(N0);
      printf("\nN1 (%i): ", i); //q_polynomial_print(N1);
      printf("\nV0 (%i): ", i); //q_polynomial_print(V0);
      printf("\nV1 (%i): ", i); //q_polynomial_print(V1);
    #endif
  }

  //cout<<"插值完成!"<<endl;

  /*  
   *  Step 2: Computing f (qtmp1 variable) using Loidreau's improvement for lower degree polynomials
   */
  //cout<<"A: "<<endl;
  //q_polynomial_print(A);
  //cout<<"V1: "<<endl;
  //q_polynomial_print(V1);
  //cout<<"I: "<<endl;
  //q_polynomial_print(I);
  q_polynomial_mul(qtmp1, N1, A);
  q_polynomial_left_div(qtmp3, qtmp2, qtmp1, V1);
  q_polynomial_add(qtmp1, qtmp3, I);
  q_polynomial_left_div(qtmp3, qtmp2, qtmp1, V2);


  /*  
   *  Step 3: Decoding the message as the value of the k first coordinates of f (qtmp1 variable)
   */

  ffi_vec_set(m, qtmp3.values, 3);

  #ifdef VERBOSE
    printf("\nquotient: "); //q_polynomial_print(qtmp1);
    printf("\nremainder: "); //q_polynomial_print(qtmp2);
    printf("\nmu: "); //ffi_vec_print(m, PARAM_K);
    printf("\n# Gabidulin Decoding - End #\n");
  #endif

  ffi_vec_clear(&u0);
  ffi_vec_clear(&u1);
  q_polynomial_clear(&A);
  q_polynomial_clear(&I);
  q_polynomial_clear(&N0);
  q_polynomial_clear(&N1);
  q_polynomial_clear(&V0);
  q_polynomial_clear(&V1);
  q_polynomial_clear(&qtmp1);
  q_polynomial_clear(&qtmp2);
  q_polynomial_clear(&qtmp3);
  q_polynomial_clear(&qtmp4);
#undef V2
#undef y
#undef gc
#undef m
}



void gabidulin_code_decode_2(ffi_vec* m_ptr, const gabidulin_code* gc_ptr, const ffi_vec* y_ptr, q_polynomial* V2_ptr) {
#define m (*m_ptr)
#define gc (*gc_ptr)
#define y (*y_ptr)
#define V2 (*V2_ptr)

  /*  
   *  Step 1: Solving the q-polynomial reconstruction2 problem 
   */

  // int t = (gc.n - gc.k) / 2;
  // int max_degree_N = (gc.n - gc.k) % 2 == 0 ? gc.k + t - 1 : gc.k + t;
  int t = 36;
  int max_degree_N = 38;
  q_polynomial A = {0}, I = {0};
  q_polynomial N0 = {0}, N1 = {0}, V0 = {0}, V1 = {0};
  q_polynomial qtmp1 = {0}, qtmp2 = {0}, qtmp3 = {0}, qtmp4 = {0};
  ffi_vec u0 = {0}, u1 = {0};
  ffi_elt e1 = {0}, e2 = {0}, tmp1 = {0}, tmp2 = {0};

  gabidulin_q_polynomial_init(&A, gc.k);
  gabidulin_q_polynomial_init(&I, gc.k - 1);
  gabidulin_q_polynomial_init(&N0, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&N1, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&V0, (unsigned int) t);
  gabidulin_q_polynomial_init(&V1, (unsigned int) t);
  gabidulin_q_polynomial_init(&qtmp1, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&qtmp2, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&qtmp3, (unsigned int) t);
  gabidulin_q_polynomial_init(&qtmp4, (unsigned int) t);
  ffi_vec_init(&u0);
  ffi_vec_init(&u1);


  // Initialization step
  
  // A(g[i]) = 0 for 0 <= i <= k - 1
  // I(g[i]) = y[i] for 0 <= i <= k - 1
  
  q_polynomial_set_interpolate_vect_and_zero(A, I, gc.g, y, gc.k);
  q_polynomial_set_one(N0);
  q_polynomial_set_zero(N1);
  q_polynomial_set_zero(V0);
  q_polynomial_set_one(V1);

  q_polynomial_set_zero(qtmp1);
  q_polynomial_set_zero(qtmp2);
  q_polynomial_set_zero(qtmp3);
  q_polynomial_set_zero(qtmp4);

  // u0[i] = A(g[i]) - V0(y[i])
  // u1[i] = I(g[i]) - V1(y[i])

  for(unsigned int i = 0 ; i < gc.n ; ++i) {
    q_polynomial_evaluate(tmp1, A, ffi_vec_get_coeff(gc.g, i));
    q_polynomial_evaluate(tmp2, V0, ffi_vec_get_coeff(y, i));
    ffi_elt_add(tmp1, tmp1, tmp2);
    ffi_vec_set_coeff(u0, tmp1, i);

    q_polynomial_evaluate(tmp1, I, ffi_vec_get_coeff(gc.g, i));
    q_polynomial_evaluate(tmp2, V1, ffi_vec_get_coeff(y, i));
    ffi_elt_add(tmp1, tmp1, tmp2);
    ffi_vec_set_coeff(u1, tmp1, i);
  }

  #ifdef VERBOSE
    printf("\n\n# Gabidulin Decoding - Begin #");
    printf("\n\ng: "); //ffi_vec_print(gc.g, PARAM_N);
    printf("\nA: "); //q_polynomial_print(A);
    printf("\nI: "); //q_polynomial_print(I);
    printf("\nN0 (init): "); //q_polynomial_print(N0);
    printf("\nN1 (init): "); //q_polynomial_print(N1);
    printf("\nV0 (init): "); //q_polynomial_print(V0);
    printf("\nV1 (init): "); //q_polynomial_print(V1);
  #endif


  // Interpolation step 
  int updateType = -1;
  for(unsigned int i = gc.k ; i < gc.n ; ++i) {

    unsigned int j = i;
    while(ffi_elt_is_zero(ffi_vec_get_coeff(u0, j)) == 0 && 
          ffi_elt_is_zero(ffi_vec_get_coeff(u1, j)) == 1 &&
          j < gc.n) j++;
    
    if(j == gc.n) {
      break;
    } else {
      if(i != j) {
        // Permutation of the coordinates of positions i and j
        ffi_elt_set(tmp1, ffi_vec_get_coeff(u0, i));
        ffi_vec_set_coeff(u0, ffi_vec_get_coeff(u0, j), i);
        ffi_vec_set_coeff(u0, tmp1, j);

        ffi_elt_set(tmp1, ffi_vec_get_coeff(u1, i));
        ffi_vec_set_coeff(u1, ffi_vec_get_coeff(u1, j), i);
        ffi_vec_set_coeff(u1, tmp1, j);
      }
    }


    // Update q_polynomials according to discrepancies
    if(ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) != 1) {
      updateType = 1;

      // e1 = - u1[i]^q / u1[i] 
      // e2 = - u0[i] / u1[i]
      // N0' = N1^q - e1.N1
      // V0' = V1^q - e1.V1
      // N1' = N0 - e2.N1 
      // V1' = V0 - e2.V1
      
      ffi_elt_inv(tmp1, ffi_vec_get_coeff(u1, i));
      ffi_elt_sqr(e1, ffi_vec_get_coeff(u1, i));
      ffi_elt_mul(e1, e1, tmp1);
      ffi_elt_mul(e2, ffi_vec_get_coeff(u0, i), tmp1);
      
      q_polynomial_scalar_mul(qtmp1, N1, e1);
      q_polynomial_qexp(qtmp2, N1);
      q_polynomial_scalar_mul(qtmp3, V1, e1);
      q_polynomial_qexp(qtmp4, V1);

      q_polynomial_scalar_mul(N1, N1, e2);
      q_polynomial_add(N1, N0, N1);

      q_polynomial_scalar_mul(V1, V1, e2);
      q_polynomial_add(V1, V0, V1);
      
      q_polynomial_add(N0, qtmp1, qtmp2);
      q_polynomial_add(V0, qtmp3, qtmp4);
    } 

    if(ffi_elt_is_zero(ffi_vec_get_coeff(u0, i)) == 1 && 
       ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) == 1) {
      updateType = 2;

      // N0' = N1^q 
      // V0' = V1^q
      // N1' = N0 
      // V1' = V0 
      
      q_polynomial_qexp(qtmp1, N1);
      q_polynomial_qexp(qtmp2, V1);

      q_polynomial_set(N1, N0);
      q_polynomial_set(V1, V0);
      q_polynomial_set(N0, qtmp1);
      q_polynomial_set(V0, qtmp2);
    } 


    // Update discrepancies
    for(unsigned int k = i + 1 ; k < gc.n ; ++k) {
      if(updateType == 1) {

        // u0[k]' = u1[k]^q - e1.u1[k]
        // u1[k]' = u0[k] - e2.u1[k] 
      
        ffi_elt_mul(tmp1, e1, ffi_vec_get_coeff(u1, k));
        ffi_elt_sqr(tmp2, ffi_vec_get_coeff(u1, k));
        ffi_elt_add(tmp1, tmp1, tmp2);

        ffi_elt_mul(tmp2, e2, ffi_vec_get_coeff(u1, k));
        ffi_elt_add(tmp2, tmp2, ffi_vec_get_coeff(u0, k));
        ffi_vec_set_coeff(u1, tmp2, k);

        ffi_vec_set_coeff(u0, tmp1, k);
      } 
      
      if(updateType == 2) {

        // u0[k]' = u0[k]
        // u1[k]' = u1[k]^q
        
        ffi_elt_sqr(tmp1, u1.data[k]);
        ffi_vec_set_coeff(u1, tmp1, k);
      }
    }

    #ifdef VERBOSE
      printf("\nN0 (%i): ", i); //q_polynomial_print(N0);
      printf("\nN1 (%i): ", i); //q_polynomial_print(N1);
      printf("\nV0 (%i): ", i); //q_polynomial_print(V0);
      printf("\nV1 (%i): ", i); //q_polynomial_print(V1);
    #endif
  }



  /*  
   *  Step 2: Computing f (qtmp1 variable) using Loidreau's improvement for lower degree polynomials
   */
  //P=A*N_1/V_1+I
  // q_polynomial_mul(qtmp1, N1, A);
  // q_polynomial_left_div(qtmp3, qtmp2, qtmp1, V1);
  // q_polynomial_add(qtmp1, qtmp3, I);
  //cout<<"A:"<<endl;
  ////q_polynomial_print(A);
  //cout<<"N1:"<<endl;
  //q_polynomial_print(N1);
  //cout<<"V1:"<<endl;
  //q_polynomial_print(V1);
  //cout<<"V2:"<<endl;
  //q_polynomial_print(V2);
  //cout<<"I:"<<endl;
  //q_polynomial_print(I);

  //A*N_1+V_1*I
  q_polynomial_mul(qtmp1, N1, A);//max_degree
  //cout<<"A*N1:"<<endl;
  //q_polynomial_print(qtmp1);

  q_polynomial_mul(qtmp3, V1, I);//t
  //cout<<"V1*I:"<<endl;
  //q_polynomial_print(qtmp3);

  q_polynomial_add(qtmp2, qtmp1, qtmp3);//max_degree
  //cout<<"A*N1+V1*I:"<<endl;
  //q_polynomial_print(qtmp2);
  //V_1*V_2
  q_polynomial_mul(qtmp1, V1, V2);
  //cout<<"V1*V2:"<<endl;
  //q_polynomial_print(qtmp1);

  q_polynomial_left_div(qtmp3,qtmp4,qtmp2,qtmp1);
  //cout<<"quotient"<<endl;
  //q_polynomial_print(qtmp3);
  //cout<<"remainder"<<endl;
  //q_polynomial_print(qtmp4);
  //f=A*N_1+w_1*I/V_1*V_2

  ffi_vec result1 = {0};
  ffi_vec_init(&result1);
  ffi_vec_set_length(result1, PARAM_N_);
  for (int i = 0; i < PARAM_N_; i++)
  {
    q_polynomial_evaluate(result1.data[i], V2, y.data[i]);
  }
  //cout<<"result1的值："<<endl;
  //ffi_vec_print(result1, PARAM_N_);
  ffi_vec result2 = {0};
  ffi_vec_init(&result2);
  ffi_vec_set_length(result2, PARAM_N_);
  for (int i = 0; i < PARAM_N_; i++)
  {
    q_polynomial_evaluate(result2.data[i], V2, gc.g.data[i]);
  }
  //cout<<"result2的值："<<endl;
  //ffi_vec_print(result2, PARAM_N_);
  /*  
   *  Step 3: Decoding the message as the value of the k first coordinates of f (qtmp1 variable)
   */

  ffi_vec_set(m, qtmp3.values, gc.k);
  #ifdef VERBOSE
    printf("\nquotient: "); //q_polynomial_print(qtmp1);
    printf("\nremainder: "); //q_polynomial_print(qtmp2);
    printf("\nmu: "); //ffi_vec_print(m, PARAM_K);
    printf("\n# Gabidulin Decoding - End #\n");
  #endif

  ffi_vec_clear(&result1);
  ffi_vec_clear(&result2);
  ffi_vec_clear(&u0);
  ffi_vec_clear(&u1);
  q_polynomial_clear(&A);
  q_polynomial_clear(&I);
  q_polynomial_clear(&N0);
  q_polynomial_clear(&N1);
  q_polynomial_clear(&V0);
  q_polynomial_clear(&V1);
  q_polynomial_clear(&qtmp1);
  q_polynomial_clear(&qtmp2);
  q_polynomial_clear(&qtmp3);
  q_polynomial_clear(&qtmp4);
#undef V2
#undef y
#undef gc
#undef m
}

void gabidulin_code_decode(ffi_vec* m_ptr, const gabidulin_code* gc_ptr, const ffi_vec* y_ptr) {
#define m (*m_ptr)
#define gc (*gc_ptr)
#define y (*y_ptr)
  //cout<<"开始打印各项数据"<<endl;
  //cout<<"n: "<<gc.n<<endl;
  //cout<<"k: "<<gc.k<<endl;
  //cout<<"g: "<<endl;
  //ffi_vec_print(gc.g,gc.n);
  /*  
   *  Step 1: Solving the q-polynomial reconstruction2 problem 
   */

  int t = (gc.n - gc.k) / 2;
  int max_degree_N = (gc.n - gc.k) % 2 == 0 ? gc.k + t - 1 : gc.k + t;
  //cout<<"t: "<<t<<endl;
  //cout<<"max_degree_N: "<<max_degree_N<<endl;

  q_polynomial A = {0}, I = {0};
  q_polynomial N0 = {0}, N1 = {0}, V0 = {0}, V1 = {0};
  q_polynomial qtmp1 = {0}, qtmp2 = {0}, qtmp3 = {0}, qtmp4 = {0};
  ffi_vec u0 = {0}, u1 = {0};
  ffi_elt e1 = {0}, e2 = {0}, tmp1 = {0}, tmp2 = {0};

  gabidulin_q_polynomial_init(&A, gc.k);
  gabidulin_q_polynomial_init(&I, gc.k - 1);
  gabidulin_q_polynomial_init(&N0, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&N1, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&V0, (unsigned int) t);
  gabidulin_q_polynomial_init(&V1, (unsigned int) t);
  gabidulin_q_polynomial_init(&qtmp1, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&qtmp2, (unsigned int) max_degree_N);
  gabidulin_q_polynomial_init(&qtmp3, (unsigned int) t);
  gabidulin_q_polynomial_init(&qtmp4, (unsigned int) t);
  ffi_vec_init(&u0);
  ffi_vec_init(&u1);

  //cout<<"初始化..."<<endl;
  
  // Initialization step
  
  // A(g[i]) = 0 for 0 <= i <= k - 1
  // I(g[i]) = y[i] for 0 <= i <= k - 1
  
  q_polynomial_set_interpolate_vect_and_zero(A, I, gc.g, y, gc.k);

  //cout<<"A: (应满足A(g[i]) = 0 for 0 <= i <= k - 1)"<<endl;
  //q_polynomial_print(A);
  //cout<<"I: (应满足I(g[i]) = y[i] for 0 <= i <= k - 1)"<<endl;
  //q_polynomial_print(I);

  q_polynomial_set_one(N0);
  q_polynomial_set_zero(N1);
  q_polynomial_set_zero(V0);
  q_polynomial_set_one(V1);

  q_polynomial_set_zero(qtmp1);
  q_polynomial_set_zero(qtmp2);
  q_polynomial_set_zero(qtmp3);
  q_polynomial_set_zero(qtmp4);

  //cout<<"N0: "<<endl;
  //q_polynomial_print(N0);
  //cout<<"N1: "<<endl;
  //q_polynomial_print(N1);
  //cout<<"V0: "<<endl;
  //q_polynomial_print(V0);
  //cout<<"V1: "<<endl;
  //q_polynomial_print(V1);

  // u0[i] = A(g[i]) - V0(y[i])
  // u1[i] = I(g[i]) - V1(y[i])

  for(unsigned int i = 0 ; i < gc.n ; ++i) {
    q_polynomial_evaluate(tmp1, A, ffi_vec_get_coeff(gc.g, i));
    q_polynomial_evaluate(tmp2, V0, ffi_vec_get_coeff(y, i));
    ffi_elt_add(tmp1, tmp1, tmp2);
    ffi_vec_set_coeff(u0, tmp1, i);

    q_polynomial_evaluate(tmp1, I, ffi_vec_get_coeff(gc.g, i));
    q_polynomial_evaluate(tmp2, V1, ffi_vec_get_coeff(y, i));
    ffi_elt_add(tmp1, tmp1, tmp2);
    ffi_vec_set_coeff(u1, tmp1, i);
  }

  //cout<<"u0: (应满足A(g[i]) - V0(y[i])) "<<endl;
  //ffi_vec_print(u0,gc.n);
  //cout<<"u1: (应满足I(g[i]) - V1(y[i]))"<<endl;
  //ffi_vec_print(u1,gc.n);
  //cout<<"初始化完成！"<<endl;

  #ifdef VERBOSE
    printf("\n\n# Gabidulin Decoding - Begin #");
    printf("\n\ng: "); //ffi_vec_print(gc.g, PARAM_N);
    printf("\nA: "); //q_polynomial_print(A);
    printf("\nI: "); //q_polynomial_print(I);
    printf("\nN0 (init): "); //q_polynomial_print(N0);
    printf("\nN1 (init): "); //q_polynomial_print(N1);
    printf("\nV0 (init): "); //q_polynomial_print(V0);
    printf("\nV1 (init): "); //q_polynomial_print(V1);
  #endif


  // Interpolation step 
  //cout<<"插值开始..."<<endl;
  int updateType = -1;
  for(unsigned int i = gc.k ; i < gc.n ; ++i) {
    //cout<<"第"<<i-gc.k+1<<"次循环"<<endl;
    unsigned int j = i;
    //cout<<"j: "<<j<<endl;
    while(ffi_elt_is_zero(ffi_vec_get_coeff(u0, j)) == 0 && 
          ffi_elt_is_zero(ffi_vec_get_coeff(u1, j)) == 1 &&
          j < gc.n) j++;
    //cout<<"j更新到: "<<j<<endl;
    if(j == gc.n) {
      break;
    } else {
      if(i != j) {
        // Permutation of the coordinates of positions i and j
        ffi_elt_set(tmp1, ffi_vec_get_coeff(u0, i));
        ffi_vec_set_coeff(u0, ffi_vec_get_coeff(u0, j), i);
        ffi_vec_set_coeff(u0, tmp1, j);

        ffi_elt_set(tmp1, ffi_vec_get_coeff(u1, i));
        ffi_vec_set_coeff(u1, ffi_vec_get_coeff(u1, j), i);
        ffi_vec_set_coeff(u1, tmp1, j);

        //cout<<"u0: "<<endl;
        //ffi_vec_print(u0,gc.n);
        //cout<<"u1: "<<endl;
        //ffi_vec_print(u1,gc.n);
      }
    }

    //cout<<"Update q_polynomials according to discrepancies"<<endl;
    // Update q_polynomials according to discrepancies
    if(ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) != 1) {
      updateType = 1;
      //cout<<"Updatetype"<<updateType<<endl;
      // e1 = - u1[i]^q / u1[i] 
      // e2 = - u0[i] / u1[i]
      // N0' = N1^q - e1.N1
      // V0' = V1^q - e1.V1
      // N1' = N0 - e2.N1 
      // V1' = V0 - e2.V1
      
      ffi_elt_inv(tmp1, ffi_vec_get_coeff(u1, i));
      ffi_elt_sqr(e1, ffi_vec_get_coeff(u1, i));
      ffi_elt_mul(e1, e1, tmp1);
      ffi_elt_mul(e2, ffi_vec_get_coeff(u0, i), tmp1);
      
      q_polynomial_scalar_mul(qtmp1, N1, e1);
      q_polynomial_qexp(qtmp2, N1);
      q_polynomial_scalar_mul(qtmp3, V1, e1);
      q_polynomial_qexp(qtmp4, V1);

      q_polynomial_scalar_mul(N1, N1, e2);
      q_polynomial_add(N1, N0, N1);

      q_polynomial_scalar_mul(V1, V1, e2);
      q_polynomial_add(V1, V0, V1);
      
      q_polynomial_add(N0, qtmp1, qtmp2);
      q_polynomial_add(V0, qtmp3, qtmp4);

      //cout<<"N0: "<<endl;
      //q_polynomial_print(N0);
      //cout<<"N1: "<<endl;
      //q_polynomial_print(N1);
      //cout<<"V0: "<<endl;
      //q_polynomial_print(V0);
      //cout<<"V1: "<<endl;
      //q_polynomial_print(V1);

    } 

    if(ffi_elt_is_zero(ffi_vec_get_coeff(u0, i)) == 1 && 
       ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) == 1) {
      updateType = 2;
      //cout<<"Updatetype"<<updateType<<endl;
      // N0' = N1^q 
      // V0' = V1^q
      // N1' = N0 
      // V1' = V0 
      
      q_polynomial_qexp(qtmp1, N1);
      q_polynomial_qexp(qtmp2, V1);

      q_polynomial_set(N1, N0);
      q_polynomial_set(V1, V0);
      q_polynomial_set(N0, qtmp1);
      q_polynomial_set(V0, qtmp2);

      //cout<<"N0: "<<endl;
      //q_polynomial_print(N0);
      //cout<<"N1: "<<endl;
      //q_polynomial_print(N1);
      //cout<<"V0: "<<endl;
      //q_polynomial_print(V0);
      //cout<<"V1: "<<endl;
      //q_polynomial_print(V1);
    } 

    //cout<<"Update discrepancies"<<endl;
    // Update discrepancies
    for(unsigned int k = i + 1 ; k < gc.n ; ++k) {
      if(updateType == 1) {
        //cout<<"Updatetype"<<updateType<<endl;
        // u0[k]' = u1[k]^q - e1.u1[k]
        // u1[k]' = u0[k] - e2.u1[k] 
      
        ffi_elt_mul(tmp1, e1, ffi_vec_get_coeff(u1, k));
        ffi_elt_sqr(tmp2, ffi_vec_get_coeff(u1, k));
        ffi_elt_add(tmp1, tmp1, tmp2);

        ffi_elt_mul(tmp2, e2, ffi_vec_get_coeff(u1, k));
        ffi_elt_add(tmp2, tmp2, ffi_vec_get_coeff(u0, k));
        ffi_vec_set_coeff(u1, tmp2, k);

        ffi_vec_set_coeff(u0, tmp1, k);

        //cout<<"u0: "<<endl;
        //ffi_vec_print(u0,gc.n);
        //cout<<"u1: "<<endl;
        //ffi_vec_print(u1,gc.n);
      } 
      
      if(updateType == 2) {
        //cout<<"Updatetype"<<updateType<<endl;
        // u0[k]' = u0[k]
        // u1[k]' = u1[k]^q
        
        ffi_elt_sqr(tmp1, u1.data[k]);
        ffi_vec_set_coeff(u1, tmp1, k);

        //cout<<"u0: "<<endl;
        //ffi_vec_print(u0,gc.n);
        //cout<<"u1: "<<endl;
        //ffi_vec_print(u1,gc.n);
      }
    }

    #ifdef VERBOSE
      printf("\nN0 (%i): ", i); //q_polynomial_print(N0);
      printf("\nN1 (%i): ", i); //q_polynomial_print(N1);
      printf("\nV0 (%i): ", i); //q_polynomial_print(V0);
      printf("\nV1 (%i): ", i); //q_polynomial_print(V1);
    #endif
  }

  //cout<<"插值完成!"<<endl;

  /*  
   *  Step 2: Computing f (qtmp1 variable) using Loidreau's improvement for lower degree polynomials
   */
  //cout<<"A: "<<endl;
  //q_polynomial_print(A);
  //cout<<"V1: "<<endl;
  //q_polynomial_print(V1);
  //cout<<"I: "<<endl;
  //q_polynomial_print(I);
  q_polynomial_mul(qtmp1, N1, A);
  q_polynomial_left_div(qtmp3, qtmp2, qtmp1, V1);
  q_polynomial_add(qtmp1, qtmp3, I);



  /*  
   *  Step 3: Decoding the message as the value of the k first coordinates of f (qtmp1 variable)
   */

  ffi_vec_set(m, qtmp1.values, gc.k);

  #ifdef VERBOSE
    printf("\nquotient: "); //q_polynomial_print(qtmp1);
    printf("\nremainder: "); //q_polynomial_print(qtmp2);
    printf("\nmu: "); //ffi_vec_print(m, PARAM_K);
    printf("\n# Gabidulin Decoding - End #\n");
  #endif

  ffi_vec_clear(&u0);
  ffi_vec_clear(&u1);
  q_polynomial_clear(&A);
  q_polynomial_clear(&I);
  q_polynomial_clear(&N0);
  q_polynomial_clear(&N1);
  q_polynomial_clear(&V0);
  q_polynomial_clear(&V1);
  q_polynomial_clear(&qtmp1);
  q_polynomial_clear(&qtmp2);
  q_polynomial_clear(&qtmp3);
  q_polynomial_clear(&qtmp4);
#undef y
#undef gc
#undef m
}


// void gabidulin_code_decode(ffi_vec& m, gabidulin_code gc, const ffi_vec& y) {

//   /*  
//    *  Step 1: Solving the q-polynomial reconstruction2 problem 
//    */

//   int t = (gc.n - gc.k) / 2;
//   int max_degree_N = (gc.n - gc.k) % 2 == 0 ? gc.k + t - 1 : gc.k + t;
//   //max_degree_N+=32;
//   q_polynomial A = q_polynomial_init(gc.k);
//   q_polynomial I = q_polynomial_init(gc.k - 1);

//   q_polynomial N0 = q_polynomial_init(max_degree_N);
//   q_polynomial N1 = q_polynomial_init(max_degree_N);
//   q_polynomial V0 = q_polynomial_init(t);
//   q_polynomial V1 = q_polynomial_init(t);

//   q_polynomial qtmp1 = q_polynomial_init(max_degree_N);
//   q_polynomial qtmp2 = q_polynomial_init(max_degree_N);
//   q_polynomial qtmp3 = q_polynomial_init(t);
//   q_polynomial qtmp4 = q_polynomial_init(t);

//   ffi_vec u0, u1;
//   ffi_elt e1, e2, tmp1, tmp2;


//   // Initialization step
  
//   // A(g[i]) = 0 for 0 <= i <= k - 1
//   // I(g[i]) = y[i] for 0 <= i <= k - 1
  
//   q_polynomial_set_interpolate_vect_and_zero(A, I, gc.g, y, gc.k);
//   q_polynomial_set_one(N0);
//   q_polynomial_set_zero(N1);
//   q_polynomial_set_zero(V0);
//   q_polynomial_set_one(V1);

//   q_polynomial_set_zero(qtmp1);
//   q_polynomial_set_zero(qtmp2);
//   q_polynomial_set_zero(qtmp3);
//   q_polynomial_set_zero(qtmp4);

//   // u0[i] = A(g[i]) - V0(y[i])
//   // u1[i] = I(g[i]) - V1(y[i])

//   for(unsigned int i = 0 ; i < gc.n ; ++i) {
//     q_polynomial_evaluate(tmp1, A, ffi_vec_get_coeff(gc.g, i));
//     q_polynomial_evaluate(tmp2, V0, ffi_vec_get_coeff(y, i));
//     ffi_elt_add(tmp1, tmp1, tmp2);
//     ffi_vec_set_coeff(u0, tmp1, i);

//     q_polynomial_evaluate(tmp1, I, ffi_vec_get_coeff(gc.g, i));
//     q_polynomial_evaluate(tmp2, V1, ffi_vec_get_coeff(y, i));
//     ffi_elt_add(tmp1, tmp1, tmp2);
//     ffi_vec_set_coeff(u1, tmp1, i);
//   }

//   #ifdef VERBOSE
//     printf("\n\n# Gabidulin Decoding - Begin #");
//     printf("\n\ng: "); //ffi_vec_print(gc.g, PARAM_N);
//     printf("\nA: "); //q_polynomial_print(A);
//     printf("\nI: "); //q_polynomial_print(I);
//     printf("\nN0 (init): "); //q_polynomial_print(N0);
//     printf("\nN1 (init): "); //q_polynomial_print(N1);
//     printf("\nV0 (init): "); //q_polynomial_print(V0);
//     printf("\nV1 (init): "); //q_polynomial_print(V1);
//   #endif


//   // Interpolation step 
//   int updateType = -1;
//   for(unsigned int i = gc.k ; i < gc.n ; ++i) {

//     unsigned int j = i;
//     while(ffi_elt_is_zero(ffi_vec_get_coeff(u0, j)) == 0 && 
//           ffi_elt_is_zero(ffi_vec_get_coeff(u1, j)) == 1 &&
//           j < gc.n) j++;
    
//     if(j == gc.n) {
//       break;
//     } else {
//       if(i != j) {
//         // Permutation of the coordinates of positions i and j
//         ffi_elt_set(tmp1, ffi_vec_get_coeff(u0, i));
//         ffi_vec_set_coeff(u0, ffi_vec_get_coeff(u0, j), i);
//         ffi_vec_set_coeff(u0, tmp1, j);

//         ffi_elt_set(tmp1, ffi_vec_get_coeff(u1, i));
//         ffi_vec_set_coeff(u1, ffi_vec_get_coeff(u1, j), i);
//         ffi_vec_set_coeff(u1, tmp1, j);
//       }
//     }


//     // Update q_polynomials according to discrepancies
//     if(ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) != 1) {
//       updateType = 1;

//       // e1 = - u1[i]^q / u1[i] 
//       // e2 = - u0[i] / u1[i]
//       // N0' = N1^q - e1.N1
//       // V0' = V1^q - e1.V1
//       // N1' = N0 - e2.N1 
//       // V1' = V0 - e2.V1
      
//       ffi_elt_inv(tmp1, ffi_vec_get_coeff(u1, i));
//       ffi_elt_sqr(e1, ffi_vec_get_coeff(u1, i));
//       ffi_elt_mul(e1, e1, tmp1);
//       ffi_elt_mul(e2, ffi_vec_get_coeff(u0, i), tmp1);
      
//       q_polynomial_scalar_mul(qtmp1, N1, e1);
//       q_polynomial_qexp(qtmp2, N1);
//       q_polynomial_scalar_mul(qtmp3, V1, e1);
//       q_polynomial_qexp(qtmp4, V1);

//       q_polynomial_scalar_mul(N1, N1, e2);
//       q_polynomial_add(N1, N0, N1);

//       q_polynomial_scalar_mul(V1, V1, e2);
//       q_polynomial_add(V1, V0, V1);
      
//       q_polynomial_add(N0, qtmp1, qtmp2);
//       q_polynomial_add(V0, qtmp3, qtmp4);
//     } 

//     if(ffi_elt_is_zero(ffi_vec_get_coeff(u0, i)) == 1 && 
//        ffi_elt_is_zero(ffi_vec_get_coeff(u1, i)) == 1) {
//       updateType = 2;

//       // N0' = N1^q 
//       // V0' = V1^q
//       // N1' = N0 
//       // V1' = V0 
      
//       q_polynomial_qexp(qtmp1, N1);
//       q_polynomial_qexp(qtmp2, V1);

//       q_polynomial_set(N1, N0);
//       q_polynomial_set(V1, V0);
//       q_polynomial_set(N0, qtmp1);
//       q_polynomial_set(V0, qtmp2);
//     } 


//     // Update discrepancies
//     for(unsigned int k = i + 1 ; k < gc.n ; ++k) {
//       if(updateType == 1) {

//         // u0[k]' = u1[k]^q - e1.u1[k]
//         // u1[k]' = u0[k] - e2.u1[k] 
      
//         ffi_elt_mul(tmp1, e1, ffi_vec_get_coeff(u1, k));
//         ffi_elt_sqr(tmp2, ffi_vec_get_coeff(u1, k));
//         ffi_elt_add(tmp1, tmp1, tmp2);

//         ffi_elt_mul(tmp2, e2, ffi_vec_get_coeff(u1, k));
//         ffi_elt_add(tmp2, tmp2, ffi_vec_get_coeff(u0, k));
//         ffi_vec_set_coeff(u1, tmp2, k);

//         ffi_vec_set_coeff(u0, tmp1, k);
//       } 
      
//       if(updateType == 2) {

//         // u0[k]' = u0[k]
//         // u1[k]' = u1[k]^q
        
//         ffi_elt_sqr(tmp1, u1[k]);
//         ffi_vec_set_coeff(u1, tmp1, k);
//       }
//     }

//     #ifdef VERBOSE
//       printf("\nN0 (%i): ", i); //q_polynomial_print(N0);
//       printf("\nN1 (%i): ", i); //q_polynomial_print(N1);
//       printf("\nV0 (%i): ", i); //q_polynomial_print(V0);
//       printf("\nV1 (%i): ", i); //q_polynomial_print(V1);
//     #endif
//   }



//   /*  
//    *  Step 2: Computing f (qtmp1 variable) using Loidreau's improvement for lower degree polynomials
//    */
//   //P=A*N_1/V_1+I
//   q_polynomial_mul(qtmp1, N1, A);
//   q_polynomial_left_div(qtmp3, qtmp2, qtmp1, V1);
//   //cout<<"gabidulin_remainder"<<endl;
//   //q_polynomial_print(qtmp3);
//   //cout<<"gabidulin_euotient"<<endl;
//   //q_polynomial_print(qtmp2);
//   q_polynomial_add(qtmp1, qtmp3, I);

//   // //A*N_1+V_1*I
//   // q_polynomial_mul(qtmp1, N1, A);//max_degree
//   // q_polynomial_mul(qtmp3, V1, I);//t
//   // q_polynomial_add(qtmp2, qtmp1, qtmp3);//max_degree
//   // //V_1*V_2
//   // q_polynomial_mul(qtmp1, V1, V2);
//   // q_polynomial_left_div(qtmp3,qtmp4,qtmp1,qtmp2)
//   //f=A*N_1+w_1*I/V_1*V_2
  
  
//   /*  
//    *  Step 3: Decoding the message as the value of the k first coordinates of f (qtmp1 variable)
//    */

//   ffi_vec_set(m, qtmp1.values, gc.k);
//   #ifdef VERBOSE
//     printf("\nquotient: "); //q_polynomial_print(qtmp1);
//     printf("\nremainder: "); //q_polynomial_print(qtmp2);
//     printf("\nmu: "); //ffi_vec_print(m, PARAM_K);
//     printf("\n# Gabidulin Decoding - End #\n");
//   #endif
// }
