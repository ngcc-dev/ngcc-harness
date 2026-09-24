#include "Fql.h" 

#if TSUOV_q == 31
  // inv i in fq
  Fq Fq_inv_table[TSUOV_q] = {
     0, 1,16,21, 8,25,26, 9,
     4, 7,28,17,13,12,20,29,
     2,11,19,18,14, 3,24,27,
    22, 5, 6,23,10,15,30
  } ; 
#else
#  error "Unsupported: TSUOV_q = " # TSUOV_q
#endif
